#ifndef __SENS_SPI_H__
#define __SENS_SPI_H__

#include "sensminiCfg.h"


#define SPI_MODE_SPI			0
#define SPI_MODE_QSPI			1

#define SPI_MODULE_NORMAL		0
#define SPI_MODULE_NOR_FLASH	1

typedef struct _SPI_INSTANCE
{	SPI_T		*spiCtx;
	QSPI_T		*qspiCtx;
	PDMA_T 		*pdmaCtx;
	//uint8_t		dmaBuf[SPI_NOR_FLASH_SECTOR_SIZE];
	uint8_t		*dmaBuf;
	uint32_t	dmsBufSize;
	uint32_t	spiOpMode;	//mode0~3
	uint32_t	spiOpClk;
	uint8_t		spiMode;	//spi or qspi
	uint8_t		spiModuleType;
}SPI_INSTANCE;

#define SPI_TX_DMA_CH	0
#define SPI_RX_DMA_CH	1

#define NOR_FLASH_CMD_ONLY		0
#define NOR_FLASH_CMD_ARG_8		1
#define NOR_FLASH_CMD_ARG_16	2
#define NOR_FLASH_CMD_ARG_24	3

#define SPI_CMD_ONLY			0
#define SPI_CMD_ARG_8			1
#define SPI_CMD_ARG_16			2
#define SPI_CMD_ARG_24			3



#define SPI_NON_DATA		0
#define SPI_READ_DATA		1
#define SPI_WRITE_DATA		2
#define SPI_READ_DATA_W_PDMA	3
#define SPI_WRITE_DATA_W_PDMA	4
#define SPI_READ_DATA_W_PDMA_NON_BLOCK	5
//#define SPI_WRITE_DATA_W_PDMA_NON_BLOCK	6

extern void SENS_SPI_IS_BUSY(SPI_INSTANCE *spiInst);
extern void initPdma(SPI_INSTANCE *spiInst);
extern void SENS_SPI_CTRL(SPI_INSTANCE *spiInst, uint8_t cmd, uint32_t arg, uint8_t triggerMode, uint8_t mode, uint8_t *buf, uint32_t byteLength);

extern void SENS_SPI_OPEN(SPI_INSTANCE *spiInst);

extern void D2D3_SwitchToNormalMode(void);
extern void D2D3_SwitchToQuadMode(void);




#endif