#ifndef __FSA_FS_H__
#define __FSA_FS_H__

#include "sensminiCfg.h"

enum FS_VOLUME
{	FS_TF,
	FS_MAX
};

#define MSD_TF "0:"

#define MAX_FILE_PATH_LEN 64

#define SENS_FILE			FIL
#define SENS_FILE_PTR		FIL *

#define SENS_FILE_OPEN									FSA_FILE_OPEN
#define SENS_FILE_CLOSE									FSA_FILE_CLOSE
//#define SENS_FILE_SEEK(a, b, c)					f_lseek(a, b)
#define SENS_FILE_SEEK									FSA_FILE_SEEK
#define SENS_FILE_READ									FSA_FILE_READ
#define SENS_FILE_WRITE									FSA_FILE_WRITE
#define SENS_FILE_TELL									f_tell
#define SENS_FILE_GET									f_gets
#define SENS_FILE_DELETE								FSA_FILE_DELETE
#define SENS_FILE_DEL_ALL_FILE_IN_DIR		FSA_FILE_DEL_ALL_FILE_IN_DIR
#define SENS_FILE_DIR_EXIST							FSA_FILE_DIR_EXIST
#define SENS_FILE_DIR_CREATE						FSA_FILE_DIR_CREATE

extern SENS_FILE *FSA_FILE_OPEN(enum FS_VOLUME fsVolume, char *filename, char *attribute, int *error);
extern int FSA_FILE_CLOSE(SENS_FILE *filePtr);
extern int FSA_FILE_READ(SENS_FILE *filePtr, uint8_t *readBuf, int length, int *readLength);
extern int FSA_FILE_WRITE(SENS_FILE *filePtr, uint8_t *writeBuf, int length, int *writeLength);
extern int FSA_FILE_DELETE(enum FS_VOLUME fsVolume, char *filename);
extern int FSA_FILE_DEL_ALL_FILE_IN_DIR(enum FS_VOLUME fsVolume, char *dirPath, char removeDir);
extern int FSA_FILE_DIR_EXIST(char *dirPath);
extern int FSA_FILE_DIR_CREATE(char *dirPath);

extern int FSA_FILE_SEEK(FIL* fp, int offset, int flag);

#define OVER_WRITE_MODE			0
#define CONTINUE_WRITE_MODE	1
#define WRITE_NAN_DATA_MODE	2

#define READ_LENGTH_MODE	0
#define NORMAL_READ_MODE	1
#define BIN_READ_MODE			2
#define BIN_READ_MODE2		3

#endif
