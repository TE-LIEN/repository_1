#include "sensors/sensorApp.h"
#include "sensDev.h"
#include "sensSystem.h"
#include "sensCommon.h"

//------------------------------------------------------------------------------
static DCON_INSTANCE *getDconInst(DEV_INSTANCE	*devInst)
{	DCON_INSTANCE	*dconInst = NULL;
	NORMAL_SENSOR_INSTANCE *normalSensorInst;
	NORMAL_SENSOR_CONTEXT	*ctx;
	
	normalSensorInst = (NORMAL_SENSOR_INSTANCE *)devInst->extDevInstPtr;
	if(normalSensorInst == NULL)
		return dconInst;
	ctx = normalSensorInst->ctx;
	if(ctx == NULL)
		return dconInst;
	dconInst = ctx->dconInst;
	
	return dconInst;
}
//------------------------------------------------------------------------------
static int dconSendData(DCON_INSTANCE *dconInst, int startTimeTicket)
{	UART_CONFIG *uartCfg = dconInst->uartCfg;
	
	SENS_UART_CLEAR_FIFO(uartCfg->ctx);
	SENS_UART_WRITE(uartCfg->ctx, (uint8_t *)dconInst->sendBuf, strlen(dconInst->sendBuf));
	dconInst->recvLength = 0;
	dconInst->rspTimeout = 2000;
	dconInst->rspTimer = startTimeTicket;
	return 0;
}
//------------------------------------------------------------------------------
static int dconRecvData(DCON_INSTANCE *dconInst, int startTimeTicket)
{	UART_CONFIG *uartCfg = dconInst->uartCfg;
	char recvByte;
	int ret = -1;
	
	if((startTimeTicket - dconInst->rspTimer) < dconInst->rspTimeout)
	{	if(SENS_UART_STATUS(uartCfg->ctx))
		{	SENS_UART_READ(uartCfg->ctx, (uint8_t *)&recvByte, 1);
			if(recvByte == '\r')
			{	ret = 0;
				dconInst->isPqReady = 1;
				
			}
			else
			{	dconInst->recvBuf[dconInst->recvLength++] = recvByte;
				dconInst->recvBuf[dconInst->recvLength] = '\0';
			}
		}
	}
	else	//timeout
	{	dconInst->timeoutCount++;
		if(dconInst->timeoutCount > 3)
		{	dconInst->isRspTimeout = 1;
			dconInst->timeoutCount--;
			ret = 0;
		}
	}
	
	return ret;
}
//------------------------------------------------------------------------------
static void addDconHwPqInf(NORMAL_SENSOR_INSTANCE *sensorInst, SENSOR_HW_PQ_STRUCT *newHwPqInf)
{	SENSOR_HW_PQ_STRUCT *hwPqInf;
	
	if(sensorInst->hwPqInf == NULL)
		sensorInst->hwPqInf = newHwPqInf;
	else
	{	hwPqInf = sensorInst->hwPqInf;
		while(hwPqInf->next)
		{	hwPqInf = hwPqInf->next;
		}
		hwPqInf->next = hwPqInf;
	}
}
//------------------------------------------------------------------------------
static SENSOR_HW_PQ_STRUCT* getHwPqInf(NORMAL_SENSOR_INSTANCE *sensorInst, int hwPqIdx)
{	SENSOR_HW_PQ_STRUCT *hwPqInf = NULL;
	
	for(hwPqInf = sensorInst->hwPqInf;hwPqInf;hwPqInf=hwPqInf->next)
	{	if(hwPqInf->hwPqIdx == hwPqIdx)
			break;
	}
	
	return hwPqInf;
}
//------------------------------------------------------------------------------
static int dconParser(DCON_INSTANCE *dconInst)
{	int hwPqIdx = 0;
	int idx;
	int parserStart=0;
	NORMAL_SENSOR_INSTANCE *sensorInst = dconInst->sensorInst;
	SENSOR_HW_PQ_STRUCT		*hwPqInf;
	int addHwPqInf;
	char hwPqValArray[15];
	uint32_t hwPqVal;
	char *endptr;
	uint8_t isLastParser = 0;
	
	if(dconInst->isRspTimeout)
	{	for(hwPqInf = sensorInst->hwPqInf;hwPqInf;hwPqInf=hwPqInf->next)
		{	hwPqInf->pqVal = SENS_SET_VAL_NAN();
			hwPqInf->isHwPqReady = 1;
		}
		dconInst->isRspTimeout = 0;
		return 0;
	}
	parserStart = dconInst->startParseByte;
	
	while(parserStart < dconInst->recvLength)
	{	addHwPqInf = 0;
		hwPqInf = getHwPqInf(sensorInst, hwPqIdx);
		if(hwPqInf == NULL)
		{	hwPqInf = SENS_MEM_ZALLOC(sizeof(SENSOR_HW_PQ_STRUCT));
			addHwPqInf = 1;
			hwPqInf->hwPqIdx = hwPqIdx;
		}
		for(idx=0;idx<dconInst->parserLength;idx++)
		{	if(((parserStart + idx) >= dconInst->recvLength) || (dconInst->recvBuf[parserStart + idx] == '\r'))
			{	isLastParser = 1;
				break;
			}
			hwPqValArray[idx] = dconInst->recvBuf[parserStart + idx];
		}
		hwPqValArray[idx] = '\0';
		parserStart += idx;
		hwPqVal = strtol(hwPqValArray, &endptr, 16);
		hwPqInf->pqVal = (float)hwPqVal;
		hwPqInf->isHwPqReady = 1;
		if(addHwPqInf)
			addDconHwPqInf(sensorInst, hwPqInf);
		hwPqIdx++;
		if(isLastParser)
			break;
	}
	
	dconInst->isPqReady = 0;
	dconInst->timeoutCount = 0;
	return 0;
}
//------------------------------------------------------------------------------
static int processDconSensor(DEV_INSTANCE	*devInst, int *findNext)
{	DCON_INSTANCE	*dconInst = getDconInst(devInst);
	NORMAL_SENSOR_INSTANCE	*sensorInst = (NORMAL_SENSOR_INSTANCE *)devInst->extDevInstPtr;
	NORMAL_SENSOR_CONTEXT	*ctx = (NORMAL_SENSOR_CONTEXT *)sensorInst->ctx;
	SENSOR_PROCESS_CONTEXT *processCtx = (SENSOR_PROCESS_CONTEXT *)&ctx->processCtx;
	
	int pwrCtrl;
	int startTimeTicket = GetTimeTicks();
	
	if(sysCfg->sensSysCfg.psm == SYS_CFG_PSM_ADVANCE)
		pwrCtrl = 1;
	else
		pwrCtrl = 0;
	
	switch(dconInst->fsm)
	{	case DCON_DEV_FSM_IDLE:
				processCtx->activeWaitTimeout = 0;
				processCtx->activeWaitTimer = startTimeTicket;
		case DCON_DEV_FSM_INIT:
				dconInst->fsm = DCON_DEV_FSM_INIT;
				if(processCtx->activeWaitTimeout)
				{	if((startTimeTicket - processCtx->activeWaitTimer) > processCtx->activeWaitTimeout)
					{	dconInst->fsm = DCON_DEV_FSM_PWR_ON;
					}
					else
						break;
				}
		case DCON_DEV_FSM_PWR_ON:
				dconInst->fsm = DCON_DEV_FSM_PWR_ON;
				if(pwrCtrl)
				{
					
				}
				processCtx->pwrOnDelayTimer = startTimeTicket;
				dconInst->fsm = DCON_DEV_FSM_PWR_ON_WAIT;
		case DCON_DEV_FSM_PWR_ON_WAIT:
				if((startTimeTicket - processCtx->pwrOnDelayTimer) > processCtx->pwrOnDelayTimeout)
				{	dbg_msg("[SENSOR] sensor wait power on delay timer(%d) done \r\n", processCtx->pwrOnDelayTimeout);
					processCtx->pollingTimer = startTimeTicket;
					dconInst->fsm = DCON_DEV_FSM_POLLING_WAIT;
				}
				else
					break;
		case DCON_DEV_FSM_POLLING_WAIT:
				if((startTimeTicket - processCtx->pollingTimer) > processCtx->pollingTimeout)
				{	//dbg_msg("[SENSOR] sensor wait power on delat timer(%d) done \r\n", processCtx->pwrOnDelayTimeout);
					dconInst->fsm = DCON_DEV_FSM_SEND;
				}
				else
					break;
		case DCON_DEV_FSM_SEND:
				*findNext = 0;
				dconSendData(dconInst, startTimeTicket);
		case DCON_DEV_FSM_RECV:
				{	*findNext = 0;
					dconInst->fsm = DCON_DEV_FSM_RECV;
					if(!dconRecvData(dconInst, startTimeTicket))
					{	dconInst->fsm = DCON_DEV_FSM_GET_HW_PQ;
					}
					else
						break;
				}
		case DCON_DEV_FSM_GET_HW_PQ:
				dconParser(dconInst);
				*findNext = 1;
		case DCON_DEV_FSM_SET_SW_PQ:
				{	SENSOR_HW_PQ_STRUCT		*hwPqInf;
					hwPqInf = sensorInst->hwPqInf;
					setSwPq(devInst, hwPqInf);
					dconInst->fsm = DCON_DEV_FSM_PWR_OFF;
				}
		case DCON_DEV_FSM_PWR_OFF:
				if(pwrCtrl)
				{
				}
				else
				{	processCtx->pwrOffDelayTimeout = 0;
					dconInst->fsm = DCON_DEV_FSM_PWR_OFF_WAIT;
				}
		case DCON_DEV_FSM_PWR_OFF_WAIT:
				if(processCtx->pwrOffDelayTimeout)
				{	if((startTimeTicket - processCtx->pwrOffDelayTimer) > processCtx->pwrOffDelayTimeout)
						dconInst->fsm = DCON_DEV_FSM_IDLE;
				}
				else
				{	processCtx->pollingTimer = startTimeTicket;
					dconInst->fsm = DCON_DEV_FSM_POLLING_WAIT;
				}
				break;
		default:
				break;
	}
	return 0;
}
//------------------------------------------------------------------------------
static DEV_INSTANCE *extDevDconProcess(DEV_INSTANCE *devInst, int findNext)
{	DEV_INSTANCE *nextDevInst = devInst;
	
	if(devInst->instIsValid)
	{	processDconSensor(devInst, &findNext);
	}
	
	
	if(findNext)
	{	if(devInst->next)
			nextDevInst = devInst->next;
		else
			nextDevInst = sensDev->dconDevInsts;
	}
	
	return nextDevInst;
}
//------------------------------------------------------------------------------
DEV_INSTANCE *dconProcess(DEV_INSTANCE *devInst)
{	DEV_INSTANCE *retDevInst = NULL;
	
	if(devInst)
		retDevInst = extDevDconProcess(devInst, 1);
	return retDevInst;
}
//------------------------------------------------------------------------------
static int parserNormalSensor(DEV_INSTANCE	*devInst)
{	NORMAL_SENSOR_INSTANCE	*sensorInst = devInst->extDevInstPtr;
	NORMAL_SENSOR_CONTEXT	*ctx;
	DCON_INSTANCE			*dconInst;
	SENSOR_PROCESS_CONTEXT	*processCtx;
	EXT_DEV_CONFIG 			*extDevCfg;
	SENSOR_HW_PQ_STRUCT		*hwPqInf;
	
	sensorInst->devInst = devInst;
	if(sensorInst->ctx == NULL)
		sensorInst->ctx = SENS_MEM_ZALLOC(sizeof(NORMAL_SENSOR_CONTEXT));
	ctx = sensorInst->ctx;
	processCtx = &ctx->processCtx;
	processCtx->pollingTimeout = 1000;	//default time
	processCtx->pwrOnDelayTimeout = 4000; //default
	
	if(ctx->dconInst == NULL)
		ctx->dconInst = SENS_MEM_ZALLOC(sizeof(DCON_INSTANCE));
	dconInst = ctx->dconInst;
	extDevCfg = getExtDevCfgByIdx(devInst->extDevNo);
	dconInst->channel = devInst->interface;
	dconInst->sendBuf = SENS_MEM_ZALLOC(strlen(extDevCfg->command) + 2);	//add EOF
	strcpy(dconInst->sendBuf, extDevCfg->command);
	strcat(dconInst->sendBuf, "\r");	//add EOF
	dconInst->recvBuf = SENS_MEM_ZALLOC(256);
	dconInst->startParseByte = extDevCfg->startParseByte;
	dconInst->parserLength = extDevCfg->dataType;
	dconInst->parserType = extDevCfg->dataOrder;
	dconInst->sensorInst = sensorInst;
	devInst->instIsValid = 1;
	
	/*hwPqInf = createHwPqInf(modbusInst, 0);
	normalInst->hwPqInf = hwPqInf;*/
	
	return 0;
}

//------------------------------------------------------------------------------
void dconInit(void)
{	DEV_INSTANCE 	*devInst;
	DCON_INSTANCE	*dconInst;
	int rsp;
	UART_CONFIG *uartCfg;
	
	for(devInst=sensDev->dconDevInsts;devInst;devInst=devInst->next)
	{	if(devInst->protocol == EXT_IF_PROTO_DCON)
		{	switch(devInst->extDevType)
			{	case EXT_DEV_TYPE_DCON_NORMAL:	rsp = parserNormalSensor(devInst);	break;
				default:						rsp = -1;							break;
			}
			if(!rsp)
			{	dconInst = getDconInst(devInst);
				if(dconInst->channel == EXT_IF_CHANNEL_RS485_1)
					uartCfg = (UART_CONFIG *)sensDev->rs485_Comm[0];
				else if(dconInst->channel == EXT_IF_CHANNEL_RS485_2)
					uartCfg = (UART_CONFIG *)sensDev->rs485_Comm[1];
				else if(dconInst->channel == EXT_IF_CHANNEL_RS232)
					uartCfg = (UART_CONFIG *)sensDev->rs232_Comm;
				dconInst->uartCfg = uartCfg;
			}
		}
	}
}
//------------------------------------------------------------------------------