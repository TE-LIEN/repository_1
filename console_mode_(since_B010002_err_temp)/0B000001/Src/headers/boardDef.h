#ifndef __BOARD_DEF_H__
#define __BOARD_DEF_H__


#define UART0_ID			0
#define UART1_ID			1
#define UART2_ID			2
#define UART3_ID			3
#define UART4_ID			4
#define UART5_ID			5
#define UART6_ID			6
#define UART7_ID			7
#define UART8_ID			8
#define UART9_ID			9
#define MAX_REAL_UART_ID	10

#define UART_VCOMM_CTRL		10
#define UART_VCOMM_AT		11
#define UART_VCOMM_PPP		12
#define UART_USB			13	//for USB Device CDC mode
#define UART_USB_VENDOR_AT	14
	#define MAX_UART_USB_VENDOR_AT_CNT	8
#define UART_USB_CDC_ECM	(UART_USB_VENDOR_AT + MAX_UART_USB_VENDOR_AT_CNT)
#define UART_MAX_ID			(UART_USB_CDC_ECM + 1)


#ifdef SENSMINIA4_PRO

/*
 * SYS PIN define
 */
#define PIN_SYS_PWR_EN		PG3
#define PIN_TEMP_ALERT		PB15
#define PIN_V_SOLAR_DET		PC10
#define PIN_V_BAT_DET		PA10
#define PIN_EEPROM_WP		PA9
#define PIN_CONFIG_IN1		PC12
#define PIN_CONFIG_IN2		PC11
#define PIN_MCU_DPD_EN		PF9
#define PIN_WAKEUP_NRST		PF8
#define PIN_WAKEUP_NINT		PF7
#define PIN_KEY0_IN			PF6
#define PIN_MB_LED3			PG5	//SYS LED
#define PIN_LED_WAN_LED4	PG6
#define PIN_INIT_SW			PG7

/*
 * PIC PIN define
 */
#define PIN_PIC_RXD			PH8
#define PIN_PIC_TXD			PH9
#define PIN_PIC_I2C_SCL		PB9
#define PIN_PIC_I2C_SDA		PB8

/*
 * I2C2 PIN define
 */
#define PIN_I2C_2_SCL		PD1
#define PIN_I2C_2_SDA		PD0

/*
 *	I2C define
 */
#define I2C0_ID		0
#define I2C1_ID		1
#define I2C2_ID		2
#define I2C0_IO_USED		0
#define I2C1_IO_USED		0
#define I2C2_IO_USED		1
/*
* SD PIN define
*/
#define PIN_SD_DAT2			PB4
#define PIN_SD_DAT3			PB5
#define PIN_SD_CMD			PB0
#define PIN_SD_CLK			PB1
#define PIN_SD_DAT0			PB2
#define PIN_SD_DAT1			PB3
#define PIN_SD_NCD			PB12
#define PIN_SD_PWR_EN		PC14

/*
* ETHERNET PIN define
*/
#define PIN_ETH_PWR_EN				PA8

#define PIN_ETH_EMAC0_PPS			PE13
#define PIN_ETH_RMII_CRSDV_RXDV		PA7
#define PIN_ETH_RMII_RXD0			PC7
#define PIN_ETH_RMII_RXD1			PC6
#define PIN_ETH_RMII_RXERR			PA6
#define PIN_ETH_RMII_TXEN			PE12
#define PIN_ETH_EMII_TXD0			PE10
#define PIN_ETH_RMII_TXD1			PE11
#define PIN_ETH_RMII_REFCLK			PC8
#define PIN_ETH_RMII_MDC			PE8
#define PIN_ETH_RMII_MDIO			PE9

#define PIN_ETH_DET_IN				PC9

#define PIN_ETH_XIN_25M				PD13

/*
* SPI NOR FLASH PIN define, QSPI0
*/
#define PIN_FLASH_QSPI_CS		PA3
#define PIN_FLASH_QSPI_MISO0	PA1
#define PIN_FLASH_QSPI_MISO1	PA5
#define PIN_FLASH_QSPI_MOSI0	PA0
#define PIN_FLASH_QSPI_CLK		PA2
#define PIN_FLASH_QSPI_MOSI1	PA4

/*
* SPI ADC PIN define, SPI1
*/
#define PIN_ADC_SPI_CLK		PH6
#define PIN_ADC_SPI_MISO	PH4
#define PIN_ADC_SPI_MOSI	PH5
#define PIN_ADC_SPI_CS		PH7
#define PIN_ADC_SPI_PWR_EN	PH0

/*
* RS232 PIN define, UART0
*/
#define PIN_RS232_EN	PB14
#define PIN_RS232_TXD	PD3
#define PIN_RS232_RXD	PD2

#if HBI_ENABLE == 1
#define CONSOLE_UART_CTX UART0
#else 
#define CONSOLE_UART_CTX UART1
#endif

/*
* RS485-1 PIN define
*/
#define PIN_RS485_1_PWR_EN		PC13
#define PIN_RS485_1_TXD			PD11	//UART1/UART8
#define PIN_RS485_1_RXD			PD10	//UART1/UART8
#define PIN_RS485_1_DE			PD12

/*
* RS485-2 PIN define
*/
#define PIN_RS485_2_PWR_EN		PE7
#define PIN_RS485_2_TXD			PH10	//UART0/UART4/UART9
#define PIN_RS485_2_RXD			PH11	//UART0/UART4/UART9
#define PIN_RS485_2_DE			PE6

/*
* HS_COMM PIN define
*/
#define PIN_HS_COMM_LOADSW_EN	PH3
#define PIN_HS_COMM_VIO_SEL		PG2	//input
#define PIN_HS_COMM_PWR_EN		PG4
#define PIN_HS_COMM_TXD			PE14	//UART2
#define PIN_HS_COMM_RXD			PE15	//UART2
#define PIN_HS_COMM_CTS			PD9
#define PIN_HS_COMM_RTS			PD8
#define PIN_HS_COMM_USB_EN		PE1

/*
* LS_COMM PIN define
*/
#define PIN_LS_COMM_RXD			PE3	//UART7
#define PIN_LS_COMM_TXD			PE2	//UART7
#define PIN_LS_COMM_USB_EN		PE0

/*
* HBI PIN define(Function pin)
*/
#define PIN_HBI_NRESET	PC2
#define PIN_HBI_NCS		PC3
#define PIN_HBI_CK		PC4
#define PIN_HBI_NCK		PC5
#define PIN_HBI_RWDS	PC1
#define PIN_HBI_DO		PG11
#define PIN_HBI_D1		PG12
#define PIN_HBI_D2		PC0
#define PIN_HBI_D3		PG10
#define PIN_HBI_D4		PG9
#define PIN_HBI_D5		PG13
#define PIN_HBI_D6		PG14
#define PIN_HBI_D7		PG15

/*
* DI PIN define
*/
#define PIN_DI_0		PF15
#define PIN_DI_1		PG0
#define PIN_DI_2		PE5
#define PIN_DI_3		PE4

/*
* DO PIN define
*/
#define PIN_DO_0		PF10
#define PIN_DO_1		PD14
#define PIN_DO_5V_EN	PG8

/*
* USB FULL SPEED Switch PIN define
*/
#define PIN_USB_FS_SEL	PH1
#define PIN_USB_FS_NOE	PH2

/*
* USB FULL SPEED PIN define
*/
#define PIN_USB_FS_VBUS			PA12
#define PIN_USB_FS_VBUS_EN		PB6
#define PIN_USB_FS_VBUS_ST		PB7
#define PIN_USB_FS_DM			PA13
#define PIN_USB_FS_DP			PA14
#define PIN_USB_FS_OTG_ID		PA15

/*
* USB HIGH SPEED PIN define
*/
//#define PIN_USB_HS_VBUS		//FIX PIN, NOT GPIO
#if HBI_ENABLE == 1
#define PIN_USB_HS_VBUS_EN		PB10
#define PIN_USB_HS_VBUS_ST		PB11
#else
#define PIN_USB_HS_VBUS_EN		PJ13
#define PIN_USB_HS_VBUS_ST		PJ12
#endif
//#define PIN_USB_HS_DM			//FIX PIN, NOT GPIO
//#define PIN_USB_HS_DP			//FIX PIN, NOT GPIO
//#define PIN_USB_HS_OTG_ID		//FIX PIN, NOT GPIO

/*
* DAC PIN define , not use
*/
#define PIN_DAC1_OUT			PB13
#define PIN_DAC1_ST				PA11

#elif defined SENSMINIA4_NEO	// for as_e18_v01_20251210.pdf

#define WAKEUP_USE_VBAT_DOMAIN	0

#define PSM_USE_WAKEUP_TIMER	1	// 0: use MCU DPD
#define WAKEUP_TIMER_USE_CARRIER_BOARD	1
#define UART_DEBUG_PCIE	0	//for debug, use UART1 to send from UART7 receive data

/*
 *	note: RS485-1 Level shift DE OE must use EN_RS485-2
 *	DI-0 & 1
 */
#define BOARD_VERSION	1	
/*
 * SYS PIN define
 */
#define PIN_EN_SYS_PWR			PH3
#define PIN_BUCK_3V3_EN			PB15
#define PIN_VSENSE_SOLAR		PA9
#define PIN_VSENSE_BAT			PA10
#define	PIN_ISENS_SOLAR			PC12
#define PIN_ISENS_LOAD			PC11
#define PIN_VSENSE_EN				PC9
#define	PIN_GPIO_DI0				PF11
#define PIN_GPIO_DI1				PF10
#define PIN_GPIOEX0_nINT		PF9
#define PIN_GPIOEX1_nINT		PF8
#define PIN_WAKEUP1_nRST		PF7
#define PIN_WAKEUP_nRST			PF6
#define	PIN_ISO_12V_EN			PG0
#define	PIN_SHUTDOWN_REQn		PG5
// #define	PIN_CONFIG_IN1			PG6		//Unused
#define	PIN_SOM_SLEEP				PG7

#define PIN_NO_EN_SYS_PWR			NU_PH3
#define PIN_NO_BUCK_3V3_EN			NU_PB15
#define PIN_NO_VSENSE_SOLAR			NU_PA9
#define PIN_NO_VSENSE_BAT			NU_PA10
#define	PIN_NO_ISENS_SOLAR			NU_PC12
#define PIN_NO_ISENS_LOAD			NU_PC11
#define PIN_NO_VSENSE_EN			NU_PC9
#define	PIN_NO_GPIO_DI0				NU_PF11
#define PIN_NO_GPIO_DI1				NU_PF10
#define PIN_NO_GPIOEX0_nINT			NU_PF9
#define PIN_NO_GPIOEX1_nINT			NU_PF8
#define PIN_NO_WAKEUP1_nRST			NU_PF7
#define PIN_NO_WAKEUP_nRST			NU_PF6
#define	PIN_NO_ISO_12V_EN			NU_PG0
#define	PIN_NO_SHUTDOWN_REQn		NU_PG5
// #define	PIN_NO_CONFIG_IN1		PG6		//Unused
#define	PIN_NO_SOM_SLEEP			NU_PG7


/*
 * PMU Ctrl PIC PIN define, UART1 And I2C0
 */
#define PIN_PIC_TXD				PH8
#define PIN_PIC_RXD				PH9
#define PIN_PIC_I2C_SCL			PB9
#define PIN_PIC_I2C_SDA			PB8

#define PIN_NO_PIC_TXD			NU_PH8
#define PIN_NO_PIC_RXD			NU_PH9
#define PIN_NO_PIC_I2C_SCL		NU_PB9
#define PIN_NO_PIC_I2C_SDA		NU_PB8

#define PMU_UART_NO				UART1_ID
//#define UART7_IO_IRQ_USED	1

#define UART1_USED			1	//UART_DEBUG_PCIE

/*
 * I/O expander PIN define, I2C1 
 */
#define PIN_I2C_1_SCL			PE1
#define PIN_I2C_1_SDA			PE0
#define	PIN_GPIOEX_I2C_EN		PE13

#define PIN_NO_I2C_1_SCL		NU_PE1
#define PIN_NO_I2C_1_SDA		NU_PE0
#define	PIN_NO_GPIOEX_I2C_EN	NU_PE13

/*
 * Temperature Sensor And EEPROM(256x8-bit) PIN define, I2C2
 */
#define PIN_I2C_2_SCL		PD1
#define PIN_I2C_2_SDA		PD0
#define PIN_BOARD_ID_WP		PC10

#define PIN_NO_I2C_2_SCL	NU_PD1
#define PIN_NO_I2C_2_SDA	NU_PD0
#define PIN_NO_BOARD_ID_WP	NU_PC10

/*
 *	I2C define
 */
#define I2C0_ID		0
#define I2C1_ID		1
#define I2C2_ID		2
#if WAKEUP_TIMER_USE_CARRIER_BOARD
#define I2C0_IO_USED		1
#define WAKEUP_TIME_I2C_ID	I2C0_ID
#else
#define I2C0_IO_USED		0
#define WAKEUP_TIME_I2C_ID	I2C2_ID
#endif
#define I2C1_IO_USED		1		// for PCAL6416AHF I/O expander
#define I2C2_IO_USED		1

#define EEPROM_I2C_ID		I2C2_ID

/*
* SD PIN define
*/
#define PIN_SD_DAT2			PB4
#define PIN_SD_DAT3			PB5
#define PIN_SD_CMD			PB0
#define PIN_SD_CLK			PB1
#define PIN_SD_DAT0			PB2
#define PIN_SD_DAT1			PB3
#define PIN_SD_NCD			PB12
#define PIN_SD_PWR_EN		PC14

#define PIN_NO_SD_DAT2			NU_PB4
#define PIN_NO_SD_DAT3			NU_PB5
#define PIN_NO_SD_CMD			NU_PB0
#define PIN_NO_SD_CLK			NU_PB1
#define PIN_NO_SD_DAT0			NU_PB2
#define PIN_NO_SD_DAT1			NU_PB3
#define PIN_NO_SD_NCD			NU_PB12
#define PIN_NO_SD_PWR_EN		NU_PC14

/*
* ETHERNET PIN define
*/
#define PIN_ETH_PWR_EN					PG1

#define PIN_ETH_EMAC0_PPS				PE13
#define PIN_ETH_RMII_CRSDV_RXDV		PA7
#define PIN_ETH_RMII_RXD0				PC7
#define PIN_ETH_RMII_RXD1				PC6
#define PIN_ETH_RMII_RXERR			PA6
#define PIN_ETH_RMII_TXEN				PE12
#define PIN_ETH_EMII_TXD0				PE10
#define PIN_ETH_RMII_TXD1				PE11
#define PIN_ETH_RMII_REFCLK			PC8
#define PIN_ETH_RMII_MDC				PE8
#define PIN_ETH_RMII_MDIO				PE9

#define PIN_NO_ETH_PWR_EN					NU_PG1
#define PIN_NO_ETH_EMAC0_PPS				NU_PE13
#define PIN_NO_ETH_RMII_CRSDV_RXDV			NU_PA7
#define PIN_NO_ETH_RMII_RXD0				NU_PC7
#define PIN_NO_ETH_RMII_RXD1				NU_PC6
#define PIN_NO_ETH_RMII_RXERR				NU_PA6
#define PIN_NO_ETH_RMII_TXEN				NU_PE12
#define PIN_NO_ETH_EMII_TXD0				NU_PE10
#define PIN_NO_ETH_RMII_TXD1				NU_PE11
#define PIN_NO_ETH_RMII_REFCLK				NU_PC8
#define PIN_NO_ETH_RMII_MDC					NU_PE8
#define PIN_NO_ETH_RMII_MDIO				NU_PE9

/*
* SPI NOR FLASH PIN define, QSPI0
*/
#define PIN_FLASH_QSPI_CS		PA3
#define PIN_FLASH_QSPI_MISO0	PA1
#define PIN_FLASH_QSPI_MISO1	PA5
#define PIN_FLASH_QSPI_MOSI0	PA0
#define PIN_FLASH_QSPI_CLK		PA2
#define PIN_FLASH_QSPI_MOSI1	PA4


#define PIN_NO_FLASH_QSPI_CS		NU_PA3
#define PIN_NO_FLASH_QSPI_MISO0		NU_PA1
#define PIN_NO_FLASH_QSPI_MISO1		NU_PA5
#define PIN_NO_FLASH_QSPI_MOSI0		NU_PA0
#define PIN_NO_FLASH_QSPI_CLK		NU_PA2
#define PIN_NO_FLASH_QSPI_MOSI1		NU_PA4

/*
* SPI NOR FLASH PIN define, SPI0
*/
#define PIN_FLASH_SPI_CS			PA3
#define PIN_FLASH_SPI_MISO		PA1
#define PIN_FLASH_SPI_MOSI		PA0
#define PIN_FLASH_SPI_CLK			PA2


#define PIN_NO_FLASH_SPI_CS			NU_PA3
#define PIN_NO_FLASH_SPI_MISO		NU_PA1
#define PIN_NO_FLASH_SPI_MOSI		NU_PA0
#define PIN_NO_FLASH_SPI_CLK		NU_PA2

/*
* ADC PIN define, SPI1
*/
#define PIN_ADC_SPI_CLK			PH6
#define PIN_ADC_SPI_MISO		PH4
#define PIN_ADC_SPI_MOSI		PH5
#define PIN_ADC_SPI_CS			PH7
#define PIN_ADC_SPI_PWR_EN		PH0
#define	PIN_ISO_ADC_3V3_EN		PD13

#define PIN_NO_ADC_SPI_CLK		NU_PH6
#define PIN_NO_ADC_SPI_MISO		NU_PH4
#define PIN_NO_ADC_SPI_MOSI		NU_PH5
#define PIN_NO_ADC_SPI_CS		NU_PH7
#define PIN_NO_ADC_SPI_PWR_EN	NU_PH0
#define	PIN_NO_ISO_ADC_3V3_EN	NU_PD13

/*
* MikroBUS PIN define, SPI2
*/
#define PIN_MK_SPI_CS			PG2
#define PIN_MK_SPI_CLK			PG3
#define PIN_MK_SPI_MISO			PG4
#define PIN_MK_SPI_MOSI			PA8

#define PIN_NO_MK_SPI_CS		NU_PG2
#define PIN_NO_MK_SPI_CLK		NU_PG3
#define PIN_NO_MK_SPI_MISO		NU_PG4
#define PIN_NO_MK_SPI_MOSI		NU_PA8

#define PIN_MK_SPI_CLK_FOR_DEBUG	PG3

/*
* RS232 PIN define, UART0 (CONSOLE UART)
*/
#define PIN_DO_5V_EN	PG8
#define PIN_RS232_EN	PB14
#define PIN_RS232_TXD	PD3
#define PIN_RS232_RXD	PD2

#define PIN_NO_DO_5V_EN		NU_PG8
#define PIN_NO_RS232_EN		NU_PB14
#define PIN_NO_RS232_TXD	NU_PD3
#define PIN_NO_RS232_RXD	NU_PD2

//#define RS232_UART_NO	UART0_ID
//#define UART0_USED	1


#if HBI_ENABLE == 1
#define CONSOLE_UART_CTX UART0
#else 
#define CONSOLE_UART_CTX UART1
#endif

/*
* IOT1 Wireless Module PIN define, UART7
*/
#define PIN_IOT_TXD		PE3
#define PIN_IOT_RXD		PE2
#define	PIN_IOT_CTS		PE4
#define	PIN_IOT_RTS		PE5

#define PIN_NO_IOT_TXD		NU_PE3
#define PIN_NO_IOT_RXD		NU_PE2
#define	PIN_NO_IOT_CTS		NU_PE4
#define	PIN_NO_IOT_RTS		NU_PE5

#define IOT_UART_NO			UART7_ID
#define UART7_IO_IRQ_USED	1
#define UART7_USED			1

#define PIN_IOT1_VBATT_EN	PE14
#define PIN_IOT1_VIO_SEL	PD8		//PE15
#define PIN_IOT2_VBATT_EN	PD9
#define PIN_IOT2_VIO_SEL	PE15	//PD8

#define	PIN_VBATT_IOT_EN	PD14

#define PIN_NO_IOT1_VBATT_EN	NU_PE14
#define PIN_NO_IOT1_VIO_SEL		NU_PE15
#define PIN_NO_IOT2_VBATT_EN	NU_PD9
#define PIN_NO_IOT2_VIO_SEL		NU_PD8
#define	PIN_NO_VBATT_IOT_EN		NU_PD14

/*
* RS485-1 PIN define, for UART1 or UART8
*/
#define PIN_RS485_1_TXD			PD11
#define PIN_RS485_1_RXD			PD10
#define PIN_RS485_1_PWR_EN		PC13
#define PIN_RS485_1_DE			PD12

#define PIN_NO_RS485_1_TXD			NU_PD11
#define PIN_NO_RS485_1_RXD			NU_PD10
#define PIN_NO_RS485_1_PWR_EN		NU_PC13
#define PIN_NO_RS485_1_DE			NU_PD12


#define RS485_1_UART_NO			UART8_ID
#define UART8_IO_IRQ_USED		1
#define UART8_USED	1

/*
* RS485-2 PIN define, for UART0 or UART4 or UART9
*/
#define PIN_RS485_2_TXD			PH10
#define PIN_RS485_2_RXD			PH11
#define PIN_RS485_2_PWR_EN		PE7
#define PIN_RS485_2_DE			PE6

#define PIN_NO_RS485_2_TXD			NU_PH10
#define PIN_NO_RS485_2_RXD			NU_PH11
#define PIN_NO_RS485_2_PWR_EN		NU_PE7
#define PIN_NO_RS485_2_DE			NU_PE6

#define RS485_2_UART_NO			UART4_ID
#define UART4_IO_IRQ_USED		1
#define UART4_USED	1

/*
* HS_COMM PIN define
*/
// #define PIN_HS_COMM_LOADSW_EN	PH3
// #define PIN_HS_COMM_VIO_SEL		PG2	//input
// #define PIN_HS_COMM_PWR_EN		PG4
// #define PIN_HS_COMM_TXD			PE14	//UART2
// #define PIN_HS_COMM_RXD			PE15	//UART2
// #define PIN_HS_COMM_CTS			PD9
// #define PIN_HS_COMM_RTS			PD8
// #define PIN_HS_COMM_USB_EN		PE1

/*
* LS_COMM PIN define
*/
// #define PIN_LS_COMM_RXD			PE3	//UART7
// #define PIN_LS_COMM_TXD			PE2	//UART7
// #define PIN_LS_COMM_USB_EN		PE0

/*
* HBI PIN define(Function pin)
*/
#define PIN_HBI_NRESET	PC2
#define PIN_HBI_NCS		PC3
#define PIN_HBI_CK		PC4
#define PIN_HBI_NCK		PC5
#define PIN_HBI_RWDS	PC1
#define PIN_HBI_DO		PG11
#define PIN_HBI_D1		PG12
#define PIN_HBI_D2		PC0
#define PIN_HBI_D3		PG10
#define PIN_HBI_D4		PG9
#define PIN_HBI_D5		PG13
#define PIN_HBI_D6		PG14
#define PIN_HBI_D7		PG15

#define PIN_NO_HBI_NRESET	NU_PC2
#define PIN_NO_HBI_NCS		NU_PC3
#define PIN_NO_HBI_CK		NU_PC4
#define PIN_NO_HBI_NCK		NU_PC5
#define PIN_NO_HBI_RWDS		NU_PC1
#define PIN_NO_HBI_DO		NU_PG11
#define PIN_NO_HBI_D1		NU_PG12
#define PIN_NO_HBI_D2		NU_PC0
#define PIN_NO_HBI_D3		NU_PG10
#define PIN_NO_HBI_D4		NU_PG9
#define PIN_NO_HBI_D5		NU_PG13
#define PIN_NO_HBI_D6		NU_PG14
#define PIN_NO_HBI_D7		NU_PG15

/*
* DI PIN define
*/
// #define PIN_DI_0		PF15
// #define PIN_DI_1		PG0
// #define PIN_DI_2		PE5
// #define PIN_DI_3		PE4

/*
* DO PIN define
*/
// #define PIN_DO_0		PF10
// #define PIN_DO_1		PD14
// #define PIN_DO_5V_EN	PG8

/*
* USB FULL SPEED Switch PIN define
*/
#define PIN_USB_FS_SEL	PH1
#define PIN_USB_FS_NOE	PH2

#define PIN_NO_USB_FS_SEL	NU_PH1
#define PIN_NO_USB_FS_NOE	NU_PH2

/*
* USB FULL SPEED PIN define
*/
#define PIN_USB_FS_VBUS			PA12
#define PIN_USB_FS_VBUS_EN		PB6
#define PIN_USB_FS_VBUS_ST		PB7
#define PIN_USB_FS_DM			PA13
#define PIN_USB_FS_DP			PA14
#define PIN_USB_FS_OTG_ID		PA15

#define PIN_NO_USB_FS_VBUS			NU_PA12
#define PIN_NO_USB_FS_VBUS_EN		NU_PB6
#define PIN_NO_USB_FS_VBUS_ST		NU_PB7
#define PIN_NO_USB_FS_DM			NU_PA13
#define PIN_NO_USB_FS_DP			NU_PA14
#define PIN_NO_USB_FS_OTG_ID		NU_PA15

/*
* USB HIGH SPEED PIN define
*/
//#define PIN_USB_HS_VBUS		//FIX PIN, NOT GPIO
#if HBI_ENABLE == 1
#define PIN_USB_HS_VBUS_EN		PB10
#define PIN_USB_HS_VBUS_ST		PB11

#define PIN_NO_USB_HS_VBUS_EN		NU_PB10
#define PIN_NO_USB_HS_VBUS_ST		NU_PB11

#else
#define PIN_USB_HS_VBUS_EN		PJ13
#define PIN_USB_HS_VBUS_ST		PJ12
#endif
//#define PIN_USB_HS_DM			//FIX PIN, NOT GPIO
//#define PIN_USB_HS_DP			//FIX PIN, NOT GPIO
//#define PIN_USB_HS_OTG_ID		//FIX PIN, NOT GPIO

/*
* DAC PIN define , not use
*/
#define PIN_DAC1_OUT			PB13
#define PIN_DAC1_ST				PA11

#define PIN_NO_DAC1_OUT			NU_PB13
#define PIN_NO_DAC1_ST			NU_PA11


/* GPIO EXPANDER INPUT PINT DEFINITIONS */
#define GPIOEX_NO_DI2    		(EXT0_P0_0)
#define GPIOEX_NO_DI3    		(EXT0_P0_1)
#define GPIOEX_NO_DI4    		(EXT0_P0_2)
#define GPIOEX_NO_DI5    		(EXT0_P0_3)
#define GPIOEX_NO_DI6    		(EXT0_P0_4)
#define GPIOEX_NO_P6     		(EXT0_P0_6)
#define GPIOEX_NO_P7     		(EXT0_P0_7)
#define GPIOEX_NO_P8     		(EXT0_P1_0)
#define GPIOEX_NO_TEMP_ALERT    (EXT0_P1_1)
#define GPIOEX_NO_I2C0_IRQ      (EXT0_P1_2)
#define GPIOEX_NO_I2C1_IRQ      (EXT0_P1_3)
#define GPIOEX_NO_I2C2_IRQ      (EXT0_P1_4)
#define GPIOEX_NO_P13    		(EXT0_P1_5)
#define GPIOEX_NO_P14    		(EXT0_P1_6)
#define GPIOEX_NO_P15    		(EXT0_P1_7)

/* GPIO EXPANDER OUTPUT PINT DEFINITIONS */
#define GPIOEX1_NO_ISENS_SOLAR_EN   (EXT1_P0_0)
#define GPIOEX1_NO_ISENS_LOAD_EN    (EXT1_P0_1)
#define GPIOEX1_NO_DO0      		(EXT1_P0_2)
#define GPIOEX1_NO_DO1      		(EXT1_P0_3)
#define GPIOEX1_NO_DO2      		(EXT1_P0_4)
#define GPIOEX1_NO_DO3      		(EXT1_P0_5)
#define GPIOEX1_NO_DO4      		(EXT1_P0_6)
#define GPIOEX1_NO_DO5      		(EXT1_P0_7)
#define GPIOEX1_NO_BUCK_12V_EN      (EXT1_P1_0)
#define GPIOEX1_NO_LDO_1V8_EN       (EXT1_P1_1)
#define GPIOEX1_NO_UARTEX_nEN       (EXT1_P1_2)
#define GPIOEX1_NO_UARTEX_A0        (EXT1_P1_3)
#define GPIOEX1_NO_UARTEX_A1        (EXT1_P1_4)
#define GPIOEX1_NO_DI_EN            (EXT1_P1_5)
#define GPIOEX1_NO_P14      		(EXT1_P1_6)
#define GPIOEX1_NO_P15      		(EXT1_P1_7)


#ifndef UART0_IO_IRQ_USED
#define UART0_IO_IRQ_USED		0
#endif

#ifndef UART1_IO_IRQ_USED
#define UART1_IO_IRQ_USED		0
#endif

#ifndef UART2_IO_IRQ_USED
#define UART2_IO_IRQ_USED		0
#endif

#ifndef UART3_IO_IRQ_USED
#define UART3_IO_IRQ_USED		0
#endif

#ifndef UART4_IO_IRQ_USED
#define UART4_IO_IRQ_USED		0
#endif

#ifndef UART5_IO_IRQ_USED
#define UART5_IO_IRQ_USED		0
#endif

#ifndef UART6_IO_IRQ_USED
#define UART6_IO_IRQ_USED		0
#endif

#ifndef UART7_IO_IRQ_USED
#define UART7_IO_IRQ_USED		0
#endif

#ifndef UART8_IO_IRQ_USED
#define UART8_IO_IRQ_USED		0
#endif

#ifndef UART9_IO_IRQ_USED
#define UART9_IO_IRQ_USED		0
#endif

#ifndef UART0_USED
#define UART0_USED	0
#endif

#ifndef UART1_USED
#define UART1_USED	0
#endif

#ifndef UART2_USED
#define UART2_USED	0
#endif

#ifndef UART3_USED
#define UART3_USED	0
#endif

#ifndef UART4_USED
#define UART4_USED	0
#endif

#ifndef UART5_USED
#define UART5_USED	0
#endif

#ifndef UART6_USED
#define UART6_USED	0
#endif

#ifndef UART7_USED
#define UART7_USED	0
#endif

#ifndef UART8_USED
#define UART8_USED	0
#endif

#ifndef UART9_USED
#define UART9_USED	0
#endif



#endif	//endof #ifdef SENSMINIA4_NEO

#ifdef SENSMINIS2
#define SUPPORT_DI
#define UART_DEBUG_PCIE	0

#define RTC_XTAL_USE_EXT	//mark for use internal 32768Hz OSC

#define UART2_ID_EMI	6
#define UART3_ID_EMI	7
#define UART_VCOMM_CTRL		8
#define UART_VCOMM_AT			9
#define UART_VCOMM_PPP		10
#define UART_USB					11
#define UART_BLE					12	//SC UART 1
#define UART_USB_VENDOR_AT		32
	#define MAX_UART_USB_VENDOR_AT_CNT	8
#define UART_USB_CDC_ECM			40
#define UART_MAX_ID	41

#define I2C0_ID		0
#define I2C1_ID		1
#define I2C2_ID		2

#define EMI_USE_I2S		0
#define EMI_USE_UART	1
	#define EMI_USE_UART2	(0 & EMI_USE_UART)	//PE14 & PE15
	#define EMI_USE_UART3	(1 & EMI_USE_UART)	//PE0 & PE1
	#define EMI_USE_UART4	(1 & EMI_USE_UART)	//PA2 & PA3
#define EMI_USE_SPI		0
#define EMI_USE_CAN		0
	#define EMI_USE_CAN0	(0 & EMI_USE_CAN)
#define EMI_USE_I2C		0
	#define EMI_USE_I2C2	(0 & EMI_USE_I2C)
#define EMI_USE_PWM		0
#define EMI_USE_ADC		0

#define CONSOLE_UART_NO									UART4_ID
	#if CONSOLE_UART_NO == UART2_ID
		#define CONSOLE_UART_MODULE					UART2_MODULE
		#define CONSOLE_UART_CLK_SEL_PLL		CLK_CLKSEL2_UART2SEL_PLL
		#define CONSOLE_UART_CLK_DIV				CLK_CLKDIV4_UART2
		#define CONSOLE_UART_CTX						UART2
		#define CONSOLE_UART_RX_PIN					PE15
		#define CONSOLE_UART_TX_PIN					PE14
	#elif CONSOLE_UART_NO == UART4_ID
		#define CONSOLE_UART_MODULE					UART4_MODULE
		#define CONSOLE_UART_CLK_SEL_PLL		CLK_CLKSEL3_UART4SEL_PLL
		#define CONSOLE_UART_CLK_DIV				CLK_CLKDIV4_UART4
		#define CONSOLE_UART_CTX						UART4
		#define CONSOLE_UART_RX_PIN					PA2
		#define CONSOLE_UART_TX_PIN					PA3
	#endif

/* IO define*/
//power define
#define MCU_BUCKBOOST_DISABLE		PF6			
#define MCU_BUCKBOOST_PWM				PB8
#define CHARGE_SHDN							PA9
#define CHARGE_STAT1						PD11
#define CHARGE_STAT2						PD10
#define SENSOR_5V_EN						PD2
#define PCIE_PWR_EN							PC5
#define VIO_PCIE_CTRL						PD14
#define VOLT_SENSE_EN						PH4
#define SOLAR_I_OUT							PB7	//ADC.7
#define SOLAR_I_SENSE_EN				PH5
#define SW_RST									PD4

#define LED											PD4
	#define LED_ON								0
	#define LED_OFF								1
	
#define DI_0_PIN								PB6

#define I_SOLAR_SENSOR_EADC_CH	7
#define V_BAT_SENSOR_EADC_CH		14
#define V_SOLAR_SENSOR_EADC_CH	13

#define I_SOLAR_SENSOR_EADC_SAMPLE_NO	4
#define V_BAT_SENSOR_EADC_SAMPLE_NO		5
#define V_SOLAR_SENSOR_EADC_SAMPLE_NO	6

//SD 
#define SD_nCD									PB12
#define SD_EN										PC3

//SPI
#define SPI_WP									PC4

//PCI-e define																							L210			R410	ME310	LE910
#define PCIE_USB_ON							PC6		//PCIe PIN 1					USB_EN, 	NA,		NA,		USB EN
#define PCIE_nPWR_SW_ONOFF			PG15	//PCIe PIN 11					PWR_KEY, 	NA,		NA,		PWR KEY
#define PCIE_RSTN								PC7		//PCIe PIN 7					RESET_n, 	NA,		NA,		RESETn
#define PCIE_PERST							PD5		//PCIe PIN 22					NA, 			NA,		NA,		NA
#define PCIE_nDISABLE						PH11	//PCIe PIN 20					PWR EN, 	NA,		NA,		PWR_EN(NA)
#define PCIE_UART_NO						UART1_ID
#define PCIE_nCTS								PE11
#define PCIE_nRTS								PE12
#define PCIE_RXD								PC8
#define PCIE_TXD								PE13
	#define PCIE_nPWR_SW_ONOFF_ACTIVE		0
	#define PCIE_nPWR_SW_ONOFF_NORMAL		1
	#define PCIE_RSTN_ACTIVE						1
	#define PCIE_RSTN_NORMAL						0
	#define PCIE_nDISABLE_ACTIVE				0
	#define PCIE_nDISABLE_NORMAL				1

#define PCIE_PWR_CHIP_CTRL			1

#define PCIE_PWR_CTRL_I2C_ID		I2C1_ID

//temperature define
#define TEMP_ALERT							PB9
#define TEMP_I2C_ID							I2C1_ID

//RS485 define
#define RS485_PWR_EN						PA4
#define RS485_DIR								PA5
#define RS485_UART_NO						UART0_ID
#define RS485_RECEIVE_MODE			0
#define RS485_TRANSMIT_MODE			1


#define MODBUS_RTU_USE_LIB			1

//24G RADAR define
#define RADAR_WLM_PWR_EN				PD9
#define RADAR_WLM_TX						PE9
#define RADAR_WLM_RX						PE8
#define RADAR_WLM_UART_NO				UART2_ID

//10G RADAR define
#define RADAR_RTC_PWR_EN				PC2
#define RADAR_RTC_INT						PE10
#define RADAR_RTC_RST						PD8
#define RADAR_RTC_I2C_ID				I2C0_ID

//BLE USE SC-UART1
//#define BLE_STATE								PE10
#define BLE_UART_NO							UART_BLE
#define BLE_PWR_EN							PC2
#define BLE_RST									PD8
#define BLE_TX									PC0
#define BLE_RX									PC1

//HEXL-MaxSona define
#define SONA_ENABLE							0
#if defined BOARD_VERSION_V3
	#define SONA_PWR_EN							PD3
	#define SONA_START_STOP					PD1
#else
#define SONA_PWR_EN							PD0
#define SONA_START_STOP					PD3
#endif
//#if SONA_ENABLE
//UART_MAX_ID
#define SONA_UART_NO						UART3_ID


//CAMERA define
#define CAMERA_PWR_EN						PD0
#define CAMERA_USE_RS485				0
#define CAMERA_USE_UART					1
#define CAMERA_UART_NO					UART3_ID
#if SONA_UART_NO == CAMERA_UART_NO
	#define CAMERA_ENABLE						!SONA_ENABLE
#else
	#define CAMERA_ENABLE					1
#endif


//USB
#define USB_VBUS_PA_12					PA12

//EMI define
#define EMI_GPIO_P3							PB6		//ACMP1_O, EPWM1_CH5, EPWM1_BRAKE1, BPWM1+CH5, UART1_RXD, USCI1_DAT1, EADC0_CH6
#define EMI_GPIO_P5							PH8		//UART1_TXD / I2C2_SCL / I2C1_SMBAL / UART3_nRTS / SPI1_CLK / I2S0_DI / SC2_PWR / QSPI0_CLK
#define EMI_GPIO_P7							PE0		//UART4_nRTS / I2C1_SDA / UART3_RXD / SPI1_MOSI / I2S0_MCLK / SC2_CLK / QSPI0_MOSI0
#define EMI_GPIO_P9							PA3		//QSPI0_SS / SPI0_SS / SC0_PWR / UART4_TXD / UART1_TXD / I2C1_SCL / I2C0_SMBAL / LCD_SEG27 / BPWM0_CH3 / EPWM0_CH2 / QEI0_B / EPWM1_BRAKE1
#define EMI_GPIO_P11						PE14	//UART2_TXD / CAN0_TXD

#define EMI_GPIO_P6							PH10	//UART0_TXD / UART4_TXD / SPI1_I2SMCLK / I2S0_LRCK / SC2_nCD / QSPI0_MISO1
#define EMI_GPIO_P8							PH9		//UART1_RXD / I2C2_SDA / I2C1_SMBSUS / UART3_nCTS / SPI1_SS / I2S0_DO / SC2_RST / QSPI0_SS	
#define EMI_GPIO_P10						PE1		//UART2_RXD / CAN0_RXD
#define EMI_GPIO_P12						PA2		//QSPI0_CLK / SPI0_CLK / SC0_RST / UART4_RXD / UART1_RXD / I2C1_SDA / I2C0_SMBSUS / LCD_SEG26 / BPWM0_CH2 / EPWM0_CH3
#define EMI_GPIO_P14						PE15	//UART2_RXD / CAN0_RXD

/*
*		EMI SPI		0			1x		2		3		qspi
*		CS				PA3		PH9		X		X
*		CLK				PA2		PH8		X		X
*		MOSI			X			PE0		X		X
*		MISO			X			PE1		X		X
*/

/*
*		EMI I2S		0			1
*		BCLK			PE1
*		DI				PH8
*		DO				PH9
*		LRCLK			PH10
*		MCLK			PE0
*/


//DEBUG IO
#define USE_DEBUG_IO						0

#ifdef SUPPORT_DI
#if defined USE_DEBUG_IO && USE_DEBUG_IO
#undef USE_DEBUG_IO
#define USE_DEBUG_IO	0
#endif
#endif

#define EMI_GPIO_DEBUG					PB6

#define PWR_5V_SRC_RADAR				0x01
#define PWR_5V_SRC_SONA					0x02
#define PWR_5V_SRC_RS485				0x04

/*
* PIN DEFINE
*/
#define SET_UART0_IO()	SYS->GPA_MFPL = (SYS->GPA_MFPL & (~(UART0_RXD_PA0_Msk | UART0_TXD_PA1_Msk))) | UART0_RXD_PA0 | UART0_TXD_PA1
#define SET_UART1_IO()	SYS->GPC_MFPH = (SYS->GPC_MFPH & (~(UART1_RXD_PC8_Msk))) | UART1_RXD_PC8;	\
												SYS->GPE_MFPH = (SYS->GPE_MFPH & (~(UART1_TXD_PE13_Msk))) | UART1_TXD_PE13
#if EMI_USE_UART2
	#define SET_UART2_IO()	SYS->GPE_MFPH = (SYS->GPE_MFPH & (~(UART2_RXD_PE15_Msk | UART2_TXD_PE14_Msk))) | UART2_RXD_PE15 | UART2_TXD_PE14
#else
	#define SET_UART2_IO()	SYS->GPE_MFPH = (SYS->GPE_MFPH & (~(UART2_RXD_PE9_Msk | UART2_TXD_PE8_Msk))) | UART2_RXD_PE9 | UART2_TXD_PE8
#endif

#if SONA_ENABLE
#define SET_UART3_RX_IO()	SYS->GPD_MFPL = (SYS->GPD_MFPL & (~(UART3_RXD_PD0_Msk))) | UART3_RXD_PD0
#define SET_UART3_TX_IO()	SYS->GPD_MFPL = (SYS->GPD_MFPL & (~(UART3_TXD_PD1_Msk))) | UART3_TXD_PD1
#else
	#if SONA_UART_NO == CAMERA_UART_NO
		#define SET_UART3_RX_IO()	SYS->GPE_MFPL = (SYS->GPE_MFPL & (~(UART3_RXD_PE0_Msk))) | UART3_RXD_PE0
		#define SET_UART3_TX_IO()	SYS->GPE_MFPL = (SYS->GPE_MFPL & (~(UART3_TXD_PE1_Msk))) | UART3_TXD_PE1
	#else
		#define SET_UART3_RX_IO()	SYS->GPD_MFPL = (SYS->GPD_MFPL & (~(UART3_RXD_PD0_Msk))) | UART3_RXD_PD0
		#define SET_UART3_TX_IO()	SYS->GPD_MFPL = (SYS->GPD_MFPL & (~(UART3_TXD_PD1_Msk))) | UART3_TXD_PD1
	#endif
#endif
#define SET_UART3_IO()	SET_UART3_RX_IO();	\
												SET_UART3_TX_IO()	
#define SET_UART4_IO()	SYS->GPA_MFPL = (SYS->GPA_MFPL & (~(UART4_RXD_PA2_Msk | UART4_TXD_PA3_Msk))) | UART4_RXD_PA2 | UART4_TXD_PA3

#define UART0_IO_USED		((0x01 << CONSOLE_UART_NO) | (0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x01
#define UART1_IO_USED		((0x01 << CONSOLE_UART_NO) | (0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x02
#define UART2_IO_USED		((0x01 << CONSOLE_UART_NO) | (0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x04
#define UART3_IO_USED		((0x01 << CONSOLE_UART_NO) | (0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x08
#define UART4_IO_USED		((0x01 << CONSOLE_UART_NO) | (0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x10
//#define UART5_IO_USED		((0x01 << CONSOLE_UART_NO) | (0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO)) & 0x20

#define UART0_IO_IRQ_USED		((0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x01
#define UART1_IO_IRQ_USED		((0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x02
#define UART2_IO_IRQ_USED		((0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x04
#define UART3_IO_IRQ_USED		((0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x08
#define UART4_IO_IRQ_USED		((0x01 << PCIE_UART_NO) | (0x01 << RS485_UART_NO) | (0x01 << RADAR_WLM_UART_NO) | (0x01 << SONA_UART_NO) | (0x01 << CAMERA_UART_NO)) & 0x10

#if UART_DEBUG_PCIE
#if !UART3_IO_USED
	#undef UART3_IO_USED
	#define UART3_IO_USED	1
#endif
#if !UART3_IO_IRQ_USED
	#undef UART3_IO_IRQ_USED
	#define UART3_IO_IRQ_USED	1
#endif
#endif


#define I2C0_IO_USED		((0x01 << RADAR_RTC_I2C_ID) | (0x01 << TEMP_I2C_ID)) & 0x01
#define I2C1_IO_USED		((0x01 << RADAR_RTC_I2C_ID) | (0x01 << TEMP_I2C_ID)) & 0x02
#define I2C2_IO_USED		((0x01 << RADAR_RTC_I2C_ID) | (0x01 << TEMP_I2C_ID)) & 0x04

#define MAX_I2C_CH	(I2C0_IO_USED? 1:0) + (I2C1_IO_USED? 1:0) + (I2C2_IO_USED? 1:0)

#endif	//endof #ifdef SENSMINIS2

#endif