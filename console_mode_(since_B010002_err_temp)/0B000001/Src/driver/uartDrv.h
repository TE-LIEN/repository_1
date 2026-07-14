#ifndef __UART_DRV_H__
#define __UART_DRV_H__

#include "sensminiCfg.h"


#define RB_MASK(rb, val)   ((val) & ((rb)->mask))
#define RB_SPACE(rb, h, t) ((rb)->size - ((h - t) & (rb)->mask) - 1U) // ÉÐ¿ÉŒ‘Èë¿Õég
#define RB_COUNT(rb, h, t) (((h - t) & (rb)->mask))

typedef void (*pinmux_fn_t)(void);

typedef struct _UART_CLK_IO_DEF
{	uint32_t 		module;
	uint32_t 		clkSel;
	uint32_t 		clkDiv;
	pinmux_fn_t	 	pinMuxFn;
	uint32_t		irqNo;
	UART_T*  		uart;
}UART_CLK_IO_DEF;


#define MAX_UART_NO		10

extern int uartInit( UART_CTX *uartCtx, uint8_t id, int baudrate, enum UART_MODE mode, uint32_t rxBufferSize, uint8_t uartType, uint8_t flowCtrl);
//extern int uartInit(void *uartCfgPtr);
//extern int uartDeInit(void *uartCfgPtr);
extern int uartDeInit(UART_CTX *uartCtx);
extern uint32_t uartRecv(UART_CTX *uartCtx, uint8_t *buffer, uint32_t length);
extern void uartClearFifo(UART_CTX *uartCtx);
extern uint32_t getUartRecvLength(UART_CTX *uartCtx);
extern uint32_t uartSend(UART_CTX *uartCtx, uint8_t *buffer, uint32_t length);
extern int getUartCtsSts(UART_CTX *uartCtx);
extern void setBaudRate(UART_CTX *uartCtx, int baudrate);
extern void setUartFlowControl(UART_CTX *uartCtx, uint8_t enable);

extern void uartFifoPut(uint8_t isIsr, uint8_t id, uint8_t *data, uint16_t length);

extern void setUartIsr(uint8_t id, uint8_t on);
#endif

