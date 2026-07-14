#include "sensors/sensorApp.h"
#include "sensSystem.h"
#include "sensDev.h"
#include "sensCommon.h"
#include "physicalQuantity.h"


//------------------------------------------------------------------------------
void displayPanelInit(void)
{	DISPLAY_PANEL_INSTANCE *displayPanelInst;
	DISPLAY_PANEL_CONTEXT *ctx;
	
	if(sensDev->displayPanelInst)
	{	displayPanelInst = sensDev->displayPanelInst;
		ctx = &displayPanelInst->ctx;
		ctx->timer = GetTimeTicks();
		ctx->timeout = 3000;
	}
}
//------------------------------------------------------------------------------
static void sendDisplayPanel(DISPLAY_PANEL_CONTEXT *ctx, EXT_DEV_CONFIG *extDevCfg)
{	UART_CONFIG *uartCfg = sensDev->rs485_Comm[1];
	int idx;
	
	for(idx=0;idx<sensPq->pqNumber;idx++)
	{	ctx->buf[0] = idx+1;
		ctx->buf[1] = 0x06;
		ctx->buf[2] = 0x00;
		ctx->buf[3] = 0x26;
		ctx->buf[4] = 0x00;
		ctx->buf[5] = 0x00;
		ctx->crc16 = swCrc16((uint8_t *)ctx->buf, 0, 6);
		ctx->buf[6] = ctx->crc16 		& 0xFF;
		ctx->buf[7] = (ctx->crc16 >> 8 )& 0xFF;
		SENS_UART_WRITE(uartCfg->ctx, (uint8_t *)ctx->buf, 8);
		SENS_TIME_DELAY(10);
		
		ctx->pqVal = (uint16_t)(sensPq->pqVal[idx] * 100);
		ctx->buf[3] = 0x27;
		ctx->buf[4] = (ctx->pqVal >> 8) & 0xFF;
		ctx->buf[5] = ctx->pqVal  		& 0xFF;
		ctx->crc16 = swCrc16((uint8_t *)ctx->buf, 0, 6);
		ctx->buf[6] = ctx->crc16 		& 0xFF;
		ctx->buf[7] = (ctx->crc16 >> 8 )& 0xFF;
		SENS_UART_WRITE(uartCfg->ctx, (uint8_t *)ctx->buf, 8);
		SENS_TIME_DELAY(10);
	}
}
//------------------------------------------------------------------------------
void displayPanelProcess(void)
{	DISPLAY_PANEL_INSTANCE *displayPanelInst;
	DISPLAY_PANEL_CONTEXT *ctx;
	EXT_DEV_CONFIG *extDevCfg;
	
	if(sensDev->displayPanelInst)
	{	displayPanelInst = sensDev->displayPanelInst;
		ctx = &displayPanelInst->ctx;
		extDevCfg = (EXT_DEV_CONFIG *)displayPanelInst->extDevCfg;
		
		if((GetTimeTicks() - ctx->timer) > ctx->timeout)
		{	sendDisplayPanel(ctx, extDevCfg);
			ctx->timer = GetTimeTicks();
		}
	}
}
