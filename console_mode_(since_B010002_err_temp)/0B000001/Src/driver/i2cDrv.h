#ifndef __I2C_DRIVER_H__
#define __I2C_DRIVER_H__

#include "sensminiCfg.h"

#define I2C_READ_MODE_FIRST_ISSUE_SLA_W		0
#define I2C_READ_MODE_FIRST_ISSUE_SLA_R		1


extern int i2cInit(uint8_t ch, uint32_t speed, uint32_t bufferSize);
extern int i2cDeInit(uint8_t ch);
extern void setI2cIsWordsizeAddressing(uint8_t ch, uint8_t isWordSize);
extern int i2cWrite(uint8_t ch, uint8_t sla, uint16_t address, uint8_t *data, uint32_t len);
extern int i2cRead(uint8_t ch, uint8_t sla, uint16_t address, uint8_t *data, uint32_t len, uint8_t readMode);




#endif

