#ifndef __OTA_PROCESS_APP_H__
#define __OTA_PROCESS_APP_H__


#include "sensminiCfg.h"



extern int otaStartProcess(HTTP_CONTEXT *httpCtx);
extern void stopNetworkOta(CLOUD_SERVER_INSTANCE *serverInst, char delTaskDirect);
extern void otaProcess(HTTP_RSP_INFO *httpRspInfo, CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx);
extern int triggerNextFota(CLOUD_SERVER_INSTANCE *serverInst);
#endif