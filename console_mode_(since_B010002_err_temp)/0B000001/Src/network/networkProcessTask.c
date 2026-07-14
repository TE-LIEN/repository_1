#include "sensCommon.h"
#include "network/netApp.h"
#include "sensSystem.h"
#include "sensDev.h"
#include "physicalQuantity.h"
#include "math.h"
#include "network/protocol/serverProtocol.h"
#include "systemTimer.h"

volatile NETWORK_CONTEXT *networkCtx;

#if defined OS_MQX
_mqx_max_type  networkProcessQ[NETWORK_PROCESS_Q_SIZE]; /* prepare message queue for 20 events */
#elif defined OS_FREERTOS
QueueHandle_t networkProcessQ;
#endif

//------------------------------------------------------------------------------
static int initServerInst(void)
{	CLOUD_SERVER_INSTANCE	*servInst;
	int idx;
	
	for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
	{	servInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
		servInst->sockRecvQueue = (SOCKET_QUEUE *)SENS_MEM_ZALLOC(sizeof(SOCKET_QUEUE));
		//servInst->sockSendQueue = (SOCKET_QUEUE *)SENS_MEM_ZALLOC(sizeof(SOCKET_QUEUE));
		servInst->fd = -1;
		servInst->servIdx = -1;
		servInst->serverType = -1;
		if(SENS_SEM_INIT(servInst->lock, 1) != MQX_OK)
		{
		}
		if(SENS_SEM_INIT(servInst->sockConnLock, 1) != MQX_OK)
		{
		}
#if AUTO_DATA_SYNC
		if(SENS_SEM_INIT(servInst->autoDataSyncCtxLock, 1) != MQX_OK)
		{
		}
#endif

#if SOCK_CONN_USE_TASK_BLOCK
		sockConnectTaskInf[idx] = SENS_NULL_TASK_ID;
		ioaSockConnectTaskInf[idx] = SENS_NULL_TASK_ID;
#endif
	}
  return 0;
}
//------------------------------------------------------------------------------
static int networkTaskInit(void)
{	int idx;
	HTTP_CONTEXT *httpCtx;
	NET_CONFIG *netCfg;
	WIRELESS_CONFIG	*wirelessCfg;
	
	if(MQX_OK != SENS_MSG_Q_INIT(networkProcessQ, LAN_NUM_MESSAGES, SENS_TASKQ_GRANM)) 
	{	return -1;
	}
	
	initServerInst();
	
	if(SENS_SEM_INIT(networkCtx->timeZoneLock, 1) != MQX_OK)
	{
	}
	
	if(SENS_SEM_INIT(networkCtx->lock, 1) != MQX_OK)
	{
	}
	setupEthernetComm();
	netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	wirelessCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
	if((sysTimer->currHistoryRecTimeSlot % wirelessCfg->mobileInterval) == (wirelessCfg->mobileInterval - 1) || (sysCtrl->pwrManager.currPsm == SYS_CFG_PSM_NONE))
		networkCtx->timeToEnableNetwork = 1;
	
	if(!networkCtx->timeToEnableNetwork)
		networkCtx->checkForNextWlWakeup = 1;
	
	setupMobileComm();
	
	for(idx=0;idx<MAX_WIRE_LAN_SOCK;idx++)
	{	networkCtx->iSkts[idx] = RTCS_SOCKET_ERROR;	//EMPTY
	}
	networkCtx->sockCnt = 0;
	networkCtx->idImgHttpCtx = SENS_MEM_ZALLOC(sizeof(HTTP_CONTEXT));
	httpCtx = networkCtx->idImgHttpCtx;
	
	httpCtx->url = SENS_MEM_ZALLOC(HTTP_URL_STR_MAX_LEN);
	httpCtx->path = SENS_MEM_ZALLOC(HTTP_PATH_STR_MAX_LEN);
	httpCtx->hostStr = SENS_MEM_ZALLOC(HTTP_URL_STR_MAX_LEN);
	
	networkCtx->twHttpCtx = SENS_MEM_ZALLOC(sizeof(HTTP_CONTEXT));
	httpCtx = networkCtx->twHttpCtx;
	
	httpCtx->url = SENS_MEM_ZALLOC(HTTP_URL_STR_MAX_LEN);
	httpCtx->path = SENS_MEM_ZALLOC(HTTP_PATH_STR_MAX_LEN);
	httpCtx->hostStr = SENS_MEM_ZALLOC(HTTP_URL_STR_MAX_LEN);
	//httpCtx->fileName = SENS_MEM_ZALLOC(HTTP_FILENAME_STR_MAX_LEN);
	return 0;
}
//------------------------------------------------------------------------------
int networkPreInit(void)
{	NET_INSTANCE	*workingInst;
	NET_CONFIG		*netCfg;
	UART_CONFIG 	*iotComm;
	
	if(!sysCfg->sensSysCfg.psm)
		networkCtx->timeToEnableNetwork = 1;
	if(networkCtx->timeToEnableNetwork)
	{	if(networkCtx->workingInst == NULL)
		{	netCfg = (NET_CONFIG *)&sysCfg->netCfg;
			
			if(netCfg->connPriority != COMM_PRIO_WIRE_LAN)
			{	
SET_TO_MOBILE:
				if(networkCtx->lsCommInst && (!networkCtx->lsCommInst->initError))
				{	dbg_msg("%s", "start LS channel\r\n");
					networkCtx->workingInst = networkCtx->lsCommInst;
				}
				else if(networkCtx->hsCommInst && (!networkCtx->hsCommInst->initError))
				{	dbg_msg("%s", "start HS channel\r\n");
					networkCtx->workingInst = networkCtx->hsCommInst;
				}
				else
				{	dbg_msg("%s", "start ETH channel\r\n");
					networkCtx->workingInst = networkCtx->lanInst;
				}
			}
			else
			{	//if(!sysStatus->wiredNetDisconnect && !networkCtx->lanInst->initError)
				{	networkCtx->workingInst = networkCtx->lanInst;
				}
				/*else
				{	networkCtx->lanInst->initError = 0;
					if(networkCtx->lsCommInst)
						networkCtx->lsCommInst->initError = 0;
					if(networkCtx->hsCommInst)
						networkCtx->hsCommInst->initError = 0;
					goto SET_TO_MOBILE;
				}*/
			}
		}
		workingInst = networkCtx->workingInst;
		if(workingInst->netType == NETWORK_TYPE_LS)
		{	workingInst->connectToCellSite = 0;
			initLsIo(workingInst);
#if defined SENSMINIA4_NEO
			networkCtx->mobileUartStartRecv = 1;
#else
			iotComm = sensDev->lsComm;
			if(iotComm->ctx)
			{	workingInst->wlInst->dev = iotComm->ctx;
				networkCtx->mobileUartStartRecv = 1;
			}
			else if(networkCtx->hsCommInst && (!networkCtx->hsCommInst->initError))
			{	networkCtx->workingInst = networkCtx->hsCommInst;
			}
			else
				networkCtx->workingInst = networkCtx->lanInst;
#endif
		}
		if(workingInst->netType == NETWORK_TYPE_HS)
		{	int hsInitResult;
			workingInst->connectToCellSite = 0;
			hsInitResult = initialHsIo(workingInst);
			if(!hsInitResult)
			{	
#ifndef SENSMINIA4_NEO
				iotComm = sensDev->hsComm;
				workingInst->wlInst->dev = iotComm->ctx;
#endif
				networkCtx->mobileUartStartRecv = 1;
			}
			else
			{	networkCtx->workingInst = networkCtx->lanInst;
			}
		}
		else if(workingInst->netType == NETWORK_TYPE_ETH)
		{	workingInst->connectToCellSite = 1;
		}
		/*if(workingInst->netType == NET_WORKING_TYPE_WIREED)
		{
		}*/
	}
	else
	{	if(networkCtx->hsCommInst || networkCtx->lsCommInst)
		{	return -1;
		}
	}
	
	return 0;
}
//------------------------------------------------------------------------------
static int networkInit(void)
{	NET_INSTANCE	*workingInst;
	MOBILE_INSTANCE *wlInst;
	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	WIRED_CONFIG *wiredCfg = (WIRED_CONFIG *)&netCfg->wiredCfg;
	
	workingInst = networkCtx->workingInst;
	if(workingInst->netType != NETWORK_TYPE_ETH)
	{	wlInst = workingInst->wlInst;
		return iotMobileInit(wlInst);
	}
	else
	{	//workingInst->enable = 1;
		//networkCtx->networkIsUp = 1;
		if(networkCtx->lsCommInst)
			networkCtx->lsCommInst->initError = 0;
		if(networkCtx->hsCommInst)
			networkCtx->hsCommInst->initError = 0;
		if(wiredCfg->ipAddr == SENS_DEFAULT_IP)
		{	return -1;
		}
		return 0;
	}
}
//------------------------------------------------------------------------------
int networkDeInit(void)
{	NET_INSTANCE	*workingInst;
	MOBILE_INSTANCE *wlInst;
	int idx;
	CLOUD_SERVER_INSTANCE *serverInst=NULL;
	
	if(networkCtx->workingInst == NULL)
		return -1;
	
	workingInst = networkCtx->workingInst;
	
	networkCtx->networkIsUp = 0;
	workingInst->enable = 0;
	
	dbgMsg("De-init, channel:%s\r\n", (workingInst->netType == NETWORK_TYPE_LS)? "LS":(workingInst->netType == NETWORK_TYPE_HS)? "HS":"ETH");
	
	for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
	{	serverInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
		if(serverInst->fd != -1)
		{	SENS_SEM_LOCK(serverInst->lock);
			lanSocketClose(serverInst, 1, __func__, __LINE__);
			//clearReceiveQueue(serverInst);
			SENS_SEM_UNLOCK(serverInst->lock);
		}
	}
	
	sensPq->onboardPq[ON_BOARD_PQ_LTE_STS] = 0;
	sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] = -115;
	sensPq->onboardPq[ON_BOARD_PQ_NB_STS] = 0;
	sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] = -115;
	
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LTE_STS, sensPq->onboardPq[ON_BOARD_PQ_LTE_STS]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LTE_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_NB_STS, sensPq->onboardPq[ON_BOARD_PQ_NB_STS]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_NB_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH]);
	
	
	if(workingInst->netType != NETWORK_TYPE_ETH)
	{	workingInst->initError = 1;
		wlInst = workingInst->wlInst;
		SENS_SEM_LOCK(iotSys.iotSem);
		mobileDeinit(wlInst);
		networkCtx->workingInst = NULL;
		SENS_SEM_UNLOCK(iotSys.iotSem);
		return 0;
	}
	else
		workingInst->initError = 1;
	
	networkCtx->mobileUartStartRecv = 0;
	networkCtx->workingInst = NULL;
	//networkCtx->netInst = NULL;
	//SENS_TIME_DELAY(2000);
	return 0;
}
//------------------------------------------------------------------------------
void setServerConnnectFail(CLOUD_SERVER_INSTANCE *servInst)
{	int idx;
	
	CLOUD_SERVER_INFO	*servInfo;
	
	servInfo = getServerInfoByServInst(servInst);
	if(servInfo)
	{	servInfo->connectionFail = 1;
		servInfo->waitForConnectionTimer = GetTimeTicks();
	}

	if(servInst->servIdx < UPLOAD_SERVER_CNT)
		networkCtx->cloudActiveBmp &= ~(1 << servInst->servIdx);
}
//------------------------------------------------------------------------------
int checkAllServerConnectFail(void)
{	int idx;
	CLOUD_SERVER_INFO *servInfo;
	uint8_t failFlag = 0;
	uint8_t servFlag = 0;
#if YS_MQTT_BROKER
	int8_t	ysMqttConnectFail = 0;
#endif
	
	//if(pingServer())
	servInfo = sysCfg->servInfos;
	
	while(servInfo)
	{	
#if YS_MQTT_BROKER
		if((servInfo->serverProtocol != MQTTS_YS_BROKER) && (servInfo->serverProtocol != MQTT_YS_BROKER))
#endif
		{	servFlag |= (1 << servInfo->servIdx);
			if(servInfo->connectionFail)
				failFlag |= (1 << servInfo->servIdx);
		}
#if YS_MQTT_BROKER
		else
		{	ysMqttConnectFail = servInfo->connectionFail;
		}
#endif
		servInfo = servInfo->next;
	}
	if(failFlag == servFlag
#if YS_MQTT_BROKER
		&& ysMqttConnectFail
#endif
		)
		return -1;
	else
		return 0;
}
//------------------------------------------------------------------------------
void networkProcessTask(void *param)
{	struct TaskQMsg msg;	

#if (defined PLATFORM_FSL) || ((defined NET_LWIP) && (defined NETWORK_USE_SELECT))
	initialTaskStruct((TASK_INFO *)&taskAct.networkRecvTaskAct);
	SENS_SEM_LOCK(taskAct.networkRecvTaskAct.lock);
	taskAct.networkRecvTaskAct.id = OSA_TASK_CREATE(NETWORK_RECV_TASK);
	if(taskAct.networkRecvTaskAct.id == SENS_NULL_TASK_ID)
	{
	}
#endif
	
	initialTaskStruct((TASK_INFO *)&taskAct.iotAtRecvTaskAct);
	SENS_SEM_LOCK(taskAct.iotAtRecvTaskAct.lock);
	taskAct.iotAtRecvTaskAct.id = OSA_TASK_CREATE(MOBILE_RECV_TASK);
	if(taskAct.iotAtRecvTaskAct.id == SENS_NULL_TASK_ID)
	{	taskAct.iotAtRecvTaskAct.sts = TASK_STS_ABORT;
		SENS_TASK_BLOCK();
	}
	
	if(SENS_SEM_INIT(iotSys.iotSem, 1) != MQX_OK)
	{	dbg_msg("%s", "Create mobile sema fail\r\n");
	}
	
#ifdef OS_FREERTOS
	SENS_TIME_DELAY(2);
#endif
	
	SENS_SEM_LOCK(taskAct.networkProcessTaskAct.lock);
#if (defined PLATFORM_FSL) || ((defined NET_LWIP) && (defined NETWORK_USE_SELECT))
	SENS_SEM_UNLOCK(taskAct.networkRecvTaskAct.lock);
#endif
	taskAct.networkProcessTaskAct.sts = TASK_STS_INITIAL;
	networkTaskInit();
	msg.msgId = NETWORK_Q_MSG_NETWORK_PRE_INIT;
	msg.ptr = NULL;
	
	if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{
	}
	SENS_SEM_UNLOCK(taskAct.iotAtRecvTaskAct.lock);
	while(1)
	{	taskAct.networkProcessTaskAct.sts = TASK_STS_BLOCKED;
		SENS_MSG_Q_RECV(networkProcessQ, (_mqx_max_type *)&msg, LWMSGQ_RECEIVE_BLOCK_ON_EMPTY, 0, 0);
		taskAct.networkProcessTaskAct.sts = TASK_STS_RUNNING;
		extendTaskTimer((TASK_INFO *)&taskAct.networkProcessTaskAct);
		switch(msg.msgId)
		{	case NETWORK_Q_MSG_NETWORK_PRE_INIT:
					if(!networkPreInit())
					{	msg.msgId = NETWORK_Q_MSG_NETWORK_INIT;
						msg.ptr = NULL;
						if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
						{	
						}
					}
					break;
			case NETWORK_Q_MSG_NETWORK_INIT:
					{	int rsp;
						NET_INSTANCE *workingInst = networkCtx->workingInst;
						MOBILE_INSTANCE *wlInst = workingInst->wlInst;
							
						rsp = networkInit();
						if(rsp != NW_INIT_ERR_OK)
						{	msg.msgId = NETWORK_Q_MSG_LAN_DEINIT;
							msg.ptr = NULL;
							if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
							{	
							}
						}
						else
						{	/*if(wlInst && sensSys->enableGps && wlInst->supportAgps)
							{
							}
							else*/
							{	workingInst->enable = 1;
								networkCtx->networkIsUp = 1;
								//resetServIdleXmitTimer(sensSys->srvActiveBmp);
							}
						}
					}
					break;
			case NETWORK_Q_MSG_LAN_DEINIT:
					networkDeInit();
					msg.msgId = NETWORK_Q_MSG_NETWORK_PRE_INIT;
					msg.ptr = NULL;
					if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
					{	
					}	
					break;
			case NETWORK_Q_MSG_SEND_CMD:
					{	MOBILE_WRITE_SOCKET_CMD *mobileWriteSocketCmd = (MOBILE_WRITE_SOCKET_CMD *)msg.ptr;
						CLOUD_SERVER_INSTANCE	*serverInst;
              
#if SUPPORT_AGPS
						if(checkGpsWorking(&msg))
							break;
#endif
						serverInst = mobileWriteSocketCmd->serverInst;
						if(networkCtx->networkIsUp)
						{	dbg_msg("%s", "[LAN] msg is LAN_Q_MSG_SEND_CMD\r\n");
							tcpSockSend(mobileWriteSocketCmd);
							if(mobileWriteSocketCmd->cmdIsFromInternet)
							{	if(serverInst->countOfProcessCmdFromInternet)
									serverInst->countOfProcessCmdFromInternet--;
							}
							resetServIdleXmitTimer(1 << serverInst->servIdx);
							//networkCtx->workingInst->noXmitTimer = GetTimeTicks();
						}
						SENS_MEM_FREE(mobileWriteSocketCmd->buf);
						SENS_MEM_FREE(mobileWriteSocketCmd);
						break;
					}
			case NETWORK_Q_MSG_TCP_SOCK_CREATE:
					{	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)msg.ptr;
						dbg_msg("[LAN] msg is LAN_Q_MSG_TCP_SOCK_CREATE, serv Idx:%d\r\n", serverInst->servIdx);
						tcpClientSockCreate(serverInst);
						dbg_msg("LAN_Q_MSG_TCP_SOCK_CREATE FINISH, serv Idx %d\r\n", serverInst->servIdx);
					}
					break;
			case NETWORK_Q_MSG_TCP_SOCK_CONNECT:
					{	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)msg.ptr;
#if SUPPORT_AGPS
						if(checkGpsWorking(&msg))
							break;
#endif
						dbg_msg("%s", "[LAN] msg is LAN_Q_MSG_TCP_SOCK_CONNECT\r\n");
						if(tcpClientSockConnect(serverInst))
						{	setServerConnnectFail(serverInst);
							if(serverInst->xmitProtocolType == PROTOCOL_MQTT)
							{	MQTTCtx *mqttCtx = (MQTTCtx *)serverInst->protCtx;
								int timeout=0;
								serverInst->socketState = SOCK_CONN_FAIL;
								while(1)
								{	SENS_TIME_DELAY(200);
									timeout++;
									if(mqttCtx->stat == WMQ_WAIT_MOBILE_INITIAL_DONE)
										break;
									if(timeout > 5)
										break;
								}
							}
							lanSocketClose(serverInst, 1, __func__, __LINE__);
							if(checkAllServerConnectFail())
							{	//workingInst = networkCtx->workingInst;
								//networkCtx->workingInst = NULL;
								msg.msgId = NETWORK_Q_MSG_LAN_DEINIT;
								msg.ptr = NULL;
								if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
								{	
								}
							}
						}
						//dbg_msg("%s", "LAN_Q_MSG_TCP_SOCK_CONNECT SUCCESS\r\n");
					}
					break;
			case NETWORK_Q_MSG_MOBILE_UREG_GET:
					{	NET_INSTANCE *workingInst = networkCtx->workingInst;
						MOBILE_INSTANCE *wlInst = workingInst->wlInst;
						if(wlInst)
							extendedPacketSwitchedNetworkRegistrationStatus(wlInst);
					}
					break;
			case NETWORK_Q_MSG_MOBILE_CSQ_GET:
					{	NET_INSTANCE *workingInst = networkCtx->workingInst;
						MOBILE_INSTANCE *wlInst = workingInst->wlInst;
						if(wlInst)
						{	getSignalQuality(wlInst);
							getClock(wlInst);
						}
					}
					break;
#if SUPPORT_AGPS
			case NETWORK_Q_MSG_AGPS_LOCATION_GET:
					{	NET_INSTANCE *workingInst = networkCtx->workingInst;
						MOBILE_INSTANCE *wlInst = workingInst->wlInst;
						
						if(wlInst)
						{	if((wlInst->module == MOBILE_TYPE_LE910C1) || (wlInst->module == MOBILE_TYPE_LE910C4))
								gpsGetAcquiredPosition(wlInst);
							else
							{	if(workingInst->currentIsGpsOnlyMode)
									gpsGetAcquiredPosition(wlInst);
							}
						}
					}
					break;
			case NETWORK_Q_MSG_AGPS_LOCATION_GET_ONCE:
					{	NET_INSTANCE *workingInst = networkCtx->workingInst;
						MOBILE_INSTANCE *wlInst = workingInst->wlInst;
						HttpCtx *httpCtx = networkCtx->httpCtx;

						//if(wlInst && (wlInst->module == MOBILE_TYPE_ME310G) && (!httpCtx->running) && (!networkCtx->otaRunning))
						if(wlInst && (wlInst->module == MOBILE_TYPE_ME310G) && (!networkCtx->imgSending) && (!networkCtx->otaRunning))
						{	//gpsEnable(mPciInf, 1);
							switchGpsWan(wlInst, 1);
							wlInst->gpsRunMode = GPS_TRIG_MODE;
							//wlInst->currentIsGpsOnlyMode = 0;
							//gpsGetAcquiredPosition(mPciInf);
							//gpsEnable(mPciInf, 0);
							setSensminiTimer((pointer)wireLanQ, 		NETWORK_Q_MSG_AGPS_LOCATION_GET,	NULL, SENSMINI_TIMER_GPS_GET_POSITION, 		GPS_ME310_GET_TIMEOUT, 	TIMER_MODE_LOOPING);
						}
						//setSensminiTimer((pointer)wireLanQ, NETWORK_Q_MSG_AGPS_LOCATION_GET_ONCE, NULL, SENSMINI_TIMER_GPS_RESTART, GPS_RESTART_TIMEOUT, TIMER_MODE_TRIGGER);
						setSensminiTimer((pointer)wireLanQ, NETWORK_Q_MSG_AGPS_LOCATION_GET_ONCE, NULL, SENSMINI_TIMER_GPS_RESTART, (sensSys->gpsPollInterval * 1000), TIMER_MODE_TRIGGER);
					}
					break;
#endif
#if SUPPORT_IOA_WEB_API
			case NETWORK_Q_MSG_IOA_GET_TOKEN:
					{	IOA_SERVER_CTX *ioaServCtx;
						HTTP_CONTEXT *httpCtx;
						CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)msg.ptr;
						int error;
	#if SUPPORT_AGPS
						if(checkGpsWorking(&msg))
							break;
	#endif
						ioaServCtx = (IOA_SERVER_CTX *)serverInst->protCtx;
						httpCtx = (HTTP_CONTEXT *)&ioaServCtx->httpCtx;
						if(httpCtx->url)
							SENS_MEM_FREE(httpCtx->url);
						httpCtx->url = SENS_MEM_ZALLOC(strlen(serverInst->serverName) + 1);
						strcpy(httpCtx->url, serverInst->serverName);
						if(httpCtx->path)
							SENS_MEM_FREE(httpCtx->path);
						httpCtx->path = SENS_MEM_ZALLOC(strlen(IOAWEB_API_PATH_AUTH) + 1);
						strcpy(httpCtx->path, IOAWEB_API_PATH_AUTH);
						httpCtx->httpCmd = HTTP_POST;
						error = ioaGetToken(serverInst, httpCtx);
						if(error)
						{	setServerConnnectFail(serverInst);
							lanSocketClose(serverInst, 1, __func__, __LINE__);
						}
							
						break;
					}
			case NETWORK_Q_MSG_IOA_SEND_DATA:
					{	CLOUD_SERVER_INSTANCE *serverInst;
						IOA_SERVER_CTX *ioaServCtx;
						HTTP_CONTEXT *httpCtx;
	#if SUPPORT_AGPS
						if(checkGpsWorking(&msg))
							break;
	#endif
						serverInst = (CLOUD_SERVER_INSTANCE *)msg.ptr;
						ioaServCtx = (IOA_SERVER_CTX *)serverInst->protCtx;
						httpCtx = (HTTP_CONTEXT *)&ioaServCtx->httpCtx;
						if(httpCtx->url)
							SENS_MEM_FREE(httpCtx->url);
						httpCtx->url = SENS_MEM_ZALLOC(strlen(serverInst->serverName)+1);
						memcpy(httpCtx->url, serverInst->serverName, strlen(serverInst->serverName));
						if(httpCtx->path)
							SENS_MEM_FREE(httpCtx->path);
						httpCtx->path = SENS_MEM_ZALLOC(strlen(IOAWEB_API_PATH_SENSOR_DATA)+1);
						memcpy(httpCtx->path, IOAWEB_API_PATH_SENSOR_DATA, strlen(IOAWEB_API_PATH_SENSOR_DATA));
						ioaSendData(serverInst, httpCtx);
						break;
					}
			case NETWORK_Q_MSG_IOA_RSP:
					{	HTTP_CLI_PARSER *httpCliParserCtx = (HTTP_CLI_PARSER *)msg.ptr;
						HTTP_RSP_INFO *httpRspInfo = (HTTP_RSP_INFO *)httpCliParserCtx->httpRspInfo;
						//HttpCtx *httpCtx;
						CLOUD_SERVER_INSTANCE *serverInst;
						IOA_SERVER_CTX *ioaServCtx;
							
						serverInst = httpRspInfo->servInst;
						ioaServCtx = (IOA_SERVER_CTX *)serverInst->protCtx;
						//httpCtx = (HttpCtx *)&ioaServCtx->httpCtx;

						if(httpRspInfo->status == 200)
						{	if(!strncmp(httpRspInfo->contentType, "application/json", strlen("application/json")))
							{	if(!ioaRspParse(ioaServCtx, httpRspInfo->content))
								{	if(ioaServCtx->fsm == IOA_WEB_API_FSM_DATA_SENDING)
										ioaServCtx->fsm = IOA_WEB_API_FSM_GET_TOKEN;
									else
										ioaServCtx->fsm = IOA_WEB_API_FSM_TOKEN_DONE;
								}
								else
									ioaServCtx->fsm = IOA_WEB_API_FSM_GET_TOKEN;
							}
						}
						else
						{	ioaServCtx->fsm = IOA_WEB_API_FSM_GET_TOKEN;
						}
						/*if(httpRspInfo->contentType)
							SENS_MEM_FREE(httpRspInfo->contentType);
						if(httpRspInfo->content)
							SENS_MEM_FREE(httpRspInfo->content);
						SENS_MEM_FREE(httpRspInfo);*/
						break;
					}
#endif
			case NETWORK_Q_MSG_HTTP_POST:
					{	HTTP_CONTEXT *httpCtx;
						IMAGE_UPLOAD_INFO *imgUploadInf = (IMAGE_UPLOAD_INFO *)msg.ptr;
#if SUPPORT_AGPS
						if(checkGpsWorking(&msg))
							break;
#endif
						httpCtx = selectOtaSvrHttpCtx();
						if(httpCtx->running)
						{	SENS_TIME_DELAY(2000);
							msg.msgId = NETWORK_Q_MSG_HTTP_POST;
							msg.ptr = (char *)imgUploadInf;
							if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
							{	
							}
							if(imgUploadInf)
								SENS_MEM_FREE(imgUploadInf);
							break;
						}
						
						timeLapseImageSend(imgUploadInf);
						break;
					}
			case NETWORK_Q_MSG_HTTP_POST_DONE:
					{	HTTP_CLI_PARSER *httpCliParserCtx = (HTTP_CLI_PARSER *)msg.ptr;
						HTTP_RSP_INFO *httpRspInfo = (HTTP_RSP_INFO *)httpCliParserCtx->httpRspInfo;
						HTTP_CONTEXT *httpCtx;
						CLOUD_SERVER_INSTANCE *serverInst;
						IMAGE_UPLOAD_REC_HEADER *imgUploadRecHeader;
						IMAGE_UPLOAD_INSTANCE	*imgUploadInst = (IMAGE_UPLOAD_INSTANCE	*)networkCtx->imgUploadInst;
              
						serverInst = findImgServerInst();
						if(serverInst == NULL)
							break;
						dbg_msg("http post done, status:%d\r\n", httpRspInfo->status);
						
						killSensminiTimer(SENSMINI_TIMER_LAN_HTTP_POST);
						httpCtx = selectOtaSvrHttpCtx();
						if(httpRspInfo->status == 200)
						{	httpCtx->running = 0;
							httpCtx->failCount = 0;
							SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
							imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_SEND_DONE;
							SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
							/*clearImgRecInfo(networkCtx->putImageInfo, &networkCtx->imagePostStep);
							if(networkCtx->putImageInfo)
								SENS_MEM_FREE(networkCtx->putImageInfo);
							networkCtx->putImageInfo = NULL;*/
						}
						else
						{	httpCtx->failCount++;
							if(httpCtx->failCount > 3)
							{	//reboot ?? or wait 5 min to resend??
								if(networkDeInit())
									break;
								httpCtx->failCount = 0;
								msg.msgId = NETWORK_Q_MSG_NETWORK_PRE_INIT;
								msg.ptr = NULL;
								if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
								{	
								}
							}
							httpCtx->running = 0;
							SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
							imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_IDLE;
							SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
							if(imgUploadInst->uploadImgInfo)
								SENS_MEM_FREE(imgUploadInst->uploadImgInfo);
							imgUploadInst->uploadImgInfo = NULL;
						}
							
						imgUploadRecHeader = imgUploadInst->imgUploadRecHeader;
						if(!imgUploadRecHeader->numOfUnSendImage)
						{	lanSocketClose(serverInst, 1, __func__, __LINE__);
							//networkCtx->imgSending = 0;
						}
						break;
					}
			case NETWORK_Q_MSG_HTTP_POST_TIMEOUT:
					dbgMsg("%s", "http post timeout, reboot!!\r\n");
					sysCtrl->isReadyToReboot = 1;
					break;
			case NETWORK_Q_MSG_HTTP_GET:			//FOTA
					{	int rsp;
						OTA_MANAGER *otaMng = (OTA_MANAGER *)&sysCtrl->otaMng;
						HTTP_CONTEXT *httpCtx = selectOtaSvrHttpCtx();
	#if SUPPORT_AGPS
						if(checkGpsWorking(&msg))
							break;
	#endif
						if(httpCtx->running)
						{	SENS_TIME_DELAY(2000);
							msg.msgId = NETWORK_Q_MSG_HTTP_GET;
							msg.ptr = NULL;
							if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
							{	
							}
							break;
						}
						
						rsp = otaStartProcess(httpCtx);
						if(rsp)
						{	setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_OTA_ACTIVE);
							otaMng->runningAutoFota = 0;
						}
						break;
					}
			case NETWORK_Q_MSG_HTTP_GET_TIMEOUT:	//FOTA
					{	CLOUD_SERVER_INSTANCE *serverInst;
						OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
							
						serverInst = findOtaServerInst();
						otaCtx->xmitTimeoutCnt++;
						if(otaCtx->xmitTimeoutCnt > 3)
						{	dbgMsg("%s", "HTTP GET timeout, stop ota\r\n");
							stopNetworkOta(serverInst, 0);
						}
						else
						{	dbgMsg("%s", "HTTP GET timeout, Retry\r\n");
							lanSocketClose(serverInst, 1, __func__, __LINE__);
							//httpCtx->running = 0;
							triggerNextFota(serverInst);
						}
						break;
					}
			case NETWORK_Q_MSG_HTTP_GET_DONE:		//FOTA
					{	HTTP_CLI_PARSER *httpCliParserCtx = (HTTP_CLI_PARSER *)msg.ptr;
						HTTP_RSP_INFO *httpRspInfo = (HTTP_RSP_INFO *)httpCliParserCtx->httpRspInfo;
						CLOUD_SERVER_INSTANCE *serverInst;
						OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
							
						serverInst = findOtaServerInst();
						HTTP_CONTEXT *httpCtx = selectOtaSvrHttpCtx();
							
						killSensminiTimer(SENSMINI_TIMER_LAN_HTTP_GET);
						otaCtx->xmitTimeoutCnt = 0;
						if(httpRspInfo->status == 200)
						{	otaProcess(httpRspInfo, serverInst, httpCtx);
						}
						else
						{	httpCtx->running = 0;
							stopNetworkOta(serverInst, 0);
						}
						/*if(httpRspSts->content)
							SENS_MEM_FREE(httpRspSts->content);*/
						//if(httpRspSts->content)
						//	SENS_MEM_FREE(httpRspSts->content);
					}
					break;
			default:
					break;
		}
	}
}


//------------------------------------------------------------------------------
void networkRecvTask(void *param)
{	SOCKET_T sock;
	SOCKET_QUEUE *queue;
	struct TaskQMsg msg;
	SERV_RECV_CMD_CTX *cmdMsg;
	CLOUD_SERVER_INSTANCE *serverInst;
	int queueIsLocked;
#ifdef NET_LWIP
	fd_set	readSet;
	fd_set	exceptSet;
	int ret;
	int readIsSet, errorIsSet;
	#if defined OS_FREERTOS && !defined NETWORK_USE_SELECT
	serverInst = (CLOUD_SERVER_INSTANCE *)param;
	#endif
	int idx;
	uint8_t isError;
	uint32_t sockCloseArray[MAX_WIRE_LAN_SOCK];
	uint8_t sockCloseIdx=0;
	struct timeval timeout;
	uint32_t maxSockNo;
#endif
	
#if (defined PLATFORM_FSL) || ((defined NET_LWIP && defined NETWORK_USE_SELECT))
	SENS_SEM_LOCK(taskAct.networkRecvTaskAct.lock);
#endif
#ifdef OS_FREERTOS
	//SENS_TIME_DELAY(2);
#endif
	
	while(1)
	{
#ifdef PLATFORM_FSL
		taskAct.wireLanRecvTask.sts = TASK_STS_RUNNING;
		//taskAct.wireLanRecvTask.runningStartTime = GetTimeTicks();
		extendTaskTimer(&taskAct.wireLanRecvTask);
#endif
		if(networkCtx && networkCtx->workingInst)
		{	if(networkCtx->sockCnt)
			{	SENS_SEM_LOCK(networkCtx->lock);
#if defined NET_RTCS
				sock = RTCS_selectset(networkCtx->iSkts, networkCtx->sockCnt, (uint_32)-1);
				if(sock == 0 || sock == RTCS_SOCKET_ERROR) 
				{	SENS_SEM_UNLOCK(networkCtx->lock);
					SENS_TIME_DELAY(10);
					continue;
				}
				serverInst = findServerInstBySocketFd(sock);
				//queueIsLocked = 0;
				if(serverInst != NULL)
				{	//queueIsLocked = 1;
					//SENS_SEM_LOCK(serverInst->sockRecvQueueLock);
					queue = createNewSockQueue(serverInst, TCP_RECV_BUFFER_SIZE);
					//dbg_msg("sock %d current Queue Cnt:%d\r\n", sock, serverInst->sockRecvQueue->count);
					dbg_msg("sock %d current Queue Cnt:%d, img sock:%s\r\n", serverInst->fd, serverInst->sockRecvQueue->count, (serverInst->xmitProtocolType == PROTOCOL_HTTP)? "YES":"NO");
	#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE
					if(serverInst->isTlsProtocol)
					{	TCP_TLS_CTX *tlsCtx = (TCP_TLS_CTX *)serverInst->tlsCtx;
						queue->len = tcpTlsRead((mbedtls_ssl_context *)&tlsCtx->ssl, queue->buf, TCP_RECV_BUFFER_SIZE);
					}
					else
	#endif
						queue->len = recv(sock, queue->buf, TCP_RECV_BUFFER_SIZE, 0);
					dbg_msg("socket %d receive done, len:%d, img sock:%s\r\n", serverInst->fd, queue->len, (serverInst->xmitProtocolType == PROTOCOL_HTTP)? "YES":"NO");
				}
				else
				{	SENS_SEM_UNLOCK(networkCtx->lock);
					SENS_TIME_DELAY(10);
					continue;
				}
				SENS_SEM_UNLOCK(networkCtx->lock);
#elif defined NET_LWIP
	#if defined NETWORK_USE_SELECT
				FD_ZERO(&readSet);
				FD_ZERO(&exceptSet);
				maxSockNo = 0;
				for(idx=0;idx<networkCtx->sockCnt;idx++)
				{	FD_SET(networkCtx->iSkts[idx], &readSet);
					FD_SET(networkCtx->iSkts[idx], &exceptSet);
					maxSockNo = MAX(maxSockNo, networkCtx->iSkts[idx]);
				}
				timeout.tv_sec = 0;
				timeout.tv_usec = 0;	//100us
				
				sock = lwip_select(maxSockNo+1, &readSet, NULL, &exceptSet, &timeout);
				SENS_SEM_UNLOCK(networkCtx->lock);
				
				if(sock == 0)
				{	SENS_TIME_DELAY(5);
					continue;
				}
				sockCloseIdx = 0;
				for(idx=0;idx<networkCtx->sockCnt;idx++)
				{	if(FD_ISSET(networkCtx->iSkts[idx], &exceptSet))
					{	dbg_msg("socket %d exception\r\n", networkCtx->iSkts[idx]);
						sockCloseArray[sockCloseIdx++] = networkCtx->iSkts[idx];
						//sock = networkCtx->iSkts[idx];
						//serverInst = findServerInstBySocketFd(sock);
						//clearReceiveQueue(serverInst);
						//lanSocketClose(serverInst, 0, __func__, __LINE__);
					}
					else if(FD_ISSET(networkCtx->iSkts[idx], &readSet))
					{	sock = networkCtx->iSkts[idx];
						serverInst = findServerInstBySocketFd(sock);
						if(serverInst != NULL)
						{	queue = createNewSockQueue(serverInst, TCP_RECV_BUFFER_SIZE);
							dbg_msg("sock %d current Queue Cnt:%d, img sock:%s\r\n", serverInst->fd, serverInst->sockRecvQueue->count, (serverInst->xmitProtocolType == PROTOCOL_HTTP)? "YES":"NO");
	#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE
							if(serverInst->isTlsProtocol)
							{	TCP_TLS_CTX *tlsCtx = (TCP_TLS_CTX *)serverInst->tlsCtx;
								queue->len = tcpTlsRead((mbedtls_ssl_context *)&tlsCtx->ssl, queue->buf, TCP_RECV_BUFFER_SIZE);
							}
							else
	#endif
								queue->len = recv(sock, queue->buf, TCP_RECV_BUFFER_SIZE, 0);
						}
						else
						{	//SENS_TIME_DELAY(10);
							continue;
						}
	#else
				if(serverInst == NULL)
				{	/*if(queueIsLocked)
					{	SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
						queueIsLocked = 0;
					}*/
					SENS_TIME_DELAY(10);
					continue;
				}
				sock = serverInst->fd;
				queue = createNewSockQueue(serverInst, TCP_RECV_BUFFER_SIZE);
				dbg_msg("current Queue Cnt:%d, img sock:%s\r\n", serverInst->sockRecvQueue->count, (serverInst->xmitProtocolType == PROTOCOL_HTTP)? "YES":"NO");
				queue->len = recv(sock, queue->buf, TCP_RECV_BUFFER_SIZE, 0);
				SENS_SEM_UNLOCK(networkCtx->lock);
	#endif
#endif
				
						if((int)queue->len <= 0)
						{	dbg_msg("SOCK recv len error :%d\r\n", (int)queue->len);
							/*if(queueIsLocked)
							{	SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
								queueIsLocked = 0;
							}*/
							setServerConnnectFail(serverInst);
							clearReceiveQueue(serverInst);
							lanSocketClose(serverInst, 0, __func__, __LINE__);
						}
						else
						{	SOCKET_QUEUE *srcQueue;
							if(serverInst->xmitProtocolType == PROTOCOL_MQTT)
							{	dbg_msg("mqtt receive len:%d\r\n", queue->len);
								displayBufInHex(queue->buf, queue->len, __func__, __LINE__);
							}
							queue->isValid = 1;
							srcQueue = serverInst->sockRecvQueue;
							srcQueue->validDataCount++;
							//networkCtx->workingInst->noXmitTimer = GetTimeTicks();
							if(serverInst->xmitProtocolType != PROTOCOL_MQTT)
							{	cmdMsg = SENS_MEM_ZALLOC(sizeof(SERV_RECV_CMD_CTX));
								if((serverInst->xmitProtocolType != PROTOCOL_HTTP) && (serverInst->xmitProtocolType != PROTOCOL_IOA_WEB_API))
									displayBufInHex(queue->buf, queue->len, __func__, __LINE__);
								else
								{	dbg_msg("image sock recv len: %d\r\n", queue->len);
									//if((queue->len != 196) && (serverInst->isImgSocket))
										//displayBufInHex(queue->buf, queue->len, __func__, __LINE__);
								}
								cmdMsg->serverInst = serverInst;//findSockCtx(sockFd);
					
								cmdMsg->generateFromInternet = 0x81;
								msg.msgId = SERV_CMD_PROCESS_Q_MSG_RECV_CMD;
								msg.ptr = (char *)cmdMsg;
					
								if(sendMsgWithErrHandle(SERV_CMD_PROCESS_TASK, SENS_MSG_Q_SEND(servCmdProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
								{	SENS_MEM_FREE(cmdMsg);
								}
							}
						}
#if defined NET_LWIP && defined NETWORK_USE_SELECT
					}
				}
				if(sockCloseIdx)
				{	for(idx=0;idx<sockCloseIdx;idx++)
					{	sock = sockCloseArray[idx];
						serverInst = findServerInstBySocketFd(sock);
						clearReceiveQueue(serverInst);
						lanSocketClose(serverInst, 0, __func__, __LINE__);
					}
				}
#endif
					
			}
		}
		SENS_TIME_DELAY(1);
	}
}