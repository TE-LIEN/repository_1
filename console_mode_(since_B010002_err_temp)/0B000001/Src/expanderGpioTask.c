#include "expanderGpioTask.h"
#include "driver/PCAL6416GpioExpander.h"
#include "sensSystem.h"
#include "sensDev.h"
#include "rtcDateTime.h"
#include "physicalQuantity.h"

#if DI_USE_IRQ

TaskHandle_t expanderTaskHandle = NULL;
//------------------------------------------------------------------------------
void GPF_IRQHandler(void)
{	volatile uint32_t u32temp;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	// NVIC_DisableIRQ(GPF_IRQn);
	// pcal6416GpioIntDisable();
	/* To check if PF.9 interrupt occurred */
	if(GPIO_GET_INT_FLAG(PF, BIT9))
	{	//printf("PF.9 INT occurred.\n");
		GPIO_CLR_INT_FLAG(PF, BIT9);
		//NVIC_DisableIRQ(GPF_IRQn);
		xTaskNotifyFromISR(expanderTaskHandle, 0, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	/* To check if PF.8 interrupt occurred */
	/*else if(GPIO_GET_INT_FLAG(PF, BIT8))
	{	GPIO_CLR_INT_FLAG(PF, BIT8);
		printf("PF.8 INT occurred.\n");
	}*/
	else
	{	/* Un-expected interrupt. Just clear all PF interrupts */
		u32temp = PF->INTSRC;
		PF->INTSRC = u32temp;
		//printf("Un-expected interrupts.\n");
	}
}
#endif
//------------------------------------------------------------------------------
static int expanderInit(void)
{	DI_INSTANCE	*diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT	*diCtx = (DI_CONTEXT *)&diInst->diCtx;
	
	//sysCtrlFlag.sysCtrlHiWord = 0;	//for clear prev setting error, prev set 12 & 13 
	//sysCtrlFlag.di0WakeupEn = 0;	//for clear prev setting, test bit field
	//sysCtrlFlag.di1WakeupEn = 0;	//for clear prev setting, test bit field
	
	//sysCtrlFlag.extGpioIn0WakeupEn = 1;	//for test
	
	sysCtrlFlag.extGpioIn2WakeupEn = 1;	//DI4 force on
	sysCtrlFlag.extGpioIn3WakeupEn = 1;	//DI5 force on
	//dbgMsg("set IRQ :%X, %X, %X\r\n", sysCtrlFlag.sysCtrlFlags, sysCtrlFlag.sysCtrlHiWord, sysCtrlFlag.sysCtrlLoWord);
#if DI_USE_IRQ
	initPCAL6416GpioExpander(~sysCtrlFlag.sysCtrlHiWord);
#else
	initPCAL6416GpioExpander(0xFFFF);	//turn off all IRQ
#endif	
	
	gExpGpioOutVal.diEn = 1;
	pcal6416GpioCtrl(gExpGpioOutVal.val);
	
	if(SENS_SEM_INIT(diCtx->diChgSem, 1) != 0)
	{
	}
	
#if DI_USE_IRQ
	GPIO_EnableInt(PF, 9, GPIO_INT_FALLING);
	#ifdef SENSMINIA4_NEO
	NVIC_SetPriority(GPF_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	#endif
	NVIC_EnableIRQ(GPF_IRQn);
#endif
	return 0;
}
//------------------------------------------------------------------------------
void addDiChgCtx(DI_CONTEXT *diCtx, DI_CHG_CONTEXT *newDiChgCtx)
{	DI_CHG_CONTEXT *diChgCtx;
	
	if(diCtx->diChgCtx == NULL)
	{	diCtx->diChgCtx = newDiChgCtx;
	}
	else
	{	diChgCtx = diCtx->diChgCtx;
		while(diChgCtx->next)
		{	diChgCtx = diChgCtx->next;
		}
		
		diChgCtx->next = newDiChgCtx;
	}
}
//------------------------------------------------------------------------------
#if DI_USE_IRQ
static void expanderProcess(DI_CONTEXT *diCtx)
#else
static void expanderProcess(DI_CONTEXT *diCtx, SYS_STATUS_FLAG1 irqSts)
#endif
{	
#if DI_USE_IRQ
	SYS_STATUS_FLAG1 irqSts;
#endif
	//SYS_STATUS_FLAG2 inputVal;
	int idx;
	DI_CHG_CONTEXT *diChgCtx;
	int stsChg=0;
	int diMapIdx;
	
#if DI_USE_IRQ
	while(1)
#endif
	{	
#if DI_USE_IRQ
		irqSts.sysStatusHiWord = pcal6416GpioGetIntStatus();
#endif
		inputExpVal.sysStatusHiWord = pcal6416GpioGetVal(INPUT_EXPANDER_SLA);
		if(irqSts.extGpioIn0IrqSts)	//DI2
			diCtx->diStsNew[2] = inputExpVal.extGpioIn0Val;
		else
		{	if(inputExpVal.extGpioIn0Val != diCtx->diStsOld[2])
				diCtx->diStsNew[2] = inputExpVal.extGpioIn0Val;
		}
		if(irqSts.extGpioIn1IrqSts)	//DI3
			diCtx->diStsNew[3] = inputExpVal.extGpioIn1Val;
		else
		{	if(inputExpVal.extGpioIn1Val != diCtx->diStsOld[3])
				diCtx->diStsNew[3] = inputExpVal.extGpioIn1Val;
		}
#if BOARD_VERSION == 2
		if(irqSts.extGpioIn4IrqSts)	//DI0
			diCtx->diStsNew[0] = inputExpVal.extGpioIn4Val;
		else
		{	if(inputExpVal.extGpioIn4Val != diCtx->diStsOld[0])
				diCtx->diStsNew[0] = inputExpVal.extGpioIn4Val;
		}
		if(irqSts.extGpioIn5IrqSts)	//DI1
			diCtx->diStsNew[1] = inputExpVal.extGpioIn5Val;
		else
		{	if(inputExpVal.extGpioIn5Val != diCtx->diStsOld[1])
				diCtx->diStsNew[1] = inputExpVal.extGpioIn5Val;
		}
#endif
		if(irqSts.extGpioIn2IrqSts)
			diCtx->diStsNew[4] = inputExpVal.extGpioIn2Val;
		else 
		{	if(inputExpVal.extGpioIn2Val != diCtx->diStsOld[4])
				diCtx->diStsNew[4] = inputExpVal.extGpioIn2Val;
		}
		if(irqSts.extGpioIn3IrqSts)
			diCtx->diStsNew[5] = inputExpVal.extGpioIn3Val;
		else 
		{	if(inputExpVal.extGpioIn3Val != diCtx->diStsOld[4])
				diCtx->diStsNew[5] = inputExpVal.extGpioIn3Val;
		}
		
	
		for(idx=0;idx<DIQUANTITY;idx++)
		{	if(diCtx->diStsNew[idx] != diCtx->diStsOld[idx])
			{	/*  extenal wired short & photocoupler from High to Low, and expander use polarity inversion from low to high
				 *  when new sts is high and old sts is low, record counter.
				 */
				dbgMsg("DI-%d status change, new is %s, old %s\r\n", idx, (diCtx->diStsNew[idx]? "Hi":"Lo"),  (diCtx->diStsOld[idx]? "Hi":"Lo"));
				if(diCtx->diStsNew[idx])	
				{	sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_0 + idx]++;
					dbgMsg("DI-%d new counter: %d\r\n", idx, (int)sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_0 + idx]);
				}
				sensPq->onboardPq[ON_BOARD_PQ_DI_STS_0 + idx] = diCtx->diStsNew[idx];
				
				
				diCtx->diStsChg[idx] = 1;
				diCtx->diStsOld[idx] = diCtx->diStsNew[idx];
				stsChg = 1;
				if(idx == 4) 
				{	if(diCtx->diStsNew[idx])
						sysCtrl->interDi4Active = 1;
					else
						sysCtrl->interDi4ReleasePrepare = 1;
				}
				if(idx == 5)
				{	if(diCtx->diStsNew[idx])
						sysCtrl->interDi5Active = 1;
					else
						sysCtrl->interDi5ReleasePrepare = 1;
				}
			}
		}
	
		if(stsChg && sysCfg->diCfg.diStatusRecord)
		{	diChgCtx = SENS_MEM_ZALLOC(sizeof(DI_CHG_CONTEXT));
			diChgCtx->unixTime = sysTimer->currentTimeStamp;
			for(idx=0;idx<DIQUANTITY;idx++)
			{	//diMapIdx = findDiMap(idx);
				diMapIdx = idx;
				diChgCtx->sts |= (diCtx->diStsOld[idx] << (7-diMapIdx));
				if(diCtx->diStsChg[idx])
				{	diCtx->diStsChg[idx] = 0;
					diChgCtx->chg |= 1<<(7-diMapIdx);
				}
			}
			SENS_SEM_LOCK(diCtx->diChgSem);
			addDiChgCtx(diCtx, diChgCtx);
			SENS_SEM_UNLOCK(diCtx->diChgSem);
		}
		
		//check IRQ release
#if DI_USE_IRQ
		if(PIN_GPIOEX0_nINT == 1)
			break;
		SENS_TIME_DELAY(1);
#endif
	}
	//NVIC_EnableIRQ(GPF_IRQn);
}

#if DI_USE_IRQ
//------------------------------------------------------------------------------	
void expanderTask(void *param)
{	DI_INSTANCE	*diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT	*diCtx = (DI_CONTEXT *)&diInst->diCtx;
	int irqIdx;
	
	
	//expanderTaskHandle = xTaskGetHandle("EXPANDER_TASK");
	expanderTaskHandle = xTaskGetCurrentTaskHandle();
	
	expanderInit();
	
	while(1)
	{	xTaskNotifyWait(0x00, 0xFFFFFFFF, NULL, portMAX_DELAY);
		expanderProcess(diCtx);
	}
}
#else
//------------------------------------------------------------------------------	
void diInit(void)
{	DI_INSTANCE	*diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT	*diCtx = (DI_CONTEXT *)&diInst->diCtx;
	
	sensPq->onboardPq[ON_BOARD_PQ_DI_STS_0] = sysStsFlag2.extGpioIn4Val;
	sensPq->onboardPq[ON_BOARD_PQ_DI_STS_1] = sysStsFlag2.extGpioIn5Val;
	sensPq->onboardPq[ON_BOARD_PQ_DI_STS_2] = sysStsFlag2.extGpioIn0Val;
	sensPq->onboardPq[ON_BOARD_PQ_DI_STS_3] = sysStsFlag2.extGpioIn1Val;
	
	
	initExpanderCtrlChannel();
	/*
	 * The bootloader uses an interrupt-driven approach, while the application uses a polling approach. 
	 * Therefore, when there is no bootloader, upon entering the application, 
	 * it is necessary to first check the IRQ status and clear the IRQ within 2 seconds to prevent system power loss.
	 * If a bootloader is present, the bootloader will handle the IRQ clearing action.
	 */
#if !defined HAVE_BOOTLOADER
	//
	if(!PIN_GPIOEX0_nINT)	//
	{	SYS_STATUS_FLAG1 irqSts;
		irqSts.sysStatusHiWord = pcal6416GpioGetIntStatus();
		expanderProcess(diCtx, irqSts);
		dbgMsg("%s", "boot from expander IRQ Wakeup\r\n");
	}
#else
	diCtx->isWakeupFromDi = sysCtrlFlag.diWakeupFromPsm;
#endif
	
	expanderInit();
}
//------------------------------------------------------------------------------	
void diPolling(void)
{	DI_INSTANCE	*diInst = (DI_INSTANCE *)&sensDev->diInst;
	DI_CONTEXT	*diCtx = (DI_CONTEXT *)&diInst->diCtx;
	SYS_STATUS_FLAG1 irqSts;
	
	irqSts.sysStatusHiWord = 0;
	expanderProcess(diCtx, irqSts);
}
#endif
