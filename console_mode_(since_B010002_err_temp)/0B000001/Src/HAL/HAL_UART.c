#include "sensminiCfg.h"


int HAL_UART_INIT( SENS_UART_CTX *uartCtx, uint8_t id, int baudrate, enum UART_MODE mode, uint32_t rxBufferSize, uint8_t uartType, uint8_t flowCtrl)
{	return uartInit(uartCtx, id, baudrate, mode, rxBufferSize, uartType, flowCtrl);
}

void HAL_UART_DE_INIT(SENS_UART_CTX *uartCtx)
{	//nothing to do
	uartDeInit(uartCtx);
}

void HAL_UART_FLUSH_INPUT(SENS_UART_CTX *uartCtx)
{	//nothing to do
	//uartClearFifo(uartCtx);
	uartClearFifo(uartCtx);
}

void HAL_UART_FFLUSH(SENS_UART_CTX *uartCtx)
{	uartClearFifo(uartCtx);
}

int HAL_UART_WRITE(SENS_UART_CTX *uartCtx, uint8_t *buf, uint32_t length)
{	return uartSend(uartCtx, buf, length);
}

int HAL_UART_STATUS(SENS_UART_CTX *uartCtx)
{	return getUartRecvLength(uartCtx);
}

int HAL_UART_READ(SENS_UART_CTX *uartCtx, uint8_t *buf, uint32_t length)
{	return uartRecv(uartCtx, buf, length);
}

/*int HAL_UART_GET_XMIT_PARAM(SENS_UART_CTX *uartCtx, void *param)
{	GetUartStautsParam(uartCtx, param);
	return 0;
}*/

void HAL_UART_CLEAR_FIFO(SENS_UART_CTX *uartCtx)
{	//nothing to do
	uartClearFifo(uartCtx);
}


