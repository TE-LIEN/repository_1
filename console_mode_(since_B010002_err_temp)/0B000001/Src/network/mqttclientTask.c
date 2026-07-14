/* mqttclient.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfMQTT.
 *
 * wolfMQTT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfMQTT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* Include the autoconf generated config.h */
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

//#include "../NET_APP/mqttclientTask.h"
#include "network/mqttClientTask.h"
#include "network/netApp.h"
#include "network/protocol/serverProtocol.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "physicalQuantity.h"

#ifdef SUPPORT_MQTT
//#include "../NET_APP/mqtt/mqtt_client.h"

#include "cJSON/cJSON.h"

//#define RED			"\033[31m"
//#define GREEN		"\033[32m"
//#define YELLOW	"\033[33m"
//#define BLUE		"\033[34m"
//#define ORG_COLOR	"\033[0m"

#if defined OS_MQX
	_mqx_max_type  mqttQ[mqttQSize];
#elif defined OS_FREERTOS
	QueueHandle_t  mqttQ;
#endif

static void mqttClientDeinit(CLOUD_SERVER_INSTANCE *serverInst);

char subscribeTopic[256];
/* Locals */

/* Configuration */

/* Maximum size for network read/write callbacks. There is also a v5 define that
   describes the max MQTT control packet size, DEFAULT_MAX_PKT_SZ. */

//extern void clearReceiveQueue(int type, int sockNum);

void mqttClientSubscribe(MQTTCtx *mqttCtx, char *topic, MqttQoS qos, char num);
int mqttClientUnsubscribe(MQTTCtx *mqttCtx, char *topicStr);
void sendCloseSocket(MQTTCtx *mqttCtx);

#ifdef WOLFMQTT_PROPERTY_CB
#define MAX_CLIENT_ID_LEN 64
char gClientId[MAX_CLIENT_ID_LEN] = {0};
#endif

#ifdef WOLFMQTT_DISCONNECT_CB
/* callback indicates a network error occurred */
static int mqtt_disconnect_cb(MqttClient* client, int error_code, void* ctx)
{	(void)client;
	(void)ctx;
	//PRINTF("Network Error Callback: %s (error %d)", MqttClient_ReturnCodeToString(error_code), error_code);
	return 0;
}
#endif

/*int mqttPublishCb(MqttPublish* publish)
{	gprs.nbPublishDone = 1;
}*/
/*
int publishLock(void)
{	return SENS_SEM_LOCK(publishSem);
}

void publishUnlock(void)
{	SENS_SEM_UNLOCK(publishSem);
}

int mqttLock(MQTTCtx *mqttCtx)
{	return SENS_SEM_LOCK(mqttCtx->mqttSem);
}

void mqttUnlock(MQTTCtx *mqttCtx)
{	SENS_SEM_UNLOCK(mqttCtx->mqttSem);
}
*/
//------------------------------------------------------------------------------
static int mqtt_message_len_error_cb(MqttClient *client)
{	//MQTTCtx* mqttCtx = (MQTTCtx*)client->ctx;
	//printf("%s\n", __func__);
	//setMqttMessageReceiveTimeout(mqttCtx);
	dbg_msg("%s enter \r\n", __func__);
	return MQTT_CODE_SUCCESS;
}
//------------------------------------------------------------------------------
static int mqtt_message_cb(MqttClient *client, MqttMessage *msg, byte msg_new, byte msg_done)
{	MQTTCtx *mqttCtx = (MQTTCtx*)client->ctx;
  MqttNet *mqttNet = &mqttCtx->net;
  CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)mqttNet->context;

	SERV_RECV_CMD_CTX *cmdMsg;
	struct TaskQMsg senstalkMsg;

	(void)mqttCtx;

	if(msg_new)
	{	/* Determine min size to dump */
		cmdMsg = SENS_MEM_ZALLOC(sizeof(SERV_RECV_CMD_CTX));
		cmdMsg->cmdProtocol = (void *)mqttCtx;
		cmdMsg->topic = SENS_MEM_ZALLOC(msg->topic_name_len+1);
		memcpy(cmdMsg->topic, msg->topic_name, msg->topic_name_len);
		cmdMsg->topicLen = msg->topic_name_len;
		cmdMsg->jsonMsg = SENS_MEM_ZALLOC(msg->buffer_len+1);
		memcpy(cmdMsg->jsonMsg, msg->buffer, msg->buffer_len);
		cmdMsg->msgLen = msg->buffer_len;
		cmdMsg->serverInst = serverInst;
		//cmdMsg->protocolType = PROTOCOL_MQTT;
		senstalkMsg.msgId = SERV_CMD_PROCESS_Q_MSG_RECV_CMD;
		senstalkMsg.ptr = (char *)cmdMsg;

		if(sendMsgWithErrHandle(SERV_CMD_PROCESS_TASK, SENS_MSG_Q_SEND(servCmdProcessQ, (uint32_t *)&senstalkMsg, 0), __func__, __LINE__))
		{	SENS_MEM_FREE(cmdMsg);
		}
	}
	return MQTT_CODE_SUCCESS;
}
//------------------------------------------------------------------------------
#ifdef WOLFMQTT_PROPERTY_CB
/* The property callback is called after decoding a packet that contains at
   least one property. The property list is deallocated after returning from
   the callback. */
static int mqtt_property_cb(MqttClient *client, MqttProp *head, void *ctx)
{	MqttProp *prop = head;
	int rc = 0;
	MQTTCtx* mqttCtx;

	if((client == NULL) || (client->ctx == NULL))
	{	return MQTT_CODE_ERROR_BAD_ARG;
	}
	mqttCtx = (MQTTCtx*)client->ctx;

	while (prop != NULL)
	{	switch(prop->type)
		{	case MQTT_PROP_ASSIGNED_CLIENT_ID:
						/* Store client ID in global */
						mqttCtx->client_id = &gClientId[0];
						/* Store assigned client ID from CONNACK*/
						XSTRNCPY((char*)mqttCtx->client_id, prop->data_str.str, MAX_CLIENT_ID_LEN);
						break;
			case MQTT_PROP_SUBSCRIPTION_ID_AVAIL:
						mqttCtx->subId_not_avail = prop->data_byte == 0;
						break;
			case MQTT_PROP_TOPIC_ALIAS_MAX:
						mqttCtx->topic_alias_max = (mqttCtx->topic_alias_max < prop->data_short) ? mqttCtx->topic_alias_max : prop->data_short;
						break;
			case MQTT_PROP_MAX_PACKET_SZ:
						if((prop->data_int > 0) && (prop->data_int <= MQTT_PACKET_SZ_MAX))
						{	client->packet_sz_max = (client->packet_sz_max < prop->data_int) ? client->packet_sz_max : prop->data_int;
						}
						else
						{	/* Protocol error */
							rc = MQTT_CODE_ERROR_PROPERTY;
						}
						break;
			case MQTT_PROP_SERVER_KEEP_ALIVE:
						mqttCtx->keep_alive_sec = prop->data_short;
						break;
			case MQTT_PROP_MAX_QOS:
						client->max_qos = prop->data_byte;
						break;
			case MQTT_PROP_RETAIN_AVAIL:
						client->retain_avail = prop->data_byte;
						break;
			case MQTT_PROP_REASON_STR:
						dbg_msg("Reason String: %s", prop->data_str.str);
						break;
			case MQTT_PROP_PLAYLOAD_FORMAT_IND:
			case MQTT_PROP_MSG_EXPIRY_INTERVAL:
			case MQTT_PROP_CONTENT_TYPE:
			case MQTT_PROP_RESP_TOPIC:
			case MQTT_PROP_CORRELATION_DATA:
			case MQTT_PROP_SUBSCRIPTION_ID:
			case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
			case MQTT_PROP_TOPIC_ALIAS:
			case MQTT_PROP_TYPE_MAX:
			case MQTT_PROP_RECEIVE_MAX:
			case MQTT_PROP_USER_PROP:
			case MQTT_PROP_WILDCARD_SUB_AVAIL:
			case MQTT_PROP_SHARED_SUBSCRIPTION_AVAIL:
			case MQTT_PROP_RESP_INFO:
			case MQTT_PROP_SERVER_REF:
			case MQTT_PROP_AUTH_METHOD:
			case MQTT_PROP_AUTH_DATA:
						break;
			case MQTT_PROP_REQ_PROB_INFO:
			case MQTT_PROP_WILL_DELAY_INTERVAL:
			case MQTT_PROP_REQ_RESP_INFO:
			case MQTT_PROP_NONE:
			default:
						/* Invalid */
						rc = MQTT_CODE_ERROR_PROPERTY;
						break;
		}
		prop = prop->next;
	}

	(void)ctx;

	return rc;
}
#endif
//------------------------------------------------------------------------------
static void mqttTaskInit(void)
{	/*if(SENS_EVENT_CREATE(writeDoneEvent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{	//SYS_Reset();
	}*/
	
	/*if(SENS_EVENT_CREATE(nbWriteDoneEvent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{	//SYS_Reset();
	}*/
	/*if(SENS_EVENT_CREATE(mqttPublishDoneEvent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{	//SYS_Reset();
	}*/
	/*if(SENS_SEM_INIT(nbReadSem, 1) != MQX_OK)
	{
	}
	if(SENS_SEM_INIT(nbWriteSem, 1) != MQX_OK)
	{
	}
	if(SENS_SEM_INIT(gprs.nbMqttCtx.mqttSem, 1) != MQX_OK)
	{
	}
	if(SENS_SEM_INIT(publishSem, 1) != MQX_OK)
	{
	}*/
	if(MQX_OK != SENS_MSG_Q_INIT(mqttQ, MQTT_NUM_MESSAGES, SENS_TASKQ_GRANM))
	{	return;
	}
}
//------------------------------------------------------------------------------
static int mqttClientConnectPrepare(MQTTCtx *mqttCtx)
{	MqttNet *net = &mqttCtx->net;
	char *lwtMsg;
	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)net->context;
  
	XMEMSET(&mqttCtx->connect, 0, sizeof(MqttConnect));
	mqttCtx->connect.keep_alive_sec = mqttCtx->keep_alive_sec;
	mqttCtx->connect.clean_session = mqttCtx->clean_session;
	mqttCtx->connect.client_id = mqttCtx->client_id;

	/* Last will and testament sent by broker to subscribers of topic when broker connection is lost */
	XMEMSET(&mqttCtx->lwt_msg, 0, sizeof(mqttCtx->lwt_msg));
	mqttCtx->connect.lwt_msg = &mqttCtx->lwt_msg;
	mqttCtx->connect.enable_lwt = mqttCtx->enable_lwt;
	if(mqttCtx->enable_lwt)
	{	/* Send client id in LWT payload */
		mqttCtx->lwt_msg.qos = mqttCtx->qos;
		mqttCtx->lwt_msg.retain = 0;
		//mqttCtx->lwt_msg.topic_name = WOLFMQTT_TOPIC_NAME"lwttopic";
		if(serverInst->serverType == MQTTS_ANIOT_BROKER)
		{	int allocSz;
			allocSz = strlen(sensSys->guid.guidString) + 5;
							
			mqttCtx->lwt_msg.topic_name = ANIOT_LWT_TOPIC;
			//mqttCtx->lwt_msg.buffer = ;
			mqttCtx->lwt_msg.buffer = SENS_MEM_ZALLOC(allocSz);
			SENS_SPRINTF((char *)mqttCtx->lwt_msg.buffer, "%s DOWN", sensSys->guid.guidString);
			mqttCtx->lwt_msg.total_len = strlen((char const *)mqttCtx->lwt_msg.buffer);
		}
		else
		{	mqttCtx->lwt_msg.topic_name = mqttCtx->topic_name;
			//mqttCtx->lwt_msg.buffer = (byte*)mqttCtx->client_id;
#if YS_MQTT_BROKER
			if((serverInst->serverType == MQTTS_YS_BROKER) || (serverInst->serverType == MQTT_YS_BROKER))
			{	cJSON *jObj;
				cJSON *RbtCntItem;
				jObj = cJSON_CreateObject();
				cJSON_AddItemToObject(jObj, "RbtCnt", cJSON_CreateNumber(sensPq->onboardPq[ON_BOARD_PQ_REBOOT_TIMES]));
				lwtMsg = cJSON_PrintUnformatted(jObj);
				mqttCtx->lwt_msg.buffer = (byte *)lwtMsg;
				mqttCtx->lwt_msg.total_len = (word16)XSTRLEN(lwtMsg);
				cJSON_Delete(jObj);
			}
			else
#endif
			{	mqttCtx->lwt_msg.buffer = (byte *)SENSTALK_LWT_MSG;
				mqttCtx->lwt_msg.total_len = (word16)XSTRLEN(SENSTALK_LWT_MSG);
			}
		}
	}
	/* Optional authentication */
	dbg_msg("MQTT username:%s\r\n", mqttCtx->username);
	dbg_msg("MQTT password:%s\r\n", mqttCtx->password);
	mqttCtx->connect.username = mqttCtx->username;
	mqttCtx->connect.password = mqttCtx->password;
#ifdef WOLFMQTT_V5
	mqttCtx->client.packet_sz_max = mqttCtx->max_packet_size;
	mqttCtx->client.enable_eauth = mqttCtx->enable_eauth;

	if(mqttCtx->client.enable_eauth == 1)
	{	/* Enhanced authentication */
		/* Add property: Authentication Method */
		MqttProp* prop = MqttClient_PropsAdd(&mqttCtx->connect.props);
		prop->type = MQTT_PROP_AUTH_METHOD;
		prop->data_str.str = (char*)DEFAULT_AUTH_METHOD;
		prop->data_str.len = (word16)XSTRLEN(prop->data_str.str);
	}
	{	/* Request Response Information */
		MqttProp* prop = MqttClient_PropsAdd(&mqttCtx->connect.props);
		prop->type = MQTT_PROP_REQ_RESP_INFO;
		prop->data_byte = 1;
	}
	{	/* Request Problem Information */
		MqttProp* prop = MqttClient_PropsAdd(&mqttCtx->connect.props);
		prop->type = MQTT_PROP_REQ_PROB_INFO;
		prop->data_byte = 1;
	}
	{	/* Maximum Packet Size */
		MqttProp* prop = MqttClient_PropsAdd(&mqttCtx->connect.props);
		prop->type = MQTT_PROP_MAX_PACKET_SZ;
		prop->data_int = (word32)mqttCtx->max_packet_size;
	}
	{	/* Topic Alias Maximum */
		MqttProp* prop = MqttClient_PropsAdd(&mqttCtx->connect.props);
		prop->type = MQTT_PROP_TOPIC_ALIAS_MAX;
		prop->data_short = mqttCtx->topic_alias_max;
	}
#endif	/*WOLFMQTT_V5*/
  
  return 0;
}
//------------------------------------------------------------------------------
static void setMqttSubscribeTopic(CLOUD_SERVER_INSTANCE *serverInst)
{	MQTTCtx *mqttCtx = (MQTTCtx *)serverInst->protCtx;
	MQTT_CONNECT_INFO *mqttConnectInfo = serverInst->mqttConnectInfo;
	
	memset(subscribeTopic, 0, 256);
	if(serverInst->serverType == MQTTS_AZURE_BROKER)
	{	SENS_SPRINTF(subscribeTopic, AZURE_SUB_TOPIC, mqttConnectInfo->clientId);
	}
	else if(serverInst->serverType == MQTTS_AVNET_BROKER)
	{	SENS_SPRINTF(subscribeTopic, AVNET_SUB_TOPIC, mqttConnectInfo->clientId);
	}
	else  if(serverInst->serverType  == MQTTS_ANIOT_BROKER)
	{	//SENS_SPRINTF(subscribeTopic, "%s", sensSys->guidString);
		SENS_SPRINTF(subscribeTopic, "%s/Req", sensSys->guid.guidString);
	}
	else if((serverInst->serverType == MQTT_I_KNOW_WATER_BROKER) || (serverInst->serverType == MQTTS_I_KNOW_WATER_BROKER))
	{	SENS_SPRINTF(subscribeTopic, "%s%s", IKW_REQ_TOPIC, sensSys->guid.ikwGuidString);
	}
	else if((serverInst->serverType == MQTT_SENSSEWER_BROKER) || (serverInst->serverType == MQTTS_SENSSEWER_BROKER))
	{	SENS_SPRINTF(subscribeTopic, "SensSewer/%s/ctrl", sensSys->guid.ikwGuidString);
	}
#if YS_MQTT_BROKER
	else if((serverInst->serverType == MQTTS_YS_BROKER) || (serverInst->serverType == MQTT_YS_BROKER))
	{	SENS_SPRINTF(subscribeTopic, "SensCfg/%s/Ctrl", sensSys->guid.guidString);
	}
#endif
	else
	{
#if SUPPORT_SENSTALK_MQTT
		//SENS_SPRINTF(subscribeTopic, "SensMini/%s/Rx", sensSys->guidString);
		SENS_SPRINTF(subscribeTopic, "SensMini/%s/Req", sensSys->guidString);
#else
		SENS_SPRINTF(subscribeTopic, "SensMini/Send");
#endif
	}
	
	mqttClientSubscribe(mqttCtx, subscribeTopic, MQTT_QOS_1, 0);
}
//------------------------------------------------------------------------------
#if defined OS_MQX
void mqtt_task(uint_32 temp)
#elif defined OS_FREERTOS
void mqtt_task(void *param)
#endif
{	long currTick;
	struct TaskQMsg msg;
	MQTTCtx *mqttCtx=NULL;
	CLOUD_SERVER_INSTANCE *serverInst;
	int rc;
	
	
	SENS_SEM_LOCK(taskAct.mqttTaskAct.lock);
	
	mqttTaskInit();

	while(1)
	{	taskAct.mqttTaskAct.sts = TASK_STS_BLOCKED;
		SENS_MSG_Q_RECV(mqttQ, (_mqx_max_type *)&msg, LWMSGQ_RECEIVE_BLOCK_ON_EMPTY, 0, 0);
		taskAct.mqttTaskAct.sts = TASK_STS_RUNNING;
		currTick = GetTimeTicks();
		taskAct.mqttTaskAct.runningStartTime = currTick;
		
		serverInst = (CLOUD_SERVER_INSTANCE *)msg.ptr;
		mqttCtx = (MQTTCtx *)serverInst->protCtx;
		switch(msg.msgId)
		{	case MQTT_Q_MSG_CREATE_SOCK_CONNECT:
						{	rc = MqttClient_NetConnect(&mqttCtx->client, mqttCtx->host, mqttCtx->port, DEFAULT_CON_TIMEOUT_MS, mqttCtx->use_tls, mqtt_tls_cb);
							if(rc == MQTT_CODE_WAIT)
							{	SENS_TIME_DELAY(100);
								if(sendMsgWithErrHandle(MQTT_TASK, SENS_MSG_Q_SEND(mqttQ, (uint32_t *)&msg, 0), __func__, __LINE__))
								{
								}
							}
              else if(rc < 0)
							{	dbg_msg("MQTT Socket %d Connect: %s (%d)\r\n", serverInst->fd, MqttClient_ReturnCodeToString(rc), rc);
								mqttClientDeinit(serverInst);
							}
							else if(rc == MQTT_CODE_SUCCESS)
							{	dbg_msg("mqtt socket %d initial done\r\n", serverInst->fd);
								if(serverInst->fd != -1)
								{	mqttCtx->stat = WMQ_MQTT_CONN;
								
									msg.msgId = MQTT_Q_MSG_MQTT_CONNECT;
									msg.ptr = (char *)serverInst;
									if(sendMsgWithErrHandle(MQTT_TASK, SENS_MSG_Q_SEND(mqttQ, (uint32_t *)&msg, 0), __func__, __LINE__))
									{
									}
								}
							}
						}
						break;
			case MQTT_Q_MSG_MQTT_CONNECT:
						{	mqttClientConnectPrepare(mqttCtx);
							
							rc = MqttClient_Connect(&mqttCtx->client, &mqttCtx->connect, 0);
							dbg_msg("%s, MQTT sock %d Connect: %s (%d), return code:%d\r\n", __func__, serverInst->fd, MqttClient_ReturnCodeToString(rc), rc, mqttCtx->client.msg.connect_ack.return_code);
							if(rc != MQTT_CODE_SUCCESS)
							{	
							}
							else
							{	if(mqttCtx->client.msg.connect_ack.return_code == MQTT_CONNECT_ACCEPT)
								{	//mqttCtx->stat = WMQ_MQTT_CONN_WAIT;
									XMEMSET(&mqttCtx->subscribe, 0, sizeof(MqttSubscribe));
									mqttCtx->stat = WMQ_MQTT_CONN_DONE;
									
									setMqttSubscribeTopic(serverInst);
									
									mqttCtx->stat = WMQ_SUB;
									//goto MQTT_SUB_PROCESSS;
								}
								else
								{	goto MQTT_DISCONNECT;
								}
							}
						}
			case MQTT_Q_MSG_MQTT_SUB:
						{
#if SUPPORT_SENSTALK_MQTT_COMPRESSION
MQTT_SUB_PROCESSS:
#endif
							rc = MqttClient_Subscribe(&mqttCtx->client, &mqttCtx->subscribe);
							if(rc == MQTT_CODE_SUCCESS)
							{	mqttCtx->stat = WMQ_SUB_WAIT;
								
								mqttCtx->currentSubscribeIdx++;
								mqttCtx->stat = WMQ_SUB_DONE;
#if SUPPORT_SENSTALK_MQTT_COMPRESSION
								if(serverInst->serverType < MQTTS_ANIOT_BROKER)
								{	if(mqttCtx->currentSubscribeIdx != mqttCtx->subscribeCnt)
									{	memset(subscribeTopic, 0, 256);
										SENS_SPRINTF(subscribeTopic, "SensMini/%s/cReq", sensSys->guidString);
										mqttClientSubscribe(mqttCtx, subscribeTopic, MQTT_QOS_1, 0);
										goto MQTT_SUB_PROCESSS;
									}
								}
#endif
								mqttCtx->stat = WMQ_WAIT_MSG;
								mqttCtx->sendPolling = 1;
								//setSensminiTimer((pointer)mqttQ, MQTT_Q_MSG_PING, sock, SENSMINI_TIMER_MQTT_PING_TIMER, mqttCtx->pingTimeout, TIMER_MODE_TRIGGER);
								mqttCtx->pingTimeout = currTick;
#if SUPPORT_SENSTALK_MQTT || 1
								if((serverInst->serverType < MQTTS_ANIOT_BROKER) 
	#if YS_MQTT_BROKER
									|| (serverInst->serverType == MQTTS_YS_BROKER) || (serverInst->serverType == MQTT_YS_BROKER)
	#endif
									)
								{	struct SenstalkCmd *cmdMsg;
									struct TaskQMsg senstalkMsg;
									cmdMsg = SENS_MEM_ZALLOC(sizeof(struct SenstalkCmd));
									cmdMsg->cmdType = STK_CONNECT;
									cmdMsg->current_tick = currTick;
									cmdMsg->cmdProtocol = (void *)mqttCtx;

									senstalkMsg.msgId = SERV_CMD_PROCESS_Q_MSG_SEND_CMD;
									senstalkMsg.ptr = (char *)cmdMsg;

									if(sendMsgWithErrHandle(SERV_CMD_PROCESS_TASK, SENS_MSG_Q_SEND(servCmdProcessQ, (uint32_t *)&senstalkMsg, 0), __func__, __LINE__))
									{	SENS_MEM_FREE(cmdMsg);
									}
								}
								
								serverInst->socketState = SOCK_MQTT_CONNECT_DONE;
#endif
#if SUPPORT_FOTA
								//checkFota(mqttCtx);
#endif
#if MQTT_SEND_SUCCESS_WITH_TAG
								//checkHistorySendStatus(0);
#endif
							}
							else
							{	dbg_msg("Mqtt sock %d Subscribe error :%d\r\n", serverInst->fd, rc);
								sendCloseSocket(mqttCtx);
							}
						}
						break;
			case MQTT_Q_MSG_PING:
						{	MqttPing ping;
							XMEMSET(&ping, 0, sizeof(ping));
							dbg_msg("sock %d keep alive, send ping\r\n", serverInst->fd);
							rc = MqttClient_Ping_ex(&mqttCtx->client, &ping);
							if(rc == MQTT_CODE_SUCCESS)
							{	dbg_msg("sock %d send ping Done, rc:%d\r\n", serverInst->fd, rc);
								//setSensminiTimer((pointer)mqttQ, MQTT_Q_MSG_PING, sock, SENSMINI_TIMER_MQTT_PING_TIMER, mqttCtx->pingTimeout, TIMER_MODE_TRIGGER);
								//sendCloseSocket(mqttCtx); //test
								mqttCtx->pingTimeout = currTick;
								mqttCtx->stat = WMQ_WAIT_MSG;
							}
							else
							{	sendCloseSocket(mqttCtx);
							}
							break;
						}
			case MQTT_Q_MSG_PUBLISH:
						{	int orgCmdTimeout;
							MqttClient *client = &mqttCtx->client;
							MQTT_SEND_INFO *mqttSendMsg = (MQTT_SEND_INFO *)serverInst->mqttSendMsg;
							
							orgCmdTimeout = client->cmd_timeout_ms;
							if(mqttSendMsg->cmdTimeout)
								client->cmd_timeout_ms = mqttSendMsg->cmdTimeout;
							mqttClientPublisher(mqttCtx, mqttSendMsg->sendTopic, MQTT_QOS_1, 0, (byte *)mqttSendMsg->sendMessage, mqttSendMsg->sendMessageLen);
SEND_PUBLISH:
							rc = MqttClient_Publish(&mqttCtx->client, &mqttCtx->publish);
							if((rc != MQTT_CODE_WAIT) && (rc != MQTT_CODE_SUCCESS))
							{	dbg_msg("sock %d MqttClient Publish return error, code:%d\r\n", serverInst->fd, rc);
#ifdef OTA_STOP_ALL_CONNECTION
								if(networkCtx->otaRunning)
								{	client->cmd_timeout_ms = orgCmdTimeout;
									goto MQTT_DISCONNECT;
								}
#endif
								if(mqttCtx->errorRetryCnt >= 1)
								{	client->cmd_timeout_ms = orgCmdTimeout;
									sendCloseSocket(mqttCtx);
									if(SENS_EVENT_SET(mqttCtx->publishDoneEvent, 0x02) != MQX_OK)
									{	
									}

									//mqttCtx->stat = WMQ_WAIT_MOBILE_INITIAL_DONE;
									mqttCtx->errorRetryCnt = 0;
								}
								else
								{	mqttCtx->errorRetryCnt++;
									dbg_msg("sock %d MqttClient_Publish retry count:%d\r\n", serverInst->fd, mqttCtx->errorRetryCnt);
									goto SEND_PUBLISH;
								}
							}
							else
							{	mqttCtx->errorRetryCnt = 0;
								client->cmd_timeout_ms = orgCmdTimeout;
								mqttCtx->stat = WMQ_WAIT_MSG;
								//if(getTimerActivity(SENSMINI_TIMER_MQTT_PING_TIMER))
								//	setSensminiTimer((pointer)mqttQ, MQTT_Q_MSG_PING, sock, SENSMINI_TIMER_MQTT_PING_TIMER, mqttCtx->pingTimeout, TIMER_MODE_TRIGGER);
								mqttCtx->pingTimeout = currTick;
								if(SENS_EVENT_SET(mqttCtx->publishDoneEvent, 0x01) != MQX_OK)
								{	
								}
							}
							break;
						}
			case MQTT_Q_MSG_POLLING:
						{	
#ifdef OTA_STOP_ALL_CONNECTION
							if(networkCtx->otaRunning)
								goto MQTT_DISCONNECT;
#endif
							
							if(mqttCtx->stat == WMQ_WAIT_MSG)
							{	mqttCtx->sendPolling = 1;
								if((currTick - mqttCtx->pingTimeout) > ((long)DEFAULT_KEEP_ALIVE_SEC * 1000))
								{	msg.msgId = MQTT_Q_MSG_PING;
									msg.ptr = (char *)serverInst;
									mqttCtx->stat = WMQ_PING;
									if(sendMsgWithErrHandle(MQTT_TASK, SENS_MSG_Q_SEND(mqttQ, (uint32_t *)&msg, 0), __func__, __LINE__))
										mqttCtx->stat = WMQ_WAIT_MSG;
									else
										continue;
								}
								rc = MqttClient_WaitMessage(&mqttCtx->client, 50);
								if(rc == MQTT_CODE_SUCCESS)
								{	dbg_msg("sock %d get Message success!!\r\n", serverInst->fd);
									mqttCtx->pingTimeout = currTick;
									//if(getTimerActivity(SENSMINI_TIMER_MQTT_PING_TIMER))
									//	setSensminiTimer((pointer)mqttQ, MQTT_Q_MSG_PING, sock, SENSMINI_TIMER_MQTT_PING_TIMER, mqttCtx->pingTimeout, TIMER_MODE_TRIGGER);
									//gprs.nbWaitSockSendData = 0;
									//gprs.nbEnRemainMessageReceiveTimeout = 0;
								}
								else if(rc != MQTT_CODE_WAIT)
								{	if(rc == MQTT_CODE_WAIT_SOCK_READ_REMAIN_LEN)
									{	mqttCtx->pingTimeout = currTick;
										//if(getTimerActivity(SENSMINI_TIMER_MQTT_PING_TIMER))
										//	setSensminiTimer((pointer)mqttQ, MQTT_Q_MSG_PING, sock, SENSMINI_TIMER_MQTT_PING_TIMER, mqttCtx->pingTimeout, TIMER_MODE_TRIGGER);
										//gprs.nbWaitSockSendData = 1;
									}
									if(rc == MQTT_CODE_ERROR_TIMEOUT)
									{
									}
									//else
									//	printf("MqttClient_WaitMessage return error, code:%d\n", rc);
								}
							}
						}
						break;
			case MQTT_Q_MSG_DISCONNECT:
						mqttClientFreeCtx(mqttCtx);
						//goto MQTT_DISCONNECT;
						break;
			case MQTT_Q_MSG_EMPTY:
						continue;
		}
		
		if(mqttCtx)
		{	
#if defined OS_FREERTOS
			if(mqttCtx->sendPolling)
			{	mqttCtx->sendPolling = 0;
				SENS_TIME_DELAY(10);
				msg.msgId = MQTT_Q_MSG_POLLING;
				msg.ptr = (char *)serverInst;
				if(sendMsgWithErrHandle(MQTT_TASK, SENS_MSG_Q_SEND(mqttQ, (uint32_t *)&msg, 0), __func__, __LINE__))
				{	
				}
			}
#elif defined OS_MQX
			if(mqttCtx->stat == WMQ_WAIT_MSG)
			{	char sendQ = 1;

				LWMSGQ_STRUCT_PTR      q_ptr = (LWMSGQ_STRUCT_PTR)&mqttQ;
				_mqx_max_type_ptr     currPtr = q_ptr->MSG_READ_LOC;
				_mqx_max_type_ptr     startPtr = q_ptr->MSG_START_LOC;
				_mqx_max_type_ptr     maxPtr = q_ptr->MSG_END_LOC;
				_mqx_max_type_ptr     endPtr = q_ptr->MSG_WRITE_LOC;
				_int_disable();
				if(q_ptr->CURRENT_SIZE)
				{	while(1)
					{	if(currPtr == endPtr)
							break;
						if((currPtr[0] == (_mqx_max_type)MQTT_Q_MSG_POLLING) && (currPtr[1] == (_mqx_max_type)serverInst))
						{	sendQ = 0;
							break;
						}
						currPtr += q_ptr->MSG_SIZE;
						if(currPtr >= maxPtr)
							currPtr = startPtr;
					}
				}
				_int_enable();
				if(sendQ)
				{	SENS_TIME_DELAY(10);
					msg.msgId = MQTT_Q_MSG_POLLING;
					msg.ptr = (char *)serverInst;
					if(sendMsgWithErrHandle(MQTT_TASK, SENS_MSG_Q_SEND(mqttQ, (uint32_t *)&msg, 0), __func__, __LINE__))
					{	
					}
				}
			}
#endif
		}
		
		mqttCtx = NULL;
		continue;
MQTT_DISCONNECT:
		rc = MqttClient_Disconnect_ex(&mqttCtx->client, &mqttCtx->disconnect);
		dbg_msg("MQTT Disconnect: %s (%d)\r\n", MqttClient_ReturnCodeToString(rc), rc);
		rc = MqttClient_NetDisconnect(&mqttCtx->client);
		dbg_msg("MQTT Socket Disconnect: %s (%d)\r\n", MqttClient_ReturnCodeToString(rc), rc);
		mqttClientFreeCtx(mqttCtx);
	}
}

extern void setServerConnnectFail(CLOUD_SERVER_INSTANCE *servInst);
//------------------------------------------------------------------------------
/* so overall tests can pull in test function */
void mqttClientFreeCtx(MQTTCtx *mqttCtx)
{	MqttClient *client = (MqttClient *)&mqttCtx->client;
	CLOUD_SERVER_INSTANCE *servInst = getServerInstFromMqtt(mqttCtx);;

	if(mqttCtx->tx_buf) WOLFMQTT_FREE(mqttCtx->tx_buf);
	if(mqttCtx->rx_buf) WOLFMQTT_FREE(mqttCtx->rx_buf);

	MqttClientNet_DeInit(&mqttCtx->net);
	MqttClient_DeInit(client);
	mqtt_free_ctx(mqttCtx);
	mqttCtx->errorRetryCnt = 0;
	mqttCtx->currentSubscribeIdx = 0;
	mqttCtx->stat = WMQ_WAIT_MOBILE_INITIAL_DONE;
	
	if(servInst->fd != -1)
	{	dbgMsg("%s", "mqtt client close socket!!\r\n");
		setServerConnnectFail(servInst);
		lanSocketClose(servInst, 1, __func__, __LINE__);
	}
}
//------------------------------------------------------------------------------
void mqttClientSubscribe(MQTTCtx *mqttCtx, char *topic, MqttQoS qos, char num)
{
	//dbg_msg("%s enter\r\n", __func__);
	mqttCtx->topics[num].topic_filter = topic;
	mqttCtx->topics[num].qos = qos;

	mqttCtx->subscribe.packet_id = mqtt_get_packetid();
	mqttCtx->subscribe.topic_count = sizeof(mqttCtx->topics) / sizeof(MqttTopic);
	mqttCtx->subscribe.topics = mqttCtx->topics;
	mqttCtx->stat = WMQ_SUB;
}
//------------------------------------------------------------------------------
void mqttClientPublisher(MQTTCtx *mqttCtx, char *topic, MqttQoS qos, int retain, byte *message, int messageSize)
{	XMEMSET(&mqttCtx->publish, 0, sizeof(MqttPublish));
	//mqttCtx->publish.retain = retain;
	mqttCtx->publish.retain = 0;
	mqttCtx->publish.qos = qos;
	mqttCtx->publish.duplicate = 0;
	mqttCtx->publish.topic_name = topic;
	mqttCtx->publish.packet_id = mqtt_get_packetid();
	mqttCtx->publish.buffer = message;
	mqttCtx->publish.total_len = messageSize;

	mqttCtx->stat = WMQ_PUB;
}
//------------------------------------------------------------------------------
int mqttClientUnsubscribe(MQTTCtx *mqttCtx, char *topicStr)
{	MqttTopic *topic;
	int idx=0;

	for(idx=0;idx<sizeof(mqttCtx->topics);idx++)
	{	topic = (MqttTopic *)&mqttCtx->topics[idx];
		if(!memcmp(topic->topic_filter, topicStr, strlen(topicStr)))
		{	break;
		}
	}
	if(idx == sizeof(mqttCtx->topics))
		return 0;

	XMEMSET(&mqttCtx->unsubscribe, 0, sizeof(MqttUnsubscribe));
	mqttCtx->unsubscribe.packet_id = mqtt_get_packetid();
	mqttCtx->unsubscribe.topic_count = sizeof(mqttCtx->topics) / sizeof(MqttTopic);
	mqttCtx->unsubscribe.topics = mqttCtx->topics;
	return 1;
}
//------------------------------------------------------------------------------
static int mqttClientNetInitial(CLOUD_SERVER_INSTANCE *serverInst)
{	return MqttClientNet_Init(serverInst);
}
//------------------------------------------------------------------------------
static void mqttClientDeinit(CLOUD_SERVER_INSTANCE *serverInst)
{	MQTTCtx *mqttCtx = (MQTTCtx *)serverInst->protCtx;
	
	/*mqttCtx->stat = WMQ_DONE;
	
	if(mqttCtx->tx_buf) WOLFMQTT_FREE(mqttCtx->tx_buf);
	if(mqttCtx->rx_buf) WOLFMQTT_FREE(mqttCtx->rx_buf);
	//sockCtx->fd = -1;
	//sockCtx->stat = SOCK_BEGIN;
	MqttClientNet_DeInit(&mqttCtx->net);
	MqttClient_DeInit(&mqttCtx->client);
	*/
	mqttClientFreeCtx(mqttCtx);
}
//------------------------------------------------------------------------------
int mqttClientInitial(CLOUD_SERVER_INSTANCE *serverInst)
{	int rc;
	MQTTCtx *mqttCtx = (MQTTCtx *)serverInst->protCtx;

	if(mqttCtx == NULL)
	{	mqttCtx = SENS_MEM_ZALLOC(sizeof(MQTTCtx));
		serverInst->protCtx = (void *)mqttCtx;
		mqttCtx->app_ctx = serverInst;
	}
	
	if(!mqttCtx->eventCreated)
	{	mqttCtx->eventCreated = 1;
		if(SENS_EVENT_CREATE(mqttCtx->writeDoneEvent, LWEVENT_AUTO_CLEAR) != MQX_OK )
		{
		}
		if(SENS_EVENT_CREATE(mqttCtx->publishDoneEvent, LWEVENT_AUTO_CLEAR) != MQX_OK )
		{
		}
	}
	
	/* init defaults */
	mqtt_init_ctx(serverInst);
	mqttCtx->app_name = "mqttclient";
	
	rc = mqttClientNetInitial(serverInst);
	if(rc != MQTT_CODE_SUCCESS)
	{	mqttClientDeinit(serverInst);
		return rc;
	}
	mqttCtx->tx_buf = (byte*)WOLFMQTT_MALLOC(MQTT_MAX_BUFFER_SIZE);
	mqttCtx->rx_buf = (byte*)WOLFMQTT_MALLOC(MQTT_MAX_BUFFER_SIZE);
	
	rc = MqttClient_Init(&mqttCtx->client, &mqttCtx->net, mqtt_message_cb, mqtt_message_len_error_cb, mqttCtx->tx_buf, MQTT_MAX_BUFFER_SIZE, mqttCtx->rx_buf, MQTT_MAX_BUFFER_SIZE, mqttCtx->cmd_timeout_ms);
	dbg_msg("MQTT Init: %s (%d)\r\n", MqttClient_ReturnCodeToString(rc), rc);
	if(rc != MQTT_CODE_SUCCESS)
	{	mqttClientDeinit(serverInst);
		return rc;
	}
	mqttCtx->client.ctx = mqttCtx;
#ifdef WOLFMQTT_DISCONNECT_CB
	/* setup disconnect callback */
	rc = MqttClient_SetDisconnectCallback(&mqttCtx->client, mqtt_disconnect_cb, NULL);
	if(rc != MQTT_CODE_SUCCESS)
	{	mqttClientDeinit(mqttCtx);
		return rc;
	}
#endif
#ifdef WOLFMQTT_PROPERTY_CB
	rc = MqttClient_SetPropertyCallback(&mqttCtx->client, mqtt_property_cb, NULL);
	if(rc != MQTT_CODE_SUCCESS)
	{	mqttClientDeinit(mqttCtx);
		return rc;
	}
#endif

	mqttCtx->stat = WMQ_TCP_CONN;
	
	return MQTT_CODE_SUCCESS;
}
//------------------------------------------------------------------------------
int sendMqttQ(MQTTCtx *mqttCtx, int id)
{	struct TaskQMsg msg;
	MqttNet *mqttNet = &mqttCtx->net;
	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)mqttNet->context;
	char sendQ = 1;
	char firstQ = 1;
	
#ifdef OS_MQX
	LWMSGQ_STRUCT_PTR      q_ptr = (LWMSGQ_STRUCT_PTR)&mqttQ;
	_mqx_max_type_ptr     currPtr = q_ptr->MSG_READ_LOC;
	_mqx_max_type_ptr     startPtr = q_ptr->MSG_START_LOC;
	_mqx_max_type_ptr     maxPtr = q_ptr->MSG_END_LOC;
	_mqx_max_type_ptr     endPtr = q_ptr->MSG_WRITE_LOC;
	
	if(id == MQTT_Q_MSG_DISCONNECT)
	{	//killSensminiTimer(SENSMINI_TIMER_MQTT_PING_TIMER);
		_int_disable();
		if(q_ptr->CURRENT_SIZE)
		{	while(1)
			{	if(currPtr == endPtr)
					break;
				if((currPtr[1] == (_mqx_max_type)serverInst))
				{	if(firstQ)
					{	firstQ = 0;
						currPtr[0] = MQTT_Q_MSG_DISCONNECT;
					}
					else
						currPtr[0] = MQTT_Q_MSG_EMPTY;
					sendQ = 0;
				}
				currPtr += q_ptr->MSG_SIZE;
				if(currPtr >= maxPtr)
					currPtr = startPtr;
			}
		}
		_int_enable();
	}
#endif
	if(sendQ)
	{	msg.msgId = id;
		msg.ptr = (char *)serverInst;
#ifdef OS_FREERTOS
		if(id == MQTT_Q_MSG_DISCONNECT)
		{	xQueueReset(mqttQ);
		}
#endif
		if(sendMsgWithErrHandle(MQTT_TASK, SENS_MSG_Q_SEND(mqttQ, (uint32_t *)&msg, 0), __func__, __LINE__))
		{	return -1;
		}
	}
	return 0;
}

void sendCloseSocket(MQTTCtx *mqttCtx)
{	//struct TaskQMsg msg;
	//MqttNet *mqttNet = &mqttCtx->net;
	//CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)mqttNet->context;
	
	/*msg.msgId = MOBILE_Q_MSG_SOCK_CLOSE_SELF;
	msg.ptr = (char *)serverInst;
	if(sendMsgWithErrHandle(MOBILE_SEND_TASK, SENS_MSG_Q_SEND(mobileQ, (uint_32 *)&msg, 0), __func__, __LINE__))
	{	
	}
	*/
	mqttCtx->stat = WMQ_NET_DISCONNECT;
	sendMqttQ(mqttCtx, MQTT_Q_MSG_DISCONNECT);
}

CLOUD_SERVER_INSTANCE *getServerInstFromMqtt(MQTTCtx *mqttCtx)
{	MqttNet *mqttNet = &mqttCtx->net;
	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)mqttNet->context;
	
	return serverInst;
}
#endif
