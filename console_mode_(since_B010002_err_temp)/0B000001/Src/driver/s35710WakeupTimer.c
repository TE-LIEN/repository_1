#include "driver/s35710WakeupTimer.h"
#include "sensCommon.h"

//------------------------------------------------------------------------------
void powerManagerChipCtrl(enum PWR_MNG_IC_ACTION_ID actionId, uint32_t time)
{	uint8_t writeBuf[3];
	uint8_t readBuf[3];
	uint8_t	address;
	
	if(actionId == PWR_MNG_IC_ACTION_ID_WRITE_WAKEUP_TIME)
	{	
		#if 0 //no need to reset wakeup timer
		#if !WAKEUP_TIMER_USE_CARRIER_BOARD //Is are MCU board
		PIN_WAKEUP_nRST = 0;
		SENS_TIME_DELAY(600);	// Specification is 500ms
		PIN_WAKEUP_nRST = 1;
		SENS_TIME_DELAY(600);
		#else //Is are Carrier board
		PIN_WAKEUP1_nRST = 0;
		SENS_TIME_DELAY(600);	// Specification is 500ms
		PIN_WAKEUP1_nRST = 1;
		SENS_TIME_DELAY(600);
		#endif
		#endif
		i2cInit(WAKEUP_TIME_I2C_ID, 100000, 32);
		address = 0x01;
		i2cRead(WAKEUP_TIME_I2C_ID, S35710_SLA, address, &readBuf[0], 3, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
		dbgMsg("Read s-35710 timer is 0x%06x\r\n", readBuf[0]<<16 | readBuf[1]<<8 | readBuf[2]);
		
		address = 0x81;
		writeBuf[0] = (uint8_t)(time >> 16);
		writeBuf[1] = (uint8_t)(time >> 8);
		writeBuf[2] = (uint8_t)time;
		i2cWrite(WAKEUP_TIME_I2C_ID, S35710_SLA, address, &writeBuf[0], 3);
		
		address = 0x01;
		readBuf[0] = 0;
		readBuf[1] = 0;
		readBuf[2] = 0;
		i2cRead(WAKEUP_TIME_I2C_ID, S35710_SLA, address, &readBuf[0], 3, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
		dbgMsg("Read again s-35710 timer is 0x%06x\r\n", readBuf[0]<<16 | readBuf[1]<<8 | readBuf[2]);
	}
	else if(actionId == PWR_MNG_IC_ACTION_ID_RESET)
	{	// RTC_SET_IOCTL_BY_GPIO();
		// PIN_WAKEUP_nRST = 0;
		// SENS_TIME_DELAY(600);
		// PIN_WAKEUP_nRST = 1;
		// SENS_TIME_DELAY(600);
		
		address = 0x01;
		i2cRead(WAKEUP_TIME_I2C_ID, S35710_SLA, address, &readBuf[0], 3, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
		
		// RTC_SetGPIOMode(7, RTC_IO_MODE_INPUT, RTC_IO_DIGITAL_ENABLE, RTC_IO_PULL_UP_DOWN_DISABLE, 1);	//PIN_WAKEUP_NINT
		// RTC_SetGPIOMode(6, RTC_IO_MODE_OUTPUT, RTC_IO_DIGITAL_ENABLE, RTC_IO_PULL_UP_DOWN_DISABLE, 1);	//PIN_WAKEUP_NRST
		// RTC_SetGPIOMode(9, RTC_IO_MODE_OUTPUT, RTC_IO_DIGITAL_ENABLE, RTC_IO_PULL_UP_DOWN_DISABLE, 1);	//PIN_MCU_DPD_EN, power off when done.
		// RTC_SetGPIOMode(9, RTC_IO_MODE_OUTPUT, RTC_IO_DIGITAL_ENABLE, RTC_IO_PULL_UP_DOWN_DISABLE, 0);	//PIN_MCU_DPD_EN
		dbgMsg("Read again s-35710 timer is 0x%06x\n", readBuf[0]<<16 | readBuf[1]<<8 | readBuf[2]);
		/*while(1)
		{	SENS_TIME_DELAY(1000);
			i2cRead(I2C2_ID, S35710_SLA, address, &readBuf[0], 3, I2C_READ_MODE_FIRST_ISSUE_SLA_R);
		}*/
		
	}
	
}