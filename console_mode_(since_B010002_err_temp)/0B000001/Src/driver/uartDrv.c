#include "driver/uartDrv.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "sensDev.h"
//#include "network/cmux.h"
#include "uart.h"
#include "powerCtrl.h"
#include "network/networkProcessTask.h"
#include "peripheral/rs485.h"

#ifdef SUPPORT_USB_DEVICE
#include "vcomSerial.h"
#endif

#ifdef SUPPORT_BLE
#include "ble/bleService.h"
#endif

//#define UART_IRQ_RECV_MASS	
uint32_t recvDataLenInIrq=0;

//UART_T *uartInsts[] = {UART0, UART1, UART2, UART3, UART4, UART5};
#if defined UART_IRQ_RECV_MASS
uint8_t uartIsrData[16];
uint8_t uartIsrDataLen;
#endif

static inline uint32_t rbCount(UART_RX_CTX *rb, uint32_t head, uint32_t tail)
{	return (head - tail) & rb->mask;
}

static inline uint32_t rbSpace(UART_RX_CTX *rb, uint32_t head, uint32_t tail)
{	return rb->size - rbCount(rb, head, tail) - 1U;
}

static inline void uart0_Pinmux(void)
{	SET_UART0_RXD_PD2();
	SET_UART0_TXD_PD3();
}

static inline void uart1_Pinmux(void)
{	SET_UART1_RXD_PH9();
	SET_UART1_TXD_PH8();
}

static inline void uart4_Pinmux(void)
{	SET_UART4_RXD_PH11();
	SET_UART4_TXD_PH10();
	
}

static inline void uart7_Pinmux(void)	//IOT, Mobile
{	SET_UART7_RXD_PE2();
	SET_UART7_TXD_PE3();
	SET_UART7_nCTS_PE4();
	SET_UART7_nRTS_PE5();
}

static inline void uart8_Pinmux(void)
{	SET_UART8_RXD_PD10();
	SET_UART8_TXD_PD11();
}


static const UART_CLK_IO_DEF uartClkIoDef[MAX_UART_NO] = 
{	{	UART0_MODULE, 
		CLK_CLKSEL1_UART0SEL_HXT,	//CLK_CLKSEL1_UART0SEL_PLL_DIV2
		CLK_CLKDIV0_UART0(1),
		uart0_Pinmux,
		UART0_IRQn,
		UART0
	},
	{	UART1_MODULE, 
		CLK_CLKSEL1_UART1SEL_HXT,	//CLK_CLKSEL1_UART1SEL_PLL_DIV2
		CLK_CLKDIV0_UART1(1),
		uart1_Pinmux,
		UART1_IRQn,
		UART1
	},
	{	UART2_MODULE, 
		CLK_CLKSEL3_UART2SEL_HXT, 	//CLK_CLKSEL3_UART2SEL_PLL_DIV2
		CLK_CLKDIV4_UART2(1),
		NULL,
		UART2_IRQn,
		UART2
	},
	{	UART3_MODULE, 
		CLK_CLKSEL3_UART3SEL_HXT,	//CLK_CLKSEL3_UART3SEL_PLL_DIV2
		CLK_CLKDIV4_UART3(1),
		NULL,
		UART3_IRQn,
		UART3
	},
	{	UART4_MODULE, 
		CLK_CLKSEL3_UART4SEL_HXT,	//CLK_CLKSEL3_UART4SEL_PLL_DIV2
		CLK_CLKDIV4_UART4(1),
		uart4_Pinmux,
		UART4_IRQn,
		UART4
	},
	{	UART5_MODULE, 
		CLK_CLKSEL3_UART5SEL_HXT, 	//CLK_CLKSEL3_UART5SEL_PLL_DIV2
		CLK_CLKDIV4_UART5(1),
		NULL,
		UART5_IRQn,
		UART5
	},
	{	UART6_MODULE, 
		CLK_CLKSEL3_UART6SEL_HXT, 	//CLK_CLKSEL3_UART6SEL_PLL_DIV2
		CLK_CLKDIV4_UART6(1),
		NULL,
		UART6_IRQn,
		UART6
	},
	{	UART7_MODULE, 
		CLK_CLKSEL3_UART7SEL_PLL_DIV2,	//CLK_CLKSEL3_UART7SEL_HXT
		CLK_CLKDIV4_UART7(1),
		uart7_Pinmux,
		UART7_IRQn,
		UART7
	},
	{	UART8_MODULE, 
		CLK_CLKSEL2_UART8SEL_HXT, 	//CLK_CLKSEL2_UART8SEL_PLL_DIV2
		CLK_CLKDIV5_UART8(1),
		uart8_Pinmux,
		UART8_IRQn,
		UART8
	},
	{	UART9_MODULE, 
		CLK_CLKSEL2_UART9SEL_HXT, 	//CLK_CLKSEL2_UART9SEL_PLL_DIV2
		CLK_CLKDIV5_UART9(1),
		NULL,
		UART9_IRQn,
		UART9
	},
	
};
//------------------------------------------------------------------------------
void setUartClk(uint8_t id, uint8_t on)
{	SYS_UnlockReg();
	
	if(id >= MAX_UART_NO)
	{	dbgMsg("enable UART CLK, unkonwn UART ID:%d\r\n", id);
		return;
	}
	
	if(on)
	{	CLK_EnableModuleClock(uartClkIoDef[id].module);
		CLK_SetModuleClock(uartClkIoDef[id].module, uartClkIoDef[id].clkSel, uartClkIoDef[id].clkDiv);
	}
	else
		CLK_DisableModuleClock(uartClkIoDef[id].module);
}
//------------------------------------------------------------------------------
void setUartIo(uint8_t id)
{	SYS_UnlockReg();
	
	if(id >= MAX_UART_NO)
	{	dbgMsg("set Uart IO, unknow UART ID:%d\r\n", id);
		return;
	}
	
	if(uartClkIoDef[id].pinMuxFn != NULL)
	{	uartClkIoDef[id].pinMuxFn();
	}
}
//------------------------------------------------------------------------------
void setUartIsr(uint8_t id, uint8_t on)
{	if(id >= MAX_UART_NO)
	{	dbgMsg("set Uart ISR, unknow UART ID:%d\r\n", id);
		return;
	}
	
	if(on)
	{	
#ifdef SENSMINIA4_NEO
		NVIC_SetPriority(uartClkIoDef[id].irqNo, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
#endif
		NVIC_EnableIRQ(uartClkIoDef[id].irqNo);
		UART_EnableInt(uartClkIoDef[id].uart, (UART_INTEN_RDAIEN_Msk));
	}
	else
	{	NVIC_DisableIRQ(uartClkIoDef[id].irqNo);
		UART_DisableInt(uartClkIoDef[id].uart, (UART_INTEN_RDAIEN_Msk));
	}
}
//------------------------------------------------------------------------------
void *getUartInst(uint8_t id)
{	if(id == UART0_ID)
	{	return (void *)UART0;
	}
	else if(id == UART1_ID)
	{	return (void *)UART1;
	}
	else if(id == UART2_ID)
	{	return (void *)UART2;
	}
	else if(id == UART3_ID)
	{	return (void *)UART3;
	}
	else if(id == UART4_ID)
	{	return (void *)UART4;
	}
	else if(id == UART5_ID)
	{	return (void *)UART5;
	}
	else if(id == UART6_ID)
	{	return (void *)UART6;
	}
	else if(id == UART7_ID)
	{	return (void *)UART7;
	}
	else if(id == UART8_ID)
	{	return (void *)UART8;
	}
	else if(id == UART9_ID)
	{	return (void *)UART9;
	}
	else
		return NULL;
}
//------------------------------------------------------------------------------
uint32_t uartRecv(UART_CTX *uartCtx, uint8_t *buffer, uint32_t length)
{	volatile int recvLength=0;
	volatile UART_RX_CTX *rxCtx = uartCtx->rxCtx;
	uint8_t id = uartCtx->id;
	uint32_t currLen;
	//uint8_t enterCritical = 0;

	if((rxCtx == NULL) || (length == 0))
		return 0;
	uint32_t head = rxCtx->head;
	uint32_t tail = rxCtx->tail;
	uint32_t canRead = (head - tail) & rxCtx->mask;
	if(canRead > length)	canRead = length;
	if(!canRead)	return 0;
	
	uint32_t toEnd = rxCtx->size - (tail & rxCtx->mask);
	uint32_t first = (canRead <= toEnd)? canRead : toEnd;
	uint32_t second = canRead - first;
	
	memcpy(buffer, &rxCtx->buffer[tail & rxCtx->mask], first);
	if(second) memcpy(buffer + first, &rxCtx->buffer[0], second);
	tail = (tail + canRead) & rxCtx->mask;
	__DMB();
	rxCtx->tail = tail;
	
	return canRead;
#if 0
	if(rxCtx == NULL)
		return 0;
	if(rxCtx->length == 0)
	{	return 0;
	}
	
	if((id < UART_VCOMM_CTRL) || (id == UART_USB) || (id >= UART_USB_VENDOR_AT))
		enterCritical = 1;
	currLen = rxCtx->length;
	for(recvLength=0;recvLength<length;)
	{	buffer[recvLength++] = *rxCtx->getPos;
		rxCtx->getPos++;
		if(rxCtx->getPos == rxCtx->endPos)
			rxCtx->getPos = rxCtx->startPos;
		currLen--;
		if(currLen == 0)
			break;
	}
	if(enterCritical)
		taskENTER_CRITICAL();
	rxCtx->length -= recvLength;
	if(enterCritical)
		taskEXIT_CRITICAL();
	return recvLength;
#endif
}
//------------------------------------------------------------------------------
void uartClearFifo(UART_CTX *uartCtx)
{	if((uartCtx == NULL) || (uartCtx->rxCtx == NULL))
		return;
	UART_RX_CTX *rxCtx = uartCtx->rxCtx;
	
	uint32_t head = rxCtx->head;
	rxCtx->tail = head;
	rxCtx->overrun = 0;
	rxCtx->dropInputCount = 0;
#if 0
	if(rxCtx)
	{	taskENTER_CRITICAL();
		rxCtx->length = 0;
		rxCtx->getPos = rxCtx->startPos;
		rxCtx->recvPos = rxCtx->startPos;
		recvDataLenInIrq = 0;
		taskEXIT_CRITICAL();
	}
#endif
}
//------------------------------------------------------------------------------
uint32_t getUartRecvLength(UART_CTX *uartCtx)
{	if((uartCtx == NULL) || (uartCtx->rxCtx == NULL))
		return 0;
	UART_RX_CTX *rxCtx = uartCtx->rxCtx;
	
	uint32_t head = rxCtx->head;
	uint32_t tail = rxCtx->tail;
	
	return RB_COUNT(rxCtx, head, tail);
#if 0
	if(uartCtx && uartCtx->rxCtx)
	{	return uartCtx->rxCtx->length;
	}
	else
		return 0;
#endif
}
//------------------------------------------------------------------------------
void uartFifoPut(uint8_t isIsr, uint8_t id, uint8_t *data, uint16_t length)
{	UART_CONFIG *uartCfg;
	UART_CTX *uartCtx;
	UART_RX_CTX *uartRxCtx;
	uint32_t head, tail;
	uint32_t space;
	
	uartCfg = sensDev->uartCfgs[id];
	if((uartCfg == NULL) || (uartCfg->ctx == NULL) || (uartCfg->ctx->rxCtx == NULL))
		return;
	
	uartRxCtx = uartCfg->ctx->rxCtx;
	head = uartRxCtx->head;
	tail = uartRxCtx->tail;
	
	space = RB_SPACE(uartRxCtx, head, tail);
	
	if(length > space)
	{	uartRxCtx->dropInputCount += (length - space);
		length = space;
		uartRxCtx->overrun++;
		if(length == 0)	return;
	}
	
	uint32_t first = uartRxCtx->size - RB_MASK(uartRxCtx, head);
	if(first > length)	first = length;
	uint32_t second = length - first;
	
	memcpy(&uartRxCtx->buffer[RB_MASK(uartRxCtx, head)], data, first);
	if(second) memcpy(&uartRxCtx->buffer[0], data+first, second);
	head = RB_MASK(uartRxCtx, head + length);
	__DMB();	//data memory barrier
	uartRxCtx->head = head;
	
#if 0
	for(int idx=0;idx<length;idx++)
	{	//if((uartRxCtx->recvPos == uartRxCtx->getPos) && uartRxCtx->length)	//fifo full
		if(uartRxCtx->length == uartRxCtx->bufSize)
		{	uartRxCtx->dropInputCount++;
		}
		else
		{	*uartRxCtx->recvPos = *data++;
			uartRxCtx->recvPos++;
			if(uartRxCtx->recvPos == uartRxCtx->endPos)
				uartRxCtx->recvPos = uartRxCtx->startPos;
			uartRxCtx->length++;
		}
	}
#endif
}
//------------------------------------------------------------------------------
int getUartReceiveSts(uint8_t id, void *uartInst)
{	return ((UART_T *)uartInst)->INTSTS & UART_INTSTS_RDAIF_Msk;
}
//------------------------------------------------------------------------------
uint32_t getUartRecvData(uint8_t id, void *uartInst)
{	return UART_READ((UART_T *)uartInst);
}
//------------------------------------------------------------------------------
void uartRecvHandlerIsr(uint8_t id, void *uartInst)
{	uint8_t uartCfgId = id;
#if !defined UART_IRQ_RECV_MASS
	uint32_t data;
#else
	volatile uint32_t delay;
	uint8_t *dataPtr;
	
	uartIsrDataLen = 0;
	dataPtr = uartIsrData;
#endif
#ifdef SUPPORT_BLE
	if(id == UART_BLE)
		uartCfgId = 6;
#endif
	
	//while(uartInst->INTSTS & UART_INTSTS_RDAIF_Msk)
	while(getUartReceiveSts(id, uartInst))
	{	
#if defined UART_IRQ_RECV_MASS
		*dataPtr++ = UART_READ(uartInst);
		uartIsrDataLen++;
		if(uartIsrDataLen >= 16)
		{	dataPtr = uartIsrData;
			uartFifoPut(1, id, (uint8_t *)dataPtr, uartIsrDataLen);
			uartIsrDataLen = 0;
		}
#else
		data = getUartRecvData(id, uartInst);
		uartFifoPut(1, uartCfgId, (uint8_t *)&data, 1);
#if UART_DEBUG_PCIE
		if(id == UART7_ID)
		{	/*while(UART_IS_TX_FULL(UART1));
			UART_Write(UART1 ,(uint8_t *)&data, 1);*/
			//if(networkCtx->mobileUartStartRecv)
				recvDataLenInIrq++;
			//UART_Write(UART1, (uint8_t *)&data, 1);
			//while(!UART_IS_TX_EMPTY(UART1)) __NOP();
			
		}
#endif
#endif
	}
#if defined UART_IRQ_RECV_MASS
	if(uartIsrDataLen)
	{	dataPtr = uartIsrData;
		uartFifoPut(1, id, (uint8_t *)dataPtr, uartIsrDataLen);
		uartIsrDataLen = 0;
	}
#endif
}
//------------------------------------------------------------------------------
#if UART0_IO_IRQ_USED
void UART0_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAIF_Msk))
	{	uartRecvHandlerIsr(UART0_ID, UART0);
		//while(UART_IS_RX_READY(UART0))
		//while(UART0->INTSTS & UART_INTSTS_RDAIF_Msk)
		//{	uartRecvHandlerIsr(UART0_ID);
		//}
	}
}
#endif
//------------------------------------------------------------------------------
#if UART1_IO_IRQ_USED
void UART1_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART1, UART_INTSTS_RDAIF_Msk))
	{	uartRecvHandlerIsr(UART1_ID, UART1);
	}
}
#endif
//------------------------------------------------------------------------------
#if UART2_IO_IRQ_USED
void UART2_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART2, UART_INTSTS_RDAIF_Msk))
	{	uartRecvHandlerIsr(UART2_ID, UART2);
	}
}
#endif
//------------------------------------------------------------------------------
#if UART3_IO_IRQ_USED
void UART3_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART3, UART_INTSTS_RDAIF_Msk))
	{	uartRecvHandlerIsr(UART3_ID, UART3);
	}

}
#endif
//------------------------------------------------------------------------------
#if UART4_IO_IRQ_USED	//RS485-2
void UART4_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART4, UART_INTSTS_RDAIF_Msk))
	{	while(UART_IS_RX_READY(UART4))
		{	uartRecvHandlerIsr(UART4_ID, UART4);
		}
	}
}
#endif
//------------------------------------------------------------------------------
#if UART5_IO_IRQ_USED
void UART5_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART5, UART_INTSTS_RDAIF_Msk))
	{	while(UART_IS_RX_READY(UART5))
		{	uartRecvHandlerIsr(UART5_ID, UART5);
		}
	}
}
#endif
//------------------------------------------------------------------------------
#if UART6_IO_IRQ_USED
void UART6_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART6, UART_INTSTS_RDAIF_Msk))
	{	while(UART_IS_RX_READY(UART6))
		{	uartRecvHandlerIsr(UART6_ID, UART6);
		}
	}
}
#endif
//------------------------------------------------------------------------------
#if UART7_IO_IRQ_USED
void UART7_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART7, UART_INTSTS_RDAIF_Msk))
	{	while(UART_IS_RX_READY(UART7))
		{	uartRecvHandlerIsr(UART7_ID, UART7);
		}
	}
}
#endif
//------------------------------------------------------------------------------
#if UART8_IO_IRQ_USED
void UART8_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART8, UART_INTSTS_RDAIF_Msk))
	{	while(UART_IS_RX_READY(UART8))
		{	uartRecvHandlerIsr(UART8_ID, UART8);
		}
	}
}
#endif
//------------------------------------------------------------------------------
#if UART9_IO_IRQ_USED
void UART9_IRQHandler(void)
{	if(UART_GET_INT_FLAG(UART9, UART_INTSTS_RDAIF_Msk))
	{	while(UART_IS_RX_READY(UART9))
		{	uartRecvHandlerIsr(UART9_ID, UART9);
		}
	}
}
#endif
//------------------------------------------------------------------------------
static inline uint32_t nextPow2(uint32_t n) 
{	if(n < 2) 
		return 2;
	n--;
	n |= n >> 1; 
	n |= n >> 2; 
	n |= n >> 4; 
	n |= n >> 8; 
	n |= n >> 16;
	return n + 1;
}
//------------------------------------------------------------------------------
static UART_RX_CTX* initUartRxBuf(uint32_t bufferSize)
{	UART_RX_CTX *uartRxCtx;
	
	uartRxCtx = SENS_MEM_ZALLOC(sizeof(UART_RX_CTX));
	if(uartRxCtx == NULL)
		return NULL;
	
	uartRxCtx->size = nextPow2(bufferSize);
	uartRxCtx->mask = uartRxCtx->size - 1U;
	uartRxCtx->buffer = SENS_MEM_ZALLOC(uartRxCtx->size);
	if(uartRxCtx->buffer == NULL)
	{	SENS_MEM_FREE(uartRxCtx);
		return NULL;
	}
	
	//uartRxCtx->head = uartRxCtx->tail = 0;
	//uartRxCtx->dropInputCount = uartRxCtx->overrun = 0;
	
	/*uartRxCtx->bufSize = bufferSize;
	uartRxCtx->buffer = SENS_MEM_ZALLOC(bufferSize);
	uartRxCtx->endPos = uartRxCtx->buffer + bufferSize;
	uartRxCtx->startPos = uartRxCtx->buffer;
	uartRxCtx->recvPos = uartRxCtx->buffer;
	uartRxCtx->getPos = uartRxCtx->buffer;
	uartRxCtx->dropInputCount = 0;
	uartRxCtx->overRunCount = 0;
	uartRxCtx->length = 0;
	*/
	return uartRxCtx;
}
//------------------------------------------------------------------------------
void setUartFlowControl(UART_CTX *uartCtx, uint8_t enable)
{	if(enable)
		UART_EnableFlowCtrl(uartCtx->inst);
	else
		UART_DisableFlowCtrl(uartCtx->inst);
}
//------------------------------------------------------------------------------
int uartInit( UART_CTX *uartCtx, uint8_t id, int baudrate, enum UART_MODE mode, uint32_t rxBufferSize, uint8_t uartType, uint8_t flowCtrl)
{	UART_RX_CTX *rxCtx;
	UART_T *uartReg;

	
	if(uartCtx->rxCtx == NULL)
		uartCtx->rxCtx = initUartRxBuf(rxBufferSize);
	else
	{	rxCtx = uartCtx->rxCtx;
		if(rxCtx->size != nextPow2(rxBufferSize))
		{	SENS_MEM_FREE(rxCtx->buffer);
			rxCtx->size = nextPow2(rxBufferSize);
			rxCtx->mask = rxCtx->size - 1U;
			rxCtx->buffer = SENS_MEM_ZALLOC(rxCtx->size);
			if(rxCtx->buffer == NULL)
			{	SENS_MEM_FREE(rxCtx);
				uartCtx->rxCtx = NULL;
			}
		}
		if(uartCtx->rxCtx)
		{	rxCtx->head = rxCtx->tail = 0;
			rxCtx->dropInputCount = rxCtx->overrun = 0;
		}
		/*if(rxCtx->bufSize != rxBufferSize)
		{	SENS_MEM_FREE(rxCtx->buffer);
			rxCtx->bufSize = rxBufferSize;
			rxCtx->buffer = SENS_MEM_ZALLOC(rxBufferSize);
		}
		
		rxCtx->endPos = rxCtx->buffer + rxBufferSize;
		rxCtx->startPos = rxCtx->buffer;
		rxCtx->recvPos = rxCtx->buffer;
		rxCtx->getPos = rxCtx->buffer;
		rxCtx->dropInputCount = 0;
		rxCtx->overRunCount = 0;
		rxCtx->length = 0;*/
	}
	uartCtx->rs485XmitMode = RS485_RECV_MODE;
	uartCtx->id = id;
	uartCtx->uartType = uartType;
	uartCtx->flowCtrl = flowCtrl;
	
	if(uartCtx->sema == NULL)
		SENS_SEM_INIT(uartCtx->sema, 1);

	if(uartCtx->uartType != UT_UART && uartCtx->uartType != UT_RS485
#ifdef SUPPORT_BLE
	&& uartCtx->uartType != UT_BLE
#endif
	)
		return 0;
	
	setUartClk(id, 1);
	setUartIo(id);
		
	uartCtx->inst = getUartInst(id);
	dbgMsg("create uart inst at %p\r\n", uartCtx->inst);
	//SYS_ResetModule(UART2_RST);
#ifdef SUPPORT_BLE
	if(uartCtx->uartType == UT_BLE)
	{	SCUART_Open(uartCtx->inst, baudrate);
		if(mode == N_8_2)
			SCUART_SetLineConfig(uartCtx->inst, baudrate, SCUART_CHAR_LEN_8, SCUART_PARITY_NONE, SCUART_STOP_BIT_2);
		else if(mode ==E_8_1)
			SCUART_SetLineConfig(uartCtx->inst, baudrate, SCUART_CHAR_LEN_8, SCUART_PARITY_EVEN, SCUART_STOP_BIT_1);
		else if(mode ==O_8_1)
			SCUART_SetLineConfig(uartCtx->inst, baudrate, SCUART_CHAR_LEN_8, SCUART_PARITY_ODD, SCUART_STOP_BIT_1);
		else
			SCUART_SetLineConfig(uartCtx->inst, baudrate, SCUART_CHAR_LEN_8, SCUART_PARITY_NONE, SCUART_STOP_BIT_1);
	}
	else
#endif
	{	UART_Open(uartCtx->inst, baudrate);

		if(mode == N_8_2)
			UART_SetLineConfig(uartCtx->inst, baudrate, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_2);
		else if(mode == E_8_1)
			UART_SetLineConfig(uartCtx->inst, baudrate, UART_WORD_LEN_8, UART_PARITY_EVEN, UART_STOP_BIT_1);
		else if(mode == O_8_1)
			UART_SetLineConfig(uartCtx->inst, baudrate, UART_WORD_LEN_8, UART_PARITY_ODD, UART_STOP_BIT_1);
		else
			UART_SetLineConfig(uartCtx->inst, baudrate, UART_WORD_LEN_8, UART_PARITY_NONE, UART_STOP_BIT_1);
		if(uartCtx->flowCtrl == FC_HW)
			setUartFlowControl(uartCtx, 1);
		else
			setUartFlowControl(uartCtx, 0);
	}
	//freertos_semaInit(&uartCtx->sema, 1);
	setUartIsr(id, 1);
	return 0;
}
//------------------------------------------------------------------------------
int uartDeInit(UART_CTX *uartCtx)
{	UART_RX_CTX *rxCtx;
	//UART_CTX *uartCtx;
	//UART_CONFIG *uartCfg = (UART_CONFIG *)uartCfgPtr;
	//uartCtx = uartCfg->ctx;
	rxCtx = uartCtx->rxCtx;
	
	if(uartCtx->uartType != UT_VCOMM)
	{	if(uartCtx->uartType == UT_UART)
			UART_Close((UART_T *)uartCtx->inst);
#ifdef SUPPORT_BLE
		else if(uartCtx->uartType == UT_BLE)
			SCUART_Close((SC_T *)uartCtx->inst);
#endif
	}
	
	setUartIsr(uartCtx->id, 0);
	setUartClk(uartCtx->id, 0);
	
	SENS_MEM_FREE(rxCtx->buffer);
	SENS_MEM_FREE(rxCtx);
	SENS_MEM_FREE(uartCtx);
	return 0;
}
//------------------------------------------------------------------------------
void waitUartTxEmpty(UART_CTX *uartCtx)
{	if(uartCtx->uartType == UT_UART)
		while(UART_IS_TX_FULL((UART_T *)uartCtx->inst));
#ifdef SUPPORT_BLE
	else	//for Sensmini S2 SCUART
		while(SCUART_IS_TX_FULL((SC_T *)uartCtx->inst));
#endif
}
//------------------------------------------------------------------------------
uint32_t uartSend(UART_CTX *uartCtx, uint8_t *buffer, uint32_t length)
{	uint32_t xmitLen;
	uint32_t offset = 0;
	uint8_t first=1;
	int bleEvent;
	
	if(uartCtx->uartType == UT_UART)
	{	if((uartCtx->id == RS485_1_UART_NO) || (uartCtx->id == RS485_2_UART_NO))
		{	setRs485Dir(uartCtx->id == RS485_1_UART_NO? RS485_CHANNEL_1:RS485_CHANNEL_2, RS485_TRANSMIT_MODE);
			SENS_TIME_DELAY(1);
		}
				
		while(UART_IS_TX_FULL((UART_T *)uartCtx->inst));
		UART_Write((UART_T *)uartCtx->inst ,(uint8_t *)buffer, length);
		//if(uartCtx->uartType == UT_RS485)
			while(!UART_IS_TX_EMPTY((UART_T *)uartCtx->inst)) __NOP();
		if((uartCtx->id == RS485_1_UART_NO) || (uartCtx->id == RS485_2_UART_NO))
		{	setRs485Dir(uartCtx->id == RS485_1_UART_NO? RS485_CHANNEL_1:RS485_CHANNEL_2, RS485_RECEIVE_MODE);
		}
	}
#if 0
	else
	{	vCommWrite(uartCtx, buffer, length);
	}
#endif
	return length;
}
//------------------------------------------------------------------------------
int getUartCtsSts(UART_CTX *uartCtx)
{	UART_T *uartInst = (UART_T *)uartCtx->inst;
	//dbgMsg("uartCtx->inst at %p\r\n", uartCtx->inst);
	if(uartInst->MODEMSTS & UART_MODEMSTS_CTSSTS_Msk)
		return 1;
	else
		return 0;
}
//------------------------------------------------------------------------------
void setBaudRate(UART_CTX *uartCtx, int baudrate)
{	UART_T	*inst;
	uint32_t lineRegVal;
	
	inst = uartCtx->inst;
	lineRegVal = inst->LINE;
	
	UART_SetLineConfig(inst, baudrate, lineRegVal, 0, 0);
}
//------------------------------------------------------------------------------