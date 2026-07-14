#include "sensors/sensorApp.h"
#include "ini2.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "physicalQuantity.h"
#include "sensDev.h"
#include "sdCardTask.h"
#include <math.h>
#include "peripheral/rs485.h"
#include "powerCtrl.h"

#define NEW_DUAL_RADAR_ALGORITHM	1

#define TEST_METHOD_1111	0	//check zoom in info

#define DUAL_RADAR_REC_FILENAME	"dualRadarRec.log"

//-----Radar detecotr command---------------------------------------------------
#define START_Tx_str           "flStart 10 10\r\n"
#define SCAN_Tx_str            "anglePeak 237 -5 5 4 9\r\n"
#define REFINE_Tx_str          "rangeRefined 240 9 -2\r\n"

#define AR77_RADAR_SET_X				"stx 2\r"
//#define RADAR_START_str           "ar 1 2 0\r"
#define RADAR_START_str           "ar 1 2 3\r"
#define RADAR_HIGH_RESOLUTION_START_str	"ar 1 1 3\r"
#define RADAR_END_str             "p\r"

#define MAX_MOVING_AVERAGE_SIZE			10
//------------------------------------------------------------------------------
static int isFastScanFrameReceiveDone(char *buf, int len)
{	if((len > (strlen(RADAR_RX_FAST_SCAN_FRAME_START_TAG) + 4)) && strstr(buf, RADAR_RX_FAST_SCAN_FRAME_START_TAG))
	{	RADAR_FAST_SCAN_FRAME *radarRxFrame;
		char *frameTag = strstr(buf, RADAR_RX_FAST_SCAN_FRAME_START_TAG);
		radarRxFrame = (RADAR_FAST_SCAN_FRAME *)frameTag;
    if(len == (radarRxFrame->packetLength+2))
		{	if(!strncmp(&buf[len-2], RADAR_RX_FRAME_END_TAG, 2))
			{	return 1;
			}
			else
				return 2;
		}
	}
	return 0;
}
//------------------------------------------------------------------------------
int isNormalFrameReceiveDone(char *buf, int len, uint16_t data_length, char radarType)
{	if(radarType == 1)	//AVDS
	{	char *frameTagPos;
		if((len > (strlen(RADAR_RX_FRAME_START_TAG) + 4)) && (frameTagPos = strstr(buf, RADAR_RX_FRAME_START_TAG)))
		{	RADAR_NORMAL_FRAME *radarRxFrame;
			radarRxFrame = (RADAR_NORMAL_FRAME *)frameTagPos;
			if(len >= (radarRxFrame->packetLength + 2))
			{	if(!strncmp((char *)radarRxFrame->frameEnd, RADAR_RX_FRAME_END_TAG, 2))
					return 1;
				else
					return 2;
			}
		}
		return 0;
	}
	else
	{	if(data_length == len && len >= 60)                                                        // end of line
  		return 1;
    return 0;
	}
}
//------------------------------------------------------------------------------
void checkAndSetRadarRecInfo(AVDS_RADAR_INSTANCE *avdsInst, float tiltDistance)
{	AVDS_RADAR_CONTEXT *avdsCtx = avdsInst->avdsCtx;
	AVDS_RADAR_CONFIG	*avdsCfg = (AVDS_RADAR_CONFIG	*)avdsInst->avdsCfgPtr;
	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	DUAL_RADAR_REC_STRUCT	*dualRadarRecStruct;
	int rlen;
	int readRtyCount=0;
	char newRec=0;
	
	
	if(!avdsCtx->dualRadarRecBufIsValid)
	{	avdsCtx->dualRadarRecBufIsValid = 1;
		if(avdsCtx->dualRadarRecStruct == NULL)
			avdsCtx->dualRadarRecStruct = SENS_MEM_ZALLOC(sizeof(DUAL_RADAR_REC_STRUCT));
		dualRadarRecStruct = avdsCtx->dualRadarRecStruct;
		if(!chkFileExist(DUAL_RADAR_REC_FILENAME))
		{	
SET_NEW_RECORD:
			dbg_msg("%s", "[AVDS] set new record\r\n");
			dualRadarRecStruct->tag = 0x55AA;
			dualRadarRecStruct->tiltMeasureDist = 0;
			if(avdsCfg->mode == AVDS_MODE_SINGLE_TILT)
			{	dualRadarRecStruct->baseIsVertical = 0;
				dualRadarRecStruct->verticalMeasureDist = avdsCtx->waterLevel;
			}
			else
			{	dualRadarRecStruct->baseIsVertical = 1;
				dualRadarRecStruct->verticalMeasureDist = avdsCtx->verticalRadarMeasureDistance;
			}
		}
		else
		{	sdReadFile(DUAL_RADAR_REC_FILENAME, (char *)dualRadarRecStruct, sizeof(DUAL_RADAR_REC_STRUCT), 0, NORMAL_READ_MODE);
			if(dualRadarRecStruct->tag != 0x55AA)
			{	memset(dualRadarRecStruct, 0, sizeof(DUAL_RADAR_REC_STRUCT));
				goto SET_NEW_RECORD;
			}
		}
	}
	dualRadarRecStruct = avdsCtx->dualRadarRecStruct;
	dualRadarRecStruct->tag = 0x55AA;
	
	if(!avdsCtx->activeIdx)
	{	if(avdsCtx->verticalRadarMeasureDistance > dualRadarRecStruct->verticalMeasureDist)	//drop
		{	if((avdsCtx->verticalRadarMeasureDistance - dualRadarRecStruct->verticalMeasureDist) > avdsCtx->dropValue)	//drop 20cm for 10 minutes
			{	dbg_msg("[AVDS] water drop down %d mm, use old rec:%d, new measure:%d\r\n", avdsCtx->dropValue, dualRadarRecStruct->verticalMeasureDist, avdsCtx->verticalRadarMeasureDistance);
				avdsCtx->verticalRadarMeasureDistance = dualRadarRecStruct->verticalMeasureDist;
			}
		}
		else
		{	if((dualRadarRecStruct->verticalMeasureDist - avdsCtx->verticalRadarMeasureDistance) > avdsCtx->dropValue)	//up 20cm for 10 minutes
			{	//avdsCtx->verticalRadarMeasureDistance = dualRadarRecStruct->verticalMeasureDist;
				dbg_msg("[AVDS] water drop UP %d mm, old measure:%d, new measure: %d\r\n", avdsCtx->dropValue, dualRadarRecStruct->verticalMeasureDist, avdsCtx->verticalRadarMeasureDistance);
			}
			/*if(gCheckMode == 0)
			{	if((dualRadarRecStruct->verticalMeasureDist - avdsCtx->verticalRadarMeasureDistance) > avdsCtx->dropValue)	//up 20cm for 10 minutes
				{	avdsCtx->verticalRadarMeasureDistance = dualRadarRecStruct->verticalMeasureDist;
				}
			}
			else //if(gCheckMode == 1)
			{	dbg_msg("measure not drop, vertical dist:%d, rec is %d\r\n", (int)avdsCtx->verticalRadarMeasureDistance, (int)dualRadarRecStruct->verticalMeasureDist);
				dualRadarRecStruct->baseIsVertical = 1;
			}*/
		}
		dualRadarRecStruct->verticalMeasureDist = avdsCtx->verticalRadarMeasureDistance;
	}
	else
	{	dualRadarRecStruct->tiltMeasureDist = (int)tiltDistance;
		/*if(gCheckMode == 1)
		{	int verticalTemp = (int)(dualRadarRecStruct->tiltMeasureDist * cos((float)avdsCtx->tiltAngle*3.14159/180.0)) - avdsCfg->distOf2Radar;
			if(verticalTemp > dualRadarRecStruct->verticalMeasureDist)	//drop
			{	dualRadarRecStruct->verticalMeasureDist = verticalTemp;
			}
		}*/
	}
	sdWriteFile(DUAL_RADAR_REC_FILENAME, (char *)dualRadarRecStruct, sizeof(DUAL_RADAR_REC_STRUCT), 0, 0);
}
//------------------------------------------------------------------------------
static SENSOR_HW_PQ_STRUCT *createHwPqInf(int pqCnt, uint32_t createPrevFilter)
{	SENSOR_HW_PQ_STRUCT		*hwPqInf;
	SENSOR_HW_PQ_STRUCT		*hwPqInfTemp;
	SENSOR_HW_PQ_STRUCT		*topHwPqInf = NULL;
	SENSOR_ALGO_STRUCT		*prevFilterAlgo;
  int pqIdx;
	
	for(pqIdx=0;pqIdx<pqCnt;pqIdx++)
	{	hwPqInf = SENS_MEM_ZALLOC(sizeof(SENSOR_HW_PQ_STRUCT));
		hwPqInf->hwPqIdx = pqIdx;
		hwPqInf->pqVal = SENS_SET_VAL_NAN();
		if(createPrevFilter & (1<<pqIdx))
		{	prevFilterAlgo = SENS_MEM_ZALLOC(sizeof(SENSOR_ALGO_STRUCT));
			prevFilterAlgo->maxCount = 20;
			prevFilterAlgo->valArray = SENS_MEM_ZALLOC(sizeof(float) * prevFilterAlgo->maxCount);
			prevFilterAlgo->stdValArray = SENS_MEM_ZALLOC(sizeof(float) * prevFilterAlgo->maxCount);
			hwPqInf->prevFilterAlgo = prevFilterAlgo;
		}
		if(topHwPqInf == NULL)
			topHwPqInf = hwPqInf;
		else
		{	hwPqInfTemp = topHwPqInf;
			while(hwPqInfTemp->next)
			{	hwPqInfTemp = hwPqInfTemp->next;
			}
			hwPqInfTemp->next = hwPqInf;
		}
	}
	
	return topHwPqInf;
}
//------------------------------------------------------------------------------
static void setRadarHwPq(SENSOR_HW_PQ_STRUCT *topHwPqInf, int pqIdx, float value)
{	SENSOR_HW_PQ_STRUCT	*hwPqInf;
	
	for(hwPqInf=topHwPqInf;hwPqInf;hwPqInf=hwPqInf->next)
	{	if(hwPqInf->hwPqIdx == pqIdx)
		{	hwPqInf->rawPqVal = value;
			hwPqInf->isRawHwPqReady = 1;
			
			getHwPqWithFilter(hwPqInf);
			if(hwPqInf->isHwFilterPqRdy && !hwPqInf->isHwPqReady)
			{	hwPqInf->pqVal = hwPqInf->filterPqVal;
				hwPqInf->isHwPqReady = 1;
			}
			break;
		}
	}
}
//------------------------------------------------------------------------------
static void avdsIdleProcess(AVDS_RADAR_INSTANCE *avdsInst)
{	AVDS_RADAR_CONTEXT *avdsCtx = avdsInst->avdsCtx;
	AVDS_RADAR_CONFIG	*avdsCfg = (AVDS_RADAR_CONFIG	*)avdsInst->avdsCfgPtr;
	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	SENSOR_HW_PQ_STRUCT	*hwPqInf;
	int prevFilter = 0;
	int pqCnt;
	
	processCtx->upperCtx = (void *)avdsCtx;
	
	processCtx->recvBuf = SENS_MEM_ZALLOC(RSP_BUF_SIZE_RADAR);
	processCtx->pwrOnDelayTimeout = 4000;
	processCtx->isAvds = 1;
	
	if(avdsCfg->unit == AVDS_UNIT_MM)
		avdsCtx->radarUnit = 1;
	else
		avdsCtx->radarUnit = 10;
	//avdsCtx->dropValue = 20 * avdsCtx->radarUnit;
	avdsCtx->dropValue = 200;
	avdsCtx->valueInterval = 200;
	
	avdsCtx->tiltAngle = avdsCfg->tiltAngle;
	avdsCtx->tiltAngleCalibration = avdsCfg->tiltAngleCalibration;
	avdsCtx->distanceOfTwoRadar = avdsCfg->distOf2Radar * avdsCtx->radarUnit;
	avdsCtx->waterLevel = avdsCfg->waterLevel * avdsCtx->radarUnit;
	
	if(avdsCfg->mode == AVDS_MODE_SINGLE_VERTICAL)
		pqCnt = AVDS_HW_PQ_TILT_DISTANCE;
	else
		pqCnt = AVDS_HW_PQ_MAX;
		
	if(avdsCfg->mode == AVDS_MODE_SINGLE_TILT)
		avdsCtx->activeIdx = 1;
	
	/*if(avdsCfg->mode == AVDS_MODE_SINGLE_VERTICAL)
		prevFilter = 0;
	else */if(avdsCfg->mode == AVDS_MODE_DUAL)
		prevFilter = ((1<<AVDS_HW_PQ_DISTANCE) | (1 << AVDS_HW_PQ_TILT_DISTANCE));
	else if(avdsCfg->mode == AVDS_MODE_SINGLE_TILT)
		prevFilter = 1 << AVDS_HW_PQ_TILT_DISTANCE;
	hwPqInf = createHwPqInf(pqCnt, prevFilter);
	avdsInst->hwPqInf = hwPqInf;	
}
//------------------------------------------------------------------------------
static void avdsPowerCtrlProcess(AVDS_RADAR_INSTANCE *avdsInst, int onOff)
{	AVDS_RADAR_CONTEXT *avdsCtx = avdsInst->avdsCtx;
	AVDS_RADAR_CONFIG	*avdsCfg = (AVDS_RADAR_CONFIG	*)avdsInst->avdsCfgPtr;
	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	int devPwrSrc;
	
	if(avdsCtx->activeIdx)
		devPwrSrc = DEVICE_PWR_SRC_DO0;
	else
		devPwrSrc = DEVICE_PWR_SRC_DO1;
	setOutputPower(devPwrSrc, onOff, __func__, __LINE__);
	processCtx->pwrOnDelayTimer = GetTimeTicks();
}
//------------------------------------------------------------------------------
static int avdsWaitPowerOffDone(AVDS_RADAR_INSTANCE *avdsInst)
{	AVDS_RADAR_CONTEXT *avdsCtx = avdsInst->avdsCtx;
	AVDS_RADAR_CONFIG	*avdsCfg = (AVDS_RADAR_CONFIG	*)avdsInst->avdsCfgPtr;
	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	
	if((GetTimeTicks() - processCtx->pwrOnDelayTimer) > processCtx->pwrOnDelayTimeout)
	{	if(avdsCtx->swapRadar == 1)
		{	avdsCtx->activeIdx = (~avdsCtx->activeIdx) & 0x01;
			avdsCtx->swapRadar = 0;
		}
		processCtx->fsm = RADAR_FSM_POWER_ON;
		return 0;
	}
	
	return 1;
}
//------------------------------------------------------------------------------
static void avdsCmdIssueProcess(AVDS_RADAR_INSTANCE *avdsInst)
{	AVDS_RADAR_CONTEXT *avdsCtx = avdsInst->avdsCtx;
	AVDS_RADAR_CONFIG	*avdsCfg = (AVDS_RADAR_CONFIG	*)avdsInst->avdsCfgPtr;
	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	DEV_INSTANCE *devInst;
	int rangeScale = 5;
	char cmdBuf[128];
	int txBackOff = 0;
	int rxGain = 36;
	int fps = 15;
	int power;
	int filterType = 0;
	int cmdLen;
	
	if(avdsCfg->mode != AVDS_MODE_SINGLE_TILT)
	{	avdsCtx->useHeatMapCmd = 0;
		avdsCtx->endRangeBin = avdsCfg->verticalEndRange / (rangeScale * 10);
		while(avdsCtx->endRangeBin > 255)
		{	rangeScale++;
			avdsCtx->endRangeBin = avdsCfg->verticalEndRange / (rangeScale * 10);
			avdsCtx->useHeatMapCmd = 1;
		}
		avdsCtx->startRangeBin = avdsCfg->verticalStartRange / (rangeScale * 10);
	}
	
	memset(cmdBuf, 0, 128);
	if(avdsCfg->mode == AVDS_MODE_SINGLE_VERTICAL)
	{	
#if TEST_METHOD_1111
		avdsCtx->checkZoomIn = 0;
#endif
		if(avdsCtx->useHeatMapCmd)
		{	avdsCtx->currRangeStart = avdsCtx->startRangeBin;
			avdsCtx->currRangeEnd = avdsCtx->endRangeBin;
			avdsCtx->currAngleStart = avdsCfg->verticalStartAzimuth;
			avdsCtx->currAngleEnd = avdsCfg->verticalEndAzimuth;
			SENS_SPRINTF(cmdBuf, "flHeatmap %d %d %d %d %d %d %d %d\r\n", (rangeScale < 5)? 5:rangeScale, txBackOff, rxGain, avdsCtx->startRangeBin, avdsCtx->endRangeBin, avdsCfg->verticalStartAzimuth, avdsCfg->verticalEndAzimuth, filterType);
		}
		else
		{	rxGain = 30;
			power = (txBackOff * 256) + rxGain;
			SENS_SPRINTF(cmdBuf, "fl %d %d %d %d %d %d %d 0\r\n", fps, power, avdsCtx->startRangeBin, avdsCtx->endRangeBin, avdsCfg->verticalStartAzimuth, avdsCfg->verticalEndAzimuth, filterType);
		}
	}
	else
	{	avdsCtx->useHeatMapCmd = 1;
		if(avdsCtx->activeIdx == 0)
		{	
#if TEST_METHOD_1111
			avdsCtx->checkZoomIn = 0;
#endif
			avdsCtx->currRangeStart = avdsCtx->startRangeBin;
			avdsCtx->currRangeEnd = avdsCtx->endRangeBin;
			avdsCtx->currAngleStart = avdsCfg->verticalStartAzimuth;
			avdsCtx->currAngleEnd = avdsCfg->verticalEndAzimuth;
			SENS_SPRINTF(cmdBuf, "flHeatmap %d %d %d %d %d %d %d %d\r\n", (rangeScale < 5)? 5:rangeScale, txBackOff, rxGain, avdsCtx->startRangeBin, avdsCtx->endRangeBin, avdsCfg->verticalStartAzimuth, avdsCfg->verticalEndAzimuth, filterType);
		}
		else
		{	int startRangeBin, endRangeBin;
			
#if TEST_METHOD_1111
			if(!avdsCtx->lowerZoomInTimeoutFlag)
#endif
			{	rangeScale = 5;
				avdsCtx->tiltStartRange = (int)lroundf(((float)avdsCtx->verticalRadarMeasureDistance + 65 + avdsCtx->distanceOfTwoRadar) / cos((float)avdsCtx->tiltAngle*3.14159/180.0));
				if(avdsCfg->mode == AVDS_MODE_SINGLE_TILT)	//only tilt
				{	DUAL_RADAR_REC_STRUCT *dualRadarRecStruct;

					if(avdsCtx->dualRadarRecStruct == NULL)
					{	avdsCtx->dualRadarRecStruct = SENS_MEM_ZALLOC(sizeof(DUAL_RADAR_REC_STRUCT));
					}
					dualRadarRecStruct = avdsCtx->dualRadarRecStruct;
					if(!chkFileExist(DUAL_RADAR_REC_FILENAME))
					{	
SET_NEW_RECORD:
						dualRadarRecStruct->tag = 0x55AA;
						dualRadarRecStruct->verticalMeasureDist = avdsCtx->waterLevel;
						dualRadarRecStruct->tiltMeasureDist = 0;
						avdsCtx->tiltStartRange = (int)lroundf(((float)avdsCtx->waterLevel + 65 + avdsCtx->distanceOfTwoRadar) / cos((float)avdsCtx->tiltAngle*3.14159/180.0));
					}
					else
					{	sdReadFile(DUAL_RADAR_REC_FILENAME, (char *)dualRadarRecStruct, sizeof(DUAL_RADAR_REC_STRUCT), 0, NORMAL_READ_MODE);
						if(dualRadarRecStruct->tag != 0x55AA)
						{	memset(dualRadarRecStruct, 0, sizeof(DUAL_RADAR_REC_STRUCT));
							goto SET_NEW_RECORD;
						}
						avdsCtx->tiltStartRange = dualRadarRecStruct->tiltMeasureDist;
					}
					avdsCtx->dualRadarRecBufIsValid = 1;
					sdWriteFile(DUAL_RADAR_REC_FILENAME, (char *)dualRadarRecStruct, sizeof(DUAL_RADAR_REC_STRUCT), 0, 0);
				}
			
				dbg_msg("[AVDS] startRange:%d, verticalRadarMeasureDist:%d, distanceOfTwoRadar:%d\r\n", avdsCtx->tiltStartRange, avdsCtx->verticalRadarMeasureDistance, avdsCtx->distanceOfTwoRadar);
			
				while((avdsCtx->tiltStartRange > (rangeScale * 2550)))
				{	rangeScale++;
				}
				startRangeBin = (int)lroundf((float)avdsCtx->tiltStartRange / (rangeScale * 10));
				dbg_msg("[AVDS] std range:%d, gRadar->fineTuneRangeHi:%d, gRadar->fineTuneRangeLo:%d\r\n", startRangeBin, avdsCfg->tiltBinRangeH, avdsCfg->tiltBinRangeL);
				endRangeBin = startRangeBin + avdsCfg->tiltBinRangeH;
				if(endRangeBin > 255)
					endRangeBin = 255;
				startRangeBin -= avdsCfg->tiltBinRangeL;
				if(startRangeBin < 0)
					startRangeBin = 40/rangeScale;
#if TEST_METHOD_1111
				//endRangeBin = 255;
#endif
				avdsCtx->currRangeStart = startRangeBin;
				avdsCtx->currRangeEnd = endRangeBin;
				avdsCtx->currAngleStart = avdsCfg->verticalStartAzimuth;
				avdsCtx->currAngleEnd = avdsCfg->verticalEndAzimuth;
			}
#if TEST_METHOD_1111
			else
			{	avdsCtx->lowerZoomInTimeoutFlag = 0;
				avdsCtx->currRangeStart++;
				avdsCtx->currRangeEnd++;
				startRangeBin = avdsCtx->currRangeStart;
				endRangeBin = avdsCtx->currRangeEnd;
			}
			processCtx->lowerZoomInTimer = sensDev->current_tick;
			processCtx->lowerZoomInTimeout = 8000;
			avdsCtx->checkZoomIn = 1;
#endif
			SENS_SPRINTF(cmdBuf, "flHeatmap %d %d %d %d %d %d %d %d\r\n", (rangeScale < 5)? 5:rangeScale, txBackOff, rxGain, startRangeBin, endRangeBin, avdsCfg->verticalStartAzimuth, avdsCfg->verticalEndAzimuth, filterType);
		}
	}
	
	cmdLen = strlen(cmdBuf);
	dbg_msg("[AVDS] issue cmd: %s\r\n", cmdBuf);
	devInst = (DEV_INSTANCE *)avdsInst->devInst;
	rs485SendData(devInst->interface, (uint8_t *)cmdBuf, cmdLen);
		
	processCtx->cmdNoResponseTimer = sysTimer->currTick;
	processCtx->cmdNoResponseTimeout = 15000;	//15sec
	processCtx->recvLen = 0;
}
//------------------------------------------------------------------------------
static int avdsParserProcess(AVDS_RADAR_INSTANCE *avdsInst)
{	AVDS_RADAR_CONTEXT *avdsCtx = avdsInst->avdsCtx;
	AVDS_RADAR_CONFIG	*avdsCfg = (AVDS_RADAR_CONFIG	*)avdsInst->avdsCfgPtr;
	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	RADAR_NORMAL_FRAME *normalFrame;
	RADAR_FAST_SCAN_FRAME *fastScanFrame;
	SENSOR_HW_PQ_STRUCT	*hwPqInf;
	char *frameTagPos;
	int ret = 1;
	float distance;
	float waterLevelVaule;
	int pqOffset;
	int hwPqReady;
	int startPqIdx, endPqIdx;
	
	if(avdsCtx->useHeatMapCmd)
	{	frameTagPos = strstr(processCtx->recvBuf, RADAR_RX_FAST_SCAN_FRAME_START_TAG);
		fastScanFrame = (RADAR_FAST_SCAN_FRAME *)frameTagPos;
		
		if((avdsCtx->currRangeStart != fastScanFrame->rangeStart) && (avdsCtx->currRangeEnd != fastScanFrame->rangeEnd))		
		{	processCtx->recvLen = 0;
			return ret;
		}
		
#if TEST_METHOD_1111
		if(avdsCtx->checkZoomIn && (fastScanFrame->zoomIndex < 100))
		{	if((sensDev->current_tick - processCtx->lowerZoomInTimer) > processCtx->lowerZoomInTimeout)
			{	avdsCtx->lowerZoomInTimeoutFlag = 1;
			}
			processCtx->recvLen = 0;
			return ret;
		}
		if(avdsCtx->checkZoomIn)
			processCtx->lowerZoomInTimer = sensDev->current_tick;
#endif
		if(avdsCtx->activeIdx)
		{	startPqIdx = AVDS_HW_PQ_TILT_DISTANCE;
			endPqIdx = AVDS_HW_PQ_TILT_WL;
		}
		else
		{	startPqIdx = AVDS_HW_PQ_DISTANCE;
			endPqIdx = AVDS_HW_PQ_WL;
		}
		
		pqOffset = avdsCtx->activeIdx * AVDS_HW_PQ_TILT_DISTANCE;
		fastScanFrame->fineRange *= 1000; 
		distance = (fastScanFrame->fineRange - 65)*cos((float)fastScanFrame->angleIndex*3.14159/180.0);
		setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_DISTANCE + pqOffset, distance);
		setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_SNR + pqOffset, fastScanFrame->snr);
		setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_POWER + pqOffset, fastScanFrame->focusRangePower);
		setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_ANGLE + pqOffset, fastScanFrame->angleIndex);
		setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_STATUS + pqOffset, 1);
		//dbg_msg("[AVDS] Range:%d, angle:%d, peakIdx:%d, zoom:%d, power:%d, bin(%d~%d), currBin(%d~%d)\r\n", (int)fastScanFrame->fineRange, fastScanFrame->angleIndex, fastScanFrame->peakIndex, fastScanFrame->zoomIndex, (int)fastScanFrame->focusRangePower, fastScanFrame->rangeStart, fastScanFrame->rangeEnd, avdsCtx->currRangeStart, avdsCtx->currRangeEnd);
		hwPqReady = 1;
		for(hwPqInf=avdsInst->hwPqInf;hwPqInf;hwPqInf=hwPqInf->next)
		{	if((hwPqInf->hwPqIdx >= startPqIdx) && (hwPqInf->hwPqIdx < endPqIdx))
			{	if(!hwPqInf->isHwPqReady)
				{	hwPqReady = 0;
					break;
				}
			}
		}
		
		if(hwPqReady)
		{	if(!avdsCtx->activeIdx)
				avdsCtx->verticalRadarMeasureDistance = distance;
			if(avdsCfg->mode != AVDS_MODE_SINGLE_VERTICAL)
				checkAndSetRadarRecInfo(avdsInst, distance);
			
			if(!avdsCtx->activeIdx)
			{	waterLevelVaule = avdsCfg->installHeight - distance;
				if(waterLevelVaule < 0)	waterLevelVaule = 0;
				setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_WL, waterLevelVaule);
			}
			else
			{	if(avdsCfg->mode == AVDS_MODE_SINGLE_TILT)
					waterLevelVaule = avdsCfg->installHeight - (distance * cos(avdsCtx->tiltAngleCalibration*3.14159/180.0));
				else
					waterLevelVaule = avdsCfg->installHeight - (distance * cos(avdsCtx->tiltAngleCalibration*3.14159/180.0) - avdsCtx->distanceOfTwoRadar);
				setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_TILT_WL, waterLevelVaule);
			}
			ret = 0;
			processCtx->recvLen = 0;
		}
	}
	else	//only AVDS_MODE_SINGLE_VERTICAL
	{	frameTagPos = strstr(processCtx->recvBuf, RADAR_RX_FRAME_START_TAG);
		normalFrame = (RADAR_NORMAL_FRAME *)frameTagPos;
		
		if(normalFrame->fineRange > 0 && normalFrame->fineRange < 100)
		{	distance = (normalFrame->fineRange * 1000 - 65) * cos(normalFrame->angleIndex*3.14159/180.0);
			setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_DISTANCE, distance);
			setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_STATUS, 1);
			waterLevelVaule = avdsCfg->installHeight - distance;
			if(waterLevelVaule < 0)	waterLevelVaule = 0;
			setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_WL, waterLevelVaule);
		}
		else
		{	setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_DISTANCE, SENS_SET_VAL_NAN());
			setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_STATUS, 2);
			setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_WL, SENS_SET_VAL_NAN());
		}
		setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_SNR, normalFrame->snr);
		setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_POWER, normalFrame->power);
		setRadarHwPq(avdsInst->hwPqInf, AVDS_HW_PQ_ANGLE, normalFrame->angleIndex);
		ret = 0;
		processCtx->recvLen = 0;
	}
	
	return ret;	
}
//------------------------------------------------------------------------------
static int checkAvdsFrame(AVDS_RADAR_CONTEXT *avdsCtx)
{	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	if(avdsCtx->useHeatMapCmd)
	{	return isFastScanFrameReceiveDone(processCtx->recvBuf, processCtx->recvLen);
	}
	else
	{	return isNormalFrameReceiveDone(processCtx->recvBuf, processCtx->recvLen, 0, 1);
	}
}
//------------------------------------------------------------------------------
static int radarReceiveProcess(RADAR_PROCESS_CONTEXT	*processCtx, int interface)
{	UART_CONFIG *uartCfg;
	char OneByte;
	char unitLen[6];
	int unitLength, fullPayloadLength;

	uartCfg = (UART_CONFIG *)&sensDev->rs485_Comm[interface];
	
	while(SENS_UART_STATUS(uartCfg->ctx))
	{	processCtx->cmdNoResponseTimer = sysTimer->currTick;
		SENS_UART_READ(uartCfg->ctx, (uint8_t *)&OneByte, 1);
		processCtx->recvBuf[processCtx->recvLen++] = OneByte;
		processCtx->recvBuf[processCtx->recvLen] = '\0';
		
		if(processCtx->isAvds)
		{	if((processCtx->recvLen == 2) && (processCtx->recvBuf[0] != 0x88) && (processCtx->recvBuf[1] != 0x77))
			{	processCtx->recvLen = 0;
				continue;
			}
			if((processCtx->recvLen > 2) && (processCtx->recvBuf[processCtx->recvLen-2] == 0x88) && (processCtx->recvBuf[processCtx->recvLen-1] == 0x77))
			{	processCtx->recvBuf[0] = 0x88;
				processCtx->recvBuf[1] = 0x77;
				processCtx->recvLen = 2;
				continue;
			}
		}
		else
		{	if((processCtx->recvLen == 2) && (processCtx->recvBuf[0] != 'A') && (processCtx->recvBuf[1] != 'R'))
			{	processCtx->recvLen = 0;
				continue;
			}
			if((processCtx->recvLen > 2) && (processCtx->recvBuf[processCtx->recvLen-2] == 'A') && (processCtx->recvBuf[processCtx->recvLen-1] == 'R'))
			{	processCtx->recvBuf[0] = 'A';
				processCtx->recvBuf[1] = 'R';
				processCtx->recvLen = 2;
				continue;
			}
			if(processCtx->recvLen == 7)
			{	unitLen[0] = OneByte;
				unitLen[1] = '\0';
				unitLength = _io_atoi(unitLen);
				fullPayloadLength = unitLength * 51 + 9;
			}
		}
		if(processCtx->recvLen >= RSP_BUF_SIZE_RADAR)
		{	dbg_msg("%s", "[RADAR] Recv buffer overflow\r\n");
			processCtx->recvLen = 0;
			continue;
		}
		
		if(processCtx->isAvds)
		{	int status;
			status = checkAvdsFrame((AVDS_RADAR_CONTEXT *)processCtx->upperCtx);
			if(status == 1)
			{	processCtx->fsm = RADAR_FSM_PARSER_DATA;
				return 0;
			}
			else if(status == 2)
			{	processCtx->recvLen = 0;
				continue;
			}
		}
	}
	
	if((sysTimer->currTick - processCtx->cmdNoResponseTimer) > processCtx->cmdNoResponseTimeout)
	{	processCtx->fsm = RADAR_FSM_RSP_TIMEOUT;
	}
	
	return 1;
}
//------------------------------------------------------------------------------
static void avdsTimeoutProcess(AVDS_RADAR_INSTANCE *avdsInst)
{	AVDS_RADAR_CONTEXT *avdsCtx = avdsInst->avdsCtx;
	AVDS_RADAR_CONFIG	*avdsCfg = (AVDS_RADAR_CONFIG	*)avdsInst->avdsCfgPtr;
	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	int pqOffset;
	DEV_INSTANCE	*devInst = (DEV_INSTANCE	*)avdsInst->devInst;
	SENSOR_HW_PQ_STRUCT	*hwPqInf;
	
	pqOffset = avdsCtx->activeIdx * AVDS_HW_PQ_TILT_DISTANCE;
	
	for(hwPqInf=avdsInst->hwPqInf;hwPqInf;hwPqInf=hwPqInf->next)
	{	if((hwPqInf->hwPqIdx >= AVDS_HW_PQ_DISTANCE + pqOffset) && (hwPqInf->hwPqIdx <= AVDS_HW_PQ_WL + pqOffset))
		{	hwPqInf->pqVal = SENS_SET_VAL_NAN();
		}
	}
	setSwPq(devInst, avdsInst->hwPqInf);
}
//------------------------------------------------------------------------------
void avdsCalibration(void)
{	DEV_INSTANCE	*devInst = sensDev->radarDevInst;
	AVDS_RADAR_INSTANCE *avdsInst = devInst->extDevInstPtr;
	AVDS_RADAR_CONTEXT *avdsCtx = avdsInst->avdsCtx;
	AVDS_RADAR_CONFIG	*avdsCfg = (AVDS_RADAR_CONFIG	*)avdsInst->avdsCfgPtr;
	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	
	
	if((devInst == NULL) || (devInst->extDevType != EXT_DEV_TYPE_RADAR_AVDS))
		return;
	
	if(avdsCfg->mode != AVDS_MODE_SINGLE_VERTICAL)
	{	processCtx->nextFsm = RADAR_FSM_CALIBRATION;
		processCtx->forceChangeFsm = 1;
	}
}
//------------------------------------------------------------------------------
static void avdsProcess(DEV_INSTANCE	*devInst)
{	AVDS_RADAR_INSTANCE *avdsInst = devInst->extDevInstPtr;
	AVDS_RADAR_CONTEXT *avdsCtx = avdsInst->avdsCtx;
	AVDS_RADAR_CONFIG	*avdsCfg = (AVDS_RADAR_CONFIG	*)avdsInst->avdsCfgPtr;
	RADAR_PROCESS_CONTEXT	*processCtx = &avdsCtx->processCtx;
	
	if(processCtx->forceChangeFsm)
	{	processCtx->fsm = processCtx->nextFsm;
		processCtx->forceChangeFsm = 0;
	}
	
	switch(processCtx->fsm)
	{	case RADAR_FSM_IDLE:
				avdsIdleProcess(avdsInst);
		case RADAR_FSM_POWER_ON:
				processCtx->fsm = RADAR_FSM_POWER_ON;
				avdsPowerCtrlProcess(avdsInst, 1);
		case RADAR_FSM_WAIT_POWER_READY:
				processCtx->fsm = RADAR_FSM_WAIT_POWER_READY;
				if((GetTimeTicks() - processCtx->pwrOnDelayTimer) > processCtx->pwrOnDelayTimeout)
				{	processCtx->fsm = RADAR_FSM_ISSUE_CMD;
				}
				else
					break;
		case RADAR_FSM_ISSUE_CMD:
				avdsCmdIssueProcess(avdsInst);
		case RADAR_FSM_RECEIVE_DATA:
				processCtx->fsm = RADAR_FSM_RECEIVE_DATA;
				if(radarReceiveProcess(processCtx, devInst->interface))
					break;
		case RADAR_FSM_PARSER_DATA:
				if(!avdsParserProcess(avdsInst))
					processCtx->fsm = RADAR_FSM_GET_PQ;
				else
				{	
#if TEST_METHOD_1111
					if(avdsCtx->lowerZoomInTimeoutFlag)
						processCtx->fsm = RADAR_FSM_POWER_OFF;
					else
#endif
						processCtx->fsm = RADAR_FSM_RECEIVE_DATA;
					break;
				}
		case RADAR_FSM_GET_PQ:
				setSwPq(devInst, avdsInst->hwPqInf);
				if(avdsCfg->mode == AVDS_MODE_DUAL && avdsCtx->activeIdx == 0)
					processCtx->fsm = RADAR_FSM_SWITCH_RADAR;
				else 
					processCtx->fsm = RADAR_FSM_RECEIVE_DATA;
				break;
		case RADAR_FSM_POWER_OFF:
				processCtx->fsm = RADAR_FSM_POWER_OFF;
				avdsPowerCtrlProcess(avdsInst, 0);
		case RADAR_FSM_WAIT_POWER_OFF_DONE:
				processCtx->fsm = RADAR_FSM_WAIT_POWER_OFF_DONE;
				avdsWaitPowerOffDone(avdsInst);
				break;
		case RADAR_FSM_SWITCH_RADAR:
				avdsCtx->swapRadar = 1;
				processCtx->fsm = RADAR_FSM_POWER_OFF;
				break;
		case RADAR_FSM_RSP_TIMEOUT:
				processCtx->fsm = RADAR_FSM_POWER_OFF;
				//avdsTimeoutProcess(avdsInst);
				break;
		case RADAR_FSM_CALIBRATION:
				sdFileDelete(DUAL_RADAR_REC_FILENAME);
				if(avdsCtx->dualRadarRecBufIsValid)
					avdsCtx->dualRadarRecBufIsValid = 0;
				if(avdsCtx->dualRadarRecStruct)
					SENS_MEM_FREE(avdsCtx->dualRadarRecStruct);
				processCtx->fsm = RADAR_FSM_POWER_OFF;
				avdsPowerCtrlProcess(avdsInst, 0);
				if(avdsCtx->activeIdx && (avdsCfg->mode == AVDS_MODE_DUAL))
				{	avdsCtx->activeIdx = 0;
					avdsCtx->swapRadar = 0;
				}
				break;
		default:
				break;
	}
}
//------------------------------------------------------------------------------
static void ar77Process(DEV_INSTANCE	*devInst)
{
}
//------------------------------------------------------------------------------
void radarOperation(void)
{	DEV_INSTANCE	*devInst = sensDev->radarDevInst;
	//not support avds & ar77 at same time
	if(devInst == NULL)
		return;
	
	switch(devInst->extDevType)
	{	case EXT_DEV_TYPE_RADAR_AVDS:
				avdsProcess(devInst);
				break;
		case EXT_DEV_TYPE_RADAR_AR77:
				ar77Process(devInst);
				break;
		default:
				break;
	}
}
