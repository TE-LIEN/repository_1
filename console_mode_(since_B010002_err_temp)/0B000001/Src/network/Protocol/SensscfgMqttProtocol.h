#ifndef __SENS_CFG_MQTT_PROTOCOL_H__
#define __SENS_CFG_MQTT_PROTOCOL_H__

#include "sensminiCfg.h"

#define SENSCFG_CONNECT_CMD_RBTCNT_STR		"RbtCnt"
#define SENSCFG_CONNECT_CMD_EXT_VOLT_STR	"ExtVolt"
#define SENSCFG_CONNECT_CMD_BAT_STR			"Bat"
#define SENSCFG_CONNECT_CMD_FW_STR			"Fw"
#define SENSCFG_CTRL_FIELD_TID_STR			"Tid"
#define SENSCFG_UPLOAD_FIELD_CONNECT_STR	"Connect"
#define SENSCFG_CTRL_CMD_STR				"Cmd"	
#define SENSCFG_CTRL_FIELD_CTRLS_STR		"Ctrls"
#define SENSCFG_CTRL_CMD_TYPE_STR			"Get"	//Get ot Set, true or false
#define SENSCFG_CTRL_CMD_RBT_STR			"Rbt"	//reboot
#define SENSCFG_CTRL_CMD_CTRL_STR			"Ctrls"
#define SENSCFG_CTRL_CMD_IM_STR				"IM"
#define SENSCFG_CTRL_CMD_CTRL_NAME_STR		"Name"
#define SENSCFG_CTRL_CMD_CTRL_VAL_STR		"Val"
#define SENSCFG_ACK_STS						"Sts"
#define SENSCFG_ACK_OK						"Ok"
#define SENSCFG_ACK_FAIL					"Fail"


#define SENSCFG_CMD_TYPE_GET	1
#define SENSCFG_CMD_TYPE_SET	0
/*	JSON Sample
SET
topic SensCfg/{GUID}/Ctrl
{	"Tid":"1234567",
	"Cmd":"cfg",	//cfg, fota
	"Get":false,
	"Rbt":true,
	"Ctrls":[
		{	"Name" : "aaaaa",
			"Val" : "XXXXXXXXXX"
		},
		{	"Name" : "bbbbbb",
			"Val" : "YYYYYYYYYY"
		}
	]
}
SET RSP
topic SensCfg/{GUID}/ack

{	"Tid":"1234567",
	"Sts":"OK"
}


GET
topic SensCfg/{GUID}/Ctrl
{	"Tid":"1234568",
	"Cmd":"cfg",	//cfg, fota
	"Get":true,
	"Ctrls":["aaaaa", "bbbbbb"]	
	or
	"Ctrls": []	//for all, but length maybe over
}
GET RSP
topic SensCfg/{GUID}/ack
{	"Tid":"1234568",
	"Ctrls":[
		{	"Name" : "aaaaa",
			"Val" : "XXXXXXXXXX"
		},
		{	"Name" : "bbbbbb",
			"Val" : "YYYYYYYYYY"
		}
	]
}
*/

typedef struct _SENSCFG_CTX
{	struct _SENSCFG_CTX	*next;
	char 				*name;
	char				*val;
}SENSCFG_CTX;

extern void parserSensCfgMqttProtocol(SERV_RECV_CMD_CTX *servRecvCmdCtx);
extern void sensCfgMqttCmdProcess(SERVER_CMD_STRUCT *servCmd);
#endif