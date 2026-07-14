#ifndef __IOA_PROTOCOL_H__
#define __IOA_PROTOCOL_H__

#include "sensminiCfg.h"

#define IOAWEB_API_TEST_URL				"https://water.anchorpoint.com.tw"


#define IOAWEB_API_PATH_AUTH			"/sensorapiv1/Auth"
#define IOAWEB_API_PATH_SENSOR_DATA		"/sensorapiv1/SensorData"

#define IOAWEB_API_RSP_STATUS			"status"
#define IOAWEB_API_RSP_TOKEN			"token"
#define IOAWEB_API_RSP_MESSAGE			"message"
#define IOAWEB_API_RSP_OK				"OK"

#define IOAWEB_API_REQ_EQUIP_ID			"equip_id"
#define IOAWEB_API_REQ_API_KEY			"api_key"

#define IOAWEB_API_REQ_SENSOR_DATA		"sensor_data"
#define IOAWEB_API_REQ_SENSOR_ID		"sensor_id"
#define IOAWEB_API_REQ_DATA_TIME		"data_time"
#define IOAWEB_API_REQ_DATA_VLAUE		"data_value"

enum IOA_WEB_API_FSM
{	IOA_WEB_API_FSM_IDLE,
	IOA_WEB_API_FSM_GET_TOKEN,
	IOA_WEB_API_FSM_WAIT_TOKEN,
	IOA_WEB_API_FSM_TOKEN_DONE,
	IOA_WEB_API_FSM_DATA_SENDING,
	IOA_WEB_API_FSM_MAX
};

typedef struct _IOA_SERVER_CTX
{	struct _IOA_SERVER_CTX	*next;
	void					*servInst;
	char					*token;
	char					*equipId;
	char					*apiKey;
	HTTP_CONTEXT			httpCtx;
	enum IOA_WEB_API_FSM	fsm;
	uint32_t				getTokenTimer;
}IOA_SERVER_CTX;


extern int ioaRspParse(IOA_SERVER_CTX *ioaServCtx, char *webRspContent);
extern char *setTokenPayload(IOA_SERVER_CTX *ioaServCtx);
extern char *setSendDataPayload(CLOUD_SERVER_INSTANCE *serverInst);

#endif
