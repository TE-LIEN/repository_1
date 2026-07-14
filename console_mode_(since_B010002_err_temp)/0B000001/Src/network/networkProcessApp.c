#include "network/netApp.h"
#include "sensCommon.h"
#include "sensDev.h"
#include "sensSystem.h"
#include "network/Protocol/serverProtocol.h"
#include "sdCardTask.h"
#include "systemTimer.h"
#include "network/networkProcessTask.h"


#if AUTO_DATA_SYNC
#include "AutoDataSync.h"
#endif

#if SUPPORT_IOA_WEB_API
#include "network/Protocol/ioaProt.h"
#endif



//------------------------------------------------------------------------------
CLOUD_SERVER_INSTANCE *findFreeServerInstance(void)
{	CLOUD_SERVER_INSTANCE *serverInst = NULL;
	int idx;
	
	for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
	{	serverInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
		if((serverInst->fd == -1) && (!serverInst->isUsing) && (!serverInst->isTempUsing))
		{	serverInst->isTempUsing = 1;
			//dbg_msg(GREEN"%s find serv inst is %d, at %p\r\n"ORG_COLOR, __func__, idx, serverInst);
			return serverInst;
		}
	}
	
	return serverInst;
}
//------------------------------------------------------------------------------
CLOUD_SERVER_INSTANCE *findServerInstByIp(char *serverIp, uint16_t port)
{	CLOUD_SERVER_INSTANCE *serverInst=NULL;
	int idx;
	
	for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
	{	serverInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
		//if((serverInst->fd != -1) || (serverInst->isUsing))
		if(serverInst->isUsing)
		{	if(serverInst->serverIp != NULL)
			{	if((!strcmp(serverInst->serverIp, serverIp)) && (port == serverInst->serverPort))
				{	return serverInst;
				}
			}
		}
	}
	return findFreeServerInstance();
}
//------------------------------------------------------------------------------
CLOUD_SERVER_INSTANCE *findServerInstBySocketFd(int socketFd)
{	CLOUD_SERVER_INSTANCE *serverInst=NULL;
	int idx;
	
	for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
	{	serverInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
		if(serverInst->fd == socketFd)
		{	dbg_msg(GREEN"%s find serv inst at %p, sock :%d\r\n"ORG_COLOR, __func__, serverInst, socketFd);
			break;
		}
	}
	return serverInst;
}
//------------------------------------------------------------------------------
CLOUD_SERVER_INSTANCE *findImgServerInst(void)
{	CLOUD_SERVER_INSTANCE *serverInst=NULL;
	int idx;
	
	for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
	{	serverInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
		if(serverInst->isImgSocket == 1)
		{	break;
		}
	}
	return serverInst;
}
//------------------------------------------------------------------------------
CLOUD_SERVER_INSTANCE *findOtaServerInst(void)
{	CLOUD_SERVER_INSTANCE *serverInst=NULL;
	int idx;
	
	for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
	{	serverInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
		if(serverInst->isOtaSocket == 1)
		{	break;
		}
	}
	return serverInst;
}
//------------------------------------------------------------------------------
CLOUD_SERVER_INSTANCE *findUsedServerInst(void)
{	CLOUD_SERVER_INSTANCE *serverInst=NULL;
	int idx;
	
	for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
	{	serverInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
		if(serverInst->isUsing == 1)
		{	break;
		}
	}
	return serverInst;
}
//------------------------------------------------------------------------------
SOCKET_QUEUE *createNewSockQueue(CLOUD_SERVER_INSTANCE *serverInst, int len)
{	SOCKET_QUEUE *queue;
	SOCKET_QUEUE *newQueue;
	SOCKET_QUEUE *srcQueue;
	
	//SENS_SEM_LOCK(serverInst->sockRecvQueueLock);
	srcQueue = serverInst->sockRecvQueue;
	queue = srcQueue;
		
	while(1)
	{	if(queue->next == NULL)
		{	break;
		}
		else
		{	queue = queue->next;
			if(!queue->isValid && (queue->len != queue->currReceiveLen) && (queue->next == NULL) && (queue->len == len))
			{	//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
				return queue;
			}
		}
	}
	newQueue = (SOCKET_QUEUE *)SENS_MEM_ZALLOC(sizeof(SOCKET_QUEUE));
	
	if(newQueue == NULL)
	{	//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
		return NULL;
	}
	//memset(newQueue, 0, sizeof(SOCKET_QUEUE));

	queue = srcQueue;
	//dbg_msg("%s, src queue at 0x%p, queue->next:0x%p\r\n", __func__, queue, queue->next);
	while(1)
	{	if(queue->next == NULL)
		{	//dbg_msg("%s, queue at 0x%p, next is null\r\n", __func__, queue);
			break;
		}
		/*else
		{	dbg_msg("%s, queue at 0x%p, next not null, find next\r\n", __func__, queue);
		}*/
		queue = queue->next;
	}
	
	newQueue->prev = queue;
	queue->next = newQueue;
	
	//dbg_msg("%s, set new prev to 0x%p, set next of prev to 0x%p\r\n", __func__, newQueue->prev, queue->next);
	newQueue->fd = serverInst->fd;
	newQueue->buf = (unsigned char *)SENS_MEM_ALLOC(len);
	if(newQueue->buf == NULL)
	{	SENS_MEM_FREE(newQueue);
		queue->next = NULL;
		//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
		return NULL;
	}
	srcQueue->count++;
	newQueue->len = len;
	//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
	return newQueue;
}
//------------------------------------------------------------------------------
void clearReceiveQueue(CLOUD_SERVER_INSTANCE *serverInst)
{	SOCKET_QUEUE *queue;
	SOCKET_QUEUE *next;
	SOCKET_QUEUE *srcQueue;
	
	//SENS_SEM_LOCK(serverInst->sockRecvQueueLock);
	srcQueue = serverInst->sockRecvQueue;
	if(srcQueue == NULL)
	{	//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
		return;
	}
	queue = srcQueue;
	while(1)
	{	if(queue->next != NULL)
		{	next = queue->next;
			removeSockQueue(serverInst, next);
		}
		if(queue->next == NULL)
			break;
		queue = queue->next;
	}
	
	srcQueue->count = 0;
	srcQueue->validDataCount = 0;
	//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
}
//------------------------------------------------------------------------------
int removeSockQueue(CLOUD_SERVER_INSTANCE *serverInst, SOCKET_QUEUE *removeQueue)
{	SOCKET_QUEUE *queue;
	SOCKET_QUEUE *next;
	SOCKET_QUEUE *srcQueue;
	
	SENS_MEM_FREE(removeQueue->buf);
	
	srcQueue = serverInst->sockRecvQueue;
   
	queue = srcQueue;

	while(1)
	{	if(queue->next == removeQueue)
		{	break;
		}
		if(queue->next == NULL)
		{	return -1;
		}
		queue = queue->next;
	}
	queue->next = removeQueue->next;
	next = removeQueue->next;
	
	if(next != NULL)
	{	next->prev = removeQueue->prev;
	}
	if(srcQueue->count)
		srcQueue->count--;
	if(srcQueue->validDataCount)
		srcQueue->validDataCount--;
	SENS_MEM_FREE(removeQueue);
	return 0;
}
//------------------------------------------------------------------------------
SOCKET_QUEUE *findOldestSockQueue(CLOUD_SERVER_INSTANCE	*serverInst)
{	SOCKET_QUEUE *queue;

	//SENS_SEM_LOCK(serverInst->sockRecvQueueLock);
	queue = serverInst->sockRecvQueue->next;
		
	if(queue != NULL)
	{	if(queue->isValid != 1)
		{	//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
			return NULL;
		}
	}
	//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
	return queue;
}
//------------------------------------------------------------------------------
int isSockPresent(uint32_t sock)
{	int idx;
	
	for(idx=0;idx<MAX_WIRE_LAN_SOCK;idx++)
	{	if(networkCtx->iSkts[idx] == sock)
		{	return idx;
		}
	}
	return -1;
}
//------------------------------------------------------------------------------
static void deleteSocket(int socket, char delTaskDirect)
{	int i, j;
#if (defined OS_FREERTOS) && (defined NET_LWIP) && !defined NETWORK_USE_SELECT
  CLOUD_SERVER_INSTANCE *serverInst;
	struct TaskQMsg msg;
#endif
	
	(void)delTaskDirect;
	if(socket < 0)
		return;
	
	i = isSockPresent(socket);
	if(i >= 0)
	{	//for (j = i; j < MAX_WIRE_LAN_SOCK-1; j++)
#if (defined OS_FREERTOS) && (defined NET_LWIP) && !defined NETWORK_USE_SELECT
		serverInst = findServerInstBySocketFd(socket);
		if(serverInst != NULL)
		{	if(delTaskDirect)
			{	vTaskDelete(serverInst->xTask);
			}
			else
			{	msg.msgId = LAN_Q_MSG_WIRE_LAN_RECV_TASK_DELETE;
				msg.ptr = (char *)serverInst;
				SENS_MSG_Q_SEND(wireLanQ, (uint_32 *)&msg, 0);
			}
		}
#endif

		for (j = i; j < networkCtx->sockCnt-1; j++)
			networkCtx->iSkts[j] = networkCtx->iSkts[j+1];
		networkCtx->iSkts[networkCtx->sockCnt-1] = EMPTY;
		networkCtx->sockCnt--;
	}
}
//------------------------------------------------------------------------------
void lanCloseSkt(uint32_t skt, char delTaskDirect)
{
#if defined NET_LWIP
	closesocket(skt);
#elif defined NET_RTCS
	shutdown(skt, 2);                                                             // 2 = FLAG_ABORT_CONNECTION
#endif
	deleteSocket(skt, delTaskDirect);
}
//------------------------------------------------------------------------------
static void addSocket(int sock)
{	int idx;
	
	if(isSockPresent(sock) >= 0)
		return;
	for(idx=0;idx<MAX_WIRE_LAN_SOCK;idx++)
	{	if(networkCtx->iSkts[idx] == EMPTY)
		{	networkCtx->iSkts[idx] = sock;
			networkCtx->sockCnt++;
			break;
		}
	}
}
//------------------------------------------------------------------------------
void clearServProtocolCtx(CLOUD_SERVER_INSTANCE *serverInst)
{	
#if SUPPORT_IOA_WEB_API
	if(serverInst->xmitProtocolType == PROTOCOL_IOA_WEB_API)
	{	IOA_SERVER_CTX *ioaServCtx;
		
		if(serverInst->protCtx)
		{	ioaServCtx = serverInst->protCtx;
			if(ioaServCtx->token)
				SENS_MEM_FREE(ioaServCtx->token);
			if(ioaServCtx->equipId)
				SENS_MEM_FREE(ioaServCtx->equipId);
			if(ioaServCtx->apiKey)
				SENS_MEM_FREE(ioaServCtx->apiKey);
			SENS_MEM_FREE(ioaServCtx);
			serverInst->protCtx = NULL;
			serverInst->ioaConnectInfo = NULL;
		}
	}
#endif
#ifdef SUPPORT_MQTT
	if(serverInst->xmitProtocolType == PROTOCOL_MQTT)
	{	//nothing to do
	}
#endif
}
//------------------------------------------------------------------------------
void clearLanSockInfo(CLOUD_SERVER_INSTANCE *serverInst)
{	//HTTP_CLI_PARSER *httpCliParserCtx;
	//HTTP_RSP_INFO *httpRspInfo;
#if AUTO_DATA_SYNC
	AUTO_DATA_SYNC_STRUCT		*autoDataSyncCtx;
	AUTO_DATA_SYNC_STRUCT		*autoDataSyncCtxNext;
#endif

	if(serverInst->servIdx == 0)
		setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_SRV1_DATA_SEND);
	else 
		setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_SRV2_DATA_SEND);
	serverInst->socketState = SOCK_BEGIN;
	serverInst->fd = -1;
	serverInst->servIdx = -1;
	
	serverInst->netSendFsm = NET_SEND_FSM_IDLE;
	serverInst->isTlsProtocol = 0;
	serverInst->serverType = -1;
	if(serverInst->servIdx < UPLOAD_SERVER_CNT)
		networkCtx->cloudActiveBmp &= ~(1 << serverInst->servIdx);
	serverInst->servIdx = -1;
	/*serverInst->readyToReceiveData = 0;
	serverInst->massDataReceving = 0;
	serverInst->readyToReceiveDataLength = 0;*/
	serverInst->isUsing = 0;
	serverInst->isTempUsing = 0;
	serverInst->isImgSocket = 0;
	serverInst->isOtaSocket = 0;
	
	if(serverInst->serverIp)
		SENS_MEM_FREE(serverInst->serverIp);
	serverInst->serverIp = NULL;	
	memset(serverInst->serverName, 0, 256);
	serverInst->tlsCtx = NULL;
	clearServProtocolCtx(serverInst);
	clearHttpCliParserCtx(serverInst);
	clearReceiveQueue(serverInst);
#ifdef USE_LWIP_RAW_TCP
	serverInst->isRawTcp = 0;
#endif
#ifdef AUTO_SEND_WITH_INFO
	SENS_SEM_LOCK(serverInst->autoSendCtxLock);
	if(serverInst->autoSendCtx)
	{	serverInst->autoSendCtx->waitForAck = 0;
	}
	SENS_SEM_UNLOCK(serverInst->autoSendCtxLock);
#endif
#if AUTO_DATA_SYNC
	SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
	
	autoDataSyncCtx = (AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx;
	while(autoDataSyncCtx)
	{	autoDataSyncCtxNext = autoDataSyncCtx->next;
		if(autoDataSyncCtx->filename)
			SENS_MEM_FREE(autoDataSyncCtx->filename);
		SENS_MEM_FREE(autoDataSyncCtx);
		autoDataSyncCtx = autoDataSyncCtxNext;
	}
	serverInst->autoDataSyncCtx = NULL;
	/*if(serverInst->autoDataSyncCtx)
	{	serverInst->autoDataSyncCtx->isReady = 0;
		serverInst->autoDataSyncCtx->waitForAck = 0;
	}*/
	SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
#endif	
}
//------------------------------------------------------------------------------
void lanSocketClose(CLOUD_SERVER_INSTANCE *serverInst, char delTaskDirect, const char *callerFunc, int callerLine)
{	
#if defined NET_LWIP && defined USE_LWIP_RAW_TCP
	RAW_TCP_CLIENT *tcpClient;
#endif
	
	dbgMsg("lan socket close call from %s(%d)\r\n", callerFunc, callerLine);
	
	SENS_SEM_LOCK(networkCtx->lock);
#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE
	if(serverInst->isTlsProtocol)
	{	if(serverInst->tlsCtx != NULL)
		{	dbgMsg(RED"server:%d, TLS Socket close!!\r\n"ORG_COLOR, serverInst->servIdx);
			mbedtlsUninit((TCP_TLS_CTX *)serverInst->tlsCtx);
			SENS_MEM_FREE(serverInst->tlsCtx);
			serverInst->tlsCtx = NULL;
		}
	}
#endif
	
#if defined NET_LWIP && defined USE_LWIP_RAW_TCP
	if(serverInst->isRawTcp)
	{	SENS_SEM_LOCK(serverInst->rawTcpClientLock);
		if(serverInst->rawTcpClient)
		{	tcpClient = serverInst->rawTcpClient;
			tcpClientConnectClose(tcpClient->tcpPcb, serverInst);
		}
		SENS_SEM_UNLOCK(serverInst->rawTcpClientLock);
	}
	else
#endif
		lanCloseSkt(serverInst->fd, delTaskDirect);
	clearLanSockInfo(serverInst);
	SENS_SEM_UNLOCK(networkCtx->lock);
}
//------------------------------------------------------------------------------
int tcpSockSend(MOBILE_WRITE_SOCKET_CMD *mobileWriteSocketCmd)
{	int count;
	int xmitLen = mobileWriteSocketCmd->bufLen;
	unsigned char *buf = mobileWriteSocketCmd->buf;
	CLOUD_SERVER_INSTANCE *serverInst = mobileWriteSocketCmd->serverInst;
#ifdef CY_PUMP
	CY_PUMP_WORK_INSTANCE	*cyPumpInst = &sensSys->cyPumpInst;
#endif

	if(serverInst->fd == -1)
		return -1;
		
	displayBufInHex(buf, xmitLen, __func__, __LINE__);
#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE
	if(serverInst->isTlsProtocol)
	{	TCP_TLS_CTX *tlsCtx = (TCP_TLS_CTX *)serverInst->tlsCtx;
		count = tcpTlsWrite((mbedtls_ssl_context *)&tlsCtx->ssl, buf, xmitLen);
	}
	else
#endif
		count = send(serverInst->fd, buf, xmitLen, 0);
	if(count != xmitLen)
	{	lanSocketClose(serverInst, 1, __func__, __LINE__);
		dbg_msg("TCP sock send %d Fail\r\n", xmitLen);
		return -1;
	}
	else
	{	dbg_msg("TCP sock send %d done\r\n", xmitLen);
		
		if(mobileWriteSocketCmd->bufType.protoType == PROTOCOL_SENSTALK_V2)
		{	if(mobileWriteSocketCmd->bufType.func == 0x00 && mobileWriteSocketCmd->bufType.dataArrayId == 0x13)
			{	if(serverInst->netSendFsm == NET_SEND_FSM_SEND_GUID)
				{	serverInst->netSendFsm = NET_SEND_FSM_SEND_GUID_DONE;
					//dbg_msg("sock send Guid done, sock:%d\r\n", sock->fd);
				}
			}
			else if(mobileWriteSocketCmd->bufType.func == 0x00 && mobileWriteSocketCmd->bufType.dataArrayId == 0x02)
			{	if(serverInst->netSendFsm == NET_SEND_FSM_SEND_PQ_TRIG)
				{	serverInst->netSendFsm = NET_SEND_FSM_SEND_PQ;
					cloudDataSendDoneProcess(serverInst);
					networkCtx->send1FrameToCloudBmp |= (1 << serverInst->servIdx);
#ifdef CY_PUMP
					if(cyPumpInst->isCIEngineMode && cyPumpInst->isFinalSend)
						cyPumpInst->isFinalSendFlag &= ~(1 << serverInst->servIdx);
#endif
					sysCtrl->sendToCloudDone = 1;
					
					if(serverInst->servIdx == 0)
						setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_SRV1_DATA_SEND);
					else 
						setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_SRV2_DATA_SEND);
					//dbg_msg("sock auto send done, sock:%d\r\n", sock->fd);
				}
			}
#if AUTO_DATA_SYNC
			else if(mobileWriteSocketCmd->bufType.func == 0x55)
			{	if(serverInst->autoDataSyncCtx && ((AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx)->isReady)
				{	dbgMsg("%s", "Auto Data Sync is done, remove this\r\n");
					removeAutoDataSyncInfo(serverInst);
				}
			}
#endif
		}
	}
	return 0;
}
//------------------------------------------------------------------------------
int tcpClientSockCreate(CLOUD_SERVER_INSTANCE *serverInst)
{	int32_t sock;
	NET_INSTANCE		*workingInst;
	MOBILE_INSTANCE		*wlInst;
	
	// 1' Establish TCP Socket
	sock = socket(PF_INET, SOCK_STREAM, 0);		// SOCK_STREAM -> TCP PROTOCOL
	if(sock == EMPTY)
	{	dbg_msg("[WARNING] TCP/IP: client socket failed, server ID: %d\r\n", serverInst->servIdx);
		return -1;
	}
	else
	{	dbg_msg("[ TCP/IP] Client socket: server ID: %d create %d OK\r\n", serverInst->servIdx, sock);
		
#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE
		if(serverInst->isTlsProtocol)
		{	TCP_TLS_CTX *tcpTlsCtx;
#ifdef NET_LWIP
			struct 
#endif
			sockaddr_in *socketAddr;
			
			socketAddr = &serverInst->socketAddr;
			if(serverInst->tlsCtx == NULL)
				serverInst->tlsCtx = SENS_MEM_ZALLOC(sizeof(TCP_TLS_CTX));
			tcpTlsCtx = (TCP_TLS_CTX *)serverInst->tlsCtx;
			
			if(strlen(serverInst->serverName) > 0)
			{	tcpTlsCtx->hostname = SENS_MEM_ZALLOC(strlen(serverInst->serverName) + 1);
				memcpy(tcpTlsCtx->hostname, serverInst->serverName, strlen(serverInst->serverName));
			}
			else
			{	tcpTlsCtx->hostname = SENS_MEM_ZALLOC(256);
				SENS_SPRINTF(tcpTlsCtx->hostname, "%d.%d.%d.%d", (socketAddr->sin_addr.s_addr >> 24)&0xFF, (socketAddr->sin_addr.s_addr >> 16)&0xFF, (socketAddr->sin_addr.s_addr >> 8)&0xFF, (socketAddr->sin_addr.s_addr & 0xFF));
			}
			if(mbedtlsInit(tcpTlsCtx) != 0)
			{	return -1;
			}
		}
#endif
		serverInst->fd = sock;
		serverInst->socketState = SOCK_CREATE_DONE;
		return 0;
	}
}
//------------------------------------------------------------------------------
int tcpClientSockConnect(CLOUD_SERVER_INSTANCE *serverInst)
{	int result;
#ifdef NET_RTCS

#if 0
	NET_INSTANCE	*workingInst;
	MOBILE_INSTANCE		*wlInst;
#endif
	uint_32 error;
	uint_32 opt_value = 10;
	// 2' Connect the TCP socket to the remote IP

	error = setsockopt(serverInst->fd, SOL_TCP, OPT_KEEPALIVE, &opt_value, sizeof(opt_value));
	if(error != RTCS_OK)
	{	dbg_msg("[WARNING] TCP/IP: Client set keep alive 10 sec, error code %lx, servId:%d, sock:%d, servInst: %p\r\n", error, serverInst->servIdx, serverInst->fd, serverInst);
	}
	opt_value = 30000;
	error = setsockopt(serverInst->fd, SOL_TCP, OPT_CONNECT_TIMEOUT, &opt_value, sizeof(opt_value));   //TIMEOUT 100SEC; default value = 480000 too long
	if(error != RTCS_OK)
	{	dbg_msg("[WARNING] TCP/IP: Client set sock timeout 100sec, error code %lx \r\n", error);
	}
	
	dbg_msg("[ TCP/IP] start to Client Connected to %d.%d.%d.%d, port %d.\r\n", IPBYTES(serverInst->socketAddr.sin_addr.s_addr), serverInst->socketAddr.sin_port);
#endif
	
#if 0
	if(networkCtx->workingInst)
	{	workingInst = networkCtx->workingInst;
		if(workingInst->netType != NETWORK_TYPE_ETH)
		{	if(workingInst->wlInst)
			{	wlInst = workingInst->wlInst;
				sockaddr_in	bindSa = {0}; 
				bindSa.sin_family = AF_INET;
				bindSa.sin_addr.s_addr = wlInst->localAddress;
				bind(serverInst->fd, (struct sockaddr*)&bindSa, sizeof(bindSa));
				dbg_msg("[ TCP/IP] Bind socket to %d.%d.%d.%d\r\n", IPBYTES(bindSa.sin_addr.s_addr));
			}
		}
		
	}
#endif
	
#ifdef NET_LWIP
	#ifdef USE_LWIP_RAW_TCP
	if(serverInst->isRawTcp)
	{	return rawTcpClientConnect(serverInst);
	}
	#endif
	
	#if LWIP_USE_NONBLOCKING
	int flags = fcntl(serverInst->fd, F_GETFL, 0);
	fcntl(serverInst->fd, F_SETFL, flags | O_NONBLOCK);
	#endif
#endif
	
	result = connect(serverInst->fd,
#if defined NET_LWIP
			(const struct sockaddr *)
#endif
			&serverInst->socketAddr, sizeof(
#if defined NET_LWIP
		struct sockaddr
#elif defined NET_RTCS
			sockaddr_in
#endif
			));
#if defined NET_LWIP && LWIP_USE_NONBLOCKING
		
	if(result < 0 && errno == EINPROGRESS)
	{	fd_set wfds;
		struct timeval tv;

		FD_ZERO(&wfds);
		FD_SET(serverInst->fd, &wfds);
		tv.tv_sec = 10;      // 10 sec timeout
		tv.tv_usec = 0;
		result = select(serverInst->fd+1, NULL, &wfds, NULL, &tv);
		if(result <= 0)
		{	//timeout or error
			//close(serverInst->fd);	//ᡃ敾Ӑclose ?̐򍊉	
		}
		else
		{	int err;
			socklen_t len = sizeof(err);
			getsockopt(serverInst->fd, SOL_SOCKET, SO_ERROR, &err, &len);
			if(err != 0)
			{	result = 0;
			}
		}
	}
#endif
		
	/*if(serverInst->servIdx < UPLOAD_SERVER_CNT)
		sysCtrl->srvConnectBmp |= (1 << serverInst->servIdx);*/
#if defined NET_LWIP && LWIP_USE_NONBLOCKING
	if(result <= 0)
#else
	if(result != RTCS_OK)
#endif
	{	
#if defined NET_LWIP
		dbg_msg("[ TCP/IP] Client Connected fail, IP:%d.%d.%d.%d, port: %d, servIdx:%d\r\n", IPBYTES(ntohl(serverInst->socketAddr.sin_addr.s_addr)), ntohs(serverInst->socketAddr.sin_port), serverInst->servIdx);
#else
		dbg_msg("[ TCP/IP] Client Connected fail, IP:%d.%d.%d.%d, port: %d, servIdx:%d\r\n", IPBYTES(serverInst->socketAddr.sin_addr.s_addr), serverInst->socketAddr.sin_port, serverInst->servIdx);
#endif
		return -1;
	}
	else
	{	if(serverInst->servIdx < UPLOAD_SERVER_CNT)
			sysCtrl->srvConnectBmp |= (1 << serverInst->servIdx);
#if defined NET_LWIP
		dbg_msg("[ TCP/IP] Client Connected to %d.%d.%d.%d, port %d.\r\n", IPBYTES(ntohl(serverInst->socketAddr.sin_addr.s_addr)), ntohs(serverInst->socketAddr.sin_port));
#elif defined NET_RTCS
		dbg_msg("[ TCP/IP] Client Connected to %d.%d.%d.%d, port %d.\r\n", IPBYTES((serverInst->socketAddr.sin_addr.s_addr)), serverInst->socketAddr.sin_port);
#endif
#if defined NET_LWIP && LWIP_USE_NONBLOCKING
		int flags = fcntl(serverInst->fd, F_GETFL, 0);
		flags &= ~O_NONBLOCK;
		fcntl(serverInst->fd, F_SETFL, flags);
		SENS_TIME_DELAY(1);
#endif
		//_time_delay(100);
#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE
		if(serverInst->isTlsProtocol)
		{	int tlsStartConnectTime = GetTimeTicks();
			if(tcpTlsConnect((TCP_TLS_CTX *)serverInst->tlsCtx, serverInst->fd) < 0)
			{	dbg_msg("%s", RED"TLS connect Fail!!\r\n"ORG_COLOR);
				return -1;
			}
			dbgMsg("Tls connect time: %d\r\n", GetTimeTicks() - tlsStartConnectTime);
		}
#endif
		SENS_SEM_LOCK(networkCtx->lock);
		addSocket(serverInst->fd);
		SENS_SEM_UNLOCK(networkCtx->lock);
		serverInst->socketState = SOCK_CONN_DONE;
		if((serverInst->servIdx >= 0) && (serverInst->servIdx < UPLOAD_SERVER_CNT))
		{	networkCtx->cloudActiveBmp |= (1 << serverInst->servIdx);
			if(serverInst->servIdx == 0)
				setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_SRV1_DATA_SEND);
			else if(serverInst->servIdx == 1)
				setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_SRV2_DATA_SEND);
		}
#if YS_MQTT_BROKER
		if((serverInst->serverType == MQTTS_YS_BROKER) || (serverInst->serverType == MQTT_YS_BROKER))
			networkCtx->ysServerConntectDone = 1;
#endif
#if (defined OS_FREERTOS) && (defined NET_LWIP) && !defined NETWORK_USE_SELECT
		xTaskCreate( networkRecvTask, "WIRE_LAN_RECV", NETWORK_RECV_TASK_STACK_SIZE, serverInst, NETWORK_RECV_TASK_PRIORITY, &serverInst->xTask );
#endif
	}
	dbg_msg("[ TCP/IP] Client socket: %d Initialize OK!\r\n", serverInst->fd);

#if TEST_N_REMOVE
	pmu_led(3);
	sensDev->ledStatus = LED_STS_CONNECTED;
#endif
  return 0;
}
//------------------------------------------------------------------------------
int networkSocketWrite(CLOUD_SERVER_INSTANCE *serverInst, unsigned char *buf, int bufLen, int timeout)
{	int count;
	//MQTTCtx *mqttCtx = (MQTTCtx *)sock->protCtx;
	
	if(serverInst->fd == -1)
		return MQTT_CODE_ERROR_NETWORK;
#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE
	if(serverInst->isTlsProtocol)
	{	TCP_TLS_CTX *tlsCtx = (TCP_TLS_CTX *)serverInst->tlsCtx;
		count = tcpTlsWrite((mbedtls_ssl_context *)&tlsCtx->ssl, buf, bufLen);
	}
	else
#endif
		count = send(serverInst->fd, buf, bufLen, 0);
	if(count != bufLen)
	{	lanSocketClose(serverInst, 1, __func__, __LINE__);
    dbg_msg("TCP sock send %d Fail\r\n", bufLen);
    return -1;
	}
	else
	{	
	}
	return bufLen;
}
//------------------------------------------------------------------------------
int networkSocketRead(CLOUD_SERVER_INSTANCE *serverInst, unsigned char *buf, int len, int timeout)
{	SOCKET_QUEUE *queue;
	//int rc;
	int rspLen = 0;
	int totalRspLen=0;
	int destOffset=0;
	long current_tick     = GetTimeTicks();
	//MQTTCtx *MqttCtx = (MQTTCtx *)sock->mqttCtx;
  if(serverInst->fd == -1)
		return MQTT_CODE_ERROR_NETWORK;
  	
	/*if(len > 2)
	{	timeout = (timeout > 5000)? timeout : 5000;
		if(len > 1024)
			timeout = (timeout > 10000)? timeout : 10000;
	}*/
	/*else
	{	timeout += 2000;
	}*/
	
	while(1)
	{	
RE_CHECK_QUEUE:
		queue = findOldestSockQueue(serverInst);
		if(queue != NULL)
		{	//dbg_msg("find Queue!!\r\n");
			if(len < (queue->len - queue->offset))
			{	memcpy(buf+destOffset, (queue->buf + queue->offset), len);
				queue->offset += len;
				totalRspLen += len;
				len = 0;
				break;
			}
			else
			{	rspLen = queue->len - queue->offset;
				memcpy(buf+destOffset, (queue->buf + queue->offset), rspLen);
				len -= rspLen;
				totalRspLen += rspLen;
				destOffset += rspLen;
				removeSockQueue(serverInst, queue);
				if(len != 0)
					goto RE_CHECK_QUEUE;
				else
				{	break;
				}	
			}
		}
		if((GetTimeTicks() - current_tick) > timeout)
		{	//netReadUnlock(MqttCtx->mobileType);
			return MQTT_CODE_ERROR_TIMEOUT;
		}
		SENS_TIME_DELAY(2);
	}

	return totalRspLen;
}

#if SUPPORT_IOA_WEB_API
//------------------------------------------------------------------------------
int ioaGetToken(CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx)
{	int error = 0;
	IOA_SERVER_CTX *ioaServCtx = serverInst->protCtx;
	HTTP_REQUEST_STRUCT *httpReqCtx;
	char *contentLengthStr;
	
	if(httpCtx->hostStr == NULL)
		httpCtx->hostStr = SENS_MEM_ZALLOC(HTTP_URL_STR_MAX_LEN);
	error = ethHttpConnect(serverInst, httpCtx);
	
	if(error)
	{	return error;
	}
	
	if(httpCtx->httpRequestCtx)
		clearRequestCtx(httpCtx);
	httpCtx->httpRequestCtx = SENS_MEM_ZALLOC(sizeof(HTTP_REQUEST_STRUCT));
	httpReqCtx = httpCtx->httpRequestCtx;
	addHttpRequestHeader(httpReqCtx, HTTP_HEADER_STR_CONTENT_TYPE, HTTP_HEADER_STR_APPLICATION_JSON);	
	httpReqCtx->payload = setTokenPayload(ioaServCtx);
	
	contentLengthStr = SENS_MEM_ZALLOC(100);
	SENS_SPRINTF(contentLengthStr, "%d", strlen(httpReqCtx->payload));
	addHttpRequestHeader(httpCtx->httpRequestCtx, HTTP_HEADER_STR_CONTENT_LENGTH, contentLengthStr);
	SENS_MEM_FREE(contentLengthStr);
	
	error = httpSendRequest(serverInst, httpCtx);
	//if(httpReqCtx->payload)
	//	SENS_MEM_FREE(httpReqCtx->payload);
	return error;
}
//------------------------------------------------------------------------------
int ioaSendData(CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx)
{	int error = 0;
	//IOA_SERVER_CTX *ioaServCtx = serverInst->protCtx;
	HTTP_REQUEST_STRUCT *httpReqCtx;
	char *contentLengthStr;
	
	if(httpCtx->hostStr == NULL)
		httpCtx->hostStr = SENS_MEM_ZALLOC(HTTP_URL_STR_MAX_LEN);
	error = ethHttpConnect(serverInst, httpCtx);
	
	if(httpCtx->httpRequestCtx)
		clearRequestCtx(httpCtx);
	httpCtx->httpRequestCtx = SENS_MEM_ZALLOC(sizeof(HTTP_REQUEST_STRUCT));
	httpReqCtx = httpCtx->httpRequestCtx;
	
	addHttpRequestHeader(httpReqCtx, HTTP_HEADER_STR_CONTENT_TYPE, HTTP_HEADER_STR_APPLICATION_JSON);
	httpReqCtx->payload = setSendDataPayload(serverInst);
	
	contentLengthStr = SENS_MEM_ZALLOC(100);
	SENS_SPRINTF(contentLengthStr, "%d", strlen(httpReqCtx->payload));
	addHttpRequestHeader(httpCtx->httpRequestCtx, HTTP_HEADER_STR_CONTENT_LENGTH, contentLengthStr);
	SENS_MEM_FREE(contentLengthStr);
	
	error = httpSendRequest(serverInst, httpCtx);
	if(!error)
	{	resetServIdleXmitTimer(1 << serverInst->servIdx);
		networkCtx->send1FrameToCloudBmp |= 1 << serverInst->servIdx;
	}
	//if(httpReqCtx->payload)
	//	SENS_MEM_FREE(httpReqCtx->payload);
	return error;
}
#endif
//------------------------------------------------------------------------------
HTTP_CONTEXT *selectOtaSvrHttpCtx(void)
{	HTTP_CONTEXT *httpCtx;
	
	if(sysCfg->sensSysCfg.imgServerType == TIME_LAPSE_UPLOAD_SERVER_TYPE_INDONESIA)
		httpCtx = networkCtx->idImgHttpCtx;
	else
		httpCtx = networkCtx->twHttpCtx;
	
	
	return httpCtx;
}
//------------------------------------------------------------------------------
void setOtaUrl(char *url, enum LTE_MDVPN_TYPE mdvpnType, uint8_t isFota)
{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	OTA_CONFIG *otaCfg;
	
	char *pos;
	//char *portPos;
	
	if(mdvpnType == LTE_MDVPN_TYPE_NONE)
	{	if(sysCfg->sensSysCfg.imgServerType == TIME_LAPSE_UPLOAD_SERVER_TYPE_INDONESIA && !isFota)
			strcat(url, DEVICE_SENSLINK_ID_DOMAIN_NAME);
		else
			strcat(url, DEVICE_SENSLINK_NET_DOMAIN_NAME);
		//strcat(url, DEVICE_SENSLINK_NET_DOMAIN_NAME);
	}
	else if(mdvpnType == LTE_MDVPN_TYPE_SENSLINK_MDVPN)
	{	strcat(url, MDVPN_DEVICE_SENSLINK_NET_IP);
	}
	else
	{	
		if(netCfg->otaCfg)
		{	//example https://device.senslink.net:443, http://device.senslink.net:80
			otaCfg = netCfg->otaCfg;
			if(otaCfg->domainname)
			{	strcat(url, otaCfg->domainname);
				pos = strstr(url, "://");
				if(pos) pos += 3;
				else	pos = url;
				if(strstr(pos, ":") == NULL)
				{	if(otaCfg->port > 0)
					{	SENS_SPRINTF(url + strlen(url), ":%d", otaCfg->port);
					}
				}
			}
			
		}
	}
}
//------------------------------------------------------------------------------
int timeLapseImagePost(CLOUD_SERVER_INSTANCE *serverInst, IMAGE_UPLOAD_INFO *imgUploadInfo)
{	HTTP_CONTEXT *httpCtx;
	int count;
	//int xmitLen;
	int sendFileSize;
	int error = 0;
	int fileOffset;
	int readLen;
	int findJpgFile=0;
	char *payload = NULL;
	IMAGE_UPLOAD_INSTANCE *imgUploadInst = networkCtx->imgUploadInst;
	NET_INSTANCE *workingInst = networkCtx->workingInst;
	int maxXmitFrame;
	
	if(workingInst->netType == NETWORK_TYPE_ETH)
		maxXmitFrame = WIRED_MAX_FRAME;
	else
		maxXmitFrame = MOBILE_MAX_FRAME;
	
	if(imgUploadInst->uploadImgInfo)
		SENS_MEM_FREE(imgUploadInst->uploadImgInfo);
	imgUploadInst->uploadImgInfo = SENS_MEM_ALLOC(sizeof(IMAGE_UPLOAD_INFO));
	if(imgUploadInfo == NULL)
	{	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
		imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_IDLE;
		SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
		return 0;
	}
	memcpy(imgUploadInst->uploadImgInfo, imgUploadInfo, sizeof(IMAGE_UPLOAD_INFO));

	imgUploadInfo = imgUploadInst->uploadImgInfo;
	
	httpCtx = selectOtaSvrHttpCtx();
	httpCtx->hostStr = SENS_MEM_ZALLOC(128);
	error = ethHttpConnect(serverInst, httpCtx);
		
	//prepare post data;
	dbgMsg("find Image File: %s\r\n", imgUploadInfo->fileName);
	if(chkFileExist(imgUploadInfo->fileName))
	{	imgUploadInfo->fileLength = getFileLength(imgUploadInfo->fileName);
		findJpgFile = 1;
		dbg_msg("[SD CARD] Image ready--%s, file length:%d\r\n", imgUploadInfo->fileName, imgUploadInfo->fileLength);
	}
	
	if(!findJpgFile)
	{	dbgMsg("can not find Image File: %s\r\n", imgUploadInfo->fileName);
		error = ERR_NO_IMAGE_FILE;
		goto IMAGE_END;
	}
	
	//cmuxDebug = 1;
	SENS_SPRINTF(httpCtx->path, "%s/images/%s?unixtime=%d%s", SENSMINI_HTTP_SERVER_PATH, sensSys->guid.guidString, (uint32_t)imgUploadInfo->unixTime, EMPTY_CHAR);
	if(httpCtx->httpRequestCtx)
		clearRequestCtx(httpCtx);
	httpCtx->httpRequestCtx = SENS_MEM_ZALLOC(sizeof(HTTP_REQUEST_STRUCT));
	addHttpRequestHeader(httpCtx->httpRequestCtx, HTTP_HEADER_STR_CONTENT_TYPE, HTTP_HEADER_STR_APPLICATION_OCTET_STREAM);
	
	payload = SENS_MEM_ZALLOC(100);
	SENS_SPRINTF(payload, "%d", imgUploadInfo->fileLength);
	addHttpRequestHeader(httpCtx->httpRequestCtx, HTTP_HEADER_STR_CONTENT_LENGTH, payload);
	SENS_MEM_FREE(payload);
	payload = NULL;
	error = httpSendRequest(serverInst, httpCtx);
	if(error)
	{	goto IMAGE_END;
	}
	
	dbg_msg("%s", "send request header done!!\r\n");
	payload = SENS_MEM_ZALLOC(maxXmitFrame);
	for(fileOffset=0;fileOffset<imgUploadInfo->fileLength;)
	{	sendFileSize = maxXmitFrame;
		if((sendFileSize + fileOffset) > imgUploadInfo->fileLength)
		{	sendFileSize = imgUploadInfo->fileLength - fileOffset;
		}
		readLen = sdReadFile(imgUploadInfo->fileName, payload, sendFileSize, fileOffset, NORMAL_READ_MODE);
		if(readLen < 0)
		{	error = -6;
			dbg_msg("read Image error, length:%d\r\n", readLen);
			goto IMAGE_END;
		}
		//dbg_msg("write file from:0x%X, size:%d to module\r\n", fileOffset, sendFileSize);
#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE
		if(serverInst->isTlsProtocol)
		{	TCP_TLS_CTX *tlsCtx = (TCP_TLS_CTX *)serverInst->tlsCtx;
			count = tcpTlsWrite((mbedtls_ssl_context *)&tlsCtx->ssl, payload, sendFileSize);
		}
		else
#endif
		{
#if defined NET_RTCS
			count = send(serverInst->fd, payload, sendFileSize, 0);
#elif defined NET_LWIP
		//count = send(serverInst->fd, payload, sendFileSize, MSG_DONTWAIT);
			count = send(serverInst->fd, payload, sendFileSize, 0);
#endif
		}
		if(count != sendFileSize)
		{	error = -1;
			dbg_msg("send Image error, count:%d\r\n", count);
			goto IMAGE_END;
		}
#ifdef OS_MQX
		//SENS_TIME_DELAY(2000);
		SENS_TIME_DELAY(1);
#endif
#if EN_WATCHDOG
		watchDogClr();
#endif
		extendTaskTimer((TASK_INFO *)&taskAct.networkProcessTaskAct);
		fileOffset += sendFileSize;
	}
	dbg_msg("%s", "send Image done!!\r\n");

	//cmuxDebug = 0;
	httpCtx->running = 1;
	
	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_WAIT_RSP;
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
	setSensminiTimer((void *)networkProcessQ, NETWORK_Q_MSG_HTTP_POST_TIMEOUT, NULL, SENSMINI_TIMER_LAN_HTTP_POST, HTTP_POST_TIMEOUT, TIMER_MODE_TRIGGER);
	
IMAGE_END:
	if(payload)
		SENS_MEM_FREE(payload);
	if(error)
	{	lanSocketClose(serverInst, 1, __func__, __LINE__);
	}
	return error;
}
//------------------------------------------------------------------------------
void timeLapseImageSend(IMAGE_UPLOAD_INFO *imgUploadInf)
{	struct TaskQMsg msg;
	CLOUD_SERVER_INSTANCE *serverInst;
	NET_INSTANCE *workingInst = networkCtx->workingInst;
	HTTP_CONTEXT *httpCtx = selectOtaSvrHttpCtx();
	int rsp;
	IMAGE_UPLOAD_INSTANCE *imgUploadInst = networkCtx->imgUploadInst;
	
	networkCtx->agpsPause = 1;
	
	serverInst = findImgServerInst();
	if(serverInst == NULL)
		serverInst = findFreeServerInstance();
	serverInst->isImgSocket = 1;
	serverInst->isUsing = 1;
	serverInst->isTempUsing = 0;
	dbg_msg("img server inst at %p\r\n", serverInst);
							
							
	serverInst->xmitProtocolType = PROTOCOL_HTTP;
	if(httpCtx->url == NULL)
		httpCtx->url = SENS_MEM_ZALLOC(HTTP_URL_STR_MAX_LEN);
	else
		memset(httpCtx->url, 0, HTTP_URL_STR_MAX_LEN);
	if(httpCtx->path == NULL)
		httpCtx->path = SENS_MEM_ZALLOC(HTTP_PATH_STR_MAX_LEN);
	else
		memset(httpCtx->path, 0, HTTP_PATH_STR_MAX_LEN);
							
	setOtaUrl(httpCtx->url, workingInst->netMdvpnType, 0);
	memset(serverInst->serverName, 0, 256);
	if(sysCfg->sensSysCfg.imgServerType == TIME_LAPSE_UPLOAD_SERVER_TYPE_INDONESIA)
		strcat(serverInst->serverName, "device.senslink.id");
	else
		strcat(serverInst->serverName, "device.senslink.net");
	httpCtx->httpCmd = HTTP_POST;
	rsp = timeLapseImagePost(serverInst, imgUploadInf);
	if(rsp == ERR_NO_IMAGE_FILE)
	{	clearImgRecInfo(imgUploadInst->uploadImgInfo);
		SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
		imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_IDLE;
		SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
	}
	else if(rsp)
	{	httpCtx->failCount++;
		if(httpCtx->failCount > 3)
		{	//reboot ?? or wait 5 min to resend??
			if(networkDeInit())
				return ;
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
	if(imgUploadInf)
		SENS_MEM_FREE(imgUploadInf);
}
//------------------------------------------------------------------------------