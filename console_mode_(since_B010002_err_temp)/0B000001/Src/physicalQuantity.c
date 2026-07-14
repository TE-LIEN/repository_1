#include "physicalQuantity.h"
#include "sensSystem.h"
#include <math.h>
#include "sensors/sensorApp.h"
#include "sensDev.h"

//NAN issue, in file option->C++->misc control add "-ffp-model=precise"

volatile SENS_PQ_STRUCT	*sensPq;

#define KALMAN_Q 0.001
#define KALMAN_R 0.005
//------------------------------------------------------------------------------
void addPqData(PQ_DATA_STRUCT *newPqData)
{	PQ_DATA_STRUCT *pqData;
	if(sensPq->pqData == NULL)
	{	sensPq->pqData = newPqData;
	}
	else
	{	pqData = sensPq->pqData;
		
		while(pqData->next)
		{	pqData = pqData->next;
		}
		pqData->next = newPqData;
	}
}
//------------------------------------------------------------------------------
void addPqCfg(PQ_CONFIG *pqCfg)
{	PQ_CONFIG *pqCfgTemp;
	
	if(sysCfg->pqCfgs == NULL)
		sysCfg->pqCfgs = pqCfg;
	else
	{	pqCfgTemp = sysCfg->pqCfgs;
		while(pqCfgTemp->next)
		{	pqCfgTemp = pqCfgTemp->next;
		}
		pqCfgTemp->next = pqCfg;
	}
}
//------------------------------------------------------------------------------
int getInfFromMapPqData(uint8_t devIdx, uint8_t measureIdx, uint8_t *rspDevIdx, uint8_t *rspMeasureIdx)
{	PQ_DATA_STRUCT *pqData;
	
	if(sensPq->pqData == NULL)
		return -1;
	for(pqData = sensPq->pqData;pqData;pqData=pqData->next)
	{	if((pqData->devIdx == devIdx) && (pqData->measureIdx == measureIdx))
		{	
		}
	}
	
	return 0;
}
//------------------------------------------------------------------------------
float kalmanFilter(KALMAN_STRUCT *kalman, float inData)
{	float kGain=0;
	kalman->preDataKalman = kalman->preDataKalman + KALMAN_Q;
	kGain = kalman->preDataKalman / (kalman->preDataKalman + KALMAN_R);
	
	inData = kalman->preData + (kGain * (inData - kalman->preData));
	kalman->preDataKalman = (1 - kGain) * kalman->preDataKalman;
	kalman->preData = inData;
	
	return inData;
}
//------------------------------------------------------------------------------
float standardDeviationFloat(float *arrays, uint16_t size, float *avg)
{	float average;
	float sum = 0;
	float powAvg = 0;
	uint32_t idx;
	
	for(idx=0;idx<size;idx++)
	{	sum += arrays[idx];
	}
	average = sum/size;
	for(idx=0;idx<size;idx++)
	{	powAvg += pow((arrays[idx] - average), 2);
	}
	
	powAvg = powAvg / size;
	powAvg = sqrt(powAvg);
	*avg = average;
	
	return powAvg;
}
//------------------------------------------------------------------------------
float stdFilter(ALOG_STD *stdAlog, float inData, float stdMultiple, int *sts)
{	float std;
	float avg;
	float validStdSum = 0;
	uint32_t validStdCount = 0;
	uint16_t idx;
	
	*sts = -1;
	
	stdAlog->array[stdAlog->arrayOffset++] = inData;
	if(stdAlog->arrayOffset >= stdAlog->arraySize)
		stdAlog->arrayOffset = 0;
	stdAlog->arrayIndex++;
	if(stdAlog->arrayIndex >= stdAlog->arraySize)
	{	std = standardDeviationFloat(stdAlog->array, stdAlog->arraySize, &avg);
		
		for(idx=0;idx<stdAlog->arraySize;idx++)
		{	if((stdAlog->array[idx] >= (avg - (std*stdMultiple))) && (stdAlog->array[idx] <= (avg + (std*stdMultiple))))
			{	validStdCount++;
				validStdSum += stdAlog->array[idx];
			}
		}
		avg = validStdSum / validStdCount;
		*sts = 0;
	}
	return avg;
}
//------------------------------------------------------------------------------
void pqFilter(PQ_DATA_STRUCT *pqData, float val)
{	if(pqData->filterType & ALGORITHM_KALMAN)
	{
	}
	if(pqData->filterType & ALGORITHM_MOVING)
	{	
	}
	else if(pqData->filterType & ALGORITHM_STD)
	{
	}
}
//------------------------------------------------------------------------------
void putValToMapPqData(int16_t devIdx, int16_t measureIdx, float val)
{	PQ_DATA_STRUCT *pqData;
	SYS_SEM	*sysSem;
	
	if(sensPq->pqData == NULL)
		return;
	sysSem = (SYS_SEM *)&sensSys->sysSem;
	SENS_SEM_LOCK(sysSem->pqLock);
	for(pqData = sensPq->pqData;pqData;pqData=pqData->next)
	{	if((pqData->devIdx == devIdx) && (pqData->measureIdx == measureIdx))
		{	if(pqData->filterType == ALGORITHM_NONE)
			{	pqData->pqVal = val;
				pqData->pqIsReady = 1;
			}
			else
			{	pqFilter(pqData, val);
			}
		}
	}
	SENS_SEM_UNLOCK(sysSem->pqLock);
}
//------------------------------------------------------------------------------
void checkAndSetOnboardPqIsRdy(void)
{	int idx;
	/*int idx;
	for(idx=0;idx<ON_BOARD_PQ_MAX;idx++)
	{	if((idx != ON_BOARD_PQ_TEMPERATURE) && 
			 (idx != ON_BOARD_PQ_BATTERY_VOLT) && (idx != ON_BOARD_PQ_EXT_VOLT) && (idx != ON_BOARD_PQ_SOLAR_CHARGE_CURRENT) && 
			 (idx != ON_BOARD_PQ_SOLAR_CHARGE_STS) &&
			 (idx != ON_BOARD_PQ_OSA_RADAR_VALUE) && (idx != ON_BOARD_PQ_OSA_RADAR_FILTER_VALUE))
			putValToMapPqData(ON_BOARD_DEV, idx, sensPq->onboardPq[idx]);
	}*/
	
	for(idx=0;idx<DIQUANTITY;idx++)
		putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_DI_COUNTER_0+idx, sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_0+idx]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_DO24_STS, sensPq->onboardPq[ON_BOARD_PQ_DO24_STS]);
	
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_DO_STS_0, sensPq->onboardPq[ON_BOARD_PQ_DO_STS_0]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_DO_STS_1, sensPq->onboardPq[ON_BOARD_PQ_DO_STS_1]);
	
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_ERROR_CODE, sensPq->onboardPq[ON_BOARD_PQ_ERROR_CODE]);
	//putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_REBOOT_TIMES, sensPq->onboardPq[ON_BOARD_PQ_REBOOT_TIMES]);
#ifndef SUPPORT_AGPS
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LATITUDE, sensPq->onboardPq[ON_BOARD_PQ_LATITUDE]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LONGITUDE, sensPq->onboardPq[ON_BOARD_PQ_LONGITUDE]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_GPS_STATUS, sensPq->onboardPq[ON_BOARD_PQ_GPS_STATUS]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_GPS_SPEED, sensPq->onboardPq[ON_BOARD_PQ_GPS_SPEED]);
#endif
}
//------------------------------------------------------------------------------
void addFormula(FORMULA_STRUCT *newFormula)
{	FORMULA_STRUCT *formula;
	if(sysCfg->formula == NULL)
	{	sysCfg->formula = newFormula;
	}
	else
	{	formula = sysCfg->formula;
		
		while(formula->next)
		{	formula = formula->next;
		}
		formula->next = newFormula;
	}
}
//------------------------------------------------------------------------------
void formulaCompute(PQ_DATA_STRUCT *pqData)
{	FORMULA_STRUCT	*formula;
	
	if((pqData->formulaIdx >= 0) && (!isnan(pqData->pqVal)))	//-1 no formula
	{	formula = sysCfg->formula;
		while(formula)
		{	if(formula->formulaIdx == pqData->formulaIdx)
				break;
			formula = formula->next;
		}
		if(formula)
		{	if(formula->type == 0)
				sensPq->pqVal[pqData->pqIdx] = (formula->a * pqData->pqVal + formula->b) * pqData->pqVal + formula->c;
			else
			{	if(((pqData->pqVal < 0) && abs((formula->b - (int)formula->b) > 0)) || (pqData->pqVal == 0 && formula->b < 0))
					sensPq->pqVal[pqData->pqIdx] = 0;
				else
					sensPq->pqVal[pqData->pqIdx] = formula->a * pow(pqData->pqVal, formula->b);
			}
			return;
		}
	}
	sensPq->pqVal[pqData->pqIdx] = pqData->pqVal;
}
//------------------------------------------------------------------------------
void pqDataToCloudPq(void)
{	PQ_DATA_STRUCT *pqData;
	SYS_SEM	*sysSem;
	int allocPqMapSize;
	//int idx;
	
	if(sensPq->pqData == NULL)
		return;
	allocPqMapSize = sensPq->pqNumber/32;
	if(sensPq->pqNumber%32)
		allocPqMapSize++;
	sysSem = (SYS_SEM *)&sensSys->sysSem;
	SENS_SEM_LOCK(sysSem->pqLock);
	for(pqData = sensPq->pqData;pqData;pqData=pqData->next)
	{	if(pqData->pqIsReady)
		{	formulaCompute(pqData);
			setModbusInputRegisterFloat((pqData->pqIdx * 2), sensPq->pqVal[pqData->pqIdx]);
			pqData->pqIsReady = 0;
			sensPq->pqGetMap[pqData->pqIdx / 32] |= (1 << (pqData->pqIdx % 32));
		}
	}
	SENS_SEM_UNLOCK(sysSem->pqLock);
}
//------------------------------------------------------------------------------
#if TEST_N_REMOVE
void alertCheck(void)
{	PQ_DATA_STRUCT *pqData;
	PARAM_ALERT_INFO *alertInfo;
	ALERT_MANAGER *alertManager;
	uint8_t alertConditionMeet = 0;
	
	if(sysCtrl->pwrManager.currPsm == PARAM_PSM_NONE)
		return;
	
	if(sysParams->alertInfo == NULL)
		return;
	alertInfo = sysParams->alertInfo;
	if(!alertInfo->enable)
		return;
	
	alertManager = (ALERT_MANAGER *)&sysCtrl->alertManager;
	if(alertManager->alertPdData == NULL)
	{	for(pqData = sensPq->pqData;pqData;pqData=pqData->next)
		{	if(pqData->pqIdx == alertInfo->alertPq)
			{	alertManager->alertPdData = pqData;
				break;
			}
		}
	}
	
	if(alertManager->alertPdData == NULL)	//can not find
	{	return;
	}
	pqData = (PQ_DATA_STRUCT *)alertManager->alertPdData;
	
	if(sensPq->pqGetMap[pqData->pqIdx / 32] & (1 << (pqData->pqIdx % 32)))
	{	if(sysParams->alertInfo->method == PARAM_ALERT_METHOD_GREATER)
		{	if(sensPq->pqVal[pqData->pqIdx] > sysParams->alertInfo->threshold)
				alertConditionMeet = 1;
		}
		else if(sysParams->alertInfo->method == PARAM_ALERT_METHOD_GREATER_EQUAL)
		{	if(sensPq->pqVal[pqData->pqIdx] >= sysParams->alertInfo->threshold)
				alertConditionMeet = 1;
		}
		else if(sysParams->alertInfo->method == PARAM_ALERT_METHOD_EQUAL)
		{	if(sensPq->pqVal[pqData->pqIdx] == sysParams->alertInfo->threshold)
				alertConditionMeet = 1;
		}
		else if(sysParams->alertInfo->method == PARAM_ALERT_METHOD_LESS)
		{	if(sensPq->pqVal[pqData->pqIdx] < sysParams->alertInfo->threshold)
				alertConditionMeet = 1;
		}
		else if(sysParams->alertInfo->method == PARAM_ALERT_METHOD_LESS_EQUAL)
		{	if(sensPq->pqVal[pqData->pqIdx] <= sysParams->alertInfo->threshold)
				alertConditionMeet = 1;
		}
		
		alertProcess(alertConditionMeet);
	}
}
#endif
//------------------------------------------------------------------------------
void initPq(void)
{	int idx;
	PQ_DATA_STRUCT *pqData;
	SYS_SEM	*sysSem;
	int allocPqMapSize;
	PQ_CONFIG *pqCfg;
	
	if(!sensPq->pqNumber)
		return;
	
	sysSem = (SYS_SEM *)&sensSys->sysSem;
	if(SENS_SEM_INIT(sysSem->pqLock, 1) != MQX_OK)
	{	return;
	}
	
	allocPqMapSize = sensPq->pqNumber/32;
	if(sensPq->pqNumber%32)
		allocPqMapSize++;
	
	sensPq->pqGetMap = SENS_MEM_ZALLOC(allocPqMapSize * sizeof(uint32_t));
	sensPq->pqGetMapTemp = SENS_MEM_ZALLOC(allocPqMapSize * sizeof(uint32_t));
	
	sensPq->pqVal = SENS_MEM_ALLOC(sizeof(float) * sensPq->pqNumber);
	memset((void *)sensPq->pqVal, 0xFF, sizeof(float) * sensPq->pqNumber);
	sensPq->bytesPerRec = sensPq->pqNumber * sizeof(float);
	for(idx=0;idx<ON_BOARD_PQ_MAX;idx++)
	{	if((idx< ON_BOARD_PQ_DI_COUNTER_0) || (idx > ON_BOARD_PQ_EMPTY_IDX_31))	//skip DI counter
		sensPq->onboardPq[idx] = SENS_SET_VAL_NAN();
	}
	
	for(idx=0;idx<((DIQUANTITY > 4)? 4:DIQUANTITY);idx++)
	{	sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_0 + idx] = 0;
		sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_0 + idx] = getVBatRegValue(RTC_SPARE_REG_ITEM_DI0_COUNTER + idx);
		setVBatRegValue(RTC_SPARE_REG_ITEM_DI0_COUNTER + idx, 0);
	}
	
	sensPq->onboardPq[ON_BOARD_PQ_REBOOT_TIMES] = sysRecSlotFlag.rebootCnt;
	//sensPq->pqData = SENS_MEM_ALLOC(sizeof(PQ_DATA_STRUCT) * sensPq->pqNumber);
	//for test use, must read param file and set it
	
	//idx = 0;
	for(pqCfg = sysCfg->pqCfgs;pqCfg;pqCfg=pqCfg->next)
	{	pqData = (PQ_DATA_STRUCT *)SENS_MEM_ZALLOC(sizeof(PQ_DATA_STRUCT));
		pqData->pqVal = SENS_SET_VAL_NAN();
		pqData->pqIdx = pqCfg->pqIdx;
		pqData->devIdx = pqCfg->pqSrc.devNo;
		pqData->formulaIdx = pqCfg->formulaId;
		pqData->filterType = pqCfg->filterType;
		pqData->measureIdx = pqCfg->pqSrc.devChanNo;
		if(pqCfg->alias[0])
		{	pqData->serv1Alias = SENS_MEM_ZALLOC(strlen(pqCfg->alias[0]) + 1);
			strcpy(pqData->serv1Alias, pqCfg->alias[0]);
		}
		if(pqCfg->alias[1])
		{	pqData->serv2Alias = SENS_MEM_ZALLOC(strlen(pqCfg->alias[1]) + 1);
			strcpy(pqData->serv2Alias, pqCfg->alias[1]);
		}
		addPqData(pqData);
		//idx++;
	}
	
	for(idx=0;idx<DIQUANTITY;idx++)
		putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_DI_COUNTER_0+idx, sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_0+idx]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_REBOOT_TIMES, sensPq->onboardPq[ON_BOARD_PQ_REBOOT_TIMES]);
	
#if !defined PARAM_FORM_FILE && 0
	pqData = (PQ_DATA_STRUCT *)SENS_MEM_ZALLOC(sizeof(PQ_DATA_STRUCT));
	pqData->pqVal = -0.0f;
	pqData->pqIdx = 0;
	pqData->devIdx = 0;
	pqData->formulaIdx = -1;
	pqData->filterType = ALGORITHM_NONE;
	pqData->measureIdx = ON_BOARD_PQ_TEMPERATURE;
	addPqData(pqData);
	
	pqData = (PQ_DATA_STRUCT *)SENS_MEM_ZALLOC(sizeof(PQ_DATA_STRUCT));
	pqData->pqVal = -0.0f;
	pqData->pqIdx = 1;
	pqData->devIdx = 0;
	pqData->formulaIdx = -1;
	pqData->filterType = ALGORITHM_NONE;
	pqData->measureIdx = ON_BOARD_PQ_EXT_VOLT;
	addPqData(pqData);
	
	pqData = (PQ_DATA_STRUCT *)SENS_MEM_ZALLOC(sizeof(PQ_DATA_STRUCT));
	pqData->pqVal = -0.0f;
	pqData->pqIdx = 2;
	pqData->devIdx = 0;
	pqData->formulaIdx = -1;
	pqData->filterType = ALGORITHM_NONE;
	pqData->measureIdx = ON_BOARD_PQ_NB_SIGNAL_STRENGTH;
	addPqData(pqData);
#endif
}
//------------------------------------------------------------------------------
PQ_CONFIG *getPqConfig(int pqIdx)
{	PQ_CONFIG *pqCfg;
	
	for(pqCfg=sysCfg->pqCfgs;pqCfg;pqCfg=pqCfg->next)
	{	if(pqCfg->pqIdx == pqIdx)
			break;
	}
	return pqCfg;
}
//------------------------------------------------------------------------------
int getTotalPq(void)
{	int pqNumber = 0;
	PQ_CONFIG *pqCfg = sysCfg->pqCfgs;
	while(pqCfg)
	{	pqNumber++;
		pqCfg = pqCfg->next;
	}
	
	return pqNumber;
}
//------------------------------------------------------------------------------
float standardDeviation(void *array, int count, float *average1, enum STD_ARRAY_TYPE stdArrType)
{	float average;
	float sum=0;
	int idx;
	float powAverage=0;
	float *floatArray;
	int *s32Array;
	uint32_t *u32Array;
	uint16_t *u16Array;
	int16_t	*s16Array;
	uint8_t	*u8Array;
	int8_t	*s8Array;
	char displayStr[20];
	
	
	if(stdArrType == STD_ARRAY_TYPE_FLOAT)
  { floatArray = (float *)array;	
  	//debugLock();
  	//dbg_msg1("%s", "STD Val: [");
    for(idx=0;idx<count;idx++)	
    {	sum += floatArray[idx];
    	//if(idx == (count-1))
    	//	dbg_msg1("%s]\r\n", my_ftoa(displayStr, floatArray[idx], 3));
    	//else
    	//	dbg_msg1("%s, ", my_ftoa(displayStr, floatArray[idx], 3));
    }
    //debugUnlock();
    average = sum / count;
    for(idx=0;idx<count;idx++)
    	powAverage += pow((floatArray[idx]-average), 2);
  }
  else if(stdArrType == STD_ARRAY_TYPE_INT32)
  { s32Array = (int *)array;			
    for(idx=0;idx<count;idx++)	
      sum += s32Array[idx];		
    average = sum / count;
    for(idx=0;idx<count;idx++)
    	powAverage += pow((s32Array[idx]-average), 2);
  }
  else if(stdArrType == STD_ARRAY_TYPE_UINT32)
  { u32Array = (uint32_t *)array;	
    for(idx=0;idx<count;idx++)	
      sum += u32Array[idx];
    average = sum / count;
    for(idx=0;idx<count;idx++)
    	powAverage += pow((u32Array[idx]-average), 2);
  }
  else if(stdArrType == STD_ARRAY_TYPE_INT16)
  { s16Array = (int16_t *)array;	
    for(idx=0;idx<count;idx++)	
      sum += s16Array[idx];
    average = sum / count;
    for(idx=0;idx<count;idx++)
    	powAverage += pow((s16Array[idx]-average), 2);
  }
  else if(stdArrType == STD_ARRAY_TYPE_UINT16)
  { u16Array = (uint16_t *)array;	
    for(idx=0;idx<count;idx++)	
      sum += u16Array[idx];
    average = sum / count;
    for(idx=0;idx<count;idx++)
    	powAverage += pow((u16Array[idx]-average), 2);
  }
  else if(stdArrType == STD_ARRAY_TYPE_INT8)
  { s8Array = (int8_t *)array;	
    for(idx=0;idx<count;idx++)	
      sum += s8Array[idx];
    average = sum / count;
    for(idx=0;idx<count;idx++)
    	powAverage += pow((s8Array[idx]-average), 2);
  }
  else if(stdArrType == STD_ARRAY_TYPE_UINT8)
  { u8Array = (uint8_t *)array;	
    for(idx=0;idx<count;idx++)	
      sum += u8Array[idx];
    average = sum / count;
    for(idx=0;idx<count;idx++)
    	powAverage += pow((u8Array[idx]-average), 2);
  }

	/*for(idx=0;idx<count;idx++)
	{	sum += array[idx];
	}

	average = sum / count;
	for(idx=0;idx<count;idx++)
	{	powAverage += pow((array[idx]-average), 2);
	}
  */

	powAverage /= count;
	powAverage = sqrt(powAverage);
	*average1 = average;
	return powAverage;
}
//------------------------------------------------------------------------------
int getHwPqWithFilter(SENSOR_HW_PQ_STRUCT *hwPqInf)
{	SENSOR_ALGO_STRUCT *prevFilterAlgo;
	float sigma, average;
	int idx, stdIdx=0;
	char displayStr[30];
	
	
	if(hwPqInf->prevFilterAlgo && hwPqInf->isRawHwPqReady && !hwPqInf->emptyPqError && !hwPqInf->isHwFilterPqRdy)
	{	if(!isnan(hwPqInf->rawPqVal))
		{	hwPqInf->isRawHwPqReady = 0;
			hwPqInf->seqNanCnt = 0;
			prevFilterAlgo = hwPqInf->prevFilterAlgo;
			prevFilterAlgo->valArray[prevFilterAlgo->valPos++] = hwPqInf->rawPqVal;
			if(prevFilterAlgo->valPos >= prevFilterAlgo->maxCount)
				prevFilterAlgo->valPos = 0;
			prevFilterAlgo->valCount++;
		
			if(prevFilterAlgo->valCount >= prevFilterAlgo->maxCount)
			{	sigma = standardDeviation(prevFilterAlgo->valArray, prevFilterAlgo->maxCount, &average, STD_ARRAY_TYPE_FLOAT);
				for(idx=0;idx<prevFilterAlgo->maxCount;idx++)
				{	if((prevFilterAlgo->valArray[idx] <= (average + (1.0*sigma))) && (prevFilterAlgo->valArray[idx] >= (average - (1.0*sigma))))
					{	prevFilterAlgo->stdValArray[stdIdx++] = prevFilterAlgo->valArray[idx];
					}
				}
				dbg_msg("STD SIGMA:%s, stdIdx:%d\r\n", my_ftoa(displayStr, sigma, 3), stdIdx);
				hwPqInf->filterPqVal = 0;
				for(idx=0;idx<stdIdx;idx++)
					hwPqInf->filterPqVal += prevFilterAlgo->stdValArray[idx];
				hwPqInf->filterPqVal /= stdIdx;
				dbg_msg("STD Final Val:%s\r\n", my_ftoa(displayStr, hwPqInf->filterPqVal, 3));
				prevFilterAlgo->valCount = 0;
				prevFilterAlgo->valPos = 0;
				hwPqInf->isHwFilterPqRdy = 1;
			}
		}
		else
		{	hwPqInf->seqNanCnt++;
			if(hwPqInf->seqNanCnt >= prevFilterAlgo->maxCount)
			{	hwPqInf->isRawHwPqReady = 0;
				hwPqInf->seqNanCnt = 0;
				hwPqInf->filterPqVal = hwPqInf->rawPqVal;
				hwPqInf->isHwFilterPqRdy = 1;
			}
		}
	}
	else if((hwPqInf->prevFilterAlgo == NULL) && hwPqInf->isRawHwPqReady && !hwPqInf->isHwFilterPqRdy)
	{	hwPqInf->isRawHwPqReady = 0;
		hwPqInf->filterPqVal = hwPqInf->rawPqVal;
		hwPqInf->isHwFilterPqRdy = 1;
	}
	return 0;
}
//------------------------------------------------------------------------------