#include <stdio.h>
#include <string.h>
#include <3ds/types.h>
#include <3ds/FS.h>
#include <3ds/svc.h>

#include "installerIcon_bin.h"

#include "filesystem.h"
#include "smdh.h"
#include "utils.h"

FS_archive sdmcArchive;

void initFilesystem(void)
{
	fsInit();
	sdmcArchive=(FS_archive){0x00000009, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
}

void exitFilesystem(void)
{
	FSUSER_CloseArchive(NULL, &sdmcArchive);
	fsExit();
}

int loadFile(char* path, void* dst, FS_archive* archive, u64 maxSize)
{
	if(!path || !dst || !archive)return -1;

	u64 size;
	u32 bytesRead;
	Result ret;
	Handle fileHandle;

	ret=FSUSER_OpenFile(NULL, &fileHandle, *archive, FS_makePath(PATH_CHAR, path), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if(ret!=0)return ret;

	ret=FSFILE_GetSize(fileHandle, &size);
	if(ret!=0)return ret;
	if(size>maxSize)return -2;

	ret=FSFILE_Read(fileHandle, &bytesRead, 0x0, dst, size);
	if(ret!=0)return ret;
	if(bytesRead<size)return -3;

	ret=FSFILE_Close(fileHandle);
	if(ret!=0)return ret;

	return 0;
}

extern int debugValues[4];

void addDirectoryToMenu(menu_s* m, char* path)
{
	static menuEntry_s tmpEntry;
	static smdh_s tmpSmdh;

	static char str[1024];
	snprintf(str, 1024, "%s/icon.bin", path);

	int ret=loadFile(str, &tmpSmdh, &sdmcArchive, sizeof(smdh_s));

	if(!ret)
	{
		initEmptyMenuEntry(&tmpEntry);
		ret=extractSmdhData(&tmpSmdh, tmpEntry.name, tmpEntry.description, tmpEntry.iconData);
	}

	if(ret)initMenuEntry(&tmpEntry, path, "test !", (u8*)installerIcon_bin);

	addMenuEntryCopy(m, &tmpEntry);
}

void scanHomebrewDirectory(menu_s* m, char* path)
{
	if(!path)return;

	Handle dirHandle;
	FS_path dirPath=FS_makePath(PATH_CHAR, path);
	FSUSER_OpenDirectory(NULL, &dirHandle, sdmcArchive, dirPath);
	
	u32 entriesRead=0;
	do
	{
		u16 entryBuffer[512];
		memset(entryBuffer,0,1024);
		FSDIR_Read(dirHandle, &entriesRead, 1, (u16*)entryBuffer);
		if(entriesRead && entryBuffer[0x10E]) //only grab directories
		{
			static char fullPath[1024];
			strncpy(fullPath, path, 1024);
			int n=strlen(fullPath);
			unicodeToChar(&fullPath[n], entryBuffer, 1024-n);
			addDirectoryToMenu(m, fullPath);
		}
	}while(entriesRead);

	svcCloseHandle(dirHandle);
}
