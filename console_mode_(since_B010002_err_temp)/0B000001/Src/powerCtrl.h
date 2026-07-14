#ifndef __POWER_CTRL_H__
#define __POWER_CTRL_H__

#include "sensminiCfg.h"

#define POWER_ON		1
#define POWER_OFF		0
#define nPOWER_ON		0
#define nPOWER_OFF		1

#define ON				1
#define OFF				0
#define nNO				0
#define nOFF			1

//#define RS485_DIRECT_OUT	1
//#define RS485_DIRECT_IN		0

extern void sysPwrOffCtrl(uint32_t offTimer, uint32_t nextWakeupTimestamp);
extern void enterPowerSavingMode(void);
extern void iotPwrCtrl(enum IOT_IF_SEL iotIfSel, uint8_t enable);
extern void setRs485Pwr(int channel, int onOff);
extern void setAdcChipPwr(int onOff);
extern void setOutputPower(int devPwrSrc, uint8_t onOff, const char *caller, int callerLine);
extern void doPwrOutSetting(void);
#endif

