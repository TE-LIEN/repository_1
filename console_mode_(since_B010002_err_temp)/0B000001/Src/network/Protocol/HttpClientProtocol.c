#include "network/protocol/serverProtocol.h"
#include "network/netApp.h"
#include "sensCommon.h"
#include "rtcDateTime.h"

//http client send
//------------------------------------------------------------------------------
void clearHttpCliParserCtx(CLOUD_SERVER_INSTANCE *serverInst)
{	HTTP_CLI_PARSER *httpCliParserCtx;
	HTTP_RSP_INFO *httpRspInfo;
	
	if(serverInst->httpCliParserCtx)
	{	httpCliParserCtx = serverInst->httpCliParserCtx;
		if(httpCliParserCtx->httpRspInfo)
		{	httpRspInfo = httpCliParserCtx->httpRspInfo;
			if(httpRspInfo->contentType)
				SENS_MEM_FREE(httpRspInfo->contentType);
			if(httpRspInfo->content)
				SENS_MEM_FREE(httpRspInfo->content);
			SENS_MEM_FREE(httpCliParserCtx->httpRspInfo);
		}
		SENS_MEM_FREE(serverInst->httpCliParserCtx);
		serverInst->httpCliParserCtx = NULL;
	}
}
//------------------------------------------------------------------------------
int ethHttpConnect(CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx)
{	
#ifdef NET_LWIP
	struct
#endif
	sockaddr_in *socketAddr;
	uint8_t	ipV4[4];
#ifdef NET_LWIP
	char ipV4Str[16];
#endif
	int error = 0;
	char *pos;
	char *portPos;
	uint16_t port;
	char *httpPos;
	
#if SUPPORT_IOA_WEB_API
	if(serverInst->xmitProtocolType != PROTOCOL_IOA_WEB_API)
#endif
		serverInst->xmitProtocolType = PROTOCOL_HTTP;
		
	serverInst->isTlsProtocol = 0;
	pos = strstr(httpCtx->url, "://");
	if(pos)
	{	httpPos = strstr(httpCtx->url, "https");
		if(httpPos)
			serverInst->isTlsProtocol = 1;
		pos += 3;
	}
	else
		pos = httpCtx->url;
	
	portPos = strstr(pos, ":");
	if(portPos)
	{	SENS_SSCANF(portPos, ":%d", &port);
		*portPos = '\0';
		if(port == 443 && serverInst->isTlsProtocol == 0)
			serverInst->isTlsProtocol = 1;
	}
	else
	{
#if SUPPORT_IOA_WEB_API
		if(serverInst->xmitProtocolType == PROTOCOL_IOA_WEB_API)
		{	if(serverInst->serverPort)
			{	port = serverInst->serverPort;
				if(port == 443)
					serverInst->isTlsProtocol = 1;
				else
					serverInst->isTlsProtocol = 0;
			}
			else
			{	if(serverInst->isTlsProtocol)
					port = 443;
				else
					port = 80;
			}
		}
		else
#endif
		{	if(serverInst->isTlsProtocol)
				port = 443;
			else
				port = 80;
		}
	}
		
	memset(httpCtx->hostStr, 0, HTTP_URL_STR_MAX_LEN);
	strcpy(httpCtx->hostStr, pos);
	
	if(serverInst->socketState == SOCK_CONN_DONE)
	{	dbg_msg("%s", "http server still connected\r\n");
		return 0;
	}
	
	socketAddr = &serverInst->socketAddr;
	
	memset(socketAddr, 0x00, sizeof(
#if defined NET_LWIP
		struct
#endif
			sockaddr_in));
	socketAddr->sin_addr.s_addr = inet_addr(httpCtx->hostStr);
	if(socketAddr->sin_addr.s_addr == 
#if defined NET_RTCS
												INADDR_BROADCAST
#elif defined NET_LWIP
												IPADDR_NONE
#endif
		)
	{	dnsQuery(httpCtx->hostStr, ipV4);
#if defined NET_RTCS
		socketAddr->sin_addr.s_addr = IPADDR(ipV4[0], ipV4[1], ipV4[2], ipV4[3]);		
#elif defined NET_LWIP
		SENS_SPRINTF(ipV4Str, "%d.%d.%d.%d", ipV4[0], ipV4[1], ipV4[2], ipV4[3]);
		socketAddr->sin_addr.s_addr = inet_addr(ipV4Str);
#endif
	}
			
#if defined NET_RTCS
	//socketAddr->sin_addr.s_addr = IPADDR(ipV4[0], ipV4[1], ipV4[2], ipV4[3]);
	socketAddr->sin_port = port;
#elif defined NET_LWIP
	//socketAddr->sin_addr.s_addr = inet_addr(ipV4Str);
	socketAddr->sin_port = htons(port);
#endif


	SENS_SPRINTF(httpCtx->hostStr+strlen(httpCtx->hostStr), ":%d", port);

	socketAddr->sin_family = AF_INET;
#if defined NET_LWIP
	memset(socketAddr->sin_zero, 0, sizeof(socketAddr->sin_zero));
#endif
	
  tcpClientSockCreate(serverInst);
  
	if(tcpClientSockConnect(serverInst))
	{	error = -1;
		dbg_msg("%s", "tcp socket connect fail!!\r\n");
		return error;
	}
	return error;
}
//------------------------------------------------------------------------------
static char *addHeaderEnd(char *headerStr, int *maxAllocSize)
{	int currLength = strlen(headerStr);
	int newLength =  currLength + 2;
	if(newLength > *maxAllocSize)
	{	headerStr = SENS_MEM_REALLOC(headerStr, newLength);
		memset(&headerStr[currLength], 0, newLength - currLength);
		*maxAllocSize = newLength;
	}
	strcat(headerStr, "\r\n");
	return headerStr;
}
//------------------------------------------------------------------------------
static char *addHeaders(char *headerStr, int *maxAllocSize, HTTP_REQUEST_STRUCT *requestCtx)
{	HTTP_REQUEST_HEADER *headerCtx;
	int nameLen, vlaueLen;
	int currLen;
	int newLength;
	HTTP_REQUEST_HEADER *headers;
	
	if(requestCtx)
	{	headers = requestCtx->headers;
		headerCtx = headers;
		while(headerCtx)
		{	nameLen = strlen(headerCtx->name);
			vlaueLen = strlen(headerCtx->value);
		
			currLen = strlen(headerStr);
			if((currLen + nameLen + vlaueLen + 5) > *maxAllocSize)
			{	newLength = currLen + nameLen + vlaueLen + 5;
				headerStr = SENS_MEM_REALLOC(headerStr, newLength);
				memset(&headerStr[currLen], 0, (newLength - currLen));
				*maxAllocSize = newLength;
			}
			SENS_SPRINTF(headerStr+currLen, "%s: %s\r\n", headerCtx->name, headerCtx->value);
			headerCtx = headerCtx->next;
		}
	}
	
	headerStr = addHeaderEnd(headerStr, maxAllocSize);
	return headerStr;
}
//------------------------------------------------------------------------------
static char *addPayload(char *headerStr, int *maxAllocSize, HTTP_REQUEST_STRUCT *requestCtx)
{	int payloadLen;
	int currLength;
	int newLength;
	char *payload;
	
	if(requestCtx == NULL)
		return headerStr;
	if(requestCtx->payload == NULL)
		return headerStr;
	payload = requestCtx->payload;
	
	payloadLen = strlen(payload);
	currLength = strlen(headerStr);
	if((payloadLen + currLength) > *maxAllocSize)
	{	newLength = payloadLen + currLength + 1;
		headerStr = SENS_MEM_REALLOC(headerStr, newLength);
		memset(&headerStr[currLength], 0, (newLength - currLength));
		*maxAllocSize = newLength;
	}
	strcat(headerStr, payload);
	return headerStr;
}
//------------------------------------------------------------------------------
void addHttpRequestHeader(HTTP_REQUEST_STRUCT *requstCtx, char *name, char *value)
{	HTTP_REQUEST_HEADER *headerTemp;
	HTTP_REQUEST_HEADER *prev = NULL;
	
	if(requstCtx->headers == NULL)
	{	requstCtx->headers = SENS_MEM_ZALLOC(sizeof(HTTP_REQUEST_HEADER));
		headerTemp = requstCtx->headers;
	}
	else
	{	prev = requstCtx->headers;
		while(prev->next)
		{	prev = prev->next;
		}
		headerTemp = SENS_MEM_ZALLOC(sizeof(HTTP_REQUEST_HEADER));
		prev->next = headerTemp;
	}
	
	headerTemp->name = SENS_MEM_ZALLOC(strlen(name)+1);
	headerTemp->value = SENS_MEM_ZALLOC(strlen(value)+1);
	memcpy(headerTemp->name, name, strlen(name));
	memcpy(headerTemp->value, value, strlen(value));
}
//------------------------------------------------------------------------------
void clearRequestCtx(HTTP_CONTEXT *httpCtx)
{	HTTP_REQUEST_STRUCT *requestCtx;
	HTTP_REQUEST_HEADER *headers;
	HTTP_REQUEST_HEADER *next;
	
	if(httpCtx == NULL)
		return;
	if(httpCtx->httpRequestCtx == NULL)
		return;
	requestCtx = httpCtx->httpRequestCtx;
	headers = requestCtx->headers;
	while(headers)
	{	next = headers->next;
		if(headers->name)
			SENS_MEM_FREE(headers->name);
		if(headers->value)
			SENS_MEM_FREE(headers->value);
		headers = next;
	}
	if(requestCtx->payload)
		SENS_MEM_FREE(requestCtx->payload);
	SENS_MEM_FREE(httpCtx->httpRequestCtx);
	httpCtx->httpRequestCtx = NULL;
}
//------------------------------------------------------------------------------
int httpSendRequest(CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx)
{	char *requestStr;
	int xmitLen;
	int count;
	int error = 0;
	HTTP_REQUEST_STRUCT *requestCtx;
	int allocSize = 4096;
	
	clearHttpCliParserCtx(serverInst);
	requestStr = SENS_MEM_ZALLOC(allocSize);
	dbg_msg("http path:%s\r\n", httpCtx->path);
	
	requestCtx = httpCtx->httpRequestCtx;
	
	if(httpCtx->httpCmd == HTTP_POST)
		strcat(requestStr, "POST ");
	else if(httpCtx->httpCmd == HTTP_GET)
		strcat(requestStr, "GET ");
	else if(httpCtx->httpCmd == HTTP_PUT)
		strcat(requestStr, "PUT ");
	else if(httpCtx->httpCmd == HTTP_DELETE)
		strcat(requestStr, "DELETE ");
	
	SENS_SPRINTF(requestStr+strlen(requestStr), HTTP_DEFAULT_HEADER, httpCtx->path, httpCtx->hostStr);
	
	requestStr = addHeaders(requestStr, &allocSize, requestCtx);
	requestStr = addPayload(requestStr, &allocSize, requestCtx);
	
	/*dbgMsg("%s", "http send Data\r\n");
	displayBufInHex((uint8_t *)requestStr, strlen(requestStr), __func__, __LINE__);*/
	
	xmitLen = strlen(requestStr);

	if(serverInst->isTlsProtocol)
	{	TCP_TLS_CTX *tlsCtx = (TCP_TLS_CTX *)serverInst->tlsCtx;
		count = tcpTlsWrite((mbedtls_ssl_context *)&tlsCtx->ssl, requestStr, xmitLen);
	}
	else
	{
#if defined NET_RTCS
		count = send(serverInst->fd, requestStr, xmitLen, 0);
#elif defined NET_LWIP
	//count = send(serverInst->fd, postStr, xmitLen, MSG_DONTWAIT);
		count = send(serverInst->fd, requestStr, xmitLen, 0);
#endif
	}
	//displayBufInHex((uint8_t *)requestStr, count, __func__, __LINE__);
	
	if(count != xmitLen)
	{	dbg_msg("tcp Tls socket error for OTA send %d\r\n", count);
		error = -1;
	}
	else
    dbg_msg("tcp Tls socket Done for OTA send %d\r\n", count);
	SENS_MEM_FREE(requestStr);
	return error;
}

//Http client parser
//------------------------------------------------------------------------------
static void combineSkbBuf(SOCKET_QUEUE *skbQueue, CLOUD_SERVER_INSTANCE *serverInst)
{	SOCKET_QUEUE *skbQueueTemp;
	SOCKET_QUEUE *skbQueueNext;
	
	//SENS_SEM_LOCK(serverInst->sockRecvQueueLock);
	skbQueueTemp = skbQueue->next;
	while(skbQueueTemp)
	{	skbQueueNext = skbQueueTemp->next;
		if(skbQueueTemp->isValid)
		{	skbQueue->buf =  SENS_MEM_REALLOC(skbQueue->buf, skbQueue->len + skbQueueTemp->len);
			memcpy(&skbQueue->buf[skbQueue->len], skbQueueTemp->buf, skbQueueTemp->len);
			skbQueue->len += skbQueueTemp->len;
			removeSockQueue(serverInst, skbQueueTemp);
		}
		skbQueueTemp = skbQueueNext;
	}
	//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
}
//------------------------------------------------------------------------------
static int parserHttpHeader(SOCKET_QUEUE *skbQueue, HTTP_CLI_PARSER *httpCliParserCtx)
{	HTTP_RSP_INFO *httpRspInfo = httpCliParserCtx->httpRspInfo;
	char *cmdBuf = NULL;
	char *pos;
	char *dataPos;
	char *sepPos;
	int processLen;
	char *chkStrPos;
	
	combineSkbBuf(skbQueue, httpCliParserCtx->servInst);
	
	cmdBuf = (char *)(skbQueue->buf + httpCliParserCtx->prevParseOffset);
	pos = cmdBuf;
		
	dataPos = strstr(pos, "\r\n\r\n");
	while(1)
	{	sepPos = strstr(pos, "\r\n");
		if(sepPos == NULL)
			break;
		processLen = sepPos - pos;
		httpCliParserCtx->prevParseOffset += (processLen+2);
		*sepPos = 0x00;
		
		//check status code
		chkStrPos = strstr(pos, "HTTP/");
		if(chkStrPos)
		{	chkStrPos = strstr(chkStrPos, "200 OK");
			if(chkStrPos)
				httpRspInfo->status = 200;
			else
			{	char *tempPos;
				chkStrPos = strstr(pos, " ");
				if(chkStrPos)
				{	chkStrPos++;
					tempPos = strstr(chkStrPos, " ");
					if(tempPos)
						*tempPos = 0x00;
					SENS_SSCANF(chkStrPos, "%d", &httpRspInfo->status);
				}
			}
			goto GET_NEXT;
		}
		
		//check Date
		chkStrPos = strstr(pos, "Date: ");
		if(chkStrPos)
		{	char dayName[4], monthName[4];
			int day, year, hour, minute, second;
			char *gmtStr;
				
			gmtStr = strstr(chkStrPos, "GMT");
			if(gmtStr)
			{	memset(dayName, 0, 4);
				memset(monthName, 0, 4);
				SENS_SEM_LOCK(networkCtx->timeZoneLock);
				SENS_SSCANF(chkStrPos, "Date: %s, %d %s %d %d:%d:%d GMT", dayName, &day, monthName, &year, &hour, &minute, &second);
#if defined NUVOTON
				networkCtx->timezone.u32Year = year;
				if((monthName[0] == 'J') && (monthName[1] == 'a'))			networkCtx->timezone.u32Month = 1;
				else if(monthName[0] == 'F')								networkCtx->timezone.u32Month = 2;
				else if((monthName[0] == 'M') && (monthName[2] == 'r'))		networkCtx->timezone.u32Month = 3;
				else if((monthName[0] == 'A') && (monthName[1] == 'p'))		networkCtx->timezone.u32Month = 4;
				else if((monthName[0] == 'M') && (monthName[2] == 'y'))		networkCtx->timezone.u32Month = 5;
				else if((monthName[0] == 'J') && (monthName[2] == 'n'))		networkCtx->timezone.u32Month = 6;
				else if((monthName[0] == 'J') && (monthName[2] == 'l'))		networkCtx->timezone.u32Month = 7;
				else if((monthName[0] == 'A') && (monthName[1] == 'u'))		networkCtx->timezone.u32Month = 8;
				else if(monthName[0] == 'S')								networkCtx->timezone.u32Month = 9;
				else if(monthName[0] == 'O')								networkCtx->timezone.u32Month = 10;
				else if(monthName[0] == 'N')								networkCtx->timezone.u32Month = 11;
				else if(monthName[0] == 'D')								networkCtx->timezone.u32Month = 12;
				networkCtx->timezone.u32Day = day;
				networkCtx->timezone.u32Hour = hour;
				networkCtx->timezone.u32Minute = minute;
				networkCtx->timezone.u32Second = second;
				dbgMsg("https %s\r\n", chkStrPos);
				dbg_msg("get DT: %4d/%02d/%02d %02d:%02d:%02d\r\n", networkCtx->timezone.u32Year, networkCtx->timezone.u32Month, networkCtx->timezone.u32Day, networkCtx->timezone.u32Hour, networkCtx->timezone.u32Minute, networkCtx->timezone.u32Second);
				setTime((S_RTC_TIME_DATA_T *)&networkCtx->timezone);
#else
				networkCtx->timezone[0] = year;
				if((monthName[0] == 'J') && (monthName[1] == 'a'))			networkCtx->timezone[1] = 1;
				else if(monthName[0] == 'F')								networkCtx->timezone[1] = 2;
				else if((monthName[0] == 'M') && (monthName[2] == 'r'))		networkCtx->timezone[1] = 3;
				else if((monthName[0] == 'A') && (monthName[1] == 'p'))		networkCtx->timezone[1] = 4;
				else if((monthName[0] == 'M') && (monthName[2] == 'y'))		networkCtx->timezone[1] = 5;
				else if((monthName[0] == 'J') && (monthName[2] == 'n'))		networkCtx->timezone[1] = 6;
				else if((monthName[0] == 'J') && (monthName[2] == 'l'))		networkCtx->timezone[1] = 7;
				else if((monthName[0] == 'A') && (monthName[1] == 'u'))		networkCtx->timezone[1] = 8;
				else if(monthName[0] == 'S')								networkCtx->timezone[1] = 9;
				else if(monthName[0] == 'O')								networkCtx->timezone[1] = 10;
				else if(monthName[0] == 'N')								networkCtx->timezone[1] = 11;
				else if(monthName[0] == 'D')								networkCtx->timezone[1] = 12;
				networkCtx->timezone[2] = day;
				networkCtx->timezone[3] = hour;
				networkCtx->timezone[4] = minute;
				networkCtx->timezone[5] = second;
				dbg_msg("https %s\r\n", chkStrPos);
				dbg_msg("get DT: %4d/%02d/%02d %02d:%02d:%02d\r\n", networkCtx->timezone[0], networkCtx->timezone[1], networkCtx->timezone[2], networkCtx->timezone[3], networkCtx->timezone[4], networkCtx->timezone[5]);
				set_time(networkCtx->timezone);
#endif
				
//#ifdef HTTP_SET_TIME
				
//#endif
				SENS_SEM_UNLOCK(networkCtx->timeZoneLock);
			}
			goto GET_NEXT;
		}
		chkStrPos = strstr(pos, "Content-Type: ");
		if(chkStrPos)
		{	int contentTypeLength = ((int)sepPos - (int)chkStrPos) - strlen("Content-Type: ");
				
			httpRspInfo->contentType = SENS_MEM_ZALLOC(contentTypeLength+1);
			SENS_SSCANF(chkStrPos + strlen("Content-Type: "), "%s", httpRspInfo->contentType);
			
			if(!strncmp(httpRspInfo->contentType, "application/octet-stream", strlen("application/octet-stream")))
				httpRspInfo->isOctetStreamData = 1;
			else if(!strncmp(httpRspInfo->contentType, "application/json", strlen("application/json")))
				httpRspInfo->isOctetStreamData = 0;
			
			goto GET_NEXT;
		}
		chkStrPos = strstr(pos, "Transfer-Encoding: chunked");
		if(chkStrPos)
		{	httpCliParserCtx->isChunked = 1;
			goto GET_NEXT;
		}
		chkStrPos = strstr(pos, "Content-Length: ");
		if(chkStrPos)
			SENS_SSCANF(chkStrPos + strlen("Content-Length: "), "%d", &httpRspInfo->fileSize);
		
GET_NEXT:
		if(sepPos == dataPos)
		{	httpCliParserCtx->fsm = HTTP_CLIENT_RSP_FSM_PAYLOAD_PARSER;
			httpCliParserCtx->prevParseOffset += 2;
			break;
		}
		pos = sepPos+2;
	}
	
	return HTTP_CLI_RSP_PARSER_RESULT_INCOMPLETE_DATA;
}
//------------------------------------------------------------------------------
static int parserHttpPayload(SOCKET_QUEUE *skbQueue, HTTP_CLI_PARSER *httpCliParserCtx)
{	char *cmdBuf;
	char *pos;
	char *sepPos;
	//char *dataPos;
	int chunkDataLength;
	int chunkLenOffset;
	HTTP_RSP_INFO *httpRspInfo = httpCliParserCtx->httpRspInfo;
	
	combineSkbBuf(skbQueue, httpCliParserCtx->servInst);
	cmdBuf = (char *)(skbQueue->buf + httpCliParserCtx->prevParseOffset);
	pos = cmdBuf;
	
	//dataPos = strstr(pos, "\r\n\r\n");
	while(1)
	{	if(httpCliParserCtx->isChunked)
		{	sepPos = strstr(pos, "\r\n");
			if(httpCliParserCtx->noMoreChunkData && (sepPos == pos))
				return HTTP_CLI_RSP_PARSER_RESULT_OK;
			if(sepPos == pos)
			{	pos = sepPos + 2;
				goto GET_NEXT;
			}	
			if(sepPos == NULL)
				break;
			*sepPos = 0;
			SENS_SSCANF(pos, "%x", &chunkDataLength);
			chunkLenOffset = sepPos - pos + 2;
			pos = sepPos+2;
			if(((int)pos + chunkDataLength) > ((int)skbQueue->buf + skbQueue->len))
			{	*sepPos = 0x0D;
				break;
			}
			if(chunkDataLength == 0)
			{	httpCliParserCtx->noMoreChunkData = 1;
			}
			else
			{	httpCliParserCtx->prevParseOffset += (chunkDataLength + chunkLenOffset);
				httpRspInfo->content = SENS_MEM_ZALLOC(chunkDataLength+1);
				memcpy(httpRspInfo->content, pos, chunkDataLength);
				dbg_msg("http response:%s\r\n", httpRspInfo->content);
				pos = pos + chunkDataLength;
			}
		}
		else 
		{	if(((int)pos + httpRspInfo->fileSize) > ((int)skbQueue->buf + skbQueue->len))
				break;
			
			httpRspInfo->content = SENS_MEM_ZALLOC(httpRspInfo->fileSize+1);
			memcpy(httpRspInfo->content, pos, httpRspInfo->fileSize);
			return HTTP_CLI_RSP_PARSER_RESULT_OK;
		}
		
GET_NEXT:
		if((int)pos >= ((int)skbQueue->buf + skbQueue->len))
			break;
	}
	
	return HTTP_CLI_RSP_PARSER_RESULT_INCOMPLETE_DATA;
}
//------------------------------------------------------------------------------
void parserHttpProtocol(SERV_RECV_CMD_CTX *servRecvCmd)
{	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)servRecvCmd->serverInst;
	SOCKET_QUEUE *skbQueue; //= servRecvCmd->skbQueue;
	HTTP_RSP_INFO *httpRspInfo;
	char isIncompleteData = 1;
	struct TaskQMsg msg;
	HTTP_CONTEXT *httpCtx;
	HTTP_CLI_PARSER *httpCliParserCtx;
	int parserResult;

	skbQueue = findOldestSockQueue(serverInst);
	if(skbQueue == NULL)
		return;
  
	httpCliParserCtx = serverInst->httpCliParserCtx;
	if(serverInst->httpCliParserCtx == NULL)
	{	serverInst->httpCliParserCtx = SENS_MEM_ZALLOC(sizeof(HTTP_CLI_PARSER));
		httpCliParserCtx = serverInst->httpCliParserCtx;
		httpCliParserCtx->httpRspInfo = SENS_MEM_ZALLOC(sizeof(HTTP_RSP_INFO));
		httpCliParserCtx->servInst = serverInst;
		httpRspInfo = httpCliParserCtx->httpRspInfo;
		httpRspInfo->id = -1;
		httpRspInfo->cmd = -1;
		httpRspInfo->status = 400;
		httpRspInfo->servInst = (void *)serverInst;
	}
  
	switch(httpCliParserCtx->fsm)
	{	case HTTP_CLIENT_RSP_FSM_IDLE:
					httpCliParserCtx->fsm = HTTP_CLIENT_RSP_FSM_HEADER_PARSER;
		case HTTP_CLIENT_RSP_FSM_HEADER_PARSER:
					parserResult = parserHttpHeader(skbQueue, httpCliParserCtx);
					if(parserResult != HTTP_CLI_RSP_PARSER_RESULT_INCOMPLETE_DATA)
						break;
		case HTTP_CLIENT_RSP_FSM_PAYLOAD_PARSER:
					parserResult = parserHttpPayload(skbQueue, httpCliParserCtx);
					if(parserResult != HTTP_CLI_RSP_PARSER_RESULT_INCOMPLETE_DATA)
						httpCliParserCtx->fsm = HTTP_CLIENT_RSP_FSM_MAX;
					if(parserResult == HTTP_CLI_RSP_PARSER_RESULT_OK)
						isIncompleteData = 0;
					break;
		default:
					break;
	}
	
	if(isIncompleteData)
		return;
	else
	{	if(skbQueue)
		{	//SENS_SEM_LOCK(serverInst->sockRecvQueueLock);
			removeSockQueue(serverInst, skbQueue);
			//SENS_SEM_UNLOCK(serverInst->sockRecvQueueLock);
		}
#if SUPPORT_IOA_WEB_API
		if(serverInst->xmitProtocolType == PROTOCOL_IOA_WEB_API)
		{	msg.msgId = NETWORK_Q_MSG_IOA_RSP;
		}
		else
#endif
		{	httpCtx = selectOtaSvrHttpCtx();
			if(httpCtx->httpCmd == HTTP_GET)
				msg.msgId = NETWORK_Q_MSG_HTTP_GET_DONE;
			else
				msg.msgId = NETWORK_Q_MSG_HTTP_POST_DONE;
		}
		msg.ptr = (char *)httpCliParserCtx;
		if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
		{	
		}
	}
	
	return;	  
}
