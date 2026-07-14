#ifndef __NET_APP_H__
#define __NET_APP_H__

#include "sensminiCfg.h"
#include "network/ethernetService.h"
#include "network/dnsService.h"
#include "network/networkProcessTask.h"
#include "network/iotProcessTask.h"
#include "network/iotProcessApp.h"
#include "network/networkProcessApp.h"
#include "network/tcpTls.h"

#ifdef NET_LWIP
#include "lwipopts.h"
#include "netif/ethernet.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"

#include "lwip/dns.h"

//#ifdef SUPPORT_SNTP
#include "lwip/apps/sntp.h"
//#endif

#endif
#include "network/modem_ppp.h"

#ifdef PPP_USE_CMUX
#include "network/cmux.h"
#endif

#include "network/mqtt/mqtt_client.h"
#include "network/mqttclientTask.h"
#include "network/mqtt/mqttnet.h"
#include "network/mqtt/mqttconfig.h"

#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE
#include "mbedtls/certs.h"
#include "network/tcpTls.h"
#endif

#include "network/otaProcessApp.h"
#endif
