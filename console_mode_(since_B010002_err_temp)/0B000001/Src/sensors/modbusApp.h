#ifndef _MODBUS_APP_H__
#define _MODBUS_APP_H__

#include "sensminiCfg.h"


#define MODBUS_ERR_OK			0
#define MODBUS_ERR_ARGS_ERR		-3

extern void modbusInit(void);
extern DEV_INSTANCE *modbusProcess(DEV_INSTANCE *devInst);
extern void setModbusCoil(int channel, uint8_t value);
extern void setModbusDiscreteInput(int channel, uint8_t value);
extern void setModbusInputRegister16(int channel, uint16_t value);
extern void setModbusInputRegisterFloat(int channel, float value);



#endif
