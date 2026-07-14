#include "littleFs/littleFsPort.h"
#include "driver/spiNorFlash.h"
#include "sensCommon.h"
#include "sensSystem.h"

#if defined SENSMINIA4_PLUS || defined SENSMINIA4
#include "APP/FSL/AppSpiNorFlash.h"
#endif

#ifdef SPI_FILE_SYSTEM

lfs_t lfsNormFs;
//lfs_file_t lfsFileW25qxx;

int spiBlockRead(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int spiBlockWrite(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int spiBlockErase(const struct lfs_config *c, lfs_block_t block);
int spiBlockSync(const struct lfs_config *c);
#ifdef LFS_THREADSAFE
int lfsLock(const struct lfs_config *c);
int lfsUnLock(const struct lfs_config *c);
#endif

typedef struct _FS_PARTITION
{	char partitionName[8];
	uint32_t start;
}FS_PARTITION;

FS_PARTITION normalFsPartition;

struct lfs_config lfsConfig;


int spiBlockRead(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{	uint32_t start = 0;
	FS_PARTITION *partition;
	
	if(c->context != NULL)
	{	partition = (FS_PARTITION *)c->context;
		start = partition->start;
	}
	memoryReadData(start+c->block_size*block+off, size, (uint8_t *)buffer);
	return LFS_ERR_OK;
}

int spiBlockWrite(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{	uint32_t start = 0;
	FS_PARTITION *partition;
	
	if(c->context != NULL)
	{	partition = (FS_PARTITION *)c->context;
		start = partition->start;
	}
	memoryWriteData(start+c->block_size*block+off, size, (uint8_t *)buffer);
	return LFS_ERR_OK;
}
int spiBlockErase(const struct lfs_config *c, lfs_block_t block)
{	uint32_t start = 0;
	FS_PARTITION *partition;
	
	if(c->context != NULL)
	{	partition = (FS_PARTITION *)c->context;
		start = partition->start;
	}
	memorySectorErase(start+c->block_size*block);
	return LFS_ERR_OK;
}
int spiBlockSync(const struct lfs_config *c)
{	return LFS_ERR_OK;
}

#ifdef LFS_THREADSAFE
int lfsLock(const struct lfs_config *c)
{	(void)c;
	spiFsLock();
	return 0;
}

int lfsUnLock(const struct lfs_config *c)
{	(void)c;
	spiFsUnLock();
	return 0;
}
#endif

lfs_file_t *lfsFileOpen(lfs_t *lfsFs, char *filename, char *attribute, int *error)
{	int flags;
	lfs_file_t *filePtr;
	
	if(!strcmp(attribute, "rb") || !strcmp(attribute, "r+") || !strcmp(attribute, "r"))
		flags = LFS_O_RDWR;
	else if(!strcmp(attribute, "a"))
		flags = LFS_O_CREAT | LFS_O_RDWR;
	else if(!strcmp(attribute, "w"))
		flags = LFS_O_RDWR;
	
	filePtr = SENS_MEM_ZALLOC(sizeof(lfs_file_t));
	*error = lfs_file_open(lfsFs, filePtr, filename, flags);
	if(*error != LFS_ERR_OK)
	{	SENS_MEM_FREE(filePtr);
		filePtr = NULL;
	}
	return filePtr;
}

int lfsFileClose(lfs_t *lfsFs, lfs_file_t *filePtr)
{	int ret;
	ret = lfs_file_close(lfsFs, filePtr);
	SENS_MEM_FREE(filePtr);
	return ret;
}

int lfsFileRead(lfs_t *lfsFs, lfs_file_t *filePtr, uint8_t *buffer, uint32_t length, uint32_t *readLength)
{	uint32_t len;
	len = lfs_file_read(lfsFs, filePtr, buffer, length);
	if(readLength != NULL)
	{	*readLength = len;
	}
	return 0;
}

int lfsFileWrite(lfs_t *lfsFs, lfs_file_t *filePtr, uint8_t *buffer, uint32_t length, uint32_t *writeLength)
{	uint32_t len;
	len = lfs_file_write(lfsFs, filePtr, buffer, length);
	if(writeLength != NULL)
	{	*writeLength = len;
	}
	return 0;
}

int lfsFileSeek(lfs_t *lfsFs, lfs_file_t *filePtr, int offset, int flags)
{	if(flags == IO_SEEK_SET)
		flags = LFS_SEEK_SET;
	else if(flags == IO_SEEK_CUR)
    flags = LFS_SEEK_CUR;
	else if(flags == IO_SEEK_END)
    flags = LFS_SEEK_END;
	return lfs_file_seek(lfsFs, filePtr, offset, flags);
}

int spiWriteFile(lfs_t *lfsFs, char *filename, uint8_t *buffer, uint32_t length, int offset, int mode)
{	lfs_file_t *filePtr;
	char attribute[3];
	int error;
	uint32_t writeLen;
	memset(attribute, 0, 3);
	
#ifdef ONLY_ONE_STORAGE
	int chunkSize = 8192;
	int remainSize = length;
	int bytesWritten, writeError=0;
#endif
	
	attribute[0] = 'a';
	if(mode == CONTINUE_WRITE_MODE)
		attribute[0] = 'w';
	
#ifdef ONLY_ONE_STORAGE
	if(mode == WRITE_NAN_DATA_MODE)
	{	if(buffer == NULL)
		{	buffer = SENS_MEM_ALLOC(chunkSize);
			memset(buffer, 0xFF, chunkSize);
		}
	}
#endif
	filePtr = lfsFileOpen(lfsFs, filename, attribute, &error);
	if(filePtr == NULL)
		return -1;
	
#ifdef ONLY_ONE_STORAGE
	if(mode == WRITE_NAN_DATA_MODE)
	{	while(remainSize)
		{	if(remainSize < chunkSize)
			{	chunkSize = remainSize;
			}
			lfsFileWrite(filePtr, buffer, chunkSize, &writeLen);
			if(writeLen != chunkSize)
			{	writeError = 1;
				break;
			}
			remainSize -= chunkSize;
		}
		lfsFileClose(filePtr);
		if(writeError)
		{	return -1;
		}
		return length;
	}
	else
#endif
	 if(mode == CONTINUE_WRITE_MODE)
		offset = lfsFileSeek(lfsFs, filePtr, 0, IO_SEEK_END);
	else
		offset = lfsFileSeek(lfsFs, filePtr, offset, IO_SEEK_SET);
	lfsFileWrite(lfsFs, filePtr, buffer, length, &writeLen);
	lfsFileClose(lfsFs, filePtr);
	
	if(writeLen != length)
	{	return -3;
	}
	return writeLen;
}

/*int spiOpenFile(lfs_t *lfsFs, char *filename)
{
}*/

int spiReadFile(lfs_t *lfsFs, char *filename, char *buf, int length, int offset, char mode)
{	lfs_file_t *filePtr;
	char attribute[3];
	int error;
	uint32_t readLen;
	memset(attribute, 0, 3);
	
	attribute[0] = 'r';
	if(mode == BIN_READ_MODE)
		attribute[1] = 'b';
	else if(mode == BIN_READ_MODE2)
		attribute[1] = '+';
	
	filePtr = lfsFileOpen(lfsFs, filename, attribute, &error);
	if(filePtr == NULL)
		return -1;
	lfsFileSeek(lfsFs, filePtr, offset, IO_SEEK_SET);
	
	error = lfsFileRead(lfsFs, filePtr, (uint8_t *)buf, length, &readLen);
	lfsFileClose(lfsFs, filePtr);
	if(readLen != length)
	{	return -3;
	}
	return readLen;
}

int spiFileExist(lfs_t *lfsFs, char *filename)
{	lfs_file_t *filePtr;
	int error = LFS_ERR_OK;
	
	filePtr = lfsFileOpen(lfsFs, filename, "r", &error);
	if(filePtr == NULL)
		return -1;
	
	lfsFileClose(lfsFs, filePtr);
	return error;
}

int spiDirExist(lfs_t *lfsFs, char *dirPath)
{	lfs_dir_t *dir;
	int openError;
	
	dir = SENS_MEM_ZALLOC(sizeof(lfs_dir_t));
	openError = lfs_dir_open(lfsFs, dir, dirPath);
	if(openError == LFS_ERR_OK)
	{	openError = lfs_dir_close(lfsFs, dir);
	}
	SENS_MEM_FREE(dir);
	return openError;
}

int spiCreateDir(lfs_t *lfsFs, char *dirPath)
{	int err;
	if(spiDirExist(lfsFs, dirPath) == LFS_ERR_OK)
		return 0;
	//no dir, create one
	err = lfs_mkdir(lfsFs, dirPath);
	return err;
}

int spiFileDelete(lfs_t *lfsFs, char *filename)
{	int err;
	
	err = lfs_remove(lfsFs, filename);
	return err;
}

int spiFileRename(lfs_t *lfsFs, char *oldName, char *newName)
{	int err;
	err = lfs_rename(lfsFs, oldName, newName);
	return err;
}

int spiGetFileSize(lfs_t *lfsFs, char *filename)
{	lfs_file_t *filePtr;
	int error = LFS_ERR_OK;
	int length;
	
	filePtr = lfsFileOpen(lfsFs, filename, "r", &error);
	if(filePtr == NULL)
		return -1;
	length = lfs_file_size(lfsFs, filePtr);
	lfsFileClose(lfsFs, filePtr);
	return length;
}

/*void littleFsDeinit(void)
{	
}*/

void littleFsInit(void)
{	uint32_t mid;
	uint32_t totSize;
	
	openSpiFlash((uint32_t *)&mid);
	memorySetProtection(0);
	SENS_TIME_DELAY(100);
	
	mid &= 0xFF;
	totSize = 0x10000;
	if((mid >= SPI_CAPACITY_1Mb) && (mid <= SPI_CAPACITY_512Mb))
	{	totSize = 0x20000;
		mid -= SPI_CAPACITY_1Mb;
		totSize <<= mid;
	}
	else if(mid >= SPI_CAPACITY_1Gb)
	{	totSize = 0x8000000;
		mid -= SPI_CAPACITY_1Gb;
		totSize <<= mid;
	}
	
	//totSize >>= 1;	//reserved for old spi ota use
		
	memset(&lfsConfig, 0, sizeof(struct lfs_config));
	memset(&normalFsPartition, 0, sizeof(FS_PARTITION));
	strcpy(&normalFsPartition.partitionName[0], "Cfgs");
	normalFsPartition.start = totSize / 2;
	lfsConfig.context = (void *)&normalFsPartition;
	
	lfsConfig.read = spiBlockRead;
	lfsConfig.prog = spiBlockWrite;
	lfsConfig.erase = spiBlockErase;
	lfsConfig.sync = spiBlockSync;
#ifdef LFS_THREADSAFE
	lfsConfig.lock = lfsLock;
	lfsConfig.unlock = lfsUnLock;
#endif
	lfsConfig.read_size = 16;
	lfsConfig.prog_size = 16;
	lfsConfig.block_size = SPI_SECTOR_SIZE;
#if defined SPI_DUAL_PARTITION
	lfsConfig.block_count = totSize / SPI_SECTOR_SIZE / 2;	//reserved for old spi ota use
#else
	lfsConfig.block_count = totSize / SPI_SECTOR_SIZE / 2;	//use half capacity
#endif
	lfsConfig.cache_size = 16;
	lfsConfig.lookahead_size = 64;
	lfsConfig.block_cycles = 500;
	
	int err = lfs_mount(&lfsNormFs, &lfsConfig);
	dbgMsg("mount lfs status %d\r\n", err);
	if(err)
	{	dbgMsg("%s", "mount lfs fail, run format!!\r\n");
		lfs_format(&lfsNormFs, &lfsConfig);
		err = lfs_mount(&lfsNormFs, &lfsConfig);
		dbgMsg("mount lfs status %d\r\n", err);
	}
	
	if(!err)
	{	sysCtrl->bakupFlashIsPresent = 1;
	}
}

#endif


