#include "sensors/sensorTask.h"
#include "sensCommon.h"
#include "sensSystem.h"
//#include "driver/i2cDrv.h"
#include "driver/tcn75a.h"
#include "physicalQuantity.h"
#include "sensDev.h"
#include "peripheral/rs485.h"
#include "systemTimer.h"

#if defined OS_MQX
_mqx_max_type  sensorQ[SENSOR_Q_SIZE]; /* prepare message queue for 20 events */
#elif defined OS_FREERTOS
QueueHandle_t sensorQ;
#endif

//------------------------------------------------------------------------------
static int sensorTaskInit(void)
{	if(MQX_OK != SENS_MSG_Q_INIT(sensorQ, SENSOR_NUM_MESSAGES, SENS_TASKQ_GRANM)) 
	{	return -1;
	}
	
	return 0;
}
//------------------------------------------------------------------------------
static DEV_INSTANCE *initialRadarDev(EXT_DEV_CONFIG *extDevCfg)
{	DEV_INSTANCE *devInst;
	
	devInst = SENS_MEM_ZALLOC(sizeof(DEV_INSTANCE));
	devInst->extDevNo = extDevCfg->devIdx;
	devInst->protocol = extDevCfg->ifProtocol;
	//devInst->extDevType = EXT_DEV_TYPE_MODBUS_NORMAL;
	devInst->interface = extDevCfg->ifChannel;
	
	if((devInst->protocol == EXT_IF_PROTO_AVDS_RADAR) && extDevCfg->speDevCfg)
	{	AVDS_RADAR_INSTANCE *avdsInst;
		AVDS_RADAR_CONTEXT	*avdsCtx;
					
		devInst->extDevType = EXT_DEV_TYPE_RADAR_AVDS;
		avdsInst = SENS_MEM_ZALLOC(sizeof(AVDS_RADAR_INSTANCE));
		avdsInst->avdsCfgPtr = extDevCfg->speDevCfg;
		avdsCtx = SENS_MEM_ZALLOC(sizeof(AVDS_RADAR_CONTEXT));
		avdsInst->avdsCtx = avdsCtx;
		devInst->extDevInstPtr = avdsInst;
		avdsInst->devInst = devInst;
	}
	else if((devInst->protocol == EXT_IF_PROTO_AR77_RADAR) && extDevCfg->speDevCfg)
	{	AR77_RADAR_INSTANCE *ar77Inst;
		AR77_RADAR_CONTEXT *ar77Ctx;
					
		devInst->extDevType = EXT_DEV_TYPE_RADAR_AR77;
		ar77Inst = SENS_MEM_ZALLOC(sizeof(AR77_RADAR_INSTANCE));
		ar77Inst->ar77CfgPtr = extDevCfg->speDevCfg;
		ar77Ctx = SENS_MEM_ZALLOC(sizeof(AR77_RADAR_CONTEXT));
		ar77Inst->ar77Ctx = ar77Ctx;
		devInst->extDevInstPtr = ar77Inst;
		ar77Inst->devInst = devInst;
	}
	return devInst;
}
//------------------------------------------------------------------------------
static DEV_INSTANCE *initialDconDev(EXT_DEV_CONFIG *extDevCfg)
{	DEV_INSTANCE *devInst;
	SENSOR_PROCESS_CONTEXT *processCtx;
	NORMAL_SENSOR_INSTANCE *normalInst;
	
	devInst = SENS_MEM_ZALLOC(sizeof(DEV_INSTANCE));
	
	devInst->extDevNo = extDevCfg->devIdx;
	devInst->protocol = extDevCfg->ifProtocol;
	devInst->extDevType = EXT_DEV_TYPE_DCON_NORMAL;
	devInst->interface = extDevCfg->ifChannel;
	
	normalInst = SENS_MEM_ZALLOC(sizeof(NORMAL_SENSOR_INSTANCE));
	normalInst->ctx = SENS_MEM_ZALLOC(sizeof(NORMAL_SENSOR_CONTEXT));
	devInst->extDevInstPtr = normalInst;
	normalInst->devInst = devInst;

	return devInst;
}
//------------------------------------------------------------------------------
static DEV_INSTANCE *initialModbusDev(EXT_DEV_CONFIG *extDevCfg)
{	DEV_INSTANCE *devInst;
	SENSOR_PROCESS_CONTEXT *processCtx;
	
	devInst = SENS_MEM_ZALLOC(sizeof(DEV_INSTANCE));
	devInst->extDevNo = extDevCfg->devIdx;
	devInst->protocol = extDevCfg->ifProtocol;
	devInst->extDevType = EXT_DEV_TYPE_MODBUS_NORMAL;
	devInst->interface = extDevCfg->ifChannel;
	if(extDevCfg->devModel == EXT_DEV_MODEL_OSA24G)
	{	OSA24G_INSTANCE *osa24Inst;
		OSA24G_CONFIG *osa24gCfg = (OSA24G_CONFIG *)extDevCfg->speDevCfg;
		OSA24G_CONTEXT *osa24Ctx;
										
		devInst->extDevType = EXT_DEV_TYPE_MODBUS_OSA24G;
		osa24Inst = SENS_MEM_ZALLOC(sizeof(OSA24G_INSTANCE));
		osa24Inst->osa24gCfgPtr = extDevCfg->speDevCfg;
		osa24Ctx = SENS_MEM_ZALLOC(sizeof(OSA24G_CONTEXT));
		osa24Inst->osa24gCtx = osa24Ctx;
		devInst->extDevInstPtr = osa24Inst;
		osa24Inst->devInst = devInst;
		processCtx = &osa24Ctx->processCtx;
		processCtx->pollingInterval = osa24gCfg->pollingInterval;
	}
	else if(extDevCfg->devModel == EXT_DEV_MODEL_VW)
	{	VW_SENSOR_INSTANCE *vwInst;
		VW_SENSOR_CONFIG *vwCfg = (VW_SENSOR_CONFIG *)extDevCfg->speDevCfg;
		VW_SENSOR_CONTEXT *vwCtx;
					
		devInst->extDevType = EXT_DEV_TYPE_MODBUS_VW_SENSOR;
		vwInst = SENS_MEM_ZALLOC(sizeof(VW_SENSOR_INSTANCE));
		vwInst->vwCfgPtr = extDevCfg->speDevCfg;
		vwCtx = SENS_MEM_ZALLOC(sizeof(VW_SENSOR_CONTEXT));
		vwInst->vwCtx = vwCtx;
		devInst->extDevInstPtr = vwInst;
		vwInst->devInst = devInst;
					
		processCtx = &vwCtx->processCtx;
		processCtx->pollingInterval = vwCfg->pollingInterval;
	}
	else if(extDevCfg->devModel == EXT_DEV_MODEL_COMPOSITE_OSA24G)
	{	COMPOSITE_SENSOR_INSTANCE	*compositeInst;
		COMPOSITE_SENSOR_CONTEXT	*compositeCtx;
		COMPOSITE_OSA_CONFIG		*compositeOsaCfg;
		
		compositeInst = SENS_MEM_ZALLOC(sizeof(COMPOSITE_SENSOR_INSTANCE));
		compositeInst->isAiDev = 0;
		compositeInst->compositeCfgPtr = extDevCfg->speDevCfg;
		compositeCtx = SENS_MEM_ZALLOC(sizeof(COMPOSITE_SENSOR_CONTEXT));
		compositeInst->compositeCtx = compositeCtx;
		
		compositeOsaCfg = (COMPOSITE_OSA_CONFIG *)compositeInst->compositeCfgPtr;
					
		if(compositeOsaCfg->radarDevNo == extDevCfg->devIdx)
		{	devInst->extDevType = EXT_DEV_TYPE_MODBUS_COMP_OSA_RADAR;
			compositeInst->isRadarDev = 1;						
		}
		else if(compositeOsaCfg->wlsDevNo == extDevCfg->devIdx)
		{	devInst->extDevType = EXT_DEV_TYPE_MODBUS_COMP_OSA_WLS;
			compositeInst->isRadarDev = 0;
		}
		devInst->extDevInstPtr = compositeInst;
		compositeInst->devInst = devInst;
		processCtx = &compositeCtx->processCtx;
		processCtx->pollingInterval = 1;
	}
	else
	{	NORMAL_SENSOR_INSTANCE *normalInst;
		normalInst = SENS_MEM_ZALLOC(sizeof(NORMAL_SENSOR_INSTANCE));
		normalInst->ctx = SENS_MEM_ZALLOC(sizeof(NORMAL_SENSOR_CONTEXT));
		devInst->extDevInstPtr = normalInst;
		normalInst->devInst = devInst;
	}
	
	return devInst;
}
//------------------------------------------------------------------------------
static void addDevInst(int devInstType, DEV_INSTANCE *newDevInst)
{	DEV_INSTANCE *devInst;
	
	if(devInstType == 0)		//modbus
	{	if(sensDev->modbusDevInsts == NULL)
			sensDev->modbusDevInsts = newDevInst;
		else
		{	devInst = sensDev->modbusDevInsts;
			while(devInst->next)
			{	devInst = devInst->next;
			}
			devInst->next = newDevInst;
		}
	}
	else if(devInstType == 1)	//dcon
	{	if(sensDev->dconDevInsts == NULL)
			sensDev->dconDevInsts = newDevInst;
		else
		{	devInst = sensDev->dconDevInsts;
			while(devInst->next)
			{	devInst = devInst->next;
			}
			devInst->next = newDevInst;
		}
	}
	
	
}
//------------------------------------------------------------------------------
static void addSwPqMappingToDevInst(DEV_INSTANCE *devInst, SW_PQ_MAPPING *nwqSwPqMapping)
{	SW_PQ_MAPPING *swPqMapping;
	
	if(devInst->swPqMapping == NULL)
		devInst->swPqMapping = nwqSwPqMapping;
	else
	{	swPqMapping = devInst->swPqMapping;
		while(swPqMapping->next)
		{	swPqMapping = swPqMapping->next;
		}
		swPqMapping->next = nwqSwPqMapping;
	}
}
//------------------------------------------------------------------------------
static void bindExtDevToDevInst(void)
{	DEV_INSTANCE *devInst;
	SW_PQ_MAPPING *swPqMapping;
	SENSOR_PROCESS_CONTEXT *processCtx;
	EXT_DEV_CONFIG *extDevCfg;
	PQ_DATA_STRUCT *pqData;
	DISPLAY_PANEL_INSTANCE *displayPanelInst = NULL;
	
	if(sysCfg->extDevCfgs == NULL)
		return;
	
	for(extDevCfg=sysCfg->extDevCfgs;extDevCfg;extDevCfg=extDevCfg->next)
	{	devInst = NULL;
		switch(extDevCfg->ifProtocol)
		{	case EXT_IF_PROTO_MODBUS:
					{	devInst = initialModbusDev(extDevCfg);
					}
					break;
			case EXT_IF_PROTO_AVDS_RADAR:
			case EXT_IF_PROTO_AR77_RADAR:
					{	devInst = initialRadarDev(extDevCfg);
					}
					break;
			case EXT_IF_PROTO_DCON:
					{	devInst = initialDconDev(extDevCfg);
					}
					break;
			case EXT_IF_PROTO_DISPLAY_PANEL:
					{	displayPanelInst = SENS_MEM_ZALLOC(sizeof(DISPLAY_PANEL_INSTANCE));
						displayPanelInst->extDevCfg = extDevCfg;
						sensDev->displayPanelInst = displayPanelInst;
					}
					break;
			default:
					break;
		}
		
		if(devInst)
		{	//get sw pq mapping
			for(pqData = sensPq->pqData;pqData;pqData=pqData->next)
			{	if(pqData->devIdx == devInst->extDevNo)
				{	swPqMapping = SENS_MEM_ZALLOC(sizeof(SW_PQ_MAPPING));
					swPqMapping->swPqIdx = pqData->pqIdx;
					swPqMapping->hwPqMeasureIdx = pqData->measureIdx;
					//swPqMapping->pqData = pqData;
					addSwPqMappingToDevInst(devInst, swPqMapping);
				}
			}
			if((devInst->extDevType == EXT_DEV_TYPE_RADAR_AVDS) || (devInst->extDevType == EXT_DEV_TYPE_RADAR_AR77))
				sensDev->radarDevInst = devInst;
			else if(devInst->extDevType == EXT_DEV_TYPE_DCON_NORMAL)
				addDevInst(1, devInst);
			else
				addDevInst(0, devInst);
		}
	}
}
//------------------------------------------------------------------------------
void sensorTask(void *param)
{	AI_INSTANCE *aiInst = (AI_INSTANCE *)&sensDev->aiInst;
	AI_CONTEXT	*aiCtx = (AI_CONTEXT *)&aiInst->aiCtx;
	INTERNAL_SENSOR_INSTANCE *intSensorInst = (INTERNAL_SENSOR_INSTANCE *)&sensDev->intSensorInst;
	TEMPERATURE_CONTEXT *temperatureCtx = (TEMPERATURE_CONTEXT *)&intSensorInst->temperatureCtx;
	DEV_INSTANCE *modbusDevInst;
	DEV_INSTANCE *dconDevInst;
	DEV_INSTANCE *radarDevInst;
	AI_CONFIG *aiCfg = (AI_CONFIG *)&sysCfg->aiCfg;
	
	sensorTaskInit();
	initOnboardTemperatureSensor();
	initAiSensor();
	bindExtDevToDevInst();
	
	//PERIPHERAL init, RS485
	rs485Init(EXT_IF_CHANNEL_RS485_1);
	rs485Init(EXT_IF_CHANNEL_RS485_2);
	modbusInit();
	dconInit();
	
	modbusDevInst = sensDev->modbusDevInsts;
	dconDevInst = sensDev->dconDevInsts;
	radarDevInst = sensDev->radarDevInst;
	temperatureCtx->pollTimer = GetTimeTicks() - 5001;
	temperatureCtx->pollTimeout = 5000;
	aiCtx->pollTimer = GetTimeTicks() - 2001;
	aiCtx->pollTimeout = 2000;
	voltSenseInit();
	voltSenseStart();
	displayPanelInit();
	
	if(sysCfg->cameraCfg)
	{	initUvcCamDevice();
		initialTaskStruct((TASK_INFO *)&taskAct.uvcCameraTaskAct);
		taskAct.uvcCameraTaskAct.id = OSA_TASK_CREATE(UVC_CAMERA_TASK);
		SENS_TIME_DELAY(2);
	}
	
	while(1)
	{	voltSenseCheck();
		
		if((GetTimeTicks() - temperatureCtx->pollTimer) > temperatureCtx->pollTimeout)
		{	float temp;
			temp = tcn75aReadTemperature();
			sensPq->onboardPq[ON_BOARD_PQ_TEMPERATURE] = temp;
			putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_TEMPERATURE, sensPq->onboardPq[ON_BOARD_PQ_TEMPERATURE]);
			temperatureCtx->pollTimer = GetTimeTicks();
		}
		
		if(aiCfg->enable && ((GetTimeTicks() - aiCtx->pollTimer) > aiCtx->pollTimeout ))
		{	aiSensorValueGet();
			aiCtx->pollTimer = GetTimeTicks();
		}
		
		if(modbusDevInst)
			modbusDevInst = modbusProcess(modbusDevInst);
		if(dconDevInst)
			dconDevInst = dconProcess(dconDevInst);
		if(radarDevInst)
			radarOperation();
		displayPanelProcess();
		SENS_TIME_DELAY(50);
	}
}
//------------------------------------------------------------------------------
void uvcCameraTask(void *param)
{	struct TaskQMsg msg;
	UVC_CAMERA_CONTEXT	*ctx;
	//initUvcCamDevice();
	ctx = uvcCamInst->ctx;
	
	while(1)
	{	taskAct.uvcCameraTaskAct.sts = TASK_STS_BLOCKED;
		SENS_MSG_Q_RECV(sensorQ, (_mqx_max_type *)&msg, LWMSGQ_RECEIVE_BLOCK_ON_EMPTY, 0, 0);
		taskAct.uvcCameraTaskAct.sts = TASK_STS_RUNNING;
		extendTaskTimer((TASK_INFO *)&taskAct.uvcCameraTaskAct);
		
		switch(msg.msgId)
		{	case SENSOR_Q_MSG_CAMERA_INIT_TIMEOUT:
					dbgMsg("USB UVC initial Timout, re-try Cnt:%d\r\n", ctx->initTimeoutCnt);
					uvcCameraPowerOff();
					ctx->initTimeoutCnt++;
					if(ctx->initTimeoutCnt > 3)
					{	sysCtrl->isReadyToReboot = 1;
						break;	
					}
					SENS_TIME_DELAY(2000);
			case SENSOR_Q_MSG_CAMERA_PRE_START:
					ctx->uvcActive = 1;
					dbgMsg("free run time memory:%lu, before turn on UVC\r\n", xPortGetFreeHeapSize());
					setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_CAMERA_ACTIVE);
					uvcCameraPowerOn();
					setSensminiTimer((void *)sensorQ, SENSOR_Q_MSG_CAMERA_INIT_TIMEOUT, NULL, SENSMINI_TIMER_USB_UVC_INIT_TIMEOUT, USB_UVC_INIT_TIMEOUT, TIMER_MODE_TRIGGER);
					break;
			case SENSOR_Q_MSG_CAMERA_START:
					{	UVC_DEV_T *vdev = (UVC_DEV_T *)msg.ptr;
						killSensminiTimer(SENSMINI_TIMER_USB_UVC_INIT_TIMEOUT);
						uvcCameraStart(vdev);
						if(ctx->doSnapshot)
						{	msg.msgId = SENSOR_Q_MSG_CAMERA_GET_IMG_DATA;
							msg.ptr = (void *)vdev;
							if(sendMsgWithErrHandle(SENSORS_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
							{
							}
						}
						else
						{	//tune pixel
							break;
						}
					}
					break;
			case SENSOR_Q_MSG_CAMERA_GET_IMG_DATA:
					{	int result;
						UVC_DEV_T *vdev = (UVC_DEV_T *)msg.ptr;
						result = uvcCameraWaitImgReady(vdev);
						
						if(!result)
						{	SENS_TIME_DELAY(2);
							msg.msgId = SENSOR_Q_MSG_CAMERA_GET_IMG_DATA;
							msg.ptr = (void *)vdev;
							if(sendMsgWithErrHandle(SENSORS_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
							{
							}
							break;
						}
						else if(result > 0)
						{	/*msg.msgId = SENSOR_Q_MSG_CAMERA_STOP;
							msg.ptr = (void *)vdev;
							if(sendMsgWithErrHandle(SENSORS_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
							{
							}*/
							//continue running
						}
						else if(result < 0)
						{	//not ready
							//break;
						}
					}
			case SENSOR_Q_MSG_CAMERA_STOP:
					{	UVC_DEV_T *vdev = (UVC_DEV_T *)msg.ptr;
						uvcCameraStop(vdev);
						if(ctx->imgReady)
						{	//continue running
						}
						else
						{	ctx->captureFailCount++;
							if(ctx->captureFailCount > 3)
							{	sysCtrl->isReadyToReboot = 1;
							}
							else
							{	SENS_TIME_DELAY(5000);	//wait 5 sec to retry
								msg.msgId = SENSOR_Q_MSG_CAMERA_PRE_START;
								msg.ptr = (void *)vdev;
								if(sendMsgWithErrHandle(SENSORS_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
								{
								}
							}
							break;
						}
					}
			case SENSOR_Q_MSG_SAVE_IMG_FILE:
					saveImgFile();
					ctx->uvcActive = 0;
					setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_CAMERA_ACTIVE);
					break;
			case SENSOR_Q_MSG_UPLOAD_IMAGE_PREPARE:
					{	
					}
					break;
			default:
				break;
		}
	}
	
}

