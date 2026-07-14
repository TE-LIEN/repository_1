#include "sensminiCfg.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "sensors/sensorTask.h"
#include "sensDev.h"
#include "sdCardTask.h"
#include "network/netApp.h"
#include "powerCtrl.h"
#include "math.h"
#include "physicalQuantity.h"
#include "rtcDateTime.h"
#include "driver/PCAL6416GpioExpander.h"

//------------------------------------------------------------------------------
#ifndef HAVE_BOOTLOADER
void TMR1_IRQHandler(void)
{
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {	/* Clear Timer1 time-out interrupt flag */
		TIMER_ClearIntFlag(TIMER1);
#if WAKEUP_USE_VBAT_DOMAIN
		//RTC_SetGPIOMode(PIN_WAKEUP1_nRST_NO, RTC_IO_MODE_OUTPUT, RTC_IO_DIGITAL_ENABLE, RTC_IO_PULL_UP_DOWN_DISABLE, 1);
		RTC_SetGPIOMode(PIN_WAKEUP1_nRST_NO, RTC_IO_MODE_OUTPUT, RTC_IO_DIGITAL_ENABLE, RTC_IO_PULL_UP_ENABLE, 1);
#else
		PIN_WAKEUP1_nRST = 1;
#endif
		gRstWakeupTimerDone = 1;
    }
}
#endif
//------------------------------------------------------------------------------
void extendTaskTimer(TASK_INFO *actTask)
{	actTask->runningStartTime = GetTimeTicks();
}
//------------------------------------------------------------------------------
void resetServIdleXmitTimer(uint8_t setIdx)
{	int idx;
	
	for(idx=0;idx<UPLOAD_SERVER_CNT;idx++)
	{	if((sysCtrl->srvActiveBmp & (1 << idx)) & (setIdx & (1 << idx)))
		{	sysCtrl->servIdleXmitTime[idx] = GetTimeTicks();
		}
	}
}
//------------------------------------------------------------------------------
static int chkServIdleXmitTimer(int chkIdx, long timeout)
{	int idx;
	int timeoutIdx = 0;
	
	for(idx=0;idx<UPLOAD_SERVER_CNT;idx++)
	{	if((sysCtrl->srvActiveBmp & (1 << idx)) & (chkIdx & (1 << idx)))
		//if(sysCtrl->srvActiveBmp & (1 << idx))
		{	if((GetTimeTicks() - sysCtrl->servIdleXmitTime[idx]) > timeout)
			{	timeoutIdx |= (1 << idx);
			}
		}
	}
	
	return timeoutIdx;
}
//------------------------------------------------------------------------------
void setPsmStatus(uint8_t setOrClr, uint32_t bmp)
{	SYS_SEM *sysSem = (SYS_SEM *)&sensSys->sysSem;
	SYS_POWER_MANAGER *pwrManager = (SYS_POWER_MANAGER	*)&sysCtrl->pwrManager;
	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	
	SENS_SEM_LOCK(sysSem->disablePsmLock);
	if(setOrClr == SET_PSM_STATUS)
		SET_BMP(pwrManager->disablePsmBmp, bmp);
	else 
		CLR_BMP(pwrManager->disablePsmBmp, bmp);
	if(pwrManager->disablePsmBmp)
	{	pwrManager->currPsm = 0;
		if(pwrManager->disablePsmBmpBak != pwrManager->disablePsmBmp)
		{	dbg_msg("set PSM disable, current state BMP: %X\r\n", pwrManager->disablePsmBmp);
			pwrManager->disablePsmBmpBak = pwrManager->disablePsmBmp;
		}
	}
	else if(!pwrManager->disablePsmBmp)
	{	pwrManager->currPsm = sensSysCfg->psm;
		if(pwrManager->disablePsmBmpBak != pwrManager->disablePsmBmp)
		{	dbg_msg("restore PSM mode: %d\r\n", pwrManager->currPsm);
			pwrManager->disablePsmBmpBak = pwrManager->disablePsmBmp;
		}
	}
	SENS_SEM_UNLOCK(sysSem->disablePsmLock);
}
//------------------------------------------------------------------------------
CLOUD_SERVER_INFO *getServerInfoById(int idx)
{	CLOUD_SERVER_INFO *servInfo;
	
	for(servInfo=sysCfg->servInfos;servInfo;servInfo=servInfo->next)
	{	if(servInfo->servIdx == idx)
		{	break;
		}
	}
	
	return servInfo;
}
//------------------------------------------------------------------------------
CLOUD_SERVER_INFO *getServerInfoByServInst(CLOUD_SERVER_INSTANCE *servInst)
{	CLOUD_SERVER_INFO *servInfo;
	
	for(servInfo=sysCfg->servInfos;servInfo;servInfo=servInfo->next)
	{	if(servInfo->servInst == servInst)
		{	break;
		}
	}
	
	return servInfo;
}
//------------------------------------------------------------------------------
void checkAndTriggerCamera(int currHistoryRecTimeSlot)
{	struct TaskQMsg msg;
	IMAGE_UPLOAD_INSTANCE *imgUploadInst;
	//if((sysCtrl->pwrManager.currPsm == SYS_CFG_PSM_NONE) || 
	//	((sysCtrl->pwrManager.currPsm != SYS_CFG_PSM_NONE) && ((sysCfg->sensSysCfg.cameraRecInterval == 1) || 
	//														   ((currHistoryRecTimeSlot % sysCfg->sensSysCfg.cameraRecInterval) == 0))))
	if((sysCfg->sensSysCfg.psm == SYS_CFG_PSM_NONE) || 
		((sysCfg->sensSysCfg.psm != SYS_CFG_PSM_NONE) && (/*(sysCfg->sensSysCfg.cameraRecInterval == 1) || */
														  ((currHistoryRecTimeSlot % sysCfg->sensSysCfg.cameraRecInterval) == 0))))
	{	//trigger Camera
#if N_TEMP_REMOVE
		sysCtrlFlag.cameraSendDone = 0;
#endif
		//uvcCameraPowerOn();
		if(networkCtx->imgUploadInst)
		{	imgUploadInst = networkCtx->imgUploadInst;
			if(imgUploadInst->isAlarmUploadImgMode)
				return;
		}
		msg.msgId = SENSOR_Q_MSG_CAMERA_PRE_START;
		msg.ptr = NULL;
		if(sendMsgWithErrHandle(UVC_CAMERA_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
		{
		}
	}
}
//------------------------------------------------------------------------------
void pollWebImgCapture(void)
{	CAMERA_WEB_MANAGER	*camWebMng;
	struct TaskQMsg msg;
	
	if(sysCfg->cameraCfg)
	{	camWebMng = (CAMERA_WEB_MANAGER	*)&sysCtrl->camWebMng;
		if(camWebMng->status == CWS_STS_TRIG)
		{	camWebMng->status = CWS_STS_RUNNING;
			msg.msgId = SENSOR_Q_MSG_CAMERA_PRE_START;
			msg.ptr = NULL;
			if(sendMsgWithErrHandle(UVC_CAMERA_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
			{
			}
		}
	}
}
//------------------------------------------------------------------------------
void recordDiHistory(void)
{	DI_INSTANCE	*diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT	*diCtx = (DI_CONTEXT *)&diInst->diCtx;
	DI_CHG_CONTEXT *diChgCtx;
	DI_CHG_CONTEXT *next;
	uint8_t diBuf[10];
	
	diChgCtx = diCtx->diChgCtx;
	SENS_SEM_LOCK(diCtx->diChgSem);
	while(diChgCtx)
	{	next = diChgCtx->next;
		if(sysCfg->diCfg.diStatusRecord)
		{	memcpy(diBuf, (void *)&diChgCtx->unixTime, 8);
			diBuf[8] = diChgCtx->sts;
			diBuf[9] = diChgCtx->chg;
			dbgMsg("[DI Hist] DI Array[%d, %d, %d, %d, %d, %d]\r\n", diCtx->diStsOld[0]);
			writeDiHistory(diChgCtx, diBuf);
		}
		SENS_MEM_FREE(diChgCtx);
		diChgCtx = next;
		diCtx->diChgCtx = diChgCtx;
	}
	SENS_SEM_UNLOCK(diCtx->diChgSem);
}
//------------------------------------------------------------------------------
static int chkTemporaryDisSleepForPsm(SYS_POWER_MANAGER *pwrManager)
{	if(sysCtrl->enTemporaryDisSleep)
	{	if(sysTimer->currTick >= sysCtrl->temporaryDisSleepEndTimer)
		{	sysCtrl->temporaryDisSleepremainTime = 0;
			sysCtrl->enTemporaryDisSleep = 0;
		}
		else
		{	sysCtrl->temporaryDisSleepremainTime = sysCtrl->temporaryDisSleepEndTimer - sysTimer->currTick;
			pwrManager->psmChkBmp |= (int)1 << PSM_CHK_BMP_TEMPORARY_DISABLE_BMP;
			return -1;
		}
	}
	setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_TEMPORARY_DIS_PSM);
	pwrManager->psmChkBmp &= ~((int)1 << PSM_CHK_BMP_TEMPORARY_DISABLE_BMP);
	return 0;
}
//------------------------------------------------------------------------------
static int chkMobileActiveForPsm(SYS_POWER_MANAGER *pwrManager)
{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	WIRELESS_CONFIG *mobileCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
	int nMobileActive = sysTimer->currHistoryRecTimeSlot % mobileCfg->mobileInterval;
	
	if((nMobileActive == 0) && ((sysTimer->currTick - sysCtrl->sysWorkingStartTime) < 65000))
	{	pwrManager->psmChkBmp |= (int)1 << PSM_CHK_BMP_WORKING_TIMER;
		setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_WORKING_TIMER);
		return 1;
	}
	setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_WORKING_TIMER);
	pwrManager->psmChkBmp &= ~((int)1 << PSM_CHK_BMP_WORKING_TIMER);
	return 0;
}
//------------------------------------------------------------------------------
void powerSaving(void)
{	int timeElapsed;
	int standbyTime;
	S_RTC_TIME_DATA_T nextWakeupTime;
	uint32_t currTimestamp;
	SYS_POWER_MANAGER *pwrManager = (SYS_POWER_MANAGER	*)&sysCtrl->pwrManager;
	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	WIRELESS_CONFIG *mobileCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
	int nMobileActive = sysTimer->currHistoryRecTimeSlot % mobileCfg->mobileInterval;
	int idx;
	EXPANDER_GPIO_OUT outExpanderIoMode;
	SYS_CTRL_FLAG	sysCtrlFlagTemp;
	
	chkTemporaryDisSleepForPsm(pwrManager);
	
	chkMobileActiveForPsm(pwrManager);
	
	if(!pwrManager->currPsm)
	{	//pwrManager->psmChkBmp |= (int)1 << PSM_CHK_BMP_PSM_DISABLE;
		return;
	}
	
	timeElapsed = (sensSys->dateTime.u32Hour*3600 + sensSys->dateTime.u32Minute*60 + sensSys->dateTime.u32Second ) % (sensSysCfg->recordSec);

	standbyTime = sensSysCfg->recordSec - sensSysCfg->WakeUpInterval - timeElapsed;	//wakeup WakeUpInterval sec before the recording time
	if(nMobileActive == (mobileCfg->mobileInterval - 1))
	{	if(timeElapsed < 20)
			return;
		if((sensSysCfg->WakeUpInterval + 30) <= 60)
			standbyTime -= (30 + pwrManager->extendWorkingTime);
		else
			standbyTime -= pwrManager->extendWorkingTime;
		//standbyTime = sensSysCfg->recordSec - sensSysCfg->WakeUpInterval - pwrManager->extendWorkingTime - 30 - timeElapsed - ((sysTimer->currTick - sysTimer->sdRecTick)/1000) - 2;
	}
	else
	{	//standbyTime = sensSysCfg->recordSec - 20 - timeElapsed - ((sysTimer->currTick - sysTimer->sdRecTick)/1000) - 2;
	}
	
	if(standbyTime < 20)
		return;
	
	currTimestamp = dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
	currTimestamp += standbyTime;	//next wakeup timestamp
	timestampToDateTime(currTimestamp, &nextWakeupTime);
#if !DI_USE_IRQ
	dbgMsg("[PSM] set IRQ MASK:%X, %X\r\n", sysCtrlFlag.sysCtrlHiWord, sysCtrlFlag.sysCtrlFlags);
	sysCtrlFlagTemp.sysCtrlFlags = sysCtrlFlag.sysCtrlFlags;
	#ifdef HAVE_BOOTLOADER
	sysCtrlFlagTemp.extGpioIn0WakeupEn = 1;	//for DI-2 counter 
	sysCtrlFlagTemp.extGpioIn1WakeupEn = 1;	//for DI-3 counter
	#if BOARD_VERSION == 2
	sysCtrlFlagTemp.extGpioIn4WakeupEn = 1;	//for DI-0 counter
	sysCtrlFlagTemp.extGpioIn5WakeupEn = 1;	//for DI-1 counter
	#endif
	#endif
	//pcal6416GpioIntEnable(sysCtrlFlag.sysCtrlHiWord);
	pcal6416GpioIntEnable(sysCtrlFlagTemp.sysCtrlHiWord);
#endif
	/*
	 * By using a variable and setting it as Output, 
	 * DI can be woken up even when it is in power saving mode; the rest are set as Input.
	*/
	outExpanderIoMode.val = 0xFFFF;
	outExpanderIoMode.diEn = 0;		//for test power consumption, turn off DI
	pcal6416GpioSetIoMode(OUTPUT_EXPANDER_CHANNEL, outExpanderIoMode.val);
	dbgMsg("[PSM] enter power saving, time:%d, time Elaspsed: %d\r\n", standbyTime, timeElapsed);
	dbgMsg("[PSM] next wakeup time: %4d/%02d/%02d %02d:%02d:%02d\r\n", nextWakeupTime.u32Year, nextWakeupTime.u32Month, nextWakeupTime.u32Day, nextWakeupTime.u32Hour, nextWakeupTime.u32Minute, nextWakeupTime.u32Second);
	//for(idx=0;idx<4;idx++)	//clear
	//	setVBatRegValue(RTC_SPARE_REG_ITEM_DI0_COUNTER+idx, 0);
	sysPwrOffCtrl(standbyTime, currTimestamp);
}
//------------------------------------------------------------------------------
static void closeSockBeforReboot(void)
{	CLOUD_SERVER_INSTANCE *serverInst;
	int idx;
	DEV_INSTANCE *devInst;
	MODBUS_INSTANCE	*modbusInst;
	//modbus_t	*modbusCtx;

	for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
	{	serverInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
		if(serverInst->fd != -1)
		{	lanSocketClose(serverInst, 1, __func__, __LINE__);
		}
	}
#if 0	//for close modbus tcp socket
	for(devInst=sensDev->devInsts;devInst;devInst=devInst->next)
	{	if(devInst->interface == EXT_DEV_CHAN_ETH)
		{	modbusInst = getModbusInst(devInst);
			if(modbusInst)
			{	modbusCtx = (modbus_t	*)modbusInst->modbusCtx;
				if(modbusCtx->s != -1)
				{	
	#if defined NET_LWIP
					closesocket(modbusCtx->s);
	#elif defined NET_RTCS
					shutdown(modbusCtx->s, 2);                                                             // 2 = FLAG_ABORT_CONNECTION
	#endif
				}
			}
		}
	}
#endif
}
//------------------------------------------------------------------------------
static void checkTaskWorkingState(void)
{	long currTime;
	
	currTime = sysTimer->currTick;
	
	if((taskAct.networkProcessTaskAct.id != SENS_NULL_TASK_ID) && (taskAct.networkProcessTaskAct.sts == TASK_STS_RUNNING))
	{	if((currTime - taskAct.networkProcessTaskAct.runningStartTime) > WATCHDOG_TIMEOUT_TICK)
		{	dbgMsg("%s", "network Process Task timeout, reboot!!\r\n");
			sysCtrl->isReadyToReboot = 1;
		}
	}
	
	if((taskAct.uvcCameraTaskAct.id != SENS_NULL_TASK_ID) && (taskAct.uvcCameraTaskAct.sts == TASK_STS_RUNNING))
	{	if((currTime - taskAct.uvcCameraTaskAct.runningStartTime) > WATCHDOG_TIMEOUT_TICK)
		{	dbgMsg("%s", "uvc camera Task timeout, reboot!!\r\n");
			sysCtrl->isReadyToReboot = 1;
		}
	}
	
	if((taskAct.servCmdProcessTaskAct.id != SENS_NULL_TASK_ID) && (taskAct.servCmdProcessTaskAct.sts == TASK_STS_RUNNING))
	{	if((currTime - taskAct.servCmdProcessTaskAct.runningStartTime) > WATCHDOG_TIMEOUT_TICK)
		{	dbgMsg("%s", "server cmd process Task timeout, reboot!!\r\n");
			sysCtrl->isReadyToReboot = 1;
		}
	}
	
	if((taskAct.mqttTaskAct.id != SENS_NULL_TASK_ID) && (taskAct.mqttTaskAct.sts == TASK_STS_RUNNING))
	{	if((currTime - taskAct.mqttTaskAct.runningStartTime) > WATCHDOG_TIMEOUT_TICK)
		{	dbgMsg("%s", "mqtt Task timeout, reboot!!\r\n");
			sysCtrl->isReadyToReboot = 1;
		}
	}
	
}
//------------------------------------------------------------------------------
void rebootCheck(void)
{	int idleServIdx;
	
	if(networkCtx && !networkCtx->otaRunning && networkCtx->networkIsUp)
	{	idleServIdx = chkServIdleXmitTimer(sysCtrl->srvActiveBmp, 90000);
		if(idleServIdx && sysStsFlag2.sdRecDone)
		{	dbgMsg("srv %d no xmit over 90 sec, reboot!!\r\n", idleServIdx);
			sysCtrl->isReadyToReboot = 1;
		}
		else if(idleServIdx)
		{	resetServIdleXmitTimer(idleServIdx);
		}
	}
	checkTaskWorkingState();
	
	if((sysCtrl->everydayRebootSecondTime == sysTimer->currTimeSec) && (!sysCfg->sensSysCfg.psm))
	{	dbg_msg("%s", "[SYSTEM] Everyday Reboot\r\n");
		sysCtrl->isReadyToReboot = 1;
	}
	
	if(sysCtrl->isReadyToReboot)
	{	closeSockBeforReboot();
		dbgMsg("%s", "[SYS] system Reboot!!\r\n");
		sysPwrOffCtrl(4, 0);	//reboot 4 second
	}
}
//------------------------------------------------------------------------------
void checkForNextWirelessWakeup(void)
{	struct TaskQMsg msg;
	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	WIRELESS_CONFIG *mobileCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
	int nMobileActive = sysTimer->currHistoryRecTimeSlot % mobileCfg->mobileInterval;
	
	if(networkCtx->checkForNextWlWakeup)
	{	if((sysTimer->currHistoryRecTimeSlot % mobileCfg->mobileInterval) == (mobileCfg->mobileInterval-1))
		{	if(!networkCtx->wlWakeupTimer)
				networkCtx->wlWakeupTimer = GetTimeTicks();
			if((GetTimeTicks() - networkCtx->wlWakeupTimer) > 20000)
			{	networkCtx->wlWakeupTimer = 0;
				networkCtx->timeToEnableNetwork = 1;
				networkCtx->checkForNextWlWakeup = 0;
				msg.msgId = NETWORK_Q_MSG_NETWORK_PRE_INIT;
				msg.ptr = NULL;
				if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
				{	networkCtx->checkForNextWlWakeup = 1;
				}
			}
		}
		else if(sensSys->sysAlarm.prevIsAlarm && sensSys->sysAlarm.alarmFlagIsSet)
		{	networkCtx->timeToEnableNetwork = 1;
			networkCtx->checkForNextWlWakeup = 0;
			msg.msgId = NETWORK_Q_MSG_NETWORK_PRE_INIT;
			msg.ptr = NULL;
			if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
			{	networkCtx->checkForNextWlWakeup = 1;
			}
		}
	}
}
//------------------------------------------------------------------------------
void InternalDiProcess(void)
{	MAINTENANCE_COTEXT  *maintenanceCtx = (MAINTENANCE_COTEXT *)&sysCtrl->maintenanceCtx;
	
	if(sysCtrl->interDi4Active && !sysCtrl->interDi4PrevActive)
	{	sysCtrl->interDi4PrevActive = sysCtrl->interDi4Active;
		setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_DI4_ACTIVE);
		maintenanceCtx->isValid = 1;
	}
	else if(sysCtrl->interDi4ReleasePrepare)
	{	sysCtrl->interDi4Active = 0;
		sysCtrl->interDi4PrevActive = 0;
		sysCtrl->interDi4ReleasePrepare = 0;
		setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_DI4_ACTIVE);
		maintenanceCtx->isValid = 0;
	}
	
	if(sysCtrl->interDi5Active && !sysCtrl->interDi5PrevActive)
	{	sysCtrl->interDi5PrevActive = sysCtrl->interDi5Active;
		setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_DI5_ACTIVE);
	}
	else if(sysCtrl->interDi5ReleasePrepare)
	{	sysCtrl->interDi5Active = 0;
		sysCtrl->interDi5PrevActive = 0;
		sysCtrl->interDi5ReleasePrepare = 0;
		setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_DI5_ACTIVE);
	}
	
	
	if(maintenanceCtx->isValid)
	{	//add process for maintenance cable insert, turn on.......
	}
}
//------------------------------------------------------------------------------
static void alarmStart(SYSTEM_ALARM *sysAlarm)
{	IMAGE_UPLOAD_INSTANCE *imgUploadInst;
	
	sysAlarm->alarmFlagIsSet = 1;
	sysStsFlag2.alarmActive = 1;
	setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG2, sysStsFlag2.sysStatusFlags);
	sysAlarm->prevIsAlarm = 1;
	
	if(sysCfg->cameraCfg)
	{	sysTimer->alertCurrSendImgSlot = sysTimer->currTimeSec / sysCfg->sensSysCfg.cameraAlertInterval;
		sysTimer->alertCurrSendImgSlot--;
		if(networkCtx->imgUploadInst)
		{	imgUploadInst = networkCtx->imgUploadInst;
			imgUploadInst->isAlarmUploadImgMode = 1;
		}
	}
	setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_ALERT_ACTIVE);
	dbg_msg("%s", "warring - PQ[0] over Alarm Value!!, disable power saving mode\r\n");
}
//------------------------------------------------------------------------------
static int alarmPolling(SYSTEM_ALARM *sysAlarm)
{	struct TaskQMsg msg;
	int idx;
	
	if(isnan(sensPq->pqVal[0]))
		return 1;
	if(sensPq->pqVal[0] >= sysCfg->sensSysCfg.alertValue)
	{	if(sysCfg->cameraCfg && isTimeToSendAlertImg())
		{	msg.msgId = SENSOR_Q_MSG_CAMERA_PRE_START;
			msg.ptr = NULL;
			if(sendMsgWithErrHandle(UVC_CAMERA_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
			{
			}
		}
		return 1;
	}
	else if(sensPq->pqVal[0] < (sysCfg->sensSysCfg.alertValue - sysCfg->sensSysCfg.alertRecoveryMargin))
	{	networkCtx->atleastSend1DataBmp = sysCtrl->srvActiveBmp;
		dbg_msg("%s", "restore power saving mode\r\n");
		for(idx=0;idx<UPLOAD_SERVER_CNT;idx++)
		{	if(networkCtx->atleastSend1DataBmp & (1 << idx))
			{	if(idx == 0)
					setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_ALERT_SEND_SRV1_DATA);
				else if(idx == 1)
					setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_ALERT_SEND_SRV2_DATA);
			}
		}
		sysAlarm->waitAlarmSendFinalData = 1;
	}
	return 0;
}
//------------------------------------------------------------------------------
static int prepareAlarmUploadImage(void)
{	struct TaskQMsg msg;
	
	SYS_POWER_MANAGER *pwrManager = (SYS_POWER_MANAGER	*)&sysCtrl->pwrManager;
	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	if(sysCfg->cameraCfg == NULL)
		return 0;
	
	if(CHK_BMP(pwrManager->disablePsmBmp, DISABLE_PSM_BMP_CAMERA_ACTIVE))
		return -1;
	msg.msgId = SENSOR_Q_MSG_CAMERA_PRE_START;
	msg.ptr = NULL;
	if(sendMsgWithErrHandle(UVC_CAMERA_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	
	}
	return 0;
}
//------------------------------------------------------------------------------
void alarmProcess(void)
{	SYSTEM_ALARM *sysAlarm = (SYSTEM_ALARM *)&sensSys->sysAlarm;
	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	
	switch(sysAlarm->alertFsm)
	{	case ALERT_FSM_IDLE:
				if((!sensSysCfg->alertEnable) || isnan(sensSysCfg->alertValue) || (sensSysCfg->psm == 0) || (sensPq->pqNumber == 0))
				{	sysAlarm->alertFsm = ALERT_FSM_MAX;
					break;
				}
				else
					sysAlarm->alertFsm = ALERT_FSM_CHK_ALERT;
		case ALERT_FSM_CHK_ALERT:
				if((!isnan((float)sensPq->pqVal[0])) && (sensPq->pqVal[0] >= sensSysCfg->alertValue) || sysAlarm->prevIsAlarm)
				{	sysAlarm->alertFsm = ALERT_FSM_ALERT_START;
				}
				else
					break;
		case ALERT_FSM_ALERT_START:
				alarmStart(sysAlarm);
				sysAlarm->alertFsm = ALERT_FSM_ALERT_POLLING;
		case ALERT_FSM_ALERT_POLLING:
				if(alarmPolling(sysAlarm))
				{	break;
				}
				sysAlarm->alertFsm = ALERT_FSM_ALERT_STOP;
		case ALERT_FSM_ALERT_STOP:
				if(prepareAlarmUploadImage())
				{	break;
				}
				sysAlarm->alertFsm = ALERT_FSM_ALERT_WAIT_FINAL_SEND;
		case ALERT_FSM_ALERT_WAIT_FINAL_SEND:
				{	IMAGE_UPLOAD_INSTANCE *imgUploadInst = networkCtx->imgUploadInst;
					if(networkCtx->atleastSend1DataBmp)
						break;
					else if(sysCfg->cameraCfg && imgUploadInst && imgUploadInst->alarmUploadFinalImg)
						break;
				}
		case ALERT_FSM_ALERT_FINAL_SEND_DONE:
				sysAlarm->alertFsm = ALERT_FSM_ALERT_FINAL_SEND_DONE;
				sysAlarm->alarmFlagIsSet = 0;
				sysStsFlag2.alarmActive = 0;
				setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG2, sysStsFlag2.sysStatusFlags);
				sysAlarm->prevIsAlarm = 0;
				setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_ALERT_ACTIVE);
				sysAlarm->alertFsm = ALERT_FSM_CHK_ALERT;
		default:
				break;
	}
}
//------------------------------------------------------------------------------
void checkPrevAlarm(void)
{	SYSTEM_ALARM *sysAlarm = (SYSTEM_ALARM *)&sensSys->sysAlarm;
	
	if(sysStsFlag2.alarmActive)
	{	if(sysCfg->sensSysCfg.psm && sysCfg->sensSysCfg.alertEnable && sensPq->pqNumber)
		{	//sysAlarm->prevIsAlarm = 1;
			//sysAlarm->alarmFlagIsSet = 1;
			alarmStart(sysAlarm);
			sysAlarm->alertFsm = ALERT_FSM_ALERT_POLLING;
		}
		else
		{	sysStsFlag2.alarmActive = 0;
			sysAlarm->prevIsAlarm = 0;
		}
	}
	
}
//------------------------------------------------------------------------------
static int chkDiKeepOn(void)
{	DI_INSTANCE	*diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT	*diCtx = (DI_CONTEXT *)&diInst->diCtx;
	int ret = DI_KEEP_ON_STS_RELEASE;
	
	if(sysCtrlFlag.extGpioIn0WakeupEn && diCtx->diStsNew[DI_ID_2])
	{	return DI_KEEP_ON_STS_KEEP_ON;
	}
	
	if(sysCtrlFlag.extGpioIn1WakeupEn && diCtx->diStsNew[DI_ID_3])
	{	return DI_KEEP_ON_STS_KEEP_ON;
	}
	
	if(sysCtrlFlag.extGpioIn4WakeupEn && diCtx->diStsNew[DI_ID_0])
	{	return DI_KEEP_ON_STS_KEEP_ON;
	}
	
	if(sysCtrlFlag.extGpioIn5WakeupEn && diCtx->diStsNew[DI_ID_1])
	{	return DI_KEEP_ON_STS_KEEP_ON;
	}
	return ret;
}
//------------------------------------------------------------------------------
void diWakeupProcess(void)
{	DI_INSTANCE	*diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT	*diCtx = (DI_CONTEXT *)&diInst->diCtx;
	
	if(sysCfg->sensSysCfg.psm == SYS_CFG_PSM_NONE)
		return;
	
	if(sysCtrlFlag.wakeupKeepOn0 || sysCtrlFlag.wakeupKeepOn1)	//DI0~DI4
	{	switch(diCtx->keepOnFsm)
		{	case DI_KEEP_ON_FSM_IDLE:
					if(diCtx->isWakeupFromDi == 0)
					{	diCtx->keepOnFsm = DI_KEEP_ON_FSM_CHK_KEEP_ON;
						break;
					}
			case DI_KEEP_ON_FSM_KEEP_ON_PREPARE:
					setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_DI_WAKEUP);
					diCtx->keepOnFsm = DI_KEEP_ON_FSM_KEEP_ON;
					break;
			case DI_KEEP_ON_FSM_CHK_KEEP_ON:
					if(chkDiKeepOn() == DI_KEEP_ON_STS_KEEP_ON)
					{	diCtx->keepOnFsm = DI_KEEP_ON_FSM_KEEP_ON;
						setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_DI_WAKEUP);
					}
					break;
			case DI_KEEP_ON_FSM_KEEP_ON:
					diCtx->isDiKeepOn = 1;
					if(chkDiKeepOn() == DI_KEEP_ON_STS_RELEASE)
					{	diCtx->keepOnFsm = DI_KEEP_ON_FSM_RELEASE_PREPARE;
					}
					break;
			case DI_KEEP_ON_FSM_RELEASE_PREPARE:
					diCtx->diReleaseStartTimer = GetTimeTicks();
					diCtx->diRelaseFlag = (DI_RELEASE_FLAG_ALL | DI_RELEASE_FLAG_REC | DI_RELEASE_FLAG_SEND);
					diCtx->keepOnFsm = DI_KEEP_ON_FSM_RELEASE_WAIT;
					break;
			case DI_KEEP_ON_FSM_RELEASE_WAIT:
					diCtx->keepOnFsm = DI_KEEP_ON_FSM_RELEASE_WAIT;
					if(diCtx->diRelaseFlag)
						break;
			case DI_KEEP_ON_FSM_RELEASE:
					diCtx->isDiKeepOn = 0;
					if(sysCfg->diCfg.diWakeupRecInterval)
					{	diCtx->keepOnFsm = DI_KEEP_ON_FSM_CHK_RELEASE_TIMER;
					}
					else
					{	//diCtx->keepOnFsm = DI_KEEP_ON_FSM_CHK_KEEP_ON;
						//setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_DI_WAKEUP);
						diCtx->keepOnFsm = DI_KEEP_ON_FSM_SOCKET_SEND_WAIT;
						diCtx->diReleaseStartTimer = GetTimeTicks();
					}
					break;
			case DI_KEEP_ON_FSM_CHK_RELEASE_TIMER:
					if(chkDiKeepOn() == DI_KEEP_ON_STS_KEEP_ON)
					{	diCtx->keepOnFsm = DI_KEEP_ON_FSM_KEEP_ON;
						setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_DI_WAKEUP);
					}
					else
					{	if((GetTimeTicks() - diCtx->diReleaseStartTimer) > (10*60*1000))
						{	diCtx->keepOnFsm = DI_KEEP_ON_FSM_CHK_KEEP_ON;
							setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_DI_WAKEUP);
						}
					}
					break;
			case DI_KEEP_ON_FSM_SOCKET_SEND_WAIT:
					if((GetTimeTicks() - diCtx->diReleaseStartTimer) > (2000))	//wait 2 sec for socket xmit finish
					{	diCtx->keepOnFsm = DI_KEEP_ON_FSM_CHK_KEEP_ON;
						setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_DI_WAKEUP);
					}
					break;
			default:
				break;
		}
	}
}
//------------------------------------------------------------------------------
void checkPrevOtaStatus(void)
{	OTA_MANAGER *otaMng = (OTA_MANAGER *)&sysCtrl->otaMng;
	OTA_REC_CTX *otaRecCtx;
	
	otaRecCtx = sdReadOtaInfo();
	if(otaRecCtx)
	{	if((otaRecCtx->tag == 0x55AA) && (otaRecCtx->retryCnt < 5))
		{	otaMng->prevOtaNotFinish = 1;
		}
		else
			deleteOtaInfoFile();
	}
	
	SENS_MEM_FREE(otaRecCtx);
}
//------------------------------------------------------------------------------
void startTrigOta(void)
{	OTA_MANAGER *otaMng = (OTA_MANAGER *)&sysCtrl->otaMng;
	struct TaskQMsg senstalkMsg;

	sysStsFlag1.otaFlag = OTA_FLAG_TRIG;
	setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1, sysStsFlag1.sysStatusFlags);

	dbg_msg("%s", "[FOTA] FOTA Start.\r\n");

	otaMng->runningAutoFota = 1;
	
	if(otaMng->prevOtaNotFinish)
	{	networkCtx->prevOtaRecCtx = sdReadOtaInfo();
		if(networkCtx->prevOtaRecCtx == NULL)
			otaMng->runningAutoFota = 0;
		else
		{	displayBufInHex((uint8_t *)networkCtx->prevOtaRecCtx, sizeof(OTA_REC_CTX), __func__, __LINE__);
			networkCtx->prevOtaRecCtx->retryCnt++;
		}
		otaMng->prevOtaNotFinish = 0;
	}

	senstalkMsg.msgId = NETWORK_Q_MSG_HTTP_GET;
	senstalkMsg.ptr = NULL;
	if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&senstalkMsg, 0), __func__, __LINE__))
	{
	}
}
//------------------------------------------------------------------------------
int checkPrevOta(void)
{	int isOtaOnTime = 0;
	OTA_MANAGER *otaMng = (OTA_MANAGER *)&sysCtrl->otaMng;
	
	if(otaMng->runningAutoFota)
	{	return isOtaOnTime;
	}
	if(sysStsFlag1.otaFlag == OTA_FLAG_FINISH)
		isOtaOnTime = 1;
	else if(otaMng->prevOtaNotFinish)
		isOtaOnTime = 1;
	
	if(isOtaOnTime)
	{	if(networkCtx && networkCtx->networkIsUp)
		{
		}
		else
			isOtaOnTime = 0;
	}
	
	return isOtaOnTime;
}
//------------------------------------------------------------------------------
int checkOta(void)
{	int isOtaOnTime = 0;
	int fotaTimeSlot1, fotaTimeSlot2;
	OTA_MANAGER *otaMng = (OTA_MANAGER *)&sysCtrl->otaMng;
#if TEST_AUTO_FOTA_FIX_TIME
	#define FOTA_TIME_UP_1_RUN	FOTA_TIME_UP_1_TEST
	#define FOTA_TIME_UP_2_RUN	FOTA_TIME_UP_2_TEST
#else
	#define FOTA_TIME_UP_1_RUN	FOTA_TIME_UP_1
	#define FOTA_TIME_UP_2_RUN	FOTA_TIME_UP_2
#endif
	fotaTimeSlot1 = FOTA_TIME_UP_1_RUN / 60;
	fotaTimeSlot2 = FOTA_TIME_UP_2_RUN / 60;
	
	//if(otaMng->runningAutoFota || ((sensPq->onboardPq[ON_BOARD_PQ_BATTERY_VOLT] <= 12.7f) && (sensPq->onboardPq[ON_BOARD_PQ_EXT_VOLT] < 8.0f)))
	if(otaMng->runningAutoFota)
	{	return isOtaOnTime;
	}
	
	if(sysCfg->sensSysCfg.autoFotaChk && (sysTimer->currHistoryRecTimeSlot == fotaTimeSlot1))
		isOtaOnTime = 1;
	else if(sysCfg->sensSysCfg.autoFotaChk && (sysTimer->currHistoryRecTimeSlot == fotaTimeSlot2))
		isOtaOnTime = 1;
	else if(sysStsFlag1.otaFlag == OTA_FLAG_FINISH)
		isOtaOnTime = 1;
	else if(otaMng->prevOtaNotFinish)
		isOtaOnTime = 1;
	
	if(isOtaOnTime)
	{	if(networkCtx && networkCtx->networkIsUp)
		{
		}
		else
			isOtaOnTime = 0;
	}
	
	return isOtaOnTime;
}

//------------------------------------------------------------------------------
