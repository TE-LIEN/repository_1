#include "network/protocol/serverProtocol.h"
#include "network/netApp.h"
#include "cJSON/cJSON.h"
#include "sdCardTask.h"
#include "AutoDataSync.h"
#include "physicalQuantity.h"
#include "math.h"

#ifdef NUVOTON
#include "rtcDateTime.h"
#endif

#if SUPPORT_IOA_WEB_API

//------------------------------------------------------------------------------
void displayIoaMessage(char *msg, uint8_t isToken)
{	char msgTemp[257];
	int loopSize;
	int remainSz;
	int dbgLength;
	int x;
	int msgLen = strlen(msg);
	
	debugLock();
	dbg_msg1("[IOA] Send %s Message(%d):", (isToken? "Token":"Data"), msgLen);
	//dbgLength = (msgLen > 1024)? 1024:msgLen;
	dbgLength = msgLen;
	loopSize = dbgLength/256;
	remainSz = dbgLength%256;
	msgTemp[256] = 0;
	for(x=0;x<loopSize;x++)
	{	memcpy(&msgTemp[0], &msg[x*256], 256);
		dbg_msg1("%s", msgTemp);
	}
	if(remainSz)
	{	//memset(msgTemp, 0, 256);
		msgTemp[remainSz] = '\0';
		memcpy(&msgTemp[0], &msg[x*256], remainSz);
		dbg_msg1("%s", msgTemp);
	}
	dbg_msg1("%s", "\r\n");
	debugUnlock();
}
//------------------------------------------------------------------------------
int ioaRspParse(IOA_SERVER_CTX *ioaServCtx, char *webRspContent)
{	cJSON *jsonObj = NULL;
	char *tokenStr;
	char *statusStr;
	cJSON *jsonTempObj;
	int error = -1;
	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)ioaServCtx->servInst;
	
	jsonObj = cJSON_Parse(webRspContent);
	
	if(jsonObj == NULL)
		goto ERROR_RET;
	
	jsonTempObj = cJSON_GetObjectItem(jsonObj, IOAWEB_API_RSP_STATUS);
	if(jsonTempObj == NULL)
		goto ERROR_RET;
	statusStr = cJSON_GetStringValue(jsonTempObj);
	if(strcmp(statusStr, IOAWEB_API_RSP_OK))
		goto ERROR_RET;
	
	jsonTempObj = cJSON_GetObjectItem(jsonObj, IOAWEB_API_RSP_TOKEN);
	if(jsonTempObj)
	{	tokenStr = cJSON_GetStringValue(jsonTempObj);
		if(tokenStr)
		{	ioaServCtx->token = SENS_MEM_ZALLOC(strlen(tokenStr) + 1);
			memcpy(ioaServCtx->token, tokenStr, strlen(tokenStr));
		}
	}
	
	if(ioaServCtx->fsm == IOA_WEB_API_FSM_DATA_SENDING)
	{	batchRemoveAutoDataSyncInfo(serverInst);
		/*SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
		if(serverInst->autoDataSyncCtx == NULL)
			sysCtrlFlag.pqSendDone = 1;
		SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);*/
	}
	
	error = 0;
	
ERROR_RET:
	if(jsonObj)
		cJSON_Delete(jsonObj);
	return error;
}
//------------------------------------------------------------------------------
char *setTokenPayload(IOA_SERVER_CTX *ioaServCtx)
{	cJSON *jsonObj;
	char *payload = NULL;
	
	jsonObj = cJSON_CreateObject();
	
	cJSON_AddItemToObject(jsonObj, IOAWEB_API_REQ_EQUIP_ID, cJSON_CreateString(ioaServCtx->equipId));
	cJSON_AddItemToObject(jsonObj, IOAWEB_API_REQ_API_KEY, cJSON_CreateString(ioaServCtx->apiKey));
	
	payload = cJSON_PrintUnformatted(jsonObj);
	//dbg_msg("[IOA] send Token Payload:%s\r\n", payload);
	displayIoaMessage(payload, 1);
	//displayBufInHex((uint8_t *)payload, strlen(payload), __func__, __LINE__);
	cJSON_Delete(jsonObj);
	return payload;
}
//------------------------------------------------------------------------------
char *setSendDataPayload(CLOUD_SERVER_INSTANCE *serverInst)
{	cJSON *jsonObj;
	cJSON *sensorDataObj;
	cJSON *sensorDataArray;
	char *payload = NULL;
	AUTO_DATA_SYNC_STRUCT *autoDataSyncCtx;
	IOA_SERVER_CTX *ioaServCtx = (IOA_SERVER_CTX *)serverInst->protCtx;

#ifdef NUVOTON
	S_RTC_TIME_DATA_T slotDt;
	uint32_t slotTimeStamp;
#else
	DATE_STRUCT slotDt;
	TIME_STRUCT slotTimeStamp;
#endif
	uint32_t startSlot;
	uint32_t fileOffset;
	char *recBuf;
	float *floatData;
	//int idx;
	int servIdx = serverInst->servIdx;
	PQ_DATA_STRUCT	*pqData;
	char *aliasStr;
	char *sensorDateTimeStr;
	int tokenLen;
	char valString[30];
	int pqIdx;
	uint8_t recSizeForV1 = 0;
	int sensorPqIsValid;
	
	jsonObj = cJSON_CreateObject();
	
	cJSON_AddItemToObject(jsonObj, IOAWEB_API_RSP_TOKEN, cJSON_CreateString(ioaServCtx->token));
	
	tokenLen = strlen(ioaServCtx->token);
	
	sensorDataArray = cJSON_CreateArray();
		
	SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
	autoDataSyncCtx = (AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx;
	
	recBuf = SENS_MEM_ZALLOC(sensPq->bytesPerRec);
	sensorDateTimeStr = SENS_MEM_ZALLOC(strlen("2024-01-01T00:00:00")+1);
	
	while(autoDataSyncCtx)
	{	autoDataSyncCtx->isReady = 1;
		autoDataSyncCtx->waitForAck = 1;
		startSlot = autoDataSyncCtx->startSlot;
		
#ifdef NUVOTON
		slotTimeStamp = autoDataSyncCtx->startTimeStamp + (startSlot * 60);
#else
		slotTimeStamp.SECONDS = autoDataSyncCtx->startTimeStamp + (startSlot * 60);
		slotTimeStamp.MILLISECONDS = 0;
#endif
#if IOA_USE_LOCAL_TIME
	#if IOA_FIXED_LOCAL_TIME_ZONE_480
		#ifdef NUVOTON
		slotTimeStamp += 480 * 60;
		#else
		slotTimeStamp.SECONDS += 480 * 60;
		#endif
	#else
		if(networkCtx->isGetTimeZone)
		{	slotTimeStamp.SECONDS += (networkCtx->timezonediff * 15 * 60);
			slotTimeStamp.SECONDS += networkCtx->daylightsaving * 3600;
		}
	#endif
#endif
#ifdef NUVOTON
		timestampToDateTime(slotTimeStamp, &slotDt);
#else
		_rtc_time_to_mqx_date(&slotTimeStamp, &slotDt);
#endif
		if(chkFileExist((char *)autoDataSyncCtx->filename))
		{	recSizeForV1 = 0;
			if(autoDataSyncCtx->recHeaderVer == FIXED_REC_INTERVAL_VERSION)
				recSizeForV1 = 180;
			fileOffset = sizeof(REC_HEADER) - recSizeForV1 + (startSlot * sensPq->bytesPerRec);
			sdReadFile((char *)autoDataSyncCtx->filename, recBuf, sensPq->bytesPerRec, fileOffset, NORMAL_READ_MODE);
		}
		else
		{	memset(recBuf, 0xFF, sensPq->bytesPerRec);
		}
		
		pqData = sensPq->pqData;
		floatData = (float *)recBuf;
		sensorPqIsValid = 0;
		while(pqData)
		{	aliasStr = (servIdx == 0)? pqData->serv1Alias:pqData->serv2Alias;
			if(aliasStr)
			{	if((!isnan(floatData[pqIdx])) && (floatData[pqIdx] >= 0) && (!isinf(floatData[pqIdx])))
				{	sensorPqIsValid = 1;
					sensorDataObj = cJSON_CreateObject();
					cJSON_AddItemToObject(sensorDataObj, IOAWEB_API_REQ_SENSOR_ID, cJSON_CreateString(aliasStr));
#ifdef NUVOTON
					SENS_SPRINTF(sensorDateTimeStr, "%04d-%02d-%02dT%02d:%02d:%02d", slotDt.u32Year, slotDt.u32Month, slotDt.u32Day, slotDt.u32Hour, slotDt.u32Minute, slotDt.u32Second);
#else
					SENS_SPRINTF(sensorDateTimeStr, "%04d-%02d-%02dT%02d:%02d:%02d", slotDt.YEAR, slotDt.MONTH, slotDt.DAY, slotDt.HOUR, slotDt.MINUTE, slotDt.SECOND);
#endif
					cJSON_AddItemToObject(sensorDataObj, IOAWEB_API_REQ_DATA_TIME, cJSON_CreateString(sensorDateTimeStr));
					cJSON_AddItemToObject(sensorDataObj, IOAWEB_API_REQ_DATA_VLAUE, cJSON_CreateString(my_ftoa(valString, floatData[pqIdx], 3)));
					cJSON_AddItemToArray(sensorDataArray, sensorDataObj);
				}
			}
			pqData = pqData->next;
		}
		if(sensorPqIsValid)
		{	payload = cJSON_PrintUnformatted(sensorDataArray);
			if((strlen(payload) + tokenLen) > 4096 - 256)
			{	SENS_MEM_FREE(payload);
				break;
			}
			SENS_MEM_FREE(payload);
		}
		autoDataSyncCtx = autoDataSyncCtx->next;
	}
	SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
	SENS_MEM_FREE(sensorDateTimeStr);
	
	cJSON_AddItemToObject(jsonObj, IOAWEB_API_REQ_SENSOR_DATA, sensorDataArray);
	
	payload = cJSON_PrintUnformatted(jsonObj);
	//dbg_msg("[IOA] send Data Payload: %s\r\n", payload);
	displayIoaMessage(payload, 0);
	//displayBufInHex((uint8_t *)payload, strlen(payload), __func__, __LINE__);
	cJSON_Delete(jsonObj);
	return payload;
}
#endif