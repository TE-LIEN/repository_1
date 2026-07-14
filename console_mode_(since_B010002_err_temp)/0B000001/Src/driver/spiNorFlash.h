#ifndef __SPI_NOR_FLASH_H__
#define __SPI_NOR_FLASH_H__

#include "sensminiCfg.h"
#include "driver/SENS_SPI.h"

#define DISABLE_WRITE_PDMA	1
#define DISABLE_READ_PDMA	0

#if (defined SUPPORT_FOTA && (SUPPORT_FOTA == 1)) || (TEST_SPI) || (defined SPI_FILE_SYSTEM)

#if defined(SENSMINIA4_PRO) || defined(SENSMINIA4_NEO)
#define SPI_FLASH_CHANNEL		0
#define SPI_FLASH_USE_QSPI		0
#define QSPI_READ_USE_QUAD		0

#if SPI_FLASH_USE_QSPI
#define SENS_SPI_FLASH_CLK_SEL	CLK_CLKSEL2_QSPI0SEL_PCLK0	//CLK_CLKSEL2_QSPI0SEL_PLL_DIV2
#define SENS_FLASH_SPI_CTX		QSPI0
#else
#define SENS_SPI_FLASH_CLK_SEL	CLK_CLKSEL2_SPI0SEL_PLL_DIV2
#define SENS_FLASH_SPI_CTX		SPI0
#endif
#endif

#ifdef SENSMINIS2
#define SPI_FLASH_CHANNEL		1
#define SPI_FLASH_USE_QSPI		0
#define SENS_SPI_FLASH_CLK_SEL	CLK_CLKSEL2_SPI1SEL_PLL_DIV2
#define SENS_FLASH_SPI_CTX		SPI1
#endif

#define SPI_CAPACITY_512Kb		0x05	//65536, 			0x00010000				
#define SPI_CAPACITY_1Mb		0x10	//131072,  		0x00020000
#define SPI_CAPACITY_2Mb		0x11	//262144,  		0x00040000
#define SPI_CAPACITY_4Mb		0x12	//524288,  		0x00080000
#define SPI_CAPACITY_8Mb		0x13	//1048576, 		0x00100000
#define SPI_CAPACITY_16Mb		0x14	//2097152, 		0x00200000 
#define SPI_CAPACITY_32Mb		0x15	//4194304, 		0x00400000
#define SPI_CAPACITY_64Mb		0x16	//8388608, 		0x00800000
#define SPI_CAPACITY_128Mb		0x17	//16777216, 	0x01000000
#define SPI_CAPACITY_256Mb		0x18	//33554432, 	0x02000000
#define SPI_CAPACITY_512Mb		0x19	//67108864, 	0x04000000
#define SPI_CAPACITY_1Gb		0x20	//134217728, 	0x08000000
#define SPI_CAPACITY_2Gb		0x21	//268435456, 	0x10000000

#define SPI_NOR_FLASH_SECTOR_SIZE	4096
#define SPI_NOR_FLASH_PAGE_SIZE		256

#define SPI_SECTOR_SIZE				SPI_NOR_FLASH_SECTOR_SIZE

#define NOR_FLASH_CMD_WRITE_ENABLE						0x06
#define NOR_FLASH_CMD_VOLATILR_SR_WRITE_ENABLE			0x50
#define NOR_FLASH_CMD_WRITE_DISABLE						0x04
#define NOR_FLASH_CMD_READ_STATUS_REG_1					0x05
#define NOR_FLASH_CMD_WRITE_STATUS_REG_1				0x01
#define NOR_FLASH_CMD_READ_STATUS_REG_2					0x35
#define NOR_FLASH_CMD_WRITE_STATUS_REG_2				0x31
#define NOR_FLASH_CMD_READ_STATUS_REG_3					0x15
#define NOR_FLASH_CMD_WRITE_STATUS_REG_3				0x11
#define NOR_FLASH_CMD_CHIP_ERASE						0xC7
#define NOR_FLASH_CMD_CHIP_ERASE_1						0x60
#define NOR_FLASH_CMD_EP_SUSPEND						0x75
#define NOR_FLASH_CMD_EP_RESUME							0x7A
#define NOR_FLASH_CMD_PWR_DOWN							0xB9
#define NOR_FLASH_CMD_RELEASE_PWR_DOWN					0xAB
#define NOR_FLASH_CMD_READ_MANUFACTURER_ID				0x90
#define NOR_FLASH_CMD_READ_JEDEC_ID						0x9F
#define NOR_FLASH_CMD_READ_UNIQUE_ID					0x4B
#define NOR_FLASH_CMD_PAGE_PROGRAM						0x02
#define NOR_FLASH_CMD_QUAD_PAGE_PROGRAM					0x32
#define NOR_FLASH_CMD_SECTOR_ERASE						0x20
#define NOR_FLASH_CMD_BLOCK_ERASE_32K					0x52
#define NOR_FLASH_CMD_BLOCK_ERASE_64K					0xD8
#define NOR_FLASH_CMD_READ_DATA							0x03
#define NOR_FLASH_CMD_FAST_READ							0x0B
#define NOR_FLASH_CMD_FAST_READ_QUAD					0xEB




#define SPI_FALSH_STS1_BSY_POS	0
#define SPI_FALSH_STS1_BSY_MASK	(1 << SPI_FALSH_STS1_BSY_POS)

#define SPI_FALSH_STS1_WEL_POS	1
#define SPI_FALSH_STS1_WEL_MASK	(1 << SPI_FALSH_STS1_WEL_POS)

#define SPI_FALSH_STS1_BP_POS	2
#define SPI_FALSH_STS1_BP_MASK	(1 << SPI_FALSH_STS1_BP_POS)

#define SPI_FALSH_STS1_TB_POS	5
#define SPI_FALSH_STS1_TB_MASK	(1 << SPI_FALSH_STS1_TB_POS)

#define SPI_FALSH_STS1_SEC_POS	6
#define SPI_FALSH_STS1_SEC_MASK	(1 << SPI_FALSH_STS1_SEC_POS)

#define SPI_FALSH_STS1_SRP0_POS	7
#define SPI_FALSH_STS1_SRP0_MASK	(1 << SPI_FALSH_STS1_SRP0_POS)

#define READ_WRITE_USE_PIO	0
#define READ_WRITE_USE_PDMA	1

#define READ_WRITE_NON_BLOCK	0
#define READ_WRITE_BLOCK			1


//extern volatile SPI_T *spiPort;

extern int openSpiFlash(uint32_t *mid);
extern unsigned int spiFlashReadMidDid(void);
extern void spiFlashWriteProtect(uint8_t protect);
extern void spiFlashSectorErase(uint32_t address);
extern uint8_t spiFlashReadStatusReg1(void);
extern int spiFlashPageProgram(uint32_t startAddress, uint8_t *buffer, uint32_t size, uint8_t usePdma, uint8_t blockXmit);
extern void spiFlashReadData(uint32_t startAddress, uint8_t *buffer, uint32_t size, uint8_t usePdma, uint8_t blockXmit);
//extern int spiFlashPdmaReadDataCheckDone(void);

extern uint32_t memoryReadData(uint32_t addr, uint32_t size, uint8_t *data);
extern void memorySetProtection(uint8_t protect);
extern void memorySectorErase(uint32_t addr);
extern uint32_t memoryWriteData(uint32_t addr, uint32_t size, uint8_t *data);

extern void memoryRWTest(void);
#endif

#endif

