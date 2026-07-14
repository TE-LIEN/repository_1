#ifndef __PCAL6416_Gpio_Expander_H__
#define __PCAL6416_Gpio_Expander_H__

#include "sensminiCfg.h"

#define PCAL6416A01_SLA                   0x20
#define PCAL6416A02_SLA                   0x21

#define INPUT_EXPANDER_SLA					PCAL6416A01_SLA
#define OUTPUT_EXPANDER_SLA					PCAL6416A02_SLA

#define INPUT_EXPANDER_CHANNEL				0
#define OUTPUT_EXPANDER_CHANNEL				1

#define WRITE_VALUE_CHECK_FIRST				1

/* PCAL6416A REGISTER DEFINE */
#define PCAL6416A_REG_INPUT_PORT0         0x00
#define PCAL6416A_REG_INPUT_PORT1         0x01
#define PCAL6416A_REG_OUTPUT_PORT0        0x02
#define PCAL6416A_REG_OUTPUT_PORT1        0x03
#define PCAL6416A_REG_POLARITY_INV_PORT0  0x04
#define PCAL6416A_REG_POLARITY_INV_PORT1  0x05
#define PCAL6416A_REG_CONFIG_PORT0        0x06
#define PCAL6416A_REG_CONFIG_PORT1        0x07
#define PCAL6416A_REG_00_OUTPUT_DRIVER_STRENGTH    0x40
#define PCAL6416A_REG_01_OUTPUT_DRIVER_STRENGTH    0x41
#define PCAL6416A_REG_10_OUTPUT_DRIVER_STRENGTH    0x42
#define PCAL6416A_REG_11_OUTPUT_DRIVER_STRENGTH    0x43
#define PCAL6416A_REG_0_INPUT_LATCH       0x44
#define PCAL6416A_REG_1_INPUT_LATCH       0x45
#define PCAL6416A_REG_0_PULL_UP_DOWN_ENABLE       0x46
#define PCAL6416A_REG_1_PULL_UP_DOWN_ENABLE       0x47
#define PCAL6416A_REG_0_PULL_UP_DOWN_SELECT       0x48
#define PCAL6416A_REG_1_PULL_UP_DOWN_SELECT       0x49
#define PCAL6416A_REG_0_INT_MASK          0x4A
#define PCAL6416A_REG_1_INT_MASK          0x4B
#define PCAL6416A_REG_0_INT_STATUS        0x4C
#define PCAL6416A_REG_1_INT_STATUS        0x4D
#define PCAL6416A_REG_OUTPUT_PORT_CFG     0x4F

/* GPIO EXPANDER PIN DEFINITIONS */
#define GPIOE_PORT_ALL_HIGH  0xFFFF
#define GPIOE_PORT_ALL_LOW   0x0000
#define GPIOE_PORT0    0
#define GPIOE_PORT1    1

#define GPIOE_P0_0    0x0001
#define GPIOE_P0_1    0x0002
#define GPIOE_P0_2    0x0004
#define GPIOE_P0_3    0x0008
#define GPIOE_P0_4    0x0010
#define GPIOE_P0_5    0x0020
#define GPIOE_P0_6    0x0040
#define GPIOE_P0_7    0x0080
#define GPIOE_P1_0    0x0100
#define GPIOE_P1_1    0x0200
#define GPIOE_P1_2    0x0400
#define GPIOE_P1_3    0x0800
#define GPIOE_P1_4    0x1000
#define GPIOE_P1_5    0x2000
#define GPIOE_P1_6    0x4000
#define GPIOE_P1_7    0x8000

/* GPIO EXPANDER INPUT PINT DEFINITIONS */
#define GPIOEX_DI2    (GPIOE_P0_0)
#define GPIOEX_DI3    (GPIOE_P0_1)
#define GPIOEX_DI4    (GPIOE_P0_2)
#define GPIOEX_DI5    (GPIOE_P0_3)
#define GPIOEX_DI6    (GPIOE_P0_4)
#define GPIOEX_P6     (GPIOE_P0_6)
#define GPIOEX_P7     (GPIOE_P0_7)
#define GPIOEX_P8     (GPIOE_P1_0)
#define TEMP_ALERT    (GPIOE_P1_1)
#define I2C0_IRQ      (GPIOE_P1_2)
#define I2C1_IRQ      (GPIOE_P1_3)
#define I2C2_IRQ      (GPIOE_P1_4)
#define GPIOEX_P13    (GPIOE_P1_5)
#define GPIOEX_P14    (GPIOE_P1_6)
#define GPIOEX_P15    (GPIOE_P1_7)

/* GPIO EXPANDER OUTPUT PINT DEFINITIONS */
#define ISENS_SOLAR_EN   (GPIOE_P0_0)
#define ISENS_LOAD_EN    (GPIOE_P0_1)
#define GPIOEX1_DO0      (GPIOE_P0_2)
#define GPIOEX1_DO1      (GPIOE_P0_3)
#define GPIOEX1_DO2      (GPIOE_P0_4)
#define GPIOEX1_DO3      (GPIOE_P0_5)
#define GPIOEX1_DO4      (GPIOE_P0_6)
	#define GPIOEX1_MK1_PWR_EN		GPIOEX1_DO4
#define GPIOEX1_DO5      (GPIOE_P0_7)
	#define GPIOEX1_MK2_PWR_EN		GPIOEX1_DO5
#define BUCK_12V_EN      (GPIOE_P1_0)
#define LDO_1V8_EN       (GPIOE_P1_1)
#define UARTEX_nEN       (GPIOE_P1_2)
#define UARTEX_A0        (GPIOE_P1_3)
#define UARTEX_A1        (GPIOE_P1_4)
#define DI_EN            (GPIOE_P1_5)
#define GPIOEX1_P14      (GPIOE_P1_6)
#define GPIOEX1_P15      (GPIOE_P1_7)

#define GPIOE_PORT_UARTEX_nEN_OFF   UARTEX_nEN

typedef union _EXPANDER_GPIO_OUT
{	uint16_t val;
	struct 
	{	uint16_t isensEn:2;
		uint16_t doEn:4;
		uint16_t mkPwrEn:2;
		uint16_t independentCtrl0:3;
		uint16_t uartExA:2;
		uint16_t independentCtrl1:3;
	};
	struct
	{	uint16_t isensSolarEn:1;
		uint16_t isensLoadEn:1;
		uint16_t do0En:1;
		uint16_t do1En:1;
		uint16_t do2En:1;
		uint16_t do3En:1;
		uint16_t mk1PwrEn:1;
		uint16_t mk2PwrEn:1;
		uint16_t buck12vEn:1;
		uint16_t ldo1v8En:1;
		uint16_t uartExnEn:1;
		uint16_t uartExA0:1;
		uint16_t uartExA1:1;
		uint16_t diEn:1;
		uint16_t gpioEx1P14:1;
		uint16_t gpioEx1P15:1;
	};
}EXPANDER_GPIO_OUT;

extern volatile EXPANDER_GPIO_OUT gExpGpioOutVal;

extern void pcal6416GpioIntEnable(uint16_t irqMask);
extern void pcal6416GpioIntDisable(void);
extern uint16_t pcal6416GpioGetIntStatus(void);
extern uint16_t pcal6416GpioGetVal(uint8_t sla);
extern void pcal6416GpioCtrl(uint16_t val);
extern void pcal6416GpioSetVal(uint16_t gpioepin, bool val);
extern void initPCAL6416GpioExpander(uint16_t irqMask);
extern void pcal6416GpioSetIoMode(uint8_t channel, uint16_t configVal);
extern void initExpanderCtrlChannel(void);
#endif // __PCAL6416_Gpio_Expander_H__