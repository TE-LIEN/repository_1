#ifndef __LITTLE_FS_PORT_H__
#define __LITTLE_FS_PORT_H__

//#include "global.h"
#include "sensminiCfg.h"
#include "lfs.h"

extern lfs_t lfsNormFs;


extern void littleFsInit(void);
//extern int lfsFileSeek(lfs_file_t *filePtr, int offset, int flags);
//extern int lfsFileWrite(lfs_file_t *filePtr, uint8_t *buffer, uint32_t length, uint32_t *writeLength);
//extern int lfsFileRead(lfs_file_t *filePtr, uint8_t *buffer, uint32_t length, uint32_t *readLength);
//extern int lfsFileClose(lfs_file_t *filePtr);
//extern lfs_file_t *lfsFileOpen(char *filename, char *attribute, int *error);

#define SENS_SPI_FS_WRITE_FILE(a, b, c, d, e)		spiWriteFile(&lfsNormFs, a, b, c, d, e)
#define SENS_SPI_FS_FILE_EXIST(a)								spiFileExist(&lfsNormFs, a)
#define SENS_SPI_FS_FILE_DELETE(a)							spiFileDelete(&lfsNormFs, a)
#define SENS_SPI_FS_GET_FILE_SIZE(a)						spiGetFileSize(&lfsNormFs, a)
#define SENS_SPI_FS_FILE_RENAME(a, b)						spiFileRename(&lfsNormFs, a, b)
#define SENS_SPI_FS_READ_FILE(a, b, c, d, e)		spiReadFile(&lfsNormFs, a, b, c, d, e)
//#define SENS_SPI_FS_DIR_EXIST(a)								spiDirExist(&lfsNormFs, a)
#define SENS_SPI_FS_CREATE_DIR(a)								spiCreateDir(&lfsNormFs, a)
//#define SENS_SPI_FS_OPEN(a)											lfsFileClose(&lfsNormFs, a, "");
//#define SENS_SPI_FS_CLOSE(a)										lfsFileClose()

extern int spiWriteFile(lfs_t *lfsFs, char *filename, uint8_t *buffer, uint32_t length, int offset, int mode);
extern int spiFileExist(lfs_t *lfsFs, char *filename);
extern int spiFileDelete(lfs_t *lfsFs, char *filename);
extern int spiGetFileSize(lfs_t *lfsFs, char *filename);
extern int spiFileRename(lfs_t *lfsFs, char *oldName, char *newName);
extern int spiReadFile(lfs_t *lfsFs, char *filename, char *buf, int length, int offset, char mode);
//extern int spiDirExist(lfs_t *lfsFs, char *dirPath);
extern int spiCreateDir(lfs_t *lfsFs, char *dirPath);

#define SPI_FILE_IS_EXIST				LFS_ERR_OK
//#define SPI_FILE_IS_NOT_EXIST			//other

#endif
