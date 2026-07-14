#include "FS/fsCtrl.h"
#include "string.h"
#include "sensCommon.h"
#include "diskio.h"
#include "sensSystem.h"


FATFS fatfsTf;

const char *cVolumeStr[2] = {MSD_TF, 0};

void nameToAbsolutePath(enum FS_VOLUME fsVolume, char *filename, char *path)
{	char *pos;

	pos = strstr(filename, cVolumeStr[fsVolume]);
	if(pos == NULL)
	{	SENS_SPRINTF(path, "%s%s"EMPTY_CHAR, cVolumeStr[fsVolume], filename);
	}
	else
		SENS_SPRINTF(path, "%s"EMPTY_CHAR, filename);
}

FRESULT ff_getFileLength(enum FS_VOLUME fsVolume, char *filename, int *length)
{	FRESULT rc;
	FIL fil;
	FILINFO fno;
	char path[MAX_FILE_PATH_LEN];

	nameToAbsolutePath(fsVolume, filename, (char *)&path);

	rc = f_open(&fil, path, FA_READ);

	if(rc /*== FR_NO_FILE*/)
	{	dbgMsgFs("%s", "open file fail\r\n");
		*length = 0;
	}
	else if(!rc)
	{	//dbgMsgFs("open success!!\r\n");
		rc = f_stat(path, &fno);
		if(!rc)
		{	*length = fno.fsize;
		}
		rc = f_close(&fil);
	}
	return rc;
}

int readFile(enum FS_VOLUME fsVolume, char *filename, unsigned char *readBuf, int length, int offset)
{	char path[MAX_FILE_PATH_LEN];
	FIL fil;
	FRESULT rc;
	UINT br;

	nameToAbsolutePath(fsVolume, filename, (char *)&path);

	rc = f_open(&fil, path, FA_READ);
	if(rc)
	{	dbgMsgFs("ERROR : f_open returned %d\r\n", rc);
		return (rc*-1);
	}
	rc = f_lseek(&fil, offset);
	if(rc)
	{	dbgMsgFs("ERROR : f_lseek returned %d\r\n", rc);
		rc = f_close(&fil);
		return rc;
	}
	rc = f_read(&fil, (void*)readBuf, length, &br);
	if(rc)
	{	dbgMsgFs("ERROR : f_read returned %d\r\n", rc);
	}
	if(length != br)
	{	dbgMsgFs("ERROR : read length %d, %d\r\n", br, length);
	}
	rc = f_close(&fil);
	return br;
}

int writeFile(enum FS_VOLUME fsVolume, char *filename, unsigned char *writeBuf, int length, int overrideWrite)
{	char path[MAX_FILE_PATH_LEN];
	FIL fil;
	FRESULT rc;
	UINT bw;
	char *pos;
	char *nextPos;
	char dir[MAX_FILE_PATH_LEN];
	char temp[MAX_FILE_PATH_LEN];
	FILINFO fno;
	int offset;

	nameToAbsolutePath(fsVolume, filename, (char *)&path);
OPEN_WRITE_FILE:
	rc = f_open(&fil, path, FA_WRITE);
	if(rc)
	{	if(rc == FR_NO_PATH)
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
					rc = f_mkdir(dir);
					if(rc != FR_EXIST)
					{	dbgMsgFs("mk dir error, rc:%d\r\n", rc);
					}
				}
				pos = nextPos;
			}
			goto OPEN_WRITE_FILE;
		}
		else if(rc == FR_NO_FILE)
		{	rc = f_open(&fil, path, FA_CREATE_ALWAYS | FA_WRITE);
			if(rc)
				dbgMsgFs("%s, ERROR : open file fail, rc:%d\r\n", __func__, rc);
		}
		else
		{	dbgMsgFs("%s, ERROR : open file fail, rc:%d\r\n", __func__, rc);
			return (rc*-1);
		}
	}
	if(overrideWrite)
	{	//rc = f_open(&fil, path, FA_CREATE_ALWAYS | FA_WRITE);
		rc = f_lseek(&fil, 0);
	}
	else
	{	//rc = f_open(&fil, path, FA_WRITE);
		rc = f_stat(path, &fno);
		if(!rc)
		{	offset = fno.fsize;
		}
		rc = f_lseek(&fil, offset);
	}
	rc = f_write(&fil,(void*) writeBuf, length, &bw);

	rc = f_close(&fil);

	return bw;
}

FRESULT ff_deleteFile(enum FS_VOLUME fsVolume, char *filename)
{	char path[128];
	FIL fil;
	FRESULT rc;

	memset(path, 0, 128);
	nameToAbsolutePath(fsVolume, filename, (char *)&path);
	dbgMsgFs("delete file path:%s\r\n", path);
	rc = f_unlink(path);
	if(rc)
	{	dbgMsgFs("ERROR : delete returned %d\r\n", rc);
	}
	return rc;
}

FRESULT renameFile(enum FS_VOLUME fsVolume, char *oldFilename, char *newFilename)
{	char oldPath[64];
	char newPath[64];
	FRESULT rc;

	memset(oldPath, 0, 64);
	memset(newPath, 0, 64);
	nameToAbsolutePath(fsVolume, oldFilename, oldPath);
	nameToAbsolutePath(fsVolume, newFilename, newPath);
	dbgMsgFs("rename file %s to %s\r\n", oldPath, newPath);
	rc = f_rename(oldPath, newPath);
	if(rc)
	{	dbgMsgFs("ERROR : rename returned %d\r\n", rc);
	}
	return rc;
}

FRESULT formatSD(enum FS_VOLUME fsVolume)
{	FRESULT rc;
	uint32_t secCnt;
	uint8_t formatType;

	disk_ioctl(fsVolume, GET_SECTOR_COUNT , &secCnt);
	dbgMsgFs("secCnt %d\r\n", secCnt);

	if(secCnt > CAPACITY_32GB)
		formatType = FM_EXFAT;
	else if(secCnt > CAPACITY_2GB)
		formatType = FM_FAT32;
	else
		formatType = FM_FAT;

	rc = f_mkfs(MSD_TF, formatType, CLUSTER_32K, NULL, 0);
	if(rc)
		dbgMsgFs("ERROR : format returned %d\r\n", rc);
	return rc;
}


FRESULT mountFs(enum FS_VOLUME fsVolume, int formatWithNoFileSystem)
{	FRESULT rc;

	rc = f_mount(&fatfsTf, MSD_TF, 1);
	if(rc)
	{	dbgMsgFs("ERROR : mount returned %d\r\n", rc);
		if(formatWithNoFileSystem && (rc == FR_NO_FILESYSTEM))
		{	rc = formatSD(fsVolume);
		}
	}
	return rc;
}
