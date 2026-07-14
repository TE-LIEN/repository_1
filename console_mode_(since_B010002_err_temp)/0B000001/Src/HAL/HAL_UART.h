#ifndef __HAL_UART_H__
#define __HAL_UART_H__

#include "driver/uartDrv.h"

#define SENS_UART_CTX						UART_CTX
//#define SENS_UART_CTX_PTR					UART_CTX_PTR
#define SENS_UART_INIT						HAL_UART_INIT
#define SENS_UART_FLUSH_INPUT			HAL_UART_FLUSH_INPUT
#define SENS_UART_FFLUSH					HAL_UART_FFLUSH
#define SENS_UART_WRITE						HAL_UART_WRITE
#define SENS_UART_STATUS					HAL_UART_STATUS
#define SENS_UART_READ						HAL_UART_READ
#define SENS_UART_GET_XMIT_PARAM	HAL_UART_GET_XMIT_PARAM
#define SENS_UART_CLEAR_FIFO			HAL_UART_CLEAR_FIFO
#define SENS_UART_DE_INIT					HAL_UART_DE_INIT


extern int HAL_UART_INIT( SENS_UART_CTX *uartCtx, uint8_t id, int baudrate, enum UART_MODE mode, uint32_t rxBufferSize, uint8_t uartType, uint8_t flowCtrl);
extern void HAL_UART_DE_INIT(SENS_UART_CTX *uartCtx);
extern void HAL_UART_FLUSH_INPUT(SENS_UART_CTX *uartCtx);
extern void HAL_UART_FFLUSH(SENS_UART_CTX *uartCtx);
extern int HAL_UART_WRITE(SENS_UART_CTX *uartCtx, uint8_t *buf, uint32_t length);
extern int HAL_UART_STATUS(SENS_UART_CTX *uartCtx);
extern int HAL_UART_READ(SENS_UART_CTX *uartCtx, uint8_t *buf, uint32_t length);
//extern int HAL_UART_GET_XMIT_PARAM(SENS_UART_CTX *uartCtx, void *param);
extern void HAL_UART_CLEAR_FIFO(SENS_UART_CTX *uartCtx);

#endif
