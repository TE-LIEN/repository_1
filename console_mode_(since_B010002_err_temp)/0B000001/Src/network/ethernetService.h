#ifndef __ETHERNET_SERVICE_H__
#define __ETHERNET_SERVICE_H__

#include "sensminiCfg.h"
//#include "network/netApp.h"


extern int ethernetInit(void);
extern int setupEthernetComm(void);
extern void addDnsServerCfg(DNS_CONFIG *dnsCfg);
#endif