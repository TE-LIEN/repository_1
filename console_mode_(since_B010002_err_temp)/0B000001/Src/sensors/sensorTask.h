#ifndef __SENSOR_TASK_H__
#define __SENSOR_TASK_H__

#include "sensminiCfg.h"
#include "sensors/sensorApp.h"


#define SENSOR_NUM_MESSAGES	32	//max queue size
#define SENSOR_Q_SIZE	SENSOR_NUM_MESSAGES * SENS_TASKQ_GRANM * sizeof(_mqx_max_type)
	
extern QueueHandle_t sensorQ;

extern void sensorTask(void *param);
extern void uvcCameraTask(void *param);

#endif
