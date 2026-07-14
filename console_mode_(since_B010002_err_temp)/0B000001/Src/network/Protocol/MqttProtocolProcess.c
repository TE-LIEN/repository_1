#include "network/protocol/serverProtocol.h"
#include "network/netApp.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "rtcDateTime.h"

#if 0
#include "general.h"
#include "main_task.h"
#endif
#include "param.h"

#ifndef PARAM_IS_JSON
#include "ini2.h"
#endif

extern char f2aBuf[32];
#ifndef SUPPORT_MQTT
#define STRUCT_ARRAY_SIZE(x)		sizeof(x) / sizeof(x[0])

struct SENSTALK_JSON_WRITE_SUB_CMD_STR wCfgTable[] =
{	{"FOTATri",	SENSTALK_MQTT_SUB_CMD_FW_OTA, 					WR_CFG_POS_AUTO_OTA,	0, UNIT_BOOL},
};

#define SIZE_OF_WR_CFG()	STRUCT_ARRAY_SIZE(wCfgTable)

#endif

#ifdef SUPPORT_MQTT

extern int SIZE_OF_WR_CFG(void);

#if defined NUVOTON
extern uint64_t rtctimeForSenstalkMqtt;
#else
extern TIME_STRUCT rtctimeForSenstalkMqtt;
#endif

//------------------------------------------------------------------------------
void clearMqttSendMsg(CLOUD_SERVER_INSTANCE *serverInst)
{	MQTT_SEND_INFO *mqttSendMsg;
	
	mqttSendMsg = (MQTT_SEND_INFO *)serverInst->mqttSendMsg;
	if(mqttSendMsg)
	{	if(mqttSendMsg->sendTopic)
			SENS_MEM_FREE(mqttSendMsg->sendTopic);
		if(mqttSendMsg->sendMessage)
			SENS_MEM_FREE(mqttSendMsg->sendMessage);
		SENS_MEM_FREE(mqttSendMsg);
		serverInst->mqttSendMsg = NULL;
	}
}
//------------------------------------------------------------------------------
void displayMqttMessage(char *msg)
{	char msgTemp[257];
	int loopSize;
	int remainSz;
	int dbgLength;
	int x;
	int msgLen = strlen(msg);
	
	debugLock();
	dbg_msg1("send Message(%d):", msgLen);
	dbgLength = (msgLen > 1024)? 1024:msgLen;
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
void reQueueSenstalkCmd(int type, void *oldServCmd, int waitTime, int callFromLine)
{	struct TaskQMsg senstalkMsg;
	SERVER_CMD_STRUCT *servCmd;

	(void)type;
	//if(type == TASK_PROCESS_TYPE_SENSTALK)
	{	servCmd = SENS_MEM_ZALLOC(sizeof(SERVER_CMD_STRUCT));
		memcpy(servCmd, oldServCmd, sizeof(SERVER_CMD_STRUCT));
		senstalkMsg.msgId = SERV_CMD_PROCESS_Q_MSG_SEND_CMD;
		senstalkMsg.ptr = (char *)servCmd;
	}

	if(waitTime)
		SENS_TIME_DELAY(waitTime);

	if(sendMsgWithErrHandle(SERV_CMD_PROCESS_TASK, SENS_MSG_Q_SEND(servCmdProcessQ, (uint32_t *)&senstalkMsg, 0), __func__, __LINE__))
	{	//if(type == TASK_PROCESS_TYPE_SENSTALK)
		SENS_MEM_FREE(servCmd);
	}
}
//------------------------------------------------------------------------------
char mqttPublishJson(int type, void *cmd, char *sendMsg, char waitPublishFinish, char sendAndWriteTagFlag)
{
#if defined OS_MQX
	_mqx_uint mask;
#endif
	uint32_t result;
	SERVER_CMD_STRUCT *servCmdStruct;
	MQTTCtx *mqttCtx;
	int waitTimeout;
	char sendSuccess = 0;
	
	CLOUD_SERVER_INSTANCE *serverInst;
	MQTT_SEND_INFO *mqttSendMsg;
	
	servCmdStruct = (SERVER_CMD_STRUCT *)cmd;
	mqttCtx = (MQTTCtx *)servCmdStruct->cmdProtocol;
	serverInst = getServerInstFromMqtt(mqttCtx);
	mqttSendMsg = (MQTT_SEND_INFO *)serverInst->mqttSendMsg;
		
	/*else
	{	mqttSendMsg.sendTopic = SENS_MEM_ZALLOC(strlen(mqttCtx->topic_name)+1);
		memcpy(mqttSendMsg.sendTopic, mqttCtx->topic_name, strlen(mqttCtx->topic_name));
	}*/
	dbg_msg("send topic:"YELLOW"%s\r\n"ORG_COLOR, mqttSendMsg->sendTopic);
	displayMqttMessage(mqttSendMsg->sendMessage);

	if(waitPublishFinish)
	{	if(mqttSendMsg->sendMessageLen > 4096)
			waitTimeout = 20;
		else if(mqttSendMsg->sendMessageLen > 2048)
			waitTimeout = 14;
		else if(mqttSendMsg->sendMessageLen > 1024)
			waitTimeout = 10;
		else 
			waitTimeout = 10;
#if defined OS_MQX
		_lwevent_clear(&mqttCtx->publishDoneEvent, 1);
#elif defined OS_FREERTOS
		xEventGroupClearBits(mqttCtx->publishDoneEvent, 1);
#endif
		mqttSendMsg->cmdTimeout = ((waitTimeout/2)-1)*1000;
	}
	
	sendMqttQ(mqttCtx, MQTT_Q_MSG_PUBLISH);
	if(!waitPublishFinish)
		return 1;
	dbg_msg("%s", "start wait publish\r\n");
	result = SENS_EVENT_WAIT(mqttCtx->publishDoneEvent, 1, FALSE, 200*waitTimeout);
	
#if defined OS_MQX
	switch(result)
  {	case MQX_OK:
      		mask = _lwevent_get_signalled();
      		if(mask & 0x01)
      		{	sendSuccess = 1;
      			break;
      		}
      		else
      		{	break;
      		}
    case LWEVENT_WAIT_TIMEOUT:
      		break;
    default:
      		break;
  }
#elif defined OS_FREERTOS
	if(result & 0x01)
	{	sendSuccess = 1;
	}
#endif
  dbg_msg("wait MQTT publish done, status:%d\r\n", sendSuccess);
	return sendSuccess;
}
//------------------------------------------------------------------------------
void parserMqttProtocol(SERV_RECV_CMD_CTX *servRecvCmd)
{	char *topic;
	char *msg;
	
	topic = servRecvCmd->topic;
	msg = servRecvCmd->jsonMsg;
	
	if(strstr(topic, "/messages/devicebound"))	//AZURE or AVNET
	{
	}
#if defined SUPPORT_MQTT_IKW || defined SUPPORT_MQTTS_IKW
	else if(strstr(topic, IKW_REQ_TOPIC))	//IKW
	{	parserIkwMqttProtocol(servRecvCmd);
	}
#endif
#if defined SUPPORT_MQTT_SENSSEWER || defined SUPPORT_MQTTS_SENSSEWER
	else if(strstr(topic, "SensSewer"))	//IKW
	{	parserSensSewerMqttProtocol(servRecvCmd);
	}
#endif
#if YS_MQTT_BROKER
	else if(strstr(topic, "SensCfg"))
	{	parserSensCfgMqttProtocol(servRecvCmd);
	}
#endif
	else
	{	parserSenstalkMqttProtocol(servRecvCmd);
	}
	SENS_MEM_FREE(msg);
	SENS_MEM_FREE(topic);
}
#endif





