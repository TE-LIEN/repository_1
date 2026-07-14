#include "sensCommon.h"
#include "driver/tcn75a.h"

/**************************************************************************
 *********              Program Define                            *********
 **************************************************************************/
#define TCN75A_SLA				0x48  // TCN75A Slave Address
#define TCN75A_REG_TEMP			0x00  // Ambient Temperature Register (TA)
#define TCN75A_REG_CONFIG		0x01  // Sensor Configuration Register (CONFIG)
#define TCN75A_REG_TLOW			0x02  // Temperature Low-Limit Register (TLOW)
#define TCN75A_REG_THIGH		0x03  // Temperature High-Limit Register (THIGH)
 
/**************************************************************************
 *********              Program Controller Function               *********
 **************************************************************************/
/* Ambient Temperature Register (TA) */
float tcn75aReadTemperature(void)
{	uint8_t tempBuf[2];
  int16_t tempData;
  float temperature;
  
  i2cRead(I2C2_ID, TCN75A_SLA, TCN75A_REG_TEMP, &tempBuf[0], 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
  //dbgMsg("TCN75A Temperature buf is 0x%02x 0x%02x \r\n", tempBuf[0], tempBuf[1]);
  /* Expand 9-bit two's complement to 16-bit two's complement */
  // tempData = (int16_t)tempBuf[0] << 1 | tempBuf[1] >> 7;
  /* Resolution 0.5 C for TCN75A */
  // temperature = (float)tempData * 0.5f;
  
  /* Expand 10-bit two's complement to 16-bit two's complement */
  // tempData = (int16_t)tempBuf[0] << 2 | tempBuf[1] >> 6;
  /* Resolution 0.25 C for TCN75A */
  // temperature = (float)tempData * 0.25f;

  /* Expand 11-bit two's complement to 16-bit two's complement */
  // tempData = (int16_t)tempBuf[0] << 3 | tempBuf[1] >> 5;
  /* Resolution 0.125 C for TCN75A */
  // temperature = (float)tempData * 0.125f;

	/* Expand 12-bit two's complement to 16-bit two's complement */
  tempData = (int16_t)tempBuf[0] << 4 | tempBuf[1] >> 4;
  /* Resolution 0.0625 C for TCN75A */
	temperature = (float)tempData * 0.0625f;
  
  //dbgMsg("TCN75A Temperature is %.2f C\r\n", temperature);
  return (int16_t)temperature;
}

/* SENSOR CONFIGURATION REGISTER (CONFIG) */
void tcn75aConfig(uint8_t confData)
{	uint8_t tempBuf[1];
  
  i2cRead(I2C2_ID, TCN75A_SLA, TCN75A_REG_CONFIG, &tempBuf[0], 1, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
  dbgMsg("TCN75A get current config data is 0x%02x \r\n", tempBuf[0]);
  if(tempBuf[0] != confData)
  {	tempBuf[0] = confData;
    dbgMsg("TCN75A set config data is 0x%02x \r\n", tempBuf[0]);
    i2cWrite(I2C2_ID, TCN75A_SLA, TCN75A_REG_CONFIG, &tempBuf[0], 1);
  }
}

/* TEMPERATURE LOW-LIMIT REGISTER (TLOW) */
void tcn75aTLowSet(float confData)
{	uint8_t tempBuf[2];
  float confData16 = 0, confData16_2 = 0;
  
  i2cRead(I2C2_ID, TCN75A_SLA, TCN75A_REG_TLOW, &tempBuf[0], 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
  confData16 = (((uint16_t)tempBuf[0] << 8 | tempBuf[1]) >> 7) * 0.5; //get current TLow value
  dbgMsg("TCN75A get current TLow raw is 0x%02x 0x%02x, Temp %.1f C\r\n", tempBuf[0], tempBuf[1], confData16);
  if((int8_t)confData16 != confData)
  {	confData16_2 = (uint16_t)(confData / 0.5f) << 7; //calculate TLow value to be set
    // dbgMsg("TCN75A Set TLow raw is 0x%04x, Temp %.1f C\r\n", (uint16_t)confData16_2, confData);
    tempBuf[0] = (uint16_t)confData16_2 >> 8; //set TLow MSB
    tempBuf[1] = (uint16_t)confData16_2 ; //set TLow LSB to 0
    dbgMsg("TCN75A Set TLow raw is 0x%02x 0x%02x, Temp %.1f C\r\n", tempBuf[0], tempBuf[1], confData);
    i2cWrite(I2C2_ID, TCN75A_SLA, TCN75A_REG_TLOW, &tempBuf[0], 2);
  }
}

/* TEMPERATURE HIGH-LIMIT REGISTER (THIGH) */
void tcn75aTHighSet(float confData)
{	uint8_t tempBuf[2];
  float confData16 = 0, confData16_2 = 0;
  
  i2cRead(I2C2_ID, TCN75A_SLA, TCN75A_REG_THIGH, &tempBuf[0], 2, I2C_READ_MODE_FIRST_ISSUE_SLA_W);
  confData16 = (((uint16_t)tempBuf[0] << 8 | tempBuf[1]) >> 7) * 0.5; //get current THigh value
  dbgMsg("TCN75A get current THigh raw is 0x%02x 0x%02x, Temp %.1f C\r\n", tempBuf[0], tempBuf[1], confData16);
  if((int8_t)confData16 != confData)
  {	confData16_2 = (uint16_t)(confData / 0.5f) << 7; //calculate THigh value to be set
    // dbgMsg("TCN75A Set THigh raw is 0x%04x\r\n", (uint16_t)confData16_2);
    tempBuf[0] = (uint16_t)confData16_2 >> 8; //set THigh MSB
    tempBuf[1] = (uint16_t)confData16_2 ; //set THigh LSB to 0
    dbgMsg("TCN75A Set THigh raw is 0x%02x 0x%02x, Temp %.1f C\r\n", tempBuf[0], tempBuf[1], confData);
    i2cWrite(I2C2_ID, TCN75A_SLA, TCN75A_REG_THIGH, &tempBuf[0], 2);
  }
}