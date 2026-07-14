#include "sensCommon.h"

#include "driver/i2cDrv.h"
#include "driver/i2cEeprom.h"

/**************************************************************************
 *********              Program Define                            *********
 **************************************************************************/
#define I2C_EEPROM_SLA				0x53  // 24AA024A Slave Address

/**************************************************************************
 *********              Program Controller Function               *********
 **************************************************************************/
int i2cEepromReadData(uint8_t regAddr, uint8_t *dataBuf, uint16_t len)
{	return i2cRead(I2C2_ID, I2C_EEPROM_SLA, regAddr, dataBuf, len, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
}

int i2cEepromWriteData(uint8_t regAddr, uint8_t *dataBuf, uint16_t len)
{	int ret;
  if(len == 0)
    return -1;
  
  PIN_BOARD_ID_WP = 0; // disable write-protection
  if(len > 16)
  { for(uint16_t offset = 0; offset < len; offset +=16)
    { uint16_t writeLen = ((len - offset) > 16) ? 16 : (len - offset);
      ret = i2cWrite(I2C2_ID, I2C_EEPROM_SLA, regAddr + offset, &dataBuf[offset], writeLen);
      if(ret < 0)
        return ret;
      SENS_TIME_DELAY(5); //EEPROM write cycle time
    }
  }
  else
  { ret = i2cWrite(I2C2_ID, I2C_EEPROM_SLA, regAddr, dataBuf, len);
  }
  PIN_BOARD_ID_WP = 1; // enable write-protection
  return ret;
}

float toggleEEP = false;
static uint32_t testCount = 0, errorCount = 0;
void i2cEepromRWTest(void)
{	uint8_t testBufW;
  uint8_t testBufR;
  uint8_t addr;
  int ret, idx;

  if(toggleEEP == false)
  { toggleEEP = true;
    for(idx = 0; idx < 256; idx++)
      testBufW = 0x55;
  }
  else
  { toggleEEP = false;
    for(idx = 0; idx < 256; idx++)
      testBufW = 0xAA;
  }
  
  addr = rand() % 256;
  printf("%s", "========================= I2C EEPROM ==========================\n");
  dbgMsg("I2C EEPROM Read/Write Data: 0x%X\r\n", testBufW);
  dbgMsg("I2C EEPROM Random Address: 0x%X\r\n", addr);
  
  ret = i2cEepromWriteData(addr, &testBufW, 1);
  if(ret < 0)
  {	dbgMsg("%s", "I2C EEPROM Write Data Failed\r\n");
    return;
  }
  SENS_TIME_DELAY(500); // EEPROM write cycle time
  
  ret = i2cEepromReadData(addr, &testBufR, 1);
  if(ret < 0)
  {	dbgMsg("%s", "I2C EEPROM Read Data Failed\r\n");
    return;
  }
  SENS_TIME_DELAY(500); // EEPROM read cycle time

  if(testBufR != testBufW)
  {	errorCount++;
    dbgMsg("I2C EEPROM Read/Write Data Mismatch at index %d: W=0x%02X, R=0x%02X\r\n", addr, testBufW, testBufR);
  }

  dbgMsg("I2C EEPROM Read/Write Test Count = %d\r\n", testCount++);
  
  if(errorCount > 0)
    dbgMsg("I2C EEPROM Read/Write Byte Error Count = %d\r\n", errorCount);
}

#define TEST_BUFFER_SIZE 4
void i2cEepromRWTestPlan2(void)
{	uint8_t testBufW[TEST_BUFFER_SIZE];
  uint8_t testBufR[TEST_BUFFER_SIZE];
  uint8_t addr;
  int ret, idx;

  if(toggleEEP == false)
  { toggleEEP = true;
    for(idx = 0; idx < TEST_BUFFER_SIZE; idx++)
      testBufW[idx] = 0x55;
  }
  else
  { toggleEEP = false;
    for(idx = 0; idx < TEST_BUFFER_SIZE; idx++)
      testBufW[idx] = 0xAA;
  }

  addr = (rand() % (256 / TEST_BUFFER_SIZE)) * TEST_BUFFER_SIZE;
  printf("%s", "========================= I2C EEPROM PLAN 2 ==========================\n");
  dbgMsg("I2C EEPROM Read/Write Data: 0x%X\r\n", testBufW[0]);
  dbgMsg("I2C EEPROM Random Address: 0x%X\r\n", addr);
  
  ret = i2cEepromWriteData(addr, testBufW, TEST_BUFFER_SIZE);
  if(ret < 0)
  {	dbgMsg("%s", "I2C EEPROM Write Data Failed\r\n");
    return;
  }
  SENS_TIME_DELAY(500); // EEPROM write cycle time

  ret = i2cEepromReadData(addr, testBufR, TEST_BUFFER_SIZE);
  if(ret < 0)
  {	dbgMsg("%s", "I2C EEPROM Read Data Failed\r\n");
    return;
  }
  SENS_TIME_DELAY(500); // EEPROM read cycle time

  for(idx = 0; idx < TEST_BUFFER_SIZE; idx++)
  { if(testBufR[idx] != testBufW[idx])
    {	errorCount++;
      dbgMsg("I2C EEPROM Read/Write Data Mismatch at index %d: W=0x%02X, R=0x%02X\r\n", addr, testBufW[idx], testBufR[idx]);
    }
  }

  if(errorCount > 0)
    dbgMsg("I2C EEPROM Read/Write Byte Error Count = %d\r\n", errorCount);
    
  dbgMsg("I2C EEPROM Read/Write Test Count = %d\r\n", testCount++);
  
}
/**************************************************************************
 *********              End of File                              *********
 **************************************************************************/
