#ifndef __SENSTALK_PROTOCOL_H__
#define __SENSTALK_PROTOCOL_H__

#include "sensminiCfg.h"

#define SENSTALK_REAL_TIME_WITH_TS				0


#define SENSTALK_FUNC_DA_READ					0x00
#define SENSTALK_FUNC_DA_WRITE					0x01
	#define SENSTALK_DA_ID_DI					0x00
	#define SENSTALK_DA_ID_DO					0x01
	#define	SENSTALK_DA_ID_AI_FLOAT				0x02
	#define	SENSTALK_DA_ID_AI_INT				0x03
	#define	SENSTALK_DA_ID_LAN_PARAM			0x06
	#define	SENSTALK_DA_ID_CLIENT_IP			0x07
	#define	SENSTALK_DA_ID_PARAM				0x08
	#define	SENSTALK_DA_ID_PARAM_1				0x0A
	#define	SENSTALK_DA_ID_PARAM_2				0x0B
	#define	SENSTALK_DA_ID_APN					0x0C
	#define	SENSTALK_DA_ID_KEEPALIVE			0x0F
	#define	SENSTALK_DA_ID_SN					0x13
	#define	SENSTALK_DA_ID_TIME					0x14
	#define	SENSTALK_DA_ID_FW_VER				0x15
	#define	SENSTALK_DA_ID_AI_FLOAT_WITH_TS		0x16	//with timestamp
	#define	SENSTALK_DA_ID_AI_INT_WITH_TS		0x17	//with timestamp
	
	
#define SENSTALK_FUNC_REC_DAY					0x52
#define SENSTALK_FUNC_REC_HOUR					0x53
#define SENSTALK_FUNC_REC_MIN					0x54
#define SENSTALK_FUNC_REC_ANY					0x55
#define SENSTALK_FUNC_REC_5MIN					0x57
#define SENSTALK_FUNC_EVENT_DAY					0x72
#define SENSTALK_FUNC_EVENT_HOUR				0x73
#define SENSTALK_FUNC_EVENT_MIN					0x74
#define SENSTALK_FUNC_IMAGE							0x81

SENS_PACK_IAR typedef struct _SenstalkV2_Normal_Header	//
{	uint16_t	tag;
	uint8_t		sn[4];
	uint16_t	size;
	uint8_t		func;
	uint8_t		dataType;
	uint16_t	dataArrayId;
	uint16_t	startAddr;
	uint16_t	quantity;
}SENS_PACK_GCC SenstalkV2NormalHeader;

SENS_PACK_IAR typedef struct _SenstalkV2_Rec_Header
{	uint16_t	tag;
	uint8_t		sn[4];
	uint16_t	size;
	uint8_t		func;
	uint8_t		dataType;
	uint16_t	dataArrayId;
	uint8_t		unixTime[8];
}SENS_PACK_GCC SenstalkV2RecHeader;

SENS_PACK_IAR typedef struct _SenstalkV2_File_Header
{	uint16_t	tag;
	uint8_t		sn[4];
	uint16_t	size;
	uint8_t		func;
	uint8_t		unixTime[8];
}SENS_PACK_GCC SenstalkV2FileHeader;

#define DATATYPE_BIT    0
#define DATATYPE_CHAR   1
#define DATATYPE_UINT16 2
#define DATATYPE_UINT32 3
#define DATATYPE_UINT64 4
#define DATATYPE_SINGLE 5
#define DATATYPE_DOUBLE 6

typedef struct _MQTT_SEND_INFO
{	char 	*sendTopic;
	char 	*sendMessage;
	int		sendMessageLen;
	char	publishToBroker;
#if MQTT_SEND_SUCCESS_WITH_TAG
	char	sendAndWriteTagFlag;
#endif
	int		cmdTimeout;
}MQTT_SEND_INFO;

SENS_PACK_IAR typedef struct _SENSTALK_DI_REC_FORMAT
{	//uint_64 dt;
	union{
		unsigned char d[8];
		uint64_t unixTime;
	};
	char status;
	char flag;
}SENS_PACK_GCC SENSTALK_DI_REC_FORMAT;

extern void parserSenstalkProtocol(SERV_RECV_CMD_CTX *senstalkRecvCmd);


#endif