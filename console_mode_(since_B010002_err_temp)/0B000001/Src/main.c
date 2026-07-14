#include "sensminiCfg.h"
#include "sensSystem.h"
#include "sensCommon.h"
#include <stdio.h>
#include <stdlib.h>
#include "rtcDateTime.h"
#include "network/netApp.h"
#include "network/protocol/serverProtocol.h"
#include "sdCardTask.h"
#include "param.h"
#include "sensDev.h"
#include "sensors/sensorTask.h"
#include "physicalQuantity.h"
#include "powerCtrl.h"
#if SPI_FILE_SYSTEM
#include "littleFs/littleFsPort.h"
#endif

#ifdef NET_LWIP
#include "lwipopts.h"
#include "netif/ethernet.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"

#include "lwip/dns.h"

//#ifdef SUPPORT_SNTP
#include "lwip/apps/sntp.h"
//#endif

#endif

#include "driver/s35710WakeupTimer.h"
#include "driver/PCAL6416GpioExpander.h"
#include "driver/tcn75a.h"
#include "driver/i2cEeprom.h"
#include "driver/spiNorFlash.h"
#include "driver/ad7124Adc.h"
#include "expanderGpioTask.h"

#if SUPPORT_NUVOTON_USB_HOST
#include "usbHost/usbHostTask.h"
#endif

#ifdef OS_FREERTOS
	#ifdef NUVOTON
	
		#if defined SENSMINIA4_PRO || defined SENSMINIA4_NEO
				
#define INT_RAM_HEAP_SIZE			(128 * 1024)
#define EXT_SRAM_START_ADDR			((uint8_t *)HYPERRAM_BASE)
#define EXT_SRAM_SIZE				(8 * 1024 * 1024)
		#elif defined SENSMINIS2
#define INT_RAM_HEAP_SIZE			(176 * 1024)
#define EXT_SRAM_START_ADDR			((uint8_t *)EBI_BANK0_BASE_ADDR)
#define EXT_SRAM_SIZE				(512 * 1024)
		#endif
	
// __attribute__ ((aligned(8))) static uint8_t iRamHeap[INT_RAM_HEAP_SIZE];

HeapRegion_t xHeapRegions[]= 
		{	// {	iRamHeap, 						INT_RAM_HEAP_SIZE}, 	//internal RAM 0x200001C0 ??64KB
			{	EXT_SRAM_START_ADDR,			EXT_SRAM_SIZE}, 	//External RAM 0x200001C0 ??64KB
			{	NULL,							0}
		};
	
	#endif	//end of #ifdef NUVOTON
#endif	//end of #ifdef FREERTOS

const SENS_TASK_TEMPLATE  taskTemplateList[] =
{
// #ifdef FILESYSTEM_USE_TASK
#if 1
	{	SDCARD_TASK,			//Task Index
		sdcardTask,				//Function	
		SDCARD_TASK_STACK_SIZE,	//Stack
		SDCARD_TASK_PRIORITY,	//Priority
		"SDcard",				//Name
		0,						//Attributes
		0,						//Param
		0						//Time Slice
	},
#endif
	{	MAIN_TASK,				//Task Index
		mainTask,				//Function
		MAIN_TASK_STACK_SIZE,	//Stack
		MAIN_TASK_PRIORITY,		//Priority
		"MAIN_TASK",			//Name
		MQX_AUTO_START_TASK,	//Attributes
		0,						//Param
		0						//Time Slice
	},
	{	SERV_CMD_PROCESS_TASK,	
		serverCmdProcessTask,  				
		SERV_CMD_PROCESS_TASK_STACK_SIZE,					
		SERV_CMD_PROCESS_TASK_PRIORITY,			
		"SERV_CMD_PROCESS",	   				
		0,			   			
		0,			
		0			
	},
	{	NETWORK_PROCESS_TASK,			
		networkProcessTask,  			
		NETWORK_PROCESS_TASK_STACK_SIZE,				
		NETWORK_PROCESS_TASK_PRIORITY,		
		"NETWORK_PROCESS",				
		0,						
		0,			
		0			
	},
	{ 	NETWORK_RECV_TASK,			
		networkRecvTask,  			
		NETWORK_RECV_TASK_STACK_SIZE,				
		NETWORK_RECV_TASK_PRIORITY,		
		"NETWORK_RECV",				
		0,						
		0,			
		0			
	},
	{	MOBILE_RECV_TASK,				
		iotAtRecvTask,  			
		MOBILE_RECV_TASK_STACK_SIZE,				
		MOBILE_RECV_TASK_PRIORITY,		
		"MOBILE_RECV",				
		0,						
		0,			
		0			
	},
	//{ CMUX_TASK,					cMuxTask,  					CMUX_TASK_STACK_SIZE,						CMUX_TASK_PRIORITY,				"CMUX_TASK",				0,						0,			0			},
	{	SENSORS_TASK, 				
		sensorTask,					
		SENSORS_TASK_STACK_SIZE,					
		SENSORS_TASK_PRIORITY, 			
		"SENSOR_TASK",				
		0, 						
		0,			
		0			
	},
	//not ready
	{	MQTT_TASK,					
		mqtt_task,					
		MQTT_TASK_STACK_SIZE,						
		MQTT_TASK_PRIORITY,				
		"MQTT",						
		0,						
		0,			
		0			
	},
	{	UVC_CAMERA_TASK,					
		uvcCameraTask,					
		UVC_CAMERA_TASK_STACKSIZE,						
		UVC_CAMERA_TASK_PRIORITY,				
		"MQTT",						
		0,						
		0,			
		0			
	},
	//{ NETWORK_TASK,					network_task,				NETWORK_TASK_STACK_SIZE,					NETWORK_TASK_PRIORITY,			"NETWORK",					0,						0,			0			},
#if SUPPORT_NUVOTON_USB_HOST
	{	USB_HOST_TASK,				//Task Index
		usbHostTask,				//Function
		USB_HOST_TASK_STACK_SIZE,	//Stack
		USB_HOST_TASK_PRIORITY,		//Priority
		"USB_HOST_TASK",			//Name
		0,							//Attributes
		0,							//Param
		0							//Time Slice
	},
#endif
#if DI_USE_IRQ
	{	EXPANDER_TASK,
		expanderTask,
		EXPANDER_TASK_STACKSIZE,
		EXPANDER_TASK_PRIORITY,
		"EXPANDER_TASK",
		0,
		0,
		0
	},
#endif
	{ 0 }
};

extern void test_loop();
volatile TASK_ACTIVE taskAct={0};

//int gPppMru /*= PPP_MRU*/;

//char patternBuffer[4096];
//int bufPtrInfo[4096];
extern volatile int gWriteJpg2File;
//int wtiteJpgCnt = 0;
extern uint8_t *snapshotBufPool;
extern volatile int gSnapshotLen;
extern volatile int gDoSnapshot;

int errno;


void initSysSem(void);
void initialViriable(void);

void initSysSem(void)
{	SYS_SEM *sysSem = (SYS_SEM *)&sensSys->sysSem;
	if(SENS_SEM_INIT(sysSem->dbgUart, 1) != 0)
	{
	}
	if(SENS_SEM_INIT(sysSem->strtok, 1) != 0)
	{
	}
	if(SENS_SEM_INIT(sysSem->sdWriteLock, 1) != MQX_OK)
	{
	}
	if(SENS_SEM_INIT(sysSem->sdLock, 1) != MQX_OK)
	{
	}
	if(SENS_SEM_INIT(sysSem->fsLock, 1) != MQX_OK)
	{
	}
	if(SENS_SEM_INIT(sysSem->webDbgMsgLock, 1) != MQX_OK)
	{
	}
	if(SENS_SEM_INIT(sysSem->spiFsLock, 1) != MQX_OK)
	{
	}
	if(SENS_SEM_INIT(sysSem->disablePsmLock, 1) != MQX_OK)
	{
	}
}
//------------------------------------------------------------------------------
void initialViriable(void)
{	DEBUG_STRUCT *dbg;

	sensSys = SENS_MEM_ZALLOC(sizeof(SENS_SYSTEM_STRUCT));
	networkCtx = SENS_MEM_ZALLOC(sizeof(NETWORK_CONTEXT));
#ifdef SUPPORT_WIRED_LAN
	networkCtx->lanInst = SENS_MEM_ZALLOC(sizeof(WIRED_INSTANCE));
#endif	
	sensDev = SENS_MEM_ZALLOC(sizeof(SENS_DEV_STRUCT));
	sensPq = SENS_MEM_ZALLOC(sizeof(SENS_PQ_STRUCT));
	sensPq->onboardPq = SENS_MEM_ZALLOC(sizeof(float) * ON_BOARD_PQ_MAX);
	
	sysCfg = (SYS_CONFIG *)&sensSys->sysCfg;
	sysCtrl = (SYS_CTRL *)&sensSys->sysCtrl;
	sysTimer = (SYS_TIMER *)&sensSys->sysTimer;
	dbg = (DEBUG_STRUCT *)&sensSys->dbg;
	dbg->msg = SENS_MEM_ZALLOC(DEBUG_MSG_BUF);
	dbg->webDbgMsg = SENS_MEM_ZALLOC(1024);
	dbg->modbusLibMsg = SENS_MEM_ZALLOC(DEBUG_MSG_BUF);
}
#define MAX_PATTERN_SIZE	(512*1024)	//2M alloc
#define PATTERN_ALL_O		0
#define PATTERN_ALL_FF		1
#define PATTERN_0x55		2
#define PATTERN_0xAA		3
#define PATTERN_0x55AA		4
#define PATTERN_INC			5
#define PATTERN_DEC			6
#define PATTERN_RAND		7
#define PATTERN_0x33		8
#define PATTERN_0xCC		9
#define PATTERN_0x33CC		10
#define PATTERN_MAX			11
//------------------------------------------------------------------------------
static int createPatternBuffer(uint8_t *patternBuf, uint32_t bufSize)
{	int patternType;
	uint16_t *u16PatternBuf;
	int idx;
	srand(GetTimeTicks());
	
	patternType = rand() % PATTERN_MAX;
	
	switch(patternType)
	{	case PATTERN_ALL_O:
				memset(patternBuf, 0, bufSize);
				break;
		case PATTERN_ALL_FF:
				memset(patternBuf, 0xFF, bufSize);
				break;
		case PATTERN_0x55:
				memset(patternBuf, 0x55, bufSize);
				break;
		case PATTERN_0xAA:
				memset(patternBuf, 0xAA, bufSize);
				break;
		case PATTERN_0x55AA:
				u16PatternBuf = (uint16_t *)patternBuf;
				for(idx=0;idx<bufSize/2;idx++)
				{	u16PatternBuf[idx] = 0x55AA;
				}
				break;
		case PATTERN_INC:
				for(idx=0;idx<bufSize;idx++)
				{	patternBuf[idx] = idx & 0xFF;
				}
				break;
		case PATTERN_DEC:
				for(idx=0;idx<bufSize;idx++)
				{	patternBuf[idx] = (~idx) & 0xFF;
				}
				break;
		case PATTERN_RAND:
				for(idx=0;idx<bufSize;idx++)
				{	patternBuf[idx] = rand() & 0xFF;
				}
				break;
		case PATTERN_0x33:
				memset(patternBuf, 0x33, bufSize);
				break;
		case PATTERN_0xCC:
				memset(patternBuf, 0xCC, bufSize);
				break;
		case PATTERN_0x33CC:
				u16PatternBuf = (uint16_t *)patternBuf;
				for(idx=0;idx<bufSize/2;idx++)
				{	u16PatternBuf[idx] = 0x33CC;
				}
				break;
		default:
			break;
	}
	
	return patternType;
}
//------------------------------------------------------------------------------
static uint8_t * createPattern(uint32_t *bufSize, uint32_t maxSize, uint32_t minSize, int roundDown, int *patternType)
{	int lPatternType;
	uint8_t *patternBuf;
	uint32_t allocSize;
	
	srand(GetTimeTicks());
	allocSize = rand() % maxSize;
	if(allocSize == 0)
		allocSize = maxSize;
	if(allocSize < minSize)	
		allocSize = minSize;
	
	if(roundDown)
		allocSize = ROUNDDOWN(allocSize, minSize);
	
	patternBuf = SENS_MEM_ALLOC(allocSize);
	
	lPatternType = createPatternBuffer(patternBuf, allocSize);
	*patternType = lPatternType;
	
	*bufSize = allocSize;
	return patternBuf;
}
//------------------------------------------------------------------------------
void spiFlashReadWriteTest(void)
{	uint32_t mid;
	uint8_t buf[10], readBuf[10];
	uint32_t idx;
	
	openSpiFlash(&mid);
	memorySetProtection(0);
	SENS_TIME_DELAY(100);
	for(idx=0;idx<10;idx++)
	{	buf[idx] = idx;
	}
	for(idx=0;idx<16*1048576;idx+=40996)
		spiFlashSectorErase(idx);
	
	memoryWriteData(0, 10, buf);
	memoryReadData(0, 10, readBuf);
	
	for(idx=0;idx<10;idx++)
	{	if(readBuf[idx] != buf[idx])
		{	dbgMsg("spi error, Read is %X, wirte is %X\r\n", readBuf[idx], buf[idx]);
		}
	}
}
//------------------------------------------------------------------------------
void mainTask(taskArg param)
{	(void)param;
	// uint8_t ipV4[4];
	// uint8_t getDns = 0;
	uint8_t cameraFilename[64];
	uint8_t *jpgSoi, *jpgEoi;
	int idx = 0, ret = 0;
	uint8_t *writePtr;
	char *comparePtr;
	int writeIndex;
	int bufPtrInfoIndex = 0;
	uint32_t count=0;
	uint32_t bufSize;
	int patternType;
	char fileName[128];
	uint32_t adcChipId;
	int8_t adcIdErr = 0;
	uint32_t adcTimer;
	uint8_t channels;
	float aiVal[8];
	char floatValStr[30];
	uint32_t u32VECMAP;
	// uint32_t *midTemp = 0;
	
#if 0
	FMC_Open();
#ifdef HAVE_BOOTLOADER
	u32VECMAP = FMC_GetVECMAP();

    /* Set Vector Table Offset Register */
    if(u32VECMAP == 0x100000)
        SCB->VTOR = FMC_LDROM_BASE;
    else
        SCB->VTOR = u32VECMAP;	// change vector table
#endif
#endif

	initialViriable();
	initSysSem();
	rtcInit();
	RTC_EnableSpareAccess();
	

	sysStsFlag1.sysStatusFlags = getVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1);
	sysStsFlag2.sysStatusFlags = getVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG2);
	sysCtrlFlag.sysCtrlFlags = getVBatRegValue(RTC_SPARE_REG_ITEM_SYS_CTRL_FLAG);
	sysRecSlotFlag.recSlotFlag = getVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG);
	sysRecSlotFlag.rebootCnt++;
	setVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG, sysRecSlotFlag.recSlotFlag);
	setVBatRegValue(RTC_SPARE_REG_ITEM_NEXT_WAKEUP_TIMESTAMP, 0);
	//sysStsFlag1.execHramFlag = EXEC_HRAM_FLAG_NONE;
	setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1, sysStsFlag1.sysStatusFlags);
#if PSM_USE_WAKEUP_TIMER
	if(sysStsFlag1.psmMethod != PSM_METHOD_WAKEUP_TIMER)
	{	
		sysStsFlag1.psmMethod = PSM_METHOD_WAKEUP_TIMER;
		setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1, sysStsFlag1.sysStatusFlags);
	}
#else
	if(sysStsFlag1.psmMethod != PSM_METHOD_MCU_DPD)
	{	sysStsFlag1.psmMethod = PSM_METHOD_MCU_DPD;
		setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1, sysStsFlag1.sysStatusFlags);
	}
#endif
	/*sensSys->guid.ucid[0] = FMC_ReadCID();
	sensSys->guid.ucid[1] = FMC_ReadUID(0);
	sensSys->guid.ucid[2] = FMC_ReadUID(1);
	sensSys->guid.ucid[3] = FMC_ReadUID(2);
	getGuidFromOtp();*/
#if !USE_NUVOTON_EVB
	getEepromInfo();
#endif
	sysCtrl->sysWorkingStartTime = GetTimeTicks();
#if DI_USE_IRQ
	initialTaskStruct((TASK_INFO *)&taskAct.expanderTaskAct);
	SENS_SEM_LOCK(taskAct.expanderTaskAct.lock);
	taskAct.expanderTaskAct.id = OSA_TASK_CREATE(EXPANDER_TASK);
#else
	#if !USE_NUVOTON_EVB
	diInit();
	#endif
#endif
	
	/*dbgMsg("%s", "*************************\r\n");
	dbgMsg(" SensMini A4 Neo Ver: %08X Build Date: %s %s\r\n", FW_VERSION, __DATE__, __TIME__);
	dbgMsg("%s", "*************************\r\n");
	dbgMsg("GUID IS :%s\r\n", sensSys->guid.guidString);
	*/
#if !TEST_ETH_MASS_XMIT
	initialTaskStruct((TASK_INFO *)&taskAct.networkProcessTaskAct);
	SENS_SEM_LOCK(taskAct.networkProcessTaskAct.lock);
	taskAct.networkProcessTaskAct.id = OSA_TASK_CREATE(NETWORK_PROCESS_TASK);
#endif
#ifdef OS_FREERTOS
	SENS_TIME_DELAY(1);
#endif

#if !TEST_ETH_MASS_XMIT
	initialTaskStruct((TASK_INFO *)&taskAct.usbHostTaskAct);
	SENS_SEM_LOCK(taskAct.usbHostTaskAct.lock);
	taskAct.usbHostTaskAct.id = OSA_TASK_CREATE(USB_HOST_TASK);
	
	initialTaskStruct((TASK_INFO *)&taskAct.servCmdProcessTaskAct);
	SENS_SEM_LOCK(taskAct.servCmdProcessTaskAct.lock);
	taskAct.servCmdProcessTaskAct.id = OSA_TASK_CREATE(SERV_CMD_PROCESS_TASK);
	
	initialTaskStruct((TASK_INFO *)&taskAct.mqttTaskAct);
	SENS_SEM_LOCK(taskAct.mqttTaskAct.lock);
	taskAct.mqttTaskAct.id = OSA_TASK_CREATE(MQTT_TASK);
#endif
	
#ifdef OS_FREERTOS
	SENS_TIME_DELAY(1);
#endif
	//spiFlashReadWriteTest();

#if EN_WATCHDOG
	watchDogClr();
#endif
#if SPI_FILE_SYSTEM
	littleFsInit();
#endif
	sdInit();
	sysCtrl->logIsReady = 1;
	getSysParameter();
	
	dbgMsg("%s", "*************************\r\n");
	dbgMsg(" SensMini A4 Neo Ver: %08X Build Date: %s %s\r\n", FW_VERSION, __DATE__, __TIME__);
	dbgMsg("%s", "*************************\r\n");
	dbgMsg("GUID IS :%s\r\n", sensSys->guid.guidString);
	
#if EN_LOAD_BOOT_ZONE2
	loadBootZone2ToSpi();
#endif
	sysCtrl->pwrManager.currPsm = SYS_CFG_PSM_NORMAL;
	sensSys->sysTimer.testPsmTick = GetTimeTicks();
#if EN_WATCHDOG
	watchDogClr();
#endif

	initPq();
	
#if !USE_NUVOTON_EVB
	doPwrOutSetting();
#endif
//	ethernetInit();
	
#if !TEST_ETH_MASS_XMIT
	SENS_SEM_UNLOCK(taskAct.usbHostTaskAct.lock);
	SENS_TIME_DELAY(10);
	SENS_SEM_UNLOCK(taskAct.networkProcessTaskAct.lock);
	SENS_SEM_UNLOCK(taskAct.servCmdProcessTaskAct.lock);
	SENS_SEM_UNLOCK(taskAct.mqttTaskAct.lock);
	
	initialTaskStruct((TASK_INFO *)&taskAct.sensorTaskAct);
	taskAct.sensorTaskAct.id = OSA_TASK_CREATE(SENSORS_TASK);
#endif
	SENS_TIME_DELAY(2);
	
#if !USE_NUVOTON_EVB
#ifndef HAVE_BOOTLOADER
	while(!gRstWakeupTimerDone)
	{	SENS_TIME_DELAY(10);
	}
#endif
#endif
#if TEST_LOOP
	test_loop();
#else
	sysCtrlFlag.wdtActiveCnt = 0;
	setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_CTRL_FLAG, sysCtrlFlag.sysCtrlFlags);
	
	sysCtrl->everydayRebootSecondTime = 43230;	//for no psm 
#ifndef HAVE_BOOTLOADER	
	sysStsFlag2.sdRecDone = 0;
#endif
#if !USE_NUVOTON_EVB
	checkPrevAlarm();
#endif
	checkPrevOtaStatus();

	sysTimer->activeTimer = GetTimeTicks();
	while(1)
	{	SENS_TIME_DELAY(100);
#if EN_WATCHDOG
		watchDogClr();
#endif
		devTimeGet();		//first get time
#if !DI_USE_IRQ
		diPolling();					//check di status
#endif
		recordDiHistory();				//record di change status to di history file
		diWakeupProcess();				//di wakeup process, check di keep power on ?
		checkAndSetOnboardPqIsRdy();	//set on-board pq to map sw pq
		alarmProcess();					//check pq No.x alarm status
		pqDataToCloudPq();				//write sw-pq(pqData struct) to pqVal array.
		recordPqToTf();					//record pq to TF card
		prepareDataToCloud();			//create socket`connect socket and trigger to send data to cloud.
		pollWebImgCapture();			//polling web image capture control
		powerSaving();					//check power saving
		rebootCheck();					//check reboot condition and reboot
		InternalDiProcess();			//check maintenance port connect status and process RS-232 maintenance command protocol
		if(checkPrevOta())
		{	startTrigOta();
		}
	}
#endif
	
#ifdef TEST_ADC
	PIN_ISO_ADC_3V3_EN = 1;
	SENS_TIME_DELAY(20);
	dbgMsg("%s", "Start ADC Init\r\n");
#if TEST_MODE == 1
	openAdcSpi(&adcChipId, (uint8_t)SINGLE_END_MODE);
#else
	openAdcSpi(&adcChipId, (uint8_t)DIFFERENTIAL_MODE);
#endif
	
	//if(adcChipId != 0x14)
	//	adcIdErr = -1;
	SENS_TIME_DELAY(20);
	adcTimer = GetTimeTicks();
	while(1)
	{	SENS_TIME_DELAY(10);
		if(!adcIdErr)
		{	if((GetTimeTicks() - adcTimer) > 2000)
			{	
#if TEST_MODE == 1
				channels = 8;
#else
				channels = 4;
#endif
				for(idx=0;idx<channels;idx++)
				{	aiVal[idx] = ad7124ReadChannelValue(idx);
					
				}
				dbgMsg("%s", "ADC Value: [");
				for(idx=0;idx<channels;idx++)
				{	if(idx)
					{	dbgMsg1("%s", ",");
					}
#if TEST_ADC_TYPE == 1
					dbgMsg1("%s V", my_ftoa(floatValStr, aiVal[idx], 3));
#else
					dbgMsg1("%s mA", my_ftoa(floatValStr, aiVal[idx], 3));
#endif
				}
				dbgMsg1("%s", "]\r\n");
				adcTimer = GetTimeTicks();
			}
		}
	}
#endif
	
#ifdef TEST_CODE_SD
	sdInit();
	//dbgMsg("%s(%d), mem free size:%d\r\n", __func__, __LINE__, xPortGetFreeHeapSize());
	while(1)
	{	if(sensSys->sdInst.sdPresence)
		{	writePtr = createPattern(&bufSize, MAX_PATTERN_SIZE, 512, 0, &patternType);
			if(writePtr == NULL)
			{	dbgMsg("%s", "alloc Buffer FULL STOP!!\r\n");
				while(1)
				{	SENS_TIME_DELAY(1000);
				}
			}
			memset(fileName, 0, 128);
			SENS_SPRINTF(fileName, "F%08d.txt", count);
			if(isFileExist(fileName))
				sdFileDelete(fileName);
			
			sdWriteFile(fileName, (char *)writePtr, bufSize, 0, OVER_WRITE_MODE);
			comparePtr = SENS_MEM_ALLOC(bufSize);
			sdReadFile(fileName, (char *)comparePtr, bufSize, 0, NORMAL_READ_MODE);
			for(writeIndex=0;writeIndex<bufSize;writeIndex++)
			{	if(comparePtr[writeIndex] != writePtr[writeIndex])
				{	dbgMsg("Compare error, file %d, size:%d, at idx:0x%X, pattern:%d, write:0x%X, read:0x%X, STOP!!\r\n", count, bufSize, writeIndex, patternType, writePtr[writeIndex], comparePtr[writeIndex]);
					while(1)
					{	SENS_TIME_DELAY(1000);
					}
				}
			}
			sdFileDelete(fileName);
			SENS_MEM_FREE(comparePtr);
			SENS_MEM_FREE(writePtr);
			dbgMsg("Compare success, file %d, size:%d, pattern:%d, mem free size:%d\r\n", count, bufSize, patternType, xPortGetFreeHeapSize());	
			count++;
			if(count > 1000000)
			{	dbgMsg("%s", "test success over 1000000, STOP!!\r\n");
				while(1)
				{	SENS_TIME_DELAY(1000);
				}
			}
		}
	}
#endif

#if 0 //test power saving
	dbgMsg("Power Saving Test, current tick is %d\r\n", sensSys->sysTimer.testPsmTick);
	while(1)
	{	devTimeGet();
		checkPowerSaving();
		SENS_TIME_DELAY(1000);
	}
#endif

#if 0 //test I2C2 24AA024T EEPROM read/write Address 0x53
	dbgMsg("%s", "SPI Nor Flash test start\r\n");
	ret = openSpiFlash(midTemp);
	if(ret != 0)
		dbgMsg("%s", "openSpiFlash failed\r\n");
	
	dbgMsg("%s", "I2C2 EEPROM test start\r\n");
	i2cInit(I2C2_ID, 100000, 32);
	while(1)
	{	devTimeGet();

		SENS_TIME_DELAY(1000);
		// i2cEepromRWTest();
		i2cEepromRWTestPlan2();

		SENS_TIME_DELAY(1000);
		memoryRWTest();
	}	
#endif

#if 0	//test PCAL6416A GPIO expander
	uint16_t pcal6416GpioIntStatus = 0;
	uint16_t pcal6416GpioVal = 0;
	uint8_t gpioVal;
	dbgMsg("%s", "PCAL6416A GPIO Expander test start\r\n");
	while(1)
	{	devTimeGet();
		//set P1.0 & P0.0 to high
		pcal6416GpioSetVal(ISENS_SOLAR_EN, 1);
		pcal6416GpioSetVal(BUCK_12V_EN, 1);
		SENS_TIME_DELAY(50);
		//set P1.0 & P0.0 to low
		pcal6416GpioSetVal(ISENS_SOLAR_EN, 0);
		pcal6416GpioSetVal(BUCK_12V_EN, 0);
		SENS_TIME_DELAY(50);

		if(GPIO_GET_INT_FLAG(PF, BIT9))
		{	dbgMsg("%s", "PCAL6416A Interrupt from PF.9\r\n");

			// Read pcal6416 I/O expander interrupt status
			pcal6416GpioIntStatus = pcal6416GpioGetIntStatus();
			dbgMsg("PCAL6416A Interrupt status is 0x%04x\r\n", pcal6416GpioIntStatus);
			if(pcal6416GpioIntStatus & GPIOEX_DI2)
			{	// Read pcal6416 I/O expander input port value
				pcal6416GpioVal = pcal6416GpioGetVal(PCAL6416A01_SLA);
				// Target GPIOEX_DI2 pin to check
				if(pcal6416GpioVal & GPIOEX_DI2)
					dbgMsg("PCAL6416A GPIOEX_DI2 Input pin value high is 0x%04x\r\n", pcal6416GpioVal);
				else if((pcal6416GpioVal & GPIOEX_DI2) == 0)
					dbgMsg("PCAL6416A GPIOEX_DI2 Input pin value low is 0x%04x\r\n", pcal6416GpioVal);
			}
			else
			{	// Read pcal6416 I/O expander input port value
				pcal6416GpioVal = pcal6416GpioGetVal(PCAL6416A01_SLA);
				dbgMsg("PCAL6416A Input pin value is 0x%04x\r\n", pcal6416GpioVal);
			}

			GPIO_CLR_INT_FLAG(PF, BIT9);
		}
	}
#endif

#if 0 //test I2C2 temperature sensor
	float temperature = 0.0f;
	while(1)
	{	devTimeGet();
		temperature = tcn75aReadTemperature();
		dbgMsg("TCN75A Temperature is %.2f C\r\n", temperature);
		SENS_TIME_DELAY(1000);
 	}
#endif

#if 0
//#ifdef TEST_CODE_USB_UVC //test USB HS camera
	dbgMsg("%s", "USB HS Camera test start\r\n");
	
	//sdInit();
	OSA_TASK_CREATE(USB_HOST_TASK);
	
	while(1)
	{	SENS_TIME_DELAY(10);
		devTimeGet();

		if(gWriteJpg2File)
		{	memset(cameraFilename, 0, 64);
			SENS_SPRINTF((char *)cameraFilename, "DCIM/JPG_%d.jpg", wtiteJpgCnt);
			createImgFile((char *)cameraFilename);
			for(idx=0;idx< gSnapshotLen; idx++)
			{	if(snapshotBufPool[idx] == 0xFF && snapshotBufPool[idx+1] == 0xD8)
				{	jpgSoi = snapshotBufPool + idx;
					dbgMsg("find SOI, at 0x%X, total length:%d\r\n", idx, gSnapshotLen);
					break;
				}
			}
			if(wtiteJpgCnt == 0)
			{	displayBufInHex(snapshotBufPool, idx, __func__, __LINE__);
			}
			for(;idx< gSnapshotLen; idx++)
			{	if(snapshotBufPool[idx] == 0xFF && snapshotBufPool[idx+1] == 0xD9)
				{	jpgEoi = snapshotBufPool + idx + 2;
					dbgMsg("find EOI, at 0x%X\r\n", idx);
					break;
				}
			}
			int jpgLen = jpgEoi - jpgSoi;
			//int jpgLen = gSnapshotLen;
			sdWriteFile((char *)cameraFilename, (char *)snapshotBufPool, jpgLen, 0, OVER_WRITE_MODE);
			wtiteJpgCnt++;
			gWriteJpg2File = 0;
			 if(wtiteJpgCnt < 20)
			{	gDoSnapshot = 1;
			 	dbgMsg("write JPG-%d success, size:%d, re-get one jpg\r\n", (wtiteJpgCnt-1), jpgLen);
				dbgMsg("Receiver JPG-%d, re-get one jpg\r\n", (wtiteJpgCnt-1));
			}
			else
			{	dbgMsg("write JPG-%d success, size:%d ,stop\r\n", (wtiteJpgCnt-1), jpgLen);
			}
		}
	}
#endif
	
#if 0
	ethernetInit();	
    //netif_set_link_up(ethNetif);
		
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "time.google.com");
	sntp_init();

	while(1)
	{	
		ethNetif = (struct netif *)&wiredInst->ethNetif;
		if(!getDns && netif_is_up(ethNetif))
		{	dbgMsg("%s", "start to query IP of device.senslink.net\r\n");
			dnsQuery("device.senslink.net", ipV4);
			dbgMsg("device.senslink.net at %d.%d.%d.%d\r\n", ipV4[0], ipV4[1], ipV4[2], ipV4[3]);
			getDns = 1;
		}
		SENS_TIME_DELAY(1000);
		dbgMsg("%s", "main App Running\r\n");
	}
#endif
#if 0
	char *writePtr;
	char *comparePtr;
	int writeIndex;
	int bufPtrInfoIndex = 0;
	int allocSize;
	uint32_t count=0;
#endif
	
	
#if 0	//test hyper ram
	comparePtr = SENS_MEM_ZALLOC(4096);
	
	while(1)
	{	
		srand(GetTimeTicks());
		allocSize = rand() % 4096;
		if(allocSize == 0)
			allocSize = 4096;
		writePtr = SENS_MEM_ZALLOC(allocSize);
		if(((int)writePtr > 0x806F0000) || (bufPtrInfoIndex >2000 ))
		{	SENS_MEM_FREE(writePtr);
			for(writeIndex=0;writeIndex<bufPtrInfoIndex;writeIndex++)
			{	writePtr = (char *)bufPtrInfo[writeIndex];
				SENS_MEM_FREE(writePtr);
			}
			bufPtrInfoIndex = 0;
			continue;
			
		}
		if(writePtr)
		{	bufPtrInfo[bufPtrInfoIndex] = (int)writePtr;
			srand(GetTimeTicks());
			for(writeIndex=0;writeIndex<allocSize;writeIndex++)
			{	comparePtr[writeIndex] = rand() & 0xFF;
			}
			
			for(writeIndex=0;writeIndex<allocSize;writeIndex++)
			{	writePtr[writeIndex] = comparePtr[writeIndex];
			}
			
			for(writeIndex=0;writeIndex<allocSize;writeIndex++)
			{	if(writePtr[writeIndex] != comparePtr[writeIndex])
				{	while(1);
				}
			}
			bufPtrInfoIndex++;
			count++;
		}
		else
		{	while(1);
			
		}
		//SENS_MEM_FREE(writePtr);
	}
#endif
}

#ifdef OS_FREERTOS
int main(void)
{	
#ifdef NUVOTON
	prvSetupHardware();
#endif
#if SENSMINIA4_NEO
	/*uint32_t hRamStartAddr, hRamEndAddr, idx;
	hRamStartAddr = HYPERRAM_BASE;
	hRamEndAddr = hRamStartAddr + EXT_SRAM_SIZE;
	for(idx=hRamStartAddr;idx<hRamEndAddr;idx+=4)
	{	outp32(idx, 0);
	}*/
#endif
	vPortDefineHeapRegions(xHeapRegions);
	
	OSA_TASK_CREATE(MAIN_TASK);
	vTaskStartScheduler();
	while(1);
	
	return 0;
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{	/* vApplicationMallocFailedHook() will only be called if
		 configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
		 function that will get called if a call to pvPortMalloc() fails.
		 pvPortMalloc() is called internally by the kernel whenever a task, queue,
		 timer or semaphore is created.  It is also called by various parts of the
		 demo application.  If heap_1.c or heap_2.c are used, then the size of the
		 heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
		 FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
		 to query the size of free heap space that remains (although it does not
		 provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	//dbgMsg("%s", "mem malloc failed\r\n");
	//printf("mem malloc failed\n");
	//for(;;);
	SYS_ResetChip();
	while(1);
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void)
{
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
    to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
    task.  It is essential that code added to this hook function never attempts
    to block in any way (for example, call xQueueReceive() with a block time
    specified, or call vTaskDelay()).  If the application makes use of the
    vTaskDelete() API function (as this demo application does) then it is also
    important that vApplicationIdleHook() is permitted to return to its calling
    function, because it is the responsibility of the idle task to clean up
    memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{	(void) pcTaskName;
	(void) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();

	//__BKPT();

	//dbgMsg("Stack overflow task name=%s\r\n", pcTaskName);
	//printf("Stack overflow task name=%s\n", pcTaskName);

	SYS_ResetChip();
	while(1);
	//for(;;);
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void)
{
    /* This function will be called by each tick interrupt if
    configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
    added here, but the tick hook is called from an interrupt context, so
    code must not attempt to block, and only the interrupt safe FreeRTOS API
    functions can be used (those that end in FromISR()).  The code in this
    tick hook implementation is for demonstration only - it has no real
    purpose.  It just gives a semaphore every 50ms.  The semaphore unblocks a
    task that then toggles an LED.  Additionally, the call to
    vQueueSetAccessQueueSetFromISR() is part of the "standard demo tasks"
    functionality. */

    /* The semaphore and associated task are not created when the simple blinky
    demo is used. */

}
/*-----------------------------------------------------------*/

void vApplicationAssert(const char *filename, uint32_t line)
{	volatile const char *localFileName = filename;
	volatile uint32_t localLine = line;
	//dbgMsg("Assert Failed in FILE: %s, LINE: %d\r\n", localFileName, localLine);
	
	taskDISABLE_INTERRUPTS(); 
	SYS_ResetChip();
	while(1);
	//for( ;; );
}


#endif