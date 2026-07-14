#ifndef __FS_CTRL_H__
#define __FS_CTRL_H__

#include "sensminiCfg.h"

#define CAPACITY_32GB	0x04000000
#define CAPACITY_2GB	0x00400000

#define CLUSTER_16K		16384
#define CLUSTER_32K		32768

#define READ_FILE_FAIL	-19

extern FATFS fatfsTf;	//sd/TF

extern FRESULT ff_getFileLength(enum FS_VOLUME fsVolume, char *filename, int *length);
extern int readFile(enum FS_VOLUME fsVolume, char *filename, unsigned char *readBuf, int length, int offset);
extern int writeFile(enum FS_VOLUME fsVolume, char *filename, unsigned char *writeBuf, int length, int overrideWrite);
extern FRESULT ff_deleteFile(enum FS_VOLUME fsVolume, char *filename);
extern FRESULT mountFs(enum FS_VOLUME fsVolume, int formatWithNoFileSystem);
extern void nameToAbsolutePath(enum FS_VOLUME fsVolume, char *filename, char *path);
extern FRESULT renameFile(enum FS_VOLUME fsVolume, char *oldFilename, char *newFilename);
extern FRESULT formatSD(enum FS_VOLUME fsVolume);

extern const char *cVolumeStr[2];


#endif
