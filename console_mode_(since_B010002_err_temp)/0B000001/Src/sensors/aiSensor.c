#include "sensors/sensorApp.h"
#include "sensSystem.h"
#include "sensCommon.h"
#include "driver/ad7124Adc.h"
#include "systemTimer.h"
#include "sensors/sensorTask.h"
#include "physicalQuantity.h"
#include "powerCtrl.h"

//------------------------------------------------------------------------------
void initAiSensor(void)
{	AI_CONFIG *aiCfg = (AI_CONFIG *)&sysCfg->aiCfg;
	uint32_t adcChipId;
	
	/*aiCfg->enable = 1;
	aiCfg->differential = DIFFERENTIAL_MODE;	//for volt
	aiCfg->sensorType[0] = AI_TYPE_4_20MA;
	aiCfg->sensorType[1] = AI_TYPE_4_20MA;
	aiCfg->sensorType[2] = AI_TYPE_4_20MA;
	aiCfg->sensorType[3] = AI_TYPE_5V;
	aiCfg->sensorType[4] = AI_TYPE_5V;*/
	
	if(!aiCfg->enable)
		return;
	
	setAdcChipPwr(1);
	SENS_TIME_DELAY(100);
	openAdcSpi(&adcChipId, (uint8_t)aiCfg->differential);
	//setSensminiTimer((void *)sensorQ, SENSOR_Q_MSG_GET_AI_SENSOR, NULL, SENSMINI_TIMER_AI_SENSOR, AI_SENSOR_POLLONG_TIMEOUT, TIMER_MODE_TRIGGER);
}
//------------------------------------------------------------------------------
void aiSensorValueGet(void)
{	
#if ADC_USE_OLD_CIRCUIT
	int channels = 8;
#else
	int channels = 5;
#endif
	int idx;
	AI_CONFIG *aiCfg = (AI_CONFIG *)&sysCfg->aiCfg;
	
#if ADC_USE_OLD_CIRCUIT
	if(aiCfg->differential == DIFFERENTIAL_MODE)
		channels = 4;
#endif
	
	for(idx=0;idx<channels;idx++)
	{	
#if !ADC_USE_OLD_CIRCUIT
		if((aiCfg->differential == DIFFERENTIAL_MODE) && (idx == 4))
			break;
#endif
		sensPq->onboardPq[idx] = ad7124ReadChannelValue(idx);
		/* The effective range of 4-20mA is typically 4.0mA to 20.0mA, so if the value is outside this range, it may indicate an error. */
		if(aiCfg->sensorType[idx] == AI_TYPE_4_20MA && sensPq->onboardPq[idx] < 4.0f)
		{	sensPq->onboardPq[idx] = 4.0f;
		}
		else if(aiCfg->sensorType[idx] == AI_TYPE_0_20MA && sensPq->onboardPq[idx] < 0.0f)
		{	sensPq->onboardPq[idx] = 0.0f;
		}
		else if(aiCfg->sensorType[idx] == AI_TYPE_20MA_BIPOLAR && sensPq->onboardPq[idx] < -20.0f)
		{	sensPq->onboardPq[idx] = -20.0f;
		}

		if((aiCfg->sensorType[idx] == AI_TYPE_4_20MA || aiCfg->sensorType[idx] == AI_TYPE_0_20MA || \
				aiCfg->sensorType[idx] == AI_TYPE_20MA_BIPOLAR) && sensPq->onboardPq[idx] > 20.0f)
		{	sensPq->onboardPq[idx] = 20.0f;
		}
		putValToMapPqData(ON_BOARD_DEV, idx, sensPq->onboardPq[idx]);
	}
	setSensminiTimer((void *)sensorQ, SENSOR_Q_MSG_GET_AI_SENSOR, \
					NULL, SENSMINI_TIMER_AI_SENSOR, AI_SENSOR_POLLONG_TIMEOUT, TIMER_MODE_TRIGGER);
}
//------------------------------------------------------------------------------