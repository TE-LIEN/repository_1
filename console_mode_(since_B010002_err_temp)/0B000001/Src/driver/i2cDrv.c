#include "driver/i2cDrv.h"

static volatile uint8_t s_u8TimeoutFlag = 0;

typedef void (*I2C_FUNC)(uint8_t ch, uint32_t u32Status);

typedef struct _I2C_INSTANCE
{	uint8_t initialDone;
	uint8_t isWordSizeAddress;
	uint8_t devAddr;
	volatile uint8_t timeoutFlag;
	volatile uint8_t	endFlag;
	volatile uint8_t txAbortFlag;
	volatile uint8_t rxAbortFlag;
	uint8_t reStartFlag;
	uint8_t errorRetryCnt;
	uint8_t readMode;	//0: STA->SLA+W->ADDR->SLA+R->DATA, 1: STA->SLA+R->DATA
	uint16_t xmitLen;
	uint16_t totalXmitLen;
	uint8_t *buffer;
	uint32_t bufferSize;
	uint32_t clkSpeed;
	I2C_FUNC handler;
	I2C_T	*reg;
	SENS_SEM_STRUCT	lock;
}I2C_INSTANCE;

volatile I2C_INSTANCE gI2cInst[3];


void I2C_MasterRx(uint8_t ch, uint32_t u32Status)
{	I2C_INSTANCE *i2cInst = (I2C_INSTANCE *)&gI2cInst[ch];
	
	if(u32Status == 0x08)                       /* START has been transmitted and prepare SLA+W */	
	{	if(i2cInst->readMode == I2C_READ_MODE_FIRST_ISSUE_SLA_R)
		{	I2C_SET_DATA(i2cInst->reg, (uint32_t)((i2cInst->devAddr << 1) | 0x01));	/* Write SLA+R to Register I2CDAT */
		}
		else 
		{	I2C_SET_DATA(i2cInst->reg, (uint32_t)(i2cInst->devAddr << 1));    /* Write SLA+W to Register I2CDAT */
		}
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
	}
	else if(u32Status == 0x18)                  /* SLA+W has been transmitted and ACK has been received */
	{	I2C_SET_DATA(i2cInst->reg, i2cInst->buffer[i2cInst->xmitLen++]);
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
	}
	else if(u32Status == 0x20)                  /* SLA+W has been transmitted and NACK has been received */
	{	I2C_STOP(i2cInst->reg);
		I2C_START(i2cInst->reg);
	}
	else if(u32Status == 0x28)                  /* DATA has been transmitted and ACK has been received */
	{	if((i2cInst->xmitLen != 2) && i2cInst->isWordSizeAddress)
		{	I2C_SET_DATA(i2cInst->reg, i2cInst->buffer[i2cInst->xmitLen++]);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else
		{	i2cInst->xmitLen = 0;
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STA_SI);
		}
	}
	else if(u32Status == 0x10)                  /* Repeat START has been transmitted and prepare SLA+R */
	{	I2C_SET_DATA(i2cInst->reg, (uint32_t)((i2cInst->devAddr << 1) | 0x01));   /* Write SLA+R to Register I2CDAT */
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
	}
	else if(u32Status == 0x40)                  /* SLA+R has been transmitted and ACK has been received */
	{	if((i2cInst->xmitLen + 1) < i2cInst->totalXmitLen)
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI | I2C_CTL_AA);
		else
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
	}
	else if(u32Status == 0x58)                  /* DATA has been received and NACK has been returned */
	{	i2cInst->buffer[i2cInst->xmitLen++] = (unsigned char) I2C_GET_DATA(i2cInst->reg);
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		i2cInst->endFlag = 1;
	}
	else if(u32Status == 0x50)
	{	i2cInst->buffer[i2cInst->xmitLen++] = I2C_GET_DATA(i2cInst->reg);
		if((i2cInst->xmitLen + 1) < i2cInst->totalXmitLen)
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI | I2C_CTL_AA); //Send ACK to get next data
		}
		else
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
#if 0
		if(i2cInst->xmitLen<i2cInst->totalXmitLen)
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI | I2C_CTL_AA); //Send ACK to get next data
		else
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI); //Send NACK & STOP
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
			i2cInst->endFlag = 1;
			//tmp = g_u16MasterDataLen;
		}
#endif
	}
	else
	{	/* Error condition process */
		//printf("[MasterRx] Status [0x%x] Unexpected abort!! Anykey to re-start\n", u32Status);
		if(u32Status == 0x38)                 /* Master arbitration lost, stop I2C and clear SI */
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else if(u32Status == 0x30)            /* Master transmit data NACK, stop I2C and clear SI */
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else if(u32Status == 0x48)            /* Master receive address NACK, stop I2C and clear SI */
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else if(u32Status == 0x00)            /* Master bus error, stop I2C and clear SI */
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		/*Setting MasterRx abort flag for re-start mechanism*/
		i2cInst->rxAbortFlag = 1;
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		while(i2cInst->reg->CTL0 & I2C_CTL0_SI_Msk);
	}
}
/*---------------------------------------------------------------------------------------------------------*/
/*  I2C Tx Callback Function                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void I2C_MasterTx(uint8_t ch, uint32_t u32Status)
{	I2C_INSTANCE *i2cInst = (I2C_INSTANCE *)&gI2cInst[ch];
	if(u32Status == 0x08)                       /* START has been transmitted */
	{	I2C_SET_DATA(i2cInst->reg, (uint32_t)(i2cInst->devAddr << 1));    /* Write SLA+W to Register I2CDAT */
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
	}
	else if(u32Status == 0x18)                  /* SLA+W has been transmitted and ACK has been received */
	{	I2C_SET_DATA(i2cInst->reg, i2cInst->buffer[i2cInst->xmitLen++]);
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
	}
	else if(u32Status == 0x20)                  /* SLA+W has been transmitted and NACK has been received */
	{	I2C_STOP(i2cInst->reg);
		I2C_START(i2cInst->reg);
	}
	else if(u32Status == 0x28)                  /* DATA has been transmitted and ACK has been received */
	{	if(i2cInst->xmitLen < i2cInst->totalXmitLen)
		{	I2C_SET_DATA(i2cInst->reg, i2cInst->buffer[i2cInst->xmitLen++]);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
			i2cInst->endFlag = 1;
		}
	}
	else
	{	/* Error condition process */
		//printf("[MasterTx] Status [0x%x] Unexpected abort!! Anykey to re-start\n", u32Status);
		if(u32Status == 0x38)                   /* Master arbitration lost, stop I2C and clear SI */
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else if(u32Status == 0x00)              /* Master bus error, stop I2C and clear SI */
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else if(u32Status == 0x30)              /* Master transmit data NACK, stop I2C and clear SI */
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else if(u32Status == 0x48)              /* Master receive address NACK, stop I2C and clear SI */
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else if(u32Status == 0x10)              /* Master repeat start, clear SI */
		{	I2C_SET_DATA(i2cInst->reg, (uint32_t)((i2cInst->devAddr << 1) | 0x01));   /* Write SLA+R to Register I2CDAT */
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		else
		{	I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STO_SI);
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		}
		/*Setting MasterTRx abort flag for re-start mechanism*/
		i2cInst->txAbortFlag = 1;
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_SI);
		while(i2cInst->reg->CTL0 & I2C_CTL0_SI_Msk);
	}
}


#if I2C0_IO_USED
void I2C0_IRQHandler(void)
{	uint32_t u32Status;
	I2C_INSTANCE *i2cInst;
	
	i2cInst = (I2C_INSTANCE *)&gI2cInst[I2C0_ID];
	
	u32Status = I2C_GET_STATUS(i2cInst->reg);

	if(I2C_GET_TIMEOUT_FLAG(i2cInst->reg))
	{	/* Clear I2C0 Timeout Flag */
		I2C_ClearTimeoutFlag(i2cInst->reg);
		i2cInst->timeoutFlag = 1;
	}
	else
	{	if(i2cInst->handler != NULL)
			i2cInst->handler(I2C0_ID, u32Status);
	}
}
#endif

#if I2C1_IO_USED
void I2C1_IRQHandler(void)
{	uint32_t u32Status;
	I2C_INSTANCE *i2cInst;
	
	i2cInst = (I2C_INSTANCE *)&gI2cInst[I2C1_ID];
	
	u32Status = I2C_GET_STATUS(i2cInst->reg);

	if(I2C_GET_TIMEOUT_FLAG(i2cInst->reg))
	{	/* Clear I2C0 Timeout Flag */
		I2C_ClearTimeoutFlag(i2cInst->reg);
		i2cInst->timeoutFlag = 1;
	}
	else
	{	if(i2cInst->handler != NULL)
			i2cInst->handler(I2C1_ID, u32Status);
	}
}
#endif

#if I2C2_IO_USED
void I2C2_IRQHandler(void)
{	uint32_t u32Status;
	I2C_INSTANCE *i2cInst;
	
	i2cInst = (I2C_INSTANCE *)&gI2cInst[I2C2_ID];
	
	u32Status = I2C_GET_STATUS(i2cInst->reg);

	if(I2C_GET_TIMEOUT_FLAG(i2cInst->reg))
	{	/* Clear I2C0 Timeout Flag */
		I2C_ClearTimeoutFlag(i2cInst->reg);
		i2cInst->timeoutFlag = 1;
	}
	else
	{	if(i2cInst->handler != NULL)
			i2cInst->handler(I2C2_ID, u32Status);
	}
}
#endif

void setI2cGpioMux(uint8_t ch)
{	SYS_UnlockReg();

#if I2C0_IO_USED	
	if(ch == I2C0_ID)
	{	//SYS->GPC_MFPL = (SYS->GPC_MFPL & (~(I2C0_SCL_PC1_Msk | I2C0_SDA_PC0_Msk))) | I2C0_SCL_PC1 | I2C0_SDA_PC0;
		//PC->SMTEN |= (GPIO_SMTEN_SMTEN0_Msk | GPIO_SMTEN_SMTEN1_Msk);
	}
#endif
	
#if I2C1_IO_USED
	if(ch == I2C1_ID)
	{
	}
#endif
	
#if I2C2_IO_USED
	if(ch == I2C2_ID)
	{
	}
#endif
}

void i2cIpInit(uint8_t ch, I2C_INSTANCE *i2cInst)
{	I2C_T *i2cCtx = i2cInst->reg;
	
	I2C_Open(i2cCtx, i2cInst->clkSpeed);
	I2C_EnableInt(i2cCtx);
	if(ch == I2C0_ID)
	{	
#ifdef SENSMINIA4_NEO
		NVIC_SetPriority(I2C0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
#endif
		NVIC_EnableIRQ(I2C0_IRQn);
	}
	else if(ch == I2C1_ID)
	{	
#ifdef SENSMINIA4_NEO
		NVIC_SetPriority(I2C1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
#endif
		NVIC_EnableIRQ(I2C1_IRQn);
	}
	else if(ch == I2C2_ID)
	{	
#ifdef SENSMINIA4_NEO
		NVIC_SetPriority(I2C2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
#endif
		NVIC_EnableIRQ(I2C2_IRQn);
	}
}

void i2cIpReset(uint8_t ch)
{	if(ch == I2C0_ID)
	{	SYS->IPRST1 |= SYS_IPRST1_I2C0RST_Msk;
		SYS->IPRST1 = 0;
	}
	else if(ch == I2C1_ID)
	{	SYS->IPRST1 |= SYS_IPRST1_I2C1RST_Msk;
		SYS->IPRST1 = 0;
	}
	else if(ch == I2C2_ID)
	{	SYS->IPRST1 |= SYS_IPRST1_I2C2RST_Msk;
		SYS->IPRST1 = 0;
	}
}

int i2cInit(uint8_t ch, uint32_t speed, uint32_t bufferSize)
{	I2C_T *i2cCtx;
	I2C_INSTANCE *i2cInst;
	
	i2cInst = (I2C_INSTANCE *)&gI2cInst[ch];
	if(i2cInst->initialDone)
		return 0;
	
	memset(i2cInst, 0, sizeof(I2C_INSTANCE));
	
	if(SENS_SEM_INIT(i2cInst->lock, 1) != MQX_OK)
	{	return -1;
	}
	
	if(ch == I2C0_ID)
	{	
#if I2C0_IO_USED		
		i2cCtx = I2C0;
	#ifdef SENSMINIA4_NEO
		SET_I2C0_SCL_PB9();
		SET_I2C0_SDA_PB8();
		PB->SMTEN |= GPIO_SMTEN_SMTEN8_Msk | GPIO_SMTEN_SMTEN9_Msk;
	#endif
		CLK_EnableModuleClock(I2C0_MODULE);
#else
		return -1;
#endif
	}
	else if(ch == I2C1_ID)
	{	
#if I2C1_IO_USED
		i2cCtx = I2C1;
	#ifdef SENSMINIA4_NEO
		SET_I2C1_SCL_PE1();
		SET_I2C1_SDA_PE0();
		PE->SMTEN |= GPIO_SMTEN_SMTEN0_Msk | GPIO_SMTEN_SMTEN1_Msk;
	#endif
		CLK_EnableModuleClock(I2C1_MODULE);
#else
		return -1;
#endif
	}
	else if(ch == I2C2_ID)
	{
#if I2C2_IO_USED
		i2cCtx = I2C2;
	#ifdef SENSMINIA4_NEO
		SET_I2C2_SCL_PD1();
		SET_I2C2_SDA_PD0();
		PD->SMTEN |= GPIO_SMTEN_SMTEN0_Msk | GPIO_SMTEN_SMTEN1_Msk;
	#endif
		CLK_EnableModuleClock(I2C2_MODULE);
#else
		return -1;
#endif
	}
	
	i2cInst->reg = i2cCtx;
	i2cInst->buffer = SENS_MEM_ZALLOC(bufferSize + 2);	//add reg address
	i2cInst->clkSpeed = speed;
	i2cIpInit(ch, i2cInst);
	
	i2cInst->initialDone = 1;
	return 0;
}

int i2cDeInit(uint8_t ch)
{	I2C_T *i2cCtx;
	I2C_INSTANCE *i2cInst;
	
	i2cInst = (I2C_INSTANCE *)&gI2cInst[ch];
	if(!i2cInst->initialDone)
		return 0;
	
	I2C_DisableInt(i2cInst->reg);
	if(ch == I2C0_ID)
		NVIC_DisableIRQ(I2C0_IRQn);
	else if(ch == I2C1_ID)
		NVIC_DisableIRQ(I2C1_IRQn);
	else if(ch == I2C2_ID)
		NVIC_DisableIRQ(I2C2_IRQn);
	
	I2C_Close(i2cInst->reg);
	
	if(ch == I2C0_ID)
		CLK_DisableModuleClock(I2C0_MODULE);
	else if(ch == I2C1_ID)
		CLK_DisableModuleClock(I2C1_MODULE);
	else if(ch == I2C2_ID)
		CLK_DisableModuleClock(I2C2_MODULE);
	
	vSemaphoreDelete(i2cInst->lock);
	SENS_MEM_FREE(i2cInst->buffer);
	memset(i2cInst, 0, sizeof(I2C_INSTANCE));
	return 0;
}

void setI2cIsWordsizeAddressing(uint8_t ch, uint8_t isWordSize)
{	I2C_INSTANCE *i2cInst;
	i2cInst = (I2C_INSTANCE *)&gI2cInst[ch];
	i2cInst->isWordSizeAddress = isWordSize;
}

int i2cWrite(uint8_t ch, uint8_t sla, uint16_t address, uint8_t *data, uint32_t len)
{	//volatile I2C_INSTANCE *i2cInst;
	I2C_INSTANCE *i2cInst;
	uint8_t *ptr;
	uint32_t idx;
	
	i2cInst = (I2C_INSTANCE *)&gI2cInst[ch];
	if(!i2cInst->initialDone)
		return -1;
	
	SENS_SEM_LOCK(i2cInst->lock);
	i2cInst->devAddr = sla;
	ptr = i2cInst->buffer;
	if(i2cInst->isWordSizeAddress)
		*ptr++ = address >> 8;
	*ptr++ = address & 0xFF;
	i2cInst->totalXmitLen = len + 1 + i2cInst->isWordSizeAddress;
	for(idx=0;idx<len;idx++)
		*ptr++ = data[idx];
	
	i2cInst->errorRetryCnt = 0;
	
	do
	{	I2C_EnableTimeout(i2cInst->reg, 0);
		i2cInst->handler = (I2C_FUNC)I2C_MasterTx;
		i2cInst->endFlag = 0;
		i2cInst->xmitLen = 0;
		i2cInst->timeoutFlag = 0;
		i2cInst->txAbortFlag = 0;
		i2cInst->reStartFlag = 0;
		I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STA);
	
		while(1)
		{	if(i2cInst->timeoutFlag)
			{	i2cIpReset(ch);
				i2cIpInit(ch, (I2C_INSTANCE *)i2cInst);
				i2cInst->txAbortFlag = 1;
			}
			if(i2cInst->endFlag || i2cInst->txAbortFlag)
				break;
		}

		if(i2cInst->txAbortFlag)
		{	i2cInst->errorRetryCnt++;
			if(i2cInst->errorRetryCnt > 2)
				break;
			i2cInst->txAbortFlag = 0;
			i2cInst->reStartFlag = 1;
		}
	}while(i2cInst->reStartFlag);
	

	if(i2cInst->errorRetryCnt > 2)
	{	SENS_SEM_UNLOCK(i2cInst->lock);
		return -1;
	}
	SENS_SEM_UNLOCK(i2cInst->lock);
	return i2cInst->xmitLen - (i2cInst->isWordSizeAddress ? 2:1);
}

int i2cRead(uint8_t ch, uint8_t sla, uint16_t address, uint8_t *data, uint32_t len, uint8_t readMode)
{	//volatile I2C_INSTANCE *i2cInst;
	I2C_INSTANCE *i2cInst;
	uint8_t *ptr;
	uint32_t idx;
	uint16_t recvLen, offset;
	uint16_t totalRecvLen=0;
	
	i2cInst = (I2C_INSTANCE *)&gI2cInst[ch];
	if(!i2cInst->initialDone)
		return -1;
	SENS_SEM_LOCK(i2cInst->lock);
	i2cInst->errorRetryCnt = 0;	
	offset = 0;
	while(len)
	{	i2cInst->devAddr = sla;
		ptr = i2cInst->buffer;
		if(readMode != I2C_READ_MODE_FIRST_ISSUE_SLA_R)
		{	if(i2cInst->isWordSizeAddress)
				*ptr++ = address >> 8;
			*ptr++ = address & 0xFF;
		}
		recvLen = len;
#ifdef SENSMINIS2
		if(address & 0x03)
			recvLen = 1;
#endif
		//recvLen = 1;
		i2cInst->readMode = readMode;
		i2cInst->totalXmitLen = recvLen;
		do
		{	I2C_EnableTimeout(i2cInst->reg, 0);
			i2cInst->handler = (I2C_FUNC)I2C_MasterRx;
			i2cInst->endFlag = 0;
			i2cInst->xmitLen = 0;
			i2cInst->timeoutFlag = 0;
			i2cInst->rxAbortFlag = 0;
			i2cInst->reStartFlag = 0;
			I2C_SET_CONTROL_REG(i2cInst->reg, I2C_CTL_STA);
		
			while(1)
			{	if(i2cInst->timeoutFlag)
				{	i2cIpReset(ch);
					i2cIpInit(ch, (I2C_INSTANCE *)i2cInst);
					i2cInst->rxAbortFlag = 1;
				}
				if(i2cInst->endFlag || i2cInst->rxAbortFlag)
					break;
			}
			if(i2cInst->rxAbortFlag)
			{	i2cInst->errorRetryCnt++;
				if(i2cInst->errorRetryCnt > 2)
					break;
				i2cInst->rxAbortFlag = 0;
				i2cInst->reStartFlag = 1;
			}
		}while(i2cInst->reStartFlag);
		
		if(i2cInst->errorRetryCnt <= 2)
		{	ptr = i2cInst->buffer;
			for(idx=0;idx<i2cInst->xmitLen;idx++)
				data[idx + offset] = *ptr++;
			
			offset += recvLen;
			address += recvLen;
			len -= recvLen;
			totalRecvLen += i2cInst->xmitLen;
		}
		else
			break;
	}
	
	if(i2cInst->errorRetryCnt > 2)
	{	SENS_SEM_UNLOCK(i2cInst->lock);
		return -1;
	}
	SENS_SEM_UNLOCK(i2cInst->lock);
	return totalRecvLen;
}
