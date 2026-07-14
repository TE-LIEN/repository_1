#include "sensSystem.h"
#include "sensCommon.h"
#include "rtcDateTime.h"
#include "powerCtrl.h"
#include "driver/s35710WakeupTimer.h"
#include "network/netApp.h"
#include "sensDev.h"
#include "network/protocol/serverProtocol.h"
#if AUTO_DATA_SYNC
#include "AutoDataSync.h"
#endif
#include "sensDev.h"
#include "sdCardTask.h"
#include "driver/spiNorFlash.h"

#define USB_CLK_SRC_IS_PLL	1

volatile SENS_SYSTEM_STRUCT	*sensSys;
volatile SYS_CONFIG			*sysCfg;
volatile SYS_CTRL			*sysCtrl;
volatile SYS_TIMER			*sysTimer;

#ifdef NUVOTON
#ifndef HAVE_BOOTLOADER
volatile uint8_t gRstWakeupTimerDone;
//------------------------------------------------------------------------------
void startTimerForCompleteWkTmrRst(void)
{	SYS_UnlockReg();
	
	CLK_EnableModuleClock(TMR1_MODULE);
	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);
	TIMER_Open(TIMER1, TIMER_ONESHOT_MODE, 2);
    TIMER_EnableInt(TIMER1);
	NVIC_EnableIRQ(TMR1_IRQn);
	gRstWakeupTimerDone = 0;
	TIMER_Start(TIMER1);
}
#endif
//------------------------------------------------------------------------------
void prvSetupHardware( void )
{	uint32_t u32VECMAP;
	SYS_UnlockReg();
	
	CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk | CLK_PWRCTL_LXTEN_Msk);
	CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk | CLK_STATUS_LXTSTB_Msk);
	
	CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);
#if USB_CLK_SRC_IS_PLL
	CLK_SetCoreClock(192000000);	//for USB HOST usb PLL source
#else
	CLK_SetCoreClock(200000000);
	CLK_EnableXtalRC(CLK_PWRCTL_HIRC48MEN_Msk);
	CLK_WaitClockReady(CLK_PWRCTL_HIRC48MEN_Msk);
#endif
	
	CLK_DisableXtalRC(CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_LIRCEN_Msk);
	
	CLK->AHBCLK0 |= CLK_AHBCLK0_GPACKEN_Msk | CLK_AHBCLK0_GPBCKEN_Msk | CLK_AHBCLK0_GPCCKEN_Msk | CLK_AHBCLK0_GPDCKEN_Msk |
                    CLK_AHBCLK0_GPECKEN_Msk | CLK_AHBCLK0_GPFCKEN_Msk | CLK_AHBCLK0_GPGCKEN_Msk | CLK_AHBCLK0_GPHCKEN_Msk;
	CLK->AHBCLK1 |= CLK_AHBCLK1_GPICKEN_Msk | CLK_AHBCLK1_GPJCKEN_Msk;
	
	FMC_Open();
#ifdef HAVE_BOOTLOADER
	u32VECMAP = FMC_GetVECMAP();

    /* Set Vector Table Offset Register */
    if(u32VECMAP == 0x100000)
        SCB->VTOR = FMC_LDROM_BASE;
    else
        SCB->VTOR = u32VECMAP;	// change vector table
#endif

#if HBI_ENABLE == 1
	SYS_ResetModule(UART0_RST);
	CLK_EnableModuleClock(UART0_MODULE);
	CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
	
	SET_UART0_RXD_PD2();
	SET_UART0_TXD_PD3();
  
#if UART_DEBUG_PCIE
	SYS_ResetModule(UART1_RST);
	CLK_EnableModuleClock(UART1_MODULE);
	CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HXT, CLK_CLKDIV0_UART1(1));
	
	SET_UART1_RXD_PH9();
	SET_UART1_TXD_PH8();
#endif
	
	//HBI->INTEN |= HBI_INTEN_OPINTEN_Msk;
	//NVIC_EnableIRQ(HBI_IRQn);
	//CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
  //CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);
	CLK_EnableModuleClock(RTC_MODULE);
	RTC_SetClockSource(RTC_CLOCK_SOURCE_LXT);
	// DPD_PWR_DOWN_EN
#ifndef HAVE_BOOTLOADER
	#if WAKEUP_USE_VBAT_DOMAIN
	RTC_SetGPIOMode(PIN_WAKEUP1_nRST_NO, RTC_IO_MODE_OUTPUT, RTC_IO_DIGITAL_ENABLE, RTC_IO_PULL_UP_DOWN_DISABLE, 0);
	#else
	RTC_SET_IOCTL_BY_GPIO();	//use S-35710 set MCU_DPD_EN PIN to Output High, must let PIN_MCU_DPD_EN change to normal GPIO first

	// Power Save
	// Wakeup Rest Pin for CPU Board
	//PIN_WAKEUP_nRST = 0;
	//GPIO_SetMode(PF, BIT6, GPIO_MODE_OUTPUT);	//PIN_WAKEUP_nRST
	//PIN_WAKEUP_nRST = 1;
	// Wakeup1 Rest Pin for Carrier Board
	PIN_WAKEUP1_nRST = 0;
	GPIO_SetMode(PF, BIT7, GPIO_MODE_OUTPUT);	//PIN_WAKEUP1_nRST
	PIN_WAKEUP1_nRST = 0;
	#endif
	startTimerForCompleteWkTmrRst();
	PIN_SHUTDOWN_REQn = 1;
	GPIO_SetMode(PG, BIT5, GPIO_MODE_OUTPUT);
	PIN_SHUTDOWN_REQn = 1;
	//PIN_SOM_SLEEP	=	0;
	//GPIO_SetMode(PG, BIT7, GPIO_MODE_OUTPUT);
	//PIN_SOM_SLEEP	=	0;
	
	
#endif
	// RS232
	GPIO_SetMode(PB, BIT14, GPIO_MODE_OUTPUT);
	PIN_RS232_EN = 1;
	
	//GPIO_SetMode(PG, BIT18, GPIO_MODE_OUTPUT);
	//PIN_DO_5V_EN = 1;
	
	// Ethernet
	PIN_ETH_PWR_EN = 0;
	GPIO_SetMode(PG, BIT1, GPIO_MODE_OUTPUT);
	
	
	// USB HIGH SPEED
	PIN_USB_HS_VBUS_EN = 0;
	GPIO_SetMode(PB, BIT10, GPIO_MODE_OUTPUT);

	// EEPROM 24AA024 write-protected(WP) Pin
	GPIO_SetMode(PC, BIT10, GPIO_MODE_OUTPUT);

	// GPIO EXPANDER Interrupt
	GPIO_SetMode(PF, BIT9, GPIO_MODE_INPUT);
	//GPIO_SetMode(PF, BIT8, GPIO_MODE_INPUT);
	
	//ADC POWER ENABLE
	GPIO_SetMode(PB, BIT15, GPIO_MODE_OUTPUT);
	PIN_BUCK_3V3_EN = 1;
	//PIN_BUCK_3V3_EN = 0;
	
	GPIO_SetMode(PD, BIT13, GPIO_MODE_OUTPUT);
	PIN_ISO_ADC_3V3_EN = 0;
	
	/*
	 * IOT Config
	 */
	
	PIN_IOT1_VBATT_EN = 0;
	GPIO_SetMode(PE, BIT14, GPIO_MODE_OUTPUT);
	PIN_IOT1_VIO_SEL = 0;
	GPIO_SetMode(PD, BIT8, GPIO_MODE_OUTPUT);
	PIN_IOT2_VBATT_EN = 0;
	GPIO_SetMode(PD, BIT9, GPIO_MODE_OUTPUT);
	PIN_IOT2_VIO_SEL = 0;
	GPIO_SetMode(PE, BIT15, GPIO_MODE_OUTPUT);
	PIN_VBATT_IOT_EN = 0;
	GPIO_SetMode(PD, BIT14, GPIO_MODE_OUTPUT);
	
	//USB FS SWITCH SEL
	PIN_USB_FS_SEL = 0;	//sel = LOW, select IOT1
	GPIO_SetMode(PH, BIT1, GPIO_MODE_OUTPUT);
	PIN_USB_FS_NOE = 1;	//oe disable
	GPIO_SetMode(PH, BIT2, GPIO_MODE_OUTPUT);
	
	//RS485_1
	GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);	//power enable
	GPIO_SetMode(PD, BIT12, GPIO_MODE_OUTPUT);	//DE/RE
	PIN_RS485_1_PWR_EN = 0;
	PIN_RS485_1_DE = 0;
	//RS485_2
	GPIO_SetMode(PE, BIT7, GPIO_MODE_OUTPUT);	//power enable
	GPIO_SetMode(PE, BIT6, GPIO_MODE_OUTPUT);	//DE/RE
	PIN_RS485_2_PWR_EN = 0;
	PIN_RS485_2_DE = 0;
	
	//VOLT_SENSE
	GPIO_SetMode(PC, BIT9, GPIO_MODE_OUTPUT);
	PIN_VSENSE_EN = 0;
	
	// DO ISO_12V_EN
	GPIO_SetMode(PG, BIT0, GPIO_MODE_OUTPUT);
	PIN_ISO_12V_EN = 0;
	
#else	//USB Nuvonton EVB
	SYS_ResetModule(UART1_RST);
	CLK_EnableModuleClock(UART1_MODULE);
	CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART1(1));
	SET_UART1_RXD_PB2();
	SET_UART1_TXD_PB3();
	
	GPIO_SetMode(PJ, BIT13, GPIO_MODE_OUTPUT);
	PIN_USB_HS_VBUS_EN = 0;
	
	SET_HSUSB_VBUS_EN_PJ13();
	SET_HSUSB_VBUS_ST_PJ12();
#endif
	
	UART_Open(CONSOLE_UART_CTX, 115200);
#if UART_DEBUG_PCIE
	UART_Open(UART1, 115200);
#endif
	
	
#if !USE_NUVOTON_EVB
	//USB HOST
	CLK_EnableModuleClock(USBH_MODULE);
	SET_USB_D_N_PA13();
	SET_USB_D_P_PA14();
	//SET_USB_OTG_ID_PA15();	//don't care, USB is HOST MODE
	SET_USB_VBUS_PA12();
	//SET_USB_VBUS_EN_PB6();	//only for Mikro bus
	SET_USB_VBUS_ST_PB7();	//must set before usb init, ohci set "ST" to low active
	
	//SET_HSUSB_VBUS_EN_PB10();
	SET_HSUSB_VBUS_ST_PB11();
	
#if USB_CLK_SRC_IS_PLL
	CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_PLL_DIV2, CLK_CLKDIV0_USB(2));
#else
	CLK_SetModuleClock(USBH_MODULE, CLK_CLKSEL0_USBSEL_HIRC48M, CLK_CLKDIV0_USB(1));
#endif
	SYS->USBPHY = SYS_USBPHY_HSUSBEN_Msk | (0x1 << SYS_USBPHY_HSUSBROLE_Pos) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk | (0x1 << SYS_USBPHY_USBROLE_Pos);
	delay_us(20);
	SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
	//NVIC_EnableIRQ(RTC_IRQn);
#endif
	/*
	 *	SD
	 */
	CLK_EnableModuleClock(SDH0_MODULE);
	CLK_SetModuleClock(SDH0_MODULE, CLK_CLKSEL0_SDH0SEL_HCLK, CLK_CLKDIV0_SDH0(4));
	
#if USE_NUVOTON_EVB
	SET_SD0_nCD_PD13();
	SET_SD0_CLK_PE6();
	SET_SD0_CMD_PE7();
	SET_SD0_DAT0_PE2();
	SET_SD0_DAT1_PE3();
	SET_SD0_DAT2_PE4();
	SET_SD0_DAT3_PE5();
#else
	SET_SD0_nCD_PB12();
	SET_SD0_CLK_PB1();
	SET_SD0_CMD_PB0();
	SET_SD0_DAT0_PB2();
	SET_SD0_DAT1_PB3();
	SET_SD0_DAT2_PB4();
	SET_SD0_DAT3_PB5();
	GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
	PIN_SD_PWR_EN = 1;
#endif

#if GPIO_DEBUG
	PIN_MK_SPI_CLK_FOR_DEBUG = 0;
	GPIO_SetMode(PG, BIT3, GPIO_MODE_OUTPUT);
#endif
#if EN_WATCHDOG
	CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
  CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

	CLK_EnableModuleClock(WDT_MODULE);
	//CLK_SetModuleClock(WDT_MODULE, CLK_CLKSEL1_WDTSEL_LXT, 0);	//can not use LXT?? why
	CLK_SetModuleClock(WDT_MODULE, CLK_CLKSEL1_WDTSEL_LIRC, 0);	
	
	//NVIC_EnableIRQ(WDT_IRQn);
	WDT_Open(WDT_TIMEOUT_2POW18, WDT_RESET_DELAY_18CLK, TRUE, FALSE);	//(2^18)/32768 = 8 sec
	//WDT_Open(WDT_TIMEOUT_2POW20, WDT_RESET_DELAY_18CLK, TRUE, FALSE);	//(2^20) / 32768 = 32 sec
	//WDT_EnableInt();
#endif
	
	//SYS_LockReg();
}
#endif	//end of #ifdef NUVOTON
//------------------------------------------------------------------------------
void devTimeGet(void)
{	//SYS_TIMER *sysTimer;
	S_RTC_TIME_DATA_T currDt;
	int idx;
	
	//sysTimer = (SYS_TIMER *)&sensSys->sysTimer;
	sysTimer->currTick = systemTickGet();
	
	if(sysTimer->currentTimeStamp == 0)
	{	taskENTER_CRITICAL();
		sysTimer->currentTimeStamp = dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
		taskEXIT_CRITICAL();
	}
	memcpy((void *)&currDt, (void *)&sensSys->dateTime, sizeof(S_RTC_TIME_DATA_T));
	sysTimer->currTimeSec = ((long)currDt.u32Hour * 60 + (long)currDt.u32Minute) * 60 + (long)currDt.u32Second;
	sysTimer->currHistoryRecTimeSlot = sysTimer->currTimeSec / 60;	//1 min to rec
	
	if(sysCfg->sensSysCfg.autoSendInterval)
		sysTimer->currAutosendTimeSlot = sysTimer->currTimeSec / sysCfg->sensSysCfg.autoSendInterval;
}
//------------------------------------------------------------------------------
#if 0
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
//------------------------------------------------------------------------------
void setUploadImgFsmChg(IMAGE_UPLOAD_INSTANCE *imgUploadInst, enum IMG_UPLOAD_FSM next, uint32_t timeout)
{	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	
	imgUploadInst->waitTimer = GetTimeTicks();
	imgUploadInst->waitTimeout = timeout;
	imgUploadInst->imgUploadFsmNext = next;
	imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_STATE_CHANGE;
	
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
}
//------------------------------------------------------------------------------
static int waitForUploadImgFsmChg(IMAGE_UPLOAD_INSTANCE *imgUploadInst)
{	int nFsmChg = -1;
	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	
	if((GetTimeTicks() - imgUploadInst->waitTimer) > imgUploadInst->waitTimeout)
	{	imgUploadInst->imgUploadFsm = imgUploadInst->imgUploadFsmNext;
		nFsmChg = 0;
	}
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
	return nFsmChg;
}
//------------------------------------------------------------------------------
static void uploadImageIdle(void)
{	IMAGE_UPLOAD_INSTANCE *imgUploadInst;
	IMAGE_UPLOAD_REC_HEADER *imgUploadRecHeader;
	
	imgUploadInst = networkCtx->imgUploadInst;
	if(imgUploadInst->imgUploadRecHeader == NULL)
		return;
	imgUploadRecHeader = imgUploadInst->imgUploadRecHeader;
	
	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	if(imgUploadRecHeader->numOfUnSendImage)
	{	imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_PREPARE;
		setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_IMAGE_UPLOAD);
	}
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
}
//------------------------------------------------------------------------------
static void uploadImagePrepare(void)
{	struct TaskQMsg msg;
	IMAGE_UPLOAD_INFO	*imageUploadInfo;
	IMAGE_UPLOAD_INSTANCE *imgUploadInst;
#if defined NUVOTON
	S_RTC_TIME_DATA_T imageDate;
	uint32_t timestamp;
#else
	DATE_STRUCT date1;
	TIME_STRUCT rtctime;
#endif
	
	imgUploadInst = networkCtx->imgUploadInst;
	imageUploadInfo = SENS_MEM_ZALLOC(sizeof(IMAGE_UPLOAD_INFO));
	
	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	memcpy((char *)imageUploadInfo, imgUploadInst->imgUploadRecInfoBuf, sizeof(IMAGE_UPLOAD_INFO));
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
	
#if defined NUVOTON
	timestamp = imageUploadInfo->unixTime;
	timestampToDateTime(timestamp, &imageDate);
	dbg_msg("[MESSAGE] AutoSend JPEG, %04d/%02d/%02d %02d:%02d:%02d (%llu), %04d/%02d/%02d %02d:%02d:%02d\r\n",
				imageDate.u32Year,
				imageDate.u32Month,
				imageDate.u32Day,
				imageDate.u32Hour,
				imageDate.u32Minute,
				imageDate.u32Second,
				imageUploadInfo->unixTime,
				sensSys->dateTime.u32Year,
				sensSys->dateTime.u32Month,
				sensSys->dateTime.u32Day,
				sensSys->dateTime.u32Hour,
				sensSys->dateTime.u32Minute,
				sensSys->dateTime.u32Second);
#else
	rtctime.SECONDS = imageUploadInfo->unixTime;
	rtctime.MILLISECONDS = 0;
	_rtc_time_to_mqx_date(&rtctime, &date1);
	dbg_msg("[MESSAGE] AutoSend JPEG, %04d/%02d/%02d %02d:%02d:%02d (%llu), %04d/%02d/%02d %02d:%02d:%02d\r\n",
				date1.YEAR,
				date1.MONTH,
				date1.DAY,
				date1.HOUR,
				date1.MINUTE,
				date1.SECOND,
				imageUploadInfo->unixTime,
				sensSys->date_time[0],
				sensSys->date_time[1],
				sensSys->date_time[2],
				sensSys->date_time[3],
				sensSys->date_time[4],
				sensSys->date_time[5]);
#endif
	msg.msgId = NETWORK_Q_MSG_HTTP_POST;
	msg.ptr = (char *)imageUploadInfo;
	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_SEND;
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
	if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	SENS_MEM_FREE(imageUploadInfo);
		setUploadImgFsmChg(imgUploadInst, IMG_UPLOAD_FSM_IDLE, 2000);
		//SENS_TIME_DELAY(500);
	}
}
//------------------------------------------------------------------------------
static void uploadImageDone(void)
{	IMAGE_UPLOAD_INSTANCE	*imgUploadInst;
	
	imgUploadInst = networkCtx->imgUploadInst;
	clearImgRecInfo(imgUploadInst->uploadImgInfo);
	if(imgUploadInst->uploadImgInfo)
		SENS_MEM_FREE(imgUploadInst->uploadImgInfo);
	imgUploadInst->uploadImgInfo = NULL;
	//imgUploadInst->imgUploadFsm = IMG_UPLOAD_FSM_DELETE_FILE;
}
//------------------------------------------------------------------------------
void putTaskQueueForSendImage(void)
{	IMAGE_UPLOAD_INSTANCE *imgUploadInst;
	enum IMG_UPLOAD_FSM	uploadFsm;
	if(sysCfg->cameraCfg == NULL)
		return;
	
	if(networkCtx->imgUploadInst == NULL)
		return;
	imgUploadInst = networkCtx->imgUploadInst;
	
	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	uploadFsm = imgUploadInst->imgUploadFsm;
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
	
	if(uploadFsm == IMG_UPLOAD_FSM_STATE_CHANGE)
	{	if(waitForUploadImgFsmChg(imgUploadInst))	//no change FSM
		{	return;
		}
	}
	
	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	uploadFsm = imgUploadInst->imgUploadFsm;
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
	
	switch(uploadFsm)
	{	case IMG_UPLOAD_FSM_IDLE:		uploadImageIdle();		break;
		case IMG_UPLOAD_FSM_PREPARE:	uploadImagePrepare();	break;	
		case IMG_UPLOAD_FSM_SEND:		break;	//nothing to do, just wait network send done
		case IMG_UPLOAD_FSM_WAIT_RSP:	break;	//nothing to do, just wait network send done
		case IMG_UPLOAD_FSM_SEND_DONE:	uploadImageDone();	//clear record info and delete Image file
		//case IMG_UPLOAD_FSM_DELETE_FILE:	delFileAfterUploadImageDone();
				break;
		default:						break;
	}
}
//------------------------------------------------------------------------------
static int serverIsMqttBroker(int serverType)
{	if(	(serverType == MQTT_SENSTALK_BROKER)
			|| (serverType == MQTTS_SENSTALK_BROKER)
	#ifdef SUPPORT_MQTT_ANIOT
	 		|| (serverType == MQTTS_ANIOT_BROKER)
	#endif
	#ifdef SUPPORT_MQTTS_AZURE
	 		|| (serverType == MQTTS_AZURE_BROKER)
	#endif
	#ifdef SUPPORT_MQTTS_AVNET
	 		|| (serverType == MQTTS_AVNET_BROKER)
	#endif
	#ifdef SUPPORT_MQTT_THINGS_BOARD
	 		|| (serverType == MQTT_THINGSBOARD_BROKER)
	#endif
	#ifdef SUPPORT_MQTT_IKW
	 		|| (serverType == MQTT_I_KNOW_WATER_BROKER)
	#endif
	#ifdef SUPPORT_MQTTS_IKW
	 		|| (serverType == MQTTS_I_KNOW_WATER_BROKER)
	#endif
	#ifdef SUPPORT_MQTT_SENSSEWER
	 		|| (serverType == MQTT_SENSSEWER_BROKER)
	#endif
	#ifdef SUPPORT_MQTTS_SENSSEWER
	 		|| (serverType == MQTTS_SENSSEWER_BROKER)
	#endif
	#if YS_MQTT_BROKER
			|| (serverType == MQTTS_YS_BROKER)
			|| (serverType == MQTT_YS_BROKER)
	#endif
		)
	{	return 1;
	}
	else
		return 0;
}
//------------------------------------------------------------------------------
void mqttInfToSock(CLOUD_SERVER_INFO *servInfo, CLOUD_SERVER_INSTANCE *serverInst, int idx)
{	//MQTT_CONNECT_INFO *mqttConnInfo = (MQTT_CONNECT_INFO *)servInfo->servConnectInfo;
	
	serverInst->mqttConnectInfo = (MQTT_CONNECT_INFO *)servInfo->servConnectInfo;
	/*if(serverInst->mqttDevName)
	{	SENS_MEM_FREE(serverInst->mqttDevName);
		serverInst->mqttDevName = NULL;
	}
	if(serverInst->mqttUserName)
	{	SENS_MEM_FREE(serverInst->mqttUserName);
		serverInst->mqttUserName = NULL;
	}
	if(serverInst->mqttPassword)
	{	SENS_MEM_FREE(serverInst->mqttPassword);
		serverInst->mqttPassword = NULL;
	}
	if(serverInst->mqttPubTopic)
	{	SENS_MEM_FREE(serverInst->mqttPubTopic);
		serverInst->mqttPubTopic = NULL;
	}
	
	if(mqttConnInfo == NULL)
		return;
	if(mqttConnInfo->mqttDevName)
	{	serverInst->mqttDevName = SENS_MEM_ZALLOC(strlen(mqttConnInfo->mqttDevName)+1);
		strcpy(serverInst->mqttDevName, mqttConnInfo->mqttDevName);
	}
	if(mqttConnInfo->mqttUserName)
	{	serverInst->mqttUserName = SENS_MEM_ZALLOC(strlen(mqttConnInfo->mqttUserName)+1);
		strcpy(serverInst->mqttUserName, mqttConnInfo->mqttUserName);
	}
	if(mqttConnInfo->mqttPassword)
	{	serverInst->mqttPassword = SENS_MEM_ZALLOC(strlen(mqttConnInfo->mqttPassword)+1);
		strcpy(serverInst->mqttPassword, mqttConnInfo->mqttPassword);
	}
	if(mqttConnInfo->mqttPubTopic)
	{	serverInst->mqttPubTopic = SENS_MEM_ZALLOC(strlen(mqttConnInfo->mqttPubTopic)+1);
		strcpy(serverInst->mqttPubTopic, mqttConnInfo->mqttPubTopic);
	}
	if(memcmp(sensSys->mqttPubTopic[idx], NULL_MQTT_INFO_STR, strlen(NULL_MQTT_INFO_STR)))
	{	serverInst->mqttPubTopic = SENS_MEM_ZALLOC(strlen(sensSys->mqttPubTopic[idx])+1);
		memcpy(serverInst->mqttPubTopic, sensSys->mqttPubTopic[idx], strlen(sensSys->mqttPubTopic[idx]));
	}*/
}
//------------------------------------------------------------------------------
static void sockCreateProcess(CLOUD_SERVER_INFO *servInfo, CLOUD_SERVER_INSTANCE	*serverInst, int servIdx, char *serverIp, int serverPort, int serverType)
{	uint8_t ipV4[4];
#ifdef NET_LWIP
	char ipV4Str[16];
	struct
#endif
		sockaddr_in *socketAddr;
	MQTTCtx *mqttCtx;
	struct TaskQMsg msg;
	NET_INSTANCE *workingInst;
	char *connectIp;
	uint8_t freeConnectIp=0;
	
	serverInst->isTempUsing = 0;
	serverInst->servIdx = servIdx;
	serverInst->isUsing = 1;
	if(serverInst->serverIp != NULL)
		SENS_MEM_FREE(serverInst->serverIp);
	serverInst->serverIp = SENS_MEM_ZALLOC(strlen(serverIp)+1);
	memcpy(serverInst->serverIp, serverIp, strlen(serverIp));
	serverInst->serverPort = serverPort;
	socketAddr = &serverInst->socketAddr;
	memset(socketAddr, 0x00, sizeof(
#ifdef NET_LWIP
		struct
#endif
		sockaddr_in));
	strcpy(serverInst->serverName, serverIp);
	socketAddr->sin_addr.s_addr = inet_addr(serverIp);
	//check is Domain name or IP
	if(socketAddr->sin_addr.s_addr == 
#if defined NET_RTCS
			INADDR_BROADCAST
#elif defined NET_LWIP
			IPADDR_NONE
#endif
		)
	{	//use DNS try again
		dnsQuery(serverIp, ipV4);
#if defined NET_RTCS
		socketAddr->sin_addr.s_addr = IPADDR(ipV4[0], ipV4[1], ipV4[2], ipV4[3]);		
#elif defined NET_LWIP
		SENS_SPRINTF(ipV4Str, "%d.%d.%d.%d", ipV4[0], ipV4[1], ipV4[2], ipV4[3]);
		socketAddr->sin_addr.s_addr = inet_addr(ipV4Str);
#endif
		dbg_msg("%s", RED"must use dns rosolv domainname to ip\r\n"ORG_COLOR);
	}

	if(serverIsMqttBroker(serverType))
	{	serverInst->xmitProtocolType = PROTOCOL_MQTT;
		if((serverType == MQTTS_SENSTALK_BROKER) || 
			 (serverType == MQTTS_AZURE_BROKER) || 
			 (serverType == MQTTS_AVNET_BROKER)|| 
			 (serverType == MQTTS_I_KNOW_WATER_BROKER) || 
			 (serverType == MQTTS_SENSSEWER_BROKER) 
#if YS_MQTT_BROKER
         || (serverType == MQTTS_YS_BROKER)
#endif
           )
		{	serverInst->isTlsProtocol = 1;
		}
		else
			serverInst->isTlsProtocol = 0;
		mqttCtx = (MQTTCtx *)serverInst->protCtx;
		if(mqttCtx && mqttCtx->stat != WMQ_WAIT_MOBILE_INITIAL_DONE)
			return;
	}
	else
		serverInst->xmitProtocolType = PROTOCOL_SENSTALK_V2;

  dbg_msg(GREEN"%s find serv inst at %p, sock begin, idx:%d, ip:%s, port:%d\r\n"ORG_COLOR, __func__, serverInst, servIdx, serverInst->serverIp, serverInst->serverPort);
#if defined NET_RTCS
	socketAddr->sin_port = serverPort;
#elif defined NET_LWIP
	socketAddr->sin_port = htons(serverPort);
#endif
	socketAddr->sin_family = AF_INET;
	serverInst->serverType = serverType;
	serverInst->socketState = SOCK_CREATE_WAIT;
	if(serverInst->xmitProtocolType != PROTOCOL_MQTT)
	{	msg.msgId = NETWORK_Q_MSG_TCP_SOCK_CREATE;
		msg.ptr = (char *)serverInst;
		if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
		{
		}
	}
	else
	{	mqttInfToSock(servInfo, serverInst, servIdx);
		int rc = mqttClientInitial(serverInst);
		if(!rc)
		{	msg.msgId = MQTT_Q_MSG_CREATE_SOCK_CONNECT;
			msg.ptr = (char *)serverInst;
			if(sendMsgWithErrHandle(MQTT_TASK, SENS_MSG_Q_SEND(mqttQ, (uint32_t *)&msg, 0), __func__, __LINE__))
			{
			}
		}
	}
}
//------------------------------------------------------------------------------
static void sockCreateDoneProcess(CLOUD_SERVER_INSTANCE *serverInst)
{	struct TaskQMsg msg;
	
	dbg_msg(GREEN"%s find serv inst at %p, sock create done, ip:%s, port:%d\r\n"ORG_COLOR, __func__, serverInst, serverInst->serverIp, serverInst->serverPort);
#ifdef SUPPORT_MQTT
	if(serverInst->xmitProtocolType != PROTOCOL_MQTT)
#endif
	{	serverInst->socketState = SOCK_CONN_WAIT;
		msg.msgId = NETWORK_Q_MSG_TCP_SOCK_CONNECT;
		msg.ptr = (char *)serverInst;
		if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
		{
		}
	}
}
//------------------------------------------------------------------------------
uint8_t isTimeSendToCloud(long current_time_slot, long orgTimeSlot)
{	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	if((sensSysCfg->autoSendInterval > 0) && (current_time_slot != orgTimeSlot))  // The time slot changed to the next time slot.
	{	return 1;
	}
	return 0;
}
//------------------------------------------------------------------------------
static int checkRsiIsValid(CLOUD_SERVER_INSTANCE	*serverInst, int *send, int skipCheck)
{	int dataSyncIsValid = 1;
	
#if AUTO_DATA_SYNC
	AUTO_DATA_SYNC_STRUCT *autoDataSyncCtx = (AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx;
	if(serverInst->serverType != SENSTALK_V2)
	{	*send = 0;
		return -1;
	}
	
	if(!skipCheck)
	{	SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
		if(autoDataSyncCtx == NULL)
		{	SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
			checkRecordFromHistoryFile(serverInst, __func__, __LINE__);
		}
		else
			SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
	}
	
	SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
	autoDataSyncCtx = (AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx;
	if(autoDataSyncCtx)
	{	if(!autoDataSyncCtx->isReady)
		{	dbg_msg("[CLOUD COMM] AutoSend DATA SYNC (Client_%d LAN), sock:%d\r\n", serverInst->servIdx + 1, serverInst->fd);
			*send = 3;
			autoDataSyncCtx->isReady = 1;
			autoDataSyncCtx->isReadyTimer = GetTimeTicks();
			SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
			dataSyncIsValid = 0;
			return dataSyncIsValid;
		}
		else if((GetTimeTicks() - autoDataSyncCtx->isReadyTimer) > 30000)
		{	autoDataSyncCtx->isReady = 0;
		}	
	}
	else
	{	
		dataSyncIsValid = -1;
	}
	SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
#else
	dataSyncIsValid = -1;
#endif
	return dataSyncIsValid;
}
//------------------------------------------------------------------------------
static void sockConnectDoneProcess(CLOUD_SERVER_INSTANCE	*serverInst)
{	struct TaskQMsg msg;
	int timeToSend = 0;
	SERV_RECV_CMD_CTX *cmdMsg;
	int send = 0;
	
#if SUPPORT_CY_PUMP
	CY_PUMP_WORK_INSTANCE	*cyPumpInst = &sensSys->cyPumpInst;
#endif
  long currentAutosendTimeSlot = sysTimer->currAutosendTimeSlot;
  
	if(serverInst->xmitProtocolType == PROTOCOL_MQTT)
	{	
#if YS_MQTT_BROKER
    if((serverInst->serverType == MQTTS_YS_BROKER) || (serverInst->serverType == MQTT_YS_BROKER))	//no upload data, only config
			return;
#endif
		//serverInst->netSendFsm = NET_SEND_FSM_SEND_PQ;
		/*if(serverInst->socketState != SOCK_MQTT_CONNECT_DONE)
			return;*/
		/*if(serverInst->netSendFsm == NET_SEND_FSM_IDLE)
			serverInst->netSendFsm = NET_SEND_FSM_SEND_FIRST_PQ;*/
		goto READY_TO_ETH_SEND;
	}
	if(serverInst->netSendFsm == NET_SEND_FSM_IDLE)
	{	dbg_msg("[CLOUD COMM] First GUID Send (Server_%d LAN), sock:%d\r\n", serverInst->servIdx+1, serverInst->fd);
		send = 1;
		serverInst->netSendFsm = NET_SEND_FSM_SEND_GUID;
	}
#ifdef SUPPORT_MQTT
READY_TO_ETH_SEND:
#endif
	timeToSend = isTimeSendToCloud(currentAutosendTimeSlot, serverInst->autosendTimeSlot);
#ifdef SUPPORT_MQTT_SENSSEWER
	if((serverInst->serverType >= MQTT_I_KNOW_WATER_BROKER) && (serverInst->serverType <= MQTTS_SENSSEWER_BROKER))
	{	if(serverInst->netSendFsm == NET_SEND_FSM_IDLE)
		{	serverInst->netSendFsm = NET_SEND_FSM_SEND_PQ;
			SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
			if(serverInst->autoDataSyncCtx == NULL)
			{	SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
				checkRecordFromHistoryFile(serverInst, __func__, __LINE__);
			}
			else
				SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
		}
		SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
		if(serverInst->autoDataSyncCtx && serverInst->netSendFsm == NET_SEND_FSM_SEND_PQ)
		{	if(!serverInst->autoDataSyncCtx->isReady)
			{	dbg_msg("[CLOUD COMM] AutoSend DATA SYNC (Server_%d LAN), sock:%d\r\n", serverInst->servIdx + 1, serverInst->fd);
				send = 3;
				serverInst->autoDataSyncCtx->isReady = 1;
				serverInst->autoDataSyncCtx->isReadyTimer = GetTimeTicks();
				SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
				goto SEND_DATA;
			}
			else if((GetTimeTicks() - serverInst->autoDataSyncCtx->isReadyTimer) > 30000)
			{	serverInst->autoDataSyncCtx->isReady = 0;
			}	
		}
		SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
		return;
	}
#endif
#if SUPPORT_CY_PUMP
	if(serverInst->netSendFsm == NET_SEND_FSM_SEND_PQ && cyPumpInst->isCIEngineMode && (cyPumpInst->isFinalSendFlagTemp & (1 << serverInst->servIdx)))
	{	dbg_msg("[CLOUD COMM] CY PUMP last send real time PQ (Server_%d LAN), sock:%d\r\n", serverInst->servIdx+1, serverInst->fd);
		send = 2;
		cyPumpInst->isFinalSendFlagTemp &= ~(1 << serverInst->servIdx);
		serverInst->netSendFsm = NET_SEND_FSM_SEND_PQ_TRIG;
	}
#endif
	if(serverInst->netSendFsm == NET_SEND_FSM_SEND_PQ)
	{	if(checkRsiIsValid(serverInst, &send, 1) == -1)
		{	if(timeToSend)
			{	serverInst->autosendTimeSlot = currentAutosendTimeSlot;
				dbg_msg("[CLOUD COMM] AutoSend real time PQ (Server_%d LAN), sock:%d\r\n", serverInst->servIdx+1, serverInst->fd);
				send = 2;
				serverInst->netSendFsm = NET_SEND_FSM_SEND_PQ_TRIG;
			}
		}
	}
	if(serverInst->netSendFsm == NET_SEND_FSM_SEND_FIRST_PQ)
	{	dbg_msg("[CLOUD COMM] First autoSend real time PQ (Server_%d LAN), sock:%d\r\n", serverInst->servIdx+1, serverInst->fd);
		send = 2;
		serverInst->netSendFsm = NET_SEND_FSM_SEND_PQ_TRIG;
	}
	if(serverInst->netSendFsm == NET_SEND_FSM_SEND_GUID_DONE)
	{	/*dbg_msg("[MESSAGE] First AutoSend AI (Client_%d LAN), sock:%d\r\n", serverInst->servIdx+1, serverInst->fd);
		send = 2;
		serverInst->netSendFsm = NET_SEND_FSM_SEND_PQ_TRIG;*/
		
		serverInst->netSendFsm = NET_SEND_FSM_CHK_HIST;
	}
	if(serverInst->netSendFsm == NET_SEND_FSM_CHK_HIST)
	{	//check data sync
		int dataSyncIsValid;
		dataSyncIsValid = checkRsiIsValid(serverInst, &send, 0);
		if(dataSyncIsValid == -1)
			serverInst->netSendFsm = NET_SEND_FSM_SEND_FIRST_PQ;
	}
#ifdef SUPPORT_MQTT_SENSSEWER
SEND_DATA:
#endif
	if(send)
	{	
#ifdef SUPPORT_MQTT
		if(serverInst->xmitProtocolType == PROTOCOL_MQTT)
		{	struct SenstalkCmd *senstalkCmdMsg;
			senstalkCmdMsg= SENS_MEM_ZALLOC(sizeof(struct SenstalkCmd));
			if(send == 3)
				senstalkCmdMsg->cmdType = IKW_DATA_SYNC;
			else
				senstalkCmdMsg->cmdType = STK_AUTOSEND;
			senstalkCmdMsg->current_tick = sysTimer->currTick;
			senstalkCmdMsg->cmdProtocol = serverInst->protCtx;
					
			msg.msgId = SERV_CMD_PROCESS_Q_MSG_SEND_CMD;
			msg.ptr = (char *)senstalkCmdMsg;
		}
		else
#endif
		{	cmdMsg = SENS_MEM_ZALLOC(sizeof(SERV_RECV_CMD_CTX));
			cmdMsg->serverInst = serverInst;
			cmdMsg->generateFromInternet = 0x80;
			msg.msgId = SERV_CMD_PROCESS_Q_MSG_RECV_CMD;
			msg.ptr = (char *)cmdMsg;
		}
		if(sendMsgWithErrHandle(SERV_CMD_PROCESS_TASK, SENS_MSG_Q_SEND(servCmdProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
		{ if(send == 1)
				serverInst->netSendFsm = NET_SEND_FSM_IDLE;
			else
				serverInst->netSendFsm = NET_SEND_FSM_SEND_PQ;
			SENS_MEM_FREE(cmdMsg);
		}
	}
}
//------------------------------------------------------------------------------
static void normalCloudUpload(CLOUD_SERVER_INFO *servInfo, CLOUD_SERVER_INSTANCE *serverInst, int servIdx, char *serverIp, int serverPort, int serverType)
{	servInfo->servInst = (void *)serverInst;
		
	switch((int)serverInst->socketState)
	{	
#ifdef PING_IP_BEFORE_CONNECT
		case SOCK_PING_IP:
				if(!pingServer(serverIp))
				{	serverInst->socketState = SOCK_BEGIN;
				}
				break;
#endif
		case SOCK_BEGIN:				sockCreateProcess(servInfo, serverInst, servIdx, serverIp, serverPort, serverType);	break;
		case SOCK_CREATE_DONE:			sockCreateDoneProcess(serverInst);																				break;
		case SOCK_MQTT_CONNECT_DONE:	
		case SOCK_CONN_DONE:			sockConnectDoneProcess(serverInst);	break;
		defalut:						break;
	}
}
#if SUPPORT_IOA_WEB_API
//------------------------------------------------------------------------------
void ioaInfToSock(CLOUD_SERVER_INFO *servInfo, CLOUD_SERVER_INSTANCE *serverInst, int idx)
{	IOA_SERVER_CTX	*ioaServCtx;
	IOA_CONNECT_INFO *ioaConnInf;
	
	if(servInfo->servConnectInfo == NULL)
		return;
	serverInst->ioaConnectInfo = (IOA_CONNECT_INFO *)servInfo->servConnectInfo;
	ioaConnInf = (IOA_CONNECT_INFO *)serverInst->ioaConnectInfo;
	
	ioaServCtx = SENS_MEM_ZALLOC(sizeof(IOA_SERVER_CTX));
	
	ioaServCtx->equipId = SENS_MEM_ZALLOC(strlen(ioaConnInf->equipId) + 1);
	strcpy(ioaServCtx->equipId, ioaConnInf->equipId);
	ioaServCtx->apiKey = SENS_MEM_ZALLOC(strlen(ioaConnInf->apiKey) + 1);
	strcpy(ioaServCtx->apiKey, ioaConnInf->apiKey);
	serverInst->protCtx = ioaServCtx;
}
//------------------------------------------------------------------------------
static void ioaIdleProcess(CLOUD_SERVER_INSTANCE	*serverInst, IOA_SERVER_CTX *ioaServCtx)
{	
#ifdef AUTO_DATA_SYNC
	SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
	if(serverInst->autoDataSyncCtx == NULL)
	{	SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
		checkRecordFromHistoryFile(serverInst, __func__, __LINE__);
	}
	else
		SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
	ioaServCtx->fsm = IOA_WEB_API_FSM_GET_TOKEN;
#endif
}
//------------------------------------------------------------------------------
static void ioaGetTokenProcess(CLOUD_SERVER_INSTANCE *serverInst, IOA_SERVER_CTX *ioaServCtx)
{	uint8_t readyToSend = 0;
	struct TaskQMsg msg;
	AUTO_DATA_SYNC_STRUCT	*autoDataSyncCtx;// = (AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx;
	
#ifdef AUTO_DATA_SYNC
	SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
	if(serverInst->autoDataSyncCtx)
	{	autoDataSyncCtx = (AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx;
		if(!autoDataSyncCtx->isReady)
		{	autoDataSyncCtx->isReady = 1;
			readyToSend = 1;
		}
	}
	SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
	
	if(!readyToSend)
	{	if(serverInst->servIdx == 0)
			setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_SRV1_DATA_SEND);
		else 
			setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_SRV2_DATA_SEND);
		return;
	}
#endif
	if(ioaServCtx->token)
	{	ioaServCtx->fsm = IOA_WEB_API_FSM_TOKEN_DONE;
		return;
	}
	
	dbg_msg("%s", "[IOA] Ready to get Token!!\r\n");
	msg.msgId = NETWORK_Q_MSG_IOA_GET_TOKEN;	//LAN_Q_MSG_IOA_GET_TOKEN;
	msg.ptr = (char *)serverInst;
	ioaServCtx->fsm = IOA_WEB_API_FSM_WAIT_TOKEN;
	ioaServCtx->getTokenTimer = GetTimeTicks();
	if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	ioaServCtx->fsm = IOA_WEB_API_FSM_GET_TOKEN;
#ifdef AUTO_DATA_SYNC
		SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
		if(serverInst->autoDataSyncCtx)
		{	autoDataSyncCtx = (AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx;
			if(autoDataSyncCtx->isReady)
			{	autoDataSyncCtx->isReady = 0;
			}
		}
		SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
#endif
	}
}
//------------------------------------------------------------------------------
static void ioaWaitTokenPrecess(CLOUD_SERVER_INSTANCE	*serverInst, IOA_SERVER_CTX *ioaServCtx)
{	if((GetTimeTicks() - ioaServCtx->getTokenTimer) > 60000)
	{	//close socket and retry.
		/*if(serverInst->fd != -1)
		{	lanSocketClose(serverInst, 1, __func__, __LINE__);
		}*/
		//reboot
		sysCtrl->isReadyToReboot = 1;
	}
}
//------------------------------------------------------------------------------
static void ioaGetTokenDone(CLOUD_SERVER_INSTANCE	*serverInst, IOA_SERVER_CTX *ioaServCtx)
{	struct TaskQMsg msg;
	AUTO_DATA_SYNC_STRUCT *autoDataSyncCtx;
	
	dbg_msg("[IOA] AutoSend PQ (Server_%d LAN), sock:%d\r\n", serverInst->servIdx+1, serverInst->fd);
	msg.msgId = NETWORK_Q_MSG_IOA_SEND_DATA;
	msg.ptr = (char *)serverInst;
	ioaServCtx->fsm = IOA_WEB_API_FSM_DATA_SENDING;
	ioaServCtx->getTokenTimer = GetTimeTicks();
	if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	ioaServCtx->fsm = IOA_WEB_API_FSM_GET_TOKEN;
#ifdef AUTO_DATA_SYNC
		SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
		if(serverInst->autoDataSyncCtx)
		{	autoDataSyncCtx = (AUTO_DATA_SYNC_STRUCT *)serverInst->autoDataSyncCtx;
			if(autoDataSyncCtx->isReady)
			{	autoDataSyncCtx->isReady = 0;
			}
		}
		SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
#endif
	}	
	else
	{	//ioaServCtx->fsm = IOA_WEB_API_FSM_GET_TOKEN;
	}
}
//------------------------------------------------------------------------------
static void ioaCloudUploadProcess(CLOUD_SERVER_INFO *servInfo, CLOUD_SERVER_INSTANCE *serverInst, int servIdx, char *serverIp, int serverPort, int serverType)
{	//uint16_t *serverType;
	IOA_SERVER_CTX *ioaServCtx;
	
	
	if(serverInst->protCtx == NULL)
	{	dbg_msg("%s", "[IOA] create new IOA Context!!\r\n");
		serverInst->isTempUsing = 0;
		serverInst->isUsing = 1;
		serverInst->servIdx = servIdx;
		serverInst->xmitProtocolType = PROTOCOL_IOA_WEB_API;
		serverInst->protCtx = SENS_MEM_ZALLOC(sizeof(IOA_SERVER_CTX));
		serverInst->serverType = serverType;
		ioaInfToSock(servInfo, serverInst, servIdx);
				
		if(serverInst->serverIp != NULL)
			SENS_MEM_FREE(serverInst->serverIp);
		serverInst->serverIp = SENS_MEM_ZALLOC(strlen(serverIp)+1);
		memcpy(serverInst->serverIp, serverIp, strlen(serverIp));
		serverInst->serverPort = serverPort;
		memset(serverInst->serverName, 0, 256);
		strcpy(serverInst->serverName, serverIp);
		serverInst->isTlsProtocol = 1;
	}
	
	ioaServCtx = (IOA_SERVER_CTX *)serverInst->protCtx;
	sysCtrl->servIdleXmitTime[servIdx] = GetTimeTicks();
	switch(ioaServCtx->fsm)
	{	case IOA_WEB_API_FSM_IDLE:			ioaIdleProcess(serverInst, ioaServCtx);			break;
		case IOA_WEB_API_FSM_GET_TOKEN:		ioaGetTokenProcess(serverInst, ioaServCtx);		break;
		case IOA_WEB_API_FSM_TOKEN_DONE:	ioaGetTokenDone(serverInst, ioaServCtx);		break;
		case IOA_WEB_API_FSM_WAIT_TOKEN:		
		case IOA_WEB_API_FSM_DATA_SENDING:	ioaWaitTokenPrecess(serverInst, ioaServCtx);	break;
		case IOA_WEB_API_FSM_MAX:			break;
	}
}
#endif
//------------------------------------------------------------------------------
void putTaskQueueForSendToCloud(void)
{	CLOUD_SERVER_INSTANCE	*serverInst;
	char *serverIp;
	int serverPort;
	uint16_t serverType;
	int idx = 0;
#ifdef NET_LWIP
	ip_addr_t	ipAddr;
#endif
	NET_INSTANCE *workingInst;

	CLOUD_SERVER_INFO *servInfo;
	int servIdx = -1;
	
	for(servInfo = sysCfg->servInfos;servInfo;servInfo=servInfo->next, idx++)
	{	serverIp = &servInfo->serverDomainName[0];
		serverPort = servInfo->serverPort;
		serverType = servInfo->serverProtocol;
		
		servIdx++;
		if(!strcmp(serverIp, "0"))
			continue;
		if(servInfo->waitForConnectionTimer)
		{	if((GetTimeTicks() - servInfo->waitForConnectionTimer) > 5000)
			{	dbg_msg("svr:%d, connect fail over 5 sec, retry again!!\r\n", idx);
				servInfo->waitForConnectionTimer = 0;
			}
			else
				continue;
		}
		
#if YS_MQTT_BROKER
		if((serverType == MQTTS_YS_BROKER) || (serverType == MQTT_YS_BROKER))
		{	workingInst = networkCtx->workingInst;
			if(workingInst->netMdvpnType == LTE_MDVPN_TYPE_OTHER_MDVPN)
				continue;
			if(workingInst->netMdvpnType == LTE_MDVPN_TYPE_SENSLINK_MDVPN)
			{	serverIp = YS_MQTT_MDVPN_IP;
			}
		}
#endif
		serverInst = findServerInstByIp(serverIp, serverPort);
		if(serverInst == NULL)
		{	sysCtrl->isReadyToReboot = 1;
			dbg_msg("%s", "No free server instance!!\r\n");
			return;
		}
#if SUPPORT_IOA_WEB_API
		if((serverType >= IOA_WEB_API_PROTOCOL) && (serverType < IOA_PLATFORM_MAX))
			ioaCloudUploadProcess(servInfo, serverInst, servIdx, serverIp, serverPort, serverType);
		else
#endif	//end of SUPPORT_IOA_WEB_API
		{	normalCloudUpload(servInfo, serverInst, servIdx, serverIp, serverPort, serverType);
		}
		
	}
}
//------------------------------------------------------------------------------
void prepareDataToCloud(void)
{	NET_INSTANCE *workingInst;
	
	if(networkCtx == NULL)
		return;
	if(!networkCtx->networkIsUp)
		return;
		
	if(networkCtx->workingInst == NULL)
		return;
	workingInst = networkCtx->workingInst;
	if(workingInst->enable == 0)
		return;
	
	//if(networkCtx->otaRunning)
	//	return;

#if SUPPORT_AGPS
	workingInst = networkCtx->workingInst;
	if(workingInst->currentIsGpsOnlyMode)
		return;
#endif
	putTaskQueueForSendImage();
	putTaskQueueForSendToCloud();
}
//------------------------------------------------------------------------------
int isTimeToRecPq(void)
{	uint32_t recTimeSlot;
#ifdef REC_INTERVAL_FIXED
	uint8_t recCondition = PQ_REC_CONDITION_NONE;
#endif
	DI_INSTANCE	*diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT	*diCtx = (DI_CONTEXT *)&diInst->diCtx;
	
	if(sensSys->dateTime.u32Year < VALID_YEAR)
		return 0;
	
#ifdef REC_INTERVAL_FIXED
	if(sysCtrl->sysIsInstallMode)	// install mode 1 min rec
		recCondition = PQ_REC_CONDITION_IM;
	else if((sysTimer->currHistoryRecTimeSlot % sysTimer->recTimeSlotInterval) == 0)	//normal history record time slot, every 10min or others
		recCondition = PQ_REC_CONDITION_REC_TIME;
	else if(sysCfg->sensSysCfg.psm != SYS_CFG_PSM_NONE)
	{	/*
		 *	alert mode always 1 min rec history.
		 */
		if(sysCfg->sensSysCfg.alertEnable && sysStsFlag2.alarmActive && ((sysTimer->currHistoryRecTimeSlot % sysTimer->alertTimeSlotInverval) == 0))	
			recCondition = PQ_REC_CONDITION_ALARM_TIME;
		else if(diCtx->isDiKeepOn && ((sysTimer->currHistoryRecTimeSlot % sysTimer->diWakeupRecSlot) == 0))
			recCondition = PQ_REC_CONDITION_DI_TIME;
	}
	
	/*if(!rec && (sysCtrl->pwrManager.currPsm != SYS_CFG_PSM_NONE) && (sysCfg->sensSysCfg.alertEnable))
	{	if(sysStsFlag2.alarmActive && ((sysTimer->currHistoryRecTimeSlot % sysTimer->alertTimeSlotInverval) == 0))
			rec = 1;
	}*/
#endif
	sysRecSlotFlag.recSlotFlag = getVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG);
	recTimeSlot = sysRecSlotFlag.currHistoryRecSlot;
	
#if defined REC_INTERVAL_FIXED
	/*if(abs((int)sysTimer->currHistoryRecTimeSlot-(int)recTimeSlot) > sysTimer->recTimeSlotInterval)
	{	sysTimer->currHistoryRecTimeSlot = ROUNDDOWN(sysTimer->currHistoryRecTimeSlot, sysTimer->recTimeSlotInterval);
		rec = 1;
	}*/
	
	if((sysTimer->currHistoryRecTimeSlot != recTimeSlot) && (recCondition != PQ_REC_CONDITION_NONE))
#else
	if(sysTimer->currHistoryRecTimeSlot != recTimeSlot)
#endif
	{	sysRecSlotFlag.recSlotFlag = getVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG);
		dbg_msg("[MESSAGE] Record Time Slot: %d, old is %d\r\n", sysTimer->currHistoryRecTimeSlot, sysRecSlotFlag.currHistoryRecSlot);
		sysRecSlotFlag.currHistoryRecSlot = sysTimer->currHistoryRecTimeSlot;
		setVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG, sysRecSlotFlag.recSlotFlag);
		return 1;
	}
	return 0;
}
//------------------------------------------------------------------------------
int isTimeToSendAlertImg(void)
{	long newTimeSlot;

	newTimeSlot = sysTimer->currTimeSec / sysCfg->sensSysCfg.cameraAlertInterval;
	if(newTimeSlot != sysTimer->alertCurrSendImgSlot)
	{	sysTimer->alertCurrSendImgSlot = newTimeSlot;
		return 1;
	}
	return 0;
}
//------------------------------------------------------------------------------
#if 1//EN_WATCHDOG//[DL]20260611
void watchDogClr(void)
{	WDT_RESET_COUNTER();
	if((GetTimeTicks() - sysTimer->activeTimer) > 10000)
	{	sysTimer->activeTimer = GetTimeTicks();
		dbgMsg("%s", "[SYS] system Active!!\r\n");
	}
}
#endif
//------------------------------------------------------------------------------
void clearDiReleaseFlag(int releaseFlag)
{	DI_INSTANCE *diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT *diCtx = (DI_CONTEXT *)&diInst->diCtx;
	
	if(releaseFlag == DI_RELEASE_FLAG_REC)
	{	if(diCtx->diRelaseFlag & DI_RELEASE_FLAG_REC)
			diCtx->diRelaseFlag &= ~DI_RELEASE_FLAG_REC;
	}
	else
	{	if(diCtx->diRelaseFlag & DI_RELEASE_FLAG_REC)	//must rec first, then send 
			return;
		else
		{	if(diCtx->diRelaseFlag & DI_RELEASE_FLAG_SEND)
				diCtx->diRelaseFlag &= ~DI_RELEASE_FLAG_SEND;
		}
	}
	if((diCtx->diRelaseFlag & 0x07) == DI_RELEASE_FLAG_ALL)
		diCtx->diRelaseFlag = 0;
}
//------------------------------------------------------------------------------
void cloudDataSendDoneProcess(CLOUD_SERVER_INSTANCE *serverInst)
{	SYSTEM_ALARM *sysAlarm = (SYSTEM_ALARM *)&sensSys->sysAlarm;
	DI_INSTANCE *diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT *diCtx = (DI_CONTEXT *)&diInst->diCtx;
	
	
	if(sysAlarm->waitAlarmSendFinalData)
	{	networkCtx->atleastSend1DataBmp &= ~(1 << serverInst->servIdx);
		if(serverInst->servIdx == 0)
			setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_ALERT_SEND_SRV1_DATA);
		else if(serverInst->servIdx == 1)
			setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_ALERT_SEND_SRV2_DATA);
	}
	clearDiReleaseFlag(DI_RELEASE_FLAG_SEND);
}
//------------------------------------------------------------------------------
#if EN_LOAD_BOOT_ZONE2
void loadBootZone2ToSpi(void)
{	uint8_t *buf;
	int fileLen;
	int idx;
	int writeLen;
	int offset;
	int readLen;
	
	if(isFileExist(BOOT_ZONE2_FILE_NAME))
	{	buf = SENS_MEM_ZALLOC(SPI_NOR_FLASH_SECTOR_SIZE);
		
		fileLen = getFileLength(BOOT_ZONE2_FILE_NAME);
		offset = 0;
		for(idx=0;idx<fileLen;idx+=SPI_NOR_FLASH_SECTOR_SIZE)
		{	writeLen = SPI_NOR_FLASH_SECTOR_SIZE;
			if((idx + writeLen) > fileLen)
				writeLen = fileLen - idx;
		
			readLen = sdReadFile(BOOT_ZONE2_FILE_NAME, (char *)buf, writeLen, offset, NORMAL_READ_MODE);
			if(readLen == writeLen)
			{
			}
			else
			{	dbgMsg("read fail, read len %u, write len %u\r\n", readLen, writeLen);
				return ;
			}
			memorySectorErase(offset);
			memoryWriteData(offset, writeLen, buf);
			dbgMsg("write Boot Zone 2 Addr:%u, size:%u\r\n", offset, writeLen);
			offset += writeLen;
#if EN_WATCHDOG
			watchDogClr();
#endif
		}
		SENS_MEM_FREE(buf);
		
		sdFileDelete(BOOT_ZONE2_FILE_NAME);
		dbgMsg("%s", "write Boot Zone 2 Done\r\n");
	}
	
	
	
}
#endif
//------------------------------------------------------------------------------