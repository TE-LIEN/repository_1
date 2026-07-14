#include "network/protocol/serverProtocol.h"
#include "network/netApp.h"
#include "cJSON/cJSON.h"
#include "param.h"
#include "ini2.h"
#include "sensSystem.h"
#include "sensCommon.h"
#include "physicalQuantity.h"

#if YS_MQTT_BROKER

extern int decodeCtrlsJson(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj);
//------------------------------------------------------------------------------
static MQTT_SEND_INFO *setSensCfgMqttMsg(cJSON *jsonObj, MQTTCtx *mqttCtx, int ack)
{	MQTT_SEND_INFO *mqttSendMsg;
	CLOUD_SERVER_INSTANCE *serverInst;
	int allocSize;
	
	serverInst = getServerInstFromMqtt(mqttCtx);
	
	mqttSendMsg = SENS_MEM_ZALLOC(sizeof(MQTT_SEND_INFO));
	
	if(ack)
	{	allocSize = strlen("SensCfg/") + strlen(sensSys->guid.guidString) + strlen("/ack") + 1;
		mqttSendMsg->sendTopic = SENS_MEM_ZALLOC(allocSize);
		SENS_SPRINTF(mqttSendMsg->sendTopic, "SensCfg/%s/ack", sensSys->guid.guidString);
	}
	else
	{	allocSize = strlen("SensCfg/") + strlen(sensSys->guid.guidString) + strlen("/connect") + 1;
		mqttSendMsg->sendTopic = SENS_MEM_ZALLOC(allocSize);
		SENS_SPRINTF(mqttSendMsg->sendTopic, "SensCfg/%s/connect", sensSys->guid.guidString);
	}
	
	mqttSendMsg->sendMessage = cJSON_PrintUnformatted(jsonObj);
	mqttSendMsg->sendMessageLen = strlen(mqttSendMsg->sendMessage);
	serverInst->mqttSendMsg = mqttSendMsg;
	return mqttSendMsg;
}
//------------------------------------------------------------------------------
static void sensCfgSetImProcess(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj)
{	MiniFile *mIniFile;
	int error = 1;
	mIniFile = mini_parse_file(INIFILE);
	if(mIniFile == NULL)
		goto FUNC_RET;
	
	error = 0;
	mini_file_update_value_for_key(mIniFile, SECTION, SYSTEM_INSTALL_MODE_STR, (servCmd->cmdDir)? "1":"0");
	mini_save_to_file(mIniFile, NULL, INI_BAK_FILE);
	mini_file_free(mIniFile);
FUNC_RET:
	cJSON_AddItemToObject(jsonObj, SENSCFG_ACK_STS, cJSON_CreateString(error? SENSCFG_ACK_FAIL:SENSCFG_ACK_OK));
}
//------------------------------------------------------------------------------
static void sensCfgSetCmdProcess(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj)
{	cJSON *readCfgArrays;
	MiniFile *mIniFile;
	int idx;
	int arrSize;
	int error = 1;
	cJSON *cmdCtrlObj;
	cJSON *cmdCtrlNameItem;
	cJSON *cmdCtrlValItem;
	char *nameStr;
	char *valStr;
	int iniSave = 0;
	int iniFree = 0;
	
	readCfgArrays = (cJSON *)servCmd->otherCmdInfo;
	if(!cJSON_IsArray(readCfgArrays))
		goto FUNC_RET;
		
	arrSize = cJSON_GetArraySize(readCfgArrays);
	mIniFile = mini_parse_file(INIFILE);
	if(mIniFile == NULL)
		goto FUNC_RET;

	iniSave = 1;
	iniFree = 1;
		
	for(idx=0;idx<arrSize;idx++)
	{	cmdCtrlObj = cJSON_GetArrayItem(readCfgArrays, idx);
		if(cJSON_IsObject(cmdCtrlObj))
		{	cmdCtrlNameItem = cJSON_GetObjectItem(cmdCtrlObj, SENSCFG_CTRL_CMD_CTRL_NAME_STR);
			if(cmdCtrlNameItem == NULL)
			{	iniSave = 0;
				goto FUNC_RET;
			}
			nameStr = cJSON_GetStringValue(cmdCtrlNameItem);
			cmdCtrlValItem = cJSON_GetObjectItem(cmdCtrlObj, SENSCFG_CTRL_CMD_CTRL_VAL_STR);
			if(cmdCtrlValItem == NULL)
			{	iniSave = 0;
				goto FUNC_RET;
			}
			valStr = cJSON_GetStringValue(cmdCtrlValItem);
			mini_file_update_value_for_key(mIniFile, SECTION, nameStr, valStr);
		}
		else
		{	iniSave = 0;
			goto FUNC_RET;
		}
	}
	error = 0;
FUNC_RET:		
	if(iniSave)
		mini_save_to_file(mIniFile, NULL, INI_BAK_FILE);
	if(iniFree)
		mini_file_free(mIniFile);
	cJSON_AddItemToObject(jsonObj, SENSCFG_ACK_STS, cJSON_CreateString(error? SENSCFG_ACK_FAIL:SENSCFG_ACK_OK));
}
//------------------------------------------------------------------------------
static void sensCfgGetCmdProcess(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj)
{	cJSON *writeCfgArrays;
	int arrSize;
	MiniFile *mIniFile;
	int idx;
	cJSON *rspCtrlArrays = NULL;
	cJSON *rspOneCtrlObj;
	cJSON *rspOneCtrlNameItem;
	cJSON *rspOneCtrlValItem;
	char *strVal;
	cJSON *cmdCtrlNameItem;
	char *paramValue;
	
	writeCfgArrays = (cJSON *)servCmd->otherCmdInfo;
	if(!cJSON_IsArray(writeCfgArrays))
		goto FUNC_RET;
		
	arrSize = cJSON_GetArraySize(writeCfgArrays);
	
	mIniFile = mini_parse_file(INIFILE);
	if(mIniFile == NULL)
		goto FUNC_RET;
	rspCtrlArrays = cJSON_CreateArray();
	for(idx=0;idx<arrSize;idx++)
	{	cmdCtrlNameItem = cJSON_GetArrayItem(writeCfgArrays, idx);
		strVal = cJSON_GetStringValue(cmdCtrlNameItem);
		paramValue = mini_file_get_value(mIniFile, SECTION, strVal);
		
		rspOneCtrlObj = cJSON_CreateObject();
		rspOneCtrlNameItem = cJSON_CreateString(strVal);
		cJSON_AddItemToObject(rspOneCtrlObj, SENSCFG_CTRL_CMD_CTRL_NAME_STR, rspOneCtrlNameItem);
		rspOneCtrlValItem = cJSON_CreateString(paramValue);
		cJSON_AddItemToObject(rspOneCtrlObj, SENSCFG_CTRL_CMD_CTRL_VAL_STR, rspOneCtrlValItem);
		
		cJSON_AddItemToArray(rspCtrlArrays, rspOneCtrlObj);
	}
	mini_file_free(mIniFile);
	
FUNC_RET:
	if(rspCtrlArrays)
		cJSON_AddItemToObject(jsonObj, SENSCFG_CTRL_CMD_CTRL_STR, rspCtrlArrays);
	else
		cJSON_AddItemToObject(jsonObj, SENSCFG_ACK_STS, cJSON_CreateString(SENSCFG_ACK_FAIL));
}
//------------------------------------------------------------------------------
static void sensCfgUploadProcess(SERVER_CMD_STRUCT *servCmd)
{	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	CLOUD_SERVER_INSTANCE *serverInst;
	cJSON *jsonObj=NULL;
	char verStr[9];
		
	serverInst = getServerInstFromMqtt(mqttCtx);
	jsonObj = cJSON_CreateObject();

	cJSON_AddNumberToObject(jsonObj, SENSCFG_CONNECT_CMD_RBTCNT_STR, (uint32_t)sensPq->onboardPq[ON_BOARD_PQ_REBOOT_TIMES]);
	cJSON_AddNumberToObject(jsonObj, SENSCFG_CONNECT_CMD_EXT_VOLT_STR, sensPq->onboardPq[ON_BOARD_PQ_EXT_VOLT]);
	cJSON_AddNumberToObject(jsonObj, SENSCFG_CONNECT_CMD_BAT_STR, sensPq->onboardPq[ON_BOARD_PQ_BATTERY_VOLT]);
	memset(verStr, 0, 9);
	SENS_SPRINTF(verStr, "%X", FW_VERSION);
	cJSON_AddStringToObject(jsonObj, SENSCFG_CONNECT_CMD_FW_STR, verStr);
		
	setSensCfgMqttMsg(jsonObj, mqttCtx, 0);
	if(mqttPublishJson(TASK_PROCESS_TYPE_IKW_UPLOAD, servCmd, NULL, 1, 0))
	{	networkCtx->ysServerSendConnDone = 1;
		networkCtx->ysServerOffTimer = GetTimeTicks();
	}
	cJSON_Delete(jsonObj);
	clearMqttSendMsg(serverInst);
}
//------------------------------------------------------------------------------
void sensCfgMqttCmdProcess(SERVER_CMD_STRUCT *servCmd)
{	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	CLOUD_SERVER_INSTANCE *serverInst = getServerInstFromMqtt(mqttCtx);
	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	cJSON *jsonObj;
	int isRbt = 0;
	int isFota = 0;
	int isCali = 0;
	int isSetIm = 0;
	
	if(mqttCtx->stat < WMQ_WAIT_MSG)
	{	reQueueSenstalkCmd(TASK_PROCESS_TYPE_SENSTALK, servCmd, 200, __LINE__);
		return;
	}
	
	if(servCmd->cmdType == STK_CONNECT)
	{	sensCfgUploadProcess(servCmd);
		return;
	}
	
	isRbt = servCmd->rspType;
	jsonObj = cJSON_CreateObject();
	cJSON_AddStringToObject(jsonObj, SENSCFG_CTRL_FIELD_TID_STR, servCmd->tidStr);
		
	if(servCmd->cmdType == SCFG_FOTA)
	{	cJSON_AddItemToObject(jsonObj, SENSCFG_ACK_STS, cJSON_CreateString(SENSCFG_ACK_OK));
		isRbt = 0;
		isFota = 1;
	}
	else if(servCmd->cmdType == SCFG_CALIBRATION)
	{	cJSON_AddItemToObject(jsonObj, SENSCFG_ACK_STS, cJSON_CreateString(SENSCFG_ACK_OK));
		isCali = 1;
	}
	else if(servCmd->cmdType == STK_REBOOT)
	{	cJSON_AddItemToObject(jsonObj, SENSCFG_ACK_STS, cJSON_CreateString(SENSCFG_ACK_OK));
		isRbt = 1;
	}
	else if(servCmd->cmdType == SCFG_IM)
	{	//cJSON_AddItemToObject(jsonObj, SENSCFG_ACK_STS, cJSON_CreateString(SENSCFG_ACK_OK));
		sysCtrl->sysIsInstallMode = servCmd->cmdDir;
		sensSysCfg->sysIsInstallMode = servCmd->cmdDir;
		sensCfgSetImProcess(servCmd, jsonObj);
		isSetIm = 1;
	}
	else
	{	if(servCmd->cmdDir == SENSCFG_CMD_TYPE_GET)
			sensCfgGetCmdProcess(servCmd, jsonObj);
		else
			sensCfgSetCmdProcess(servCmd, jsonObj);
	}	
	setSensCfgMqttMsg(jsonObj, mqttCtx, 1);
	mqttPublishJson(TASK_PROCESS_TYPE_SENSTALK, servCmd, NULL, 1, 0);
	cJSON_Delete(jsonObj);
	clearMqttSendMsg(serverInst);
	cJSON_Delete((cJSON *)servCmd->otherCmdInfo);
	
	if(isRbt)
		sysCtrl->isReadyToReboot = 1;
	if(isFota)
		startTrigOta();
#if TEST_N_REMOVE
	if(isCali)
		avdsCalibration();
#endif
	if(isSetIm)
	{	if(sysCtrl->sysIsInstallMode)
			setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_INSTALL_MODE);
		else
			setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_INSTALL_MODE);
	}
}
//------------------------------------------------------------------------------
int decodeCtrlsJson(SERVER_CMD_STRUCT *servCmd, cJSON *jsonObj)
{	int success = -1;
  cJSON *ctrlsItem;
	
	if(cJSON_HasObjectItem(jsonObj, SENSCFG_CTRL_CMD_CTRL_STR))
	{	ctrlsItem = cJSON_GetObjectItem(jsonObj, SENSCFG_CTRL_CMD_CTRL_STR);
		servCmd->otherCmdInfo = cJSON_Duplicate(ctrlsItem, 1);
		success = 0;
	}
	
	return success;
}
//------------------------------------------------------------------------------
static SERVER_CMD_STRUCT* sensCfgMqttDecodeJson(char *buf, unsigned int len, MQTTCtx *mqttCtx)
{	SERVER_CMD_STRUCT *servCmd;
	//int idx;
	cJSON *jsonObj;
	cJSON *tidObj;
	cJSON *rbtObj;
	cJSON *cmdTypeObj;
	cJSON *cmdCodeObj;
	cJSON *imObj;
	char *tidStr;
	char *cmdStr;
	
	buf[len] = '\0';
	
  dbg_msg("receive SensCfg MQTT message(%d):"GREEN"%s\r\n"ORG_COLOR, len, buf);
  
	jsonObj = cJSON_Parse(buf);
	if(jsonObj == NULL)
		return NULL;
	
	
	
	tidObj = cJSON_GetObjectItem(jsonObj, SENSCFG_CTRL_FIELD_TID_STR);
	if(!cJSON_IsString(tidObj))
		return NULL;
	
	tidStr = cJSON_GetStringValue(tidObj);
	servCmd = SENS_MEM_ZALLOC(sizeof(SERVER_CMD_STRUCT));
	memset(servCmd->tidStr, 0, 11);
	memcpy(servCmd->tidStr, tidStr, strlen(tidStr));
	
	servCmd->cmdType = MQTT_CMD_TYPE_MAX;
	
	cmdCodeObj = cJSON_GetObjectItem(jsonObj, SENSCFG_CTRL_CMD_STR);
	cmdStr = cJSON_GetStringValue(cmdCodeObj);
	if(!strcmp(cmdStr, "fota"))
	{	servCmd->cmdType = SCFG_FOTA;
		cJSON_Delete(jsonObj);	
		return servCmd;
	}
	else if(!strcmp(cmdStr, "cali"))
	{	servCmd->cmdType = SCFG_CALIBRATION;
		cJSON_Delete(jsonObj);	
		return servCmd;
	}
	else if(!strcmp(cmdStr, "rbt"))
	{	servCmd->cmdType = STK_REBOOT;
		cJSON_Delete(jsonObj);	
		return servCmd;
	}
	else if(!strcmp(cmdStr, "im"))
	{	servCmd->cmdType = SCFG_IM;
		imObj = cJSON_GetObjectItem(jsonObj, SENSCFG_CTRL_CMD_IM_STR);
		if(imObj && cJSON_IsTrue(imObj))
			servCmd->cmdDir = SENSCFG_CMD_TYPE_GET;
		else if(imObj)
			servCmd->cmdDir = SENSCFG_CMD_TYPE_SET;
		else
		{	SENS_MEM_FREE(servCmd);
			cJSON_Delete(jsonObj);
			return NULL;
		}
		cJSON_Delete(jsonObj);
		return servCmd;
	}
	else
	{	cmdTypeObj = cJSON_GetObjectItem(jsonObj, SENSCFG_CTRL_CMD_TYPE_STR);
		if(cmdTypeObj && cJSON_IsTrue(cmdTypeObj))
			servCmd->cmdDir = SENSCFG_CMD_TYPE_GET;
		else if(cmdTypeObj)
			servCmd->cmdDir = SENSCFG_CMD_TYPE_SET;
		else
		{	SENS_MEM_FREE(servCmd);
			return NULL;
		}
	}
	if(cJSON_HasObjectItem(jsonObj, SENSCFG_CTRL_CMD_RBT_STR))
	{	rbtObj = cJSON_GetObjectItem(jsonObj, SENSCFG_CTRL_CMD_RBT_STR);
		if(cJSON_IsTrue(rbtObj))
		{	servCmd->rspType = 1;
		}
	}
	
	if(decodeCtrlsJson(servCmd, jsonObj))
	{	SENS_MEM_FREE(servCmd);
		return NULL;
	}
	cJSON_Delete(jsonObj);	
	return servCmd;
}
//------------------------------------------------------------------------------
void parserSensCfgMqttProtocol(SERV_RECV_CMD_CTX *servRecvCmdCtx)
{	MQTTCtx *mqttCtx;
	char *topic;
	char *msg;
	SERVER_CMD_STRUCT *servCmd;
	unsigned int topicLen;
	unsigned int msgLen;
	char *subscribeTopic;
	
	topic = servRecvCmdCtx->topic;
	msg = servRecvCmdCtx->jsonMsg;
	topicLen = servRecvCmdCtx->topicLen;
	msgLen = servRecvCmdCtx->msgLen;
	mqttCtx = (MQTTCtx *)servRecvCmdCtx->cmdProtocol;
	
	
	subscribeTopic = SENS_MEM_ZALLOC(50);
	SENS_SPRINTF(subscribeTopic, "SensCfg/%s/Ctrl", sensSys->guid.guidString);
	if(!memcmp(topic, subscribeTopic, strlen(subscribeTopic)))	//senstalk mqtt protocol
	{	topic[topicLen] = '\0';
		dbg_msg("receive Senstalk MQTT Topic(%d):"GREEN"%s\r\n"ORG_COLOR, topicLen, topic);
		servCmd = sensCfgMqttDecodeJson(msg, msgLen, mqttCtx);
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
//End_Senstalk_Protocol:
	SENS_MEM_FREE(subscribeTopic);
}
#endif