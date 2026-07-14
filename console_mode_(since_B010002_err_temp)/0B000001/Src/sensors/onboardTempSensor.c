#include "sensors/sensorApp.h"
#include "driver/i2cDrv.h"
#include "driver/tcn75a.h"
#include "systemTimer.h"
#include "sensors/sensorTask.h"

#define TEMP_9_BIT_RESOLUTION	0
#define TEMP_10_BIT_RESOLUTION	1
#define TEMP_11_BIT_RESOLUTION	2
#define TEMP_12_BIT_RESOLUTION	3

#define TCN75A_SHUTDOWN(x)			x << 0
#define TCN75A_COMP_INT(x)			x << 1
#define TCN75A_ALERT_POLARITY(x)	x << 2
#define TCN75A_FAULT_QUEUE(x)		x << 3
#define TCN75A_RESOLUTION(x)		x << 5
#define TCN75A_ONE_SHOT(x)			x << 7

int initOnboardTemperatureSensor(void)
{	uint8_t tempCfg;
	i2cInit(I2C2_ID, 100000, 32);
	
	tempCfg = TCN75A_SHUTDOWN(0) | TCN75A_RESOLUTION(TEMP_12_BIT_RESOLUTION);
	tcn75aConfig(tempCfg);
	//setSensminiTimer((void *)sensorQ, SENSOR_Q_MSG_GET_NORMAL_SENSOR_VALUE, NULL, SENSMINI_TIMER_I2C_THERMOSENSOR, THERMO_SENSOR_POLLONG_TIMEOUT, TIMER_MODE_LOOPING);
	return 0;
}

