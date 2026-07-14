#include "peripheral/rs485.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "sensDev.h"
#include "powerCtrl.h"

//------------------------------------------------------------------------------
void setRs485Dir(int channel, uint8_t direction)
{	if(channel == EXT_IF_CHANNEL_RS485_1)
		PIN_RS485_1_DE = direction;
	else
		PIN_RS485_2_DE = direction;
}
//------------------------------------------------------------------------------
int rs485SendData(int channel, uint8_t *buf, uint32_t length)
{	UART_CONFIG *uartCfg;
	
	uartCfg = sensDev->rs485_Comm[channel];
	return SENS_UART_WRITE(uartCfg->ctx, buf, length);
}
//------------------------------------------------------------------------------
void rs485Init(int channel)
{	COMM_IF_CONFIG	*commIfCfg;
	UART_CONFIG *uartCfg;
	
	commIfCfg = getCommIfConfigByIfNo(channel);
	
	if(commIfCfg->protocol != EXT_IF_PROTO_NONE)
	{	if(sensDev->rs485_Comm[channel] == NULL)
		{	sensDev->rs485_Comm[channel] = SENS_MEM_ZALLOC(sizeof(UART_CONFIG));
		}
		uartCfg = sensDev->rs485_Comm[channel];
		uartCfg->baudrate = commIfCfg->baudrate;
		uartCfg->mode = commIfCfg->format;
		
		if(uartCfg->ctx == NULL)
			uartCfg->ctx = SENS_MEM_ZALLOC(sizeof(SENS_UART_CTX));
		uartCfg->flowCtrl = FC_NONE;
		if(channel == EXT_IF_CHANNEL_RS485_1)
			uartCfg->id = RS485_1_UART_NO;
		else 
			uartCfg->id = RS485_2_UART_NO;
		uartCfg->uartType = UT_UART;
		uartCfg->bufferLength = 512;
		addUartCfg(uartCfg);
		SENS_UART_INIT(uartCfg->ctx, uartCfg->id, uartCfg->baudrate, uartCfg->mode, uartCfg->bufferLength, UT_UART, uartCfg->flowCtrl);
		SENS_UART_FLUSH_INPUT(uartCfg->ctx);
		setRs485Pwr(channel, 1);
		setRs485Dir(channel, RS485_RECEIVE_MODE);
	}
	else
		setRs485Pwr(channel, 0);
}