#include "driver/PCAL6416GpioExpander.h"
#include "sensCommon.h"

#include "driver/i2cDrv.h"

/**************************************************************************
 *********             PCAL6416A DEFINE                           *********
 **************************************************************************/

//void GPF_IRQHandler(void);
volatile EXPANDER_GPIO_OUT gExpGpioOutVal;
/**************************************************************************
 *********             PCAL6416A CONTROLLER FUNCTION              *********
 **************************************************************************/
#if 0
void GPF_IRQHandler(void)
{	volatile uint32_t u32temp;

	// NVIC_DisableIRQ(GPF_IRQn);
	// pcal6416GpioIntDisable();
	/* To check if PF.9 interrupt occurred */
	if(GPIO_GET_INT_FLAG(PF, BIT9))
	{	printf("PF.9 INT occurred.\n");
		// gPcal6416GpioIntStatus = pcal6416GpioGetIntStatus();
		GPIO_CLR_INT_FLAG(PF, BIT9);
		// NVIC_EnableIRQ(GPF_IRQn);
		// pcal6416GpioIntEnable();
		// return;
	}
	/* To check if PF.8 interrupt occurred */
	else if(GPIO_GET_INT_FLAG(PF, BIT8))
	{	GPIO_CLR_INT_FLAG(PF, BIT8);
		printf("PF.8 INT occurred.\n");
	}
	else
	{	/* Un-expected interrupt. Just clear all PF interrupts */
		u32temp = PF->INTSRC;
		PF->INTSRC = u32temp;
		printf("Un-expected interrupts.\n");
	}
}
#endif
//------------------------------------------------------------------------------
void pcal6416GpioSetIoMode(uint8_t channel, uint16_t configVal)
{	uint8_t sla;
	
	sla = (channel == OUTPUT_EXPANDER_CHANNEL)? OUTPUT_EXPANDER_SLA:INPUT_EXPANDER_SLA;
	i2cWrite(I2C1_ID, sla, PCAL6416A_REG_CONFIG_PORT0, (uint8_t *)&configVal, 2);
}
//------------------------------------------------------------------------------
void pcal6416GpioIntEnable(uint16_t irqMask)
{ //uint8_t dataBuf[2];
  
  //dataBuf[0] = irqMask;
	i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_0_INPUT_LATCH, (uint8_t *)&irqMask, 2);
	irqMask = ~irqMask;
	i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_0_INT_MASK, (uint8_t *)&irqMask, 2);
}
//------------------------------------------------------------------------------
void pcal6416GpioIntDisable(void)
{ uint8_t dataBuf[1];
  
  dataBuf[0] = 0xFF;
  i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_0_INT_MASK, dataBuf, 1);
  i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_1_INT_MASK, dataBuf, 1);
}
//------------------------------------------------------------------------------
uint16_t pcal6416GpioGetIntStatus(void)
{ uint8_t dataBuf[2];
  i2cRead(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_0_INT_STATUS, dataBuf, 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
  return (dataBuf[1] << 8) | dataBuf[0];
}
//------------------------------------------------------------------------------
uint16_t pcal6416GpioGetVal(uint8_t sla)
{	uint8_t dataBuf[2];
	uint16_t addr;
	
	if(sla == INPUT_EXPANDER_SLA)
		addr = PCAL6416A_REG_INPUT_PORT0;
	else
		addr = PCAL6416A_REG_OUTPUT_PORT0;
	
	i2cRead(I2C1_ID, sla, addr, dataBuf, 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
	return (dataBuf[1] << 8) | dataBuf[0];
}
//------------------------------------------------------------------------------
void pcal6416GpioCtrl(uint16_t val)
{ uint8_t dataBuf[2];
  dataBuf[0] = val & 0xFF;
  dataBuf[1] = (val >> 8) & 0xFF;
  i2cWrite(I2C1_ID, OUTPUT_EXPANDER_SLA, PCAL6416A_REG_OUTPUT_PORT0, dataBuf, 2);
}
//------------------------------------------------------------------------------
void pcal6416GpioSetVal(uint16_t gpioepin, bool val)
{ uint8_t dataBuf[2];
  uint16_t currentVal = pcal6416GpioGetVal(OUTPUT_EXPANDER_SLA);
  if(val)
    currentVal |= gpioepin;
  else
    currentVal &= ~gpioepin;
  dataBuf[0] = currentVal & 0xFF;
  dataBuf[1] = (currentVal >> 8) & 0xFF;
  i2cWrite(I2C1_ID, OUTPUT_EXPANDER_SLA, PCAL6416A_REG_OUTPUT_PORT0, dataBuf, 2);
}
//------------------------------------------------------------------------------
void initExpanderCtrlChannel(void)
{	SYS_UnlockReg();
	
	GPIO_SetMode(PE, BIT13, GPIO_MODE_OUTPUT);
	PIN_GPIOEX_I2C_EN = 1;
	
	i2cInit(I2C1_ID, 100000, 32);
}
//------------------------------------------------------------------------------
void initPCAL6416GpioExpander(uint16_t irqMask)
{	//uint8_t writeBuf[2];
	uint16_t writeWBuf;
#if WRITE_VALUE_CHECK_FIRST
	uint16_t readWVal;
#endif
	//uint16_t inputLatch = ~irqMask;
#if DI_IRQ_NEW_CIRCUIT
	uint16_t inputLatch = 0x0000;	//clear all input latch
#else
	uint16_t inputLatch = 0xFFFF;	//set all input latch
#endif
  
	initExpanderCtrlChannel();
	
	// Configure PCAL6416_01 pins to input
	writeWBuf = 0xFFFF;
#if WRITE_VALUE_CHECK_FIRST
	i2cRead(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_CONFIG_PORT0, (uint8_t *)&readWVal, 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
	if(readWVal != writeWBuf)
#endif
	{	i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_CONFIG_PORT0, (uint8_t *)&writeWBuf, 2);
		//i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_CONFIG_PORT1, dataBuf, 1);
	}
	
#if WRITE_VALUE_CHECK_FIRST
	i2cRead(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_POLARITY_INV_PORT0, (uint8_t *)&readWVal, 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
	if(readWVal != writeWBuf)
#endif
	{	i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_POLARITY_INV_PORT0, (uint8_t *)&writeWBuf, 2);
	}
	
#if WRITE_VALUE_CHECK_FIRST
	i2cRead(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_0_INPUT_LATCH, (uint8_t *)&readWVal, 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
	if(readWVal != inputLatch)
#endif
	{	i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_0_INPUT_LATCH, (uint8_t *)&inputLatch, 2);
	}
	
	// Configure PCAL6416_01 pins to interrupt enable
#if WRITE_VALUE_CHECK_FIRST
	i2cRead(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_0_INT_MASK, (uint8_t *)&readWVal, 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
	if(readWVal != irqMask)
#endif
	{	i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_0_INT_MASK, (uint8_t *)&irqMask, 2);
		//i2cWrite(I2C1_ID, INPUT_EXPANDER_SLA, PCAL6416A_REG_1_INT_MASK, dataBuf, 1);
	}
	
	writeWBuf = 0;
#if WRITE_VALUE_CHECK_FIRST
	i2cRead(I2C1_ID, OUTPUT_EXPANDER_SLA, PCAL6416A_REG_CONFIG_PORT0, (uint8_t *)&readWVal, 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
	if(readWVal != writeWBuf)
#endif
	{	// Configure PCAL6416_02 pins to output
		i2cWrite(I2C1_ID, OUTPUT_EXPANDER_SLA, PCAL6416A_REG_CONFIG_PORT0, (uint8_t *)&writeWBuf, 2);
		//i2cWrite(I2C1_ID, OUTPUT_EXPANDER_SLA, PCAL6416A_REG_CONFIG_PORT1, dataBuf, 1);
	}
	//pcal6416GpioCtrl(GPIOE_PORT_ALL_LOW);
	gExpGpioOutVal.val = 0;
	gExpGpioOutVal.uartExnEn = 1;	//turn off uart multiplexer
	gExpGpioOutVal.diEn = 1;		//keep turn on DI
	//pcal6416GpioCtrl(GPIOE_PORT_UARTEX_nEN_OFF);
	pcal6416GpioCtrl(gExpGpioOutVal.val);

	// Enable interrupt for PCAL6416A INT pins
	/*
	GPIO_EnableInt(PF, 9, GPIO_INT_FALLING);
	//GPIO_EnableInt(PF, 8, GPIO_INT_FALLING);
#ifdef SENSMINIA4_NEO
	NVIC_SetPriority(GPF_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
#endif
	 NVIC_EnableIRQ(GPF_IRQn);
	 */
}
/**************************************************************************
 *********             PCAL6416A CONTROLLER FUNCTION END          *********
 **************************************************************************/