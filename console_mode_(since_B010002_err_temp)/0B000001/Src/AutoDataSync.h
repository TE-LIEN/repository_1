#ifndef __AUTO_REPAIR_UPLOAD_DATA_LOST_H__
#define __AUTO_REPAIR_UPLOAD_DATA_LOST_H__

#include "sensminiCfg.h"

#define MAX_REC_HISTORY_INFO_COUNT		100
#define HISTORY_REC_FILENAME_SIZE		24
#define FIXED_REC_INTERVAL_VERSION		0x01
#define FIXED_REC_INTERVAL_VERSION_2	0x02

typedef struct _AUTO_DATA_SYNC_STRUCT
{	struct _AUTO_DATA_SYNC_STRUCT	*next;
	uint8_t *filename;
	uint32_t startSlot;
	int16_t timeInterval;
	uint16_t slotCnt;
	uint8_t isReady;
	uint8_t waitForAck;
	uint8_t recHeaderVer;
	uint32_t isReadyTimer;
	//uint8_t isFinalRetry;
	uint32_t startTimeStamp;
	uint32_t ackSeq;
	//uint32_t delaySendTimer;
}AUTO_DATA_SYNC_STRUCT;

typedef struct _TAG_OF_REC_HEADER
{	uint16_t 	tag;
	uint16_t	ver;
	uint16_t  	recCnt;
	uint16_t	sendCnt;
	int16_t		compareStartSlot;
	int16_t 	checkSendSlot;
	int16_t		endChkSlot;
	uint32_t	currTimestamp;
	uint32_t 	lastRecSlot;
}TAG_OF_REC_HEADER;

typedef struct _REC_HEADER
{	TAG_OF_REC_HEADER headerTag;
	uint8_t		recSlotBmpFlag[180];	//86400/60/8
	uint8_t		sendSlotBmpFlag[180];
	uint8_t		sendSlotBmpFlag1[180];
}REC_HEADER;

/*
* rec in file for check rec data send done or not
*/
typedef struct _REC_FILE_INFO
{	int 			valid;
	uint32_t 	unixTime;
	char 			filename[HISTORY_REC_FILENAME_SIZE];
}REC_FILE_INFO;

typedef struct _HISTORY_REC_HEADER
{	uint16_t tag;
	uint16_t version;
	uint32_t numOfUnChkFile;	//for check file count
	int32_t nextWriteIdx;
	int32_t currFileIdx;
	uint16_t serverPort;
	uint16_t serverType;
	char serverIp[256];
}HISTORY_REC_HEADER;

typedef struct _HISTORY_REC_HEADER_V1
{	uint16_t tag;
	uint16_t version;
	uint32_t numOfUnChkFile;	//for check file count
	int32_t nextWriteIdx;
	int32_t currFileIdx;
}HISTORY_REC_HEADER_V1;

extern int getRecFileVersion(char *filename);
extern void batchRemoveAutoDataSyncInfo(CLOUD_SERVER_INSTANCE *servInst);
extern void addRecordToHistoryFile(char *filename, uint32_t currentTimeStamp, int recIdx);

extern void removeAutoDataSyncInfo	(CLOUD_SERVER_INSTANCE *servInst);
extern void checkRecordFromHistoryFile(CLOUD_SERVER_INSTANCE *servInst, const char *caller, uint32_t line);

extern void removeAutoSendInfo(CLOUD_SERVER_INSTANCE *servInst);
extern void setGlobalAutoSendCtxInfo(char *filename, REC_HEADER *recHeader, uint32_t currentTimestamp, uint32_t currentTimeSlot);

#endif
