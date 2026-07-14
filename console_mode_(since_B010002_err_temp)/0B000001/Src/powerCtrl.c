#include "powerCtrl.h"
#include "driver/PCAL6416GpioExpander.h"
#include "sensCommon.h"
#include "sensDev.h"
#include "driver/s35710WakeupTimer.h"
#include "sensSystem.h"
#include "sensors/modbusApp.h"
#include "physicalQuantity.h"
//GPIO CTRL

//------------------------------------------------------------------------------
int SENS_GPIO_WRITE(uint32_t gpioNo, uint8_t val)
{	uint32_t port, pin;
	int ret = -1;
	
	if(gpioNo < EXT0_P0_0)
	{	port = gpioNo / 16;
		pin = gpioNo % 16;
		GPIO_PIN_DATA(port, pin);
		ret = 0;
	}
	else if((gpioNo > EXT0_P1_7) && (gpioNo < GPIO_MAX))
	{	pin = 1 << (gpioNo - EXT1_P0_0);
		pcal6416GpioSetVal(pin, val);
		ret = 0;
	}
	else if(gpioNo < EXT1_P0_0)
	{	dbgMsg("%s", "can not set GPIO, it is Input Mode!!\r\n");
	}
	else 
	{	dbgMsg("%s", "GPIO Not Defined\r\n");
	}
	return ret;
}
//------------------------------------------------------------------------------
uint8_t SENS_GPIO_READ(uint32_t gpioNo)
{	uint8_t val = 0;
	uint16_t wVal;
	uint32_t port, pin;
	if(gpioNo < EXT0_P0_0)
	{	port = gpioNo / 16;
		pin = gpioNo % 16;
		val = GPIO_PIN_DATA(port, pin);
	}
	else if(gpioNo < EXT1_P0_0)
	{	pin = 1 << (gpioNo - EXT0_P0_0);
		wVal = pcal6416GpioGetVal(PCAL6416A01_SLA);
		val = (wVal & pin)? 1:0;
	}
	else if(gpioNo < GPIO_MAX)
	{	pin = 1 << (gpioNo - EXT1_P0_0);
		wVal = pcal6416GpioGetVal(PCAL6416A01_SLA);
		val = (wVal & pin)? 1:0;
	}
	
	return val;
}
//------------------------------------------------------------------------------
GPIO_T *getNuGpioPortInst(int portNo)
{	GPIO_T *gpioInst = NULL;
	
	if(portNo == 0)	gpioInst = PA;
	else if(portNo == 1)	gpioInst = PB;
	else if(portNo == 2)	gpioInst = PC;
	else if(portNo == 3)	gpioInst = PD;
	else if(portNo == 4)	gpioInst = PE;
	else if(portNo == 5)	gpioInst = PF;
	else if(portNo == 6)	gpioInst = PG;
	else if(portNo == 7)	gpioInst = PH;
	else if(portNo == 8)	gpioInst = PI;
	else if(portNo == 9)	gpioInst = PJ;
	
	return gpioInst;
}
//------------------------------------------------------------------------------
void SENS_GPIO_INIT(uint32_t gpioNo, enum GPIO_TYPE type, uint8_t val, enum GPIO_PULLSEL _pullsel)
{	uint32_t port, pin;
	GPIO_T *gpioInst;
	uint32_t mode;
	uint32_t pullSel;
	
	if(gpioNo < EXT0_P0_0)
	{	port = gpioNo / 16;
		pin = gpioNo % 16;
		switch((uint32_t)type)
		{	case GPIO_TYPE_NO_CHANGE:
			case GPIO_TYPE_INPUT:
					mode = GPIO_MODE_INPUT;
					break;
			case GPIO_TYPE_OUTPUT:
			case GPIO_TYPE_POST_SET_OUTPUT:
			case GPIO_TYPE_PRE_SET_OUTPUT:
					mode = GPIO_MODE_OUTPUT;
					break;
			case GPIO_TYPE_RTC_INPUT:
					mode = RTC_IO_MODE_INPUT;
					break;
			case GPIO_TYPE_RTC_OUTPUT:
					mode = RTC_IO_MODE_OUTPUT;
					break;
			default:
				break;
		}
		
		if((type < GPIO_TYPE_RTC_INPUT) && (type > GPIO_TYPE_NO_CHANGE))
		{	if((gpioNo >= NU_PF4) && (gpioNo <= NU_PF11))
			{	RTC_SET_IOCTL_BY_GPIO();
			}
		}
		
		if(type == GPIO_TYPE_PRE_SET_OUTPUT)
		{	SENS_GPIO_WRITE(gpioNo, val);
		}
		if((type >= GPIO_TYPE_RTC_INPUT) && (type <= GPIO_TYPE_RTC_OUTPUT))
		{	if((gpioNo < NU_PF4) || (gpioNo > NU_PF11))
			{	dbgMsg("%s", "Not in Vbat Domain power\r\n");
				return;
			}
			if(!(RTC->LXTCTL & RTC_LXTCTL_IOCTLSEL_Msk))	//by GPIO
			{	RTC_SET_IOCTL_BY_RTC();
			}
			
			if(_pullsel == GPIO_PULLSEL_NONE)
				pullSel = RTC_IO_PULL_UP_DOWN_DISABLE;
			else if(_pullsel == GPIO_PULLSEL_PULLHIGH)
				pullSel = RTC_IO_PULL_UP_ENABLE;
			else if(_pullsel == GPIO_PULLSEL_PULLLOW)
				pullSel = RTC_IO_PULL_DOWN_ENABLE;
			RTC_SetGPIOMode(pin, mode, RTC_IO_DIGITAL_ENABLE, pullSel, val);
		}
		else
			GPIO_SetMode(getNuGpioPortInst(port), pin, GPIO_MODE_OUTPUT);
		
		
		//GPIO_SetPullCtl
	}
	else if(gpioNo < EXT1_P0_0)
	{
	}
	else if(gpioNo < GPIO_MAX)
	{
	}
}
//------------------------------------------------------------------------------
void disableAllModule(void)
{
	uint32_t irqNo;
	for(irqNo=ISP_IRQn;irqNo <= EADC23_IRQn;irqNo++)
	{	if(irqNo != RTC_IRQn)
		{	NVIC_DisableIRQ(irqNo);
			NVIC_ClearPendingIRQ(irqNo);
		}
	}
	
	SYS_UnlockReg();
	
}
//------------------------------------------------------------------------------
void sysPwrOffCtrl(uint32_t offTimer, uint32_t nextWakeupTimestamp)
{	
#if defined SENSMINIA4_NEO
	sdWriteLock();
#if EN_WATCHDOG
	watchDogClr();
#endif
	setVBatRegValue(RTC_SPARE_REG_ITEM_NEXT_WAKEUP_TIMESTAMP, nextWakeupTimestamp);
	powerManagerChipCtrl(PWR_MNG_IC_ACTION_ID_WRITE_WAKEUP_TIME, offTimer);
	PIN_SHUTDOWN_REQn = 0;
	SENS_TIME_DELAY(10);
	PIN_SHUTDOWN_REQn = 1;
	while(1);
	
#elif defined SENSMINIA4_PLUS
	pmu_wdt_clr(sensSys->sysStandbyTime/5 + 2);
	dbg_msg("[SETTING] 1. Sleeping time: %d sec.\r\n", sensSys->sysStandbyTime);
	SENS_TIME_DELAY(2000);        
	sdWriteLock();
	while(1)
	{	pmu_standby(sensSys->sysStandbyTime); 
		SENS_TIME_DELAY(2000);
	}
#endif
}

//------------------------------------------------------------------------------
void enterPowerSavingMode(void)
{	//uint32_t u32TimeOutCnt;
	
#if 1
	
#else
	taskENTER_CRITICAL();
	
	SysTick->CTRL = 0;
	CLK_DisableModuleClock(USBH_MODULE);
	
	
	CLK_SetPowerDownMode(CLK_PMUCTL_PDMSEL_DPD);

	CLK_ENABLE_RTCWK();
	
	u32TimeOutCnt = SystemCoreClock;
	UART_WAIT_TX_EMPTY(CONSOLE_UART_CTX)
	if(--u32TimeOutCnt == 0)	
		break;
	
	CLK_PowerDown();
	
	while(1)
	{	SYS_ResetChip();
	}
#endif
}
//------------------------------------------------------------------------------
static int findDoMap(int doChannel)
{	int realWiredChannel = DO_NO_WIRED;
	int idx;
	EEPROM_CONTEXT *eepromCtx = (EEPROM_CONTEXT *)sensSys->eepromBuf;
	
	for(idx=0;idx<DOQUANTITY;idx++)
	{	if(eepromCtx->doMap[idx] == doChannel)
		{	realWiredChannel = idx;
			break;
		}
	}
	return realWiredChannel;
}
//------------------------------------------------------------------------------
void setDoPwrStatus(int channel, uint8_t value, uint8_t flag)
{	uint8_t onOff = (value ? 1:0);
	int realWiredCh = findDoMap(channel);
	
	if(realWiredCh == DO_NO_WIRED)
		return;
	/*if(onOff && sensDev->doOnCount[channel])
		return;
	else if(!onOff && !sensDev->doInst.doCtx.doOnCnt[channel])
		return;*/
	if(onOff && sensDev->doInst.doCtx.doOnCnt[realWiredCh])
	{	sensDev->doInst.doCtx.doOnCnt[realWiredCh]++;
		dbg_msg("set DO-%d(real DO-%d) ON, already ON, DO On Count:%d\r\n", channel, realWiredCh, sensDev->doInst.doCtx.doOnCnt[realWiredCh]);
		return;
	}
	else if(!onOff && (sensDev->doInst.doCtx.doOnCnt[realWiredCh] > 1))
	{	sensDev->doInst.doCtx.doOnCnt[realWiredCh]--;
		dbg_msg("set DO-%d(real DO-%d) OFF, reamin DO On Count:%d\r\n", channel, realWiredCh, sensDev->doInst.doCtx.doOnCnt[realWiredCh]);
		return;
	}
	
	gExpGpioOutVal.buck12vEn = 1;
	
	if(realWiredCh == 0)
		//SENS_GPIO_WRITE(EXT1_P0_2, onOff);			// GPIOEX1_DO0
		gExpGpioOutVal.do0En = onOff;
	else if(realWiredCh == 1)
		//SENS_GPIO_WRITE(EXT1_P0_3, onOff);			// GPIOEX1_DO1
		gExpGpioOutVal.do1En = onOff;
	else if(realWiredCh == 2)
		//SENS_GPIO_WRITE(EXT1_P0_4, onOff);			// GPIOEX1_DO2 and ISO_LOUT2_12V
		gExpGpioOutVal.do2En = onOff;
	else if(realWiredCh == 3)
		//SENS_GPIO_WRITE(EXT1_P0_5, onOff);			// GPIOEX1_DO3 and ISO_LOUT3_12V
		gExpGpioOutVal.do3En = onOff;
	
	if(gExpGpioOutVal.doEn)
	{	gExpGpioOutVal.buck12vEn = 1;
		PIN_ISO_12V_EN = 1;
	}
	else
	{	gExpGpioOutVal.buck12vEn = 0;
		PIN_ISO_12V_EN = 0;
	}
	pcal6416GpioCtrl(gExpGpioOutVal.val);
	
	if(onOff)
		sensDev->doInst.doCtx.doOnCnt[realWiredCh]++;
	else
	{	if(sensDev->doInst.doCtx.doOnCnt[realWiredCh])
			sensDev->doInst.doCtx.doOnCnt[realWiredCh]--;
	}
	
	if(flag == SET_DO_BOTH)	//set cfg & sts
	{	sensDev->doInst.doCtx.doSts[realWiredCh] = onOff;
		sensPq->onboardPq[ON_BOARD_PQ_DO_STS_0 + channel]  = onOff;
		
		//sensSys->sysCfg.doCfg.powerCtrl[realWiredCh] = onOff;	//do not set this
	}
	else if(flag == SET_DO_STS)	//set status
	{	sensDev->doInst.doCtx.doSts[realWiredCh] = onOff;
		sensPq->onboardPq[ON_BOARD_PQ_DO_STS_0 + channel]  = onOff;
	}
	else if(flag == SET_DO_PWR)	//set cfg
	{	//sensSys->sysCfg.doCfg.powerCtrl[realWiredCh] = onOff;
	}
	setModbusCoil(realWiredCh, onOff);
	dbg_msg("set DO-%d(real DO-%d) %s, count:%d\r\n", channel, realWiredCh, onOff? "ON":"OFF", sensDev->doInst.doCtx.doOnCnt[realWiredCh]);
}
//------------------------------------------------------------------------------
static void setIOUsbChannel(enum IOT_IF_SEL iotIfSel, uint8_t enable)
{	if(iotIfSel == IOT_IF_SEL_IOT1_USB)
	{	PIN_USB_FS_NOE = 1;
		SENS_TIME_DELAY(100);
		PIN_USB_FS_SEL = 0;
		PIN_USB_FS_NOE = enable? 0:1;
	}
	else if(iotIfSel == IOT_IF_SEL_IOT2_USB)
	{	PIN_USB_FS_NOE = 1;
		SENS_TIME_DELAY(100);
		PIN_USB_FS_SEL = 1;
		PIN_USB_FS_NOE = enable? 0:1;
	}
	else
	{	PIN_USB_FS_NOE = 1;
	}
	
	if(enable)
	{	//SYS_UnlockReg();
		//SET_USB_OTG_ID_PA15();
		//SET_USB_VBUS_PA12();
		//SET_USB_VBUS_ST_PB7();	//only for Mikro bus
		
		//SET_USB_VBUS_EN_PB6();	//only for Mikro bus
	}
	else
	{	//SET_GPIO_PA15();
		//SET_GPIO_PA12();
		PIN_USB_FS_VBUS_EN = 0;
		GPIO_SetMode(PB, BIT6, GPIO_MODE_OUTPUT);
		SET_GPIO_PB6();
		//SET_GPIO_PB7();
		//SET_USB_VBUS_PA12();
	}
}
//------------------------------------------------------------------------------
static void mkbus5vCtrl(enum IOT_IF_SEL iotIfSel, uint8_t enable)
{	switch(iotIfSel)
	{	case IOT_IF_SEL_MK1_UART:
		case IOT_IF_SEL_MK1_I2C:
		case IOT_IF_SEL_MK1_SPI:	
				//pcal6416GpioSetVal(GPIOEX1_MK1_PWR_EN, enable);	
				gExpGpioOutVal.mk1PwrEn = enable;
				pcal6416GpioCtrl(gExpGpioOutVal.val);
				break;
		case IOT_IF_SEL_MK2_UART:
		case IOT_IF_SEL_MK2_I2C:
		case IOT_IF_SEL_MK2_SPI:	
				gExpGpioOutVal.mk2PwrEn = enable;
				pcal6416GpioCtrl(gExpGpioOutVal.val);
				//pcal6416GpioSetVal(GPIOEX1_MK2_PWR_EN, enable);	
				break;
		case IOT_IF_SEL_MK1_USB:
		case IOT_IF_SEL_MK2_USB:
				{	if(enable)	//IOT priority high?? must check??
					{	SET_USB_VBUS_EN_PB6();
					}
					else
					{	PIN_USB_FS_VBUS_EN = 0;
						GPIO_SetMode(PB, BIT6, GPIO_MODE_OUTPUT);
						SET_GPIO_PB6();
					}
				}
		default:
				break;
	}
}
//------------------------------------------------------------------------------
// only one channel available
static void setIotUartChannel(enum IOT_IF_SEL iotIfSel, uint8_t enable)
{	if(enable)
	{	if(iotIfSel == IOT_IF_SEL_IOT1_UART)
		{	gExpGpioOutVal.uartExA = 0;
			//pcal6416GpioSetVal(UARTEX_A0, 0);
			//pcal6416GpioSetVal(UARTEX_A1, 0);
		}
		else if(iotIfSel == IOT_IF_SEL_IOT2_UART)
		{	//pcal6416GpioSetVal(UARTEX_A0, 1);
			//pcal6416GpioSetVal(UARTEX_A1, 0);
			gExpGpioOutVal.uartExA = 1;
		}
		else if(iotIfSel == IOT_IF_SEL_MK1_UART)
		{	//pcal6416GpioSetVal(UARTEX_A0, 0);
			//pcal6416GpioSetVal(UARTEX_A1, 1);
			gExpGpioOutVal.uartExA = 2;
		}
		else if(iotIfSel == IOT_IF_SEL_MK2_UART)
		{	//pcal6416GpioSetVal(UARTEX_A0, 1);
			//pcal6416GpioSetVal(UARTEX_A1, 1);
			gExpGpioOutVal.uartExA = 3;
		}
	}
	gExpGpioOutVal.uartExnEn = enable? 0:1;
	//pcal6416GpioSetVal(UARTEX_nEN, enable? 0:1);
	pcal6416GpioCtrl(gExpGpioOutVal.val);
}
//------------------------------------------------------------------------------
void iotVbattCtrl(uint8_t channel, uint8_t enable)
{	IO_PWR_CTL	*iotVbatt;
	uint32_t bmpTemp;
	
	iotVbatt = (IO_PWR_CTL *)&sensDev->iotVbatt;
	bmpTemp = iotVbatt->onBmp;
	if(!enable)
		bmpTemp &= ~(1<<channel);
	if(enable && iotVbatt->onBmp)
	{	//iotVbatt->powerEnableCount++;
		iotVbatt->onBmp |= (1 << channel);
		dbgMsg("set IoT VBATT ON, now already ON, on count:%d\r\n", iotVbatt->powerEnableCount);
		return;
	}
	else if(!enable && bmpTemp)
	{	iotVbatt->onBmp &= ~(1 << channel);
		dbgMsg("set IoT VBATT OFF, Keep ON, on count:%d\r\n", iotVbatt->powerEnableCount);
		return;
	}
	
	PIN_VBATT_IOT_EN = enable;
	
	if(enable)
	{	//iotVbatt->powerEnableCount++;
		iotVbatt->onBmp |= (1 << channel);
	}
	else
	{	/*if(iotVbatt->powerEnableCount)
			iotVbatt->powerEnableCount--;*/
		iotVbatt->onBmp &= ~(1 << channel);
	}
	
	dbg_msg("set IoT VBATT %s, ON BMP:%X\r\n", enable? "ON":"OFF", iotVbatt->onBmp);
}
//------------------------------------------------------------------------------
void iotPwrCtrl(enum IOT_IF_SEL iotIfSel, uint8_t enable)
{	
	if((iotIfSel == IOT_IF_SEL_IOT1_USB) || (iotIfSel == IOT_IF_SEL_IOT2_USB))	//IOT1, IO2 uss FS USB
	{	setIOUsbChannel(iotIfSel, enable);
	}
	else if(iotIfSel < IOT_IF_SEL_IOT1_USB)	//
	{	setIotUartChannel(iotIfSel, enable);
	}
	
	if((iotIfSel == IOT_IF_SEL_IOT1_USB) || (iotIfSel == IOT_IF_SEL_IOT1_UART))
	{	iotVbattCtrl(0, enable);
	}
	else if((iotIfSel == IOT_IF_SEL_IOT2_USB) || (iotIfSel == IOT_IF_SEL_IOT2_UART))
	{	iotVbattCtrl(1, enable);
	}
	else	//Mikro Bus power ctrl
		mkbus5vCtrl(iotIfSel, enable);
	
	if((iotIfSel == IOT_IF_SEL_IOT1_USB) || (iotIfSel == IOT_IF_SEL_IOT1_UART))
	{	PIN_IOT1_VBATT_EN = enable;
		//PIN_IOT1_VIO_SEL = 0;
	}
	else if((iotIfSel == IOT_IF_SEL_IOT2_USB) || (iotIfSel == IOT_IF_SEL_IOT2_UART))
	{	PIN_IOT2_VBATT_EN = enable;
		//PIN_IOT2_VIO_SEL = 0;
	}
	//SENS_TIME_DELAY(1000);
}
//------------------------------------------------------------------------------
void setRs485Pwr(int channel, int onOff)
{	IO_PWR_CTL *rs485PwrCtrl = (IO_PWR_CTL *)&sensDev->rs485PwrCtrl[channel];
	
	if(rs485PwrCtrl->powerEnableCount && onOff)
	{	dbgMsg("[RS485] pwr already ON, enable count:%d\r\n", rs485PwrCtrl->powerEnableCount);
		rs485PwrCtrl->powerEnableCount++;
		return;
	}
	else if(!onOff && (rs485PwrCtrl->powerEnableCount > 1))
	{	dbgMsg("[RS485] Turn off pwr, enable count:%d, keep\r\n", rs485PwrCtrl->powerEnableCount);
		rs485PwrCtrl->powerEnableCount--;
		return;
	}
	
	
	if(channel == EXT_IF_CHANNEL_RS485_1)
	{	PIN_RS485_1_PWR_EN = onOff;
#if BOARD_VERSION == 1
		if(onOff)
			PIN_RS485_2_PWR_EN = onOff;
#endif	
	}
	else
	{	
#if BOARD_VERSION == 1
		if((PIN_RS485_1_PWR_EN == 1) && !onOff)
		{
		}
		else
#endif
		PIN_RS485_2_PWR_EN = onOff;
	}
	
	dbgMsg("[RS485] Channel %d, Set power %s\r\n", channel, onOff? "ON":"OFF");
	
	if(onOff)
		rs485PwrCtrl->powerEnableCount++;
	else if(rs485PwrCtrl->powerEnableCount)
		rs485PwrCtrl->powerEnableCount--;
}
//------------------------------------------------------------------------------
void setAdcChipPwr(int onOff)
{	PIN_ISO_ADC_3V3_EN = onOff;
}
//------------------------------------------------------------------------------
void Power24V_enable(uint8_t onOff)
{	SENS_GPIO_WRITE(EXT1_P1_6, onOff);
	sensPq->onboardPq[ON_BOARD_PQ_DO24_STS] = onOff;
	dbg_msg("Set 24V Power %s\r\n", onOff? "ON":"OFF");
}
//------------------------------------------------------------------------------
void setOutputPower(int devPwrSrc, uint8_t onOff, const char *caller, int callerLine)
{	dbg_msg("set do %d, %s, call from %s(%d)\r\n", devPwrSrc, onOff? "ON":"OFF", caller, callerLine);
	if(devPwrSrc == DEVICE_PWR_SRC_24V)
		Power24V_enable(onOff);
	else if(devPwrSrc <= DEVICE_PWR_SRC_DO1)
		setDoPwrStatus((int)devPwrSrc, onOff, SET_DO_STS);
}
//------------------------------------------------------------------------------
void doPwrOutSetting(void)
{	if(sensSys->sysCfg.doCfg.mode == 0 || sensSys->sysCfg.doCfg.mode == 1)
	{	if(sensSys->sysCfg.doCfg.powerCtrl[0] == 1 || sensSys->sysCfg.doCfg.powerCtrl[1] == 1 || sensSys->sysCfg.doCfg.DO24VPwr == 1)
		{	gExpGpioOutVal.buck12vEn = 1;
			pcal6416GpioCtrl(gExpGpioOutVal.val);
			//SENS_GPIO_WRITE(GPIOEX1_NO_BUCK_12V_EN, 1); // BUCK_12V_EN / VDD12V_DO_EN
			PIN_ISO_12V_EN = 1;
		}
		
		dbg_msg("Do power status [DO-0 %d, DO-1 %d]\r\n", sensSys->sysCfg.doCfg.powerCtrl[0], sensSys->sysCfg.doCfg.powerCtrl[1]);

		if(sensSys->sysCfg.doCfg.powerCtrl[0] == 1)
		{	setDoPwrStatus(DEVICE_PWR_SRC_DO0, 1, SET_DO_STS);
		}
		else if(sensSys->sysCfg.doCfg.powerCtrl[0] == 0)
		{	setDoPwrStatus(DEVICE_PWR_SRC_DO0, 0, SET_DO_STS);
		}

		if(sensSys->sysCfg.doCfg.powerCtrl[1] == 1)
		{	setDoPwrStatus(DEVICE_PWR_SRC_DO1, 1, SET_DO_STS);
		}
		else if(sensSys->sysCfg.doCfg.powerCtrl[1] == 0)
		{	setDoPwrStatus(DEVICE_PWR_SRC_DO1, 0, SET_DO_STS);
		}

		if(sensSys->sysCfg.doCfg.DO24VPwr == 1)
		{ setOutputPower(DEVICE_PWR_SRC_24V, 1, __func__, __LINE__);
		}
		else if(sensSys->sysCfg.doCfg.DO24VPwr == 0)
		{ setOutputPower(DEVICE_PWR_SRC_24V, 0, __func__, __LINE__);
		}
		
	}
}