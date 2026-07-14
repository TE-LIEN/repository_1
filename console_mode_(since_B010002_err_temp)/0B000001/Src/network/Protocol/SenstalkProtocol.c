#include "network/protocol/serverProtocol.h"
#include "network/netApp.h"
#include "sensCommon.h"
//#include <math.h>
#include "sdCardTask.h"
#include "sensSystem.h"
#include "physicalQuantity.h"
#include "sensDev.h"
#ifndef PARAM_IS_JSON
#include "ini2.h"
#endif
#include "param.h"

#if AUTO_DATA_SYNC
#include "AutoDataSync.h"
#endif

#if NUVOTON
#include "rtcDateTime.h"
#endif

#if 0
#include "SDCard_task.h"
#include "sens_device.h"
#include "DArray.h"
#include "sensors/sensors.h"
#include "pwrLedCtl.h"
#include "param.h"
#include "cJSON/cJSON.h"
#include "AutoDataSync.h"
#endif

#ifndef PARAM_IS_JSON
//#include "ini2.h"
#endif

#define CRC16_SIZE	2
//------------------------------------------------------------------------------
static struct Sock_Queue *findSockDataAndCombine(struct Sock_Queue *skbQueue, int checkLength, CLOUD_SERVER_INSTANCE *serverInst)
{	struct Sock_Queue *nextQueue;
	char *buf;
	int newLength;
	
	while(1)
	{	if(skbQueue->len < checkLength)
		{	nextQueue = skbQueue->next;
			if(nextQueue != NULL)
			{	//combine buf
				dbg_msg(GREEN"combine data, len:%d, next:%d\r\n"ORG_COLOR, skbQueue->len, nextQueue->len);
				newLength = skbQueue->len + nextQueue->len;
				//buf = reAlloc(skbQueue->buf, skbQueue->len, newLength);
				buf = SENS_MEM_ALLOC(newLength);
				memcpy(buf, skbQueue->buf, skbQueue->len);
				memcpy(&buf[skbQueue->len], nextQueue->buf, nextQueue->len);
				SENS_MEM_FREE(nextQueue->buf);
				nextQueue->buf = (uint8_t *)buf;
				nextQueue->len = newLength;
				//SENS_SEM_LOCK(serverInst->sockRecvQueueLock);
				removeSockQueue(serverInst, skbQueue);
				//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
				skbQueue = nextQueue;
			}
			else	//length 
			{	dbg_msg(GREEN"no enough buffer, check length:%d, curr length:%d\r\n"ORG_COLOR, checkLength, skbQueue->len);
				
				return NULL;
			}
		}
		else
			break;
	}
	return skbQueue;
}
//------------------------------------------------------------------------------
void parserSenstalkProtocol(SERV_RECV_CMD_CTX *senstalkRecvCmd)
{	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)senstalkRecvCmd->serverInst;
	struct Sock_Queue *skbQueue=NULL; //= senstalkRecvCmd->skbQueue;
	struct Sock_Queue *nextQueue;
	char *sendBuf = NULL;
	char *cmdBuf = NULL;
	char func;
	int payloadSize, idx;
	uint16_t dataArrayId, totalSize, headerSize, quantity, crc16, startAddr;
	char str_a[32], str_b[32];
	SenstalkV2NormalHeader *cmdNormalHdr;
	SenstalkV2RecHeader	*cmdRecHdr;
	uint64_t unixTime;
	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
#if defined NUVOTON
	S_RTC_TIME_DATA_T dateTime;
	uint32_t timestamp;
#else
	DATE_STRUCT date1;
	TIME_STRUCT rtctime;
#endif
	char *payload;
	uint16_t dtbuf[7];
	int date_time[7], date_time1[7];	// year, mon, day, hour, min, sec
	long numOfByte;
	int numOfRec;		// Read Hour History Data
	
#if AUTO_DATA_SYNC
	uint8_t isAutoDataSyncPacket = 0;
	void *writeCmdCtx = NULL;
	AUTO_DATA_SYNC_STRUCT *autoDataSyncCtx = (AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx;
#endif
	
	//enum sockQueueType socketQueueType;
	char srcIsLan = senstalkRecvCmd->generateFromInternet & 0x80;
	
	/*if(srcIsLan)
		socketQueueType = LAN_RECV_QUEUE_SOCK;
	else
		socketQueueType = RECV_QUEUE_SOCK;*/
	
	if((senstalkRecvCmd->generateFromInternet & 0x7F) == 0)
	{	if(serverInst->netSendFsm == NET_SEND_FSM_SEND_GUID)
		{	func = SENSTALK_FUNC_DA_READ;
			dataArrayId = SENSTALK_DA_ID_SN;
			headerSize = 0x10;
			quantity = 16;
		}
#ifdef AUTO_DATA_SYNC
		else if(autoDataSyncCtx)
		{	dataArrayId = 0x01;
			func = SENSTALK_FUNC_REC_ANY;
			headerSize = 22;
			quantity = autoDataSyncCtx->slotCnt;
			isAutoDataSyncPacket = 1;
			writeCmdCtx = (void *)autoDataSyncCtx;
		}
#endif
		else if(serverInst->netSendFsm == NET_SEND_FSM_SEND_PQ_TRIG)
		{	func = SENSTALK_FUNC_DA_READ;
#if SENSTALK_REAL_TIME_WITH_TS
			dataArrayId = SENSTALK_DA_ID_AI_FLOAT_WITH_TS;
			quantity = sensPq->pqNumber + 1;	//timestamp
#else
			dataArrayId = SENSTALK_DA_ID_AI_FLOAT;
			quantity = sensPq->pqNumber;
#endif
			headerSize = 0x10;
		}
		/*
		 *	senstalk normal packet
		 *   _________________________________________________________________________________________
		 *	|							Header                              |   payload      |  CRC  |
		 *  |-------+---------------+-------+---+---+-------+-------+-------+----------------+-------+
		 *  |   ID  |       SN      |SIZE   |FC |DT |DA ID  |SA     |   QT  |  payload array |  CRC  |
		 *  |-------+---------------+-------+---+---+---------------+-------+----------------+-------+
		 *  |FD |FF |XX |XX |XX |XX |XX |XX |XX |XX |XX |XX |XX |XX |XX |XX | xxxxxxxxxxxxxx |XX |XX |
		 *  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+----------------+---+---+
		 * 	"ID": Fix 0xFFFD, 	"SN": GUID, 	"Size": payload size??	"FC": Function code
		 *	"DT": payload data type??	"DA ID": Data Array ID, 	"SA":start address, QT: PQ Quantity
		 *	"payload array": real time data array, "CRC": crc16 value.
		 *	
		 *	new real-time data with timestamp, put timestamp(4 byte) in fist 32bit of payload field
		 */
		cmdBuf = SENS_MEM_ZALLOC(headerSize + 2);
		cmdNormalHdr = (SenstalkV2NormalHeader *)cmdBuf;
		cmdNormalHdr->tag = 0xFFFD;
		cmdNormalHdr->func = func;
		cmdNormalHdr->sn[0] = sensSys->guid.guid[12];
		cmdNormalHdr->sn[1] = sensSys->guid.guid[13];
		cmdNormalHdr->sn[2] = sensSys->guid.guid[14];
		cmdNormalHdr->sn[3] = sensSys->guid.guid[15];

		if(func == SENSTALK_FUNC_DA_READ)
		{	cmdNormalHdr->size = 8;
			if(dataArrayId == SENSTALK_DA_ID_AI_FLOAT)
				cmdNormalHdr->dataType = DATATYPE_SINGLE;
			else if(dataArrayId == SENSTALK_DA_ID_DI)
				cmdNormalHdr->dataType = DATATYPE_BIT;
			else if(dataArrayId == SENSTALK_DA_ID_SN)
				cmdNormalHdr->dataType = DATATYPE_CHAR;
			else if(dataArrayId == SENSTALK_DA_ID_AI_FLOAT_WITH_TS)
				cmdNormalHdr->dataType = DATATYPE_SINGLE;
			cmdNormalHdr->dataArrayId = dataArrayId;
			cmdNormalHdr->quantity = quantity;
			startAddr = 0;
		}
#ifdef AUTO_DATA_SYNC
		else if(func == SENSTALK_FUNC_REC_ANY)
		{	cmdRecHdr = (SenstalkV2RecHeader *)cmdBuf;
			cmdRecHdr->size = 0x0E;
			cmdRecHdr->dataType = DATATYPE_SINGLE;
			cmdRecHdr->dataArrayId = dataArrayId;
		}
#endif
	}
	else
	{	uint16_t crc16_1;
		SERV_RECV_CMD_CTX *cmdMsg;
		int remainLen;
		int frameLen;
		struct TaskQMsg msg;
		
#ifdef SUPOORT_LORA_WAN
		if(protocolType == PROTOCOL_LORA_WAN_SENSTALK_V2)
		{	return;
		}
#endif
		
RE_GET_DATA:
		dbg_msg(GREEN"start find srv %d skbQueue at sock:%d\r\n"ORG_COLOR, serverInst->servIdx, serverInst->fd);
			
		skbQueue = findOldestSockQueue(serverInst);
		if(skbQueue == NULL)
			goto END_OF_PARSER;
		
		dbg_msg(GREEN"find skbQueue at %p, length:%d\r\n"ORG_COLOR, skbQueue, skbQueue->len);
			
		skbQueue = findSockDataAndCombine(skbQueue, 8, serverInst);
		if(skbQueue == NULL)
			goto END_OF_PARSER;
		
		cmdBuf = (char *)skbQueue->buf;
			
		cmdNormalHdr = (SenstalkV2NormalHeader *)cmdBuf;
		crc16 = (uint16_t)swCrc16((unsigned char *)cmdBuf , 0, cmdNormalHdr->size + 8);
		crc16_1 = cmdBuf[cmdNormalHdr->size + 8 + 1] << 8 | cmdBuf[cmdNormalHdr->size + 8];
		frameLen = cmdNormalHdr->size + 8 + CRC16_SIZE;
		if((skbQueue->len < frameLen) || (crc16 != crc16_1))
		{	if(skbQueue->len < frameLen)
			{	skbQueue = findSockDataAndCombine(skbQueue, frameLen, serverInst);
				if(skbQueue == NULL)
				{	SENS_MEM_FREE(senstalkRecvCmd);
					return;
				}
			}
			dbg_msg("[WARNING] command: CRC error.(%d, %d, %X, %X)\r\n", skbQueue->len, frameLen, crc16, crc16_1);
			if(skbQueue->len > frameLen)
			{	char *buf;
				
				remainLen = skbQueue->len - frameLen;
				buf = SENS_MEM_ALLOC(remainLen);
				memcpy(buf, &skbQueue->buf[frameLen], remainLen);
				SENS_MEM_FREE(skbQueue->buf);
				skbQueue->buf = (unsigned char *)buf;
				skbQueue->len = remainLen;
				goto RE_GET_DATA;
			}
		}
			
		if(skbQueue->len > frameLen)
		{	remainLen = skbQueue->len - frameLen;
			nextQueue = skbQueue->next;
				
			if(nextQueue == NULL)
			{	//SENS_SEM_LOCK(serverInst->sockRecvQueueLock);
				nextQueue = createNewSockQueue(serverInst, remainLen);
				memcpy(nextQueue->buf, &skbQueue->buf[frameLen], remainLen);
				nextQueue->isValid = 1;
				//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
				dbg_msg(RED"ramain length:%d, create new Queue!!\r\n"ORG_COLOR, remainLen);
			}
			else
			{	sendBuf = SENS_MEM_ALLOC(remainLen + nextQueue->len);
				memcpy(sendBuf, &skbQueue->buf[frameLen], remainLen);
				memcpy(&sendBuf[remainLen], nextQueue->buf, nextQueue->len);
				SENS_MEM_FREE(nextQueue->buf);
				nextQueue->buf = (unsigned char *)sendBuf;
				nextQueue->len = remainLen + nextQueue->len;
				sendBuf = NULL;
				dbg_msg(RED"ramain length:%d, copy to next queue!!\r\n"ORG_COLOR, remainLen);
			}
			cmdMsg = SENS_MEM_ZALLOC(sizeof(SERV_RECV_CMD_CTX));
			cmdMsg->serverInst = serverInst;
			cmdMsg->generateFromInternet = 2 | srcIsLan;
			msg.msgId = SERV_CMD_PROCESS_Q_MSG_RECV_CMD;
			msg.ptr = (char *)cmdMsg;
			
			if(sendMsgWithErrHandle(SERV_CMD_PROCESS_TASK, SENS_MSG_Q_SEND(servCmdProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
			{	SENS_MEM_FREE(cmdMsg);
			}
		}
		cmdBuf = (char *)skbQueue->buf;
		cmdNormalHdr = (SenstalkV2NormalHeader *)cmdBuf;
			
		//if((cmdNormalHdr->sn[0] != sensSys->guid.guid[12]) || (cmdNormalHdr->sn[1] != sensSys->guid.guid[13]) ||
		//	 (cmdNormalHdr->sn[2] != sensSys->guid.guid[14]) || (cmdNormalHdr->sn[3] != sensSys->guid.guid[15]))
		if(memcmp(&cmdNormalHdr->sn[0], (void *)&sensSys->guid.guid[12], 4))
		{	dbg_msg("%s", "[WARNING] command: GUID error.\r\n");
			goto END_OF_PARSER;
		}
		func = cmdNormalHdr->func;
		if(func < 0x80)
		{	dataArrayId = cmdNormalHdr->dataArrayId;
			startAddr = cmdNormalHdr->startAddr;
			if(func <= 0x01 )
				quantity = cmdNormalHdr->quantity;	//note: if read real-time data with TS, server must +1 for TS field in first Payload data
		} 
	}
	
	dbg_msg("parser func %X from serv-%d\r\n", func, serverInst->servIdx);
	//make response
	if(func == SENSTALK_FUNC_DA_READ)
	{	if((dataArrayId == SENSTALK_DA_ID_DI) || (dataArrayId == SENSTALK_DA_ID_DO))
		{	DI_INSTANCE *diInst = (DI_INSTANCE *)&sensDev->diInst;
			DI_CONTEXT	*diCtx = (DI_CONTEXT	*)&diInst->diCtx;
			DO_INSTANCE *doInst = (DO_INSTANCE *)&sensDev->diInst;
			DO_CONTEXT	*doCtx = (DO_CONTEXT	*)&doInst->doCtx;
			
			
			dbg_msg ("%s","[MESSAGE] Read DI / DO parameters\r\n");
			if((dataArrayId == 0) && (startAddr + quantity) > DIQUANTITY)
			{	startAddr = 0;
				quantity = DIQUANTITY;
			}
			else if((dataArrayId == 1) && (startAddr + quantity) > DO_WIRED_QUANTITY)
			{	startAddr = 0;
				quantity = DO_WIRED_QUANTITY;
			}
			payloadSize = quantity / 8;
			if(quantity % 8)
				payloadSize++;
			payload = SENS_MEM_ZALLOC(payloadSize);
			for(idx=0;idx<quantity;idx++)
			{	if(dataArrayId == 0)
				{	payload[idx / 8] |= (diCtx->diStsNew[startAddr + idx] << idx);
				}
				else
				{	payload[idx / 8] |= (doCtx->doSts[startAddr + idx] << idx);
				}
			}
		}
		else if((dataArrayId == SENSTALK_DA_ID_AI_FLOAT) || (dataArrayId == SENSTALK_DA_ID_AI_FLOAT_WITH_TS))
		{	int hasTs = 0;
			int offset = 0;
			uint32_t ts = dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
			dbg_msg("[SETTING] Read AI parameters(AI: %d, Qty: %d)\r\n", sensPq->pqNumber, quantity);
			
			if(dataArrayId == SENSTALK_DA_ID_AI_FLOAT_WITH_TS)
			{	hasTs = 1;
				offset = 4;
			}
			if((startAddr + quantity) > (sensPq->pqNumber + hasTs))
			{	startAddr = 0;
				quantity = sensPq->pqNumber + hasTs;
			}
			payloadSize = quantity*4;
			payload = SENS_MEM_ZALLOC(payloadSize);
			if(hasTs)
				memcpy(payload, (uint8_t *)&ts, 4);
			memcpy(&payload[offset], (char *)&sensPq->pqVal[startAddr], payloadSize);
		}
		else if(dataArrayId == SENSTALK_DA_ID_TIME)
		{	if(quantity != 1)
			{	dbg_msg("[WARNING] COMMAND: SIM<-ST Read unix time quantity != %d", 1);
				goto END_OF_PARSER;
			}
#if defined NUVOTON
			timestamp = dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
			unixTime = (uint64_t)timestamp;
#else
			date1.YEAR=sensSys->date_time[0];date1.MONTH=sensSys->date_time[1];date1.DAY=sensSys->date_time[2];
			date1.HOUR=sensSys->date_time[3];date1.MINUTE=sensSys->date_time[4];date1.SECOND=sensSys->date_time[5];
			date1.MILLISEC = 0;
			_rtc_time_from_mqx_date(&date1, &rtctime);
			unixTime = (uint_64)rtctime.SECONDS;
#endif
			payloadSize = quantity*8;
			payload = SENS_MEM_ZALLOC(payloadSize);
			memcpy(payload, (char *)&unixTime, payloadSize);
		}
		else if(dataArrayId == SENSTALK_DA_ID_FW_VER)                                                 // Data Array 0x15: FW version & status
		{	if(quantity != 5)
			{	dbg_msg("[WARNING] COMMAND: SIM<-ST Read FW version quantity != %d", 5);
				goto END_OF_PARSER;
			}

			unixTime = (uint32_t)FW_VERSION;
			payloadSize = quantity*5;
			payload = SENS_MEM_ZALLOC(payloadSize);
			memcpy(payload, (char*)&unixTime, 4);                                   // copy FW Version
#if TEST_N_REMOVE
			memcpy(&payload[4], (char*)&sensDev->Update_status, 1);
#endif
		}
#ifdef SUPPORT_WIRED_LAN
		else if(dataArrayId == SENSTALK_DA_ID_LAN_PARAM)                                                 // Data Array 0x06: LAN->IP/MASK/GATEWAY
		{	WIRED_CONFIG *wiredCfg = (WIRED_CONFIG *)&netCfg->wiredCfg;
			
			dbg_msg ("%s","[SETTING] Read LAN->IP/MASK/GATEWAY parameters\r\n");

			payloadSize = 12;
			payload = SENS_MEM_ZALLOC(payloadSize);
			payload[0] = (wiredCfg->ipAddr >> 24) & 0xFF;
			payload[1] = (wiredCfg->ipAddr >> 16) & 0xFF;
			payload[2] = (wiredCfg->ipAddr >> 8) & 0xFF;
			payload[3] = (wiredCfg->ipAddr >> 0) & 0xFF;
			
			payload[4] = (wiredCfg->maskAddr >> 24) & 0xFF;
			payload[5] = (wiredCfg->maskAddr >> 16) & 0xFF;
			payload[6] = (wiredCfg->maskAddr >> 8) & 0xFF;
			payload[7] = (wiredCfg->maskAddr >> 0) & 0xFF;
			
			payload[8] = (wiredCfg->gwAddr >> 24) & 0xFF;
			payload[9] = (wiredCfg->gwAddr >> 16) & 0xFF;
			payload[10] = (wiredCfg->gwAddr >> 8) & 0xFF;
			payload[11] = (wiredCfg->gwAddr >> 0) & 0xFF;
		}
#endif
		else if(dataArrayId == SENSTALK_DA_ID_CLIENT_IP)                                                   // Data Array 0x07: Client IP/Port
		{	
#if TEST_N_REMOVE	//ґ???oҢxK??Ŀǰґ??֧ԮDomain name
			int16_t tmp_i;
			dbg_msg ("%s","[SETTING] Read Client IP/Port parameters\r\n");
			
			payloadSize = 10;
			payload = SENS_MEM_ZALLOC(payloadSize);
			tmp_i = (int16_t)gprs.client_mode_ip[0];
			memcpy(payload, (char*)&tmp_i, 2);
			tmp_i = (int16_t)gprs.client_mode_ip[1];
			memcpy(payload+2, (char*)&tmp_i, 2);
			tmp_i = (int16_t)gprs.client_mode_ip[2];
			memcpy(payload+4, (char*)&tmp_i, 2);
			tmp_i = (int16_t)gprs.client_mode_ip[3];
			memcpy(payload+6, (char*)&tmp_i, 2);
			tmp_i = (int16_t)serverInst->serverPort;
			memcpy(payload+8, (char*)&tmp_i, 2);
#endif
		}
		else if(dataArrayId == SENSTALK_DA_ID_PARAM)                                                   // Data Array 0x08: power-saving, server/client mode, wire/wireless, reboot
		{	dbg_msg ("%s","[SETTING] Read power-saving, server/client mode, wire/wireless, reboot parameters\r\n");
			payloadSize = 1;
			payload = SENS_MEM_ZALLOC(payloadSize);
			payload[0] = ((uint8_t)sensSysCfg->psm&0x01) | (0x01<<1) | (((uint8_t)netCfg->connPriority-1)&0x01)<<2;
		}
		else if(dataArrayId == SENSTALK_DA_ID_PARAM_2)                                                   // Data Array 0x09: recording interval, num of AI/DI/DO
		{	int16_t tmp_i;
			
			dbg_msg ("%s","[SETTING] Read recording interval, num of AI/DI/DO parameters\r\n");
			
			payloadSize = 8;
			payload = SENS_MEM_ZALLOC(payloadSize);
			tmp_i = (int16_t)(sensSysCfg->recordSec);
			memcpy(payload + 0, (char*)&tmp_i, 2);
			tmp_i = (int16_t)sensPq->pqNumber;
			memcpy(payload + 2, (char*)&tmp_i, 2);
			tmp_i = (int16_t)DIQUANTITY;
			memcpy(payload + 4, (char*)&tmp_i, 2);
			tmp_i = (int16_t)DO_WIRED_QUANTITY;
			memcpy(payload + 6, (char*)&tmp_i, 2);
		}
		else if(dataArrayId == SENSTALK_DA_ID_APN)                                                   // Data Array 0x0B: APN
		{	NET_INSTANCE *workingInst = networkCtx->workingInst;
			MOBILE_INSTANCE *wlInst;
			dbg_msg ("%s","[SETTING] Read APN parameters\r\n");

			if(workingInst->netType != NETWORK_TYPE_ETH)
			{	wlInst = workingInst->wlInst;
				payloadSize = strlen(wlInst->apn);
				payload = SENS_MEM_ZALLOC(payloadSize);
				memcpy(payload, wlInst->apn, payloadSize);
			}
		}
		else if(dataArrayId == SENSTALK_DA_ID_SN)                                                   // Data Array 0x13: read GUID
		{	dbg_msg ("%s","[SETTING] Read GUID parameters\r\n");

			payloadSize = 16;
			payload = SENS_MEM_ZALLOC(payloadSize);
			memcpy(payload, (void *)sensSys->guid.guid, 16);
		}
		totalSize = sizeof(SenstalkV2NormalHeader) + payloadSize;
		sendBuf = SENS_MEM_ZALLOC(totalSize + CRC16_SIZE);
		memcpy(sendBuf, cmdBuf, sizeof(SenstalkV2NormalHeader));
		memcpy(&sendBuf[sizeof(SenstalkV2NormalHeader)], payload, payloadSize);
		SENS_MEM_FREE(payload);
		cmdNormalHdr = (SenstalkV2NormalHeader *)sendBuf;
		cmdNormalHdr->startAddr = startAddr;
		cmdNormalHdr->quantity = quantity;
		cmdNormalHdr->size = totalSize - 8;
	}
	else if(func == SENSTALK_FUNC_DA_WRITE)
	{	
#if defined PARAM_IS_JSON
		cJSON *objJson;
#else
		MiniFile *mIniFile = NULL;
#endif
		
		char writeIni = 1;
		cmdNormalHdr = (SenstalkV2NormalHeader *)cmdBuf;
		payload = (char *)&cmdNormalHdr[1];
		dbg_msg("[SETTING] Start address: %d, quantity: %d\r\n", startAddr, quantity);
		if(dataArrayId != SENSTALK_DA_ID_KEEPALIVE)
		{	
#if defined PARAM_IS_JSON
			objJson = getJsonObjFromFile(JSON_PARAM_FILE);
			if(objJson == NULL)
			{	dbg_msg ("[WARNING] FUNC: Can't parse '%s' file!\r\n", INIFILE);
				sensSys->IsReboot = 1;
				goto END_OF_PARSER;
			}
#else
			mIniFile = mini_parse_file(INIFILE);
			if(mIniFile == NULL)
			{	dbg_msg ("[WARNING] FUNC: Can't parse '%s' file!\r\n", INIFILE);
				sysCtrl->isReadyToReboot = 1;
				goto END_OF_PARSER;
			}
#endif
		}
		totalSize = skbQueue->len - CRC16_SIZE;
      
		sendBuf = SENS_MEM_ALLOC(totalSize + CRC16_SIZE);
		memcpy(sendBuf, cmdBuf, totalSize);
		
		if(dataArrayId == SENSTALK_DA_ID_DO)                                                      // Data Array 0x01: write DO Status
		{	DO_CONFIG *doCfg = (DO_CONFIG *)&sysCfg->doCfg;
			
			//---------------data read --> write---------------
			if(doCfg->mode == 0)	//do remote ctrl mode
			{	if(quantity != 1)
				{	dbg_msg("[SETTING] DO quantity != %d", 1);
#if defined PARAM_IS_JSON
					cJSON_Delete(objJson);
#else
					mini_file_free(mIniFile);
#endif
				  SENS_MEM_FREE(sendBuf);
				  goto END_OF_PARSER;
				}
				dbg_msg ("%s","[SETTING] Write DO Status parameters\r\n");
				
				//sensDev->do_address = (char)startAddr;
				doCfg->powerCtrl[startAddr] = (payload[0] >> startAddr)&0x01;
				
#if defined PARAM_IS_JSON
				int intVal = 0;
				if(sensSys->DO_power[0]) intVal |= 1<<2;
				if(sensSys->DO_power[1]) intVal |= 1<<3;
				updateJsonParamArrayValue(objJson, PARAM_JSON_DO, 0, PARAM_DO_ITEM_VALUE_BMP, intVal, NULL);
#else
				memset(str_a, 0, 32);
				SENS_SPRINTF(str_a, "%d %d %d %d %d %d", doCfg->powerCtrl[0], doCfg->powerCtrl[1], doCfg->powerCtrl[2], doCfg->powerCtrl[3], doCfg->powerCtrl[4], doCfg->powerCtrl[5]);
				mini_file_update_value_for_key(mIniFile, SECTION, DO_OUTPUT_POWER_str, str_a);
#endif
			}
		}
		else if(dataArrayId == SENSTALK_DA_ID_TIME)                                                 // Data Array 0x14: Date and Time
		{	if(quantity != 1)
			{	dbg_msg("[SETTING] Time quantity != %d\r\n", 1);
#if defined PARAM_IS_JSON
				cJSON_Delete(objJson);
#else
				mini_file_free(mIniFile);
#endif
				SENS_MEM_FREE(sendBuf);
				goto END_OF_PARSER;
			}
			dbg_msg ("%s","[SETTING] Write Date and Time parameters\r\n");
			
			memcpy((char*)&unixTime, payload, 8);

#if defined NUVOTON
			timestamp = (uint32_t)unixTime;
			timestampToSetRtc(timestamp);
#else
			rtctime.SECONDS = (uint_32)unixTime;
			rtctime.MILLISECONDS = 0;
			_rtc_time_to_mqx_date(&rtctime, &date1);                                  //unix time->cmdData  (rtctime1->date1)
			
			dtbuf[0]=date1.YEAR;dtbuf[1]=date1.MONTH;dtbuf[2]=date1.DAY;
			dtbuf[3]=date1.HOUR;dtbuf[4]=date1.MINUTE;dtbuf[5]=date1.SECOND;
			if(dtbuf[0] > 2017) set_time(dtbuf);
			dbg_msg("[SETTING] Time: [%d/%d/%d %d:%d:%d]\r\n", dtbuf[0], dtbuf[1], dtbuf[2], dtbuf[3], dtbuf[4], dtbuf[5]);			
#endif
			writeIni = 0;
		}
		else if(dataArrayId == SENSTALK_DA_ID_LAN_PARAM)                                                 // Data Array 0x06: LAN->IP/MASK/GATEWAY
		{	dbg_msg ("%s","[SETTING] Write LAN->IP/MASK/GATEWAY parameters\r\n");
			//---------------data read --> write---------------
			if(startAddr == 0 && quantity == 12)
			{	memset(str_a, 0, 32);
				memcpy(str_a, payload,   quantity);
				
				memset(str_b, 0, 32);
				SENS_SPRINTF(str_b, "%d.%d.%d.%d", str_a[0], str_a[1], str_a[2], str_a[3]);
#if defined PARAM_IS_JSON
				updateJsonParamArrayValue(objJson, PARAM_JSON_LAN, 0, PARAM_LAN_ITEM_IP, 0, str_b);
#else
				mini_file_update_value_for_key(mIniFile, SECTION, "IP_ADDRESS", str_b);
#endif
				memset(str_b, 0, 32);
				SENS_SPRINTF(str_b, "%d.%d.%d.%d", str_a[4], str_a[5], str_a[6], str_a[7]);
#if defined PARAM_IS_JSON
				updateJsonParamArrayValue(objJson, PARAM_JSON_LAN, 0, PARAM_LAN_ITEM_MASK, 0, str_b);
#else
				mini_file_update_value_for_key(mIniFile, SECTION, "IP_MASK", str_b);
#endif	
				memset(str_b, 0, 32);
				SENS_SPRINTF(str_b, "%d.%d.%d.%d", str_a[8], str_a[9], str_a[10], str_a[11]);
#if defined PARAM_IS_JSON
				updateJsonParamArrayValue(objJson, PARAM_JSON_LAN, 0, PARAM_LAN_ITEM_GW, 0, str_b);
#else
				mini_file_update_value_for_key(mIniFile, SECTION, "IP_GATEWAY", str_b);
#endif
			}
		}
		else if(dataArrayId == SENSTALK_DA_ID_CLIENT_IP)		// Data Array 0x07: Client IP/Port
		{	dbg_msg ("%s","[SETTING] Write Client IP/Port parameters\r\n");
			//---------------data read --> write---------------
			if(startAddr == 0 && quantity == 5)
			{	int16_t ip[4], port;
				uint16_t *payloadW;
				
				memset(str_a, 0, 32);
				payloadW = (uint16_t *)payload;
				ip[0] = payloadW[0];
				ip[1] = payloadW[1];
				ip[2] = payloadW[2];
				ip[3] = payloadW[3];
				port  = payloadW[4];
				SENS_SPRINTF(str_a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
#if defined PARAM_IS_JSON
				updateJsonParamArrayValue(objJson, PARAM_JSON_SERVER, 0, PARAM_SERV_ITEM_IP, 0, str_a);
				updateJsonParamArrayValue(objJson, PARAM_JSON_SERVER, 0, PARAM_SERV_ITEM_PORT, port, NULL);
#else
				SENS_SPRINTF(str_b, "%d", port);
				//my_ftoa(str_b, tmp_i, 0);
				mini_file_update_value_for_key(mIniFile, SECTION, "SENSLINK_ADDRESS", str_a);
				mini_file_update_value_for_key(mIniFile, SECTION, "SENSLINK_PORT", str_b);
#endif
			}
		}
		else if(dataArrayId == SENSTALK_DA_ID_PARAM)                                                 // Data Array 0x08: power-saving, server/client mode, wire/wireless, reboot
		{	
#ifndef PARAM_IS_JSON
			char writeCfg = 1;
#endif
			dbg_msg ("%s","[SETTING] Write power-saving, server/client mode, wire/wireless, reboot parameters\r\n");
			//---------------data read --> write---------------
			if(startAddr == 0 && quantity == 1)
			{	//---Power-saving---
#if defined PARAM_IS_JSON				
				updateJsonParamArrayValue(objJson, PARAM_JSON_SYSTEM, 0, PARAM_SYS_ITEM_PSM, payload[0], NULL);
				my_ftoa(str_b, payload[0] & 0x01, 0);
#else
				SENS_SPRINTF(str_a, "PIC_POWERDOWN");
#endif	
			}
			else if(startAddr == 1 && quantity == 1)
			{	//remove server mode
#if !defined PARAM_IS_JSON
				my_ftoa(str_b, ((payload[0] & 0x01)+1), 0);
				SENS_SPRINTF(str_a, "SENSMODE");
#endif
			}
			else if(startAddr == 2 && quantity == 1)
			{	//---wire/wireless---
#if defined PARAM_IS_JSON
				updateJsonParamArrayValue(objJson, PARAM_JSON_SYSTEM, 0, PARAM_SYS_ITEM_PRIORITY_CONNECTION, ((payload[0] & 0x01)+1), NULL);
#else
				SENS_SPRINTF(str_a, "PRIORITY_CONNECTION");
				my_ftoa(str_b, ((payload[0] & 0x01)+1), 0);
#endif
			}
			else if(startAddr == 3 && quantity == 1)
			{	//---reboot---
				//memcpy((char*)&tmp_i8, cmdData + ReceiveDataPosition + PARSER_DATA_ARRAY_HEADER_SIZE, 1);
				if((payload[0] & 0x01) == 1)
				{	sysCtrl->isReadyToReboot = 1;
				}
#ifndef PARAM_IS_JSON
				writeCfg = 0;
#endif
			}
#ifndef PARAM_IS_JSON
			if(writeCfg)
			{	mini_file_update_value_for_key(mIniFile, SECTION, str_a, str_b);
			}
#endif
		}
		else if(dataArrayId == SENSTALK_DA_ID_PARAM_2)                                                 // Data Array 0x09: recording interval, num of AI/DI/DO
		{	dbg_msg ("%s","[SETTING] Write recording interval, num of AI/DI/DO parameters\r\n");
			//---------------data read --> write---------------
			if(startAddr == 0 && quantity == 1)
			{	//---Recording interval---
				uint16_t *payloadW;
				payloadW = (uint16_t *)payload;
#if defined PARAM_IS_JSON
				updateJsonParamArrayValue(objJson, PARAM_JSON_SYSTEM, 0, PARAM_SYS_ITEM_REC_TIME, payloadW[0], NULL);
#else
				my_ftoa(str_a, payloadW[0], 0);
				mini_file_update_value_for_key(mIniFile, SECTION, "SD_REC_SEC", str_a);
#endif
			}
		}
		else if(dataArrayId == SENSTALK_DA_ID_APN)                                                 // Data Array 0x0B: APN
		{	dbg_msg ("%s","[SETTING] Write APN parameters\r\n");
			if(quantity > 30)	quantity = 30;
			//---------------data read --> write---------------
			if(startAddr == 0)
			{	//---APN---
				memcpy(str_a, payload, quantity);
				str_a[quantity] = '\0';
#if defined PARAM_IS_JSON
				updateJsonParamArrayValue(objJson, PARAM_JSON_MOBILE, 0, PARAM_MOBILE_ITEM_LTE_APN, 0, str_a);
#else
				mini_file_update_value_for_key(mIniFile, SECTION, "SENS_APN", str_a);
#endif
			}
		}
		
		if(dataArrayId == SENSTALK_DA_ID_KEEPALIVE)
		{	dbg_msg("%s", "[MESSAGE] Server sends response back.\r\n");
		}
		else
		{	if(writeIni)
			{	SENS_TIME_DELAY(10);
#if defined PARAM_IS_JSON
				writeJsonObjToFile(JSON_PARAM_FILE, objJson);
#else
				mini_save_to_file(mIniFile, NULL, INI_BAK_FILE);
#endif	
			}
#if defined PARAM_IS_JSON
			cJSON_Delete(objJson);
#else
			mini_file_free(mIniFile);
#endif
		}
	}
	else if((func == SENSTALK_FUNC_REC_DAY) || (func == SENSTALK_FUNC_REC_HOUR)/* || (func == 0x57)*/ || (func == SENSTALK_FUNC_REC_ANY))     // ---[[Read History Data]]--- --> Day / Hour / 5 mins / any time
	{	cmdRecHdr = (SenstalkV2RecHeader *)cmdBuf;
		//payload = (char *)&cmdRecHdr[1];
		memcpy((char *)&unixTime, cmdRecHdr->unixTime, 8);
		if(func == SENSTALK_FUNC_REC_ANY)
		{	
#ifdef AUTO_DATA_SYNC
			if(isAutoDataSyncPacket)
			{
			}
			else
#endif
			{	
#if defined NUVOTON
				timestamp = (uint32_t)(unixTime - (unixTime%(sensSysCfg->recordSec)));
				timestampToDateTime(timestamp, &dateTime);

				date_time1[0]=dateTime.u32Year;	date_time1[1]=dateTime.u32Month;	date_time1[2]=dateTime.u32Day;
				date_time1[3]=dateTime.u32Hour;	date_time1[4]=dateTime.u32Minute;	date_time1[5]=dateTime.u32Second;
#else
				rtctime.SECONDS = (uint_32)(unixTime - (unixTime%(sd.record_sec)));
				rtctime.MILLISECONDS = 0;
				_rtc_time_to_mqx_date(&rtctime, &date1);

				date_time1[0]=date1.YEAR;date_time1[1]=date1.MONTH;date_time1[2]=date1.DAY;
				date_time1[3]=date1.HOUR;date_time1[4]=date1.MINUTE;date_time1[5]=date1.SECOND;
#endif
			}
		}
		else
		{	
#if defined NUVOTON
			timestamp = (uint32_t)unixTime;
			timestampToDateTime(timestamp, &dateTime);

			dtbuf[0]=dateTime.u32Year;	dtbuf[1]=dateTime.u32Month;		dtbuf[2]=dateTime.u32Day;
			dtbuf[3]=dateTime.u32Hour;	dtbuf[4]=dateTime.u32Minute;	dtbuf[5]=dateTime.u32Second;
#else
			rtctime.SECONDS = (uint_32)unixTime;
			rtctime.MILLISECONDS = 0;
			_rtc_time_to_mqx_date(&rtctime, &date1);                                // unix time->data  (rtctime1->date1)

			dtbuf[0]=date1.YEAR;dtbuf[1]=date1.MONTH;dtbuf[2]=date1.DAY;
			dtbuf[3]=date1.HOUR;dtbuf[4]=date1.MINUTE;dtbuf[5]=date1.SECOND;
#endif
			for(idx = 0; idx < 6; idx++)
				date_time[idx] = (int)dtbuf[idx];
		}
		if(func == SENSTALK_FUNC_REC_DAY || func == SENSTALK_FUNC_REC_HOUR /*|| func == 0x57*/)
		{	if(func == SENSTALK_FUNC_REC_DAY)																					// one day
			{	numOfByte = 86400/(sensSysCfg->recordSec) * sensPq->bytesPerRec;			// total bytes
				numOfRec = 86400/(sensSysCfg->recordSec);
				date_time[3]=0;date_time[4]=0;date_time[5]=0;
				dbg_msg("[SETTING] One day record: [%d:%02d:%02d]\r\n", date_time[0], date_time[1], date_time[2]);
			}
			else if(func == SENSTALK_FUNC_REC_HOUR)                                                   // one hour
			{	numOfByte = 3600/(sensSysCfg->recordSec)*sensPq->bytesPerRec;		// total bytes
				numOfRec = 3600/(sensSysCfg->recordSec);
				date_time[4]=0;date_time[5]=0;
				dbg_msg("[SETTING] One hour record: [%d/%02d/%02d H%02d]\r\n", date_time[0], date_time[1], date_time[2], date_time[3]);
			}
			payloadSize = numOfByte;
			sendBuf = SENS_MEM_ALLOC(payloadSize+4+sizeof(SenstalkV2RecHeader)+CRC16_SIZE);
			if(sendBuf != NULL)
			{	payload = &sendBuf[sizeof(SenstalkV2RecHeader)];
#if REC_INTERVAL_FIXED
				{	int idx1;
					int bufOffset = 4;
					int fileOffset;
					int currSlot;
					char *filename;
					int recTimeInterval = sensSysCfg->recordSec / 60;
					int recSizeForV1 = 0;
					
					filename = SENS_MEM_ZALLOC(64);
					SENS_SPRINTF(filename, "%04d/%02d/%02d%02dn%03d.bin", date_time[0], date_time[1], date_time[1], date_time[2], sensPq->pqNumber);
					
					if(getRecFileVersion(filename) == FIXED_REC_INTERVAL_VERSION)
						recSizeForV1 = 180;
					
					currSlot = (date_time[3] * 3600) / 60;
					if(chkFileExist(filename))
					{	for(idx1=0;idx1<numOfRec;idx1++)
						{	fileOffset = sizeof(REC_HEADER) - recSizeForV1 + (currSlot * sensPq->bytesPerRec);
							sdReadFile((char *)filename, &payload[bufOffset], sensPq->bytesPerRec, fileOffset, NORMAL_READ_MODE);
							currSlot += recTimeInterval;
							bufOffset += sensPq->bytesPerRec;
						}
					}
					else
					{	memset(&payload[bufOffset], 0xFF, payloadSize);
					}
					SENS_MEM_FREE(filename);
				}
#else
				SD_ReadRecNByte(date_time, sd.record_sec, da.ai_number, da.byte_per_rec, numOfByte, &payload[4], &payloadSize);
#endif
			}
			else
			{	payloadSize = 0;
				numOfRec = 0;
				sendBuf = SENS_MEM_ALLOC(4+sizeof(SenstalkV2RecHeader)+CRC16_SIZE);
				payload = &sendBuf[sizeof(SenstalkV2RecHeader)];
			}
			
			memcpy(sendBuf, cmdBuf, sizeof(SenstalkV2RecHeader));
			totalSize = sizeof(SenstalkV2RecHeader) + payloadSize + 4;
			uint16_t *payloadW;
			
			payloadW = (uint16_t *)payload;
			payloadW[0] =sensPq->pqNumber;
			payloadW[1] = numOfRec;
			dbg_msg("[SETTING] PQ Item: %d, PQ Record number: %d\r\n", sensPq->pqNumber, numOfRec);
			
			//*re_len += 24;                                                              // TotalBytes = re_len + command byte
		}
		if(func == SENSTALK_FUNC_REC_ANY)                                                       // any time
		{	uint16_t timeSpan;
			uint16_t *payloadW;
#if AUTO_DATA_SYNC
			uint16_t recTimeInterval;
#endif

			//cmdRecHdr = (SenstalkV2RecHeader *)cmdBuf;
			payload = (char *)&cmdRecHdr[1];
			payloadW = (uint16_t *)&cmdRecHdr[1];
#if AUTO_DATA_SYNC
			if(isAutoDataSyncPacket)
			{	unixTime = autoDataSyncCtx->startTimeStamp + autoDataSyncCtx->startSlot * 60;
				memcpy(cmdRecHdr->unixTime, (void *)&unixTime, 8);
				if(autoDataSyncCtx->timeInterval == -1)
				{	//timeSpan = 1;
					timeSpan = 2;
					recTimeInterval = 1;
				}
				else
				{	timeSpan = autoDataSyncCtx->slotCnt * autoDataSyncCtx->timeInterval;
					recTimeInterval = autoDataSyncCtx->timeInterval;
				}
				payloadW[0] = timeSpan;
				numOfRec = autoDataSyncCtx->slotCnt;
				numOfByte = autoDataSyncCtx->slotCnt * sensPq->bytesPerRec;
				payloadSize = numOfByte;
			}
			else
#endif
			{	timeSpan = payloadW[0];
				numOfByte = (timeSpan*60)/(sensSysCfg->recordSec)*sensPq->bytesPerRec;
				numOfRec = (timeSpan*60)/(sensSysCfg->recordSec);
			
				dbg_msg("[SETTING] any time record: [%d/%02d/%02d %02d:%02d:%02d], timespan = %d\r\n",date_time1[0], date_time1[1], date_time1[2], date_time1[3], date_time1[4], date_time1[5], timeSpan);
			
				payloadSize = numOfByte;
			}
			sendBuf = SENS_MEM_ALLOC(payloadSize+6+sizeof(SenstalkV2RecHeader)+CRC16_SIZE);
			
			if(sendBuf)
			{	payload = &sendBuf[sizeof(SenstalkV2RecHeader)];
#if AUTO_DATA_SYNC
				if(isAutoDataSyncPacket)
				{	int idx;
					int bufOffset = 6;
					AUTO_DATA_SYNC_STRUCT *autoDataSyncCtx = serverInst->autoDataSyncCtx; 
					int fileOffset;
					int currSlot = autoDataSyncCtx->startSlot;
					int recSizeForV1 = 0;

					if(autoDataSyncCtx->recHeaderVer == FIXED_REC_INTERVAL_VERSION)
						recSizeForV1 = 180;
					
					for(idx=0;idx<numOfRec;idx++)
					{	fileOffset = sizeof(REC_HEADER) - recSizeForV1 + (currSlot * sensPq->bytesPerRec);
						sdReadFile((char *)autoDataSyncCtx->filename, &payload[bufOffset], sensPq->bytesPerRec, fileOffset, NORMAL_READ_MODE);
						currSlot += recTimeInterval;
						bufOffset += sensPq->bytesPerRec;
					}
				}
				else
#endif
				{
#if REC_INTERVAL_FIXED
					int idx1;
					int bufOffset = 6;
					int fileOffset;
					int currSlot;
					char *filename;
					int recTimeInterval = sensSysCfg->recordSec / 60;
					int recSizeForV1 = 0;
					
					filename = SENS_MEM_ZALLOC(64);
					SENS_SPRINTF(filename, "%04d/%02d/%02d%02dn%03d.bin", date_time[0], date_time[1], date_time[1], date_time[2], sensPq->pqNumber);
					
					if(getRecFileVersion(filename) == FIXED_REC_INTERVAL_VERSION)
						recSizeForV1 = 180;
					
					currSlot = (date_time[3] * 3600) / 60;
					if(chkFileExist(filename))
					{	for(idx1=0;idx1<numOfRec;idx1++)
						{	fileOffset = sizeof(REC_HEADER) - recSizeForV1 + (currSlot * sensPq->bytesPerRec);
							sdReadFile((char *)filename, &payload[bufOffset], sensPq->bytesPerRec, fileOffset, NORMAL_READ_MODE);
							currSlot += recTimeInterval;
							bufOffset += sensPq->bytesPerRec;
						}
					}
					else
					{	memset(&payload[bufOffset], 0xFF, payloadSize);
					}
					SENS_MEM_FREE(filename);
#else
					SD_ReadRecNByte(date_time1, (sd.record_sec), da.ai_number, da.byte_per_rec, numOfByte, &payload[6], &payloadSize);
#endif
				}
			}
			else
			{	payloadSize = 0;
				numOfRec = 0;
				sendBuf = SENS_MEM_ALLOC(6+sizeof(SenstalkV2RecHeader)+CRC16_SIZE);
				payload = &sendBuf[sizeof(SenstalkV2RecHeader)];
			}
			
			memcpy(sendBuf, cmdBuf, sizeof(SenstalkV2RecHeader)+2);
			
			payloadW = (uint16_t *)payload;
			payloadW[1] = sensPq->pqNumber;
			payloadW[2] = numOfRec;
			dbg_msg("[SETTING] PQ Item: %d, PQ Record number: %d\r\n", sensPq->pqNumber, numOfRec);
			
			// *re_len += (24 + TIMESPAN_SIZE);                                          // TotalBytes = re_len + command byte
			totalSize = sizeof(SenstalkV2RecHeader) + payloadSize + 6;
		}
		cmdRecHdr = (SenstalkV2RecHeader *)sendBuf;
		cmdRecHdr->size = totalSize - 8;
	}
	else if(func == SENSTALK_FUNC_EVENT_HOUR)                                                       // ---[[read DI status]]--- --> data by hour
	{	uint16_t *payloadW;
			
		cmdRecHdr = (SenstalkV2RecHeader *)cmdBuf;
		memcpy((char *)&unixTime, cmdRecHdr->unixTime, 8);
		
#if defined NUVOTON
		timestamp = (uint32_t)unixTime;
		timestampToDateTime(timestamp, &dateTime);
		dtbuf[0]=dateTime.u32Year; dtbuf[1]=dateTime.u32Month;  dtbuf[2]=dateTime.u32Day;
		dtbuf[3]=dateTime.u32Hour; dtbuf[4]=dateTime.u32Minute; dtbuf[5]=dateTime.u32Second;
#else
		rtctime.SECONDS = (uint_32)unixTime;
		rtctime.MILLISECONDS = 0;
		_rtc_time_to_mqx_date(&rtctime, &date1);                                  // unix time->data  (rtctime1->date1)
		
		dtbuf[0]=date1.YEAR; dtbuf[1]=date1.MONTH;  dtbuf[2]=date1.DAY;
		dtbuf[3]=date1.HOUR; dtbuf[4]=date1.MINUTE; dtbuf[5]=date1.SECOND;
#endif
		for(idx = 0; idx < 6; idx++) date_time[idx] = (int)dtbuf[idx];
		date_time[4]=0;date_time[5]=0;date_time[6]=0;
		
		numOfByte = 0;
		payloadSize = readDiHistory(date_time, DIQUANTITY, NULL, payloadSize, READ_LENGTH_MODE, 0);
		if(payloadSize)
		{	sendBuf = SENS_MEM_ALLOC(payloadSize+4+sizeof(SenstalkV2RecHeader)+CRC16_SIZE);
			if(sendBuf)
			{	payload = &sendBuf[sizeof(SenstalkV2RecHeader)];
				payloadSize = readDiHistory(date_time, DIQUANTITY, &payload[4], payloadSize, NORMAL_READ_MODE, 0);
			}
			else
			{	payloadSize = 0;
				numOfRec = 0;
			}
			//SD_ReadDIhrRec(date_time, DIQUANTITY, re_data+PARSER_HISTORY_HEADER_SIZE+4, re_len, 0);
		}
		if(!payloadSize)
		{	sendBuf = SENS_MEM_ALLOC(6+sizeof(SenstalkV2RecHeader)+CRC16_SIZE);
			payload = &sendBuf[sizeof(SenstalkV2RecHeader)];
		}
		
		numOfRec = payloadSize / 10;
		payloadW = (uint16_t *)payload;
		memcpy(sendBuf, cmdBuf, sizeof(SenstalkV2RecHeader));
		payloadW[0] = DIQUANTITY;
		payloadW[1] = numOfRec;
		totalSize = sizeof(SenstalkV2RecHeader) + payloadSize + 4;
		dbg_msg("[SETTING] DI Item: %d, DI Record number: %d\r\n", DIQUANTITY, numOfRec);
		
		cmdRecHdr = (SenstalkV2RecHeader *)sendBuf;
		cmdRecHdr->size = totalSize - 8;
		/*crc16 = (uint_16)crc((unsigned char *)re_data, 0, *re_len);
		re_data[*re_len] = crc16&0xff;
		re_data[*re_len+1] = crc16>>8;
		*re_len = *re_len + CRC16_SIZE;
		*/
	}
	else
	{	dbg_msg("%s", "unknown func code, skip!!\r\n");
		goto END_OF_PARSER;
	}
	
	crc16 = (uint16_t)swCrc16((unsigned char *)sendBuf, 0, totalSize);
	sendBuf[totalSize + 0] = crc16 & 0xFF;
	sendBuf[totalSize + 1] = crc16 >> 8;
	totalSize += CRC16_SIZE;
	
	dbg_msg("[MESSAGE] V2/Command + Data: %d (bytes)\r\n", totalSize);
	
	struct TaskQMsg msg;
	MOBILE_WRITE_SOCKET_CMD *mobileWriteSocketCmd;
	
	mobileWriteSocketCmd = SENS_MEM_ALLOC(sizeof(MOBILE_WRITE_SOCKET_CMD));
	mobileWriteSocketCmd->buf = (unsigned char *)sendBuf;
	mobileWriteSocketCmd->serverInst = serverInst;
	mobileWriteSocketCmd->bufLen = totalSize;
	mobileWriteSocketCmd->bufType.protoType = PROTOCOL_SENSTALK_V2;
	mobileWriteSocketCmd->bufType.func = func;
	mobileWriteSocketCmd->bufType.dataArrayId = dataArrayId;
	mobileWriteSocketCmd->cmdIsFromInternet = senstalkRecvCmd->generateFromInternet & 0x7F;
	
#ifdef SUPPORT_LORA_WAN
	if(protocolType == PROTOCOL_SENSTALK_V2) 
#endif
	{	if(mobileWriteSocketCmd->cmdIsFromInternet)
		{	serverInst->countOfProcessCmdFromInternet++;
		}
		msg.msgId = NETWORK_Q_MSG_SEND_CMD;
		msg.ptr = (char *)mobileWriteSocketCmd;
		if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
		{	SENS_MEM_FREE(mobileWriteSocketCmd);
		}
	}
#ifdef SUPPORT_LORA_WAN
	else
	{	msg.msgId = MESH_Q_MSG_LORA_WAN_SOCK_SEND;
		msg.ptr = (char *)mobileWriteSocketCmd;
		if(sendMsgWithErrHandle(MESH_TASK, SENS_MSG_Q_SEND(meshQ, (uint_32 *)&msg, 0), __func__, __LINE__))
		{	SENS_MEM_FREE(mobileWriteSocketCmd);
		}
	}
#endif

END_OF_PARSER:
	if(skbQueue)
	{	//SENS_SEM_LOCK(serverInst->sockRecvQueueLock);
		removeSockQueue(serverInst, skbQueue);
		//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
	}
	if((senstalkRecvCmd->generateFromInternet & 0x7F) == 0)
	{	if(cmdBuf)
			SENS_MEM_FREE(cmdBuf);
	}
}