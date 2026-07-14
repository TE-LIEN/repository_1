#ifndef __NETWORK_PROCESS_APP_H__
#define __NETWORK_PROCESS_APP_H__

#include "sensminiCfg.h"


#define RTCS_OK				0
#define RTCS_SOCKET_ERROR	-1

#define ERR_NO_IMAGE_FILE 	-3

//#define MAX_FRAME	(512)	//(1024-29)
#define MOBILE_MAX_FRAME	(512)	//(1024-29)
#define WIRED_MAX_FRAME		(512)	//(1024-29)
//#define MAX_FRAME	(768-29)	//(1024-29)
//#define MAX_FRAME	(1400)	//(1024-29)

#define EMPTY RTCS_SOCKET_ERROR

//#define MDVPN_DEVICE_SENSLINK_NET_IP	"https://192.168.116.43"
#define MDVPN_DEVICE_SENSLINK_NET_IP	"http://192.168.116.201"
#define DEVICE_SENSLINK_NET_DOMAIN_NAME	"https://device.senslink.net"
#define DEVICE_SENSLINK_ID_DOMAIN_NAME	"https://device.senslink.id"
//#define DEVICE_SENSLINK_NET_DOMAIN_NAME	"https://211.21.191.28"


#define SENSMINI_V3_WEB_SERVER_PATH				"/v3/sensmini"
#define SENSMINI_V4_TEST_WEB_SERVER_PATH		"/demo/v3/sensmini"

//#define SENSMINI_HTTP_SERVER_PATH		SENSMINI_V4_TEST_WEB_SERVER_PATH
#define SENSMINI_HTTP_SERVER_PATH		SENSMINI_V3_WEB_SERVER_PATH

#define POST_HEADER						"POST %s HTTP/1.1\r\n"									\
	            						"User-Agent: Mozilla/5.0\r\n"							\
		        						"Accept: */*\r\n"										\
		        						"Host: %s\r\n"											\
		        						"Accept-Encoding: gzip, deflate, br\r\n"				\
		        						"Connection: keep-alive\r\n"							\
		        						"Content-Type: multipart/form-data; boundary=%s\r\n"	\
		        						"Content-Length: %d\r\n%s"
		        									
		        									
#define POST_HEADER1					"\r\n%s\r\n"																\
										"Content-Disposition: form-data; name=\"\"; filename=\"sensmini.jpg\"\r\n"	\
		        						"Content-Type: image/jpeg\r\n\r\n"
		        									
#define HTTP_DEFAULT_HEADER		"%s HTTP/1.1\r\n"							\
								"User-Agent: Mozilla/5.0\r\n"				\
								"Accept: */*\r\n"							\
								"Host: %s\r\n"								\
								"Accept-Encoding: gzip, deflate, br\r\n"	\
								"Connection: keep-alive\r\n"

#define HTTP_HEADER_STR_CONTENT_TYPE								"Content-Type"
	#define HTTP_HEADER_STR_APPLICATION_OCTET_STREAM				"application/octet-stream"
	#define HTTP_HEADER_STR_APPLICATION_JSON 						"application/json"
#define HTTP_HEADER_STR_CONTENT_LENGTH 								"Content-Length"
#define HTTP_HEADER_STR_AUTHORIZATION								"Authorization"
	#define HTTP_HEADER_STR_BEARER									"Bearer "
		        									
#define POST_APPLICATION_OCTET_STREAM		"POST %s HTTP/1.1\r\n"							\
											"Content-Type:application/octet-stream\r\n"		\
											"User-Agent: Mozilla/5.0\r\n"					\
											"Accept: */*\r\n"								\
											"Host: %s\r\n"									\
											"Accept-Encoding: gzip, deflate, br\r\n"		\
											"Connection: keep-alive\r\n"					\
											"Content-Length: %d\r\n\r\n"
																				
#define GET_APPLICATION_OCTET_STREAM		"GET %s HTTP/1.1\r\n"						\
											"User-Agent: Mozilla/5.0\r\n"				\
											"Accept: */*\r\n"							\
											"Host: %s\r\n"								\
											"Accept-Encoding: gzip, deflate, br\r\n"	\
											"Connection: keep-alive\r\n\r\n"									
																				

extern CLOUD_SERVER_INSTANCE *findFreeServerInstance(void);
extern CLOUD_SERVER_INSTANCE *findServerInstByIp(char *serverIp, uint16_t port);
extern CLOUD_SERVER_INSTANCE *findServerInstBySocketFd(int socketFd);
extern CLOUD_SERVER_INSTANCE *findImgServerInst(void);
extern CLOUD_SERVER_INSTANCE *findOtaServerInst(void);
extern CLOUD_SERVER_INSTANCE *findUsedServerInst(void);

extern SOCKET_QUEUE *createNewSockQueue(CLOUD_SERVER_INSTANCE *serverInst, int len);
extern void clearReceiveQueue(CLOUD_SERVER_INSTANCE *serverInst);
extern int removeSockQueue(CLOUD_SERVER_INSTANCE *serverInst, SOCKET_QUEUE *removeQueue);
extern SOCKET_QUEUE *findOldestSockQueue(CLOUD_SERVER_INSTANCE	*serverInst);
extern void lanSocketClose(CLOUD_SERVER_INSTANCE *serverInst, char delTaskDirect, const char *callerFunc, int callerLine);

extern int tcpSockSend(MOBILE_WRITE_SOCKET_CMD *mobileWriteSocketCmd);
extern int tcpClientSockCreate(CLOUD_SERVER_INSTANCE *serverInst);
extern int tcpClientSockConnect(CLOUD_SERVER_INSTANCE *serverInst);

extern int networkSocketWrite(CLOUD_SERVER_INSTANCE *serverInst, unsigned char *buf, int bufLen, int timeout);
extern int networkSocketRead(CLOUD_SERVER_INSTANCE *serverInst, unsigned char *buf, int len, int timeout);


extern int ioaGetToken(CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx);
extern int ioaSendData(CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx);

extern HTTP_CONTEXT *selectOtaSvrHttpCtx(void);
extern void timeLapseImageSend(IMAGE_UPLOAD_INFO *imageInfo);
//extern int timeLapseImagePost(CLOUD_SERVER_INSTANCE *serverInst, IMAGE_UPLOAD_INFO *imageInfo);
extern void setOtaUrl(char *url, enum LTE_MDVPN_TYPE mdvpnType, uint8_t isFota);
#endif
