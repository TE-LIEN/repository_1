#ifndef __SD_CARD_TASK_H__
#define __SD_CARD_TASK_H__

#include "sensminiCfg.h"


#define SDTASK_NUM_MESSAGES	32	                                        //max queue size

#define sdSendQSize SDTASK_NUM_MESSAGES * SENS_TASKQ_GRANM * sizeof(uint32_t)

extern void senstalkTask(void *param);
//extern QueueHandle_t servCmdProcessQ;

extern int isFileExist(char *filename);
extern int sdInit(void);
extern void sdcardTask(void *param);
extern int sdReadFile(char *fileName, char *buf, int length, int offset, char mode);
extern int sdWriteFile(char *fileName, char *buf, int len, int offset, int mode);
extern int getFileLength(char *fileName);
extern void recordPqToTf(void);
extern void readRecNByte(int datetime[7], int recSec, int pqNum, int bytePerRec, long NByte, char* readBuf, int *readBytes);
extern void clearAllJpegFile(void);
extern int createImgFile(char *filename);
extern void writeLog(char *msg);
extern int sdFileDelete(char *filename);

extern int iniParserFile(char *fileName, struct _MiniFile *mIniFile);
extern int mIniSaveToTf(char *fileName, struct _MiniFile *mIniFile);
extern int writeDiHistory(DI_CHG_CONTEXT *diChgCtx, uint8_t *diBuf);
extern int readDiHistory(int *dateTime, int diNumber, char *readBuf, int readBytes, char mode, int offset);

#define chkFileExist	isFileExist
#define sdDeleteFileNew	sdFileDelete

extern void sdWriteOtaInfo(OTA_REC_CTX *otaRecCtx, char *ackId, char type);
extern OTA_REC_CTX *sdReadOtaInfo(void);

extern void deleteOtaInfoFile(void);

extern void deleteOtaInfoFile(void);
extern void writeLog(char *msg);


//#define SD_RW_OPEN_FILE_FAIL	-1
//#define SD_RW_LENGTH_ERROR		-2

#endif
