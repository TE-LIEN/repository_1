#include "network/protocol/serverProtocol.h"
#include "network/netApp.h"
#include "cJSON/cJSON.h"
#include "sensCommon.h"
#include "physicalQuantity.h"
#include "sensSystem.h"
#include "sensDev.h"
#include "rtcDateTime.h"
#include "powerCtrl.h"
#include "param.h"
#include "ini2.h"
#include "AutoDataSync.h"
#include "sdCardTask.h"

#if 0
#include <math.h>

#include "pwrLedCtl.h"
#include "param.h"
#include "general.h"
#include "main_task.h"
#include "SDCard_task.h"
#include "sens_device.h"
#include "DArray.h"
#include "sensors/sensors.h"
#ifndef PARAM_IS_JSON
#include "ini2.h"
#endif
#include "AutoDataSync.h"
#endif

#ifdef SUPPORT_MQTT

char tidArray[11];

char f2aBuf[32];

#define STRUCT_ARRAY_SIZE(x)		sizeof(x) / sizeof(x[0])

	//#include "jsmn.h"

	//jsmntok_t jsonToken[MAX_JSON_DECODE_TOKEN];
	//char jsonString[MAX_JSON_DECODE_STR_SIZE];
#if defined NUVOTON
uint64_t rtctimeForSenstalkMqtt;
#else
TIME_STRUCT rtctimeForSenstalkMqtt;
#endif

uint32_t mqttFwVersion;

const struct SENSTALK_JSON_CMD_STR senstalkJsonCmdTable[] = 
{	{"ReIOs", 	STK_READ_IOs},
	{"ReAI", 	STK_READ_AI},
	{"ReDI", 	STK_READ_DI},
	{"ReDO", 	STK_READ_DO},
	{"ReRecs", 	STK_READ_RECS},
	{"WrDO", 	STK_WRITE_DO},
	{"ReSys", 	STK_READ_SYS},
	{"Cnt", 	STK_CONNECT},	//a4 send to server
	{"ReCfg", 	STK_READ_CFG},
	{"WrCfg", 	STK_WRITE_CFG},
	{"Rbt", 	STK_REBOOT},
	{"ReLCfg",	STK_READ_L_CFG},
	{"WrLCfg",	STK_WRITE_L_CFG},
	//{"Slp", 		STK_SLEEP},
};

struct SENSTALK_JSON_SUB_CMD_STR iotpsTable[] = 
{	//*name			subcmd														*value   unit
	{"AI", 		SENSTALK_MQTT_SUB_CMD_AI, 				0, UNIT_FLOAT},
	{"DI", 		SENSTALK_MQTT_SUB_CMD_DI, 				0, UNIT_BOOL},
	{"DO", 		SENSTALK_MQTT_SUB_CMD_DO, 				0, UNIT_BOOL},
	{"Temp", 	SENSTALK_MQTT_SUB_CMD_TEMP, 			0, UNIT_FLOAT},
	{"Time", 	SENSTALK_MQTT_SUB_CMD_TIME, 			0, UNIT_UINT},
	{"RbtCt", 	SENSTALK_MQTT_SUB_CMD_RBTCT, 			0, UNIT_INT},
	{"BatV", 	SENSTALK_MQTT_SUB_CMD_BATV, 			0, UNIT_FLOAT},
	{"ExtV", 	SENSTALK_MQTT_SUB_CMD_EXTV, 			0, UNIT_FLOAT},
	{"SI4G", 	SENSTALK_MQTT_SUB_CMD_4GSI, 			0, UNIT_INT},
	{"SINB", 	SENSTALK_MQTT_SUB_CMD_NBSI, 			0, UNIT_INT},
	{"RecIv",	SENSTALK_MQTT_SUB_CMD_RECIV, 			0, UNIT_LONG},
	{"Slp", 	SENSTALK_MQTT_SUB_CMD_SLP, 				0, UNIT_BOOL},
	{"FWVer", 	SENSTALK_MQTT_SUB_CMD_FW_VER, 			0, UNIT_HEX_STRING},
	{"Time", 	SENSTALK_MQTT_SUB_CMD_TIME_CALIBRATION, 0, UNIT_LONG},
	{"FOTATri",	SENSTALK_MQTT_SUB_CMD_FW_OTA,			0, UNIT_BOOL},	//is not support read always false
//#if SUPPORT_AUTO_FOTA
//	{"FOTAAU",	SENSTALK_MQTT_SUB_CMD_FW_OTA_AUTO,			0, UNIT_BOOL},	//is not support read always false
//#endif
};

struct SENSTALK_JSON_WRITE_SUB_CMD_STR wCfgTable[] =
{	{"RecIv",	SENSTALK_MQTT_SUB_CMD_RECIV, 			WR_CFG_POS_REC_IV,		0, UNIT_LONG},
	{"Slp", 	SENSTALK_MQTT_SUB_CMD_SLP, 				WR_CFG_POS_SLP,			0, UNIT_BOOL},
	{"Time", 	SENSTALK_MQTT_SUB_CMD_TIME_CALIBRATION, WR_CFG_POS_TIME,		0, UNIT_LONG},
	{"FOTATri",	SENSTALK_MQTT_SUB_CMD_FW_OTA, 			WR_CFG_POS_AUTO_OTA,	0, UNIT_BOOL},
};
//------------------------------------------------------------------------------
int SIZE_OF_WR_CFG(void)
{	return STRUCT_ARRAY_SIZE(wCfgTable);
}
//------------------------------------------------------------------------------
static void reScheduleRecTypeCmd(SERVER_CMD_STRUCT *servCmd, int dt)
{	struct TaskQMsg senstalkMsg;
	SERVER_CMD_STRUCT *reScheServCmd;
			
	reScheServCmd = SENS_MEM_ZALLOC(sizeof(SERVER_CMD_STRUCT));
	memcpy(reScheServCmd, servCmd, sizeof(SERVER_CMD_STRUCT));
	reScheServCmd->startDt = dt;
	/*if(servCmd->subCmdType != SENSTALK_MQTT_SUB_CMD_AI)
	{	reScheServCmd->recDiOffset = servCmd->recDiOffset;
		//reScheServCmd->startDt = dt;
	}*/	
	senstalkMsg.msgId = SERV_CMD_PROCESS_Q_MSG_SEND_CMD;
	senstalkMsg.ptr = (char *)reScheServCmd;
	
	if(sendMsgWithErrHandle(SERV_CMD_PROCESS_TASK, SENS_MSG_Q_SEND(servCmdProcessQ, (uint32_t *)&senstalkMsg, 0), __func__, __LINE__))
	{	SENS_MEM_FREE(reScheServCmd);
	}
}
//------------------------------------------------------------------------------
/*static void getJsonString(char *json, char *jsonStrBuf, jsmntok_t *tok)
{	memset(jsonStrBuf, 0, MAX_JSON_DECODE_STR_SIZE);
	memcpy(jsonStrBuf, json+tok->start, tok->end-tok->start);
}*/
//------------------------------------------------------------------------------
void setSenstalkMqttPubTopic(MQTT_SEND_INFO *mqttSendMsg, MQTTCtx *mqttCtx)
{	mqttSendMsg->sendTopic = SENS_MEM_ZALLOC(strlen(mqttCtx->topic_name)+1);
	memcpy(mqttSendMsg->sendTopic, mqttCtx->topic_name, strlen(mqttCtx->topic_name));
}
//------------------------------------------------------------------------------
MQTT_SEND_INFO *jsonToMqttMsg(cJSON *jsonObj, MQTTCtx *mqttCtx)
{	MQTT_SEND_INFO *mqttSendMsg;
	CLOUD_SERVER_INSTANCE *serverInst;
	
	serverInst = getServerInstFromMqtt(mqttCtx);
	
	mqttSendMsg = SENS_MEM_ZALLOC(sizeof(MQTT_SEND_INFO));
	mqttSendMsg->sendMessage = cJSON_PrintUnformatted(jsonObj);
	mqttSendMsg->sendMessageLen = strlen(mqttSendMsg->sendMessage);
	setSenstalkMqttPubTopic(mqttSendMsg, mqttCtx);
	serverInst->mqttSendMsg = mqttSendMsg;
	return mqttSendMsg;
}
//------------------------------------------------------------------------------
static void autoSendToSenslink(SERVER_CMD_STRUCT *servCmd)
{	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	CLOUD_SERVER_INSTANCE *serverInst;
	cJSON *jsonObj=NULL;
	cJSON *jsonPqArray=NULL;
	uint32_t ts = dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
	//20 02 00 05
#ifdef OTA_STOP_ALL_CONNECTION
	if(networkCtx->otaRunning)
		return;
#endif

	serverInst = getServerInstFromMqtt(mqttCtx);

	if((mqttCtx->stat >= WMQ_WAIT_MSG) && (mqttCtx->stat < WMQ_DISCONNECT))
	{	jsonObj = cJSON_CreateObject();
		cJSON_AddNumberToObject(jsonObj, "Ver", 3);
		//cJSON_AddNumberToObject(jsonObj, "Tid", NAN);
		//cJSON_AddNumberToObject(jsonObj, "Tid", 0);
		cJSON_AddNumberToObject(jsonObj, "Tid", ts);	//use timestamp
		cJSON_AddStringToObject(jsonObj, "Cmd", "ReAI");
		jsonPqArray = cJSON_CreateFloatArray((const float *)sensPq->pqVal, sensPq->pqNumber);
		cJSON_AddItemToObject(jsonObj, "Vs", jsonPqArray);
		jsonToMqttMsg(jsonObj, mqttCtx);
		
		if(mqttPublishJson(TASK_PROCESS_TYPE_SENSTALK, servCmd, NULL, 1, 0))
		{	cloudDataSendDoneProcess(serverInst);
			networkCtx->send1FrameToCloudBmp |= (1 << serverInst->servIdx);
			sysCtrl->sendToCloudDone = 1;
			resetServIdleXmitTimer(1 << serverInst->servIdx);
		}
		serverInst->netSendFsm = NET_SEND_FSM_SEND_PQ;
		cJSON_Delete(jsonObj);
		clearMqttSendMsg(serverInst);
	}
}
//------------------------------------------------------------------------------
static void connectMsgSendToSenslink(SERVER_CMD_STRUCT *servCmd)
{	char fwVersionStr[9];
	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	cJSON *jsonObj=NULL;
	CLOUD_SERVER_INSTANCE *serverInst = getServerInstFromMqtt(mqttCtx);
	
	if(mqttCtx->stat >= WMQ_WAIT_MSG && serverInst->netSendFsm == NET_SEND_FSM_IDLE && serverInst->serverType < MQTTS_ANIOT_BROKER)
	{	jsonObj = cJSON_CreateObject();
		cJSON_AddNumberToObject(jsonObj, "Ver", 3);
		cJSON_AddStringToObject(jsonObj, "Cmd", "Cnt");
		memset(fwVersionStr, 0, 9);
		SENS_SPRINTF(fwVersionStr, "%X", FW_VERSION);
		cJSON_AddStringToObject(jsonObj, "FWVer", fwVersionStr);
		jsonToMqttMsg(jsonObj, mqttCtx);
		
		if(mqttPublishJson(TASK_PROCESS_TYPE_SENSTALK, servCmd, NULL, 1, 0))
		{	
		}
		serverInst->netSendFsm = NET_SEND_FSM_SEND_FIRST_PQ;
		cJSON_Delete(jsonObj);
		clearMqttSendMsg(serverInst);
	}
}
//------------------------------------------------------------------------------
static void readSetIosToSenslink(SERVER_CMD_STRUCT *servCmd)
{	int idx;
	uint8_t arraySize = 0;
	cJSON *jsonObj = NULL;
	cJSON *jsonDiArray = NULL;
	cJSON *jsonPqArray = NULL;
	cJSON *jsonDoArray = NULL;
	cJSON *jsonVs = NULL;
	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	CLOUD_SERVER_INSTANCE *serverInst = getServerInstFromMqtt(mqttCtx);
	char strTemp[11];
	DI_INSTANCE *diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT	*diCtx = (DI_CONTEXT	*)&diInst->diCtx;
	DO_INSTANCE *doInst = (DO_INSTANCE *)&sensDev->doInst;
	DO_CONTEXT *doCtx = (DO_CONTEXT *)&doInst->doCtx;
	
	if(mqttCtx->stat >= WMQ_WAIT_MSG)
	{	if(servCmd->cmdType == STK_READ_AI)
			servCmd->subCmdType |= SENSTALK_MQTT_SUB_CMD_AI;
		if(servCmd->cmdType == STK_READ_DI)	
			servCmd->subCmdType |= SENSTALK_MQTT_SUB_CMD_DI;
		if(servCmd->cmdType == STK_READ_DO || servCmd->cmdType == STK_WRITE_DO)	
			servCmd->subCmdType |= SENSTALK_MQTT_SUB_CMD_DO;
		
		if((servCmd->subCmdType & SENSTALK_MQTT_SUB_CMD_AI) == SENSTALK_MQTT_SUB_CMD_AI)
		{	jsonPqArray = cJSON_CreateFloatArray((const float *)sensPq->pqVal, sensPq->pqNumber);
			arraySize++;
		}
		
		if((servCmd->subCmdType & SENSTALK_MQTT_SUB_CMD_DI) == SENSTALK_MQTT_SUB_CMD_DI)
		{	jsonDiArray = cJSON_CreateArray();
			for(idx=0;idx<DIQUANTITY;idx++)
				cJSON_AddItemToArray(jsonDiArray	, cJSON_CreateNumber((diCtx->diStsNew[idx])? 1:0));
			arraySize++;
		}
		if((servCmd->subCmdType & SENSTALK_MQTT_SUB_CMD_DO) == SENSTALK_MQTT_SUB_CMD_DO)
		{	jsonDoArray = cJSON_CreateArray();
			for(idx=0;idx<DO_WIRED_QUANTITY;idx++)
				cJSON_AddItemToArray(jsonDoArray	, cJSON_CreateNumber((doCtx->doSts[idx])? 1:0));
			arraySize++;
		}
		
		jsonObj = cJSON_CreateObject();
		cJSON_AddNumberToObject(jsonObj, "Ver", 3);
		
#if defined MQTT_TID_IS_INTEGER
		cJSON_AddNumberToObject(jsonObj, "Tid", servCmd->tid);
#else
		memset(strTemp, 0 , 11);
		SENS_SPRINTF(strTemp, "%llu", servCmd->tid);
		cJSON_AddStringToObject(jsonObj, "Tid", strTemp);
#endif
		memset(strTemp, 0, 11);
		if(servCmd->cmdType == STK_READ_IOs)
			strcat(strTemp, "ReIOs");
		else if(servCmd->cmdType == STK_READ_AI)
			strcat(strTemp, "ReAI");
		else if(servCmd->cmdType == STK_READ_DI)
			strcat(strTemp, "ReDI");
		else if(servCmd->cmdType == STK_READ_DO)
			strcat(strTemp, "ReDO");
		else 
			strcat(strTemp, "WrDO");
		cJSON_AddStringToObject(jsonObj, "Cmd", strTemp);
		if(arraySize > 1)
		{	jsonVs = cJSON_CreateObject();
			if(jsonPqArray)
				cJSON_AddItemToObject(jsonVs, "AI", jsonPqArray);
			if(jsonDiArray)
				cJSON_AddItemToObject(jsonVs, "DI", jsonDiArray);
			if(jsonDoArray)
				cJSON_AddItemToObject(jsonVs, "DO", jsonDoArray);
		}
		else
		{	if(jsonPqArray)
				jsonVs = jsonPqArray;
			else if(jsonDiArray)
				jsonVs = jsonDiArray;
			else if(jsonDoArray)
				jsonVs = jsonDoArray;
		}	
		cJSON_AddItemToObject(jsonObj, "Vs", jsonVs);
		jsonToMqttMsg(jsonObj, mqttCtx);
		mqttPublishJson(TASK_PROCESS_TYPE_SENSTALK, servCmd, NULL, 1, 0);
		cJSON_Delete(jsonObj);
		clearMqttSendMsg(serverInst);
	}
	else
		reQueueSenstalkCmd(TASK_PROCESS_TYPE_SENSTALK, servCmd, 200, __LINE__);
}
//------------------------------------------------------------------------------
static void mqttWriteDo(SERVER_CMD_STRUCT *servCmd)
{	DO_CONFIG *doCfg = (DO_CONFIG *)&sysCfg->doCfg;
#if defined PARAM_IS_JSON
	cJSON *objJson;
#else
	MiniFile *mIniFile;
#endif
	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;

	if(mqttCtx->stat >= WMQ_WAIT_MSG)
	{	if(doCfg->mode == 0 )
		{	if(servCmd->cfgInfoValid[0])
			{	doCfg->powerCtrl[0] = servCmd->cfgInfo[0];
			}
			if(servCmd->cfgInfoValid[0])
			{	doCfg->powerCtrl[1] = servCmd->cfgInfo[1];
			}
			
#if TEST_N_REMOVE
			do_output_setting();
#endif
		}
END_OF_SET_DO:
		readSetIosToSenslink(servCmd);
	}
	else
		reQueueSenstalkCmd(TASK_PROCESS_TYPE_SENSTALK, servCmd, 200, __LINE__);
}
//------------------------------------------------------------------------------
static void readSysToSenslink(SERVER_CMD_STRUCT *servCmd)
{	
#ifndef NUVOTON
	DATE_STRUCT date1;
#endif
	int idx;
	char strBuf[32];
	cJSON *jsonObj;
	cJSON *sysItemObj;
	cJSON *sysArray;
	uint8_t isNan;
	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	CLOUD_SERVER_INSTANCE *serverInst = getServerInstFromMqtt(mqttCtx);
	
	if(mqttCtx->stat >= WMQ_WAIT_MSG)
	{	if((servCmd->subCmdType & SENSTALK_MQTT_SUB_CMD_ALL_SYS) == 0x00)
			servCmd->subCmdType |= SENSTALK_MQTT_SUB_CMD_ALL_SYS;
#if defined NUVOTON
		rtctimeForSenstalkMqtt = (uint64_t)dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
#else
		date1.YEAR=sensSys->date_time[0];date1.MONTH=sensSys->date_time[1];date1.DAY=sensSys->date_time[2];
		date1.HOUR=sensSys->date_time[3];date1.MINUTE=sensSys->date_time[4];date1.SECOND=sensSys->date_time[5];
		date1.MILLISEC = 0;
		_rtc_time_from_mqx_date(&date1, &rtctimeForSenstalkMqtt);
#endif
		sysArray = cJSON_CreateArray();
		for(idx=SENSTALK_MQTT_SUB_CMD_TEMP_IDX;idx<=SENSTALK_MQTT_SUB_CMD_NBSI_IDX;idx++)
		{	if((servCmd->subCmdType & iotpsTable[idx].subcmd) == iotpsTable[idx].subcmd)
			{	isNan = 0;
				memset(strBuf, 0, 32);
				sysItemObj = cJSON_CreateObject();
				if(iotpsTable[idx].unit == UNIT_FLOAT || iotpsTable[idx].unit == UNIT_INT)
				{	if(iotpsTable[idx].unit == UNIT_INT)
					{	if((servCmd->subCmdType & iotpsTable[idx].subcmd) == SENSTALK_MQTT_SUB_CMD_4GSI)
						{	if(SENS_IS_NAN(sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH]))
							{	isNan = 1;
							}
						}
						if((servCmd->subCmdType & iotpsTable[idx].subcmd) == SENSTALK_MQTT_SUB_CMD_NBSI)
						{	if(SENS_IS_NAN(sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH]))
								isNan = 1;
						}
					}
				}
				
				if(isNan)
					cJSON_AddStringToObject(sysItemObj, iotpsTable[idx].name, "NaN");
				else
					cJSON_AddNumberToObject(sysItemObj, iotpsTable[idx].name, *(unsigned int *)iotpsTable[idx].value);
				cJSON_AddItemToArray(sysArray, sysItemObj);
			}
		}
		jsonObj = cJSON_CreateObject();
		cJSON_AddNumberToObject(jsonObj, "Ver", 3);
#if defined MQTT_TID_IS_INTEGER
		cJSON_AddNumberToObject(jsonObj, "Tid", servCmd->tid);
#else
		memset(strBuf, 0 , 11);
		SENS_SPRINTF(strBuf, "%llu", servCmd->tid);
		cJSON_AddStringToObject(jsonObj, "Tid", strBuf);
#endif
		cJSON_AddStringToObject(jsonObj, "Cmd", "ReSys");
		cJSON_AddItemToObject(jsonObj, "Sys", sysArray);
		jsonToMqttMsg(jsonObj, mqttCtx);
		mqttPublishJson(TASK_PROCESS_TYPE_SENSTALK, servCmd, NULL, 1, 0);
		cJSON_Delete(jsonObj);
		clearMqttSendMsg(serverInst);
	}
	else
		reQueueSenstalkCmd(TASK_PROCESS_TYPE_SENSTALK, servCmd, 200, __LINE__);
}
//------------------------------------------------------------------------------
static int readCfgToSenslink(SERVER_CMD_STRUCT *servCmd)
{	cJSON *jsonObj;
	cJSON *cfgJsonObj;
	int idx;
#ifndef NUVOTON
	DATE_STRUCT date1;
#endif
	//char strBuf[11];
	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	CLOUD_SERVER_INSTANCE *serverInst = getServerInstFromMqtt(mqttCtx);
	
	if(mqttCtx->stat >= WMQ_WAIT_MSG)
	{	if(servCmd->cmdType == STK_READ_CFG)
		{	if((servCmd->subCmdType & SENSTALK_MQTT_SUB_CMD_ALL_CFG) == 0x00)
			{	servCmd->subCmdType |= SENSTALK_MQTT_SUB_CMD_ALL_CFG;	//only read use
			}
		}
		else
			servCmd->subCmdType &= ~(SENSTALK_MQTT_SUB_CMD_FW_VER);
		
		cfgJsonObj = cJSON_CreateObject();
		
		for(idx=SENSTALK_MQTT_SUB_CMD_RECIV_IDX;idx<=SENSTALK_MQTT_SUB_CMD_FW_OTA_IDX;idx++)
		{	if((servCmd->subCmdType & iotpsTable[idx].subcmd) == iotpsTable[idx].subcmd)
			{	if((iotpsTable[idx].unit == UNIT_LONG))
				{	long *lVal = (long *)iotpsTable[idx].value;
					if(iotpsTable[idx].subcmd == SENSTALK_MQTT_SUB_CMD_TIME_CALIBRATION)
					{	
#if defined NUVOTON
						rtctimeForSenstalkMqtt = (uint64_t)dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
#else
						date1.YEAR = sensSys->date_time[0];	date1.MONTH = sensSys->date_time[1];	date1.DAY = sensSys->date_time[2];
						date1.HOUR = sensSys->date_time[3];	date1.MINUTE = sensSys->date_time[4];	date1.SECOND = sensSys->date_time[5];
						_rtc_time_from_mqx_date(&date1, &rtctimeForSenstalkMqtt);
#endif
					}
					cJSON_AddNumberToObject(cfgJsonObj, iotpsTable[idx].name, *lVal);
				}
				else if(iotpsTable[idx].unit == UNIT_BOOL)
				{	int *iVal = (int *)iotpsTable[idx].value;
					cJSON_AddNumberToObject(cfgJsonObj, iotpsTable[idx].name, (*iVal == 1)? 1:0);
				}
				else if(iotpsTable[idx].unit == UNIT_HEX_STRING)
				{	int *iVal = (int *)iotpsTable[idx].value;
					char verStr[9];
					memset(verStr, 0, 9);
					SENS_SPRINTF(verStr, "%X", *iVal);
					cJSON_AddStringToObject(cfgJsonObj, iotpsTable[idx].name, verStr);
				}
			}
		}
		jsonObj = cJSON_CreateObject();
		cJSON_AddNumberToObject(jsonObj, "Ver", 3);
#if defined MQTT_TID_IS_INTEGER
		cJSON_AddNumberToObject(jsonObj, "Tid", servCmd->tid);
#else
		memset(strBuf, 0 , 11);
		SENS_SPRINTF(strBuf, "%llu", servCmd->tid);
		cJSON_AddStringToObject(jsonObj, "Tid", strBuf);
#endif
		cJSON_AddStringToObject(jsonObj, "Cmd", (servCmd->cmdType == STK_READ_CFG)? "ReCfg":"WrCfg");
		cJSON_AddItemToObject(jsonObj, "Cfgs", cfgJsonObj);
		jsonToMqttMsg(jsonObj, mqttCtx);
		mqttPublishJson(TASK_PROCESS_TYPE_SENSTALK, servCmd, NULL, 1, 0);
		cJSON_Delete(jsonObj);
		clearMqttSendMsg(serverInst);
		return 0;
	}
	else
	{	reQueueSenstalkCmd(TASK_PROCESS_TYPE_SENSTALK, servCmd, 200, __LINE__);
		return 1;
	}
}
//------------------------------------------------------------------------------
static void readRecordToSenslink(SERVER_CMD_STRUCT *servCmd)
{	cJSON *jsonObj;
	cJSON *recsJsonObj = NULL;
	cJSON *singlePqsJsonArray = NULL;
	int idx;
	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	CLOUD_SERVER_INSTANCE *serverInst = getServerInstFromMqtt(mqttCtx);
	int endDt = servCmd->startDt;
	int realNumOfRec=0;
	uint64_t unixTime = servCmd->startDt;
	int dateTime[7];
#if defined NUVOTON
	S_RTC_TIME_DATA_T date;
#else
	DATE_STRUCT date;
	TIME_STRUCT rtctime;
#endif
	int currSlot;
	int startSlot;
	int fileOffset;
	char filename[32];
	uint32_t utcOfRec;
	uint32_t nextUtcOfRec;
	uint32_t startXmitDtAlign;
	char *recBuf = NULL;
	float *pqVal;
	char timestampStr[16];
	char *tempJsonStr;
	char reSchedule=0;
	SENSTALK_DI_REC_FORMAT *diFmt;
	int diRecRdLen;
	int diRecRealRdLen;
	int readFileLen;
	int recSizeForV1 = 0;
	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	//int only1DiRec;
	
	if(mqttCtx->stat < WMQ_WAIT_MSG)
	{	reQueueSenstalkCmd(TASK_PROCESS_TYPE_SENSTALK, servCmd, 200, __LINE__);
		return;
	}
  
	if(servCmd->subCmdType == SENSTALK_MQTT_SUB_CMD_AI)
	{	//totalDt = servCmd->endDt - ROUNDUP(servCmd->startDt, sd.record_sec);
		dbg_msg("%s, recordSec:%d, bytesPerRec:%d, edt:%d, sdt:%d\r\n", __func__, sensSysCfg->recordSec, sensPq->bytesPerRec, servCmd->endDt, ROUNDUP(servCmd->startDt, sensSysCfg->recordSec));
	}
	else
	{	dbg_msg("%s, recordSec:%d, bytesPerRec:%d, edt:%d, sdt:%d, offset:%d\r\n", __func__, sensSysCfg->recordSec, sensPq->bytesPerRec, servCmd->endDt, servCmd->startDt, servCmd->recDiOffset);
	}
  
	if(servCmd->subCmdType == SENSTALK_MQTT_SUB_CMD_AI)
	{	startXmitDtAlign = ROUNDUP(unixTime, sensSysCfg->recordSec);
  	
		while(startXmitDtAlign <= servCmd->endDt)
		{	utcOfRec = ROUNDDOWN(startXmitDtAlign, 86400);
			nextUtcOfRec = utcOfRec + 86400;
			
#if defined NUVOTON
			timestampToDateTime(utcOfRec, &date);
			dateTime[0]=date.u32Year;	dateTime[1]=date.u32Month;	dateTime[2]=date.u32Day;
			dateTime[3]=date.u32Hour;	dateTime[4]=date.u32Minute;	dateTime[5]=date.u32Second;
#else
			rtctime.SECONDS  = (uint_32)utcOfRec;
			rtctime.MILLISECONDS = 0;

			_rtc_time_to_mqx_date(&rtctime, &date);
  
			dateTime[0]=date.YEAR;	dateTime[1]=date.MONTH;		dateTime[2]=date.DAY;
			dateTime[3]=date.HOUR;	dateTime[4]=date.MINUTE;	dateTime[5]=date.SECOND;    
#endif
			SENS_SPRINTF(filename, "%04d/%02d/%02d%02dn%03d.bin", dateTime[0], dateTime[1], dateTime[1], dateTime[2], sensPq->pqNumber);
			
			startSlot = startXmitDtAlign % 86400;
			if(recBuf == NULL)
				recBuf = SENS_MEM_ZALLOC(sensPq->bytesPerRec);
			recSizeForV1 = 0;
			if(getRecFileVersion(filename) == FIXED_REC_INTERVAL_VERSION)
				recSizeForV1 = 180;
			
			while(startXmitDtAlign < nextUtcOfRec)	//check timestamp less next day UTC-0 timestamp
			{	currSlot = startSlot / 60;
				if(isFileExist(filename))
				{	fileOffset = sizeof(REC_HEADER) - recSizeForV1 + (currSlot * sensPq->bytesPerRec);
					sdReadFile((char *)filename, recBuf, sensPq->bytesPerRec, fileOffset, NORMAL_READ_MODE);
				}
				else
				{	memset(recBuf, 0xFF, sensPq->bytesPerRec);
				}
				pqVal = (float *)recBuf;
			
				if(servCmd->rspType)
				{	if(recsJsonObj == NULL)
						recsJsonObj = cJSON_CreateArray();
					for(idx=0;idx<sensPq->pqNumber;idx++)
						cJSON_AddItemToArray(recsJsonObj, cJSON_CreateFloat(pqVal[idx]));
				}
				else
				{	if(recsJsonObj == NULL)
						recsJsonObj = cJSON_CreateObject();
					singlePqsJsonArray = cJSON_CreateFloatArray(pqVal, sensPq->pqNumber);
					memset(timestampStr, 0, 16);
					SENS_SPRINTF(timestampStr, "%u", startXmitDtAlign);
					cJSON_AddItemToObject(recsJsonObj, timestampStr, singlePqsJsonArray);
				}
			
				tempJsonStr = cJSON_PrintUnformatted(recsJsonObj);
				
				
				startXmitDtAlign += sensSysCfg->recordSec;
				endDt = startXmitDtAlign;
				startSlot += sensSysCfg->recordSec;
				realNumOfRec++;
			
				if(strlen(tempJsonStr) > (4096-512))	//max 4K byte data
				{	reSchedule = 1;
				}
				SENS_MEM_FREE(tempJsonStr);
				if(reSchedule)
					break;
				if(startXmitDtAlign > servCmd->endDt)	//timestamp over end timestamp
					break;
			}
			if(reSchedule)
			break;
		}
		if(reSchedule && (startXmitDtAlign <= servCmd->endDt))
			reScheduleRecTypeCmd(servCmd, startXmitDtAlign);
	}
	else	//DI rec
	{	
#if defined NUVOTON
		timestampToDateTime(unixTime, &date);
		dateTime[0]=date.u32Year;	dateTime[1]=date.u32Month;	dateTime[2]=date.u32Day;
		dateTime[3]=date.u32Hour;	dateTime[4]=date.u32Minute;	dateTime[5]=date.u32Second;
#else
		rtctime.SECONDS  = (uint_32)unixTime;
		rtctime.MILLISECONDS = 0;

		_rtc_time_to_mqx_date(&rtctime, &date);
  
		dateTime[0]=date.YEAR;	dateTime[1]=date.MONTH;		dateTime[2]=date.DAY;
		dateTime[3]=date.HOUR;	dateTime[4]=date.MINUTE;	dateTime[5]=date.SECOND;    
#endif
		dbg_msg("[SETTING] record: [%d/%02d/%02d H%02d M%02d]\r\n", dateTime[0], dateTime[1], dateTime[2], dateTime[3], dateTime[4]);
		
		readFileLen = 0;
		readFileLen = readDiHistory(dateTime, DIQUANTITY, NULL, readFileLen, READ_LENGTH_MODE, servCmd->recDiOffset);
		while(servCmd->recDiOffset < readFileLen)
		{	if(recBuf == NULL)
				recBuf = SENS_MEM_ZALLOC(sizeof(SENSTALK_DI_REC_FORMAT));
			diFmt = (SENSTALK_DI_REC_FORMAT *)recBuf;
			diRecRdLen = sizeof(SENSTALK_DI_REC_FORMAT);
			
			diRecRealRdLen = readDiHistory(dateTime, DIQUANTITY, recBuf, diRecRdLen, NORMAL_READ_MODE, servCmd->recDiOffset);
			if(diRecRdLen != diRecRealRdLen)
			{	if(recsJsonObj)
					cJSON_Delete(recsJsonObj);
				SENS_MEM_FREE(recBuf);
				return;
			}

			if(recsJsonObj == NULL)
				recsJsonObj = cJSON_CreateObject();
			singlePqsJsonArray = cJSON_CreateArray();
			for(idx=0;idx<DIQUANTITY;idx++)
			{	if(diFmt->status & (0x1<<(7-idx)))
					cJSON_AddItemToArray(singlePqsJsonArray, cJSON_CreateNumber(1));
				else
					cJSON_AddItemToArray(singlePqsJsonArray, cJSON_CreateNumber(0));
			}
			memset(timestampStr, 0, 16);
			SENS_SPRINTF(timestampStr, "%u", diFmt->unixTime);
			cJSON_AddItemToObject(recsJsonObj, timestampStr, singlePqsJsonArray);
			
			tempJsonStr = cJSON_PrintUnformatted(recsJsonObj);
			
			servCmd->recDiOffset += sizeof(SENSTALK_DI_REC_FORMAT);
			endDt = diFmt->unixTime;
			
			if(strlen(tempJsonStr) > (4096-512) && (servCmd->recDiOffset < readFileLen))	//max 4K byte data
			{	diRecRealRdLen = readDiHistory(dateTime, DIQUANTITY, recBuf, diRecRdLen, NORMAL_READ_MODE, servCmd->recDiOffset);
				reSchedule = 1;
			}
			SENS_MEM_FREE(tempJsonStr);
			
			if(reSchedule)
					break;
		}
		
		if(reSchedule)
			reScheduleRecTypeCmd(servCmd, diFmt->unixTime);
	}
	
	jsonObj = cJSON_CreateObject();
	cJSON_AddNumberToObject(jsonObj, "Ver", 3);
	cJSON_AddNumberToObject(jsonObj, "Tid", servCmd->tid);
	cJSON_AddStringToObject(jsonObj, "Cmd", "ReRecs");
	if(servCmd->subCmdType == SENSTALK_MQTT_SUB_CMD_AI)
		cJSON_AddStringToObject(jsonObj, "IOTp", "AI");
	else
		cJSON_AddStringToObject(jsonObj, "IOTp", "DI");
	cJSON_AddNumberToObject(jsonObj, "HRv", servCmd->rspType);
	cJSON_AddNumberToObject(jsonObj, "SDt", servCmd->startDt);
	cJSON_AddNumberToObject(jsonObj, "EDt", endDt);
	if(servCmd->rspType)
	{	cJSON_AddNumberToObject(jsonObj, "Iv", sensSysCfg->recordSec);
		cJSON_AddNumberToObject(jsonObj, "NPq", sensPq->pqNumber);
		cJSON_AddNumberToObject(jsonObj, "NRec", realNumOfRec);
		cJSON_AddItemToObject(jsonObj, "Vs", recsJsonObj);
	}
	else
	{ if((recsJsonObj == NULL) && (servCmd->subCmdType != SENSTALK_MQTT_SUB_CMD_AI))
      recsJsonObj = cJSON_CreateObject();
    
    cJSON_AddItemToObject(jsonObj, "Recs", recsJsonObj);
  }
	
	jsonToMqttMsg(jsonObj, mqttCtx);
	mqttPublishJson(TASK_PROCESS_TYPE_SENSTALK, servCmd, NULL, 1, 0);
	cJSON_Delete(jsonObj);
	clearMqttSendMsg(serverInst);
	SENS_MEM_FREE(recBuf);	
}
//------------------------------------------------------------------------------
static void controlTypeToSenslink(SERVER_CMD_STRUCT *servCmd)
{	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	cJSON *jsonObj;
	CLOUD_SERVER_INSTANCE *serverInst = getServerInstFromMqtt(mqttCtx);
	char strBuf[11];
	
	if(mqttCtx->stat >= WMQ_WAIT_MSG)
	{	jsonObj = cJSON_CreateObject();
		cJSON_AddNumberToObject(jsonObj, "Ver", 3);
		cJSON_AddStringToObject(jsonObj, "Cmd", (servCmd->cmdType == STK_REBOOT)? "Rbt":"Slp");
  	
		if(servCmd->cmdType != STK_SLEEP_AUTOSEND)
		{	memset(strBuf, 0 , 11);
			SENS_SPRINTF(strBuf, "%llu", servCmd->tid);
			cJSON_AddStringToObject(jsonObj, "Tid", strBuf);
		
  		if(servCmd->cmdType == STK_REBOOT)
  			cJSON_AddNumberToObject(jsonObj, "RbtCt", sensPq->onboardPq[ON_BOARD_PQ_REBOOT_TIMES]);
  	}
  	jsonToMqttMsg(jsonObj, mqttCtx);
  	mqttPublishJson(TASK_PROCESS_TYPE_SENSTALK, servCmd, NULL, 1, 0);
  	if(servCmd->cmdType == STK_REBOOT)
  		sysCtrl->isReadyToReboot = 1;
    else
    {	//sysPwrOffCtrl(sysCtrl->sysStandbyTime); //nothing to do. don't support this command
    }
  }
  else
  	reQueueSenstalkCmd(TASK_PROCESS_TYPE_SENSTALK, servCmd, 200, __LINE__);
}
//------------------------------------------------------------------------------
int getSringValueByKeyFromObj(cJSON *cJsonSrc, char *key, char **val)
{	cJSON *obj;
	
	obj = cJSON_GetObjectItem(cJsonSrc, key);
	if(obj)
	{	*val = cJSON_GetStringValue(obj);
	}
	else
		return -1;
	return 0;
}
//------------------------------------------------------------------------------
int getLongValueByKeyFromObj(cJSON *cJsonSrc, char *key, uint64_t *val)
{	cJSON *obj;
	
	obj = cJSON_GetObjectItem(cJsonSrc, key);
	if(obj)
	{	if(cJSON_IsNumber(obj))
			*val = (uint64_t)cJSON_GetNumberValue(obj);
		else
			return -1;
	}
	else
		return -1;
	return 0;
}
//------------------------------------------------------------------------------
static int getIntValueByKeyFromObj(cJSON *cJsonSrc, char *key, int *val)
{	cJSON *obj;
	
	obj = cJSON_GetObjectItem(cJsonSrc, key);
	if(obj)
	{	if(cJSON_IsNumber(obj))
			*val = (int)cJSON_GetNumberValue(obj);
		else
			return -1;
	}
	else
		return -1;
	return 0;
}
//------------------------------------------------------------------------------
static void decodeReiosResysReCfgJson(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj)
{	cJSON *jsonTemp = NULL;
	cJSON *arrayItem;
	int arraySize;
	int arrayIdx;
	char *jsonStr;
	int iotpsIdx;
	int iotpsStart;
	
	if(servCmd->cmdType == STK_READ_IOs)
	{	if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_IOTps))
		{	jsonTemp = cJSON_GetObjectItem(jsonObj, SENSTALK_JSON_STR_IOTps);
			iotpsStart = 0;
		}
	}
	if(servCmd->cmdType == STK_READ_SYS)
	{	if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_Sys))
		{	jsonTemp = cJSON_GetObjectItem(jsonObj, SENSTALK_JSON_STR_Sys);
			iotpsStart = 3;
		}
	}
	if(servCmd->cmdType == STK_READ_CFG)
	{	if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_Cfgs))
		{	jsonTemp = cJSON_GetObjectItem(jsonObj, SENSTALK_JSON_STR_Cfgs);
			iotpsStart = 10;
		}
	}
		
	if(jsonTemp && cJSON_IsArray(jsonTemp))
	{	arraySize = cJSON_GetArraySize(jsonTemp);
		
		for(arrayIdx=0;arrayIdx<arraySize;arrayIdx++)
		{	arrayItem = cJSON_GetArrayItem(jsonTemp, arrayIdx);
			jsonStr = cJSON_GetStringValue(arrayItem);
			for(iotpsIdx=iotpsStart;iotpsIdx<STRUCT_ARRAY_SIZE(iotpsTable);iotpsIdx++)
			{	if((strlen(jsonStr) == strlen(iotpsTable[iotpsIdx].name)) && (!strcmp(jsonStr, iotpsTable[iotpsIdx].name)))
				{	servCmd->subCmdType |= iotpsTable[iotpsIdx].subcmd;
					break;
				}
			}
		}
	}
}
//------------------------------------------------------------------------------
static int decodeCmdJson(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj)
{	int success = -1;
	int cmdTabIdx;
  char *jsonStr;
	
	if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_CMD))
	{	getSringValueByKeyFromObj(jsonObj, SENSTALK_JSON_STR_CMD, &jsonStr);
		for(cmdTabIdx=0;cmdTabIdx<STRUCT_ARRAY_SIZE(senstalkJsonCmdTable);cmdTabIdx++)
		{	if((strlen(jsonStr) == strlen(senstalkJsonCmdTable[cmdTabIdx].name))&&(!strcmp(jsonStr, senstalkJsonCmdTable[cmdTabIdx].name)))
			{	servCmd->cmdType = senstalkJsonCmdTable[cmdTabIdx].processType;
				success = 0;
				break;
			}
		}
	}
	
	return success;
}
//------------------------------------------------------------------------------
static void decodeRecJson(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj)
{	char *jsonStr;
	int iotpIdx;
	
	if(servCmd->cmdType == STK_READ_RECS)
	{	if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_IOTp))
		{	getSringValueByKeyFromObj(jsonObj, SENSTALK_JSON_STR_IOTp, &jsonStr);
			for(iotpIdx=0;iotpIdx<STRUCT_ARRAY_SIZE(iotpsTable);iotpIdx++)
			{	if((strlen(iotpsTable[iotpIdx].name) == strlen(jsonStr)) && (!strcmp(jsonStr, iotpsTable[iotpIdx].name)))
				{	servCmd->subCmdType |= iotpsTable[iotpIdx].subcmd;
					break;
				}
			}
		}
		if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_SDt))
		{	getIntValueByKeyFromObj(jsonObj, SENSTALK_JSON_STR_SDt, &iotpIdx);
			servCmd->startDt = iotpIdx;
		}
			
		if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_EDt))
		{	getIntValueByKeyFromObj(jsonObj, SENSTALK_JSON_STR_EDt, &iotpIdx);
			servCmd->endDt = iotpIdx;
		}
				
		if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_HRv))
		{	getIntValueByKeyFromObj(jsonObj, SENSTALK_JSON_STR_HRv, &iotpIdx);
			servCmd->rspType = iotpIdx;
		}
	}
}
//------------------------------------------------------------------------------
static void decodeWrDoJson(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj)
{	cJSON *vsJsonObj;
	cJSON *valJson;
	int doIdx;
	char doStr[3];
	
	if(servCmd->cmdType == STK_WRITE_DO)
	{	servCmd->subCmdType = SENSTALK_MQTT_SUB_CMD_DO;
		if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_Vs))
		{	vsJsonObj = cJSON_GetObjectItem(jsonObj, SENSTALK_JSON_STR_Vs);
			for(doIdx=0;doIdx<DO_WIRED_QUANTITY;doIdx++)
			{	memset(doStr, 0, 3);
				SENS_SPRINTF(doStr, "%d", doIdx);
				if(cJSON_HasObjectItem(vsJsonObj, doStr))
				{	valJson = cJSON_GetObjectItem(vsJsonObj, doStr);
          servCmd->cfgInfoValid[doIdx] = 1;
					servCmd->cfgInfo[doIdx] = (int)cJSON_GetNumberValue(valJson);
				}
			}
		}
	}
}
//------------------------------------------------------------------------------
static void decodeWrCfgJson(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj)
{	int wcfgIdx;
	cJSON *cfgObj;
	cJSON *itemObj;
	
	if(servCmd->cmdType == STK_WRITE_CFG)
	{	if(cJSON_HasObjectItem(jsonObj, SENSTALK_JSON_STR_Cfgs))
		{	cfgObj = cJSON_GetObjectItem(jsonObj, SENSTALK_JSON_STR_Cfgs);
			for(wcfgIdx=0;wcfgIdx<STRUCT_ARRAY_SIZE(wCfgTable);wcfgIdx++)
			{	if(cJSON_HasObjectItem(cfgObj, wCfgTable[wcfgIdx].name))
				{	itemObj = cJSON_GetObjectItem(cfgObj, wCfgTable[wcfgIdx].name);
					servCmd->subCmdType |= wCfgTable[wcfgIdx].subcmd;
					servCmd->cfgInfoValid[wCfgTable[wcfgIdx].arrayPosition] = 1;
					servCmd->cfgInfo[wCfgTable[wcfgIdx].arrayPosition] = (int)cJSON_GetNumberValue(itemObj);
				}
			}
		}
	}
}
//------------------------------------------------------------------------------
static SERVER_CMD_STRUCT* senstalkMqttDecodeJson(char *buf, unsigned int len, MQTTCtx *mqttCtx)
{	SERVER_CMD_STRUCT *servCmd;
	int idx;
	cJSON *jsonObj;
	//char *jsonStr;
	
	buf[len] = '\0';
	
  dbg_msg("receive Senstalk MQTT message(%d):"GREEN"%s\r\n"ORG_COLOR, len, buf);
  
	jsonObj = cJSON_Parse(buf);
	if(jsonObj == NULL)
		return NULL;
	
	if(getIntValueByKeyFromObj(jsonObj, SENSTALK_JSON_STR_VER, &idx))
		return NULL;
	if(idx != 3)
		return NULL;
	
	servCmd = SENS_MEM_ZALLOC(sizeof(SERVER_CMD_STRUCT));
	
#if defined MQTT_TID_IS_INTEGER
	getLongValueByKeyFromObj(jsonObj, SENSTALK_JSON_STR_Tid, &servCmd->tid);
#else
	if(!getSringValueByKeyFromObj(jsonObj, SENSTALK_JSON_STR_Tid, &jsonStr))
		servCmd->tid = atoll(jsonStr);
#endif
	
	if(decodeCmdJson(servCmd, jsonObj))
	{	SENS_MEM_FREE(servCmd);
		return NULL;
	}
	
	decodeReiosResysReCfgJson(servCmd, jsonObj);
	decodeRecJson(servCmd, jsonObj);
	decodeWrDoJson(servCmd, jsonObj);
	decodeWrCfgJson(servCmd, jsonObj);
	cJSON_Delete(jsonObj);	
	return servCmd;
}
//------------------------------------------------------------------------------
void parserSenstalkMqttProtocol(SERV_RECV_CMD_CTX *senstalkRecvCmd)
{	MQTTCtx *mqttCtx;
	
	char *topic;
	char *msg;
	SERVER_CMD_STRUCT *servCmd;
	unsigned int topicLen;
	unsigned int msgLen;
	char *subscribeTopic;
	
	topic = senstalkRecvCmd->topic;
	msg = senstalkRecvCmd->jsonMsg;
	topicLen = senstalkRecvCmd->topicLen;
	msgLen = senstalkRecvCmd->msgLen;
	mqttCtx = (MQTTCtx *)senstalkRecvCmd->cmdProtocol;
	
	
	subscribeTopic = SENS_MEM_ZALLOC(50);
	SENS_SPRINTF(subscribeTopic, "SensMini/%s/", sensSys->guid.guidString);
	if(!memcmp(topic, subscribeTopic, strlen(subscribeTopic)))	//senstalk mqtt protocol
	{	if(strstr(topic, "Req") == NULL)	//cReq include
		{	dbg_msg("%s\r\n", "topic no Req!!");
			goto End_Senstalk_Protocol;
		}
		topic[topicLen] = '\0';
		dbg_msg("receive Senstalk MQTT Topic(%d):"GREEN"%s\r\n"ORG_COLOR, topicLen, topic);
		servCmd = senstalkMqttDecodeJson(msg, msgLen, mqttCtx);
		if(servCmd)
		{	//senstalkMqttProcess(cmd);
			struct TaskQMsg senstalkMsg;
			servCmd->current_tick = GetTimeTicks();
			servCmd->cmdProtocol = (void *)mqttCtx;
			senstalkMsg.msgId = SERV_CMD_PROCESS_Q_MSG_SEND_CMD;
			senstalkMsg.ptr = (char *)servCmd;
				
			if(sendMsgWithErrHandle(SERV_CMD_PROCESS_TASK, SENS_MSG_Q_SEND(servCmdProcessQ, (uint32_t *)&senstalkMsg, 0), __func__, __LINE__))
			{	SENS_MEM_FREE(servCmd);
			}
		}
	}
	else
	{	dbg_msg("%s", "error subscribeTopic!!\r\n");
	}
End_Senstalk_Protocol:
	SENS_MEM_FREE(subscribeTopic);
}
//------------------------------------------------------------------------------
void senstalkWriteConfig(SERVER_CMD_STRUCT *senstalkCmd)
{	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	OTA_MANAGER *otaMng = (OTA_MANAGER *)&sysCtrl->otaMng;
#if defined PARAM_IS_JSON
	cJSON *objJson;
#else
	MiniFile *mIniFile;
#endif
	int idx, j;
#ifdef SUPPORT_MQTT
	char writeIni = 0;
	char response = 0;
	MQTTCtx *mqttCtx = (MQTTCtx *)senstalkCmd->cmdProtocol;

	if(otaMng->runningAutoFota || (mqttCtx->stat >= WMQ_WAIT_MSG))
#endif
	{	
#if defined PARAM_IS_JSON
		objJson = getJsonObjFromFile(JSON_PARAM_FILE);
#else
		mIniFile = mini_parse_file(INIFILE);
#endif
		for(idx = 0; idx < MAX_CFG_ITEM; idx++)
		{	if(senstalkCmd->cfgInfoValid[idx])
			{	for(j = 0; j < SIZE_OF_WR_CFG(); j++)
				{	if(wCfgTable[j].arrayPosition == idx)
						break;
				}
				if(j != SIZE_OF_WR_CFG())
				{	
#ifdef SUPPORT_MQTT
					if(wCfgTable[j].subcmd == SENSTALK_MQTT_SUB_CMD_RECIV)
					{	
#if defined PARAM_IS_JSON
						updateJsonParamArrayValue(objJson, PARAM_JSON_SYSTEM, 0, PARAM_SYS_ITEM_REC_TIME, senstalkCmd->cfgInfo[idx], NULL);
#else
						mini_file_update_value_for_key(mIniFile, SECTION, SD_REC_SEC_str, f2aBuf);
#endif
						sensSysCfg->recordSec = senstalkCmd->cfgInfo[idx];
						writeIni = 1;
						response = 1;
					}
					else if(wCfgTable[j].subcmd == SENSTALK_MQTT_SUB_CMD_SLP)
					{	
#if defined PARAM_IS_JSON
						updateJsonParamArrayValue(objJson, PARAM_JSON_SYSTEM, 0, PARAM_SYS_ITEM_PSM, senstalkCmd->cfgInfo[idx], NULL);
#else
						mini_file_update_value_for_key(mIniFile, SECTION, PIC_POWERDOWN_str, f2aBuf);
#endif
						sensSysCfg->psm = senstalkCmd->cfgInfo[idx];
						writeIni = 1;
						response = 1;
					}
					else if(wCfgTable[j].subcmd == SENSTALK_MQTT_SUB_CMD_TIME_CALIBRATION)
					{	
#if defined NUVOTON
						timestampToSetRtc(senstalkCmd->cfgInfo[idx]);
						rtctimeForSenstalkMqtt = senstalkCmd->cfgInfo[idx];
#elif defined PLATFORM_FSL
						TIME_STRUCT timeMqx;
						timeMqx.SECONDS = senstalkCmd->cfgInfo[idx];
						timeMqx.MILLISECONDS = 0;
						_time_set(&timeMqx);
						if(_rtc_sync_with_mqx(FALSE) != MQX_OK)
							dbg_msg("%s", "[WARNING] TIMER: Failed to set rtc time!\r\n");
						else
							rtctimeForSenstalkMqtt.SECONDS = timeMqx.SECONDS;
#elif defined PLATFORM_XILINX
						
						
						setTimeFromTimestamp(timeMqx.SECONDS);
						rtctimeForSenstalkMqtt.SECONDS = timeMqx.SECONDS;
#endif
						response = 1;
					}
					else
#endif
					if(wCfgTable[j].subcmd == SENSTALK_MQTT_SUB_CMD_FW_OTA)
					{	//long currentTick;
						//currentTick = GetTimeTicks(); 
#if TEST_N_REMOVE
						startTrigOta();
#endif
					}
				}
			}
		}
#ifdef SUPPORT_MQTT
		if(writeIni)
#if defined PARAM_IS_JSON
			writeJsonObjToFile(JSON_PARAM_FILE, objJson);
		cJSON_Delete(objJson);
#else
			mini_save_to_file(mIniFile, NULL, INI_BAK_FILE);
		mini_file_free(mIniFile);
#endif
		
		if(!response)
			return;
#endif
	}
#ifdef SUPPORT_MQTT
	else
	{	reQueueSenstalkCmd(TASK_PROCESS_TYPE_SENSTALK, senstalkCmd, 200, __LINE__);
	}
#endif
}
//------------------------------------------------------------------------------
void senstalkMqttRspProcess(SERVER_CMD_STRUCT *servCmd)
{	//MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	switch(servCmd->cmdType)
	{	case STK_AUTOSEND:
					autoSendToSenslink(servCmd);
					break;
		case STK_CONNECT:
					connectMsgSendToSenslink(servCmd);
					break;
		case STK_WRITE_DO:
					mqttWriteDo(servCmd);
					break;
		case STK_READ_IOs:
		case STK_READ_AI:
		case STK_READ_DI:
		case STK_READ_DO:
					readSetIosToSenslink(servCmd);
					break;
		case STK_READ_SYS:
					readSysToSenslink(servCmd);
					break;
		case STK_WRITE_CFG:
					senstalkWriteConfig(servCmd);
					break;
		case STK_READ_CFG:
					readCfgToSenslink(servCmd);
					break;
		case STK_READ_RECS:
					readRecordToSenslink(servCmd);
					break;
		case STK_REBOOT:
		//case STK_SLEEP:
		case STK_SLEEP_AUTOSEND:
					controlTypeToSenslink(servCmd);
					break;
		default:
					break;
	}
}
//------------------------------------------------------------------------------
void initialIotpsTable(void)
{	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	OTA_MANAGER *otaMng = (OTA_MANAGER *)&sysCtrl->otaMng;
	
	mqttFwVersion = FW_VERSION;
	iotpsTable[SENSTALK_MQTT_SUB_CMD_TEMP_IDX].value = (void *)&sensPq->onboardPq[ON_BOARD_PQ_TEMPERATURE];
#if defined NUVOTON
	iotpsTable[SENSTALK_MQTT_SUB_CMD_TIME_IDX].value = (void *)&rtctimeForSenstalkMqtt;
#else
	iotpsTable[SENSTALK_MQTT_SUB_CMD_TIME_IDX].value = (void *)&rtctimeForSenstalkMqtt.SECONDS;
#endif
	iotpsTable[SENSTALK_MQTT_SUB_CMD_RBTCT_IDX].value = (void *)&sensPq->onboardPq[ON_BOARD_PQ_REBOOT_TIMES];
	iotpsTable[SENSTALK_MQTT_SUB_CMD_BATV_IDX].value = (void *)&sensPq->onboardPq[ON_BOARD_PQ_BATTERY_VOLT];
	iotpsTable[SENSTALK_MQTT_SUB_CMD_EXTV_IDX].value = (void *)&sensPq->onboardPq[ON_BOARD_PQ_EXT_VOLT];
	iotpsTable[SENSTALK_MQTT_SUB_CMD_4GSI_IDX].value = (void *)&sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH];
	iotpsTable[SENSTALK_MQTT_SUB_CMD_NBSI_IDX].value = (void *)&sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH];
	
	iotpsTable[SENSTALK_MQTT_SUB_CMD_RECIV_IDX].value = (void *)&sensSysCfg->recordSec;
	iotpsTable[SENSTALK_MQTT_SUB_CMD_SLP_IDX].value = (void *)&sensSysCfg->psm;
	iotpsTable[SENSTALK_MQTT_SUB_CMD_FW_VER_IDX].value = (void *)&mqttFwVersion;
#if defined NUVOTON
	iotpsTable[SENSTALK_MQTT_SUB_CMD_TIME_CALIBRATION_IDX].value = (void *)&rtctimeForSenstalkMqtt;
#else
	iotpsTable[SENSTALK_MQTT_SUB_CMD_TIME_CALIBRATION_IDX].value = (void *)&rtctimeForSenstalkMqtt.SECONDS;
#endif
	
	iotpsTable[SENSTALK_MQTT_SUB_CMD_FW_OTA_IDX].value = (void *)&otaMng->fotaStatus;
}
//------------------------------------------------------------------------------
#endif