#include "network/protocol/serverProtocol.h"
#include "network/netApp.h"
#include "sensCommon.h"

//------------------------------------------------------------------------------
void prepareProtocolPayloadToServer(SERVER_CMD_STRUCT *servCmd)	//prepare payload to send
{	//senstalkMqttEncoder(senstalkCmd);
	MQTTCtx *mqttCtx = (MQTTCtx *)servCmd->cmdProtocol;
	CLOUD_SERVER_INSTANCE *serverInst;
	
	if(mqttCtx != NULL)
	{	serverInst = (CLOUD_SERVER_INSTANCE *)mqttCtx->app_ctx;
		if((serverInst->serverType == MQTT_SENSTALK_BROKER) || (serverInst->serverType == MQTTS_SENSTALK_BROKER) || (serverInst->serverType == MQTT_THINGSBOARD_BROKER))
		{	/*if(servCmd->cmdType == STK_WRITE_CFG)
			{	senstalkWriteConfig(servCmd);
			}
			else*/
				senstalkMqttRspProcess(servCmd);
		}
#ifdef SUPPORT_MQTT_IKW
		else if((serverInst->serverType == MQTT_I_KNOW_WATER_BROKER) || (serverInst->serverType == MQTTS_I_KNOW_WATER_BROKER))
		{	ikwMqttCmdProcess(servCmd);
		}
#endif
#if defined SUPPORT_MQTT_SENSSEWER
		else if((serverInst->serverType == MQTT_SENSSEWER_BROKER) || (serverInst->serverType == MQTTS_SENSSEWER_BROKER))
		{	dbgMsg("%s", "process senssewer cmd start!!\r\n");
			sensSewerMqttCmdProcess(servCmd);
			dbgMsg("%s", "process senssewer cmd End!!\r\n");
		}
#endif
#if YS_MQTT_BROKER
		else if((serverInst->serverType == MQTTS_YS_BROKER) || (serverInst->serverType == MQTT_YS_BROKER))
		{	sensCfgMqttCmdProcess(servCmd);
		}
#endif
	}
}
//------------------------------------------------------------------------------
void parserServerPayload(SERV_RECV_CMD_CTX *servRecvCmd)	//parser payload
{	CLOUD_SERVER_INSTANCE *serverInst = servRecvCmd->serverInst;
	enum XMIT_PROTOCOL_TYPE protocolType = serverInst->xmitProtocolType;
	
	SENS_SEM_LOCK(serverInst->lock);
	
	if(protocolType == PROTOCOL_MQTT)
	{	parserMqttProtocol(servRecvCmd);
	}
	else if(protocolType == PROTOCOL_SENSTALK_V2)
	{	dbg_msg(YELLOW"is Senstalk V2, from internet:%d\r\n"ORG_COLOR, servRecvCmd->generateFromInternet);
		parserSenstalkProtocol(servRecvCmd);
	}
	else if(protocolType == PROTOCOL_HTTP || (protocolType == PROTOCOL_IOA_WEB_API))
	{	parserHttpProtocol(servRecvCmd);
	}
	SENS_MEM_FREE(servRecvCmd);
	SENS_SEM_UNLOCK(serverInst->lock);
}
//------------------------------------------------------------------------------