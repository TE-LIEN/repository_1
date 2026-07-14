#ifndef __SENSTALK_MQTT_PROTOCOL_H__
#define __SENSTALK_MQTT_PROTOCOL_H__

#include "sensminiCfg.h"
#include "cJSON/cJSON.h"



/*********************************
***   senstalk mqtt protocol   ***
**********************************/
/*
* WRITE CONFIG TABLE
*/
#define WR_CFG_POS_REC_IV			0
#define WR_CFG_POS_SLP				1
#define WR_CFG_POS_TIME				2
#define WR_CFG_POS_AUTO_OTA			3

#define SENSTALK_MQTT_CMD_READ							0
#define SENSTALK_MQTT_CMD_WRITE							1
                                                		
#define SENSTALK_MQTT_SUB_CMD_AI						0x0001
#define SENSTALK_MQTT_SUB_CMD_DI						0x0002
#define SENSTALK_MQTT_SUB_CMD_DO						0x0004
#define SENSTALK_MQTT_SUB_CMD_TEMP						0x0008
#define SENSTALK_MQTT_SUB_CMD_TIME						0x0010
#define SENSTALK_MQTT_SUB_CMD_RBTCT						0x0020
#define SENSTALK_MQTT_SUB_CMD_BATV						0x0040
#define SENSTALK_MQTT_SUB_CMD_EXTV						0x0080
#define SENSTALK_MQTT_SUB_CMD_4GSI						0x0100
#define SENSTALK_MQTT_SUB_CMD_NBSI						0x0200
#define SENSTALK_MQTT_SUB_CMD_RECIV						0x0400
#define SENSTALK_MQTT_SUB_CMD_SLP						0x0800
#define SENSTALK_MQTT_SUB_CMD_FW_VER		        	0x1000
#define SENSTALK_MQTT_SUB_CMD_FW_OTA		        	0x2000
#define SENSTALK_MQTT_SUB_CMD_TIME_CALIBRATION			0x4000

#define SENSTALK_MQTT_SUB_CMD_ALL_SYS					(SENSTALK_MQTT_SUB_CMD_TEMP | SENSTALK_MQTT_SUB_CMD_TIME | SENSTALK_MQTT_SUB_CMD_RBTCT | SENSTALK_MQTT_SUB_CMD_BATV | SENSTALK_MQTT_SUB_CMD_EXTV | SENSTALK_MQTT_SUB_CMD_4GSI | SENSTALK_MQTT_SUB_CMD_NBSI)
#define SENSTALK_MQTT_SUB_CMD_ALL_CFG					(SENSTALK_MQTT_SUB_CMD_RECIV | SENSTALK_MQTT_SUB_CMD_SLP | SENSTALK_MQTT_SUB_CMD_FW_VER | SENSTALK_MQTT_SUB_CMD_FW_OTA | SENSTALK_MQTT_SUB_CMD_TIME_CALIBRATION)

#define SENSTALK_MQTT_SUB_CMD_TEMP_IDX					1
#define SENSTALK_MQTT_SUB_CMD_TIME_IDX					2
#define SENSTALK_MQTT_SUB_CMD_RBTCT_IDX					3
#define SENSTALK_MQTT_SUB_CMD_BATV_IDX					4
#define SENSTALK_MQTT_SUB_CMD_EXTV_IDX					5
#define SENSTALK_MQTT_SUB_CMD_4GSI_IDX					6
#define SENSTALK_MQTT_SUB_CMD_NBSI_IDX					7
#define SENSTALK_MQTT_SUB_CMD_RECIV_IDX					8
#define SENSTALK_MQTT_SUB_CMD_SLP_IDX					9
#define SENSTALK_MQTT_SUB_CMD_FW_VER_IDX				10
#define SENSTALK_MQTT_SUB_CMD_TIME_CALIBRATION_IDX		11
#define SENSTALK_MQTT_SUB_CMD_FW_OTA_IDX				12

#define UNIT_INT		                        		0
#define UNIT_FLOAT	                                	1
#define UNIT_BOOL		                        		2
#define UNIT_FLOAT_NO_DECIMAL_POINT	                	3
#define UNIT_UINT		                        		4
#define UNIT_STRING	                                	5
#define UNIT_LONG		                        		6
#define UNIT_HEX_STRING	                            	7

#define SENSTALK_JSON_STR_VER			"Ver"
#define SENSTALK_JSON_STR_Tid			"Tid"
#define SENSTALK_JSON_STR_CMD			"Cmd"
#define SENSTALK_JSON_STR_IOTp			"IOTp"
#define SENSTALK_JSON_STR_IOTps			"IOTps"
#define SENSTALK_JSON_STR_HRv			"HRv"
#define SENSTALK_JSON_STR_SDt			"SDt"
#define SENSTALK_JSON_STR_EDt			"EDt"
#define SENSTALK_JSON_STR_Sys			"Sys"
#define SENSTALK_JSON_STR_Cfgs			"Cfgs"
#define SENSTALK_JSON_STR_Vs			"Vs"
#define SENSTALK_JSON_STR_LCfgs			"LCfgs"

#define TASK_PROCESS_TYPE_SENSTALK	                (0)
#define TASK_PROCESS_TYPE_SENSLINK_LITE	            (5)
#define TASK_PROCESS_TYPE_IKW_UPLOAD				6
#define TASK_PROCESS_TYPE_IKW_ACK					7

#define MQTT_TID_IS_INTEGER


struct SENSTALK_JSON_CMD_STR
{	char *name;
	enum SENSTALK_MQTT_CMD_TYPE processType;
};

struct SENSTALK_JSON_SUB_CMD_STR
{	char *name;
	unsigned int subcmd;
	void *value;
	char unit;	//0: int, 1:float, 2:bool, 3:...
};

struct SENSTALK_JSON_WRITE_SUB_CMD_STR
{	char *name;
	unsigned int subcmd;
	char arrayPosition;
	int value;
	char unit;
};

extern void reQueueSenstalkCmd(int type, void *oldCmd, int waitTime, int callFromLine);
extern void initialIotpsTable(void);

extern void senstalkMqttRspProcess(SERVER_CMD_STRUCT *senstalkCmd);
extern void parserSenstalkMqttProtocol(SERV_RECV_CMD_CTX *senstalkRecvCmd);
extern int getLongValueByKeyFromObj(cJSON *cJsonSrc, char *key, uint64_t *val);
extern int getSringValueByKeyFromObj(cJSON *cJsonSrc, char *key, char **val);




#endif
