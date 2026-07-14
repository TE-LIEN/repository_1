#ifndef __HTTP_CLIENT_PROTOCOL_H__
#define __HTTP_CLIENT_PROTOCOL_H__

#include "sensminiCfg.h"

#define HTTP_DEFAULT_HEADER		"%s HTTP/1.1\r\n"							\
								"User-Agent: Mozilla/5.0\r\n"				\
								"Accept: */*\r\n"							\
								"Host: %s\r\n"								\
								"Accept-Encoding: gzip, deflate, br\r\n"	\
								"Connection: keep-alive\r\n"


extern int ethHttpConnect(CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx);
extern int httpSendRequest(CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx);
extern void clearRequestCtx(HTTP_CONTEXT *httpCtx);
extern void addHttpRequestHeader(HTTP_REQUEST_STRUCT *requstCtx, char *name, char *value);

extern void parserHttpProtocol(SERV_RECV_CMD_CTX *senstalkRecvCmd);
extern void clearHttpCliParserCtx(CLOUD_SERVER_INSTANCE *serverInst);

#endif