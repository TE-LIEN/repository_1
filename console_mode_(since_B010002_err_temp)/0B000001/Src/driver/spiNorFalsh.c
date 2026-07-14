#include <stdio.h>

#include "driver/spiNorFlash.h"
#include "sensCommon.h"
#include "driver/SENS_SPI.h"

#if (defined SUPPORT_FOTA && (SUPPORT_FOTA == 1)) || TEST_SPI || (defined SPI_FILE_SYSTEM)

#ifndef SPI_FILE_SYSTEM
extern uint8_t gForceEraseResource;
#endif

#if SPI_FLASH_USE_QSPI
	#define SENS_SPI_FLASH_MODE		SPI_MODE_QSPI
	#if SPI_FLASH_CHANNEL == 0
	#define SENS_SPI_FLASH_MODULE		QSPI0_MODULE
	#elif SPI_FLASH_CHANNEL == 1
	#define SENS_SPI_FLASH_MODULE		QSPI1_MODULE
	#else
	#error "SPI Module must set to 0 or 1"
	#endif
	
#else	//endof SPI_FLASH_USE_QSPI
	//#define SENS_SPI_T				SPI_T
	//#define SENS_SPI_IS_BUSY		SPI_IS_BUSY
	//#define SENS_SPI_CLOSE			SPI_Close
	#define SENS_SPI_FLASH_MODE		SPI_MODE_SPI	
	#if SPI_FLASH_CHANNEL == 0
	#define SENS_SPI_FLASH_MODULE	SPI0_MODULE
	//#define 
	#elif SPI_FLASH_CHANNEL == 1
	#define SENS_SPI_FLASH_MODULE	SPI1_MODULE
	#define SENS_SPI_FLASH_CLK_SEL	CLK_CLKSEL2_SPI1SEL_PLL_DIV2
	#else
	#error "SPI Module must set to 0 or 1"
	#endif
#endif	//endof SPI_USE_SPI

SPI_INSTANCE spiInst = {0};//[DL]20260611:open to test spi_flash
//static SPI_INSTANCE spiInst = {0};

uint8_t spiFlashPdmaBuf[SPI_NOR_FLASH_SECTOR_SIZE];
//------------------------------------------------------------------------------
uint32_t spiFlashReadMidDid(void)
{	uint32_t au32DestinationData;
	uint8_t mid[2];
	uint8_t cnt=0;
	
#if SPI_FLASH_USE_QSPI
	if(!spiInst.qspiCtx)
		openSpiFlash(NULL);
#else
	if(!spiInst.spiCtx)
		openSpiFlash(NULL);
#endif
	//SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, NOR_FLASH_CMD_READ_MANUFACTURER_ID, 0x00, NOR_FLASH_CMD_ARG_24, SPI_READ_DATA, (uint8_t *)mid, 3);
	SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, NOR_FLASH_CMD_READ_MANUFACTURER_ID, 0x00, NOR_FLASH_CMD_ARG_24, SPI_READ_DATA, (uint8_t *)mid, 2);

	au32DestinationData = mid[0] << 8;
	au32DestinationData |= mid[1];
	
	return au32DestinationData;
}
//------------------------------------------------------------------------------
static int spiFlashSetWriteLatch(uint8_t enable)
{	
#if SPI_FLASH_USE_QSPI
	if(!spiInst.qspiCtx)
		openSpiFlash(NULL);
#else
	if(spiInst.spiCtx == NULL)
		openSpiFlash(NULL);
#endif
	SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, (enable)? NOR_FLASH_CMD_WRITE_ENABLE : NOR_FLASH_CMD_WRITE_DISABLE, 0x00, NOR_FLASH_CMD_ONLY, SPI_NON_DATA, NULL, 0);
	return 0;
}
//------------------------------------------------------------------------------
uint8_t spiFlashReadStatusReg1(void)
{	uint8_t status;
	
#if SPI_FLASH_USE_QSPI
	if(!spiInst.qspiCtx)
		openSpiFlash(NULL);
#else
	if(spiInst.spiCtx == NULL)
		openSpiFlash(NULL);
#endif
	SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, NOR_FLASH_CMD_READ_STATUS_REG_1, 0x00, NOR_FLASH_CMD_ONLY, SPI_READ_DATA, &status, 1);
	return status;
}
//------------------------------------------------------------------------------
int8_t spiFlashWaitReady(void)
{	uint8_t status;
	
	while(1)
	{	status = spiFlashReadStatusReg1();
		if(!(status & SPI_FALSH_STS1_BSY_MASK))
			break;
	}
	
	return 0;
}
//------------------------------------------------------------------------------
int spiFlashWriteStatusReg1(uint8_t newStatus)
{	uint8_t status;
	
	//spiFlashReadStatusReg1();
	spiFlashSetWriteLatch(1);
	status = spiFlashReadStatusReg1();
	if(status & SPI_FALSH_STS1_WEL_MASK)
	{	//SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, NOR_FLASH_CMD_WRITE_STATUS_REG_1, 0, NOR_FLASH_CMD_ONLY, SPI_WRITE_DATA, &newStatus, 1);
		SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, NOR_FLASH_CMD_WRITE_STATUS_REG_1, newStatus, NOR_FLASH_CMD_ARG_8, SPI_NON_DATA, NULL, 0);
		spiFlashSetWriteLatch(0);
	}	
	else
		return -1;
	return 0;
}
//------------------------------------------------------------------------------
void spiFlashWriteProtect(uint8_t protect)
{	spiFlashWriteStatusReg1(protect);
}
//------------------------------------------------------------------------------
void spiFlashSectorErase(uint32_t address)
{	uint8_t status;
#if SPI_FLASH_USE_QSPI
	if(!spiInst.qspiCtx)
		openSpiFlash(NULL);
#else
	if(spiInst.spiCtx == NULL)
		openSpiFlash(NULL);
#endif
	address &= ~(SPI_NOR_FLASH_SECTOR_SIZE-1);
	
	spiFlashSetWriteLatch(1);
	//spiFlashReadStatusReg1();
	spiFlashWaitReady();
	SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, NOR_FLASH_CMD_SECTOR_ERASE, address, NOR_FLASH_CMD_ARG_24, SPI_NON_DATA, NULL, 0);
	
	while(1)
	{	status = spiFlashReadStatusReg1();
		if(!(status & 0x01))
			break;
	}
}
//------------------------------------------------------------------------------
uint8_t spiFlashReadStatusReg2(void)
{	uint8_t status;
	
#if SPI_FLASH_USE_QSPI
	if(!spiInst.qspiCtx)
		openSpiFlash(NULL);
#else
	if(spiInst.spiCtx == NULL)
		openSpiFlash(NULL);
#endif
	SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, NOR_FLASH_CMD_READ_STATUS_REG_2, 0x00, NOR_FLASH_CMD_ONLY, SPI_READ_DATA, &status, 1);
	return status;
}
//------------------------------------------------------------------------------
int spiFlashPageProgram(uint32_t startAddress, uint8_t *buffer, uint32_t size, uint8_t usePdma, uint8_t blockXmit)
{	uint32_t addr = startAddress;
	uint32_t count;
	uint32_t xmitLength;
	uint8_t status;
	uint8_t xmitMode;
	uint32_t alignXmitLength;
	
	if(usePdma == READ_WRITE_USE_PIO)
	{	xmitMode = SPI_WRITE_DATA;
	}
	else	//write no non block
	{	initPdma((SPI_INSTANCE *)&spiInst);
		//if(blockXmit == READ_WRITE_BLOCK)
			xmitMode = SPI_WRITE_DATA_W_PDMA;
		//else
		//	xmitMode = SPI_WRITE_DATA_W_PDMA_NON_BLOCK;
	}
	
	count = size;
	while(count>0)
	{	spiFlashSetWriteLatch(1);
		status = spiFlashReadStatusReg1();
		if(status & SPI_FALSH_STS1_WEL_MASK)
		{	xmitLength = count;
			if(xmitLength > SPI_NOR_FLASH_PAGE_SIZE - (addr & (SPI_NOR_FLASH_PAGE_SIZE - 1))) 
				xmitLength = SPI_NOR_FLASH_PAGE_SIZE - (addr & (SPI_NOR_FLASH_PAGE_SIZE - 1));
			
			if(((uint32_t)buffer & 0x03) && (xmitMode == SPI_WRITE_DATA_W_PDMA))
			{	alignXmitLength = ((uint32_t)buffer & 0x03);
				if(alignXmitLength > xmitLength)
				{ alignXmitLength = xmitLength;
				}
				xmitLength = alignXmitLength;
				SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst,  NOR_FLASH_CMD_PAGE_PROGRAM, addr, NOR_FLASH_CMD_ARG_24, SPI_WRITE_DATA, buffer, xmitLength);
			}
			else
				SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst,  NOR_FLASH_CMD_PAGE_PROGRAM, addr, NOR_FLASH_CMD_ARG_24, xmitMode, buffer, xmitLength);
			
			addr += xmitLength;
			count -= xmitLength;
			buffer += xmitLength;
			//if((addr & (SPI_NOR_FLASH_PAGE_SIZE -1)) == 0x00)
				spiFlashWaitReady();
		}
	}
	if(usePdma != READ_WRITE_USE_PIO)
	{	PDMA_Close(spiInst.pdmaCtx);
	}
	
	return 0;
}
//------------------------------------------------------------------------------
int SpiFlashWriteStatusRegs(uint8_t reg1, uint8_t reg2)
{	uint8_t status;
	uint16_t newStatus;
	
	newStatus = ((uint16_t)reg2 << 8 ) | reg1;
	//spiFlashReadStatusReg1();
	spiFlashSetWriteLatch(1);
	status = spiFlashReadStatusReg1();
	if(status & SPI_FALSH_STS1_WEL_MASK)
	{	//SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, NOR_FLASH_CMD_WRITE_STATUS_REG_1, 0, NOR_FLASH_CMD_ONLY, SPI_WRITE_DATA, &newStatus, 1);
		SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst, NOR_FLASH_CMD_WRITE_STATUS_REG_1, newStatus, NOR_FLASH_CMD_ARG_16, SPI_NON_DATA, NULL, 0);
		spiFlashSetWriteLatch(0);
	}	
	else
		return -1;
	return 0;
}
//------------------------------------------------------------------------------
void spiFlashEnableQEBit(void)
{
    uint8_t u8Status1 = spiFlashReadStatusReg1();
    uint8_t u8Status2 = spiFlashReadStatusReg2();

    u8Status2 |= 0x2;
    SpiFlashWriteStatusRegs(u8Status1, u8Status2);
    spiFlashWaitReady();
}
//------------------------------------------------------------------------------
void spiFlashDisableQEBit(void)
{
    uint8_t u8Status1 = spiFlashReadStatusReg1();
    uint8_t u8Status2 = spiFlashReadStatusReg2();

    u8Status2 &= ~0x2;
    SpiFlashWriteStatusRegs(u8Status1, u8Status2);
    spiFlashWaitReady();
}
//------------------------------------------------------------------------------
void spiFlashReadData(uint32_t startAddress, uint8_t *buffer, uint32_t size, uint8_t usePdma, uint8_t blockXmit)
{	uint8_t xmitMode;
	
	if(usePdma == READ_WRITE_USE_PIO)
	{	xmitMode = SPI_READ_DATA;
	}
	else
	{	initPdma((SPI_INSTANCE *)&spiInst);
		if(blockXmit == READ_WRITE_BLOCK)
			xmitMode = SPI_READ_DATA_W_PDMA;
		else
			xmitMode = SPI_READ_DATA_W_PDMA_NON_BLOCK;
	}
	//dbgMsg("spi read addr:%X, size:%d\r\n", startAddress, size);
		
#if SPI_FLASH_USE_QSPI
	#if QSPI_READ_USE_QUAD
	spiFlashEnableQEBit();
	SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst,  NOR_FLASH_CMD_FAST_READ_QUAD, startAddress, NOR_FLASH_CMD_ARG_24, xmitMode, buffer, size);
	#else
	SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst,  NOR_FLASH_CMD_READ_DATA, startAddress, NOR_FLASH_CMD_ARG_24, xmitMode, buffer, size);
	#endif
#else
	SENS_SPI_CTRL((SPI_INSTANCE *)&spiInst,  NOR_FLASH_CMD_READ_DATA, startAddress, NOR_FLASH_CMD_ARG_24, xmitMode, buffer, size);
#endif
	
	if(usePdma != READ_WRITE_USE_PIO)
	{	PDMA_Close(spiInst.pdmaCtx);
	}
	
#if QSPI_READ_USE_QUAD
	QSPI_DISABLE_QUAD_MODE(spiInst.qspiCtx);
    D2D3_SwitchToNormalMode();
	
	spiFlashDisableQEBit();
#endif
	return;
}
//------------------------------------------------------------------------------
int openSpiFlash(uint32_t *mid)
{	uint32_t midTemp;
	
	SYS_UnlockReg();
	
	/* Enable SPI0 module clock */
	CLK_EnableModuleClock(SENS_SPI_FLASH_MODULE);
	/* Select SPI0 module clock source as PCLK1 */
	CLK_SetModuleClock(SENS_SPI_FLASH_MODULE, SENS_SPI_FLASH_CLK_SEL, MODULE_NoMsk);
	CLK_EnableModuleClock(PDMA0_MODULE);

#if SPI_FLASH_USE_QSPI
	if(spiInst.qspiCtx)
		return 0;
#else
	if(spiInst.spiCtx)
		return 0;
#endif
#if (defined SENSMINIA4_PRO || defined SENSMINIA4_NEO) && SPI_FLASH_USE_QSPI
	SET_QSPI0_MOSI0_PA0();
	SET_QSPI0_MISO0_PA1();
	SET_QSPI0_CLK_PA2();
	SET_QSPI0_SS_PA3();
	SET_QSPI0_MOSI1_PA4();
	SET_QSPI0_MISO1_PA5();
	PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
	//GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5, GPIO_SLEWCTL_NORMAL);
	GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5, GPIO_SLEWCTL_HIGH);
	//GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5, GPIO_SLEWCTL_FAST);
	
	D2D3_SwitchToNormalMode();
#else
	/* Setup SPI0 multi-function pins */
	SET_SPI0_MOSI_PA0();
	SET_SPI0_MISO_PA1();
	SET_SPI0_CLK_PA2();
	SET_SPI0_SS_PA3();
	PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
	/* Enable SPI0 I/O high slew rate */
	GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT2 | BIT3, GPIO_SLEWCTL_HIGH);
#endif

	spiInst.spiMode = SENS_SPI_FLASH_MODE;
	spiInst.spiModuleType = SPI_MODULE_NOR_FLASH;
#if SPI_FLASH_USE_QSPI
	spiInst.qspiCtx = SENS_FLASH_SPI_CTX;
#else
	spiInst.spiCtx = SENS_FLASH_SPI_CTX;
#endif
	spiInst.spiOpClk = 80000000;	//MAX 133MHz
	spiInst.spiOpMode = SPI_MODE_0;
	SENS_SPI_OPEN((SPI_INSTANCE *)&spiInst);

	if(spiInst.spiMode == SPI_MODE_SPI)
		SPI_DisableAutoSS(spiInst.spiCtx);
	else
		QSPI_DisableAutoSS(spiInst.qspiCtx);
	spiInst.pdmaCtx = PDMA0;
	//spiInst.dmaBuf = SENS_MEM_ZALLOC(SPI_NOR_FLASH_SECTOR_SIZE);
	spiInst.dmaBuf = spiFlashPdmaBuf;	//SENS_MEM_ZALLOC(SPI_NOR_FLASH_SECTOR_SIZE);

	midTemp = spiFlashReadMidDid();
	dbgMsg("spi MID:%X\r\n", midTemp);
	if(mid != NULL)
		*mid = midTemp;
	return 0;
}
//------------------------------------------------------------------------------
int closeSpiFlash(void)
{	if(spiInst.spiCtx == NULL)
		return 0;
	if(spiInst.spiMode == SPI_MODE_SPI)
		SPI_Close(spiInst.spiCtx);
	else
		QSPI_Close(spiInst.qspiCtx);
	CLK_DisableModuleClock(SENS_SPI_FLASH_MODULE);
	CLK_DisableModuleClock(PDMA0_MODULE);
	spiInst.spiCtx = NULL;
	spiInst.qspiCtx = NULL;
	spiInst.pdmaCtx = NULL;
	return 0;
}
//------------------------------------------------------------------------------
uint32_t memoryReadData(uint32_t addr, uint32_t size, uint8_t *data)
{	
#if DISABLE_READ_PDMA
	spiFlashReadData(addr, data, size, READ_WRITE_USE_PIO, READ_WRITE_BLOCK);
#else
	spiFlashReadData(addr, data, size, READ_WRITE_USE_PDMA, READ_WRITE_BLOCK);
#endif
	return 0;
}
//------------------------------------------------------------------------------
void memorySetProtection(uint8_t protect)
{	spiFlashWriteProtect(protect);
}

void memorySectorErase(uint32_t addr)
{	
#ifndef NUVOTON
#ifndef SPI_FILE_SYSTEM
	if(!gForceEraseResource && addr == RESOURCE_BASE_ADDRESS)
    return;
#endif
#endif
	spiFlashSectorErase(addr);
}
//------------------------------------------------------------------------------
uint32_t memoryWriteData(uint32_t addr, uint32_t size, uint8_t *data)
{	
#if DISABLE_WRITE_PDMA
	spiFlashPageProgram(addr, data, size, READ_WRITE_USE_PIO, READ_WRITE_BLOCK);
#else
	spiFlashPageProgram(addr, data, size, READ_WRITE_USE_PDMA, READ_WRITE_BLOCK);
#endif
	return 0;
}
//------------------------------------------------------------------------------

#endif

/**************************************************************************
 *********              End of File                              *********
 **************************************************************************/