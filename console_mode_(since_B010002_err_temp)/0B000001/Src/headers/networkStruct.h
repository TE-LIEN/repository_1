#ifndef __NETWORK_STRUCT_H__
#define __NETWORK_STRUCT_H__

#include "sensminiCfg.h"
#include "headers/mqttEnum.h"
#include "headers/networkConfig.h"
#include "headers/networkEnum.h"


typedef struct _OTA_REC_CTX	                                                //struct save to SD
{	enum OTA_STATE_MACHINE	otaSm;
  uint16_t 					tag;	                                                                //0x55 0xAA
  int 						loadOffset;
  int 						loadSize;
  int 						binOffset;
  int 						binSize;
  int 						checksum;
  int 						checksumWriteOffset;
  int 						loadsizePerCmd;
  int 						otaFinish;
  int						loadSizePerApi;
  int						retryCnt;
#ifdef SUPPORT_FOTA_MD5
  int 						isValidMd5;
  unsigned char 			srcMd5[16];
  unsigned char 			currMd5[16];
#endif
}OTA_REC_CTX;

typedef struct _OTA_CONTEXT
{	enum OTA_STATE_MACHINE 	otaSm;
	int 					fileSize;
	int						loadSize;
	int						loadOffset;
	int						loadsizePerCmd;
	int						loadsizePerCmdTemp;
	int						remainSize;
	char 					*fsFileBuf;
	int						fsFileBufPtr;
	char					fsReadLostData;
	char					readOtaFile;
	int						fileSizePerHttps;
	int						sockConnErrorCnt;
	int						xmitTimeoutCnt;
	OTA_REC_CTX 			otaRecCtx;
}OTA_CONTEXT;

typedef struct _HTTP_REQUEST_HEADER
{	struct _HTTP_REQUEST_HEADER *next;
	char 						*name;
	char 						*value;
}HTTP_REQUEST_HEADER;

typedef struct _HTTP_REQUEST_STRUCT
{	HTTP_REQUEST_HEADER	*headers;
	char				 *payload;
}HTTP_REQUEST_STRUCT;

typedef struct _HTTP_CONTEXT
{	char 					*url;
	char 					*path;
	char 					*hostStr;
	uint8_t					httpCmd;
	char 					running;
	char 					timerExtend;
	uint8_t					failCount;
	uint8_t					apiUseCnt;	//over 50 re-connect
	enum HTTP_CLIENT_FSM 	httpClientFsm;
	HTTP_REQUEST_STRUCT 	*httpRequestCtx;
}HTTP_CONTEXT;

typedef struct _HTTP_RSP_INFO
{	int id;
	int cmd;
	int status;
	int	isOctetStreamData;
	int fileSize;
	char *contentType;
	char *content;
	void *servInst;
}HTTP_RSP_INFO;	//HttpRspSts

typedef struct _HTTP_CLI_PARSER
{	enum HTTP_CLIENT_RSP_FSM	fsm;
	uint8_t						isChunked;
	uint8_t						noMoreChunkData;
	uint32_t					prevParseOffset;
	void						*servInst;
	HTTP_RSP_INFO				*httpRspInfo;
}HTTP_CLI_PARSER;

typedef struct Sock_Queue
{	struct Sock_Queue	*next;
	struct Sock_Queue	*prev;
	SOCKET_T 			fd;
	unsigned char 		isValid;
	unsigned char 		isTop;
	unsigned char 		*buf;
	int 				len;
	unsigned int 		currReceiveLen;
	unsigned int 		offset;
	unsigned int 		count;
	uint32_t			validDataCount;
}SOCKET_QUEUE;


typedef struct _SERV_RECV_CMD_CTX	//struct SenstalkRecvCmd
{	void			*cmdProtocol;
	char 			*topic;
	union
	{	char 		*msg;
		char		*jsonMsg;
	};
	unsigned int 	topicLen;
	unsigned int 	msgLen;
	char			generateFromInternet;
	void			*serverInst;
	SOCKET_QUEUE	*skbQueue;
}SERV_RECV_CMD_CTX;

typedef struct SenstalkCmd
{ 	char				cmdDir;
	char 				rspType;
	char 				cfgInfoValid[MAX_CFG_ITEM];    
	int 				cfgInfo[MAX_CFG_ITEM]; 
	unsigned int 		subCmdType;
	unsigned int 		startDt;
	unsigned int 		endDt;
	unsigned int 		recDiOffset;
	uint64_t 			current_tick;
	uint64_t 			tid;
	char				tidStr[11];
	void 				*cmdProtocol;
	enum SENSTALK_MQTT_CMD_TYPE cmdType;
#if SUPPORT_SENSTALK_MQTT_COMPRESSION
	char			compressionTopic;
#endif
	void			*otherCmdInfo;	//IKW, SensSewer
}SERVER_CMD_STRUCT;

typedef struct _CLOUD_SERVER_INSTANCE
{	
#if AUTO_DATA_SYNC
	//AUTO_DATA_SYNC_STRUCT	*autoDataSyncCtx;
	void					*autoDataSyncCtx;
	SENS_SEM_STRUCT			autoDataSyncCtxLock;
#endif
	SENS_SEM_STRUCT			lock;
	SENS_SEM_STRUCT			sockConnLock;
	int 					servIdx;
	enum NET_SEND_FSM		netSendFsm; 
	uint8_t					isTlsProtocol;
	uint8_t					enConnectTimeout;
	uint8_t					connectRetryCnt;
	uint8_t					isUsing;
	uint8_t					isTempUsing;
	uint8_t					isImgSocket;
	uint8_t					isOtaSocket;
	int32_t					serverType;
	char					*serverIp;
	char					serverIpV4[4];
	uint16_t				serverPort;
	char					serverName[256];
	enum XMIT_PROTOCOL_TYPE	xmitProtocolType;
	SOCK_STATE				socketState;
	SOCKET_T 				fd;
	int						countOfProcessCmdFromInternet;
#if defined NET_LWIP
	struct
#endif
		sockaddr_in			socketAddr;
	long					autosendTimeSlot;
	int						connectTimer;
	void					*tlsCtx;
	void					*protCtx;	//for MQTT use
#ifdef OS_FREERTOS
	TaskHandle_t			xTask;
#endif
	SENS_SEM_STRUCT			sockRecvQueueLock;
	SOCKET_QUEUE			*sockRecvQueue;
	union
	{	MQTT_CONNECT_INFO	*mqttConnectInfo;
		IOA_CONNECT_INFO	*ioaConnectInfo;	
	};
	
	/*char					*mqttDevName;
	char 					*mqttUserName;
	char 					*mqttPassword;
	char					*mqttPubTopic;
	char					*mqttSubTopic;*/
	void					*mqttSendMsg;
	HTTP_CLI_PARSER			*httpCliParserCtx;
}CLOUD_SERVER_INSTANCE;

typedef struct _WRITE_BUF_TYPE
{	enum XMIT_PROTOCOL_TYPE protoType;
	int						func;
	int						dataArrayId;
}WRITE_BUF_TYPE;

typedef struct _MOBILE_WRITE_SOCKET_CMD
{	//SocketContext *sock;
	CLOUD_SERVER_INSTANCE	*serverInst;
	unsigned char 			*buf;
	int 					bufLen;
	WRITE_BUF_TYPE 			bufType;
	char					cmdIsFromInternet;
//#ifdef AUTO_DATA_SYNC
	void 					*ctx;
//#endif
}MOBILE_WRITE_SOCKET_CMD;


typedef struct _AT_CMD_RECV_BACK_BUF
{	uint8_t *buf;
	int len;
}AT_CMD_RECV_BACK_BUF;

typedef struct _AT_CMD_RECV
{	char *receiveBuf;
	int receivePos;
	SENS_SEM_STRUCT lock;
}AT_CMD_RECV;

typedef struct _IOT_SYSTEM
{	SENS_SEM_STRUCT	iotSem;
	AT_CMD_RECV		atCmdRecv;
	int				runningCount;
}IOT_SYSTEM;


/*
*	MOBILE NETWORK
*/

#if defined SENSMINIS2 || defined SENSMINIA4_NEO
typedef struct _MOBILE_USB_INSTANCE
{	uint8_t		isUsbHost;
	uint8_t		usbIsMultiIfDev;
	uint8_t		usbIsCdcAcm;
}MOBILE_USB_INSTANCE;
#endif

typedef struct _MOBILE_INSTANCE
{	//enum MOBILE_INIT_STATE_MACHINE	fsm;
	void 					*upperCtx;
	enum MOBILE_TYPE		module;
	enum MOBILE_PROTOCOL	protocol;
	enum MOBILE_AT_TYPE		atType;
	enum MOBILE_CELL_TYPE	initType;
	enum MOBILE_CELL_TYPE	mobileCellType;
	enum SIM_TYPE			simCardTech;
	uint32_t				baudrate;
#if defined SENSMINIA4_PLUS
	MQX_FILE_PTR			dev;
	MQX_FILE_PTR			devTemp;
	MQX_FILE_PTR			atCmdDev;
	MQX_FILE_PTR			pppDev;
#elif defined PLATFORM_XILINX
	UART_CTX				*dev;
	UART_CTX				*devTemp;
	UART_CTX				*atCmdDev;
	UART_CTX				*pppDev;
#elif defined SENSMINIS2 || defined SENSMINIA4_NEO
	MOBILE_USB_INSTANCE		usbInst;		//HOST USB
	void					*usbDev;		//HOST USB
	void					*usbDevAt1;		//HOST USB
	void 					*usbDevAt2;		//HOST USB
	void 					*usbDevNmea;	//HOST USB	
	UART_CTX				*dev;			//UART 
	UART_CTX				*devTemp;		//UART_PPP_CMUX
	UART_CTX				*atCmdDev;		//UART_PPP_CMUX
	UART_CTX				*pppDev;		//UART_PPP_CMUX
#endif
	enum WL_INIT_STS		status;
#if !defined PLATFORM_XILINX
	enum UART_FLOW_CTRL		flowCtrl;
#endif
	uint8_t					ceregRdy;
	uint8_t					uartBaudrateChange;
	uint8_t					errorRetryCnt;
	uint8_t					simAuthType;
#if SUPPORT_AGPS
	uint8_t					supportAgps;					
	//uint8_t 				currentIsGpsOnlyMode;
	uint8_t					setExtendWorkingTime;
	uint8_t					enGpsTimeout;
	uint8_t					gpsRunMode;
	uint8_t					isAgnssMode;
	long					gpsStartTimer;
	int						gpsPrevDateTime;
	int						supportGtp;
	int						firstGetLocation;
#endif
	int						netProfileId;
	enum WL_REGISTER_STS	wlStatus;
	float					wlSignalQ;
	char 					apn[32];
	//char 					plmn[6];
	char 					simPass[32];
	char					simAccount[32];
	uint8_t					ip[4];
	uint32_t				plmn;
#ifdef SUPPORT_PPP
	uint8_t					usePPP;
	#ifdef NET_RTCS
	_ip_address       		localAddress;
	_ip_address       		remoteAddress;
	void					*modemParams;
	void					*pppParams;
	uint32_t				pppHandle;
	#endif
	#ifdef NET_LWIP
	uint8_t					startUsePpp;
	struct netif			*ppposNetif;
	void					*ppp;
	#endif
	void					*cMuxCtx;
	uint8_t					isNotSupportCmuxMode;
#endif
	//uint8_t								isSenslinkMdvpn;
#if !SUPPORT_NUVOTON_USB_HOST	//for FSL
	uint8_t								isUsbMode;
	uint8_t								isCdcAcm;
	uint8_t								usbUp;
#endif
	uint8_t								isNewL210;
}MOBILE_INSTANCE;


/*
*	WIRED NETWORK
*/
#ifdef SUPPORT_WIRED_LAN

typedef struct _WIRED_INSTANCE
{	
#ifdef NET_LWIP
	ip_addr_t		ipAddr;
	ip_addr_t		ipMask;
	ip_addr_t		ipGw;
	struct netif	ethNetif;
#endif
}WIRED_INSTANCE;


#endif


typedef struct _NET_INSTANCE
{	SENS_SEM_STRUCT		lock;
	uint8_t				enable;
	//uint8_t			atLeastSendOneData;
	uint8_t				netType;	//WIRED LAN, HS-COMMM mobile, LS-COMM mobile
	uint8_t				initError;
	uint8_t				connectToCellSite;
#if SUPPORT_AGPS
  uint8_t 				currentIsGpsOnlyMode;
#endif
#ifdef SUPPORT_WIRED_LAN
	WIRED_INSTANCE		*wiredInst;
#endif
	MOBILE_INSTANCE		*wlInst;
	enum LTE_MDVPN_TYPE	netMdvpnType;
	char				*sendStr;
	char				*recvStr;
	char				*stsStr;
#if SUPPORT_USB
	int32_t				isUsbModule;
#endif
}NET_INSTANCE;

typedef struct _IMAGE_UPLOAD_INFO	//ImageInfo
{	int			valid;
	char		fileName[IMG_FILE_SIZE];
	uint64_t 	unixTime;
	int			fileLength;
	//int						processing;
}IMAGE_UPLOAD_INFO;

typedef struct _IMAGE_UPLOAD_INSTANCE
{	//int							imagePostStep;
	SENS_SEM_STRUCT				imgUploadSem;
	enum IMG_UPLOAD_FSM			imgUploadFsm;
	enum IMG_UPLOAD_FSM			imgUploadFsmNext;
	IMAGE_UPLOAD_REC_HEADER 	*imgUploadRecHeader;
	IMAGE_UPLOAD_INFO			*uploadImgInfo;
	char 						*imgUploadRecInfoBuf;	// all IMAGE_UPLOAD_INFO buf
	int							imgUploadRecInfoBufLen;
	uint32_t					waitTimer;
	uint32_t					waitTimeout;
	int8_t						alarmUploadFinalImg;
	uint8_t						isAlarmUploadImgMode;
	//char						enablePostImgTimeout;
	//long						postImgTimer;
}IMAGE_UPLOAD_INSTANCE;

typedef struct _NETWORK_CONTEXT
{	SENS_SEM_STRUCT	lock;
	NET_INSTANCE			*workingInst;
	//MESH_INSTANCE			*meshInst;			//WISUN root/Node, LoRa P2P, gps L76
	//MESH_INSTANCE			*meshWanInst;			//LoRa Wan
	NET_INSTANCE			*hsCommInst;
	NET_INSTANCE			*lsCommInst;
	NET_INSTANCE			*lanInst;
	CLOUD_SERVER_INSTANCE	cloudServers[MAX_CLOUD_SERVER];
	IMAGE_UPLOAD_INSTANCE	*imgUploadInst;
	uint8_t					timeToEnableNetwork;
	uint8_t					checkForNextWlWakeup;
	uint8_t					networkIsUp;
	uint8_t					mobileUartStartRecv;
	uint8_t					powerSavingMode;
	uint8_t 				otaRunning;
	uint8_t					send1FrameToCloudBmp;
	uint8_t					atleastSend1DataBmp;
	uint32_t 				iSkts[MAX_WIRE_LAN_SOCK];
	uint32_t				wlWakeupTimer;
	int 					sockCnt;
	//int						imagePostStep;
	int						mobileUseUsb;
	HTTP_CONTEXT			*idImgHttpCtx;	//image server, taiwan or id
	HTTP_CONTEXT			*twHttpCtx;	//ota server, only taiwan server
	OTA_CONTEXT				*otaCtx;
	SENS_SEM_STRUCT			timeZoneLock;
#if defined NUVOTON
	S_RTC_TIME_DATA_T		timezone;
#else	//FSL
	uint16_t				timezone[7];
#endif
	int 					timezonediff;
	int 					daylightsaving;
	OTA_REC_CTX				*prevOtaRecCtx;
//#if YS_MQTT_BROKER
	int 					ysServerConntectDone;
	int						ysServerSendConnDone;
	int						ysServerOffTimer;
//#endif
	//uint8_t					imgSending;
	uint8_t					agpsPause;	//only for ME310
	uint8_t					srvActiveBmp;
	uint8_t					cloudActiveBmp;
	uint8_t					srvConnectBmp;
}NETWORK_CONTEXT;

#endif