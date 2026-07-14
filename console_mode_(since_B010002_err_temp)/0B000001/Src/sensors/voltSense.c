#include "sensors/sensorApp.h"
#include "sensDev.h"
#include "physicalQuantity.h"


volatile uint8_t gIrqDoneFlag;
//------------------------------------------------------------------------------
void voltSenseReStart(void)
{	gIrqDoneFlag = 0;
#if DETECT_SOLAR_I && DETECT_LOAD_I
	EADC_START_CONV(EADC2, (BIT0 << I_SOLAR_SENSOR_EADC_SAMPLE_NO)| (BIT0 << V_BAT_SENSOR_EADC_SAMPLE_NO) | (BIT0 << V_SOLAR_SENSOR_EADC_SAMPLE_NO) | (BIT0 << I_LOAD_SENSOR_EADC_SAMPLE_NO));
#elif DETECT_SOLAR_I
	EADC_START_CONV(EADC2, (BIT0 << I_SOLAR_SENSOR_EADC_SAMPLE_NO)| (BIT0 << V_BAT_SENSOR_EADC_SAMPLE_NO) | (BIT0 << V_SOLAR_SENSOR_EADC_SAMPLE_NO));
#elif DETECT_LOAD_I
	EADC_START_CONV(EADC2, (BIT0 << V_BAT_SENSOR_EADC_SAMPLE_NO) | (BIT0 << V_SOLAR_SENSOR_EADC_SAMPLE_NO) | (BIT0 << I_LOAD_SENSOR_EADC_SAMPLE_NO));
#else
	EADC_START_CONV(EADC2, (BIT0 << V_BAT_SENSOR_EADC_SAMPLE_NO) | (BIT0 << V_SOLAR_SENSOR_EADC_SAMPLE_NO));
#endif
}
//------------------------------------------------------------------------------
void getVolt(uint8_t sampleNo)
{	VI_SENSE_INSTANCE *viSenseInst;
	VI_SENSE_CONTEXT *ctx = NULL;
	uint32_t value;
	
	viSenseInst = (VI_SENSE_INSTANCE *)&sensDev->voltCurrSenseInst;
	
#if DETECT_SOLAR_I
	if(sampleNo == I_SOLAR_SENSOR_EADC_SAMPLE_NO)
	{	ctx = &viSenseInst->solarI;
		value = (float)EADC_GET_CONV_DATA(EADC2, I_SOLAR_SENSOR_EADC_SAMPLE_NO);
	}
#endif
#if DETECT_LOAD_I
	if(sampleNo == I_SOLAR_SENSOR_EADC_SAMPLE_NO)
	{	ctx = &viSenseInst->loadI;
		value = (float)EADC_GET_CONV_DATA(EADC2, I_LOAD_SENSOR_EADC_SAMPLE_NO);
	}
#endif
	
	if(sampleNo == V_BAT_SENSOR_EADC_SAMPLE_NO)
	{	ctx = &viSenseInst->batV;
		value = EADC_GET_CONV_DATA(EADC2, V_BAT_SENSOR_EADC_SAMPLE_NO);
	}
	if(sampleNo == V_SOLAR_SENSOR_EADC_SAMPLE_NO)
	{	ctx = &viSenseInst->solarV;
		value = EADC_GET_CONV_DATA(EADC2, V_SOLAR_SENSOR_EADC_SAMPLE_NO);
	}
	if(ctx)
	{	ctx->value[ctx->count] = value;
		ctx->count++;
		if(ctx->count >= ADC_AVERAGE_COUNT)
		{	ctx->valueIsReady = 1;
			viSenseInst->readyFlag |= ( 1<<sampleNo );
		}
		viSenseInst->getOnceFlag |= ( 1<<sampleNo );
	}
	
	if((viSenseInst->readyFlag & ALL_ADC_DONE) == ALL_ADC_DONE)
	{	viSenseInst->isReady = 1;
	}
	else if((viSenseInst->getOnceFlag & ALL_ADC_DONE) == ALL_ADC_DONE)
	{	viSenseInst->getOnceFlag = 0;
		voltSenseReStart();
	}
}
//------------------------------------------------------------------------------
#if DETECT_SOLAR_I
void EADC22_IRQHandler(void)
{	//g_u32AdcIntFlag = 1;
	/* Clear the A/D ADINT0 interrupt flag */
	gIrqDoneFlag |= BIT0 << I_SOLAR_SENSOR_EADC_SAMPLE_NO;
	EADC_CLR_INT_FLAG(EADC2, EADC_STATUS2_ADIF2_Msk);
	/*if(EADC_GET_DATA_VALID_FLAG(EADC2, BIT0 << I_SOLAR_SENSOR_EADC_SAMPLE_NO))
	{	getVolt(I_SOLAR_SENSOR_EADC_SAMPLE_NO);
	}*/
}
#endif
//------------------------------------------------------------------------------
#if DETECT_LOAD_I
void EADC23_IRQHandler(void)
{	//g_u32AdcIntFlag = 1;
	/* Clear the A/D ADINT0 interrupt flag */
	gIrqDoneFlag |= BIT0 << I_LOAD_SENSOR_EADC_SAMPLE_NO;
	EADC_CLR_INT_FLAG(EADC2, EADC_STATUS2_ADIF3_Msk);
	/*if(EADC_GET_DATA_VALID_FLAG(EADC2, BIT0 << I_LOAD_SENSOR_EADC_SAMPLE_NO))
	{	getVolt(I_LOAD_SENSOR_EADC_SAMPLE_NO);
	}*/
}
#endif
//------------------------------------------------------------------------------
void EADC20_IRQHandler(void)	//solar V
{	//g_u32AdcIntFlag = 1;
	/* Clear the A/D ADINT0 interrupt flag */
	gIrqDoneFlag |= BIT0 << V_SOLAR_SENSOR_EADC_SAMPLE_NO;
	EADC_CLR_INT_FLAG(EADC2, EADC_STATUS2_ADIF0_Msk);
	/*if(EADC_GET_DATA_VALID_FLAG(EADC2, BIT0 << V_SOLAR_SENSOR_EADC_SAMPLE_NO))
	{	getVolt(V_SOLAR_SENSOR_EADC_SAMPLE_NO);
	}*/
}
//------------------------------------------------------------------------------
void EADC21_IRQHandler(void)	//bat V
{	//g_u32AdcIntFlag = 1;
	/* Clear the A/D ADINT0 interrupt flag */
	gIrqDoneFlag |= BIT0 << V_BAT_SENSOR_EADC_SAMPLE_NO;
	EADC_CLR_INT_FLAG(EADC2, EADC_STATUS2_ADIF1_Msk);
	/*if(EADC_GET_DATA_VALID_FLAG(EADC2, BIT0 << V_BAT_SENSOR_EADC_SAMPLE_NO))
	{	getVolt(V_BAT_SENSOR_EADC_SAMPLE_NO);
	}*/
}
//------------------------------------------------------------------------------
void voltSenseInit(void)
{	
	SYS_UnlockReg();
	
	PIN_VSENSE_EN = 1;
	SENS_TIME_DELAY(100);
	
	CLK_EnableModuleClock(EADC2_MODULE);
	CLK_SetModuleClock(EADC2_MODULE, CLK_CLKSEL0_EADC2SEL_PLL_DIV2, CLK_CLKDIV5_EADC2(12));
	
	SET_EADC2_CH6_PA10();	//bat V
	SET_EADC2_CH5_PA9();	//solar V
	
#if DETECT_SOLAR_I
	SET_EADC2_CH13_PC12();	//solar I
#endif
#if DETECT_LOAD_I
	SET_EADC2_CH12_PC11();	//load I
#endif
	GPIO_DISABLE_DIGITAL_PATH(PA, BIT9 | BIT10);
#if DETECT_SOLAR_I
	GPIO_DISABLE_DIGITAL_PATH(PC, BIT12);
#endif	
#if DETECT_LOAD_I
	GPIO_DISABLE_DIGITAL_PATH(PC, BIT11);
#endif

	//EADC2->CTL1 &= ~EADC_CTL1_RESSEL_Msk;
	//EADC2->CTL1 |= (ADC_RESOLUTION_VAL << EADC_CTL1_RESSEL_Pos);

	EADC_Open(EADC2, EADC_CTL_DIFFEN_SINGLE_END);
	
#if DETECT_SOLAR_I
	EADC_ConfigSampleModule(EADC2, I_SOLAR_SENSOR_EADC_SAMPLE_NO, EADC_SOFTWARE_TRIGGER, I_SOLAR_SENSOR_EADC_CH);
#endif
#if DETECT_LOAD_I
	EADC_ConfigSampleModule(EADC2, I_LOAD_SENSOR_EADC_SAMPLE_NO, EADC_SOFTWARE_TRIGGER, I_LOAD_SENSOR_EADC_CH);
#endif
	EADC_ConfigSampleModule(EADC2, V_BAT_SENSOR_EADC_SAMPLE_NO, EADC_SOFTWARE_TRIGGER, V_BAT_SENSOR_EADC_CH);
	EADC_ConfigSampleModule(EADC2, V_SOLAR_SENSOR_EADC_SAMPLE_NO, EADC_SOFTWARE_TRIGGER, V_SOLAR_SENSOR_EADC_CH);	
	
#if ENABLE_TRIGGER_DELAY
#if DETECT_SOLAR_I
	EADC_SetTriggerDelayTime(EADC2, I_SOLAR_SENSOR_EADC_SAMPLE_NO, ADC_TRIG_DELAY_CNT, EADC_SCTL_TRGDLYDIV_DIVIDER_16);
#endif
#if DETECT_LOAD_I
	EADC_SetTriggerDelayTime(EADC2, I_LOAD_SENSOR_EADC_SAMPLE_NO, ADC_TRIG_DELAY_CNT, EADC_SCTL_TRGDLYDIV_DIVIDER_16);
#endif
	EADC_SetTriggerDelayTime(EADC2, V_BAT_SENSOR_EADC_SAMPLE_NO, ADC_TRIG_DELAY_CNT, EADC_SCTL_TRGDLYDIV_DIVIDER_16);
	EADC_SetTriggerDelayTime(EADC2, V_SOLAR_SENSOR_EADC_SAMPLE_NO, ADC_TRIG_DELAY_CNT, EADC_SCTL_TRGDLYDIV_DIVIDER_16);
#endif
	
#if DETECT_SOLAR_I
	EADC_SetExtendSampleTime(EADC2, I_SOLAR_SENSOR_EADC_SAMPLE_NO, SENS_ADC_SAMPLING_TIME_EXTEND);
#endif
#if DETECT_LOAD_I
	EADC_SetExtendSampleTime(EADC2, I_LOAD_SENSOR_EADC_SAMPLE_NO, SENS_ADC_SAMPLING_TIME_EXTEND);
#endif
	EADC_SetExtendSampleTime(EADC2, V_BAT_SENSOR_EADC_SAMPLE_NO, SENS_ADC_SAMPLING_TIME_EXTEND);
	EADC_SetExtendSampleTime(EADC2, V_SOLAR_SENSOR_EADC_SAMPLE_NO, SENS_ADC_SAMPLING_TIME_EXTEND);
	
#if 0
#if DETECT_SOLAR_I
	EADC_ENABLE_ACU(EADC2, I_SOLAR_SENSOR_EADC_SAMPLE_NO, EADC_MCTL1_ACU_32);
	EADC_ENABLE_AVG(EADC2, I_SOLAR_SENSOR_EADC_SAMPLE_NO);
#endif
#if DETECT_LOAD_I
	EADC_ENABLE_ACU(EADC2, I_LOAD_SENSOR_EADC_SAMPLE_NO, EADC_MCTL1_ACU_32);
	EADC_ENABLE_AVG(EADC2, I_LOAD_SENSOR_EADC_SAMPLE_NO);
#endif
	EADC_ENABLE_ACU(EADC2, V_BAT_SENSOR_EADC_SAMPLE_NO, EADC_MCTL1_ACU_32);
	EADC_ENABLE_AVG(EADC2, V_BAT_SENSOR_EADC_SAMPLE_NO);
	EADC_ENABLE_ACU(EADC2, V_SOLAR_SENSOR_EADC_SAMPLE_NO, EADC_MCTL1_ACU_32);
	EADC_ENABLE_AVG(EADC2, V_SOLAR_SENSOR_EADC_SAMPLE_NO);
	
#endif
}
//------------------------------------------------------------------------------
static void initViSenseStruct(VI_SENSE_CONTEXT *viSensCtx)
{	viSensCtx->value = SENS_MEM_ZALLOC(sizeof(uint32_t) * ADC_AVERAGE_COUNT);
#if ADC_FILTER_STD
	viSensCtx->valFilter = SENS_MEM_ZALLOC(sizeof(ALOG_STD));
	viSensCtx->pqFilter = SENS_MEM_ZALLOC(sizeof(ALOG_STD));
#endif
}
//------------------------------------------------------------------------------
void voltSenseStart(void)
{	VI_SENSE_INSTANCE *viSenseInst;
	VI_SENSE_CONTEXT *batV;
	VI_SENSE_CONTEXT *solarV;
#if DETECT_SOLAR_I
	VI_SENSE_CONTEXT *solarI;
#endif
#if DETECT_LOAD_I
	VI_SENSE_CONTEXT *loadI;
#endif
	
	viSenseInst = (VI_SENSE_INSTANCE *)&sensDev->voltCurrSenseInst;
	batV = &viSenseInst->batV;
	solarV = &viSenseInst->solarV;
#if DETECT_SOLAR_I
	solarI = &viSenseInst->solarI;
#endif
#if DETECT_LOAD_I
	loadI = &viSenseInst->loadI;
#endif
	
	initViSenseStruct(batV);
	initViSenseStruct(solarV);
#if DETECT_SOLAR_I
	initViSenseStruct(solarI);
#endif
#if DETECT_LOAD_I
	initViSenseStruct(loadI);
#endif
	
#if DETECT_SOLAR_I && DETECT_LOAD_I
	EADC_CLR_INT_FLAG(EADC2, EADC_STATUS2_ADIF0_Msk | EADC_STATUS2_ADIF1_Msk | EADC_STATUS2_ADIF2_Msk | EADC_STATUS2_ADIF3_Msk);
	EADC_ENABLE_INT(EADC2, BIT0 | BIT1 | BIT2 | BIT3);
#elif DETECT_SOLAR_I
	EADC_CLR_INT_FLAG(EADC2, EADC_STATUS2_ADIF0_Msk | EADC_STATUS2_ADIF1_Msk | EADC_STATUS2_ADIF2_Msk);
	EADC_ENABLE_INT(EADC2, BIT0 | BIT1 | BIT2);
#elif DETECT_LOAD_I
	EADC_CLR_INT_FLAG(EADC2, EADC_STATUS2_ADIF0_Msk | EADC_STATUS2_ADIF1_Msk | EADC_STATUS2_ADIF3_Msk);
	EADC_ENABLE_INT(EADC2, BIT0 | BIT1 | BIT3);
#else
	EADC_CLR_INT_FLAG(EADC2, EADC_STATUS2_ADIF0_Msk | EADC_STATUS2_ADIF1_Msk);
	EADC_ENABLE_INT(EADC2, BIT0 | BIT1);
#endif
	
	EADC_ENABLE_SAMPLE_MODULE_INT(EADC2, 0, BIT0 << V_BAT_SENSOR_EADC_SAMPLE_NO);
	EADC_ENABLE_SAMPLE_MODULE_INT(EADC2, 1, BIT0 << V_SOLAR_SENSOR_EADC_SAMPLE_NO);
#if DETECT_SOLAR_I
	EADC_ENABLE_SAMPLE_MODULE_INT(EADC2, 2, BIT0 << I_SOLAR_SENSOR_EADC_SAMPLE_NO);
#endif
#if DETECT_LOAD_I
	EADC_ENABLE_SAMPLE_MODULE_INT(EADC2, 3, BIT0 << I_LOAD_SENSOR_EADC_SAMPLE_NO);
#endif

#ifdef SENSMINIA4_NEO
	NVIC_SetPriority(EADC20_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	NVIC_SetPriority(EADC21_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	#if DETECT_SOLAR_I
	NVIC_SetPriority(EADC22_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	#endif
	#if DETECT_LOAD_I
	NVIC_SetPriority(EADC23_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	#endif
#endif
	NVIC_EnableIRQ(EADC20_IRQn);
	NVIC_EnableIRQ(EADC21_IRQn);
#if DETECT_SOLAR_I
	NVIC_EnableIRQ(EADC22_IRQn);
#endif
#if DETECT_LOAD_I
	NVIC_EnableIRQ(EADC23_IRQn);
#endif
	
	viSenseInst->getOnceFlag = 0;
	voltSenseReStart();
}
//------------------------------------------------------------------------------
int viSenseFilter(VI_SENSE_CONTEXT *ctx)
{	int sts = -1;
	int idx;
	float sum = 0;
		
#if ADC_FILTER_STD
	ALOG_STD *stdFilterAlog;
	if(ctx->valFilter == NULL)
#endif
	{	sts = 0;
		for(idx=0;idx<ctx->count;idx++)
		{	sum += ctx->value[idx];
		}
		ctx->avg = sum / ctx->count;
		return sts;
	}
	
#if ADC_FILTER_STD
	stdFilterAlog = (ALOG_STD *)ctx->valFilter;
	if(stdFilterAlog->array == NULL)
	{	stdFilterAlog->arraySize = ctx->count;
		stdFilterAlog->array = SENS_MEM_ZALLOC(sizeof(float) * stdFilterAlog->arraySize);
	}
	for(idx=0;idx<ctx->count;idx++)
	{	ctx->avg = stdFilter(stdFilterAlog, ctx->value[idx], 1.0f, &sts);
	}
	stdFilterAlog->arrayIndex = 0;
	
	if(ctx->pqFilter == NULL)
	{	return 0;
	}
	stdFilterAlog = (ALOG_STD *)ctx->pqFilter;
	if(stdFilterAlog->array == NULL)
	{	stdFilterAlog->arraySize = ADC_STD_COUNT;
		stdFilterAlog->array = SENS_MEM_ZALLOC(sizeof(float) * stdFilterAlog->arraySize);
	}
	ctx->avg = stdFilter(stdFilterAlog, ctx->avg, 1.0f, &sts);
	//ctx->pqIsReady = (sts == 0)? 1:0;
		
	//if(ctx->pqIsReady)
	if(sts == 0)
	{	stdFilterAlog->arrayIndex = stdFilterAlog->arraySize-1;
	}
	
	return sts;
#endif
}
//------------------------------------------------------------------------------
void voltSenseCheck(void)
{	volatile VI_SENSE_INSTANCE *viSenseInst;
	volatile VI_SENSE_CONTEXT *ctx;
	char floatStr[20];
	//KALMAN_STRUCT *kalman;
	int idx;
	int sts;
	
	viSenseInst = (volatile VI_SENSE_INSTANCE *)&sensDev->voltCurrSenseInst;
	
	if((gIrqDoneFlag & ALL_ADC_DONE) == ALL_ADC_DONE)
	{	getVolt(V_SOLAR_SENSOR_EADC_SAMPLE_NO);
		getVolt(V_BAT_SENSOR_EADC_SAMPLE_NO);
#if DETECT_SOLAR_I
		getVolt(I_SOLAR_SENSOR_EADC_SAMPLE_NO);
#endif
#if DETECT_LOAD_I
		getVolt(I_LOAD_SENSOR_EADC_SAMPLE_NO);
#endif
	}
	
	
	if(viSenseInst->isReady)
	{	//battery volt
		ctx = (volatile VI_SENSE_CONTEXT *)&viSenseInst->batV;
		if(viSenseFilter((VI_SENSE_CONTEXT *)ctx) == 0)
		{	ctx->avg = TO_VOLTAGE_BAT(ctx->avg);
			ctx->count = 0;
			if(ctx->avg <= 0.001f)
				ctx->avg = 0.0f;
			sensPq->onboardPq[ON_BOARD_PQ_BATTERY_VOLT] = ctx->avg;
			putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_BATTERY_VOLT, sensPq->onboardPq[ON_BOARD_PQ_BATTERY_VOLT]);
		}
		
		ctx = (volatile VI_SENSE_CONTEXT *)&viSenseInst->solarV;
		if(viSenseFilter((VI_SENSE_CONTEXT *)ctx) == 0)
		{	ctx->avg = TO_VOLTAGE_SOLAR(ctx->avg);
			ctx->count = 0;
			if(ctx->avg <= 0.001f)
				ctx->avg = 0.0f;
			sensPq->onboardPq[ON_BOARD_PQ_EXT_VOLT] = ctx->avg;
			putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_EXT_VOLT, sensPq->onboardPq[ON_BOARD_PQ_EXT_VOLT]);
		}
		
#if DETECT_SOLAR_I
		ctx = (volatile VI_SENSE_CONTEXT *)&viSenseInst->solarI;
		if(viSenseFilter((VI_SENSE_CONTEXT *)ctx) == 0)
		{	ctx->count = 0;
			if(ctx->avg < 0.0001f)
				ctx->avg = 0.0f;
			ctx->avg = ctx->avg / (0.05f * 25.0f);
			sensPq->onboardPq[ON_BOARD_PQ_SOLAR_CHARGE_CURRENT] = TO_VOLTAGE_SOLAR(ctx->avg);
			putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_SOLAR_CHARGE_CURRENT, sensPq->onboardPq[ON_BOARD_PQ_SOLAR_CHARGE_CURRENT]);
		}
#endif
		
#if DETECT_LOAD_I
		ctx = (volatile VI_SENSE_CONTEXT *)&viSenseInst->loadI;
		if(viSenseFilter((VI_SENSE_CONTEXT *)ctx) == 0)
		{	ctx->count = 0;
			if(ctx->avg < 0.0001f)
				ctx->avg = 0.0f;
			ctx->avg = ctx->avg / (0.05f * 25.0f);
			sensPq->onboardPq[ON_BOARD_PQ_LOAD_CURRENT] = TO_VOLTAGE_SOLAR(ctx->avg);
			putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LOAD_CURRENT, sensPq->onboardPq[ON_BOARD_PQ_LOAD_CURRENT]);
		}
#endif
		viSenseInst->readyFlag = 0;
		viSenseInst->isReady = 0;
		voltSenseReStart();
	}
}
//------------------------------------------------------------------------------