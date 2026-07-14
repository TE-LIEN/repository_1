#ifndef __SERVER_PROTOCOL_H__
#define __SERVER_PROTOCOL_H__


#include "sensminiCfg.h"
#include "network/protocol/serverCmdProcessTask.h"
#include "network/protocol/serverCmdProcessApp.h"
#include "network/protocol/HttpClientProtocol.h"
#include "network/protocol/ioaProt.h"
#include "network/protocol/SenstalkProtocol.h"
#include "network/protocol/SenstalkMqttProtocol.h"
#include "network/protocol/MqttProtocolProcess.h"
#include "network/protocol/SenssCfgMqttProtocol.h"

#if SUPPORT_IOA_WEB_API
#include "network//protocol/ioaProt.h"
#endif

#endif