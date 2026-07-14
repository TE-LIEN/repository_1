#ifndef __AI_SENSOR_H__
#define __AI_SENSOR_H__

#include "sensminiCfg.h"

#define AI_SENSOR_POLLONG_TIMEOUT	1000

extern void initAiSensor(void);
extern void aiSensorValueGet(void);

#endif