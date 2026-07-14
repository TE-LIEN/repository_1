#ifndef __TCN75A_H__
#define __TCN75A_H__

#include "sensminiCfg.h"
#include "driver/i2cDrv.h"



extern float tcn75aReadTemperature(void);
extern void tcn75aConfig(uint8_t confData);
extern void tcn75aTLowSet(float confData);
extern void tcn75aTHighSet(float confData);

#endif // __TCN75A_H__