#ifndef __S35710_Wakeup_Timer_H__
#define __S35710_Wakeup_Timer_H__

#include "sensminiCfg.h"
#include "driver/i2cDrv.h"

#define S35710_SLA	0x32


extern void powerManagerChipCtrl(enum PWR_MNG_IC_ACTION_ID actionId, uint32_t time);



#endif