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

#include "driver/s35710WakeupTimer.h"
#include "driver/PCAL6416GpioExpander.h"
#include "driver/tcn75a.h"
#include "driver/i2cEeprom.h"
#include "driver/spiNorFlash.h"
#include "driver/ad7124Adc.h"
#include "user_def.h"

#define MAX_PATTERN_SIZE	(512*1024)	//2M alloc
extern volatile int gWriteJpg2File;
//int wtiteJpgCnt = 0;
int bufPtrInfo[4096];
extern uint8_t *snapshotBufPool;
extern volatile int gSnapshotLen;
extern volatile int gDoSnapshot;
//extern void checkPowerSaving(void);
/*extern*/ uint32_t u32Error;

uint8_t ipV4[4];
uint8_t getDns = 0;
uint32_t adcChipId;
int8_t adcIdErr = 0;
uint32_t adcTimer;
uint8_t channels;
float aiVal[8];
char floatValStr[30];
uint32_t *midTemp = 0;
uint8_t cameraFilename[64];
uint8_t *jpgSoi, *jpgEoi;
int idx = 0;
int ret = 0;

uint8_t *writePtr;
char *comparePtr;
int writeIndex;
int bufPtrInfoIndex = 0;
uint32_t count=0;
uint32_t bufSize;
int patternType;
char fileName[128];
int test_count;

void test_loop();
void test_adc(int test_count);
void test_sd(int test_count);
void test_power_saving(int test_count);
void test_i2c_eeprom_rw(int test_count);
void test_gpio_expander(int test_count);
void test_i2c2_temperature_sensor(int test_count);
void test_code_usb_uvc(int test_count);
void test_ethernet(int test_count);
void test_hyper_ram(int test_count);

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
int split_string(char *input_str, char *output_arr[], int max_tokens) {
    int count = 0;
    
    // 定義分割符號，這裡使用空白字元（包含空格 ' ' 和 tab '\t'）
    const char *delim = " \t"; 
    
    // 取得第一個子字串
    char *token = strtok(input_str, delim);
    
    // 循環取得後續的子字串，直到沒有為止，或達到陣列上限
    while (token != NULL && count < max_tokens) {
        output_arr[count] = token;
        count++;
        token = strtok(NULL, delim); // 繼續切分剩餘字串
    }
    
    return count;
}

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
extern int8_t spiFlashWaitReady(void);
int SpiFlash_NormalPageProgram(SPI_INSTANCE *spiInst, uint32_t u32StartAddress, uint8_t *u8DataBuffer)
{
  uint32_t u32Cnt = 0;

  // /CS: active
  SPI_SET_SS_LOW(spiInst->spiCtx);

  // send Command: 0x06, Write enable
  SPI_WRITE_TX(spiInst->spiCtx, 0x06);

  // wait tx finish
  SENS_SPI_IS_BUSY(spiInst);

  // /CS: de-active
  SPI_SET_SS_HIGH(spiInst->spiCtx);
	
  // /CS: active
  SPI_SET_SS_LOW(spiInst->spiCtx);

  // send Command: 0x02, Page program
  SPI_WRITE_TX(spiInst->spiCtx, 0x02);

  // send 24-bit start address
  SPI_WRITE_TX(spiInst->spiCtx, (u32StartAddress >> 16) & 0xFF);
  SPI_WRITE_TX(spiInst->spiCtx, (u32StartAddress >> 8)  & 0xFF);
  SPI_WRITE_TX(spiInst->spiCtx, u32StartAddress       & 0xFF);

  // write data
  while(1)
  {
    if(!SPI_GET_TX_FIFO_FULL_FLAG(spiInst->spiCtx))
		{ SPI_WRITE_TX(spiInst->spiCtx, u8DataBuffer[u32Cnt++]);
      if(u32Cnt > 255) break;
    }
  }

  // wait tx finish
  SENS_SPI_IS_BUSY((SPI_INSTANCE *)&spiInst);

  // /CS: de-active
  SPI_SET_SS_HIGH(spiInst->spiCtx);

  SPI_ClearRxFIFO(spiInst->spiCtx);
	return 0;
}

int SpiFlash_NormalRead(SPI_INSTANCE *spiInst, uint32_t u32StartAddress, uint8_t *u8DataBuffer)
{
	uint32_t u32Cnt;

  // /CS: active
  SPI_SET_SS_LOW(spiInst->spiCtx);

	// send Command: 0x03, Read data
	SPI_WRITE_TX(spiInst->spiCtx, 0x03);
	// send 24-bit start address
	SPI_WRITE_TX(spiInst->spiCtx, (u32StartAddress >> 16) & 0xFF);
	SPI_WRITE_TX(spiInst->spiCtx, (u32StartAddress >> 8)  & 0xFF);
	SPI_WRITE_TX(spiInst->spiCtx, u32StartAddress & 0xFF);

	SENS_SPI_IS_BUSY(spiInst);
	// clear RX buffer
	SPI_ClearRxFIFO(spiInst->spiCtx);
	// read data
	for(u32Cnt = 0; u32Cnt < 256; u32Cnt++)
	{
		SPI_WRITE_TX(spiInst->spiCtx, 0x00);
		SENS_SPI_IS_BUSY(spiInst);
		u8DataBuffer[u32Cnt] = (uint8_t)SPI_READ_RX(spiInst->spiCtx);
	}

	// wait tx finish
	SENS_SPI_IS_BUSY(spiInst);

	// /CS: de-active
	SPI_SET_SS_HIGH(spiInst->spiCtx);
	return 0;
}

#define	SPI_NOR_FLASH_TOTAL_SIZE		(16*1024*1024)  /* 16MB */
// #define TEST_BLOCK		256   /* block numbers */
#define PAGE_LENGTH		256   /* page numbers */
#define TEST_LENGTH		256
#define ONE_SECTOR_NUM		(4096 / PAGE_LENGTH)  /* 4KB */

uint32_t u32Error = 0, u32TotalTestCount = 0;
// uint8_t s_au8SrcArray[PAGE_LENGTH];
// uint8_t s_au8DestArray[PAGE_LENGTH];

extern SPI_INSTANCE spiInst;
//SPI_INSTANCE spiInst_1 = {0};

float toggleNF = false;
volatile uint32_t u32FlashAddress = 0;
void memoryRWTest(void)
{	volatile uint16_t u32ByteCount, u32PageNumber, u16SectorNumber = 0, u16RandomValue;
	uint8_t s_au8SrcArray[PAGE_LENGTH];
	uint8_t s_au8DestArray[PAGE_LENGTH];

	memset(s_au8SrcArray, 0, PAGE_LENGTH);
	memset(s_au8DestArray, 0, PAGE_LENGTH);

	/* init source data buffer */
	if(toggleNF == false)
	{ toggleNF = true;
		for(u32ByteCount = 0; u32ByteCount < PAGE_LENGTH; u32ByteCount++)
		{
			s_au8SrcArray[u32ByteCount] = 0x55;
		}
	}
	else
	{ toggleNF = false;
		for(u32ByteCount = 0; u32ByteCount < PAGE_LENGTH; u32ByteCount++)
		{
			s_au8SrcArray[u32ByteCount] = 0xAA;
		}
	}
	
	u16RandomValue = systemTickGet() & 0xFFFF;
	srand(u16RandomValue);
	u16SectorNumber = u16RandomValue % (SPI_NOR_FLASH_TOTAL_SIZE / SPI_NOR_FLASH_SECTOR_SIZE);
	u32FlashAddress = u16SectorNumber * SPI_NOR_FLASH_SECTOR_SIZE;

	printf("%s", "========================= SPI NOR FLASH ==========================\n");
	dbgMsg("SPI Flash Read/Write Sector Data: 0x%X\r\n", s_au8SrcArray[0]);
	dbgMsg("SPI Flash Random Sector Number[0 - 4095]: %d\r\n", u16SectorNumber);
	dbgMsg("SPI Flash Random Address[0 - 16777215]: %d\r\n", u32FlashAddress);

	memorySectorErase(u32FlashAddress);

	for(u32PageNumber = 0; u32PageNumber < ONE_SECTOR_NUM; u32PageNumber++)
  {
    /* page program */
    SpiFlash_NormalPageProgram((SPI_INSTANCE *)&spiInst, u32FlashAddress, s_au8SrcArray);
		spiFlashWaitReady();
    u32FlashAddress += 0x100;
	}

	SENS_TIME_DELAY(500); // Flash write cycle time

	memset(s_au8DestArray, 0, PAGE_LENGTH);

	/* Read SPI flash */
  u32FlashAddress -= SPI_NOR_FLASH_SECTOR_SIZE;
	
	for(u32PageNumber = 0; u32PageNumber < ONE_SECTOR_NUM; u32PageNumber++)
	{
		/* page read */
		SpiFlash_NormalRead((SPI_INSTANCE *)&spiInst, u32FlashAddress, s_au8DestArray);

		for(u32ByteCount = 0; u32ByteCount < TEST_LENGTH; u32ByteCount++)
		{
			if(s_au8DestArray[u32ByteCount] != s_au8SrcArray[u32ByteCount])
			{	u32Error++;
				dbgMsg("Data mismatched at address 0x%X, expect: 0x%X, read: 0x%X\r\n", (u32FlashAddress + u32ByteCount), s_au8SrcArray[u32ByteCount], s_au8DestArray[u32ByteCount]);
				break;
			}
		}
		
		u32FlashAddress += 0x100;
	}
	
	if(u32Error > 0)
    dbgMsg("SPI Flash Test Byte Error Count = %d\r\n", u32Error);

  dbgMsg("SPI Flash Read/Write Test Count = %d\r\n", u32TotalTestCount++);
  
}
//------------------------------------------------------------------------------
#if 1
#define TEST_WAKEUP_TIMER
void checkPowerSaving(void)
{	
#ifdef TEST_WAKEUP_TIMER	//use S-35710
	if((GetTimeTicks() - sensSys->sysTimer.testPsmTick) < 30000)
		return;
	
	dbgMsg("%s", "Active Over 0.5 Minutes, enter to Power saving 0.5 Minutes\r\n");
	powerManagerChipCtrl(PWR_MNG_IC_ACTION_ID_WRITE_WAKEUP_TIME, 30);

	SENS_TIME_DELAY(3000);
	dbgMsg("%s", "Send SOM_SLEEP_PIN pulse go to sleep.\r\n");
#if 0 // for CPU board
	PIN_SOM_SLEEP = 1;
#else	// for carrier board
	PIN_EN_SYS_PWR = 1;
#endif
	// dbgMsg("%s", "Active Over 0.5 Minutes, enter to Power saving 2 Minutes\r\n");
	// powerManagerChipCtrl(PWR_MNG_IC_ACTION_ID_WRITE_WAKEUP_TIME, 120);
	// SENS_TIME_DELAY(100);
	// powerManagerChipCtrl(PWR_MNG_IC_ACTION_ID_RESET, 0);

	/*sensSys->sysTimer.testPsmTick = GetTimeTicks();
	while(1)
	{	SENS_TIME_DELAY(1000);
		powerManagerChipCtrl(PWR_MNG_IC_ACTION_ID_RESET, 30);
		
	}*/
	while(1);
#endif
	
#if 0
	if(sysCtrl->pwrManager.currPsm == SYS_CFG_PSM_NONE)
		return;
	
	recTime = sysCfg->sensSysCfg.recordSec;
	
	timeElapsed = ( sensSys->dateTime.u32Hour*3600 + sensSys->dateTime.u32Minute*60 + sensSys->dateTime.u32Second ) % (recTime);
	standbyTime = recTime - timeElapsed;
	if(standbyTime < 0)
	{	standbyTime = (int)sysCfg->sensSysCfg.recordSec + standbyTime;
	}
	
	if(standbyTime < 15)	//less than 15 sec, wait for next rec time
	{	return;
	}
	
	currTimeStamp = dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
	dbgMsg("currTimeStamp:%d, standbyTime:%d\r\n", currTimeStamp, standbyTime);
#endif
#if 0
	currTimeStamp += standbyTime;
		
	timestampToDateTime(currTimeStamp, &alarmDt);
	RTC_DisableInt(RTC_INTEN_TICKIEN_Msk);
	setRtcAlarm(&alarmDt);
	dbgMsg("enter Sleep, next weak up time is %04d-%02d-%02d %02d:%02d:%02d\r\n", alarmDt.u32Year, alarmDt.u32Month, alarmDt.u32Day, alarmDt.u32Hour, alarmDt.u32Minute, alarmDt.u32Second);
#endif
	//setPciePwr(POWER_ON, 1, V_OUT_FLOOR_SET_3_DOT_150V);
	//enterPowerSavingMode();		
}
#endif
//=============================================================================
#ifdef TEST_ADC
void test_adc(int test_count)
{
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
	
	while (1)
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
				{	
					aiVal[idx] = ad7124ReadChannelValue(idx);
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
				test_count --;
				if (test_count == 0)
					break;
			}
		}
	}
}
#endif

#ifdef TEST_CODE_SD
void test_sd(int test_count)
{
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
//			if(count > 10)//00000)
//			{	dbgMsg("%s", "test success over 1000000, STOP!!\r\n");
//				while(1)
//				{	SENS_TIME_DELAY(1000);
//				}
//			}
			test_count --;
			if (test_count == 0)
				break;
		}
	}
}
#endif

#if 1 //test power saving
void test_power_saving(int test_count)
{
	dbgMsg("Power Saving Test, current tick is %d\r\n", sensSys->sysTimer.testPsmTick);
	while(1)
	{	devTimeGet();
		checkPowerSaving();
		SENS_TIME_DELAY(1000);
//		test_count --;
//		if (test_count == 0)
//			break;
	}
}
#endif

#if 1 //test I2C2 24AA024T EEPROM read/write Address 0x53
void test_i2c_eeprom_rw(int test_count)
{
	dbgMsg("%s", "I2C2 EEPROM test start\r\n");
	i2cInit(I2C2_ID, 100000, 32);
	while(1)
	{	devTimeGet();

		SENS_TIME_DELAY(1000);
		i2cEepromRWTestPlan2();//external i2c eeprom r/w test

		test_count --;
		if (test_count == 0)
			break;
	}	
}
void test_spi_flash_rw(int test_count)
{
	dbgMsg("%s", "SPI Nor Flash test start\r\n");
	ret = openSpiFlash(midTemp);
	if(ret != 0)
		dbgMsg("%s", "openSpiFlash failed\r\n");
	
	while(1)
	{	devTimeGet();

		SENS_TIME_DELAY(1000);
		memoryRWTest();//external spi NOR flash r/w test
		if(u32Error != 0)//[DL]if any error happens, then break directly
			break;
		test_count --;
		if (test_count == 0)
			break;
	}	
}
#endif

#if 1	//test PCAL6416A GPIO expander
void test_gpio_expander(int test_count)
{
	uint16_t pcal6416GpioIntStatus = 0;
	uint16_t pcal6416GpioVal = 0;
	uint8_t gpioVal;
	dbgMsg("%s", "PCAL6416A GPIO Expander test start\r\n");
//	initPCAL6416GpioExpander(0xffff);
//GPIO_SetPullCtl(PF, BIT9, GPIO_PUSEL_PULL_UP);
//GPIO_SetMode(PF, BIT9, GPIO_MODE_INPUT);
	while(1)
	{	devTimeGet();
//dbgPrint("%s", ".*");
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
//else if(pcal6416GpioIntStatus & GPIOEX_P6)
//{	// Read pcal6416 I/O expander input port value
//	pcal6416GpioVal = pcal6416GpioGetVal(PCAL6416A01_SLA);
//	// Target GPIOEX_P6 pin to check
//	if(pcal6416GpioVal & GPIOEX_P6)
//		dbgMsg("PCAL6416A GPIOEX_P6 Input pin value high is 0x%04x\r\n", pcal6416GpioVal);
//	else if((pcal6416GpioVal & GPIOEX_P6) == 0)
//		dbgMsg("PCAL6416A GPIOEX_P6 Input pin value low is 0x%04x\r\n", pcal6416GpioVal);
//}
//else if(pcal6416GpioIntStatus & GPIOEX_P7)
//{	// Read pcal6416 I/O expander input port value
//	pcal6416GpioVal = pcal6416GpioGetVal(PCAL6416A01_SLA);
//	// Target GPIOEX_P7 pin to check
//	if(pcal6416GpioVal & GPIOEX_P7)
//		dbgMsg("PCAL6416A GPIOEX_P7 Input pin value high is 0x%04x\r\n", pcal6416GpioVal);
//	else if((pcal6416GpioVal & GPIOEX_P7) == 0)
//		dbgMsg("PCAL6416A GPIOEX_P7 Input pin value low is 0x%04x\r\n", pcal6416GpioVal);
//}
			else
			{	// Read pcal6416 I/O expander input port value
				pcal6416GpioVal = pcal6416GpioGetVal(PCAL6416A01_SLA);
				dbgMsg("PCAL6416A Input pin value is 0x%04x\r\n", pcal6416GpioVal);
			}

			GPIO_CLR_INT_FLAG(PF, BIT9);
		}
	}
}
#endif

#if 1 //test I2C2 temperature sensor
void test_i2c2_temperature_sensor(int test_count)
{
	float temperature = 0.0f;
	initOnboardTemperatureSensor();
//	i2cInit(I2C2_ID, 100000, 32);
	while(1)
	{	devTimeGet();
		temperature = tcn75aReadTemperature();
		dbgMsg("_TCN75A Temperature is %.2f C\r\n", temperature);
		SENS_TIME_DELAY(1000);
		test_count --;
		if (test_count == 0)
			break;
 	}
}
#endif

#ifdef TEST_CODE_USB_UVC //test USB HS camera
#include "systemTimer.h"
bool f_uvc_camera_initialized;
extern void usbh_vendor_init(void);
extern int setSensminiTimer(void *taskQHandler, unsigned int msgId, void *ctx, unsigned int timerId, unsigned int interval, unsigned char mode);
extern int sensorTaskInit(void);
void test_code_usb_uvc(int test_count)
{
	UVC_DEV_T       *vdev;
	IMAGE_FORMAT_E  format;
	int width, height;
	uint8_t *snapshotBuf;
	int i, ret;
	int postSnapshotTime = 0;
	static uint8_t s_last_dev_num = 0;
	int FJF = 0;
	
	int start = 0, end = 0;
  uint32_t testTick = 0;
	
	struct TaskQMsg msg;

	dbgMsg("%s", "USB HS Camera test start\r\n");
	
	if(!sensSys->sdInst.sdPresence)
		sdInit();

	if(taskAct.uvcCameraTaskAct.id == 0)
	{
		if(MQX_OK != SENS_MSG_Q_INIT(sensorQ, SENSOR_NUM_MESSAGES, SENS_TASKQ_GRANM)) 
		{	
			return;// -1;
		}
//		sensorTaskInit();	

		initUvcCamDevice();
		initialTaskStruct((TASK_INFO *)&taskAct.uvcCameraTaskAct);
		taskAct.uvcCameraTaskAct.id = OSA_TASK_CREATE(UVC_CAMERA_TASK);
		SENS_TIME_DELAY(100);
	}
	
	if(taskAct.usbHostTaskAct.id == 0)
	{
		initialTaskStruct((TASK_INFO *)&taskAct.usbHostTaskAct);
		taskAct.usbHostTaskAct.id = OSA_TASK_CREATE(USB_HOST_TASK);
		SENS_TIME_DELAY(100);
	}
	
	while (1)
	{
		msg.msgId = SENSOR_Q_MSG_CAMERA_PRE_START;
		msg.ptr = NULL;
		if(sendMsgWithErrHandle(UVC_CAMERA_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
		{
		}

		while(!uvcCamInst->ctx->uvcActive)//wait for uvc active flag start
		{	SENS_TIME_DELAY(100);
		}
		while(uvcCamInst->ctx->uvcActive)//wait for uvc active flag complete
		{	SENS_TIME_DELAY(100);
		}
		while (!isFileExist(uvcCamInst->saveFileName))//wait for target file exist
		{
			SENS_TIME_DELAY(10);
		}
		SENS_TIME_DELAY(1000);//delay for write complete
		
		test_count --;
		if (test_count == 0)
			break;
	}
}
//[DL]reset solution #1
//	while (!isFileExist(uvcCamInst->saveFileName))
//	{
//		SENS_TIME_DELAY(10);
//	}
//	SENS_TIME_DELAY(100);
//	NVIC_SystemReset();
	
//[DL]reset solution #2
//	SENS_TIME_DELAY(500);//(1000)
//	NVIC_SystemReset();
#endif
	
#if 1
void test_ethernet(int test_count)
{
	struct netif *ethNetif;//(DL add)
	NET_INSTANCE *lanInst = networkCtx->lanInst;//(DL add)
	WIRED_INSTANCE	*wiredInst = lanInst->wiredInst;//(DL add)
	
//	ethernetInit();	
    //netif_set_link_up(ethNetif);
		
	sntp_setoperatingmode(SNTP_OPMODE_POLL);//設定為「輪詢模式」，設備會主動向伺服器發送請求（例如每隔 1 小時一次），詢問：「現在幾點了？」
	sntp_setservername(0, "time.google.com");//指定時間伺服器地址=Google 提供的公共 NTP 伺服器地址
	sntp_init();//啟動 SNTP 服務

	while(1)
	{	
		ethNetif = (struct netif *)&wiredInst->ethNetif;
		if(netif_is_up(ethNetif))
//		if(!getDns && netif_is_up(ethNetif))
		{	dbgMsg("%s", "start to query IP of device.senslink.net\r\n");
			dnsQuery("device.senslink.net", ipV4);
			dbgMsg("device.senslink.net at %d.%d.%d.%d\r\n", ipV4[0], ipV4[1], ipV4[2], ipV4[3]);
//			getDns = 1;
			dbgMsg("%s", "Get IP address !!\r\n");
			SENS_TIME_DELAY(1000);
			
			test_count --;
			if (test_count == 0)
				break;
			else
				continue;
		}
//		SENS_TIME_DELAY(1000);
//		dbgMsg("%s", "main App Running\r\n");
	}
}
#endif
	
#if 1	//test hyper ram
void test_hyper_ram(int test_count)
{
	char *writPetr;
	comparePtr = SENS_MEM_ZALLOC(4096);
	dbgMsg("%s", "\r\n");
	
	while(1)
	{	
		srand(GetTimeTicks());
		uint16_t allocSize = rand() % 4096;
		if(allocSize == 0)
			allocSize = 4096;
		writePtr = SENS_MEM_ZALLOC(allocSize);
		if((int)writePtr > 0x806F0000)
//		if(((int)writePtr > 0x806F0000) || (bufPtrInfoIndex >2000 ))
		{	SENS_MEM_FREE(writePtr);
			for(writeIndex=0;writeIndex<bufPtrInfoIndex;writeIndex++)
			{	writPetr = (char *)bufPtrInfo[writeIndex];
				SENS_MEM_FREE(writPetr);//return back memory allocated before
			}
			bufPtrInfoIndex = 0;
			continue;
			
		}
		if(writePtr)
		{	bufPtrInfo[bufPtrInfoIndex] = (int)writePtr;
			dbgPrint("writePtr=%x\r\n",writePtr);
			srand(GetTimeTicks());
			for(writeIndex=0;writeIndex<allocSize;writeIndex++)
			{	comparePtr[writeIndex] = rand() & 0xFF;
			}
			
			for(writeIndex=0;writeIndex<allocSize;writeIndex++)
			{	writePtr[writeIndex] = comparePtr[writeIndex];
			}
			
			for(writeIndex=0;writeIndex<allocSize;writeIndex++)
			{	if(writePtr[writeIndex] != comparePtr[writeIndex])
				{	while(1)
					;
				}
			}
			bufPtrInfoIndex++;
			count++;
		}
		else
		{	while(1)
			;
		}
		//SENS_MEM_FREE(writePtr);
		test_count --;
		if (test_count == 0)
			break;
	}
}
#endif

#if TEST_LOOP
#define MAX_BUF_SIZE 64
#define MAX_LINE 8
uint16_t len;
char buf[MAX_BUF_SIZE];
uint8_t recvBuf[MAX_BUF_SIZE];
int test_count;

#define ITEM_ADC "adc"
#define ITEM_SD "sd"
#define ITEM_POWER_SAVING "power_saving"
#define ITEM_I2C_EEPROM "i2c_eeprom"
#define ITEM_SPI_FLASH "spi_flash"
#define ITEM_GPIO_EXPANDER "gpio_expander"
#define ITEM_TEMPERATURE_SENSOR "temperature_sensor"
#define ITEM_USB_UVC "usb_uvc"
#define ITEM_ETHERNET "ethernet"
#define ITEM_HYPER_RAM "hyperram"
#define ITEM_KNOWN "unknown_command"
#define TEST_RESULT_PASS " PASS!! #############################\r\n"
#define TEST_RESULT_NG " NG!! #############################\r\n"

#define MAX_TOKEN_COUNT 3
void test_loop()
{
	char dataToSend[MAX_BUF_SIZE]="Result:";
	int dataLen;
	char *result[MAX_TOKEN_COUNT];
	int token_count=0;
	
WDT_RESET_COUNTER();
	while(1)
	{	SENS_TIME_DELAY(500);
		devTimeGet();
		while(1)
		{
			int ch = getchar();//waiting for input from UART
			if(ch == '\n') //end of line 0x0D[\r][CR] or 0x0A[\n][LF] or 0x0D 0x0A[\r\n][CR LF]
			{	
				buf[len] = '\0';
				if(len > 0)
				{	
					strtokLock();
					token_count = split_string(buf, result, MAX_TOKEN_COUNT);
					strtokUnLock();
//					for (int i = 0; i < token_count; i++) 
//					{
//						printf("第 %d 段: %s\n", i + 1, result[i]);
//					}
					
//					dbgMsg("Received string: %s", buf);//(debug message sent to CONSOLE_UART_CTX
//					printf("Received string: %s", buf);//(message sent to CONSOLE_UART_CTX)
//					UART_Write(CONSOLE_UART_CTX, (uint8_t *)dataToSend, strlen((const char *)dataToSend));(message sent to CONSOLE_UART_CTX)
//					while(!UART_IS_TX_EMPTY(CONSOLE_UART_CTX)) __NOP();

					test_count =  _io_atoi(result[1]);
					if (test_count == 0)
					{
						dbgMsg("test count invalid =%d\r\n", test_count);						
					}
					else
					{
						if (strncmp((void*)buf, ITEM_ADC, strlen(ITEM_ADC))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_adc(test_count);
							strcpy(dataToSend+7, TEST_RESULT_PASS);
							dbgMsg("%s", dataToSend);
						}
						else if (strncmp((void*)buf, ITEM_SD,  strlen(ITEM_SD))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_sd(test_count);
							strcpy(dataToSend+7, TEST_RESULT_PASS);
							dbgMsg("%s", dataToSend);
						}
						else if (strncmp((void*)buf, ITEM_POWER_SAVING, strlen(ITEM_POWER_SAVING))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_power_saving(test_count);
							strcpy(dataToSend+7, TEST_RESULT_PASS);
							dbgMsg("%s", dataToSend);
						}
						else if (strncmp((void*)buf, ITEM_I2C_EEPROM, strlen(ITEM_I2C_EEPROM))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_i2c_eeprom_rw(test_count);
							strcpy(dataToSend+7, TEST_RESULT_PASS);
							dbgMsg("%s", dataToSend);
						}
						else if (strncmp((void*)buf, ITEM_SPI_FLASH, strlen(ITEM_SPI_FLASH))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_spi_flash_rw(test_count);
							if(u32Error == 0)
								strcpy(dataToSend+7, TEST_RESULT_PASS);
							else
								strcpy(dataToSend+7, TEST_RESULT_NG);
							dbgMsg("%s", dataToSend);
						}
						else if (strncmp((void*)buf, ITEM_GPIO_EXPANDER, strlen(ITEM_GPIO_EXPANDER))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_gpio_expander(test_count);
							strcpy(dataToSend+7, TEST_RESULT_PASS);
							dbgMsg("%s", dataToSend);
						}
						else if (strncmp((void*)buf, ITEM_TEMPERATURE_SENSOR, strlen(ITEM_TEMPERATURE_SENSOR))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_i2c2_temperature_sensor(test_count);
							strcpy(dataToSend+7, TEST_RESULT_PASS);
							dbgMsg("%s", dataToSend);
						}
						else if (strncmp((void*)buf, ITEM_USB_UVC, strlen(ITEM_USB_UVC))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_code_usb_uvc(test_count);
							strcpy(dataToSend+7, TEST_RESULT_PASS);
							dbgMsg("%s", dataToSend);
						}
						else if (strncmp((void*)buf, ITEM_ETHERNET, strlen(ITEM_ETHERNET))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_ethernet(test_count);
							strcpy(dataToSend+7, TEST_RESULT_PASS);
							dbgMsg("%s", dataToSend);
						}
						else if (strncmp((void*)buf, ITEM_HYPER_RAM, strlen(ITEM_HYPER_RAM))==0)
						{
							dbgMsg("test- %s\r\n", buf);
							test_hyper_ram(test_count);
							strcpy(dataToSend+7, TEST_RESULT_PASS);
							dbgMsg("%s", dataToSend);
						}
						else
						{
							dbgMsg("test- %s\r\n", "unknown command!!\r\n");
						}
					}
					len = 0;
					memset(buf, 0, sizeof(buf));
				}
			}
			else
			{	
				if (ch != '\0') //[DL]the first byte received will be 0x00
					buf[len++] = (char)ch;
				if(len >= MAX_BUF_SIZE)
				{	dbgMsg("%s", "Received string too long, reset buffer\r\n");
					len = 0;
				}
			}
		}
	}
}
#endif

