#include "ad7124Adc.h"
#include "sensSystem.h"

#define STRUCT_ARRAY_SIZE(x)		                sizeof(x) / sizeof(x[0])

static SPI_INSTANCE adcSpiInst = {0};

int readAdcReg(AD7124_DEV *dev, ADC_REG_INFO *regInf);
int openAdcSpi(uint32_t *chipId, uint8_t differential);

ADC_REG_INFO adcRegInfo[] =	//reduce code size, change struct
		//	Addr, 				size,
		{	{REG_STATUS,		1,	0,	ADC_REG_READ_ONLY, 	0},
			{REG_ADC_CTRL, 		2,	0,	ADC_REG_READ_WRITE, 0x01C8},
			{REG_ADC_DATA, 		3,	1,	ADC_REG_READ_ONLY,  0},
			{REG_IO_CTRL_1, 	3,	0,	ADC_REG_READ_WRITE, 0},
			{REG_IO_CTRL_2, 	2,	0,	ADC_REG_READ_WRITE, 0},
			{REG_ID, 			1,	0,	ADC_REG_READ_ONLY,	0x00},
			{REG_ERR, 			3,	0,	ADC_REG_READ_ONLY, 	0},
			{REG_ERR_EN, 		3,	0,	ADC_REG_READ_WRITE, 0x000000},
			{REG_MCLK_CNT, 		1,	0,	ADC_REG_READ_ONLY, 	0},
			{REG_CHANNEL_0, 	2,	0,	ADC_REG_READ_WRITE, 0x8001},
			{REG_CHANNEL_1, 	2,	0,	ADC_REG_READ_WRITE, 0x0001},
			{REG_CHANNEL_2, 	2,	0,	ADC_REG_READ_WRITE, 0x0001},
			{REG_CHANNEL_3, 	2,	0,	ADC_REG_READ_WRITE, 0x0001},
			{REG_CHANNEL_4, 	2,	0,	ADC_REG_READ_WRITE, 0x0001},
			{REG_CHANNEL_5, 	2,	0,	ADC_REG_READ_WRITE, 0x0001},
			{REG_CHANNEL_6, 	2,	0,	ADC_REG_READ_WRITE, 0x0001},
			{REG_CHANNEL_7, 	2,	0,	ADC_REG_READ_WRITE, 0x0001},
			{REG_CHANNEL_8, 	2,	0,	ADC_REG_READ_WRITE, 0x1210},
			{REG_CHANNEL_9, 	2,	0,	ADC_REG_READ_WRITE, 0x1210},
			{REG_CHANNEL_10, 	2,	0,	ADC_REG_READ_WRITE, 0x1210},
			{REG_CHANNEL_11, 	2,	0,	ADC_REG_READ_WRITE, 0x1210},
			{REG_CHANNEL_12, 	2,	0,	ADC_REG_READ_WRITE, 0x1210},
			{REG_CHANNEL_13, 	2,	0,	ADC_REG_READ_WRITE, 0x1210},
			{REG_CHANNEL_14, 	2,	0,	ADC_REG_READ_WRITE, 0x1210},
			{REG_CHANNEL_15, 	2,	0,	ADC_REG_READ_WRITE, 0x1210},
#if ADC_USE_OLD_CIRCUIT
			{REG_CONFIG_0, 		2,	0,	ADC_REG_READ_WRITE, 0x0860},	//UNIPOLAR, REF_BUFP, REF_BUFM, AIN_BUFP, AIN_BUFM, PGA_GAIN=0
			{REG_CONFIG_1, 		2,	0,	ADC_REG_READ_WRITE, 0x0860}, //UNIPOLAR, REF_BUFP, REF_BUFM, AIN_BUFP, AIN_BUFM, PGA_GAIN=1
			{REG_CONFIG_2, 		2,	0,	ADC_REG_READ_WRITE, 0x0860},	//UNIPOLAR, REF_BUFP, REF_BUFM, AIN_BUFP, AIN_BUFM, PGA_GAIN=16
			{REG_CONFIG_3, 		2,	0,	ADC_REG_READ_WRITE, 0x0860},	//BIPOLAR, REF_BUFP, REF_BUFM, AIN_BUFP, AIN_BUFM, PGA_GAIN=0
#else
			{REG_CONFIG_0,	2,	0,	ADC_REG_READ_WRITE, 0x01E0},	//UNIPOLAR, REF_BUFP, REF_BUFM, AIN_BUFP, AIN_BUFM, PGA_GAIN=0
			{REG_CONFIG_1, 	2,	0,	ADC_REG_READ_WRITE, 0x01E1}, //UNIPOLAR, REF_BUFP, REF_BUFM, AIN_BUFP, AIN_BUFM, PGA_GAIN=1
			{REG_CONFIG_2, 	2,	0,	ADC_REG_READ_WRITE, 0x01E4},	//UNIPOLAR, REF_BUFP, REF_BUFM, AIN_BUFP, AIN_BUFM, PGA_GAIN=16
			{REG_CONFIG_3, 	2,	0,	ADC_REG_READ_WRITE, 0x09E0},	//BIPOLAR, REF_BUFP, REF_BUFM, AIN_BUFP, AIN_BUFM, PGA_GAIN=0
#endif
			{REG_CONFIG_4, 		2,	0,	ADC_REG_READ_WRITE, 0x0860},
			{REG_CONFIG_5, 		2,	0,	ADC_REG_READ_WRITE, 0x0860},
			{REG_CONFIG_6, 		2,	0,	ADC_REG_READ_WRITE, 0x0860},
			{REG_CONFIG_7, 		2,	0,	ADC_REG_READ_WRITE, 0x0860},
			{REG_FILTER_0, 		3,	0,	ADC_REG_READ_WRITE, 0x060180},
			{REG_FILTER_1, 		3,	0,	ADC_REG_READ_WRITE, 0x060180},
			{REG_FILTER_2,		3,	0,	ADC_REG_READ_WRITE, 0x060180},
			{REG_FILTER_3,		3,	0,	ADC_REG_READ_WRITE, 0x060180},
			{REG_FILTER_4, 		3,	0,	ADC_REG_READ_WRITE, 0x060180},
			{REG_FILTER_5, 		3,	0,	ADC_REG_READ_WRITE, 0x060180},
			{REG_FILTER_6,		3,	0,	ADC_REG_READ_WRITE, 0x060180},
			{REG_FILTER_7,		3,	0,	ADC_REG_READ_WRITE, 0x060180},
			{REG_OFFSET_0, 		3,	0,	ADC_REG_READ_WRITE, 0x800000},
			{REG_OFFSET_1, 		3,	0,	ADC_REG_READ_WRITE, 0x800000},
			{REG_OFFSET_2, 		3,	0,	ADC_REG_READ_WRITE, 0x800000},
			{REG_OFFSET_3, 		3,	0,	ADC_REG_READ_WRITE, 0x800000},
			{REG_OFFSET_4, 		3,	0,	ADC_REG_READ_WRITE, 0x800000},
			{REG_OFFSET_5, 		3,	0,	ADC_REG_READ_WRITE, 0x800000},
			{REG_OFFSET_6, 		3,	0,	ADC_REG_READ_WRITE, 0x800000},
			{REG_OFFSET_7, 		3,	0,	ADC_REG_READ_WRITE, 0x800000},
			{REG_GAIN_0, 		3,	0,	ADC_REG_READ_WRITE, 0x500000},	//0x555331
			{REG_GAIN_1, 		3,	0,	ADC_REG_READ_WRITE, 0x500000},
			{REG_GAIN_2,		3,	0,	ADC_REG_READ_WRITE, 0x500000},
			{REG_GAIN_3,		3,	0,	ADC_REG_READ_WRITE, 0x500000},
			{REG_GAIN_4, 		3,	0,	ADC_REG_READ_WRITE, 0x500000},
			{REG_GAIN_5, 		3,	0,	ADC_REG_READ_WRITE, 0x500000},
			{REG_GAIN_6,		3,	0,	ADC_REG_READ_WRITE, 0x500000},
			{REG_GAIN_7,		3,	0,	ADC_REG_READ_WRITE, 0x500000},
			{REG_RESET,			7,	0,	ADC_REG_WRITE_ONLY, 0x000000},
		};
		
AD7124_DEV	ad7124Dev;
		
		
ADC_REG_INFO *findRegInfo(int regAddr)
{	int idx;
	ADC_REG_INFO *regInf = NULL;

	for(idx=0;idx<STRUCT_ARRAY_SIZE(adcRegInfo);idx++)
	{	if(regAddr == adcRegInfo[idx].regAddr)
		{	regInf = (ADC_REG_INFO *)&adcRegInfo[idx];
			break;
		}
	}
	return regInf;
}

int ad7124WriteAndRead(ADC_REG_INFO *regInf, uint8_t *writeBuf, uint8_t *readBuf, uint16_t size)
{	uint8_t regReadBuf[8];
	int idx;
	int argc;
	int argsType;
	int xmitMode;
	
	if(!adcSpiInst.spiCtx)
		openAdcSpi(NULL, ad7124Dev.differential);
	
	if(regInf->regAddr == REG_RESET)
	{	SENS_SPI_CTRL((SPI_INSTANCE *)&adcSpiInst, 0xFF, 0xFFFFFF, SPI_CMD_ARG_24, SPI_WRITE_DATA, writeBuf, 4);
	}
	else
	{	if(regInf->size == 1)
			argsType = SPI_CMD_ARG_8;
		else if(regInf->size == 2)
			argsType = SPI_CMD_ARG_16;
		else if(regInf->size == 3)
			argsType = SPI_CMD_ARG_24;
		
		if(readBuf == NULL)
		{	xmitMode = SPI_WRITE_DATA;
			argc = 0;
			for(idx=0;idx<regInf->size;idx++)
			{	argc |= (uint32_t)writeBuf[regInf->size-idx] << (idx * 8);
			}
		}
		else
		{	argsType = SPI_CMD_ONLY;
			xmitMode = SPI_READ_DATA;
		}
		
		SENS_SPI_CTRL((SPI_INSTANCE *)&adcSpiInst, writeBuf[0], argc, argsType, xmitMode, regReadBuf, size-1);
	}	
	
	
	if(readBuf != NULL)
	{	for(idx=0;idx<size;idx++)
		{	readBuf[idx] = regReadBuf[idx];
		}
	}
	
	return 0;
}

int ad7124WaitForSpiReady(AD7124_DEV *dev, uint32_t timeout)
{	ADC_REG_INFO *regInf;
	uint8_t ready = 0;
	
	regInf = ad7124Dev.adcRegInfo;
	
	while(!ready && --timeout)
	{	readAdcReg(dev, &regInf[REG_ERR]);
		ready = (regInf[REG_ERR].value & AD7124_ERR_REG_SPI_IGNORE_ERR) == 0;
	}
	
	return timeout? 0:-1;	//0: no timeout , -1: timeout
}

int ad7124NoChkReadReg(AD7124_DEV *dev, ADC_REG_INFO *regInf)
{	uint8_t regWriteBuf[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t addStatusLength = 0;
	uint8_t readBuf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int i;

	if(regInf == NULL)
		return -1;

	regWriteBuf[0] =  AD7124_COMM_REG_WEN | AD7124_COMM_REG_RD | CR_RS(regInf->regAddr);

	if(regInf->regAddr == REG_ADC_DATA && dev->adcRegInfo[REG_ADC_CTRL].value & AD7124_ADC_CTRL_REG_DATA_STATUS)
	{	addStatusLength = 1;
	}
	ad7124WriteAndRead(regInf, regWriteBuf, readBuf, ((dev->useCrc != AD7124_DISABLE_CRC)? regInf->size+1: regInf->size) + 1 + addStatusLength);

	if(dev->useCrc == AD7124_USE_CRC)
	{	//msgBuf[0] = AD7124_COMM_REG_WEN | AD7124_COMM_REG_RD | CR_RS(regInf->regAddr);

	}

	if(addStatusLength)
		dev->adcRegInfo[REG_STATUS].value = readBuf[regInf->size+1];

	regInf->value = 0;
	for(i=0;i<regInf->size;i++)
	{	regInf->value <<= 8;
		regInf->value +=  readBuf[i];
	}

	return 0;
}

int ad7124NoChkWriteReg(AD7124_DEV *dev, ADC_REG_INFO *regInf)
{	uint8_t regWriteBuf[8];
	uint32_t regValue = 0;
	int i;

	memset(regWriteBuf, 0xFF, 8);

	regWriteBuf[0] = AD7124_COMM_REG_WEN | AD7124_COMM_REG_WR | AD7124_COMM_REG_RA(regInf->regAddr);
	regValue = regInf->value;
	for(i=0;i<regInf->size;i++)
	{	regWriteBuf[regInf->size - i] = regValue & 0xFF;
		regValue >>= 8;
	}

	if(dev->useCrc != AD7124_DISABLE_CRC)
	{

	}

	return ad7124WriteAndRead(regInf, regWriteBuf, NULL, ((dev->useCrc != AD7124_DISABLE_CRC)? regInf->size+1: regInf->size) + 1);
}

int readAdcReg(AD7124_DEV *dev, ADC_REG_INFO *regInf)
{	int ret;

	if(regInf == NULL)
		return -1;

	if(regInf->regAddr != REG_ERR && dev->chkRdy)
	{	ret = ad7124WaitForSpiReady(dev, ad7124Dev.rdyPollCnt);
		if(ret < 0)
			return ret;
	}
	return ad7124NoChkReadReg(dev, regInf);
}

int writeAdcReg(AD7124_DEV *dev, ADC_REG_INFO *regInf)
{	int ret;

	if(dev->chkRdy)
	{	ret = ad7124WaitForSpiReady(dev, ad7124Dev.rdyPollCnt);
		if(ret < 0)
			return ret;
	}
	return ad7124NoChkWriteReg(dev, regInf);
}

int adc7124WaitPorOn(void)
{	int powerOn = 0;
	int timeout = ad7124Dev.rdyPollCnt;
	ADC_REG_INFO *regInf;

	regInf = ad7124Dev.adcRegInfo;

	while(!powerOn && timeout--)
	{	readAdcReg(&ad7124Dev, &regInf[REG_STATUS]);
		powerOn = (regInf[REG_STATUS].value & AD7124_STATUS_REG_POR_FLAG) == 0;
	}

	return (timeout || powerOn) ? 0:-1;
}

int adc7124Reset(void)
{	ADC_REG_INFO *regInf;
	uint8_t wrBuf[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	regInf = ad7124Dev.adcRegInfo;

	ad7124WriteAndRead(&regInf[REG_RESET], wrBuf, NULL, 8);
	ad7124Dev.useCrc = AD7124_DISABLE_CRC;
	adc7124WaitPorOn();
	return 0;
}

int ad7124WaitForConvertReady(uint8_t channel, uint32_t timeout)
{	ADC_REG_INFO *regInf;
	uint8_t ready = 0;
	int ret;
	regInf = ad7124Dev.adcRegInfo;
	uint8_t convChannel;
	//uint32_t currTick = GetTimeTicks();
	//int timeoutFlag = 0;

	while(--timeout)
	{	ret = readAdcReg(&ad7124Dev, &regInf[REG_STATUS]);
		if(ret < 0)
			return ret;
		ready = (regInf[REG_STATUS].value & AD7124_STATUS_REG_RDY) == 0;
		if(regInf[REG_STATUS].value & AD7124_STATUS_REG_ERROR_FLAG)
		{	readAdcReg(&ad7124Dev, &regInf[REG_ERR]);
			// dbg_msg(RED"[ADC] channel %d, error flag %X\r\n"ORG_COLOR, channel, regInf[REG_ERR].value);
			//ad7124Dev.errorOccurred = 1;
			//return 0;
			//return -1;
		}
		convChannel = regInf[REG_STATUS].value & 0x0F;
		if(ready && ( convChannel == channel ))
		{	// dbgMsg(GREEN"[ADC] channel %d is Done, timeout:%d\r\n"ORG_COLOR, channel, timeout);
			break;
		}
		if(ready && (convChannel != channel))
		{	// dbgMsg(RED"[ADC] channel %d is Ready, but conv Channel diff:%d, timeout:%d\r\n"ORG_COLOR, channel, convChannel, timeout);
		}
		if(ready == 0)
			SENS_TIME_DELAY(10);
		/*if((GetTimeTicks() - currTick) >= timeout)
		{	timeoutFlag = 1;
			break;
		}*/
	}
	//if(timeoutFlag)
	if(!timeout)
	{	dbgMsg(RED"[ADC] channel %d Timeout\r\n"ORG_COLOR, channel);
	}

	return timeout ? 0:-1;
	//return timeoutFlag? -1:0;
}

int adcReadDataReg(int *data)
{	//uint8_t readBuf[4];
	//int status;
	ADC_REG_INFO *regInf;

	regInf = ad7124Dev.adcRegInfo;
	readAdcReg(&ad7124Dev, &regInf[REG_ADC_DATA]);
	if((regInf[REG_ADC_DATA].value == 0) || (regInf[REG_ADC_DATA].value == 0xFFFFFF))
	{	ad7124Dev.errorOccurred = 1;
		return -1;
	}
	*data = regInf[REG_ADC_DATA].value;
	return 0;
}

void setupSpiAdcChannel(uint8_t channel, uint8_t differential)
{	//int adcCtrlReg;
	ADC_REG_INFO *regInf;
	regInf = ad7124Dev.adcRegInfo;

	SET_SETUP(regInf[REG_CHANNEL_0+channel].value, 0);
	if(!channel)
	{	regInf[REG_CHANNEL_0+channel].value |= CHANNEL_ENABLE;
	}
	else
	{	CLR_CHANNEL_EN(regInf[REG_CHANNEL_0+channel].value);
	}
	if(differential == DIFFERENTIAL_MODE)
	{	SET_AINP(regInf[REG_CHANNEL_0+channel].value, AIN0 + channel*2);
		SET_AINM(regInf[REG_CHANNEL_0+channel].value, AIN1 + channel*2);	//for differential
	}
	else
	{	SET_AINP(regInf[REG_CHANNEL_0+channel].value, AIN0 + channel);
		// SET_AINM(regInf[REG_CHANNEL_0+channel].value, AIN1 + channel);	//for single ended
		SET_AINM(regInf[REG_CHANNEL_0+channel].value, AIN15);	//for single ended
	}


#if 0

	adcFilterRegRead(channel, &adcCtrlReg);
	SET_SINGLE_CYCLE(adcCtrlReg);
	SET_FILTER_OUT_DATA_RATE(adcCtrlReg, 2047);
	SET_FILTER_TYPE(adcCtrlReg, FILTER_SINC3);
	adcFilterRegWrite(channel, adcCtrlReg);

	adcConfigRegRead(channel, &adcCtrlReg);
	//dbg_msg("config %d reg=0x%x \n\r", channel, adcCtrlReg);
	SET_BIPOLAR(adcCtrlReg);
#if defined ADC_OLD
	SET_REF_SEL(adcCtrlReg, INTERNAL_REF);
#else
	SET_REF_SEL(adcCtrlReg, REFIN1_PM);
#endif
	SET_AINP_BUF(adcCtrlReg);
	SET_AINM_BUF(adcCtrlReg);

	//SET_PGA(adcCtrlReg, PGA_1DOT25V);
	//SET_PGA(adcCtrlReg, PGA_19DOT53mV);
	//dbg_msg("write config %d reg=0x%x \n\r", channel, adcCtrlReg);
	adcConfigRegWrite(channel, adcCtrlReg);

	adcChannelRegRead(channel, &adcCtrlReg);
	//dbg_msg("channel %d reg=0x%x \n\r", channel, adcCtrlReg);
	SET_SETUP(adcCtrlReg, channel);
	if(!channel)
	{	adcCtrlReg |= CHANNEL_ENABLE;
	}
	else
	{	CLR_CHANNEL_EN(adcCtrlReg);
	}
	if(differential == DIFFERENTIAL_MODE)
	{	SET_AINP(adcCtrlReg, AIN0 + channel*2);
		SET_AINM(adcCtrlReg, AIN1 + channel*2);	//for differential
	}
	else
	{	SET_AINP(adcCtrlReg, AIN0 + channel);
		SET_AINM(adcCtrlReg, AIN15);	//for single ended
	}
	//dbg_msg("write channel %d reg=0x%x \n\r", channel, adcCtrlReg);
	adcChannelRegWrite(channel, adcCtrlReg);
#endif
}

#if SINGLE_CONVERSION_SINGLE_CHANNEL_EN == 1
static int enableSpiAdcIndicateChannel(uint8_t channel, uint8_t *isBipolar, uint8_t *gainVal)
{	int status=0;
	//int adcCtrlReg;
	ADC_REG_INFO *regInf;
	uint16_t configNumber;

	regInf = ad7124Dev.adcRegInfo;

	readAdcReg(&ad7124Dev, &regInf[REG_ADC_CTRL]);
	SET_MODE(regInf[REG_ADC_CTRL].value, MODE_STANDBY);
	writeAdcReg(&ad7124Dev, &regInf[REG_ADC_CTRL]);
	SENS_TIME_DELAY(10);

	readAdcReg(&ad7124Dev, &regInf[REG_CHANNEL_0 + ad7124Dev.prevEnableChannel]);
	CLR_CHANNEL_EN(regInf[REG_CHANNEL_0 + ad7124Dev.prevEnableChannel].value);
	writeAdcReg(&ad7124Dev, &regInf[REG_CHANNEL_0 + ad7124Dev.prevEnableChannel]);

	readAdcReg(&ad7124Dev, &regInf[REG_CHANNEL_0+channel]);
	SET_CHANNEL_EN(regInf[REG_CHANNEL_0+channel].value);
	writeAdcReg(&ad7124Dev, &regInf[REG_CHANNEL_0+channel]);
	
	configNumber = GET_SETUP(regInf[REG_CHANNEL_0+channel].value);
	if(configNumber != 3)
		*isBipolar = UNIPOLAR_MODE;
	else
		*isBipolar = BIPOLAR_MODE;
	if((configNumber == 0) || (configNumber == 3))
		*gainVal = 1;
	else if(configNumber == 1)
		*gainVal = 2;
	else if(configNumber == 2)
		*gainVal = 16;
	
	readAdcReg(&ad7124Dev, &regInf[REG_ADC_CTRL]);
	SET_MODE(regInf[REG_ADC_CTRL].value, MODE_SINGLE_CONVERSION);
	writeAdcReg(&ad7124Dev, &regInf[REG_ADC_CTRL]);

#if 0
	adcChannelRegRead(prevEnableChannel, &adcCtrlReg);
	CLR_CHANNEL_EN(adcCtrlReg);
	adcChannelRegWrite(prevEnableChannel, adcCtrlReg);

#if defined ADC_OLD
	adcChannelRegRead(channel, &adcCtrlReg);
	SET_CHANNEL_EN(adcCtrlReg);
	adcChannelRegWrite(channel, adcCtrlReg);
#endif
	status = adcCtrlRegRead(&adcCtrlReg);
	//adcCtrlRegTemp = adcCtrlReg;
	SET_POWER_MODE(adcCtrlReg, POWER_MODE_FULL);
#if defined ADC_OLD
	SET_REF_EN(adcCtrlReg);
#else
	CLR_REF_EN(adcCtrlReg);
#endif
	SET_MODE(adcCtrlReg, MODE_SINGLE_CONVERSION);
	SET_DATA_STATUS(adcCtrlReg);
	adcCtrlRegWrite(adcCtrlReg);

#ifndef ADC_OLD
	adcChannelRegRead(channel, &adcCtrlReg);
	SET_CHANNEL_EN(adcCtrlReg);
	adcChannelRegWrite(channel, adcCtrlReg);
#endif
#endif
	ad7124Dev.prevEnableChannel = channel;
	return status;
}
#elif SINGLE_CONVERSION_SEVERAL_CHANNELS_EN == 1
static int enableSpiAdcIndicateChannel(uint8_t channel, uint8_t *isBipolar, uint8_t *gainVal)
{	int status = 0, idx = 0;
	//int adcCtrlReg;
	ADC_REG_INFO *regInf;
	uint16_t configNumber;

	regInf = ad7124Dev.adcRegInfo;

	readAdcReg(&ad7124Dev, &regInf[REG_ADC_CTRL]);
	SET_MODE(regInf[REG_ADC_CTRL].value, MODE_STANDBY);
	writeAdcReg(&ad7124Dev, &regInf[REG_ADC_CTRL]);
	SENS_TIME_DELAY(10);

	for(idx=0;idx<16;idx++)
	{	CLR_CHANNEL_EN(regInf[REG_CHANNEL_0 + idx].value);
		writeAdcReg(&ad7124Dev, &regInf[REG_CHANNEL_0 + idx]);
	}
	// readAdcReg(&ad7124Dev, &regInf[REG_CHANNEL_0 + ad7124Dev.prevEnableChannel]);
	// CLR_CHANNEL_EN(regInf[REG_CHANNEL_0 + ad7124Dev.prevEnableChannel].value);
	// writeAdcReg(&ad7124Dev, &regInf[REG_CHANNEL_0 + ad7124Dev.prevEnableChannel]);

	// readAdcReg(&ad7124Dev, &regInf[REG_CHANNEL_0+channel]);
	for(idx=0;idx<channel;idx++)
	{	SET_CHANNEL_EN(regInf[REG_CHANNEL_0+idx].value);
		writeAdcReg(&ad7124Dev, &regInf[REG_CHANNEL_0+idx]);

		configNumber = GET_SETUP(regInf[REG_CHANNEL_0+idx].value);
		if(configNumber != 3)
			isBipolar[idx] = UNIPOLAR_MODE;
		else
			isBipolar[idx] = BIPOLAR_MODE;
		
		if(configNumber == 0)
			gainVal[idx] = 0;
		else if(configNumber == 3)
			gainVal[idx] = 1;
		else if(configNumber == 1)
			gainVal[idx] = 2;
		else if(configNumber == 2)
			gainVal[idx] = 16;
		
		dbg_msg("ADC_CHANNEL_EN_%d :%X\r\n", idx, regInf[REG_CHANNEL_0+idx].value);
	}
	readAdcReg(&ad7124Dev, &regInf[REG_ADC_CTRL]);
	SET_DATA_STATUS(regInf[REG_ADC_CTRL].value);
	SET_MODE(regInf[REG_ADC_CTRL].value, MODE_CONT_CONVERSION);
	writeAdcReg(&ad7124Dev, &regInf[REG_ADC_CTRL]);
	readAdcReg(&ad7124Dev, &regInf[REG_ADC_CTRL]);
	dbg_msg("[ADC_CTRL_MODE] MODE_CONT_CONVERSION Start :%X\r\n", regInf[REG_ADC_CTRL].value);

	// ad7124Dev.prevEnableChannel = channel;
	return status;
}
#endif

int ad7124Initial(uint8_t maxChannel, uint8_t differential, uint32_t *chipId)
{	//int adcCtrlReg;
	int status = 0;
	int channel;
	ADC_REG_INFO *regInf;
	int regNr;
	AI_CONFIG *aiCfg = (AI_CONFIG *)&sysCfg->aiCfg;

	//config 0, set to unipolar. and PGA 2.5V(1)		//for 4~20mA, 0~20mA, 10V
	//config 1, set to unipolar, and PGA 1.25V(2)		//for 5V
	//config 2, set to unipolar, and PGA 156.25mV(16)	//for 500mV
	//config 3, set to bipolar, and PGA 2.5V(1)			//for +-20mA, +-6V

	ad7124Dev.prevEnableChannel = 0;
	ad7124Dev.errorOccurred = 0;
	ad7124Dev.adcRegInfo = (ADC_REG_INFO*)&adcRegInfo;
	ad7124Dev.rdyPollCnt = 25000;
	//ad7124Dev.gainVal = PGA_2DOT5V;
	adc7124Reset();
	ad7124Dev.chkRdy = 1;

	SENS_TIME_DELAY(10);
	regInf = ad7124Dev.adcRegInfo;

	readAdcReg(&ad7124Dev, &regInf[REG_ID]);
	*chipId = regInf[REG_ID].value;
	//dbg_msg("adcId=0x%x \n\r", adcId[0]);
	if((regInf[REG_ID].value & 0xF0) != 0x10)
	{	dbg_msg("adc id error :%X\r\n", regInf[REG_ID].value);
		return -1;
	}

#if ADC_USE_OLD_CIRCUIT
	SET_REF_SEL(regInf[REG_CONFIG_0].value, INTERNAL_REF);
	//SET_REF_SEL(regInf[REG_CONFIG_0].value, REFIN1_PM);
	SET_PGA(regInf[REG_CONFIG_0].value, PGA_2DOT5V);
	//SET_REF_SEL(regInf[REG_CONFIG_0].value, REFIN1_PM);
	SET_BIPOLAR(regInf[REG_CONFIG_0].value);
	//if(ad7124Dev.bipolarSel) // bipolar
	//{ SET_BIPOLAR(regInf[REG_CONFIG_0].value);
	//}
	//else	// unipolar
	//	SET_UNIPOLAR(regInf[REG_CONFIG_0].value);
	SET_REFP_BUF(regInf[REG_CONFIG_0].value);
	SET_REFM_BUF(regInf[REG_CONFIG_0].value);
	SET_AINP_BUF(regInf[REG_CONFIG_0].value);
	SET_AINM_BUF(regInf[REG_CONFIG_0].value);

	//REG_FILTER_0

	//SET_FILTER_OUT_DATA_RATE(regInf[REG_CONFIG_0+channel].value, 0x);

	for(channel=0;channel<maxChannel;channel++)
		setupSpiAdcChannel(channel, differential);
#else
	int configNumber = 0;
	if(aiCfg->sensorType[0] == AI_TYPE_20MA_BIPOLAR)
		configNumber = 3;
	SET_SETUP(regInf[REG_CHANNEL_0].value, configNumber);
	SET_CHANNEL_EN(regInf[REG_CHANNEL_0].value);
	SET_AINP(regInf[REG_CHANNEL_0].value, AIN0);
	SET_AINM(regInf[REG_CHANNEL_0].value, AIN1);
	//channel 1, use AIN2, AIN3
	configNumber = 0;
	if(aiCfg->sensorType[1] == AI_TYPE_20MA_BIPOLAR)
		configNumber = 3;
	SET_SETUP(regInf[REG_CHANNEL_1].value, configNumber);
	CLR_CHANNEL_EN(regInf[REG_CHANNEL_1].value);
	SET_AINP(regInf[REG_CHANNEL_1].value, AIN2);
	SET_AINM(regInf[REG_CHANNEL_1].value, AIN3);
	//channel 2, use AIN4, AIN5
	configNumber = 0;
	if(aiCfg->sensorType[2] == AI_TYPE_20MA_BIPOLAR)
		configNumber = 3;
	SET_SETUP(regInf[REG_CHANNEL_2].value, configNumber);
	CLR_CHANNEL_EN(regInf[REG_CHANNEL_2].value);
	SET_AINP(regInf[REG_CHANNEL_2].value, AIN4);
	SET_AINM(regInf[REG_CHANNEL_2].value, AIN5);
	//channel 3, use AIN4, AIN5
	configNumber = 0;
	
	if(aiCfg->sensorType[3] == AI_TYPE_500MV)
		configNumber = 2;
	else if(aiCfg->sensorType[3] == AI_TYPE_5V)
		configNumber = 1;
	else if(aiCfg->sensorType[3] == AI_TYPE_6V_BIPOLAR)
		configNumber = 3;
	SET_SETUP(regInf[REG_CHANNEL_3].value, configNumber);
	CLR_CHANNEL_EN(regInf[REG_CHANNEL_3].value);
	SET_AINP(regInf[REG_CHANNEL_3].value, AIN6);
	if(aiCfg->differential == DIFFERENTIAL_MODE)
	{	SET_AINM(regInf[REG_CHANNEL_3].value, AIN7);
	}
	else
	{	SET_AINM(regInf[REG_CHANNEL_3].value, AVSS);
	}
	configNumber = 0;
	if(aiCfg->differential == SINGLE_END_MODE)
	{	if(aiCfg->sensorType[4] == AI_TYPE_500MV)
			configNumber = 2;
		else if(aiCfg->sensorType[4] == AI_TYPE_5V)
			configNumber = 1;
		else if(aiCfg->sensorType[4] == AI_TYPE_6V_BIPOLAR)
			configNumber = 3;
		SET_SETUP(regInf[REG_CHANNEL_4].value, configNumber);
		CLR_CHANNEL_EN(regInf[REG_CHANNEL_4].value);
		SET_AINP(regInf[REG_CHANNEL_4].value, AIN6);
		SET_AINM(regInf[REG_CHANNEL_4].value, AVSS);
	}
	
#endif
	ad7124Dev.prevEnableChannel = 0;
	regInf[REG_ERR_EN].value = (AD7124_ERR_REG_LDO_CAP_ERR | AD7124_ERR_REG_ADC_CONV_ERR | AD7124_ERR_REG_ADC_SAT_ERR
			/*| AD7124_ERR_REG_AINP_OV_ERR | AD7124_ERR_REG_AINP_UV_ERR | AD7124_ERR_REG_AINM_OV_ERR | AD7124_ERR_REG_AINM_UV_ERR | AD7124_ERR_REG_REF_DET_ERR*/
			| AD7124_ERR_REG_DLDO_PSM_ERR | AD7124_ERR_REG_ALDO_PSM_ERR /*| AD7124_ERREN_REG_SPI_IGNORE_ERR_EN*/);


	for(regNr=REG_STATUS;regNr<REG_OFFSET_0;regNr++)
	{	if(regInf[regNr].RW == ADC_REG_READ_WRITE)
		{	writeAdcReg(&ad7124Dev, &regInf[regNr]);
		}
		if(regNr == REG_ERR_EN)
		{	if(regInf[REG_ERR_EN].value & AD7124_ERREN_REG_SPI_CRC_ERR_EN)
				ad7124Dev.useCrc = AD7124_USE_CRC;
			else
				ad7124Dev.useCrc = AD7124_DISABLE_CRC;
			if(regInf[REG_ERR_EN].value & AD7124_ERREN_REG_SPI_IGNORE_ERR_EN)
				ad7124Dev.chkRdy = 1;
			else
				ad7124Dev.chkRdy = 0;
		}
	}
#if 0
	status = adcCtrlRegRead(&adcCtrlReg);
	if(status != 0)
		return XST_FAILURE;
	SET_POWER_MODE(adcCtrlReg, POWER_MODE_FULL);
#if defined ADC_OLD
	SET_REF_EN(adcCtrlReg);
#else
	CLR_REF_EN(adcCtrlReg);
#endif
	SET_MODE(adcCtrlReg, MODE_SINGLE_CONVERSION);
	//SET_MODE(adcCtrlReg, MODE_CONT_CONVERSION);
	//SET_DATA_STATUS(adcCtrlReg);
	//SET_CONT_READ(adcCtrlReg);
	adcCtrlRegWrite(adcCtrlReg);
	status = adcCtrlRegRead(&adcCtrlReg);
#endif
	return status;
}


int openAdcSpi(uint32_t *chipId, uint8_t differential)
{	uint32_t chipIdTemp;
	uint8_t channels = 8;
	SYS_UnlockReg();
	
	/* Enable SPI0 module clock */
	CLK_EnableModuleClock(SENS_ADC_SPI_MODULE);
	/* Select SPI0 module clock source as PCLK1 */
	CLK_SetModuleClock(SENS_ADC_SPI_MODULE, SENS_ADC_SPI_CLK_SEL, MODULE_NoMsk);

	if(adcSpiInst.spiCtx)
		return 0;

#if ADC_SPI_CHANNEL == 0
	SET_SPI0_MOSI_PA0();
	SET_SPI0_MISO_PA1();
	SET_SPI0_CLK_PA2();
	SET_SPI0_SS_PA3();
	PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
	/* Enable SPI0 I/O high slew rate */
	GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT2 | BIT3, GPIO_SLEWCTL_HIGH);
#elif ADC_SPI_CHANNEL == 1
  
	SET_SPI1_MOSI_PH5();
	SET_SPI1_MISO_PH4();
	SET_SPI1_CLK_PH6();
	SET_SPI1_SS_PH7();
	PH->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
	GPIO_SetSlewCtl(PH, BIT4 | BIT5 | BIT6 | BIT7, GPIO_SLEWCTL_HIGH);
#endif
	adcSpiInst.spiMode = SENS_ADC_SPI_MODE;
	adcSpiInst.spiOpMode = SPI_MODE_3;
	adcSpiInst.spiOpClk = 2000000;
	
	adcSpiInst.spiCtx = SENS_ADC_SPI_CTX;
		
	SENS_SPI_OPEN((SPI_INSTANCE *)&adcSpiInst);

	SPI_DisableAutoSS(adcSpiInst.spiCtx);
	adcSpiInst.pdmaCtx = NULL;
	
	//if(differential == DIFFERENTIAL_MODE)
	//	channels = 4;
	ad7124Initial(channels, differential, &chipIdTemp);
	dbgMsg("adc spi chip id:%X\r\n", chipIdTemp);
	if(chipId != NULL)
		*chipId = chipIdTemp;
	return 0;
}

int closeAdcSpi(void)
{	if(adcSpiInst.spiCtx == NULL)
		return 0;
	SPI_Close(adcSpiInst.spiCtx);
	CLK_DisableModuleClock(SENS_ADC_SPI_MODULE);
	adcSpiInst.spiCtx = NULL;
	adcSpiInst.pdmaCtx = NULL;
	return 0;
}

int waitAdcRdy(uint8_t channel)
{	return ad7124WaitForConvertReady(channel, 100);	//normal 80ms
}

#if SINGLE_CONVERSION_SINGLE_CHANNEL_EN == 1
float ad7124ReadChannelValue(uint8_t channel)
{	int adcData;
	float ret = 0;
	AI_CONFIG *aiCfg = (AI_CONFIG *)&sysCfg->aiCfg;
	ADC_REG_INFO *regInf;
	uint8_t isBipolar, gainVal;
	int Rsens = 62;	// 62??
	int Vsens = 6;	// 20K/120K
	char floatValStr[30];
	uint8_t dataStatus;

	regInf = ad7124Dev.adcRegInfo;

	// dbgMsg("%s", "====================================================\r\n");
	
	// dbg_msg("Sensor Type [0]:%d, Sensor Type [1]:%d, Sensor Type [2]:%d, Sensor Type [3]:%d, Sensor Type [4]:%d\r\n", \
	// 								aiCfg->sensorType[0], aiCfg->sensorType[1], aiCfg->sensorType[2], aiCfg->sensorType[3], aiCfg->sensorType[4]);
	// dbg_msg("AI_TYPE_4_20MA : %d\r\n \
	//                     AI_TYPE_0_20MA : %d\r\n \
	//                     AI_TYPE_20MA_BIPOLAR : %d\r\n \
	//                     AI_TYPE_500MV : %d\r\n \
	//                     AI_TYPE_5V : %d\r\n \
	//                     AI_TYPE_10V : %d\r\n \
	//                     AI_TYPE_6V_BIPOLAR : %d\r\n", \
	//                 AI_TYPE_4_20MA, AI_TYPE_0_20MA, AI_TYPE_20MA_BIPOLAR, AI_TYPE_500MV, AI_TYPE_5V, AI_TYPE_10V, AI_TYPE_6V_BIPOLAR);

	if((aiCfg->differential == DIFFERENTIAL_MODE) && (channel == 4))
		return 0;
	// dbgMsg("[ADC] convert channel %d start\r\n", channel);
	enableSpiAdcIndicateChannel(channel, &isBipolar, &gainVal);
	if(waitAdcRdy(channel))
	{	dbg_msg("%s", "ADC not ready or error\r\n");
		// return 0.0f;
	}
	adcReadDataReg(&adcData);
	if(ad7124Dev.errorOccurred)
	{	dbg_msg("%s", "ADC Error Occure!!\r\n");
		// return 0.0f;
	}
	dbgMsg("[ADC] convert channel %d Done\r\n", channel);
#if 1
	if(channel < 3)	//current mode
	{	if(isBipolar == BIPOLAR_MODE)
		{	ret = ((((((float)adcData / (1ul << 23)) - 1ul) * 2.5f) / gainVal) / Rsens) * 1000;
		}	
		else
		{	ret = ((((float)adcData / (1ul << 24)) * 2.5f / gainVal) / Rsens) * 1000;
		}
		dbg_msg("[CH %d] ADC Data: %d, AI Data: %s mA, MODE: %s, Gain: %d\r\n", channel, adcData, \
										my_ftoa(floatValStr, ret, 3), isBipolar == BIPOLAR_MODE ? "Bipolar" : "Unipolar", gainVal);
	}
	else
	{	if(isBipolar == BIPOLAR_MODE)
			ret = ((((float)adcData / (1ul << 23) - 1ul) * 2.5f) / gainVal) * Vsens;
		else	// unipolar
			ret = (((float)adcData / (1ul << 24)) * 2.5f / gainVal) * Vsens;
		
		dbg_msg("[CH %d] ADC Data: %d, AI Data: %s V, MODE: %s, Gain: %d\r\n", channel, adcData, \
											my_ftoa(floatValStr, ret, 3), isBipolar == BIPOLAR_MODE ? "Bipolar" : "Unipolar", gainVal);
	}
#else	// voltage
	{	if(isBipolar == BIPOLAR_MODE)
			ret = (((float)adcData / (1ul << 23) - 1ul) * 2.5f) / gainVal;
		else
			ret = ((float)adcData / (1ul << 24)) * 2.5f / gainVal;
		
		dbg_msg("[CH %d] ADC Data: %d, AI Data: %s V, MODE: %s, Gain: %d\r\n", channel, adcData, \
											my_ftoa(floatValStr, ret, 3), isBipolar == BIPOLAR_MODE ? "Bipolar" : "Unipolar", gainVal);
	}
#endif

	return ret;
}
#elif SINGLE_CONVERSION_SEVERAL_CHANNELS_EN == 1
int ad7124ReadChannelValue(uint8_t channel, float *aiVal)
{	int adcData[5], ret;
	// int ret = 0;
	// float aiVal[8];
	uint8_t isBipolar[5], gainVal[5];
	int Rsens = 62;	// 62Ω
	uint16_t idx = 0;
#if TEST_MODE == 1
	int maxChannel = 5;
	float multiple = 110.1f;
#else
	int maxChannel = 4;
	float multiple = 55.05f;
#endif
	char floatValStr[30];
	uint8_t dataStatus;

	AI_CONFIG *aiCfg = (AI_CONFIG *)&sysCfg->aiCfg;
	// AD7124_VAL *aiVal = (AD7124_VAL*)&adcVal->channelValue;

	dbgMsg("%s", "====================================================\r\n");

	if((aiCfg->differential == DIFFERENTIAL_MODE) && (channel == 4))
		return -1;

	dbgMsg("[ADC] convert channel total %d start\r\n", channel);
	enableSpiAdcIndicateChannel(channel, &isBipolar[0], &gainVal[0]);

	// SENS_TIME_DELAY(150);
	for(idx=0; idx<channel; idx++)
	{	if(waitAdcRdy(idx))
		{	dbg_msg("[CH %d] ADC not ready or error.\r\n", idx);
		}
		ret = adcReadDataReg(&adcData[idx]);
		
		dataStatus = (adcData[idx] & 0xFF000000) >> 24;
		adcData[idx] = adcData[idx] & 0x00FFFFFF;

		dbg_msg("[CH %d] Data Status: 0x%02x, ADC Data: %d\r\n", idx, dataStatus, adcData[idx]);

		if(idx < 3)
		{
#if TEST_ADC_TYPE == 0	//current
			if(isBipolar[idx] == BIPOLAR_MODE) // bipolar
				aiVal[idx] = ((((((float)adcData[idx] / (1ul << 23)) - 1ul) * 2.5f)/ (1 << gainVal[idx])) / Rsens) * 1000;
			else	// unipolar
				aiVal[idx] = ((((float)adcData[idx] / (1ul << 24)) * 2.5f / (1 << gainVal[idx])) / Rsens) * 1000;
#else
			dbg_msg("[CH %d] Data Status: 0x%02x, ADC Data: %d, Gain: %d\r\n", idx, dataStatus, adcData[idx], (1 << gainVal[idx]));		//voltage
			if(isBipolar[idx] == BIPOLAR_MODE)	// bipolar
				aiVal[idx] = ((((float)adcData[idx] / (1ul << 23)) * 2.5f) - 1ul)  / (1 << gainVal[idx]);
			else	// unipolar
				aiVal[idx] = (((float)adcData[idx] / (1ul << 24)) * 2.5f) / (1 << gainVal[idx]);
#endif
		}
		else if(idx < channel)
		{ if(isBipolar[idx] == BIPOLAR_MODE)
				aiVal[idx] = ((((float)adcData[idx] / (1ul << 23)) * 2.5f) - 1ul) / (1 << gainVal[idx]);
			else	// unipolar
				aiVal[idx] = ((float)adcData[idx] / (1ul << 24)) * 2.5f / (1 << gainVal[idx]);
		}
		// if(ad7124Dev.errorOccurred)
		// if(ret)
		// {	dbg_msg("[CH %d] ADC Error Occure!!\r\n", idx);
		// 	aiVal[idx] = 0.0f;
		// }
	}
	
	dbgMsg("%s", "ADC Value: [");
	for(idx=0; idx<channel; idx++)
	{
		if(idx < 3)
		{
#if TEST_ADC_TYPE == 0
			dbgMsg1("%s mA", my_ftoa(floatValStr, aiVal[idx], 3));

			if(idx + 1 < channel)
				dbg_msg1("%s", ", ");
#else
			dbgMsg1("%s V", my_ftoa(floatValStr, aiVal[idx], 3));

			if(idx + 1 < channel)
				dbg_msg1("%s", ", ");
#endif
		}
		else if(idx < channel)
		{
			dbgMsg1("%s V", my_ftoa(floatValStr, aiVal[idx], 3));

			if(idx + 1 < channel)
				dbg_msg1("%s", ", ");
		}
	}
	dbgMsg1("%s", "]\r\n");

	return ret;
}
#endif