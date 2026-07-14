#ifndef __MQTT_PROTOCOOL_PROCESS_H__
#define __MQTT_PROTOCOOL_PROCESS_H__


#include "sensminiCfg.h"

extern char mqttPublishJson(int type, void *cmd, char *sendMsg, char waitPublishFinish, char sendAndWriteTagFlag);
extern void senstalkMqttEncoder(SERVER_CMD_STRUCT *senstalkCmd);
extern void parserMqttProtocol(SERV_RECV_CMD_CTX *senstalkRecvCmd);
extern void senstalkWriteConfig(SERVER_CMD_STRUCT *senstalkCmd);
extern void clearMqttSendMsg(CLOUD_SERVER_INSTANCE *serverInst);
extern void reQueueSenstalkCmd(int type, void *oldCmd, int waitTime, int callFromLine);

#endif