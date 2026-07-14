#ifndef __IOT_PROCESS_APP_H__
#define __IOT_PROCESS_APP_H__

#include "sensminiCfg.h"

extern IOT_SYSTEM iotSys;


extern void displayCrrentSendAtCmd(uint8_t send, char *str, char const *func, int line);
extern int setupMobileComm(void);
extern int initLsIo(NET_INSTANCE *netInst);
extern int initialHsIo(NET_INSTANCE *netInst);
extern int iotMobileInit(MOBILE_INSTANCE *wlInst);
extern int setPppMode(MOBILE_INSTANCE *wlInst);
extern int mobileDeinit(MOBILE_INSTANCE	*wlInst);

extern int extendedPacketSwitchedNetworkRegistrationStatus(MOBILE_INSTANCE *wlInst);
extern int getSignalQuality(MOBILE_INSTANCE *wlInst);
extern int getClock(MOBILE_INSTANCE *wlInst);
#endif
