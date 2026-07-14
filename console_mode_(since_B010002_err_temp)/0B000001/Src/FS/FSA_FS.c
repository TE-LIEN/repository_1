#include "sensminiCfg.h"
//#include "FS/FSA_FS.h"
//#include "SDCard_task.h"
#include "FS/fsCtrl.h"
#include "string.h"
#include "sensCommon.h"
#include "sensSystem.h"


SENS_FILE *FSA_FILE_OPEN(enum FS_VOLUME fsVolume, char *filename, char *attribute, int *error)
{	//int rc=0;
	char path[MAX_FILE_PATH_LEN];
	SENS_FILE *filePtr;

	nameToAbsolutePath(fsVolume, filename, path);	
	char mode;
	char *pos;
	char *nextPos;
	char dir[MAX_FILE_PATH_LEN];
	char temp[MAX_FILE_PATH_LEN];
	
	if(!strcmp(attribute, "rb") || !strcmp(attribute, "r+") || !strcmp(attribute, "r"))
		mode = FA_READ;
	else if(!strcmp(attribute, "a"))
		mode = FA_CREATE_ALWAYS | FA_WRITE;
	else if(!strcmp(attribute, "w"))
		mode = FA_WRITE;
	filePtr = SENS_MEM_ALLOC(sizeof(SENS_FILE));
	
	//dbgMsgFs("open file : %s\r\n", path);
	
OPEN_WRITE_FILE:
	*error = f_open(filePtr, path, mode);
	if((*error != 0) && (mode == FA_WRITE))
	{	if(*error == FR_NO_FILE)
		{	*error = f_open(filePtr, path, FA_CREATE_ALWAYS | FA_WRITE);
		}
		else if(*error == FR_NO_PATH)
		{	pos = strstr(path, "/");
			memset(dir, 0, MAX_FILE_PATH_LEN);
			strcat(dir, cVolumeStr[fsVolume]);
			dir[2] = '\0';
			while(pos)
			{	memset(temp, 0, MAX_FILE_PATH_LEN);
				nextPos = strstr(pos+1, "/");
				if(nextPos)
				{	memcpy(temp, pos+1, nextPos-pos-1);
					strcat(dir, "/");
					strcat(dir, (char *)&temp);
					*error = f_mkdir(dir);
					if(*error != FR_EXIST)
					{	dbgMsgFs("mk dir (%s) error, error:%d\r\n", dir, *error);
					}
				}
				pos = nextPos;
			}
			goto OPEN_WRITE_FILE;
		}
	}		
	return filePtr;
}

int FSA_FILE_CLOSE(SENS_FILE *filePtr)
{
#if defined FS_MFS
	if(filePtr == NULL)
		return 0;
	fclose(filePtr);
#elif defined FS_FFS
	f_close(filePtr);
	if(filePtr)
		SENS_MEM_FREE(filePtr);
#endif
	return 0;
}

int FSA_FILE_READ(SENS_FILE *filePtr, uint8_t *readBuf, int length, int *readLength)
{	
#if defined FS_MFS
	if(readLength == NULL)
		read(filePtr, readBuf, length);
	else
		*readLength = read(filePtr, readBuf, length);
#elif defined FS_FFS
	UINT br;
	if(readLength == NULL)
		f_read(filePtr, readBuf, length, &br);
	else
		f_read(filePtr, readBuf, length, (uint32_t *)readLength);
#endif
	return 0;
}

int FSA_FILE_WRITE(SENS_FILE *filePtr, uint8_t *writeBuf, int length, int *writeLength)
{	
#if defined FS_MFS
	if(writeLength == NULL)
		write(filePtr, writeBuf, length);
	else
		*writeLength = write(filePtr, writeBuf, length);
#elif defined FS_FFS
	UINT br;
	if(writeLength == NULL)
		f_write(filePtr, writeBuf, length, &br);
	else
		f_write(filePtr, writeBuf, length, (uint32_t *)writeLength);
#endif
	return 0;
}

int FSA_FILE_DELETE(enum FS_VOLUME fsVolume, char *filename)
{	
#if defined FS_MFS
	char  path[MAX_FILE_PATH_LEN];
	nameToAbsolutePath(fsVolume, filename, path);
	return ioctl(g_fs, IO_IOCTL_DELETE_FILE, path);
#elif defined FS_FFS
	return ff_deleteFile(fsVolume, filename);
#endif
}

int FSA_FILE_DIR_EXIST(char *dirPath)
{	
#if defined FS_MFS
	return ioctl(g_fs, IO_IOCTL_CHECK_DIR_EXIST, (pointer)dirPath);
#elif defined FS_FFS
	DIR dir;
	FRESULT fr;
	
	fr = f_opendir(&dir, dirPath);
	if(fr == FR_OK)
		f_closedir(&dir);
	return fr;
#endif
}

int FSA_FILE_DIR_CREATE(char *dirPath)
{
#if defined FS_MFS
	return ioctl(g_fs, IO_IOCTL_CREATE_SUBDIR, (pointer)dirPath);
#elif defined FS_FFS
	char path[MAX_FILE_PATH_LEN];
	char *pos;
	char *nextPos;
	char dirname1[MAX_FILE_PATH_LEN];
	char temp[MAX_FILE_PATH_LEN];
	FRESULT rc;
	char headerName[5];
		
	fsLock();
	memset(path, 0, MAX_FILE_PATH_LEN);
	strcat(path, dirPath);
	memset(headerName, 0, 5);
	pos = strstr(dirPath, MSD_TF);
	if(pos)
		strcat(headerName, MSD_TF);
	/*else
	{	pos = strstr(dirPath, MSD_EMMC);
		if(pos)
			strcat(headerName, MSD_EMMC);
	}*/
	
	rc = f_mkdir(path);
	if(rc == FR_NO_PATH)
	{	while(1)
		{	if(rc == FR_OK)
				break;
			pos = strstr(path, "/");
			memset(dirname1, 0, MAX_FILE_PATH_LEN);
			strcat(dirname1, headerName);
			dirname1[2] = '\0';
			while(pos)
			{	memset(temp, 0, MAX_FILE_PATH_LEN);
				nextPos = strstr(pos+1, "/");
				if(nextPos)
				{	memcpy(temp, pos+1, nextPos-pos-1);
					strcat(dirname1, "/");
					strcat(dirname1, (char *)&temp);
					rc = f_mkdir(dirname1);
					dbgMsgFs("mk dir NAME: %s\r\n", dirname1);
					if(rc != FR_EXIST)
					{	dbgMsgFs("mk dir error, rc:%d\r\n", rc);
					}
					else
					{	dbgMsgFs("mk dir success, rc:%d\r\n", rc);
					}
				}
				else
				{	rc = f_mkdir(path);
				}
				pos = nextPos;
			}
		}
	}
	else
	{	if(rc == FR_EXIST)
			rc = 0;
	}
	fsUnlock();
	return rc;
#endif
}

int FSA_FILE_DEL_ALL_FILE_IN_DIR(enum FS_VOLUME fsVolume, char *dirPath, char removeDir)
{	char path[MAX_FILE_PATH_LEN];
#if defined FS_MFS
	MFS_SEARCH_PARAM search;
	MFS_SEARCH_DATA search_data;
	char filePath[MAX_FILE_PATH_LEN];
	char delFilePath[MAX_FILE_PATH_LEN];
	_mqx_int errorCode;

	nameToAbsolutePath(fsVolume, dirPath, path);
	
	if(!FSA_FILE_DIR_EXIST(path))
	{	memset(filePath, 0, MAX_FILE_PATH_LEN);
		strcpy(filePath, path);
		strcat(filePath, "/*");

		search.ATTRIBUTE = MFS_SEARCH_ANY;
		search.WILDCARD = filePath;
		search.SEARCH_DATA_PTR = &search_data;

		errorCode = ioctl(g_fs,IO_IOCTL_FIND_FIRST_FILE,(uint_32_ptr)&search);
		while(errorCode == MFS_NO_ERROR)
		{	if(search_data.NAME[0] != '.')
			{	memset(delFilePath, 0, MAX_FILE_PATH_LEN);
				SENS_SPRINTF(delFilePath, "%s/%s", path, search_data.NAME);
				SENS_FILE_DELETE(FS_TF, delFilePath);
			}
			errorCode = ioctl(g_fs,IO_IOCTL_FIND_NEXT_FILE,(uint_32_ptr)&search_data);
		}
		if(removeDir)
			ioctl(g_fs,IO_IOCTL_REMOVE_SUBDIR,(pointer)path);
	}
	return errorCode;
	
#elif defined FS_FFS
	char delPath[MAX_FILE_PATH_LEN];
	DIR dir;
	FILINFO fno;
	FRESULT fr;
	
	nameToAbsolutePath(fsVolume, dirPath, path);
	fr = f_opendir(&dir, path);
	if(fr != FR_OK)
		return fr;
	
	while(1)
	{	fr = f_readdir(&dir, &fno);
		if(fr != FR_OK || !fno.fname[0])
			break;
		SENS_SPRINTF(delPath, "%s/%s", dirPath, fno.fname);
		if(fno.fattrib == AM_DIR)
		{	fr = FSA_FILE_DEL_ALL_FILE_IN_DIR(fsVolume, delPath, removeDir);
		}
		else
		{	fr = ff_deleteFile(fsVolume, delPath);
		}
		if(fr != FR_OK)
			break;
	}
	
	return fr;
#endif
}

#ifdef FS_FFS
int FSA_FILE_SEEK(FIL* fp, int offset, int flag)
{	if(flag == IO_SEEK_END)
	{	offset = f_size(fp);
	}
	return f_lseek(fp, offset);
}

#endif
