#include "driver/SENS_SPI.h"
#include "sensCommon.h"

//------------------------------------------------------------------------------
void D2D3_SwitchToNormalMode(void)
{	SYS_UnlockReg();
    SYS->GPA_MFP1 =  SYS->GPA_MFP1 & ~(SYS_GPA_MFP1_PA4MFP_Msk | SYS_GPA_MFP1_PA5MFP_Msk);
    GPIO_SetMode(PA, BIT4, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT5, GPIO_MODE_OUTPUT);
    PA4 = 1;
    PA5 = 1;
}
//------------------------------------------------------------------------------
void D2D3_SwitchToQuadMode(void)
{	SYS_UnlockReg();
    SET_QSPI0_MOSI1_PA4();
    SET_QSPI0_MISO1_PA5();
}
//------------------------------------------------------------------------------
__STATIC_INLINE void wait_SPI_IS_BUSY(SPI_T *spi)
{	uint32_t u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */

	while(SPI_IS_BUSY(spi))
	{	if(--u32TimeOutCnt == 0)
		{	printf("Wait for SPI time-out!\n");
			break;
		}
	}
}
//------------------------------------------------------------------------------
__STATIC_INLINE void wait_QSPI_IS_BUSY(QSPI_T *spi)
{	uint32_t u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */

	while(QSPI_IS_BUSY(spi))
	{	if(--u32TimeOutCnt == 0)
		{	//printf("Wait for SPI time-out!\n");
			break;
		}
	}
}
//------------------------------------------------------------------------------
void SENS_SPI_IS_BUSY(SPI_INSTANCE *spiInst)
{	if(spiInst->spiMode == SPI_MODE_SPI)
	{	wait_SPI_IS_BUSY(spiInst->spiCtx);
	}
	else
	{
		wait_QSPI_IS_BUSY(spiInst->qspiCtx);
	}
}
//------------------------------------------------------------------------------
void initPdma(SPI_INSTANCE *spiInst)
{	SPI_T *spiCtx = (SPI_T *)spiInst->spiCtx;
	PDMA_T *pdmaCtx = spiInst->pdmaCtx;
	QSPI_T	*qspiCtx = spiInst->qspiCtx;
	
	SYS_ResetModule(PDMA0_RST);
	
	PDMA_Open(pdmaCtx, (1 << SPI_TX_DMA_CH) | (1 << SPI_RX_DMA_CH));
	
	PDMA_SetTransferCnt(pdmaCtx, SPI_TX_DMA_CH, PDMA_WIDTH_8, 8);
	PDMA_SetTransferCnt(pdmaCtx, SPI_RX_DMA_CH, PDMA_WIDTH_8, 8);
	
	if(spiInst->spiMode == SPI_MODE_QSPI)
	{	PDMA_SetTransferAddr(pdmaCtx, SPI_TX_DMA_CH, 0, PDMA_SAR_INC, (uint32_t)&qspiCtx->TX, PDMA_DAR_FIX);
		PDMA_SetTransferAddr(pdmaCtx, SPI_RX_DMA_CH, (uint32_t)&qspiCtx->RX, PDMA_SAR_FIX, 0, PDMA_DAR_INC);
		PDMA_SetTransferMode(pdmaCtx, SPI_TX_DMA_CH, PDMA_QSPI0_TX, FALSE, 0);
		PDMA_SetTransferMode(pdmaCtx, SPI_RX_DMA_CH, PDMA_QSPI0_RX, FALSE, 0);
	}
	else
	{	PDMA_SetTransferAddr(pdmaCtx, SPI_TX_DMA_CH, 0, PDMA_SAR_INC, (uint32_t)&spiCtx->TX, PDMA_DAR_FIX);
		PDMA_SetTransferAddr(pdmaCtx, SPI_RX_DMA_CH, (uint32_t)&spiCtx->RX, PDMA_SAR_FIX, 0, PDMA_DAR_INC);
#if SPI_FLASH_CHANNEL == 0	//SensMini A4 Neo Use SPI0
		PDMA_SetTransferMode(pdmaCtx, SPI_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);
		PDMA_SetTransferMode(pdmaCtx, SPI_RX_DMA_CH, PDMA_SPI0_RX, FALSE, 0);
#elif SPI_FLASH_CHANNEL == 1	//SensMini S2 use SPI1
		PDMA_SetTransferMode(pdmaCtx, SPI_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);
		PDMA_SetTransferMode(pdmaCtx, SPI_RX_DMA_CH, PDMA_SPI1_RX, FALSE, 0);
#endif
	}
	
	PDMA_SetBurstType(pdmaCtx, SPI_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
	PDMA_SetBurstType(pdmaCtx, SPI_RX_DMA_CH, PDMA_REQ_SINGLE, 0);
	
	pdmaCtx->DSCT[SPI_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;
	pdmaCtx->DSCT[SPI_RX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;
}
//------------------------------------------------------------------------------
static int isCorrectDmaBuff(const void *buf, uint32_t bufSize)
{	uint32_t _buf = (uint32_t) buf;

	return (((_buf & 0x03) == 0) &&                                        /* Word-aligned buffer base address */
        (((_buf >> 28) == 0x2) && (bufSize <= (0x30000000 - _buf))));   /* 0x20000000-0x2FFFFFFF */
}
//------------------------------------------------------------------------------
void setSpiXmitWidth(SPI_INSTANCE *spiInst, int width)
{	SPI_T	*spiCtx = spiInst->spiCtx;
	QSPI_T	*qspiCtx = spiInst->qspiCtx;
	
	if(spiInst->spiMode == SPI_MODE_QSPI)
	{	if(width == 8)
			qspiCtx->CTL = (qspiCtx->CTL & ~QSPI_CTL_DWIDTH_Msk) | (8 << QSPI_CTL_DWIDTH_Pos);
		else if(width == 16)
			qspiCtx->CTL = (qspiCtx->CTL & ~QSPI_CTL_DWIDTH_Msk) | (16 << QSPI_CTL_DWIDTH_Pos);
		else if(width == 24)
			qspiCtx->CTL = (qspiCtx->CTL & ~QSPI_CTL_DWIDTH_Msk) | (24 << QSPI_CTL_DWIDTH_Pos);
		else 
			qspiCtx->CTL = (qspiCtx->CTL & ~QSPI_CTL_DWIDTH_Msk);
	}
	else
	{	if(width == 8)
			spiCtx->CTL = (spiCtx->CTL & ~SPI_CTL_DWIDTH_Msk) | (8 << SPI_CTL_DWIDTH_Pos);
		else if(width == 16)
			spiCtx->CTL = (spiCtx->CTL & ~SPI_CTL_DWIDTH_Msk) | (16 << SPI_CTL_DWIDTH_Pos);
		else if(width == 24)
			spiCtx->CTL = (spiCtx->CTL & ~SPI_CTL_DWIDTH_Msk) | (24 << SPI_CTL_DWIDTH_Pos);
		else
			spiCtx->CTL = (spiCtx->CTL & ~SPI_CTL_DWIDTH_Msk);
	}
}
//------------------------------------------------------------------------------
void setSpiSsLow(SPI_INSTANCE *spiInst)
{	SPI_T	*spiCtx = spiInst->spiCtx;
	QSPI_T	*qspiCtx = spiInst->qspiCtx;
	
	if(spiInst->spiMode == SPI_MODE_QSPI)
	{	QSPI_SET_SS_LOW(qspiCtx);
	}
	else
	{	SPI_SET_SS_LOW(spiCtx);
	}
}
//------------------------------------------------------------------------------
void setSpiSsHigh(SPI_INSTANCE *spiInst)
{	SPI_T	*spiCtx = spiInst->spiCtx;
	QSPI_T	*qspiCtx = spiInst->qspiCtx;
	
	if(spiInst->spiMode == SPI_MODE_QSPI)
	{	QSPI_SET_SS_HIGH(qspiCtx);
	}
	else
	{	SPI_SET_SS_HIGH(spiCtx);
	}
}
//------------------------------------------------------------------------------
void spiWriteTx(SPI_INSTANCE *spiInst, uint32_t outputData)
{	SPI_T	*spiCtx = spiInst->spiCtx;
	QSPI_T	*qspiCtx = spiInst->qspiCtx;
	
	if(spiInst->spiMode == SPI_MODE_QSPI)
	{	QSPI_WRITE_TX(qspiCtx, outputData);	//spiCtx->TX = sendData;
	}
	else
	{	SPI_WRITE_TX(spiCtx, outputData);	//spiCtx->TX = sendData;
	}
}
//------------------------------------------------------------------------------
uint32_t spiReadRx(SPI_INSTANCE *spiInst)
{	SPI_T	*spiCtx = spiInst->spiCtx;
	QSPI_T	*qspiCtx = spiInst->qspiCtx;
	uint32_t inputData;
	
	if(spiInst->spiMode == SPI_MODE_QSPI)
	{	inputData = QSPI_READ_RX(qspiCtx);	//spiCtx->TX = sendData;
	}
	else
	{	inputData = SPI_READ_RX(spiCtx);	//spiCtx->TX = sendData;
	}
	return inputData;
}
//------------------------------------------------------------------------------
static void spiCtrl(SPI_INSTANCE *spiInst, uint8_t cmd, uint32_t arg, uint8_t triggerMode, uint8_t mode, uint8_t *buf, uint32_t byteLength)
{	uint32_t sendData;
	//uint32_t *readData;
	uint32_t rwData;
	uint16_t remainLength;
	uint16_t xmitLength;
	uint8_t *dmaBuf = NULL;
	uint8_t useSrcBuf = 1;
	volatile uint32_t pdmaIntSts;
	uint32_t pdmaUsingCh;
	uint8_t dmaIsByteAlign = 1;
	SPI_T	*spiCtx = spiInst->spiCtx;
	PDMA_T 	*pdmaCtx = spiInst->pdmaCtx;
	QSPI_T	*qspiCtx = spiInst->qspiCtx;
	int xmitWidth;
	int isQspiFastQuadRead = 0;
	
	if((mode == SPI_READ_DATA_W_PDMA) || (mode == SPI_READ_DATA_W_PDMA_NON_BLOCK))
	{	if(!isCorrectDmaBuff(buf, byteLength))
		{	dmaBuf = (uint8_t *)spiInst->dmaBuf;
			useSrcBuf = 0;
		}
		else
			dmaBuf = buf;
		PDMA_SetTransferCnt(pdmaCtx, SPI_RX_DMA_CH, PDMA_WIDTH_8, byteLength);
		if(spiInst->spiMode == SPI_MODE_QSPI)
			PDMA_SetTransferAddr(pdmaCtx, SPI_RX_DMA_CH, (uint32_t)&qspiCtx->RX, PDMA_SAR_FIX, (uint32_t)dmaBuf, PDMA_DAR_INC);
		else
			PDMA_SetTransferAddr(pdmaCtx, SPI_RX_DMA_CH, (uint32_t)&spiCtx->RX, PDMA_SAR_FIX, (uint32_t)dmaBuf, PDMA_DAR_INC);
		//pdmaRxCh->CSR |= (PDMA_CSR_TRIG_EN_Msk | PDMA_CSR_PDMACEN_Msk);
	}
	else if(mode == SPI_WRITE_DATA_W_PDMA)
	{	if(!isCorrectDmaBuff(buf, byteLength))
		{	dmaBuf = (uint8_t *)spiInst->dmaBuf;
			useSrcBuf = 0;
			memcpy(dmaBuf, buf, byteLength);
		}
		else
			dmaBuf = buf;
		PDMA_SetTransferCnt(pdmaCtx, SPI_TX_DMA_CH, PDMA_WIDTH_8, byteLength);
		if(spiInst->spiMode == SPI_MODE_QSPI)
			PDMA_SetTransferAddr(pdmaCtx, SPI_TX_DMA_CH, (uint32_t)dmaBuf, PDMA_SAR_INC, (uint32_t)&qspiCtx->TX, PDMA_DAR_FIX);
		else
			PDMA_SetTransferAddr(pdmaCtx, SPI_TX_DMA_CH, (uint32_t)dmaBuf, PDMA_SAR_INC, (uint32_t)&spiCtx->TX, PDMA_DAR_FIX);
		//pdmaTxCh->CSR |= (PDMA_CSR_TRIG_EN_Msk | PDMA_CSR_PDMACEN_Msk);
	}
	
	if((spiInst->spiModuleType == SPI_MODULE_NOR_FLASH) && (cmd == 0xEB))
		isQspiFastQuadRead = 1;
	
	if(isQspiFastQuadRead)
	{	sendData = cmd;
		xmitWidth = 8;
	}
	else
	{	if(triggerMode == NOR_FLASH_CMD_ONLY)
		{	sendData = cmd;
			xmitWidth = 8;
		}
		else if(triggerMode == NOR_FLASH_CMD_ARG_8)
		{	sendData = (arg & 0xFF) | cmd << 8;
			xmitWidth = 16;
		}
		else if(triggerMode == NOR_FLASH_CMD_ARG_16)
		{	sendData = (arg & 0xFFFF) | cmd << 16;
			xmitWidth = 24;
		}
		else if(triggerMode == NOR_FLASH_CMD_ARG_24)
		{	sendData = (arg & 0xFFFFFF) | cmd << 24;
			xmitWidth = 32;
		}
	}
	setSpiXmitWidth(spiInst, xmitWidth);
	
	// /CS: active
	setSpiSsLow(spiInst);
	spiWriteTx(spiInst, sendData);
	
#if defined SENSMINIS2
	wait_SPI_IS_BUSY(spiCtx);
#elif defined SENSMINIA4_NEO
	//SPI_GET_TX_FIFO_FULL_FLAG(spiCtx);
	if(spiInst->spiMode == SPI_MODE_QSPI)
	{	if((mode != SPI_WRITE_DATA) && (mode != SPI_WRITE_DATA_W_PDMA))
			wait_QSPI_IS_BUSY(qspiCtx);
		else
			while((qspiCtx->STATUS & QSPI_STATUS_TXFULL_Msk) >> QSPI_STATUS_TXFULL_Pos);
	}
	else
	{	if((mode != SPI_WRITE_DATA) && (mode != SPI_WRITE_DATA_W_PDMA))
			wait_SPI_IS_BUSY(spiCtx);
		else
			while((spiCtx->STATUS & SPI_STATUS_TXFULL_Msk) >> SPI_STATUS_TXFULL_Pos);
	}
#endif
	if(isQspiFastQuadRead)
	{	D2D3_SwitchToQuadMode();
		QSPI_ENABLE_QUAD_OUTPUT_MODE(qspiCtx);
		
		QSPI_WRITE_TX(qspiCtx, (arg >> 16) & 0xFF);
		QSPI_WRITE_TX(qspiCtx, (arg >> 8)  & 0xFF);
		QSPI_WRITE_TX(qspiCtx, arg       	 & 0xFF);
		//dummy byte
		QSPI_WRITE_TX(qspiCtx, 0xF0);
		QSPI_WRITE_TX(qspiCtx, 0x00);
		QSPI_WRITE_TX(qspiCtx, 0x00);
		
		wait_QSPI_IS_BUSY(qspiCtx);
		QSPI_ENABLE_QUAD_INPUT_MODE(qspiCtx);
	}
	
	if(spiInst->spiMode == SPI_MODE_QSPI)
	{	QSPI_ClearRxFIFO(qspiCtx);
	}
	else
	{	SPII2S_CLR_RX_FIFO(spiCtx);
	}
	if(byteLength && (mode < SPI_READ_DATA_W_PDMA))
	{	//readData = (uint32_t *)buf;
		remainLength = byteLength;
		
		while(remainLength)
		{	if(isQspiFastQuadRead)
				xmitLength = 1;
			else
			{	xmitLength = 4;
				if(remainLength < 4)
					xmitLength = remainLength;
			}
			rwData = 0;
			if(xmitLength == 1)
			{	xmitWidth = 8;
				rwData = buf[0];
			}
			else if(xmitLength == 2)
			{	xmitWidth = 16;
				rwData = buf[0] << 8;
				rwData |= buf[1];
			}
			else if(xmitLength == 3)
			{	xmitWidth = 24;
				rwData = buf[0] << 16;
				rwData |= (buf[1] << 8);
				rwData |= buf[2];
			}
			else
			{	xmitWidth = 32;
				rwData = buf[0] << 24;
				rwData |= (buf[1] << 16);
				rwData |= (buf[2] << 8);
				rwData |= buf[3];
			}
			setSpiXmitWidth(spiInst, xmitWidth);
			if(mode == SPI_READ_DATA)
				spiWriteTx(spiInst, 0x00);		//SPI_WRITE_TX(spiCtx, 0x00);	//spiCtx->TX = 0x00;
			else
				spiWriteTx(spiInst, rwData);	//SPI_WRITE_TX(spiCtx, rwData);	//spiCtx->TX = rwData;
			
#if defined SENSMINIS2
			wait_SPI_IS_BUSY(spiCtx);
#elif defined SENSMINIA4_NEO
			if(mode != SPI_WRITE_DATA)
			{	if(spiInst->spiMode == SPI_MODE_QSPI)
				{	wait_QSPI_IS_BUSY(qspiCtx);
				}
				else
				{	wait_SPI_IS_BUSY(spiCtx);
				}
			}	//#define SPI_GET_TX_FIFO_FULL_FLAG(spi)   ( ((spi)->STATUS & SPI_STATUS_TXFULL_Msk) >> SPI_STATUS_TXFULL_Pos )
			else if(mode == SPI_WRITE_DATA)
			{	if(spiInst->spiMode == SPI_MODE_QSPI)
					while((qspiCtx->STATUS & QSPI_STATUS_TXFULL_Msk) >> QSPI_STATUS_TXFULL_Pos);
				else
					while((spiCtx->STATUS & SPI_STATUS_TXFULL_Msk) >> SPI_STATUS_TXFULL_Pos);
			}
#endif
			remainLength -= xmitLength;
			
			// /CS: de-active
#if defined SENSMINIS2
			SPI_SET_SS_HIGH(spiCtx);
#endif
			if(mode == SPI_READ_DATA)
			{	/*while(1)
				{	if(!SPI_GET_RX_FIFO_EMPTY_FLAG(spiCtx))
						break;
				}*/
				
				rwData = spiReadRx(spiInst);
				if(xmitLength == 1)
					buf[0] = rwData & 0xFF;
				else if(xmitLength == 2)
				{	buf[0] = (rwData >> 8) & 0xFF;
					buf[1] = rwData & 0xFF;
				}
				else if(xmitLength == 3)
				{	buf[0] = (rwData >> 16) & 0xFF;
					buf[1] = (rwData >> 8) & 0xFF;
					buf[2] = (rwData >> 0) & 0xFF;
				}
				else if(xmitLength == 4)
				{	buf[0] = (rwData >> 24) & 0xFF;
					buf[1] = (rwData >> 16) & 0xFF;
					buf[2] = (rwData >> 8) & 0xFF;
					buf[3] = (rwData >> 0) & 0xFF;
				}
			}
			
			buf += xmitLength;
		}
#if defined SENSMINIA4_NEO
		//wait_SPI_IS_BUSY(spiCtx);
		SENS_SPI_IS_BUSY(spiInst);
#endif

	}
	else if(mode >= SPI_READ_DATA_W_PDMA)	// use PDMA, mass data read/write
	{	if(spiInst->spiMode == SPI_MODE_QSPI)
			qspiCtx->CTL = (qspiCtx->CTL & ~QSPI_CTL_DWIDTH_Msk) | (0x08 << QSPI_CTL_DWIDTH_Pos);
		else
			spiCtx->CTL = (spiCtx->CTL & ~SPI_CTL_DWIDTH_Msk) | (0x08 << SPI_CTL_DWIDTH_Pos);
		
		if(mode == SPI_WRITE_DATA_W_PDMA /*|| (mode == SPI_WRITE_DATA_W_PDMA_NON_BLOCK)*/)
		{	// enable SPI PDMA
			if(spiInst->spiMode == SPI_MODE_QSPI)
				QSPI_TRIGGER_TX_PDMA(qspiCtx);
			else
				SPI_TRIGGER_TX_PDMA(spiCtx);
			pdmaUsingCh = SPI_TX_DMA_CH;
		}
		else
		{	// enable SPI PDMA
			if(spiInst->spiMode == SPI_MODE_QSPI)
				QSPI_TRIGGER_RX_PDMA(qspiCtx);
			else
				SPI_TRIGGER_RX_PDMA(spiCtx);
			pdmaUsingCh = SPI_RX_DMA_CH;
		}
		while(1)
		{	pdmaIntSts = PDMA_GET_INT_STATUS(pdmaCtx);
			if(pdmaIntSts & PDMA_INTSTS_TDIF_Msk)
			{	if((PDMA_GET_TD_STS(pdmaCtx) & (1 << pdmaUsingCh)) == (1 << pdmaUsingCh)) 
				{	//wait_SPI_IS_BUSY(spiCtx);
					SENS_SPI_IS_BUSY(spiInst);
#if USE_DEBUG_IO	
					EMI_GPIO_DEBUG = 1;
#endif
					PDMA_CLR_TD_FLAG(pdmaCtx, (1 << pdmaUsingCh));
					if(spiInst->spiMode == SPI_MODE_QSPI)
						QSPI_DISABLE_TX_RX_PDMA(qspiCtx);
					else
						SPI_DISABLE_TX_RX_PDMA(spiCtx);
					if(!useSrcBuf && (mode == SPI_READ_DATA_W_PDMA))
					{	memcpy(buf, dmaBuf, byteLength);
					}
					break;
				}
			}
			//SENS_TIME_DELAY(1);
		}
	}
	// /CS: de-active
	setSpiSsHigh(spiInst);
}
//------------------------------------------------------------------------------
void SENS_SPI_CTRL(SPI_INSTANCE *spiInst, uint8_t cmd, uint32_t arg, uint8_t triggerMode, uint8_t mode, uint8_t *buf, uint32_t byteLength)
{	//if(spiInst->spiMode == SPI_MODE_SPI)
	//{	
		spiCtrl(spiInst, cmd, arg, triggerMode, mode, buf, byteLength);
	//}
	//else
	//{	
	//}
}
//------------------------------------------------------------------------------
void SENS_SPI_OPEN(SPI_INSTANCE *spiInst)
{
	if(spiInst->spiMode == SPI_MODE_SPI)
	{	if(spiInst->spiOpClk == 0)
			SPI_Open(spiInst->spiCtx, SPI_MASTER, SPI_MODE_0, 8, 20000000);
		else
			SPI_Open(spiInst->spiCtx, SPI_MASTER, spiInst->spiOpMode, 8, spiInst->spiOpClk);
	}
	else
	{	QSPI_Open(spiInst->qspiCtx, QSPI_MASTER, QSPI_MODE_0, 8, 40000000);
	}
}
//------------------------------------------------------------------------------