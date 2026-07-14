#ifndef __SERVER_CMD_PROCESS_APP_H__
#define __SERVER_CMD_PROCESS_APP_H__

#include "sensminiCfg.h"

extern void prepareProtocolPayloadToServer(SERVER_CMD_STRUCT *senstalkCmd);
extern void parserServerPayload(SERV_RECV_CMD_CTX *senstalkRecvCmd);

#endif
