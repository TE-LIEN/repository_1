#include "AutoDataSync.h"
#include "sdCardTask.h"
#include "physicalQuantity.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "math.h"
#include "rtcDateTime.h"

#if AUTO_DATA_SYNC
//------------------------------------------------------------------------------
static void restoreRecFileHeader(char *filename, REC_HEADER* recHeader, uint32_t currentTimestamp)
{	uint32_t pqIdx;
	uint8_t *pqBuf;
	float *fPqBufPtr;
	uint8_t *recSlotBmp;
	int chkBmpIdx, chkBmpBitNo;
	int currSlot;
	int fileOffset;
	//int allPqIsNan;
	
	dbg_msg("header damage, File: %s, restore now\r\n", filename);
	recHeader->headerTag.tag = 0x55AA;
	recHeader->headerTag.ver = FIXED_REC_INTERVAL_VERSION_2;
	recHeader->headerTag.recCnt = 0;
	recHeader->headerTag.sendCnt = 0;
	recHeader->headerTag.compareStartSlot = 0;
	recHeader->headerTag.checkSendSlot = 0;
	recHeader->headerTag.endChkSlot = 1;
	recHeader->headerTag.currTimestamp = currentTimestamp;
	recHeader->headerTag.lastRecSlot = 0;
	
	recSlotBmp = (uint8_t *)recHeader->recSlotBmpFlag;
	for(currSlot=0;currSlot<180;currSlot++)
	{	recSlotBmp[currSlot] = 0x00;
		recHeader->sendSlotBmpFlag[currSlot] = 0x00;
		recHeader->sendSlotBmpFlag1[currSlot] = 0x00;
	}
	pqBuf = SENS_MEM_ZALLOC(sensPq->bytesPerRec);
	for(currSlot = 0;currSlot<1440;currSlot++)
	{	fileOffset = sizeof(REC_HEADER) + (currSlot * sensPq->bytesPerRec);
		sdReadFile(filename, (char *)pqBuf, sensPq->bytesPerRec, fileOffset, NORMAL_READ_MODE);
		fPqBufPtr = (float *)pqBuf;
		//allPqIsNan = 1;
		for(pqIdx=0;pqIdx<sensPq->pqNumber;pqIdx++)
		{	if(!isnan(*fPqBufPtr))
			{	//allPqIsNan = 0;
				chkBmpIdx = currSlot / 8;
				chkBmpBitNo = currSlot % 8;
				recSlotBmp[chkBmpIdx] |= (1<<chkBmpBitNo);
				recHeader->headerTag.recCnt++;
				recHeader->headerTag.endChkSlot = currSlot + 1;
				recHeader->headerTag.lastRecSlot = currSlot;
				break;
			}
			fPqBufPtr++;
		}
	}
	
	sdWriteFile(filename, (char *)recHeader, sizeof(REC_HEADER), 0, OVER_WRITE_MODE);
	//while(1);
}
//------------------------------------------------------------------------------
int getRecFileVersion(char *filename)
{	REC_HEADER *recHeader;
  char *recHeaderBuf;
  int recVersion;
  
	recHeaderBuf = SENS_MEM_ZALLOC(sizeof(REC_HEADER));
	sdReadFile((char *)filename, recHeaderBuf, sizeof(REC_HEADER), 0, NORMAL_READ_MODE);
	recHeader = (REC_HEADER *)recHeaderBuf;
	
	recVersion = recHeader->headerTag.ver;
	SENS_MEM_FREE(recHeaderBuf);
	return recVersion;
}
//------------------------------------------------------------------------------
static void removeRecordFromHistoryFile(int recIdx)
{	AUTO_DATA_SYNC_INSTANCE *autoDataSyncInst = (AUTO_DATA_SYNC_INSTANCE *)&sensSys->autoDataSyncInst;
	HISTORY_REC_HEADER *histHeader;
	REC_FILE_INFO *recFileInf;
	int histFileSize;
	char *histBuf;
	char histFilename[32];
	
	memset(histFilename, 0, 32);
	histBuf = (char *)autoDataSyncInst->srvHistoryBuf[recIdx];
	if(recIdx == 0)
		strcat(histFilename, "history.bin");
	else
		strcat(histFilename, "history2.bin");
	//if not buf, mean not check or add, skip
	if(histBuf != NULL)
	{	histFileSize = sizeof(HISTORY_REC_HEADER) + sizeof(REC_FILE_INFO)*MAX_REC_HISTORY_INFO_COUNT;
		histHeader = (HISTORY_REC_HEADER *)histBuf;
		recFileInf = (REC_FILE_INFO *)&histBuf[sizeof(HISTORY_REC_HEADER) + histHeader->currFileIdx*sizeof(REC_FILE_INFO)];
		memset((void *)recFileInf, 0xFF, sizeof(REC_FILE_INFO));
		histHeader->currFileIdx++;
		if(histHeader->currFileIdx >= MAX_REC_HISTORY_INFO_COUNT)
			histHeader->currFileIdx = 0;
		histHeader->numOfUnChkFile--;
		if(!histHeader->numOfUnChkFile)
			histHeader->currFileIdx = -1;
		dbgMsg("remove Hist, next write:%d, currIdx:%d, uncheck :%d\r\n", histHeader->nextWriteIdx, histHeader->currFileIdx, histHeader->numOfUnChkFile);
		sdWriteFile(histFilename, (char *)histBuf, histFileSize, 0, OVER_WRITE_MODE);
	}
	else
	{	dbgMsg("%s", "want to remove history, but no prev action\r\n");
	}
}
//------------------------------------------------------------------------------
void batchRemoveAutoDataSyncInfo(CLOUD_SERVER_INSTANCE *servInst)
{	AUTO_DATA_SYNC_STRUCT *autoDataSyncCtx;
	AUTO_DATA_SYNC_STRUCT *autoDataSyncNext;
	char *recHeaderBuf;
	REC_HEADER *recHeader;
	uint8_t recSizeForV1 = 0;
	uint8_t *sendSlotBmpFlag;
	int idx;
	uint16_t bmpIdx;
	uint16_t startSlot;
	uint8_t bmpBit;
	char *filename;
	uint8_t remove = 1;
	int8_t retryCnt = 0;
	
	SENS_SEM_LOCK(servInst->autoDataSyncCtxLock);
	autoDataSyncCtx = servInst->autoDataSyncCtx;
	
	filename = SENS_MEM_ZALLOC(strlen((char *)autoDataSyncCtx->filename)+1);
	memcpy(filename, autoDataSyncCtx->filename, strlen((char *)autoDataSyncCtx->filename));
	recHeaderBuf = SENS_MEM_ZALLOC(sizeof(REC_HEADER));
RE_READ_FILE:
	sdReadFile(filename, recHeaderBuf, sizeof(REC_HEADER), 0, NORMAL_READ_MODE);
	recHeader = (REC_HEADER *)recHeaderBuf;
	if(recHeader->headerTag.tag == 0x55AA)
	{	sendSlotBmpFlag = (uint8_t *)recHeader->sendSlotBmpFlag;
		if(recHeader->headerTag.ver == FIXED_REC_INTERVAL_VERSION)
			recSizeForV1 = 180;
		else
		{	if(servInst->servIdx == 1)
				sendSlotBmpFlag = (uint8_t *)recHeader->sendSlotBmpFlag1;
		}
	}
	else 
	{	retryCnt++;
		if(retryCnt <= 3)
		{	goto RE_READ_FILE;
		}
	}
	while(autoDataSyncCtx)
	{	autoDataSyncNext = autoDataSyncCtx->next;
		
		if(autoDataSyncCtx->waitForAck)
		{	autoDataSyncCtx->waitForAck = 0;
			startSlot = autoDataSyncCtx->startSlot;
			for(idx=0;idx<autoDataSyncCtx->slotCnt;idx++)
			{	bmpIdx = startSlot / 8;
				bmpBit = startSlot % 8;
				if(recHeader->headerTag.tag == 0x55AA)
					sendSlotBmpFlag[bmpIdx] |= (1 << bmpBit);
			}
		}
		else
			break;
	
		SENS_MEM_FREE(autoDataSyncCtx->filename);
		SENS_MEM_FREE(autoDataSyncCtx);
		autoDataSyncCtx = autoDataSyncNext;
	}
	servInst->autoDataSyncCtx = autoDataSyncCtx;
	SENS_SEM_UNLOCK(servInst->autoDataSyncCtxLock);
	
	if(recHeader->headerTag.tag == 0x55AA)
	{	if(ROUNDDOWN(sysTimer->currentTimeStamp, 86400) > ROUNDDOWN(recHeader->headerTag.currTimestamp, 86400))
		{	for(idx=0;idx<180;idx++)
			{	if(sendSlotBmpFlag[idx] != recHeader->recSlotBmpFlag[idx])
				{	remove = 0;
					break;
				}
			}
			if(remove)
			{	removeRecordFromHistoryFile(servInst->servIdx);
			}
		}
		sdWriteFile(filename, recHeaderBuf, sizeof(REC_HEADER)-recSizeForV1, 0, OVER_WRITE_MODE);
	}
	SENS_MEM_FREE(filename);
	SENS_MEM_FREE(recHeaderBuf);
	SENS_SEM_LOCK(servInst->autoDataSyncCtxLock);
	if(servInst->autoDataSyncCtx == NULL)
	{	SENS_SEM_UNLOCK(servInst->autoDataSyncCtxLock);
		checkRecordFromHistoryFile(servInst, __func__, __LINE__);
	}
	else
		SENS_SEM_UNLOCK(servInst->autoDataSyncCtxLock);
}
//------------------------------------------------------------------------------
void removeAutoDataSyncInfo(CLOUD_SERVER_INSTANCE *servInst)
{	AUTO_DATA_SYNC_STRUCT *autoDataSyncNext;
	AUTO_DATA_SYNC_STRUCT *autoDataSync;
	char *recHeaderBuf;
	REC_HEADER *recHeader;
	int idx;
	uint16_t bmpIdx;
	uint16_t startSlot;
	uint8_t bmpBit;
	uint8_t remove = 1;
	uint8_t *sendSlotBmpFlag;
	uint8_t recSizeForV1 = 0;
	int8_t retryCnt=0;
	
	SENS_SEM_LOCK(servInst->autoDataSyncCtxLock);
	autoDataSync = servInst->autoDataSyncCtx;
	autoDataSyncNext = autoDataSync->next;
	recHeaderBuf = SENS_MEM_ZALLOC(sizeof(REC_HEADER));
RE_READ_FILE:
	sdReadFile((char *)autoDataSync->filename, recHeaderBuf, sizeof(REC_HEADER), 0, NORMAL_READ_MODE);
	recHeader = (REC_HEADER *)recHeaderBuf;
	if(recHeader->headerTag.tag == 0x55AA)
	{	startSlot = autoDataSync->startSlot;
		sendSlotBmpFlag = (uint8_t *)recHeader->sendSlotBmpFlag;
		if(recHeader->headerTag.ver == FIXED_REC_INTERVAL_VERSION)
			recSizeForV1 = 180;
		else
		{		if(servInst->servIdx == 1)
				sendSlotBmpFlag = (uint8_t *)recHeader->sendSlotBmpFlag1;
		}
	
		for(idx=0;idx<autoDataSync->slotCnt;idx++)
		{	bmpIdx = startSlot / 8;
			bmpBit = startSlot % 8;
			sendSlotBmpFlag[bmpIdx] |= (1 << bmpBit);
			if(autoDataSync->timeInterval == -1)
			{	recHeader->headerTag.checkSendSlot = startSlot+1;
			}
			else
			{	startSlot += autoDataSync->timeInterval;
				recHeader->headerTag.checkSendSlot = startSlot;
			}
			recHeader->headerTag.sendCnt++;
		}
		if(ROUNDDOWN(sysTimer->currentTimeStamp, 86400) > ROUNDDOWN(recHeader->headerTag.currTimestamp, 86400))
		{	//current Data Sync day is not today, check all rec is send done?
			for(bmpIdx=0;bmpIdx<180;bmpIdx++)
			{	for(bmpBit=0;bmpBit<8;bmpBit++)
				{	if((recHeader->recSlotBmpFlag[bmpIdx] & (1 << bmpBit)) && ((sendSlotBmpFlag[bmpIdx] & (1 << bmpBit)) == 0))
					{	remove = 0;
						break;
					}
				}
			}
			if(remove)
			{	removeRecordFromHistoryFile(servInst->servIdx);
			}
		}
		else
			remove = 0;
	
		sdWriteFile((char *)autoDataSync->filename, recHeaderBuf, sizeof(REC_HEADER) - recSizeForV1, 0, OVER_WRITE_MODE);
	}
	else
	{	retryCnt++;
		if(retryCnt <= 3)
			goto RE_READ_FILE;
	}
	SENS_MEM_FREE(recHeaderBuf);
	SENS_MEM_FREE(((AUTO_DATA_SYNC_STRUCT *)servInst->autoDataSyncCtx)->filename);
	SENS_MEM_FREE(servInst->autoDataSyncCtx);
	servInst->autoDataSyncCtx = autoDataSyncNext;
	if(servInst->autoDataSyncCtx == NULL)
		dbgMsg("%s", "next auto Data Sync is NULL\r\n");
	SENS_SEM_UNLOCK(servInst->autoDataSyncCtxLock);
	if(remove)	//check next, or next day
	{	dbgMsg("%s", "call checkRecordFromHistoryFile from remove auto Data Sync data\r\n");
		checkRecordFromHistoryFile(servInst, __func__, __LINE__);
	}
}
//------------------------------------------------------------------------------
static void addAutoDataSyncInfo(CLOUD_SERVER_INSTANCE *servInst, AUTO_DATA_SYNC_STRUCT *_autoDataSync)
{	AUTO_DATA_SYNC_STRUCT *autoDataSyncTemp;
	
	if(servInst->autoDataSyncCtx == NULL)
	{	servInst->autoDataSyncCtx = _autoDataSync;
	}
	else
	{	autoDataSyncTemp = servInst->autoDataSyncCtx;
		while(autoDataSyncTemp->next)
		{	autoDataSyncTemp = autoDataSyncTemp->next;
		}
		autoDataSyncTemp->next = _autoDataSync;
	}
}
//------------------------------------------------------------------------------
static void setAutoDataSyncInfo(CLOUD_SERVER_INSTANCE *servInst, char *filename)
{	AUTO_DATA_SYNC_STRUCT *_autoDataSync = NULL;
	uint32_t chkStartSlot;
	uint8_t chkBmpIdx;
	uint8_t chkBmpBit;
	uint32_t slotTimeStamp;
#ifdef NUVOTON
	S_RTC_TIME_DATA_T slotDt;
#else
	DATE_STRUCT slotDt;
	TIME_STRUCT rtcTime;
#endif
	REC_HEADER *recHeader;
	//uint8_t recSizeForV1 = 0;
	uint8_t *sendSlotBmpFlag;
	uint32_t currentTimestamp;
	uint8_t isSameDay = 0;
	char *pqCountPos;
	int recFilePqCnt;
	int noDifferent = 1;
	int skipSlot;
	int fileMonth;
#ifdef NUVOTON
	S_RTC_TIME_DATA_T fileDt;
#else
	DATE_STRUCT fileDate;
	TIME_STRUCT fileRtcTime;
#endif
	
	if(!chkFileExist(filename))
	{	dbgMsg("filename %s is not exist\r\n", filename);
		return;
	}
	
	//pqCountPos = strstr(filename, "n");
	//SENS_SSCANF(pqCountPos, "n%03d.bin", &recFilePqCnt);
#ifdef NUVOTON
	SENS_SSCANF(filename, "%04d/%02d/%02d%02dn%03d.bin", &fileDt.u32Year, &fileMonth, &fileDt.u32Month, &fileDt.u32Day, &recFilePqCnt);
#else
	SENS_SSCANF(filename, "%04d/%02d/%02d%02dn%03d.bin", &fileDate.YEAR, &fileMonth, &fileDate.MONTH, &fileDate.DAY, &recFilePqCnt);
#endif
	
	if(recFilePqCnt != sensPq->pqNumber)
	{	dbgMsg("filename is different Pq Cnt %d, %d\r\n", recFilePqCnt, sensPq->pqNumber);
		removeRecordFromHistoryFile(servInst->servIdx);
		return;
	}
		
	recHeader = (REC_HEADER *)SENS_MEM_ZALLOC(sizeof(REC_HEADER));
	sdReadFile(filename, (char *)recHeader, sizeof(REC_HEADER), 0, NORMAL_READ_MODE);
	SENS_SEM_LOCK(servInst->autoDataSyncCtxLock);
	//currentTimestamp = recHeader->headerTag.currTimestamp;
	
#ifdef NUVOTON
	fileDt.u32Hour = 0;
	fileDt.u32Minute = 0;
	fileDt.u32Second = 0;
	currentTimestamp = dateToTimestamp(&fileDt);
#else
	fileDate.HOUR = 0;
	fileDate.MINUTE = 0;
	fileDate.SECOND = 0;
	fileDate.MILLISEC = 0;
	_rtc_time_from_mqx_date (&fileDate, &fileRtcTime);

	currentTimestamp = fileRtcTime.SECONDS;
#endif
	
	if(recHeader->headerTag.tag != 0x55AA)
	{	restoreRecFileHeader(filename, recHeader, currentTimestamp);
	}
	/*if(currentTimestamp != recHeader->headerTag.currTimestamp)
		recHeader->headerTag.currTimestamp = currentTimestamp;*/
	
	sendSlotBmpFlag = (uint8_t *)recHeader->sendSlotBmpFlag;
	if(recHeader->headerTag.ver == FIXED_REC_INTERVAL_VERSION)
	{	//recSizeForV1 = 180;
	}
	else
	{	if(servInst->servIdx == 1)
			sendSlotBmpFlag = (uint8_t *)recHeader->sendSlotBmpFlag1;
	}
	
	skipSlot = (86400/60) + 10;
	
	if(ROUNDDOWN(sysTimer->currentTimeStamp, 86400) == ROUNDDOWN(currentTimestamp, 86400))
	{	isSameDay = 1;
		skipSlot = sysTimer->currHistoryRecTimeSlot - 6;
	}
	
	recFilePqCnt = 0;
	for(chkBmpIdx=0;chkBmpIdx<180;chkBmpIdx++)
	{	if(recHeader->recSlotBmpFlag[chkBmpIdx] != sendSlotBmpFlag[chkBmpIdx])
		{	for(chkBmpBit=0;chkBmpBit<8;chkBmpBit++)
			{	if((recHeader->recSlotBmpFlag[chkBmpIdx] & (1<<chkBmpBit)) && (recHeader->recSlotBmpFlag[chkBmpIdx] & (1<<chkBmpBit)) != (sendSlotBmpFlag[chkBmpIdx] & (1<<chkBmpBit)))
				{	noDifferent = 0;
					chkStartSlot = chkBmpIdx * 8 + chkBmpBit;
					if((servInst->serverType == SENSTALK_V2) && (chkStartSlot > skipSlot))
					{	dbg_msg("check slot(%d) over skip slot(%d)\r\n", chkStartSlot, skipSlot);
						recFilePqCnt = 50;
						break;
					}
					slotTimeStamp = ROUNDDOWN(currentTimestamp, 86400) + (chkStartSlot * 60);
#ifdef NUVOTON
					timestampToDateTime(slotTimeStamp, &slotDt);
					dbgMsg("rec slot:%d, idx:%d, Bit:%d, time:%04d/%02d/%02d %02d:%02d:%02d\r\n", chkStartSlot, chkBmpIdx, chkBmpBit, slotDt.u32Year, slotDt.u32Month, slotDt.u32Day, slotDt.u32Hour, slotDt.u32Minute, slotDt.u32Second);
#else
					rtcTime.SECONDS = slotTimeStamp;
					rtcTime.MILLISECONDS = 0;
					_rtc_time_to_mqx_date(&rtcTime, &slotDt);
					dbgMsg("rec slot:%d, idx:%d, Bit:%d, time:%04d/%02d/%02d %02d:%02d:%02d\r\n", chkStartSlot, chkBmpIdx, chkBmpBit, slotDt.YEAR, slotDt.MONTH, slotDt.DAY, slotDt.HOUR, slotDt.MINUTE, slotDt.SECOND);
#endif
					_autoDataSync = SENS_MEM_ZALLOC(sizeof(AUTO_DATA_SYNC_STRUCT));
					_autoDataSync->recHeaderVer = recHeader->headerTag.ver;
					_autoDataSync->timeInterval = -1;
					_autoDataSync->startSlot = chkStartSlot;
					_autoDataSync->slotCnt = 1;
					_autoDataSync->startTimeStamp = ROUNDDOWN(currentTimestamp, 86400);// + (slotNum * 60);
					_autoDataSync->filename = SENS_MEM_ZALLOC(strlen(filename)+1);
					memcpy(_autoDataSync->filename, filename, strlen(filename));
					dbgMsg("rec one Data Sync, timeSpan:%d, cnt:%d, start slot:%d\r\n", _autoDataSync->timeInterval, _autoDataSync->slotCnt, _autoDataSync->startSlot);
					addAutoDataSyncInfo(servInst, _autoDataSync);
					recFilePqCnt++;
				}
				if(recFilePqCnt > 50)
					break;
			}
		}
		if(recFilePqCnt > 50)
			break;
	}
	SENS_SEM_UNLOCK(servInst->autoDataSyncCtxLock);
	if(noDifferent)
	{	if(ROUNDDOWN(sysTimer->currentTimeStamp, 86400) != ROUNDDOWN(recHeader->headerTag.currTimestamp, 86400))	//different day, remove
		{	removeRecordFromHistoryFile(servInst->servIdx);
			checkRecordFromHistoryFile(servInst, __func__, __LINE__);
		}
		//goto CHECK_REMOVE_FILE;
	}
}
//------------------------------------------------------------------------------
static void getHistoryRecFileBuf(int recIdx)
{	int histFileSize;
	HISTORY_REC_HEADER *histHeader;
	//HISTORY_REC_HEADER_V1	*histHeaderV1;
	int idx;
	uint8_t *srvHistoryBuf;
	char histFilename[32];
	char *v1TempBuf;
	int fileLength;
	int start, end;
	CLOUD_SERVER_INFO *servInfo;
	AUTO_DATA_SYNC_INSTANCE *autoDataSyncInst = (AUTO_DATA_SYNC_INSTANCE *)&sensSys->autoDataSyncInst;
	
	if(recIdx == 0)			{	start = 0;	end = 1;					}
	else if(recIdx == 1)	{	start = 1;	end = 2;					}
	else					{ 	start = 0;	end = UPLOAD_SERVER_CNT;	}
	
	for(idx=start;idx<end;idx++)
	{	if(sysCtrl->srvActiveBmp & (1 << idx))
		{	srvHistoryBuf = autoDataSyncInst->srvHistoryBuf[idx];
		
			if(srvHistoryBuf == NULL)
			{	histFileSize = sizeof(HISTORY_REC_HEADER) + sizeof(REC_FILE_INFO)*MAX_REC_HISTORY_INFO_COUNT;	
				memset(histFilename, 0, 32);
				if(idx == 0)
					strcat(histFilename, "history.bin");
				else
					strcat(histFilename, "history2.bin");
				if(!chkFileExist(histFilename))
				{	
CREATE_NEW_HISTORY:
					srvHistoryBuf = SENS_MEM_ZALLOC(histFileSize);
					histHeader = (HISTORY_REC_HEADER *)srvHistoryBuf;
					memset(&srvHistoryBuf[sizeof(HISTORY_REC_HEADER)], 0xFF, sizeof(REC_FILE_INFO)*MAX_REC_HISTORY_INFO_COUNT);
					histHeader->tag = 0x55AA;
					histHeader->version = 0x02;
					histHeader->nextWriteIdx = -1;
					histHeader->currFileIdx = -1;
					servInfo = getServerInfoById(idx);
					if(servInfo)
					{	strcpy(histHeader->serverIp, servInfo->serverDomainName);
						histHeader->serverPort = servInfo->serverPort;
						histHeader->serverType = servInfo->serverProtocol;
					}
				}
				else
				{	srvHistoryBuf = SENS_MEM_ZALLOC(histFileSize);
					fileLength = getFileLength(histFilename);
					if(fileLength < histFileSize)
					{	v1TempBuf = SENS_MEM_ZALLOC(fileLength);
						sdReadFile(histFilename, (char *)v1TempBuf, fileLength, 0, NORMAL_READ_MODE);
						memcpy(srvHistoryBuf, v1TempBuf, sizeof(HISTORY_REC_HEADER_V1));
						memcpy(&srvHistoryBuf[sizeof(HISTORY_REC_HEADER)], &v1TempBuf[sizeof(HISTORY_REC_HEADER_V1)], sizeof(REC_FILE_INFO)*MAX_REC_HISTORY_INFO_COUNT);
						histHeader = (HISTORY_REC_HEADER *)srvHistoryBuf;
						servInfo = getServerInfoById(idx);
						if(servInfo)
						{	strcpy(histHeader->serverIp, servInfo->serverDomainName);
							histHeader->serverPort = servInfo->serverPort;
							histHeader->serverType = servInfo->serverProtocol;
						}
						histHeader->version = 0x02;
						SENS_MEM_FREE(v1TempBuf);
					}
					else
					{	sdReadFile(histFilename, (char *)srvHistoryBuf, fileLength, 0, NORMAL_READ_MODE);
					}
				
					histHeader = (HISTORY_REC_HEADER *)srvHistoryBuf;
					if(histHeader->tag != 0x55AA)
					{	SENS_MEM_FREE(srvHistoryBuf);
						sdFileDelete(histFilename);
						goto CREATE_NEW_HISTORY;
					}
				}
				
				autoDataSyncInst->srvHistoryBuf[idx] = srvHistoryBuf;
			}
		}
	}
}
//------------------------------------------------------------------------------
void addRecordToHistoryFile(char *filename, uint32_t currentTimeStamp, int recIdx)
{	HISTORY_REC_HEADER *histHeader;
	REC_FILE_INFO *recFileInf;
	uint32_t histFileSize;
	int idx;
	uint8_t *srvHistoryBuf;
	char histFilename[32];
	int start, end;
	AUTO_DATA_SYNC_INSTANCE *autoDataSyncInst = (AUTO_DATA_SYNC_INSTANCE *)&sensSys->autoDataSyncInst;
	
	getHistoryRecFileBuf(recIdx);
	
	if(recIdx == 0)				{	start = 0;	end = 1;								}
	else if(recIdx == 1)	{	start = 1;	end = 2;								}
	else									{ start = 0;	end = UPLOAD_SERVER_CNT;	}
	
	for(idx=start;idx<end;idx++)
	{	if(sysCtrl->srvActiveBmp & (1 << idx))
		{	srvHistoryBuf = autoDataSyncInst->srvHistoryBuf[idx];
		
			histHeader = (HISTORY_REC_HEADER *)srvHistoryBuf;
			histFileSize = sizeof(HISTORY_REC_HEADER) + sizeof(REC_FILE_INFO)*MAX_REC_HISTORY_INFO_COUNT;
			memset(histFilename, 0, 32);
			if(idx == 0)
				strcat(histFilename, "history.bin");
			else
				strcat(histFilename, "history2.bin");
			if(histHeader->numOfUnChkFile < MAX_REC_HISTORY_INFO_COUNT)
			{	histHeader->nextWriteIdx++;
				if(histHeader->nextWriteIdx >= MAX_REC_HISTORY_INFO_COUNT)
					histHeader->nextWriteIdx = 0;
				histHeader->numOfUnChkFile++;
				recFileInf = (REC_FILE_INFO *)&srvHistoryBuf[sizeof(HISTORY_REC_HEADER) + histHeader->nextWriteIdx*sizeof(REC_FILE_INFO)];
				memset(recFileInf->filename, 0, HISTORY_REC_FILENAME_SIZE);
				memcpy(recFileInf->filename, filename, strlen(filename));
				recFileInf->unixTime = currentTimeStamp;
				recFileInf->valid = 0x55AAAA55;
				dbgMsg("add new Hist, next write:%d, name:%s, uncheck :%d\r\n", histHeader->nextWriteIdx, recFileInf->filename, histHeader->numOfUnChkFile);
				sdWriteFile(histFilename, (char *)srvHistoryBuf, histFileSize, 0, OVER_WRITE_MODE);
			}
		}
	}
}
//------------------------------------------------------------------------------
static void createNewRecordToHistoryFile(int recIdx)
{	char filename[MAX_FILE_PATH_LEN];
	uint32_t currentTimeStamp;
#ifdef NUVOTON
	S_RTC_TIME_DATA_T recDateTime;
#else
	DATE_STRUCT recDateTime;
	TIME_STRUCT rtcTime;
#endif
	
	currentTimeStamp = sysTimer->currentTimeStamp;
#if TIME_SHIFT_OPERATE	
	if(sysCtrl->tsoValue)
		currentTimeStamp -= (sysCtrl->tsoValue % sysCfg->sensSysCfg.recordSec);
#endif
#ifdef NUVOTON
	timestampToDateTime(currentTimeStamp, &recDateTime);
#else
	rtcTime.SECONDS = currentTimeStamp;
	rtcTime.MILLISECONDS = 0;
	_rtc_time_to_mqx_date(&rtcTime, &recDateTime);
#endif
	//timestampToDateTime(currentTimeStamp, &recDateTime);
	memset(filename, 0, MAX_FILE_PATH_LEN);
#ifdef NUVOTON
	SENS_SPRINTF(filename, "%d/%02d/%02d%02dn%03d.bin", recDateTime.u32Year, recDateTime.u32Month, recDateTime.u32Month, recDateTime.u32Day, sensPq->pqNumber);
#else
	SENS_SPRINTF(filename, "%d/%02d/%02d%02dn%03d.bin", recDateTime.YEAR, recDateTime.MONTH, recDateTime.MONTH, recDateTime.DAY, sensPq->pqNumber);
#endif
	
	if(chkFileExist(filename))
		addRecordToHistoryFile(filename, currentTimeStamp, recIdx);
}
//------------------------------------------------------------------------------
void checkRecordFromHistoryFile(CLOUD_SERVER_INSTANCE *servInst, const char *caller, uint32_t line)
{	HISTORY_REC_HEADER *histHeader;
	REC_FILE_INFO *recFileInf;
	int oldIdx;
	int recIdx;
	char histFilename[32];
	char *histBuf;
	CLOUD_SERVER_INFO *servInfo;
	int createNew = 0;
	AUTO_DATA_SYNC_INSTANCE *autoDataSyncInst = (AUTO_DATA_SYNC_INSTANCE *)&sensSys->autoDataSyncInst;
	
	dbg_msg("call %s from %s(%d)\r\n", __func__, caller, line);
	
	recIdx = servInst->servIdx;
	getHistoryRecFileBuf(recIdx);
	memset(histFilename, 0, 32);
	if(recIdx == 0)
	{	histHeader = (HISTORY_REC_HEADER *)autoDataSyncInst->srvHistoryBuf[recIdx];
		histBuf = (char *)autoDataSyncInst->srvHistoryBuf[recIdx];
		strcat(histFilename, "history.bin");
	}
	else
	{	histHeader = (HISTORY_REC_HEADER *)autoDataSyncInst->srvHistoryBuf[recIdx];
		histBuf = (char *)autoDataSyncInst->srvHistoryBuf[recIdx];
		strcat(histFilename, "history2.bin");
	}
	if(histHeader->numOfUnChkFile == 0)
		createNew = 1;
	
	servInfo = getServerInfoById(recIdx);
	
	if(servInfo == NULL)
		return ;
	
	if(!createNew && ((histHeader->serverPort != servInfo->serverPort) || (histHeader->serverType != servInfo->serverProtocol) || strcmp(histHeader->serverIp, servInfo->serverDomainName)))
	{	createNew  = 1;
	}
	
	if(createNew)
	{	dbgMsg("%s", "check Hist, no Un Check file or not server platform, create current time\r\n");
		sdFileDelete(histFilename);
		SENS_MEM_FREE(autoDataSyncInst->srvHistoryBuf[recIdx]);
		autoDataSyncInst->srvHistoryBuf[recIdx] = NULL;
		createNewRecordToHistoryFile(recIdx);
		histHeader = (HISTORY_REC_HEADER *)autoDataSyncInst->srvHistoryBuf[recIdx];
		histBuf = (char *)autoDataSyncInst->srvHistoryBuf[recIdx];
	}
	if(histHeader == NULL)
		return;
	if(histHeader->currFileIdx == -1)
		histHeader->currFileIdx = 0;
	
	oldIdx = histHeader->currFileIdx;

GET_REC_FILE_INF:
	recFileInf = (REC_FILE_INFO *)&histBuf[sizeof(HISTORY_REC_HEADER) + histHeader->currFileIdx*sizeof(REC_FILE_INFO)];
	if(recFileInf->valid != 0x55AAAA55)
	{	histHeader->currFileIdx++;
		if(histHeader->currFileIdx == MAX_REC_HISTORY_INFO_COUNT)
			histHeader->currFileIdx = 0;
		if(histHeader->currFileIdx == oldIdx)	//no valid data, create current time rec
		{	sdFileDelete(histFilename);
			SENS_MEM_FREE(autoDataSyncInst->srvHistoryBuf[recIdx]);
			autoDataSyncInst->srvHistoryBuf[recIdx] = NULL;
			createNewRecordToHistoryFile(recIdx);
			histHeader = (HISTORY_REC_HEADER *)autoDataSyncInst->srvHistoryBuf[recIdx];
		}
		else
			goto GET_REC_FILE_INF;
	}
	dbgMsg("check Hist, curr Idx:%d, count:%d, curr File:%s\r\n", histHeader->currFileIdx, histHeader->numOfUnChkFile, recFileInf->filename);
	setAutoDataSyncInfo(servInst, recFileInf->filename);
}
#endif


#ifdef AUTO_SEND_WITH_INFO
//------------------------------------------------------------------------------
void removeAutoSendInfo(CLOUD_SERVER_INSTANCE *servInst)
{	AUTO_SEND_INFO *next;
	char *recHeaderBuf;
	REC_HEADER *recHeader;
	uint16_t bmpIdx;
	uint16_t startSlot;
	uint8_t bmpBit;
	
	SENS_SEM_LOCK(servInst->autoSendCtxLock);
	next = (AUTO_SEND_INFO *)servInst->autoSendCtx->next;
	
	recHeaderBuf = SENS_MEM_ALLOC(sizeof(REC_HEADER));
	sdReadFile((char *)servInst->autoSendCtx->filename, recHeaderBuf, sizeof(REC_HEADER), 0, NORMAL_READ_MODE);
	recHeader = (REC_HEADER *)recHeaderBuf;
	startSlot = servInst->autoSendCtx->currSlot;
	bmpIdx = startSlot / 8;
	bmpBit = startSlot % 8;
	recHeader->sendSlotBmpFlag[bmpIdx] |= (1 << bmpBit);
	//recHeader->headerTag.checkSendSlot = startSlot+1;
	recHeader->headerTag.sendCnt++;
	sdWriteFile((char *)servInst->autoSendCtx->filename, recHeaderBuf, sizeof(REC_HEADER), 0, OVER_WRITE_MODE);

	SENS_MEM_FREE(servInst->autoSendCtx->filename);
	SENS_MEM_FREE(servInst->autoSendCtx);
	servInst->autoSendCtx = next;
	SENS_SEM_UNLOCK(servInst->autoSendCtxLock);
}

static void addGlobalAutoSendCtxInfo(AUTO_SEND_INFO *_autoCtxInfo)
{	AUTO_SEND_INFO *autoCtxInfoTemp;
	
	if(gAutoSendCtx == NULL)
		gAutoSendCtx = _autoCtxInfo;
	else
	{	autoCtxInfoTemp = gAutoSendCtx;
		while(autoCtxInfoTemp->next)
		{	autoCtxInfoTemp = autoCtxInfoTemp->next;
		}
		autoCtxInfoTemp->next = _autoCtxInfo;
	}
}

void setGlobalAutoSendCtxInfo(char *filename, REC_HEADER *recHeader, uint32_t currentTimestamp, uint32_t currentTimeSlot)
{	int idx=0;
	CLOUD_SERVER_INFO *serverInfo;
	AUTO_SEND_INFO *_autoSendCtx;
	
	autoSendCtxLock();
	_autoSendCtx = SENS_MEM_ZALLOC(sizeof(AUTO_SEND_INFO));
	_autoSendCtx->filename = SENS_MEM_ZALLOC(strlen(filename)+1);
	memcpy(_autoSendCtx->filename, filename, strlen(filename));
	_autoSendCtx->currSlot = currentTimeSlot;
	for(serverInfo = sysParams->serverInfo;serverInfo;serverInfo = serverInfo->next)
	{	_autoSendCtx->isValidFlag |= (1 << idx);
		idx++;
	}
	addGlobalAutoSendCtxInfo(_autoSendCtx);
	autoSendCtxUnLock();
}
#endif
