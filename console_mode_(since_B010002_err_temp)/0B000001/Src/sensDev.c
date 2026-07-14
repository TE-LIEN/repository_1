#include "sensDev.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "physicalQuantity.h"
#include "math.h"
#include "sdCardTask.h"
#include "network/netApp.h"
#include "driver/i2cDrv.h"
#include "driver/i2cEeprom.h"

volatile SENS_DEV_STRUCT *sensDev;
volatile SYS_CTRL_FLAG	sysCtrlFlag;
volatile SYS_WAKE_UP_SRC wakeupSrc;
volatile SYS_RESET_SRC sysResetSrc;
volatile REC_SLOT_FLAG	sysRecSlotFlag;	
volatile SYS_STATUS_FLAG1 sysStsFlag1;
volatile SYS_STATUS_FLAG2 sysStsFlag2;
volatile SYS_STATUS_FLAG2 inputExpVal;

volatile EEPROM_CONTEXT *eepromCtx;

#define FORCE_WRITE_GUID	0

#ifdef NUVOTON
//------------------------------------------------------------------------------
static void createDefaultGuid(void)
{	uint32_t *guidTemp;
	uint8_t *guidTempU8;

	guidTemp = (uint32_t *)&sensSys->guid.guid[0];
	guidTemp[0] = sensSys->guid.ucid[0];
	guidTempU8 = (uint8_t *)&sensSys->guid.guid[0];
	guidTempU8[4] = (sensSys->guid.ucid[1] >> 16) & 0xFF;
	guidTempU8[5] = (sensSys->guid.ucid[1] >> 24) & 0xFF;
	guidTempU8[6] = (sensSys->guid.ucid[1] >> 0) & 0xFF;
	guidTempU8[7] = (sensSys->guid.ucid[1] >> 8) & 0xFF;
	guidTempU8[8] = (sensSys->guid.ucid[2] >> 24) & 0xFF;
	guidTempU8[9] = (sensSys->guid.ucid[2] >> 16) & 0xFF;
	guidTempU8[10] = (sensSys->guid.ucid[2] >> 8) & 0xFF;
	guidTempU8[11] = (sensSys->guid.ucid[2] >> 0) & 0xFF;
	guidTempU8[12] = (sensSys->guid.ucid[3] >> 24) & 0xFF;
	guidTempU8[13] = (sensSys->guid.ucid[3] >> 16) & 0xFF;
	guidTempU8[14] = (sensSys->guid.ucid[3] >> 8) & 0xFF;
	guidTempU8[15] = (sensSys->guid.ucid[3] >> 0) & 0xFF;
	sensSys->guid.isDefault = 1;
}
//------------------------------------------------------------------------------
void getGuidFromOtp(void)
{	uint32_t  u32OtpHw = 0, u32OtpLw = 0;
	uint32_t idx;
	uint32_t *guidTemp;
	uint32_t guidIdx;
#ifdef TEST_WRITE_GUID_TO_OTP
	uint8_t writeGuidToOtp = 0;
#endif
	int	ret;
	
	//SENS_TIME_DELAY(2000);
	
	//for(idx = 240;idx < FMC_OTP_ENTRY_CNT;idx++)
	idx = OTP_GUID_START_ADDR;
	{	if(FMC_ReadOTP(idx, &u32OtpLw, &u32OtpHw) != 0)
		{	dbgMsg("%s", "Read OTP Fail, create default GUID!!\r\n");
			createDefaultGuid();
			return;
		}
		if((u32OtpLw == 0xFFFFFFFF) && (u32OtpHw == 0xFFFFFFFF))
		{	
#if defined TEST_WRITE_GUID_TO_OTP
			writeGuidToOtp = 1;
#else
			createDefaultGuid();
			return;
#endif
		}
	}
	
#ifdef TEST_WRITE_GUID_TO_OTP
	if(writeGuidToOtp)
	{	dbgMsg("%s", "write new guid to OTP\r\n");
		createDefaultGuid();
		guidTemp = (uint32_t *)&sensSys->guidParams.guid[0];
		guidIdx = 0;
		for(idx=OTP_GUID_START_ADDR;idx<OTP_GUID_START_ADDR+2;idx++)
		{	if(FMC_WriteOTP(idx, guidTemp[guidIdx*2], guidTemp[(guidIdx*2)+1]) != 0)
			{	dbgMsg("otp write fail, idx:%d\r\n", idx);
				createDefaultGuid();
				return;
			}
			FMC_ReadOTP(idx, &u32OtpLw, &u32OtpHw);
			if((u32OtpLw != guidTemp[guidIdx*2]) || (u32OtpHw != guidTemp[(guidIdx*2)+1]))
			{	dbgMsg("otp read fail, idx:%d, low :%08X, %08X, high : %08X, %08X\r\n", idx, u32OtpLw, guidTemp[guidIdx*2], u32OtpHw, guidTemp[(guidIdx*2)+1]);
				createDefaultGuid();
				return;
			}
			ret = FMC_IsOTPLocked(idx);
			if(ret == 0)
			{	dbgMsg("otp is not lock, idx:%d\r\n", idx);
				if(FMC_LockOTP(idx) != 0)
				{	dbgMsg("otp lock fail, idx:%d\r\n", idx);
					createDefaultGuid();
					return;
				}
				
				FMC_ReadOTP(idx, &u32OtpLw, &u32OtpHw);
				if((u32OtpLw != guidTemp[guidIdx*2]) || (u32OtpHw != guidTemp[(guidIdx*2)+1]))
				{	dbgMsg("otp read fail after lock, idx:%d, low :%08X, %08X, high : %08X, %08X\r\n", idx, u32OtpLw, guidTemp[guidIdx*2], u32OtpHw, guidTemp[(guidIdx*2)+1]);
					createDefaultGuid();
					return;
				}
			}
			guidIdx++;
		}
		
		dbgMsg("%s", "guid create done\r\n");
	}
#endif
	guidTemp = (uint32_t *)&sensSys->guid.guid[0];
	guidIdx = 0;
	for(idx=OTP_GUID_START_ADDR;idx<OTP_GUID_START_ADDR+2;idx++)
	{	FMC_ReadOTP(idx, &u32OtpLw, &u32OtpHw);
		guidTemp[guidIdx*2] = u32OtpLw;
		guidTemp[(guidIdx*2)+1] = u32OtpHw;
		guidIdx++;
	}
}
//------------------------------------------------------------------------------
#if FORCE_WRITE_EEPROM_INFO_WHEN_ERROR
//------------------------------------------------------------------------------
void setEepromInfo(uint32_t startAddr, uint8_t *buf)
{	i2cInit(EEPROM_I2C_ID, 100000, sizeof(EEPROM_CONTEXT));
	i2cEepromWriteData(startAddr, buf, sizeof(EEPROM_CONTEXT));
	i2cDeInit(EEPROM_I2C_ID);
}
//------------------------------------------------------------------------------
static void restoreEepromGuid(void)
{	uint16_t crc16;
	uint8_t *buf;
	EEPROM_GUID_CONTEXT *guidCtx;
	
	sensSys->guid.ucid[0] = FMC_ReadCID();
	sensSys->guid.ucid[1] = FMC_ReadUID(0);
	sensSys->guid.ucid[2] = FMC_ReadUID(1);
	sensSys->guid.ucid[3] = FMC_ReadUID(2);
	getGuidFromOtp();
	
	buf = SENS_MEM_ZALLOC(sizeof(EEPROM_GUID_CONTEXT));
	guidCtx = (EEPROM_GUID_CONTEXT *)buf;
	
	memcpy(guidCtx->guid, (void *)sensSys->guid.guid, 16);
	crc16 = swCrc16(buf, 0, sizeof(EEPROM_GUID_CONTEXT)-4);
	guidCtx->guidCrc16 = crc16;
	
	setEepromInfo(EEPROM_GUID_ADDR, buf);
	
	SENS_MEM_FREE(buf);
}
//------------------------------------------------------------------------------
static void restoreEepromCtx(void)
{	uint16_t crc16;
	memset(sensSys->eepromBuf, 0, sizeof(EEPROM_CONTEXT));
	
	eepromCtx->version = EEPROM_STRUCT_VERSION;
	eepromCtx->tag = EEPROM_STRUCT_TAG;
	eepromCtx->ipAddr[0] = 192;
	eepromCtx->ipAddr[1] = 168;
	eepromCtx->ipAddr[2] = 255;
	eepromCtx->ipAddr[3] = 1;
	eepromCtx->ipMask[0] = 255;
	eepromCtx->ipMask[1] = 255;
	eepromCtx->ipMask[2] = 255;
	eepromCtx->ipMask[3] = 0;
	eepromCtx->ipRouter[0] = 192;
	eepromCtx->ipRouter[1] = 168;
	eepromCtx->ipRouter[2] = 255;
	eepromCtx->ipRouter[3] = 254;
	eepromCtx->diMap[0] = DI_NO_WIRED;
	eepromCtx->diMap[1] = DI_NO_WIRED;
	eepromCtx->diMap[2] = 0;
	eepromCtx->diMap[3] = 1;
	eepromCtx->diMap[4] = 4;
	eepromCtx->diMap[5] = 5;
	eepromCtx->diMap[6] = -1;
	eepromCtx->doMap[0] = DO_NO_WIRED;
	eepromCtx->doMap[1] = DO_NO_WIRED;
	eepromCtx->doMap[2] = 0;
	eepromCtx->doMap[3] = 1;
	//eepromCtx->doMap[4] = 0;
	//eepromCtx->doMap[5] = 1;
	eepromCtx->extDiCnt = 2;
	eepromCtx->extDoCnt = 2;
	
	crc16 = swCrc16(sensSys->eepromBuf, 0, sizeof(EEPROM_CONTEXT)-2);
	eepromCtx->eepromCrc16 = crc16;
	
	setEepromInfo(EEPROM_CONTEXT_ADDR, sensSys->eepromBuf);
}
#endif
//------------------------------------------------------------------------------
int getEepromInfo(void)
{	uint16_t crc16;
	uint8_t retryCnt = 2;
	int8_t crcError = 0;
	uint8_t *guidBuf;
	EEPROM_GUID_CONTEXT *guidCtx;
	int idx;
	
	//get guid
	guidBuf = SENS_MEM_ZALLOC(sizeof(EEPROM_GUID_CONTEXT));
RE_GET_GUID:
	i2cInit(EEPROM_I2C_ID, 100000, sizeof(EEPROM_GUID_CONTEXT));
	i2cEepromReadData(EEPROM_GUID_ADDR, guidBuf, sizeof(EEPROM_GUID_CONTEXT));
	i2cDeInit(EEPROM_I2C_ID);
	
	guidCtx = (EEPROM_GUID_CONTEXT *)guidBuf;
	crc16 = swCrc16(guidBuf, 0, sizeof(EEPROM_GUID_CONTEXT)-4);	//with pad
	if(crc16 != guidCtx->guidCrc16)
	{	retryCnt--;
		if(retryCnt)
			goto RE_GET_GUID;
		else
		{	crcError = 1;
		}
	}
#if FORCE_WRITE_GUID
	crcError = 1;
#endif
	
	SENS_MEM_FREE(guidBuf);
	
	if(crcError)
	{	
#if FORCE_WRITE_EEPROM_INFO_WHEN_ERROR
		restoreEepromGuid();
#else
		return -1;
#endif
	}
	memcpy((void *)sensSys->guid.guid, guidCtx->guid, 16);
	sensSys->guid.guidString = SENS_MEM_ZALLOC(37);
	
	SENS_SPRINTF(sensSys->guid.guidString + strlen(sensSys->guid.guidString), "%02x%02x%02x%02x-", sensSys->guid.guid[3], sensSys->guid.guid[2], sensSys->guid.guid[1], sensSys->guid.guid[0]);
	SENS_SPRINTF(sensSys->guid.guidString + strlen(sensSys->guid.guidString), "%02x%02x-", sensSys->guid.guid[5], sensSys->guid.guid[4]);
	SENS_SPRINTF(sensSys->guid.guidString + strlen(sensSys->guid.guidString), "%02x%02x-", sensSys->guid.guid[7], sensSys->guid.guid[6]);
	for(idx = 8; idx < 16; idx++)
	{	if(idx == 10)
			strcat(sensSys->guid.guidString, "-");
		SENS_SPRINTF(sensSys->guid.guidString + strlen(sensSys->guid.guidString), "%02x", sensSys->guid.guid[idx]);
	}
	
	
	if(sensSys->eepromBuf == NULL)
		sensSys->eepromBuf = SENS_MEM_ZALLOC(sizeof(EEPROM_CONTEXT));
	
	eepromCtx = (EEPROM_CONTEXT *)sensSys->eepromBuf;
	retryCnt = 2;
	crcError = 0;
RE_GET:
	i2cInit(EEPROM_I2C_ID, 100000, sizeof(EEPROM_CONTEXT));
	i2cEepromReadData(EEPROM_CONTEXT_ADDR, sensSys->eepromBuf, sizeof(EEPROM_CONTEXT));
	i2cDeInit(EEPROM_I2C_ID);
	
	
	crc16 = swCrc16(sensSys->eepromBuf, 0, sizeof(EEPROM_CONTEXT)-2);
	
	if(crc16 != eepromCtx->eepromCrc16)
	{	retryCnt--;
		if(retryCnt)
			goto RE_GET;
		else
		{	crcError = 1;
		}
	}
	if(crcError)
	{	
#if FORCE_WRITE_EEPROM_INFO_WHEN_ERROR
		restoreEepromCtx();
#else
		return -1;
#endif
	}
	if((eepromCtx->tag == EEPROM_STRUCT_TAG) && (eepromCtx->version == EEPROM_STRUCT_VERSION))
	{	return 0;
	}
	return -1;
}
//------------------------------------------------------------------------------
void addUartCfg(UART_CONFIG *uartCfg)
{	
#ifdef SENSMINIS2
#if defined SUPPORT_USB_DEVICE || defined USE_USB_HOST_LIB
	if(uartCfg->id == UART_USB)
		sensDev->uartCfgs[5] = uartCfg;
	else if(uartCfg->id >= UART_USB_VENDOR_AT)
		sensDev->uartCfgs[8+(uartCfg->id - UART_USB_VENDOR_AT)] = uartCfg;
	else
#endif
#ifdef SUPPORT_BLE
	if(uartCfg->id == UART_BLE)
		sensDev->uartCfgs[6] = uartCfg;
	else
#endif
#endif
		sensDev->uartCfgs[uartCfg->id] = uartCfg;
}
//------------------------------------------------------------------------------
void delUartCfg(UART_CONFIG *uartCfg)
{	
#ifdef SENSMINIS2
#if defined SUPPORT_USB_DEVICE || defined USE_USB_HOST_LIB
	if(uartCfg->id == UART_USB)
		sensDev->uartCfgs[5] = NULL;
	else if(uartCfg->id >= UART_USB_VENDOR_AT)
		sensDev->uartCfgs[8+(uartCfg->id - UART_USB_VENDOR_AT)] = NULL;
	else
#endif
#ifdef SUPPORT_BLE
	if(uartCfg->id == UART_BLE)
		sensDev->uartCfgs[6] = NULL;
	else
#endif
#endif
		sensDev->uartCfgs[uartCfg->id] = NULL;
}
#endif

//------------------------------------------------------------------------------
void *getExtDevSpeConfig(int devModel)
{	EXT_DEV_CONFIG *extDevCfg;
	
	for(extDevCfg = sysCfg->extDevCfgs;extDevCfg;extDevCfg=extDevCfg->next)
	{	if(extDevCfg->devModel == devModel)
		{	break;
		}
	}
	if(extDevCfg == NULL)
		return NULL;
	else
		return extDevCfg->speDevCfg;
}
//------------------------------------------------------------------------------
COMM_IF_CONFIG *getCommIfConfigByIfNo(int iFaceNo)
{	COMM_IF_CONFIG *commIfCfg;
	
	for(commIfCfg = sysCfg->commIfCfgs;commIfCfg;commIfCfg = commIfCfg->next)
	{	if(commIfCfg->channelNo == iFaceNo)
			break;
	}
	
	return commIfCfg;
}
//------------------------------------------------------------------------------
void addExtDevConfig(EXT_DEV_CONFIG *extDevCfg)
{	EXT_DEV_CONFIG *extDevCfgTemp;
	
	if(sysCfg->extDevCfgs == NULL)
	{	sysCfg->extDevCfgs = extDevCfg;
	}
	else 
	{	extDevCfgTemp = sysCfg->extDevCfgs;
		while(extDevCfgTemp->next)
		{	extDevCfgTemp = extDevCfgTemp->next;
		}
		extDevCfgTemp->next = extDevCfg;
	}
}
//------------------------------------------------------------------------------
EXT_DEV_CONFIG *getExtDevCfgByIdx(int idx)
{	EXT_DEV_CONFIG *extDevCfg;
	
	for(extDevCfg = sysCfg->extDevCfgs;extDevCfg;extDevCfg=extDevCfg->next)
	{	if(extDevCfg->devIdx == idx)
		{	break;
		}
	}
	return extDevCfg;
}
//------------------------------------------------------------------------------
EXT_DEV_CONFIG *getExtDevCfgByProtocol(int protocol)
{	EXT_DEV_CONFIG *extDevCfg;
	
	for(extDevCfg = sysCfg->extDevCfgs;extDevCfg;extDevCfg=extDevCfg->next)
	{	if(extDevCfg->ifProtocol == protocol)
		{	break;
		}
	}
	return extDevCfg;
}
//------------------------------------------------------------------------------
void specExtDevBind(void)
{	EXT_DEV_CONFIG *extDevCfg;
	
	/*if(sysCfg->compositeSiemensCfg)
	{	if((sysCfg->compositeSiemensCfg->radarDevNo != -1) && (sysCfg->compositeSiemensCfg->wlsDevNo != -1))
		{
		}
		else if(sysCfg->compositeSiemensCfg->radarDevNo != -1)
		{
		}
	}*/
	if(sysCfg->compositeOsaCfg)
	{	if(sysCfg->compositeOsaCfg->radarDevNo)
		{	extDevCfg = getExtDevCfgByIdx(sysCfg->compositeOsaCfg->radarDevNo);
			if(extDevCfg == NULL)
				goto CHECK_COMPOSITE_PWLS;
			extDevCfg->speDevCfg = (void *)sysCfg->compositeOsaCfg;
			extDevCfg->devModel = EXT_DEV_MODEL_COMPOSITE_OSA24G;
		}
CHECK_COMPOSITE_PWLS:
		if(sysCfg->compositeOsaCfg->wlsDevNo)
		{	extDevCfg = getExtDevCfgByIdx(sysCfg->compositeOsaCfg->wlsDevNo);
			if(extDevCfg)
			{	extDevCfg->speDevCfg = (void *)sysCfg->compositeOsaCfg;
				extDevCfg->devModel = EXT_DEV_MODEL_COMPOSITE_OSA24G;
			}
		}
	}
	if(sysCfg->vwCfg)
	{	extDevCfg = getExtDevCfgByIdx(sysCfg->vwCfg->devNo);
		if(extDevCfg == NULL)
		{	SENS_MEM_FREE(sysCfg->vwCfg);
			sysCfg->vwCfg = NULL;
		}
		else
		{	extDevCfg->speDevCfg = (void *)sysCfg->vwCfg;
			extDevCfg->devModel = EXT_DEV_MODEL_VW;
		}
	}
	if(sysCfg->osa24gCfg)
	{	extDevCfg = getExtDevCfgByIdx(sysCfg->osa24gCfg->devNo);
		if(extDevCfg == NULL)
		{	SENS_MEM_FREE(sysCfg->osa24gCfg);
			sysCfg->osa24gCfg = NULL;
		}
		else
		{	extDevCfg->speDevCfg = (void *)sysCfg->osa24gCfg;
			extDevCfg->devModel = EXT_DEV_MODEL_OSA24G;
		}
	}
	if(sysCfg->ar77Cfg)
	{	extDevCfg = getExtDevCfgByProtocol(EXT_IF_PROTO_AR77_RADAR);
		if(extDevCfg == NULL)
		{	SENS_MEM_FREE(sysCfg->ar77Cfg);
			sysCfg->ar77Cfg = NULL;
		}
		else
		{	extDevCfg->speDevCfg = (void *)sysCfg->ar77Cfg;
			extDevCfg->devModel = EXT_DEV_MODEL_OSA77G;
		}
	}
	if(sysCfg->avdsCfg)
	{	extDevCfg = getExtDevCfgByProtocol(EXT_IF_PROTO_AVDS_RADAR);
		if(extDevCfg == NULL)
		{	SENS_MEM_FREE(sysCfg->avdsCfg);
			sysCfg->avdsCfg = NULL;
		}
		else
		{	extDevCfg->speDevCfg = (void *)sysCfg->avdsCfg;
			extDevCfg->devModel = EXT_DEV_MODEL_AVDS;
		}
	}
	if(sysCfg->cameraCfg)
	{	extDevCfg = getExtDevCfgByProtocol(EXT_IF_PROTO_CAMERA);
		if(extDevCfg == NULL)
		{	SENS_MEM_FREE(sysCfg->cameraCfg);
			sysCfg->cameraCfg = NULL;
		}
		else
		{	extDevCfg->speDevCfg = (void *)sysCfg->cameraCfg;
			extDevCfg->devModel = EXT_DEV_MODEL_CAMERA;
		}
	}
}
//------------------------------------------------------------------------------
int setSwPq(DEV_INSTANCE *devInst, SENSOR_HW_PQ_STRUCT *topHwPqInf)
{	int swPqIdx;
	int hwPqMeasureIdx;
	SW_PQ_MAPPING *swPqMap;
	SENSOR_HW_PQ_STRUCT *hwPqInf;
	int success = -1;
	
	for(swPqMap = devInst->swPqMapping;swPqMap;swPqMap=swPqMap->next)
	{	swPqIdx = swPqMap->swPqIdx;
		hwPqMeasureIdx = swPqMap->hwPqMeasureIdx;
		for(hwPqInf = topHwPqInf;hwPqInf;hwPqInf = hwPqInf->next)
		{	if(hwPqInf->hwPqIdx == hwPqMeasureIdx)
			{	hwPqInf->isHwPqReady = 0;
				hwPqInf->isHwFilterPqRdy = 0;
					
				putValToMapPqData(devInst->extDevNo, hwPqInf->hwPqIdx, hwPqInf->pqVal);
				success = 0;
				break;
			}
		}
	}
	return success;
}
//------------------------------------------------------------------------------
int clearImgRecInfo(IMAGE_UPLOAD_INFO *imageInfo)
{	IMAGE_UPLOAD_INFO *imageInfoTemp;
	int idx, idx1;
	IMAGE_UPLOAD_REC_HEADER *imgUploadRecHeader;	//iamgeSendRec;
	IMAGE_UPLOAD_INSTANCE *imgUploadInst;
	int tempCount;
	SYSTEM_ALARM *sysAlarm;
	CAMERA_WEB_MANAGER *camWebMng = (CAMERA_WEB_MANAGER *)&sysCtrl->camWebMng;

	if(networkCtx->imgUploadInst == NULL)
		return -1;
	
	imgUploadInst = networkCtx->imgUploadInst;
	imgUploadRecHeader = imgUploadInst->imgUploadRecHeader;
	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	tempCount = imgUploadRecHeader->numOfUnSendImage;
	for(idx=0;idx<imgUploadRecHeader->numOfUnSendImage;idx++)
	{	imageInfoTemp = (IMAGE_UPLOAD_INFO *)&imgUploadInst->imgUploadRecInfoBuf[idx * idx*sizeof(IMAGE_UPLOAD_INFO)];
		if(imageInfoTemp->valid)
		{	if(imageInfoTemp->unixTime == imageInfo->unixTime)
			{	tempCount--;
				idx1 = idx+1;
				if(idx1 != imgUploadRecHeader->numOfUnSendImage)
				{	memcpy(&imgUploadInst->imgUploadRecInfoBuf[idx*sizeof(IMAGE_UPLOAD_INFO)], &imgUploadInst->imgUploadRecInfoBuf[idx1*sizeof(IMAGE_UPLOAD_INFO)], tempCount *sizeof(IMAGE_UPLOAD_INFO));
				}
				if(!camWebMng->readyToSendImgToCloud || camWebMng->imgUploadToWebDone)
				{	sdFileDelete(imageInfo->fileName);
				}
				break;
			}
		}
		else
		{	tempCount--;
			idx1 = idx+1;
			if(idx1 != imgUploadRecHeader->numOfUnSendImage)
			{	memcpy(&imgUploadInst->imgUploadRecInfoBuf[idx*sizeof(IMAGE_UPLOAD_INFO)], &imgUploadInst->imgUploadRecInfoBuf[idx1*sizeof(IMAGE_UPLOAD_INFO)], tempCount *sizeof(IMAGE_UPLOAD_INFO));
			}
		}
	}
	imgUploadRecHeader->numOfUnSendImage = tempCount;
	sdWriteFile(IMG_REC_FILE_NAME, (char *)imgUploadRecHeader, sizeof(IMAGE_UPLOAD_REC_HEADER), 0, OVER_WRITE_MODE);
	imgUploadInst->imgUploadRecInfoBufLen = imgUploadRecHeader->numOfUnSendImage * sizeof(IMAGE_UPLOAD_INFO);
	if(imgUploadInst->imgUploadRecInfoBufLen)
	{	sdWriteFile(IMG_REC_FILE_NAME, (char *)imgUploadInst->imgUploadRecInfoBuf, imgUploadInst->imgUploadRecInfoBufLen, sizeof(IMAGE_UPLOAD_REC_HEADER), OVER_WRITE_MODE);
	}
	if(!tempCount)
	{	clearAllJpegFile();
		setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_IMAGE_UPLOAD);
	}
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
	setUploadImgFsmChg(imgUploadInst, IMG_UPLOAD_FSM_IDLE, 1000);
	//imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_IDLE;
	dbg_msg("send one done, remain %d Image to send!! run-time memory remain :%ld\r\n", tempCount, xPortGetFreeHeapSize());

	if(imgUploadInst->alarmUploadFinalImg)
	{	imgUploadInst->isAlarmUploadImgMode = 0;	//for no power saving mode coutinue send image at rec time.
		imgUploadInst->alarmUploadFinalImg = 0;
	}

	return tempCount;
}
//------------------------------------------------------------------------------
void loadImgSendRecInfFromSd(IMAGE_UPLOAD_INSTANCE *imgUploadInst)
{	IMAGE_UPLOAD_REC_HEADER *imgUploadRecHeader;
	int fileLength;
	int rwLen;

	imgUploadRecHeader = imgUploadInst->imgUploadRecHeader;

	fileLength = getFileLength(IMG_REC_FILE_NAME);
	if(fileLength)
	{	rwLen = sdReadFile(IMG_REC_FILE_NAME, (char *)imgUploadRecHeader, sizeof(IMAGE_UPLOAD_REC_HEADER), 0, NORMAL_READ_MODE);
		if(rwLen != sizeof(IMAGE_UPLOAD_REC_HEADER))
		{	dbg_msg("read image send File fail (%d/%d)\r\n", rwLen, sizeof(IMAGE_UPLOAD_REC_HEADER));
		}
		if(imgUploadRecHeader->tag == 0x55AA)
		{	imgUploadInst->imgUploadRecInfoBufLen = sizeof(IMAGE_UPLOAD_INFO) * imgUploadRecHeader->numOfUnSendImage;

			if(imgUploadInst->imgUploadRecInfoBufLen)
			{	rwLen = sdReadFile(IMG_REC_FILE_NAME, (char *)imgUploadInst->imgUploadRecInfoBuf, imgUploadInst->imgUploadRecInfoBufLen, sizeof(IMAGE_UPLOAD_REC_HEADER), NORMAL_READ_MODE);
				if(rwLen != imgUploadInst->imgUploadRecInfoBufLen)
				{	dbg_msg("read image send File fail (%d/%d)\r\n", rwLen, imgUploadInst->imgUploadRecInfoBufLen);
				}
			}
		}
		else
		{	imgUploadInst->imgUploadRecInfoBufLen = 0;
			sdFileDelete(IMG_REC_FILE_NAME);
			imgUploadRecHeader->valid = 1;
			imgUploadRecHeader->tag = 0x55AA;
			imgUploadRecHeader->numOfUnSendImage = 0;
		}
	}
	else	//no rec file, first use
	{	imgUploadRecHeader->valid = 1;
		imgUploadRecHeader->tag = 0x55AA;
		imgUploadInst->imgUploadRecInfoBufLen = 0;
	}
}
//------------------------------------------------------------------------------
int checkUnSendImg(IMAGE_UPLOAD_INSTANCE *imgUploadInst)
{	IMAGE_UPLOAD_REC_HEADER *imgUploadRecHeader;

	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	imgUploadRecHeader = imgUploadInst->imgUploadRecHeader;
	if(!imgUploadRecHeader->valid)	//copy from file
	{	loadImgSendRecInfFromSd(imgUploadInst);
	}

	dbg_msg("check, remain %d Image to send!!\r\n", imgUploadRecHeader->numOfUnSendImage);
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
  return 0;
}
//------------------------------------------------------------------------------
int findDiMap(int diNumber)
{	int idx;
	for(idx=0;idx<DIQUANTITY;idx++)
	{	if(eepromCtx->diMap[idx] == diNumber)
		{	return (int)eepromCtx->diMap[idx];
		}
	}
	return DI_NO_WIRED;
}
#ifdef NUVOTON
// RTC VBat Reg
//------------------------------------------------------------------------------
uint32_t getVBatRegValue(uint32_t num)
{	return RTC->SPR[num];
}
//------------------------------------------------------------------------------
void setVBatRegValue(uint32_t num, uint32_t value)
{	RTC->SPR[num] = value;
}
//------------------------------------------------------------------------------
#endif
