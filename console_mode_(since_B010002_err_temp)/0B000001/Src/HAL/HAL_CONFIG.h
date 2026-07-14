#ifndef __HAL_CONFIG_H__
#define __HAL_CONFIG_H__

#include "sensminiCfg.h"

#define RS485_SEND_MODE							1
#define RS485_RECV_MODE							0

enum UART_MODE
{	N_8_1,
	N_8_2,
	E_8_1,
	O_8_1
};

typedef struct _UART_RX_CTX
{	uint8_t 			*buffer;
	uint32_t			size;
	uint32_t			mask;
	volatile uint32_t 	head;
	volatile uint32_t	tail;
	volatile uint32_t	overrun;
	volatile uint32_t	dropInputCount;
	
	/*volatile uint8_t 	*recvPos;
	volatile uint8_t 	*getPos;
	uint8_t 			*startPos;
	uint8_t 			*endPos;
	uint32_t			bufSize;
	volatile uint32_t 	length;
	uint32_t 			dropInputCount;	//buffer full
	uint32_t 			overRunCount;*/
}UART_RX_CTX;

typedef struct _UART_CTX
{	uint8_t	id;
	void	*inst;
	UART_RX_CTX	*rxCtx;
	xSemaphoreHandle sema;
	uint8_t		rs485XmitMode;
	uint8_t		flowCtrl;
	uint8_t		uartType;
	void 	*vcommCtx;
}UART_CTX, * UART_CTX_PTR;

#endif
