#ifndef __I2C_EEPROM_H__
#define __I2C_EEPROM_H__
#include "sensminiCfg.h"

extern int i2cEepromReadData(uint8_t regAddr, uint8_t *dataBuf, uint16_t len);
extern int i2cEepromWriteData(uint8_t regAddr, uint8_t *dataBuf, uint16_t len);
extern void i2cEepromRWTest(void);
extern void i2cEepromRWTestPlan2(void);

#endif //__I2C_EEPROM_H__