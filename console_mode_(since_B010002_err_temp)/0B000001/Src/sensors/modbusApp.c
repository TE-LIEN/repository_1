#include "sensors/sensorApp.h"
#include "sensSystem.h"
#include "sensDev.h"
#include "sensors/modbus/modbus.h"
#include "powerCtrl.h"
#include "physicalQuantity.h"
#include "sdCardTask.h"
#include <math.h>

//------------------------------------------------------------------------------
static int getModbusInfoFromCmdParam(MODBUS_INSTANCE *modbusInst, char *cmdStr)
{	char *end;
	char convertPtr[3];
	char *cmdPtr;
	char *cmdTempPtr;
	char *posPtr;
	char sep[2];
	int cmdLen;
	int parserFail = 0;
	uint8_t convertData;
	MODBUS_REQ_CONTEXT *reqCtx = &modbusInst->reqCtx;
  
	
	cmdPtr = SENS_MEM_ZALLOC(strlen(cmdStr)+1);
	memcpy(cmdPtr, cmdStr, strlen(cmdStr));
	cmdTempPtr = cmdPtr;
	strtokLock();
	if(modbusInst->channel == EXT_IF_CHANNEL_ETH)
	{	strcpy(sep, ",");
		posPtr = strtok(cmdPtr, sep);
		cmdPtr = strtok(NULL, sep);
		reqCtx->ip = SENS_MEM_ZALLOC(16);
		memcpy(reqCtx->ip, posPtr, strlen(posPtr));
		dbg_msg("modbus TCP ip is: %s\r\n", reqCtx->ip);
	}
	dbg_msg("modbus RTU/TCP CMD is: %s\r\n", cmdPtr);
	cmdLen = strlen(cmdPtr);
	if(cmdLen < 2)
		parserFail = -3;
	if(cmdLen >= 2)
	{	convertPtr[2] = 0;
		convertPtr[0] = *cmdPtr++;
		convertPtr[1] = *cmdPtr++;
		reqCtx->slaveId = (uint8_t)strtol(convertPtr, &end, 16);
		if(cmdLen == 2)
		{	dbg_msg("modbus slave id:%02X\r\n", reqCtx->slaveId);
			parserFail = -2;
		}
	}
	if(cmdLen == 12)
	{	convertPtr[0] = *cmdPtr++;
		convertPtr[1] = *cmdPtr++;
		reqCtx->funcCode = (uint8_t)strtol(convertPtr, &end, 16);
		convertPtr[0] = *cmdPtr++;
		convertPtr[1] = *cmdPtr++;
		convertData = (uint8_t)strtol(convertPtr, &end, 16);
		reqCtx->regNo = convertData << 8;
		convertPtr[0] = *cmdPtr++;
		convertPtr[1] = *cmdPtr++;
		convertData = (uint8_t)strtol(convertPtr, &end, 16);
		reqCtx->regNo |= convertData;
		convertPtr[0] = *cmdPtr++;
		convertPtr[1] = *cmdPtr++;
		convertData = (uint8_t)strtol(convertPtr, &end, 16);
		reqCtx->readLength = convertData << 8;
		convertPtr[0] = *cmdPtr++;
		convertPtr[1] = *cmdPtr;
		convertData = (uint8_t)strtol(convertPtr, &end, 16);
		reqCtx->readLength |= convertData;
		dbg_msg("modbus slave id:%02X, func:%02X, regNo:%04X, len:%04X\r\n", reqCtx->slaveId, reqCtx->funcCode, reqCtx->regNo, reqCtx->readLength);
	}
	else if(cmdLen > 2)
		parserFail = -1;
	strtokUnLock();
	SENS_MEM_FREE(cmdTempPtr);
	
	return parserFail;
}
//------------------------------------------------------------------------------
static SENSOR_HW_PQ_STRUCT *createHwPqInf(MODBUS_INSTANCE	*modbusInst, int createPrevFilter)
{	int pqNo;
	int rspValSize;
	int bufSize;
	MODBUS_REQ_CONTEXT *reqCtx = &modbusInst->reqCtx;
	MODBUS_RSP_CONTEXT *rspCtx = &modbusInst->rspCtx;
	SENSOR_HW_PQ_STRUCT		*hwPqInf;
	SENSOR_HW_PQ_STRUCT		*hwPqInfTemp;
	SENSOR_HW_PQ_STRUCT		*topHwPqInf = NULL;
	SENSOR_ALGO_STRUCT		*prevFilterAlgo;
	int pqIdx;
	
	rspValSize = (reqCtx->funcCode < MODBUS_FC_READ_HOLDING_REGISTERS)? sizeof(uint8_t):sizeof(uint16_t);
	bufSize = reqCtx->readLength * rspValSize;
	if((reqCtx->dataType == MODBUS_DT_FLOAT) || (reqCtx->dataType == MODBUS_DT_INT32) || (reqCtx->dataType == MODBUS_DT_UINT32))
		pqNo = bufSize / 4;
	else if(reqCtx->dataType == MODBUS_DT_BOOL)
		pqNo = bufSize;
	else
		pqNo = bufSize / 2;
		
	rspCtx->rspBuf = SENS_MEM_ZALLOC(bufSize);
	
	for(pqIdx=0;pqIdx<pqNo;pqIdx++)
	{	hwPqInf = SENS_MEM_ZALLOC(sizeof(SENSOR_HW_PQ_STRUCT));
		hwPqInf->hwPqIdx = pqIdx;
		if(createPrevFilter)
		{	prevFilterAlgo = SENS_MEM_ZALLOC(sizeof(SENSOR_ALGO_STRUCT));
			prevFilterAlgo->maxCount = SPECIALL_MODBUS_SENSOR_FILTER_MAX_COUNT;
			prevFilterAlgo->valArray = SENS_MEM_ZALLOC(sizeof(float) * prevFilterAlgo->maxCount);
			prevFilterAlgo->stdValArray = SENS_MEM_ZALLOC(sizeof(float) * prevFilterAlgo->maxCount);
			hwPqInf->prevFilterAlgo = prevFilterAlgo;
		}
		if(topHwPqInf == NULL)
			topHwPqInf = hwPqInf;
		else
		{	hwPqInfTemp = topHwPqInf;
			while(hwPqInfTemp->next)
			{	hwPqInfTemp = hwPqInfTemp->next;
			}
			hwPqInfTemp->next = hwPqInf;
		}
	}
	
	return topHwPqInf;
}
//------------------------------------------------------------------------------
static int parserNormalSensor(DEV_INSTANCE	*devInst)
{	NORMAL_SENSOR_INSTANCE *normalInst = devInst->extDevInstPtr;
	int extDevNo = devInst->extDevNo;
	NORMAL_SENSOR_CONTEXT	*ctx;
	SENSOR_HW_PQ_STRUCT		*hwPqInf;
	MODBUS_INSTANCE	*modbusInst;
	MODBUS_REQ_CONTEXT	*reqCtx;
	SENSOR_PROCESS_CONTEXT *processCtx;
	int cfgParserFail;
	EXT_DEV_CONFIG *extDevCfg;
	//int pqNo;
	
	if(normalInst->ctx == NULL)
		normalInst->ctx = SENS_MEM_ZALLOC(sizeof(NORMAL_SENSOR_CONTEXT));
	ctx = normalInst->ctx;
	processCtx = &ctx->processCtx;
	processCtx->pollingTimeout = 1000;	//default time
	processCtx->pwrOnDelayTimeout = 4000; //default
	if(ctx->modbusInst == NULL)
		ctx->modbusInst = SENS_MEM_ZALLOC(sizeof(MODBUS_INSTANCE));
	
	modbusInst = ctx->modbusInst;
	extDevCfg = getExtDevCfgByIdx(devInst->extDevNo);
	modbusInst->channel = devInst->interface;
	cfgParserFail = getModbusInfoFromCmdParam(modbusInst, extDevCfg->command);
	if(!cfgParserFail)
	{	reqCtx = &modbusInst->reqCtx;
		
		reqCtx->dataType = extDevCfg->dataType;
		reqCtx->dataOrder = extDevCfg->dataOrder;
		devInst->instIsValid = 1;
		
		hwPqInf = createHwPqInf(modbusInst, 0);
		normalInst->hwPqInf = hwPqInf;
	}
	else
	{	SENS_MEM_FREE(ctx->modbusInst);
		SENS_MEM_FREE(normalInst->ctx);
		if(normalInst->hwPqInf)
			SENS_MEM_FREE(normalInst->hwPqInf);
		if(devInst->extDevInstPtr)
			SENS_MEM_FREE(devInst->extDevInstPtr);
		devInst->extDevInstPtr = NULL;
		devInst->instIsValid = 0;
	}
	return cfgParserFail;
}
//------------------------------------------------------------------------------
static int parserOsa24Sensor(DEV_INSTANCE	*devInst)
{	OSA24G_INSTANCE *osa24Inst = devInst->extDevInstPtr;
	int extDevNo = devInst->extDevNo;
	OSA24G_CONTEXT	*osa24gCtx;
	int cfgParserFail;
	EXT_DEV_CONFIG *extDevCfg;
	//int pqNo;
	MODBUS_INSTANCE	*modbusInst;
	MODBUS_REQ_CONTEXT	*reqCtx;
	SENSOR_HW_PQ_STRUCT		*hwPqInf;
	SENSOR_PROCESS_CONTEXT *processCtx;
	OSA24G_CONFIG *osa24Cfg = (OSA24G_CONFIG *)osa24Inst->osa24gCfgPtr;
	
	extDevCfg = getExtDevCfgByIdx(devInst->extDevNo);
	
	if(osa24Inst->osa24gCtx == NULL)
		osa24Inst->osa24gCtx = SENS_MEM_ZALLOC(sizeof(OSA24G_CONTEXT));
	osa24gCtx = osa24Inst->osa24gCtx;
	processCtx = &osa24gCtx->processCtx;
	processCtx->pollingTimeout = 50;
	processCtx->pwrOnDelayTimeout = 6000;
	processCtx->skipZeroValue = 1;
	if(osa24gCtx->modbusInst == NULL)
		osa24gCtx->modbusInst = SENS_MEM_ZALLOC(sizeof(MODBUS_INSTANCE));
	modbusInst = osa24gCtx->modbusInst;
	modbusInst->channel = devInst->interface;
	cfgParserFail = getModbusInfoFromCmdParam(modbusInst, extDevCfg->command);
	if(cfgParserFail != -1)
	{	reqCtx = &modbusInst->reqCtx;
		
		if((cfgParserFail == -2) || (cfgParserFail == -3))
		{	reqCtx->funcCode = 0x03;
			reqCtx->regNo = 0x03E8;
			reqCtx->readLength = 0x04;
			if(cfgParserFail == -3)
				reqCtx->slaveId = osa24Cfg->slaveId;
		}
		
		
		reqCtx->dataType = MODBUS_DT_UINT32;
		reqCtx->dataOrder = MODBUS_DO_abcd;
		devInst->instIsValid = 1;
		cfgParserFail = 0;
		hwPqInf = createHwPqInf(modbusInst, 1);
		osa24Inst->hwPqInf = hwPqInf;
	}
	else
	{	devInst->instIsValid = 0;
		SENS_MEM_FREE(osa24gCtx->modbusInst);
		SENS_MEM_FREE(osa24Inst->osa24gCtx);
		if(osa24Inst->hwPqInf)
			SENS_MEM_FREE(osa24Inst->hwPqInf);
		SENS_MEM_FREE(devInst->extDevInstPtr);
		devInst->extDevInstPtr = NULL;
	}
	return cfgParserFail;
}
//------------------------------------------------------------------------------
static int parserVwSensor(DEV_INSTANCE	*devInst)
{	VW_SENSOR_INSTANCE *vwInst = devInst->extDevInstPtr;
	int extDevNo = devInst->extDevNo;
	VW_SENSOR_CONTEXT	*vwCtx;
	VW_SENSOR_CONFIG *vwCfg = (VW_SENSOR_CONFIG *)vwInst->vwCfgPtr;
	int cfgParserFail;
	//int pqNo;
	MODBUS_INSTANCE	*modbusInst;
	MODBUS_REQ_CONTEXT	*reqCtx;
	SENSOR_HW_PQ_STRUCT		*hwPqInf;
	SENSOR_PROCESS_CONTEXT *processCtx;
	EXT_DEV_CONFIG *extDevCfg = getExtDevCfgByIdx(devInst->extDevNo);
	
	
	if(vwInst->vwCtx == NULL)
		vwInst->vwCtx = SENS_MEM_ZALLOC(sizeof(VW_SENSOR_CONTEXT));
	vwCtx = vwInst->vwCtx;
	processCtx = &vwCtx->processCtx;
	processCtx->pollingTimeout = 1000;
	processCtx->pwrOnDelayTimeout = vwCfg->pwrStableTime;
	if(vwCtx->modbusInst == NULL)
		vwCtx->modbusInst = SENS_MEM_ZALLOC(sizeof(MODBUS_INSTANCE));
	modbusInst = vwCtx->modbusInst;
	modbusInst->channel = devInst->interface;
	cfgParserFail = getModbusInfoFromCmdParam(modbusInst, extDevCfg->command);
	if(cfgParserFail != -1)
	{	reqCtx = &modbusInst->reqCtx;
		
		if((cfgParserFail == -2) || (cfgParserFail == -3))
		{	reqCtx->funcCode = 0x03;
			reqCtx->regNo = 0x0033;
			reqCtx->readLength = 0x08;
			if(cfgParserFail == -3)
				reqCtx->slaveId = vwCfg->slaveId;
		}
		reqCtx->dataType = MODBUS_DT_UINT16;
		reqCtx->dataOrder = MODBUS_DO_ab;
		devInst->instIsValid = 1;
		if(vwInst->hwPqInf == NULL)
			vwInst->hwPqInf = SENS_MEM_ZALLOC(sizeof(SENSOR_HW_PQ_STRUCT));
		cfgParserFail = 0;
		hwPqInf = createHwPqInf(modbusInst, 0);
		vwInst->hwPqInf = hwPqInf;
	}
	else
	{	devInst->instIsValid = 0;
		SENS_MEM_FREE(vwCtx->modbusInst);
		SENS_MEM_FREE(vwInst->vwCtx);
		if(vwInst->hwPqInf)
			SENS_MEM_FREE(vwInst->hwPqInf);
		SENS_MEM_FREE(devInst->extDevInstPtr);
		devInst->extDevInstPtr = NULL;
	}
	return cfgParserFail;
}
//------------------------------------------------------------------------------
static int parserCompositeSensor(DEV_INSTANCE	*devInst)
{	COMPOSITE_SENSOR_INSTANCE *compositeInst = devInst->extDevInstPtr;
	int extDevNo = devInst->extDevNo;
	int isRadar = compositeInst->isRadarDev;
	COMPOSITE_SENSOR_CONTEXT	*compositeCtx;
	MODBUS_INSTANCE	*modbusInst;
	MODBUS_REQ_CONTEXT	*reqCtx;
	int cfgParserFail;
	SENSOR_HW_PQ_STRUCT		*hwPqInf;
	SENSOR_PROCESS_CONTEXT *processCtx;
	EXT_DEV_CONFIG *extDevCfg = getExtDevCfgByIdx(devInst->extDevNo);
	
	if(compositeInst->compositeCtx == NULL)
		compositeInst->compositeCtx = SENS_MEM_ZALLOC(sizeof(COMPOSITE_SENSOR_CONTEXT));
	compositeCtx = compositeInst->compositeCtx;
	processCtx = &compositeCtx->processCtx;
	processCtx->pollingTimeout = 50;
	processCtx->pwrOnDelayTimeout = 6000;
	if(compositeCtx->modbusInst == NULL)
		compositeCtx->modbusInst = SENS_MEM_ZALLOC(sizeof(MODBUS_INSTANCE));
	modbusInst = compositeCtx->modbusInst;
	modbusInst->channel = devInst->interface;
	cfgParserFail = getModbusInfoFromCmdParam(modbusInst, extDevCfg->command);
	if((cfgParserFail != -1) && (cfgParserFail != -3))
	{	reqCtx = &modbusInst->reqCtx;
		
		if(cfgParserFail == -2)
		{	if(isRadar)
			{	reqCtx->funcCode = 0x03;
				reqCtx->regNo = 0x03E8;
				reqCtx->readLength = 0x04;
			}
			else
			{	reqCtx->funcCode = 0x03;
				reqCtx->regNo = 0x04;
				reqCtx->readLength = 0x01;
			}
		}
		if(isRadar)
		{	reqCtx->dataType = MODBUS_DT_UINT32;
			reqCtx->dataOrder = MODBUS_DO_abcd;
		}
		else
		{	reqCtx->dataType = MODBUS_DT_INT16;
			reqCtx->dataOrder = MODBUS_DO_ab;
		}
		devInst->instIsValid = 1;
		if(compositeInst->hwPqInf == NULL)
			compositeInst->hwPqInf = SENS_MEM_ZALLOC(sizeof(SENSOR_HW_PQ_STRUCT));
		cfgParserFail = 0;
		hwPqInf = createHwPqInf(modbusInst, 1);
		compositeInst->hwPqInf = hwPqInf;
	}
	else
	{	devInst->instIsValid = 0;
		SENS_MEM_FREE(compositeCtx->modbusInst);
		SENS_MEM_FREE(compositeInst->compositeCtx);
		if(compositeInst->hwPqInf)
			SENS_MEM_FREE(compositeInst->hwPqInf);
		SENS_MEM_FREE(devInst->extDevInstPtr);
		devInst->extDevInstPtr = NULL;
	}
	return cfgParserFail;
}
//------------------------------------------------------------------------------
static void *getNormalStructInfo(DEV_INSTANCE	*devInst, enum SENSOR_STRUCT_IDX sensorStrIdx)
{	NORMAL_SENSOR_INSTANCE *normalInst = (NORMAL_SENSOR_INSTANCE *)devInst->extDevInstPtr;
	NORMAL_SENSOR_CONTEXT	*ctx = normalInst->ctx;
	MODBUS_INSTANCE	*modbusInst = ctx->modbusInst;
	SENSOR_PROCESS_CONTEXT *processCtx = &ctx->processCtx;
	SENSOR_HW_PQ_STRUCT	*hwPq = normalInst->hwPqInf;
	
	if(sensorStrIdx == SENSOR_STRUCT_IDX_MODBUS_INST)
		return (void *)modbusInst;
	else if(sensorStrIdx == SENSOR_STRUCT_IDX_HW_PQ)
		return (void *)hwPq;
	else  if(sensorStrIdx == SENSOR_STRUCT_IDX_PROCESS_CTX)
		return (void *)processCtx;
	else
		return NULL;
}
//------------------------------------------------------------------------------
static void *getOsaStructInfo(DEV_INSTANCE	*devInst, enum SENSOR_STRUCT_IDX sensorStrIdx)
{	OSA24G_INSTANCE *osa24Inst = (OSA24G_INSTANCE *)devInst->extDevInstPtr;
	OSA24G_CONTEXT	*osa24gCtx = osa24Inst->osa24gCtx;
	MODBUS_INSTANCE	*modbusInst = osa24gCtx->modbusInst;
	SENSOR_PROCESS_CONTEXT *processCtx = &osa24gCtx->processCtx;
	SENSOR_HW_PQ_STRUCT	*hwPq = osa24Inst->hwPqInf;
	
	if(sensorStrIdx == SENSOR_STRUCT_IDX_MODBUS_INST)
		return (void *)modbusInst;
	else if(sensorStrIdx == SENSOR_STRUCT_IDX_HW_PQ)
		return (void *)hwPq;
	else if(sensorStrIdx == SENSOR_STRUCT_IDX_PROCESS_CTX)
		return (void *)processCtx;
	else if(sensorStrIdx == SENSOR_STRUCT_IDX_SENSOR_CTX)
		return (void *)osa24gCtx;
	else
		return NULL;
}
//------------------------------------------------------------------------------
static void *getVwStructInfo(DEV_INSTANCE	*devInst, enum SENSOR_STRUCT_IDX sensorStrIdx)
{	VW_SENSOR_INSTANCE *vwInst = (VW_SENSOR_INSTANCE *)devInst->extDevInstPtr;
	VW_SENSOR_CONTEXT	*vwCtx = vwInst->vwCtx;
	MODBUS_INSTANCE	*modbusInst = vwCtx->modbusInst;
	SENSOR_PROCESS_CONTEXT *processCtx = &vwCtx->processCtx;
	SENSOR_HW_PQ_STRUCT	*hwPq = vwInst->hwPqInf;
	
	if(sensorStrIdx == SENSOR_STRUCT_IDX_MODBUS_INST)
		return (void *)modbusInst;
	else if(sensorStrIdx == SENSOR_STRUCT_IDX_HW_PQ)
		return (void *)hwPq;
	else  if(sensorStrIdx == SENSOR_STRUCT_IDX_PROCESS_CTX)
		return (void *)processCtx;
	else
		return NULL;
}
//------------------------------------------------------------------------------
static void *getCompositeStructInfo(DEV_INSTANCE	*devInst, enum SENSOR_STRUCT_IDX sensorStrIdx)
{	COMPOSITE_SENSOR_INSTANCE *compositeInst = (COMPOSITE_SENSOR_INSTANCE *)devInst->extDevInstPtr;
	COMPOSITE_SENSOR_CONTEXT	*compositeCtx = compositeInst->compositeCtx;
	MODBUS_INSTANCE	*modbusInst = compositeCtx->modbusInst;
	SENSOR_PROCESS_CONTEXT *processCtx = &compositeCtx->processCtx;
	SENSOR_HW_PQ_STRUCT	*hwPq = compositeInst->hwPqInf;
	
	if(sensorStrIdx == SENSOR_STRUCT_IDX_MODBUS_INST)
		return (void *)modbusInst;
	else if(sensorStrIdx == SENSOR_STRUCT_IDX_HW_PQ)
		return (void *)hwPq;
	else  if(sensorStrIdx == SENSOR_STRUCT_IDX_PROCESS_CTX)
		return (void *)processCtx;
	else
		return NULL;
}
//------------------------------------------------------------------------------
MODBUS_INSTANCE	*getModbusInst(DEV_INSTANCE	*devInst)
{	MODBUS_INSTANCE *modbusInst = NULL;
	
	if(devInst->extDevInstPtr == NULL)
		return modbusInst;
	switch(devInst->extDevType)
	{	case EXT_DEV_TYPE_MODBUS_NORMAL:			modbusInst = (MODBUS_INSTANCE *)getNormalStructInfo(devInst, SENSOR_STRUCT_IDX_MODBUS_INST);		break;
		case EXT_DEV_TYPE_MODBUS_OSA24G:			modbusInst = (MODBUS_INSTANCE *)getOsaStructInfo(devInst, SENSOR_STRUCT_IDX_MODBUS_INST);				break;
		case EXT_DEV_TYPE_MODBUS_VW_SENSOR:			modbusInst = (MODBUS_INSTANCE *)getVwStructInfo(devInst, SENSOR_STRUCT_IDX_MODBUS_INST);				break;
		case EXT_DEV_TYPE_MODBUS_COMP_OSA_RADAR:	
		case EXT_DEV_TYPE_MODBUS_COMP_OSA_WLS:		modbusInst = (MODBUS_INSTANCE *)getCompositeStructInfo(devInst, SENSOR_STRUCT_IDX_MODBUS_INST);	break;
		default:	break;
	}
	return modbusInst;
}
//------------------------------------------------------------------------------
static int configureCoils(uint16_t startAddr, uint16_t number)
{	int size;
	modbus_mapping_t *modbusMapping = (modbus_mapping_t *)sensDev->modbusMapping;
	
	if(number < 1)
		return -1;
	
	size = sizeof(modbusMapping->tab_bits[0]) * number;
	
	modbusMapping->tab_bits = (uint8_t *)SENS_MEM_ZALLOC(size);
	if(modbusMapping->tab_bits == NULL)
	{	modbusMapping->start_bits = 0;
		modbusMapping->nb_bits = 0;
		return -1;
	}
	
	modbusMapping->start_bits = startAddr;
	modbusMapping->nb_bits = number;
	return 0;
}
//------------------------------------------------------------------------------
static int configureDiscreteInput(uint16_t startAddr, uint16_t number)
{	int size;
	modbus_mapping_t *modbusMapping = (modbus_mapping_t *)sensDev->modbusMapping;
	
	if(number < 1)
		return -1;
	
	size = sizeof(modbusMapping->tab_input_bits[0]) * number;
	
	modbusMapping->tab_input_bits = (uint8_t *)SENS_MEM_ZALLOC(size);
	if(modbusMapping->tab_input_bits == NULL)
	{	modbusMapping->start_input_bits = 0;
		modbusMapping->nb_input_bits = 0;
		return -1;
	}
	
	modbusMapping->start_input_bits = startAddr;
	modbusMapping->nb_input_bits = number;
	return 0;
}
//------------------------------------------------------------------------------
static int configureInputRegisters(uint16_t startAddr, uint16_t number)
{	int size;
	modbus_mapping_t *modbusMapping = (modbus_mapping_t *)sensDev->modbusMapping;
	
	if(number < 1)
		return -1;
	
	size = sizeof(modbusMapping->tab_input_registers[0]) * number;
	
	modbusMapping->tab_input_registers = (uint16_t *)SENS_MEM_ZALLOC(size);
	if(modbusMapping->tab_input_registers == NULL)
	{	modbusMapping->start_input_registers = 0;
		modbusMapping->nb_input_registers = 0;
		return -1;
	}
	memset(modbusMapping->tab_input_registers, 0xFF, size);
	modbusMapping->start_input_registers = startAddr;
	modbusMapping->nb_input_registers = number;
	return 0;
}
//------------------------------------------------------------------------------
static void configMappingTable(void)
{	modbus_mapping_t *modbusMapping;
	
	sensDev->modbusMapping = (modbus_mapping_t *)SENS_MEM_ZALLOC(sizeof(modbus_mapping_t));
	
	configureCoils(0, DO_WIRED_QUANTITY);
	configureDiscreteInput(0, DO_WIRED_QUANTITY);
	configureInputRegisters(0x0000, 206);
	modbusMapping = (modbus_mapping_t *)sensDev->modbusMapping;
	
	modbusMapping->is2SetsInputRegisters = 1;
	modbusMapping->start_input_registers_2 = 0x4000;	//old a4 addr
}
//------------------------------------------------------------------------------
void setModbusCoil(int channel, uint8_t value)
{	modbus_mapping_t *modbusMapping;
	
	if(sensDev->modbusServerInst)
	{	modbusMapping = (modbus_mapping_t *)sensDev->modbusMapping;
		if(channel < modbusMapping->nb_bits)
			modbusMapping->tab_bits[channel] = value;
	}
}
//------------------------------------------------------------------------------
void setModbusDiscreteInput(int channel, uint8_t value)
{	modbus_mapping_t *modbusMapping;
	
	if(sensDev->modbusServerInst)
	{	modbusMapping = (modbus_mapping_t *)sensDev->modbusMapping;
		if(channel < modbusMapping->nb_input_bits)
			modbusMapping->tab_input_bits[channel] = value;
	}
}
//------------------------------------------------------------------------------
void setModbusInputRegister16(int channel, uint16_t value)
{	modbus_mapping_t *modbusMapping;
	
	if(sensDev->modbusServerInst)
	{	modbusMapping = (modbus_mapping_t *)sensDev->modbusMapping;
		if(channel < modbusMapping->nb_input_registers)
			modbusMapping->tab_input_registers[channel] = value;
	}
}
//------------------------------------------------------------------------------
void setModbusInputRegisterFloat(int channel, float value)
{	modbus_mapping_t *modbusMapping;
	
	if(sensDev->modbusServerInst)
	{	modbusMapping = (modbus_mapping_t *)sensDev->modbusMapping;
		if(channel < modbusMapping->nb_input_registers)
			modbus_set_float_dcba(value, &modbusMapping->tab_input_registers[channel]);
	}
}
//------------------------------------------------------------------------------
void modbusInit(void)
{	DEV_INSTANCE	*devInst;
	uint8_t modbusClientDevInstall = 0;
	int rsp;
	MODBUS_INSTANCE *modbusInst;
	MODBUS_REQ_CONTEXT	*reqCtx;
	UART_CONFIG *uartCfg;
	COMM_IF_CONFIG *ifCfg;
	MODBUS_INSTANCE *servModbusInst;
	MODBUS_INSTANCE *clientModbusInst;
	
	for(devInst=sensDev->modbusDevInsts;devInst;devInst=devInst->next)
	{	if(devInst->protocol == EXT_IF_PROTO_MODBUS)
		{	switch(devInst->extDevType)
			{	case EXT_DEV_TYPE_MODBUS_NORMAL:			rsp = parserNormalSensor(devInst);		break;
				case EXT_DEV_TYPE_MODBUS_OSA24G:			rsp = parserOsa24Sensor(devInst);		break;
				case EXT_DEV_TYPE_MODBUS_VW_SENSOR:			rsp = parserVwSensor(devInst);			break;
				case EXT_DEV_TYPE_MODBUS_COMP_OSA_RADAR:
				case EXT_DEV_TYPE_MODBUS_COMP_OSA_WLS:		rsp = parserCompositeSensor(devInst);	break;
				default:	rsp = -1;
			}
			
			if(!rsp)
			{	modbusInst = getModbusInst(devInst);
				if(modbusInst)
				{	modbusClientDevInstall = 1;
					modbusInst->mode = MODBUS_MODE_CILENT;
					if(devInst->interface == EXT_IF_CHANNEL_ETH)
					{	reqCtx = &modbusInst->reqCtx;
						modbusInst->modbusCtx = (void *)modbus_new_tcp(reqCtx->ip, 502);
					}
					else
					{	if(modbusInst->channel == EXT_IF_CHANNEL_RS485_1)
							uartCfg = (UART_CONFIG *)sensDev->rs485_Comm[0];
						else if(modbusInst->channel == EXT_IF_CHANNEL_RS485_2)
							uartCfg = (UART_CONFIG *)sensDev->rs485_Comm[1];
						else if(modbusInst->channel == EXT_IF_CHANNEL_RS232)
							uartCfg = (UART_CONFIG *)sensDev->rs232_Comm;
						modbusInst->modbusCtx = (void *)modbus_new_rtu(modbusInst->channel, uartCfg->ctx, uartCfg->ctx);
					}
				}
			}
		}
	}
	
	ifCfg = getCommIfConfigByIfNo(EXT_IF_CHANNEL_RS485_1);
	if(ifCfg->protocol == EXT_IF_PROTO_TOUCH_PAD)
	{	MODBUS_RSP_CONTEXT *rspCtx;
		uartCfg = (UART_CONFIG *)sensDev->rs485_Comm[0];
		modbusInst = SENS_MEM_ZALLOC(sizeof(MODBUS_INSTANCE));
		reqCtx = &modbusInst->reqCtx;
		rspCtx = &modbusInst->rspCtx;
		modbusInst->mode = MODBUS_MODE_SERVER;
		modbusInst->channel = EXT_IF_CHANNEL_RS485_1;
		reqCtx->slaveId = 0x01;
		modbusInst->modbusCtx = (void *)modbus_new_rtu(modbusInst->channel, uartCfg->ctx, uartCfg->ctx);
		modbus_set_slave(modbusInst->modbusCtx, reqCtx->slaveId);
		configMappingTable();
		modbus_connect(modbusInst->modbusCtx);
		rspCtx->rspBuf = SENS_MEM_ALLOC(256);
		rspCtx->rspBufBak = SENS_MEM_ALLOC(256);
		sensDev->modbusServerInst = modbusInst;
	}
}
//------------------------------------------------------------------------------
static SENSOR_PROCESS_CONTEXT *getSensorProcessCtx(DEV_INSTANCE	*devInst)
{	SENSOR_PROCESS_CONTEXT *processCtx = NULL;
	
	if(devInst->extDevInstPtr == NULL)
		return processCtx;
	switch(devInst->extDevType)
	{	case EXT_DEV_TYPE_MODBUS_NORMAL:			processCtx = (SENSOR_PROCESS_CONTEXT *)getNormalStructInfo(devInst, SENSOR_STRUCT_IDX_PROCESS_CTX);		break;
		case EXT_DEV_TYPE_MODBUS_OSA24G:			processCtx = (SENSOR_PROCESS_CONTEXT *)getOsaStructInfo(devInst, SENSOR_STRUCT_IDX_PROCESS_CTX);		break;
		case EXT_DEV_TYPE_MODBUS_VW_SENSOR:			processCtx = (SENSOR_PROCESS_CONTEXT *)getVwStructInfo(devInst, SENSOR_STRUCT_IDX_PROCESS_CTX);			break;
		case EXT_DEV_TYPE_MODBUS_COMP_OSA_RADAR:	
		case EXT_DEV_TYPE_MODBUS_COMP_OSA_WLS:		processCtx = (SENSOR_PROCESS_CONTEXT *)getCompositeStructInfo(devInst, SENSOR_STRUCT_IDX_PROCESS_CTX);	break;
		default:	break;
	}
	return processCtx;
}
//------------------------------------------------------------------------------
static SENSOR_HW_PQ_STRUCT *getSensorHwPqCtx(DEV_INSTANCE	*devInst)
{	SENSOR_HW_PQ_STRUCT *hwPqInf = NULL;
	
	if(devInst->extDevInstPtr == NULL)
		return hwPqInf;
	switch(devInst->extDevType)
	{	case EXT_DEV_TYPE_MODBUS_NORMAL:			hwPqInf = (SENSOR_HW_PQ_STRUCT *)getNormalStructInfo(devInst, SENSOR_STRUCT_IDX_HW_PQ);		break;
		case EXT_DEV_TYPE_MODBUS_OSA24G:			hwPqInf = (SENSOR_HW_PQ_STRUCT *)getOsaStructInfo(devInst, SENSOR_STRUCT_IDX_HW_PQ);		break;
		case EXT_DEV_TYPE_MODBUS_VW_SENSOR:			hwPqInf = (SENSOR_HW_PQ_STRUCT *)getVwStructInfo(devInst, SENSOR_STRUCT_IDX_HW_PQ);			break;
		case EXT_DEV_TYPE_MODBUS_COMP_OSA_RADAR:	
		case EXT_DEV_TYPE_MODBUS_COMP_OSA_WLS:		hwPqInf = (SENSOR_HW_PQ_STRUCT *)getCompositeStructInfo(devInst, SENSOR_STRUCT_IDX_HW_PQ);	break;
		default:	break;
	}
	return hwPqInf;
}
//------------------------------------------------------------------------------
static int getOsaPwrSrc(DEV_INSTANCE	*devInst)
{	OSA24G_INSTANCE *osa24Inst = (OSA24G_INSTANCE *)devInst->extDevInstPtr;
	OSA24G_CONFIG	*osa24gCfg = (OSA24G_CONFIG *)osa24Inst->osa24gCfgPtr;
	int pwrSrc = DEVICE_PWR_SRC_EXT;
	
	if(osa24gCfg)
		pwrSrc = osa24gCfg->pwrSrc;
	return pwrSrc;
}
//------------------------------------------------------------------------------
static int getVwPwrSrc(DEV_INSTANCE	*devInst)
{	VW_SENSOR_INSTANCE *vwInst = (VW_SENSOR_INSTANCE *)devInst->extDevInstPtr;
	VW_SENSOR_CONFIG	*vwCfg = vwInst->vwCfgPtr;
	int pwrSrc = DEVICE_PWR_SRC_EXT;
	
	if(vwCfg)
		pwrSrc = vwCfg->pwrSrc;
	return pwrSrc;
}
//------------------------------------------------------------------------------
static int getCompositePwrSrc(DEV_INSTANCE	*devInst)
{	COMPOSITE_SENSOR_INSTANCE *compositeInst = (COMPOSITE_SENSOR_INSTANCE *)devInst->extDevInstPtr;
	COMPOSITE_OSA_CONFIG *compositeCfg = compositeInst->compositeCfgPtr;
	int pwrSrc = DEVICE_PWR_SRC_EXT;
	
	if(compositeInst->isRadarDev)
		pwrSrc = compositeCfg->radarPwrSrc;
	else
		pwrSrc = compositeCfg->wlsPwrSrc;
	return pwrSrc;
}
//------------------------------------------------------------------------------
static int getSensorPwrSrc(DEV_INSTANCE	*devInst)
{	int pwrSrc = DEVICE_PWR_SRC_EXT;
	
	if(devInst->extDevInstPtr == NULL)
		return pwrSrc;
	switch(devInst->extDevType)
	{	case EXT_DEV_TYPE_MODBUS_NORMAL:			pwrSrc = DEVICE_PWR_SRC_EXT;		break;
		case EXT_DEV_TYPE_MODBUS_OSA24G:			pwrSrc = getOsaPwrSrc(devInst);				break;
		case EXT_DEV_TYPE_MODBUS_VW_SENSOR:			pwrSrc = getVwPwrSrc(devInst);					break;
		case EXT_DEV_TYPE_MODBUS_COMP_OSA_RADAR:	
		case EXT_DEV_TYPE_MODBUS_COMP_OSA_WLS:		pwrSrc = getCompositePwrSrc(devInst);	break;
		default:	break; 
	}
	return pwrSrc;
}
//------------------------------------------------------------------------------
int modbusRequest(MODBUS_INSTANCE *modbusInst, int send)
{	int valueSize;
	int result = -1;
	MODBUS_REQ_CONTEXT *reqCtx = &modbusInst->reqCtx;
	MODBUS_RSP_CONTEXT *rspCtx = &modbusInst->rspCtx;
	
	if(send)
	{	if((reqCtx->funcCode < MODBUS_FC_READ_COILS) || (reqCtx->funcCode > MODBUS_FC_WRITE_MULTIPLE_REGISTERS))
			return MODBUS_ERR_ARGS_ERR;
		if(reqCtx->readLength <= 0)
			return MODBUS_ERR_ARGS_ERR;
	
		modbus_set_slave((modbus_t *)modbusInst->modbusCtx, reqCtx->slaveId);
	
		/*if((rspCtx->rspBuf == NULL) && (reqCtx->funcCode <= MODBUS_FC_READ_INPUT_REGISTERS))
		{	valueSize = (reqCtx->funcCode < MODBUS_FC_READ_HOLDING_REGISTERS)? sizeof(uint8_t):sizeof(uint16_t);
			rspCtx->rspBuf = SENS_MEM_ZALLOC(modbusInst->readLength*valueSize);
		}*/
    /*else if((modbusInst->dataBuf == NULL) && ((modbusInst->funcCode == MODBUS_FC_WRITE_SINGLE_COIL) || (modbusInst->funcCode == MODBUS_FC_WRITE_SINGLE_REGISTER)))
    { 
    }*/
	}
	switch(reqCtx->funcCode)
	{	case MODBUS_FC_READ_COILS:
					{	result = modbus_read_bits((modbus_t *)modbusInst->modbusCtx, reqCtx->regNo, reqCtx->readLength, rspCtx->rspBuf, send);
					}
					break;
		case MODBUS_FC_READ_DISCRETE_INPUTS:
					{	result = modbus_read_input_bits((modbus_t *)modbusInst->modbusCtx, reqCtx->regNo, reqCtx->readLength, rspCtx->rspBuf, send);
					}
					break;
		case MODBUS_FC_READ_HOLDING_REGISTERS:
					{	result = modbus_read_registers((modbus_t *)modbusInst->modbusCtx, reqCtx->regNo, reqCtx->readLength, (uint16_t *)rspCtx->rspBuf, send);
					}
					break;
		case MODBUS_FC_READ_INPUT_REGISTERS:
					{	result = modbus_read_input_registers((modbus_t *)modbusInst->modbusCtx, reqCtx->regNo, reqCtx->readLength, (uint16_t *)rspCtx->rspBuf, send);
					}
					break;
		case MODBUS_FC_WRITE_SINGLE_COIL:
					{	result = modbus_write_bit((modbus_t *)modbusInst->modbusCtx, reqCtx->regNo, reqCtx->readLength, send);
					}
					break;
		case MODBUS_FC_WRITE_SINGLE_REGISTER:
					{	result = modbus_write_register((modbus_t *)modbusInst->modbusCtx, reqCtx->regNo, reqCtx->readLength, send);
					}
					break;
		/*case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
					{	result = modbus_write_registers((modbus_t *)modbusInst->modbusCtx, reqCtx->regNo, reqCtx->readLength, reqCtx->writeBuf, send);
					}*/
		default:
					break;
	}
	
	if(result == -1)
		return 0;
	else if(result == -2)
		return -2;
	return reqCtx->readLength;
}
//------------------------------------------------------------------------------
static int modbusClientProcess(MODBUS_INSTANCE *modbusInst/*, int &processDone*/)
{	MODBUS_REQ_CONTEXT *reqCtx = &modbusInst->reqCtx;
	MODBUS_RSP_CONTEXT *rspCtx = &modbusInst->rspCtx;
	int finish = -1;
	
	//*processDone = 0;
	switch(modbusInst->fsm)
	{	case MODBUS_RUN_FSM_IDLE:
					modbusInst->fsm = MODBUS_RUN_FSM_PREPARE;
		case MODBUS_RUN_FSM_PREPARE:
					{	//modbus_set_slave((modbus_t *)modbusInst->modbusCtx, reqCtx->slaveId);
						modbus_set_response_timeout((modbus_t *)modbusInst->modbusCtx, 0, 0);	//non block
						modbus_connect((modbus_t *)modbusInst->modbusCtx);
						//modbus_set_debug((modbus_t *)modbusInst->modbusCtx, 1);
						modbusInst->fsm = MODBUS_RUN_FSM_SEND;
						rspCtx->isDataGet = 0;
						rspCtx->isRspTimeout = 0;
						//
						reqCtx->rspTimer = GetTimeTicks();
						reqCtx->rspTimeout = 2000;
					}
		case MODBUS_RUN_FSM_SEND:
					{	int err;
						err = modbusRequest(modbusInst, 1);
						if(err == -2)
						{	modbusInst->fsm = MODBUS_RUN_FSM_RECV;
							break;
						}
						else if(err <= 0)
						{	modbusInst->fsm = MODBUS_RUN_FSM_RECV;
							break;
						}
						else
						{	modbusInst->fsm = MODBUS_RUN_FSM_DONE;
							rspCtx->isDataGet = 1;
							break;
						}
					}
		case MODBUS_RUN_FSM_RECV:
					{	int rc;
						rc = modbusRequest(modbusInst, 0);
						if(rc <= 0)
						{ if((GetTimeTicks() - reqCtx->rspTimer) > reqCtx->rspTimeout)
							{	modbusInst->fsm = MODBUS_RUN_FSM_DONE;
								rspCtx->isRspTimeout = 1;
								break;
							}
						}
						if(rc > 0)
						{	rspCtx->isDataGet = 1;
							modbusInst->fsm = MODBUS_RUN_FSM_DONE;
						}
					}
					break;
		case MODBUS_RUN_FSM_DONE:
					//collectData(modbusInst);
					modbus_close((modbus_t *)modbusInst->modbusCtx);
					modbusInst->fsm = MODBUS_RUN_FSM_IDLE;
					finish = 0;
					//*processDone = 1;
					break;
		default:
					break;
	}
	return finish;
}
//------------------------------------------------------------------------------
static int osa24FilterPqToFinalPq(DEV_INSTANCE *devInst, SENSOR_HW_PQ_STRUCT *hwPqInf)
{	float finalPqVal;
	int newRec = 0;
	OSA24G_REC_INFO *osa24RecInfo = NULL;
	OSA24G_CONTEXT *osa24Ctx = (OSA24G_CONTEXT *)getOsaStructInfo(devInst, SENSOR_STRUCT_IDX_SENSOR_CTX);
	
	if(chkFileExist("osa24.bin"))
	{	osa24RecInfo = (OSA24G_REC_INFO *)SENS_MEM_ZALLOC(sizeof(OSA24G_REC_INFO));
		sdReadFile("osa24.bin", (char *)osa24RecInfo, sizeof(OSA24G_REC_INFO), 0, NORMAL_READ_MODE);
	}
	
	if(hwPqInf->filterPqVal <= 0)
	{	if(osa24RecInfo == NULL)
			hwPqInf->filterPqVal = SENS_SET_VAL_NAN();
		else
		{	if(hwPqInf->hwPqIdx == 0)	//Filter
			{	if((sysTimer->currentTimeStamp - osa24RecInfo->recFilterTimestamp) < (12*60*60))
				{	hwPqInf->filterPqVal = osa24RecInfo->recFilterPqVal;
				}
				else
				{	hwPqInf->filterPqVal = SENS_SET_VAL_NAN();
				}
			}
			else
			{	if((sysTimer->currentTimeStamp - osa24RecInfo->recRealTimestamp) < (12*60*60))
				{	hwPqInf->filterPqVal = osa24RecInfo->recRealPqVal;
				}
				else
				{	hwPqInf->filterPqVal = SENS_SET_VAL_NAN();
				}
			}
		}
	}
	else
	{	newRec = 1;
		if(osa24RecInfo == NULL)
			osa24RecInfo = (OSA24G_REC_INFO *)SENS_MEM_ZALLOC(sizeof(OSA24G_REC_INFO));
		if(hwPqInf->hwPqIdx == 0)	//Filter
		{	osa24RecInfo->recFilterTimestamp = sysTimer->currentTimeStamp;
			osa24RecInfo->recFilterPqVal = hwPqInf->filterPqVal;
		}
		else
		{	osa24RecInfo->recRealTimestamp = sysTimer->currentTimeStamp;
			osa24RecInfo->recRealPqVal = hwPqInf->filterPqVal;
		}
	}
	
	if(newRec)
		sdWriteFile("osa24.bin", (char*)osa24RecInfo, sizeof(OSA24G_REC_INFO), 0, OVER_WRITE_MODE);
	
	//if(osa24Ctx->minDistance && hwPqInf->filterPqVal < osa24Ctx->minDistance)
	//	finalPqVal = osa24Ctx->minDistance;
	
	if(osa24Ctx->maxDistance)	//IH
	{	if(!isnan(hwPqInf->filterPqVal))
			finalPqVal = osa24Ctx->maxDistance - hwPqInf->filterPqVal;
		else
			finalPqVal = SENS_SET_VAL_NAN();
	}
	else
		finalPqVal = hwPqInf->filterPqVal;
	
	hwPqInf->pqVal = finalPqVal;
	hwPqInf->isHwPqReady = 1;
	return 0;
}
//------------------------------------------------------------------------------
static int filterPqToFinalPq(DEV_INSTANCE *devInst, SENSOR_HW_PQ_STRUCT *hwPqInf)
{	if(devInst->extDevType == EXT_DEV_TYPE_MODBUS_OSA24G)
	{	osa24FilterPqToFinalPq(devInst, hwPqInf);
	}
	/*else if(devInst->extDevType == EXT_DEV_TYPE_MODBUS_COMP_OSA_RADAR)
	{
	}
	else if(devInst->extDevType == EXT_DEV_TYPE_MODBUS_COMP_OSA_WLS)
	{
	}*/
	else
	{	hwPqInf->pqVal = hwPqInf->filterPqVal;
		hwPqInf->isHwPqReady = 1;
	}
	return 0;
}
//------------------------------------------------------------------------------
static int getHwPq(DEV_INSTANCE	*devInst, MODBUS_INSTANCE *modbusInst, SENSOR_HW_PQ_STRUCT *topHwPqInf, int skipZeroVal)
{	MODBUS_REQ_CONTEXT *reqCtx = &modbusInst->reqCtx;
	MODBUS_RSP_CONTEXT *rspCtx = &modbusInst->rspCtx;
	int result = -1;
	SENSOR_HW_PQ_STRUCT *hwPqInf;
	int pqPos;
	
	
	if(rspCtx->isDataGet)
	{	rspCtx->rspTimeoutCnt = 0;
	}
	else
	{	if(rspCtx->rspTimeoutCnt != 0xFF)
			rspCtx->rspTimeoutCnt++;
		if(rspCtx->rspTimeoutCnt >= 3)	//set ai to NaN once
		{	goto SET_SW_AI_TO_NAN;
		}
		return result;
	}
	
	result = 0;
	
	for(hwPqInf=topHwPqInf, pqPos=0;hwPqInf;hwPqInf=hwPqInf->next, pqPos++)
	{	switch(reqCtx->dataType)
		{	case MODBUS_DT_BOOL:
						{	uint8_t boolVal;
							uint8_t *array;
              	
							array = &rspCtx->rspBuf[pqPos];
							boolVal = *array;
							hwPqInf->rawPqVal = (float)boolVal;
							hwPqInf->isRawHwPqReady = 1;
						}
						break;
			case MODBUS_DT_FLOAT:
						{	float value;
							uint16_t *array;
							array = (uint16_t *)&rspCtx->rspBuf[pqPos*4];
							if(reqCtx->dataOrder == MODBUS_DO_dcba)                    // LL LH HL HH
								value = modbus_get_float_dcba(array);
							else if(reqCtx->dataOrder == MODBUS_DO_cdab)                    // LH LL HH HL
								value = modbus_get_float_cdab(array);
							else if(reqCtx->dataOrder == MODBUS_DO_badc)                    // HL HH LL LH
								value = modbus_get_float_badc(array);
							else if(reqCtx->dataOrder == MODBUS_DO_abcd)                    // HH HL LH LL
								value = modbus_get_float_abcd(array);
							hwPqInf->rawPqVal = value;
							hwPqInf->isRawHwPqReady = 1;							
						}
						break;
			case MODBUS_DT_INT16:
			case MODBUS_DT_UINT16:
						{	uint16_t u16Ai = 0;
							uint16_t *array;
							float filterVal;

							array = (uint16_t *)&rspCtx->rspBuf[pqPos*2];
							u16Ai = *array;	//note, default is BIG
							if(reqCtx->dataOrder == MODBUS_DO_ba)
								u16Ai = bswap_16(u16Ai);
							if(reqCtx->dataType == MODBUS_DT_INT16)	
								filterVal = (float)((int16_t)u16Ai);
							else
								filterVal = (float)u16Ai;
							hwPqInf->rawPqVal = filterVal;
							hwPqInf->isRawHwPqReady = 1;
						}
						break;
			case MODBUS_DT_INT32:
			case MODBUS_DT_UINT32:	
						{	uint32_t u32Ai;
							uint16_t *array;
							float filterVal;
							array = (uint16_t *)&rspCtx->rspBuf[pqPos*4];
							if(reqCtx->dataOrder == MODBUS_DO_dcba)				// LL LH HL HH
								u32Ai = modbus_get_uint32_dcba(array);
							else if(reqCtx->dataOrder == MODBUS_DO_cdab)		// LH LL HH HL
								u32Ai = modbus_get_uint32_cdab(array);
							else if(reqCtx->dataOrder == MODBUS_DO_badc)		// HL HH LL LH
								u32Ai = modbus_get_uint32_badc(array);
							else if(reqCtx->dataOrder == MODBUS_DO_abcd)		// HH HL LH LL
								u32Ai = modbus_get_uint32_abcd(array);
						
							if(reqCtx->dataType == MODBUS_DT_INT32)
								filterVal = (float)((int32_t)u32Ai);
							else
								filterVal = (float)u32Ai;
								
							hwPqInf->rawPqVal = filterVal;
							if(skipZeroVal && (u32Ai == 0))
							{	hwPqInf->isRawHwPqReady = 0;
								hwPqInf->emptyPqCnt++;
								if(hwPqInf->prevFilterAlgo && (hwPqInf->emptyPqCnt > SPECIALL_MODBUS_SENSOR_FILTER_MAX_COUNT))
								{	hwPqInf->emptyPqError = 1;
									hwPqInf->isRawHwPqReady = 1;
									//hwPqInf->rawPqVal = NAN;
								}
							}
							else
							{	hwPqInf->isRawHwPqReady = 1;
							}
						}
						break;
			default:
						break;
		}
	}
	
SET_SW_AI_TO_NAN:
	if(result)
	{	for(hwPqInf=topHwPqInf;hwPqInf;hwPqInf=hwPqInf->next)
		{	hwPqInf->rawPqVal = SENS_SET_VAL_NAN();
			hwPqInf->isRawHwPqReady = 1;
		}
	}
	
	for(hwPqInf=topHwPqInf;hwPqInf;hwPqInf=hwPqInf->next)
	{	getHwPqWithFilter(hwPqInf);
	}
	
	result = 0;
	for(hwPqInf=topHwPqInf;hwPqInf;hwPqInf=hwPqInf->next)
	{	if(hwPqInf->isHwFilterPqRdy && !hwPqInf->isHwPqReady)
		{	//hwPqInf->isHwFilterPqRdy = 0;
			filterPqToFinalPq(devInst, hwPqInf);
			//hwPqInf->pqVal = hwPqInf->filterPqVal;
			//hwPqInf->isHwPqReady = 1;
		}
		else if(!hwPqInf->isHwPqReady)
			result = -1;
	}
	
	return result;
}
//------------------------------------------------------------------------------
static int processModbusSensor(DEV_INSTANCE	*devInst, int *findNext)
{	SENSOR_HW_PQ_STRUCT		*hwPqInf = NULL;	//= getSensorHwPqCtx(devInst);
	MODBUS_INSTANCE *modbusInst = NULL;	// = getModbusInst(devInst);
	SENSOR_PROCESS_CONTEXT *processCtx = getSensorProcessCtx(devInst);
	int pwrSrc = getSensorPwrSrc(devInst);
	int pwrCtrl;
	int startTimeTicket = GetTimeTicks();
	
	if((sysCfg->sensSysCfg.psm == SYS_CFG_PSM_ADVANCE) && (pwrSrc != DEVICE_PWR_SRC_EXT) && (pwrSrc != DEVICE_PWR_SRC_MAX))
		pwrCtrl = 1;
	else
		pwrCtrl = 0;
	
	switch(processCtx->fsm)
	{	case MODBUS_DEV_FSM_IDLE:
					processCtx->activeWaitTimeout = 0;
					processCtx->activeWaitTimer = startTimeTicket;
					if(pwrCtrl)
					{	uint32_t nextRecTimeSec;
						int nextRecTimeGap;// = 30;
						uint32_t pollingInterval;
						
						nextRecTimeGap = processCtx->pwrOnDelayTimeout / 1000 + 15;	//15 sec for read data timer
						if(nextRecTimeGap < 30)
							nextRecTimeGap = 30;
						
						if(processCtx->isNotFirst)
							pollingInterval = (processCtx->pollingInterval == DEVICE_POLLING_INTERVAL_1_MIN)? 60:((processCtx->pollingInterval == DEVICE_POLLING_INTERVAL_REC_TIME)? sysCfg->sensSysCfg.recordSec:1);
						else
						{	pollingInterval = 60;
							processCtx->isNotFirst = 1;
						}
						
						nextRecTimeSec = ROUNDUP(sysTimer->currTimeSec, pollingInterval);
						if(nextRecTimeSec == sysTimer->currTimeSec)
							nextRecTimeSec += pollingInterval;
						if((nextRecTimeSec - sysTimer->currTimeSec) < nextRecTimeGap)
						{	dbg_msg("[SENSOR] sensor check timer less %d(%d) sec, run immediate!!\r\n", nextRecTimeGap, (nextRecTimeSec - sysTimer->currTimeSec));
						}
						else
						{	processCtx->activeWaitTimeout = ((pollingInterval-nextRecTimeGap)-(sysTimer->currTimeSec % pollingInterval))*1000;
							dbg_msg("[SENSOR] set next sensor wakeup timer %d\r\n", processCtx->activeWaitTimeout / 1000);
						}
					}
		case MODBUS_DEV_FSM_INIT:
					processCtx->fsm = MODBUS_DEV_FSM_INIT;
					if(processCtx->activeWaitTimeout)
					{	if((startTimeTicket - processCtx->activeWaitTimer) > processCtx->activeWaitTimeout)
						{	processCtx->fsm = MODBUS_DEV_FSM_PWR_ON;
						}
						else
							break;
					}
		case MODBUS_DEV_FSM_PWR_ON:
					setOutputPower(pwrSrc, 1, __func__, __LINE__);
					if(devInst->interface == EXT_IF_CHANNEL_RS485_1)
					{	IO_PWR_CTL *rs485PwrCtrl = (IO_PWR_CTL *)&sensDev->rs485PwrCtrl[EXT_IF_CHANNEL_RS485_1];
						if(pwrCtrl)
							setRs485Pwr(devInst->interface, 1);
						else if(!rs485PwrCtrl->powerEnableCount)
							setRs485Pwr(devInst->interface, 1);
					}
					else if(devInst->interface == EXT_IF_CHANNEL_RS485_2)
					{	IO_PWR_CTL *rs485PwrCtrl = (IO_PWR_CTL *)&sensDev->rs485PwrCtrl[EXT_IF_CHANNEL_RS485_2];
						if(pwrCtrl)
							setRs485Pwr(devInst->interface, 1);
						else if(!rs485PwrCtrl->powerEnableCount)
							setRs485Pwr(devInst->interface, 1);
					}
#if TEST_N_REMOVE
					else if(devInst->interface == EXT_IF_CHANNEL_RS232)
					{	if(pwrCtrl)
							rs232PwrCtrl(1);
						else if(!sensDev->extIfPwrOnCount[EXT_DEV_CHAN_RS232])
							rs232PwrCtrl(1);
					}
#endif
					processCtx->pwrOnDelayTimer = startTimeTicket;
					processCtx->fsm = MODBUS_DEV_FSM_PWR_ON_WAIT;
		case MODBUS_DEV_FSM_PWR_ON_WAIT:
					if((startTimeTicket - processCtx->pwrOnDelayTimer) > processCtx->pwrOnDelayTimeout)
					{	dbg_msg("[SENSOR] sensor wait power on delay timer(%d) done \r\n", processCtx->pwrOnDelayTimeout);
						processCtx->pollingTimer = startTimeTicket;
						processCtx->fsm = MODBUS_DEV_FSM_POLLING_WAIT;
					}
					else
						break;
		case MODBUS_DEV_FSM_POLLING_WAIT:
					if((startTimeTicket - processCtx->pollingTimer) > processCtx->pollingTimeout)
					{	//dbg_msg("[SENSOR] sensor wait power on delat timer(%d) done \r\n", processCtx->pwrOnDelayTimeout);
						processCtx->fsm = MODBUS_DEV_FSM_PROCESS;
					}
					else
						break;
		case MODBUS_DEV_FSM_PROCESS:
					*findNext = 0;
					modbusInst = getModbusInst(devInst);
					if(!modbusClientProcess(modbusInst))
						processCtx->fsm = MODBUS_DEV_FSM_GET_HW_PQ;
					else
						break;
		case MODBUS_DEV_FSM_GET_HW_PQ:
					{	int result;
						
						if(hwPqInf == NULL)
							hwPqInf = getSensorHwPqCtx(devInst);
						*findNext = 1;
						result = getHwPq(devInst, modbusInst, hwPqInf, processCtx->skipZeroValue);
						if(result)
						{	processCtx->pollingTimer = startTimeTicket;
							processCtx->fsm = MODBUS_DEV_FSM_POLLING_WAIT;
							break;
						}
						else
							processCtx->fsm = MODBUS_DEV_FSM_SET_SW_PQ;
					}
		case MODBUS_DEV_FSM_SET_SW_PQ:
					if(hwPqInf == NULL)
						hwPqInf = getSensorHwPqCtx(devInst);
					setSwPq(devInst, hwPqInf);
					processCtx->fsm = MODBUS_DEV_FSM_PWR_OFF;
		case MODBUS_DEV_FSM_PWR_OFF:
					if(pwrCtrl)
					{	uint32_t currTimeSec;
						uint32_t pollingInterval = (processCtx->pollingInterval == DEVICE_POLLING_INTERVAL_1_MIN)? 60:((processCtx->pollingInterval == DEVICE_POLLING_INTERVAL_REC_TIME)? sysCfg->sensSysCfg.recordSec:1);
						
						currTimeSec = sysTimer->currTimeSec % pollingInterval;
						
						setOutputPower(pwrSrc, 0, __func__, __LINE__);
#if TEST_N_REMOVE
						if(devInst->interface == EXT_DEV_CHAN_RS485_1)
							rs485PwrCtrl1(0);
						else if(devInst->interface == EXT_DEV_CHAN_RS485_2)
							rs485PwrCtrl2(0);
						else if(devInst->interface == EXT_DEV_CHAN_RS232)
							rs232PwrCtrl(0);							
#endif
						processCtx->pwrOffDelayTimer = startTimeTicket;
						if(currTimeSec >= 30)
							processCtx->pwrOffDelayTimeout = (pollingInterval - currTimeSec)*1000 + 2000;
						else
						{	if(currTimeSec <= 28)
								processCtx->pwrOffDelayTimeout = 2000;
							else
								processCtx->pwrOffDelayTimeout = (30-currTimeSec)*1000;
						}
					}
					else
						processCtx->pwrOffDelayTimeout = 0;
					processCtx->fsm = MODBUS_DEV_FSM_PWR_OFF_WAIT;
		case MODBUS_DEV_FSM_PWR_OFF_WAIT:
					if(processCtx->pwrOffDelayTimeout)
					{	if((startTimeTicket - processCtx->pwrOffDelayTimer) > processCtx->pwrOffDelayTimeout)
							processCtx->fsm = MODBUS_DEV_FSM_IDLE;
					}
					else
					{	processCtx->pollingTimer = startTimeTicket;
						processCtx->fsm = MODBUS_DEV_FSM_POLLING_WAIT;
					}
					break;
		case MODBUS_DEV_FSM_MAX:
					break;
	}
	return 0;
}
//------------------------------------------------------------------------------
DEV_INSTANCE *extDevModbusProcess(DEV_INSTANCE	*devInst, int findNext)
{	DEV_INSTANCE	*nextDevInst = devInst;
	if((devInst->protocol == EXT_IF_PROTO_MODBUS) && (devInst->instIsValid))
	{	processModbusSensor(devInst, &findNext);
	}
	
	if(findNext)
	{	if(devInst->next)
			nextDevInst = devInst->next;
		else
			nextDevInst = sensDev->modbusDevInsts;
	}
	return nextDevInst;
}
//------------------------------------------------------------------------------
DEV_INSTANCE *modbusProcess(DEV_INSTANCE *devInst)
{	MODBUS_INSTANCE *modbusServerInst = sensDev->modbusServerInst;
	DEV_INSTANCE *retDevInst = NULL;
	
	
	if(modbusServerInst)
	{	MODBUS_RSP_CONTEXT *rspCtx = &modbusServerInst->rspCtx;
		MODBUS_REQ_CONTEXT *reqCtx = &modbusServerInst->reqCtx;
		reqCtx->readLength = modbus_receive((modbus_t *)modbusServerInst->modbusCtx, rspCtx->rspBuf);
		if((int16_t)reqCtx->readLength > 0)
		{	//servModbusInst->readLengthBak = servModbusInst->readLength;
			memcpy(rspCtx->rspBufBak, rspCtx->rspBuf, reqCtx->readLength);
			modbus_reply((modbus_t *)modbusServerInst->modbusCtx, rspCtx->rspBuf, reqCtx->readLength, (modbus_mapping_t *)sensDev->modbusMapping);
			//modbusParserCmd();	//process write protocol
		}
	}
	
	if(devInst)
		retDevInst = extDevModbusProcess(devInst, 1);
	
	return retDevInst;
}
