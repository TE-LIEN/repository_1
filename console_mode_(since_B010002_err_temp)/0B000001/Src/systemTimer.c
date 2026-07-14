#include "systemTimer.h"
#include "sensCommon.h"

void sysTimerTask(xTimerHandle xTimer);
xTimerHandle xTimers = NULL;

static SysTimer_t gSystemTimer[SENSMINI_TIMER_MAX] = {0};

void initialSysTimer(void)
{	xTimers = xTimerCreate("Timer", SENSMINI_TIMER_BASE, pdTRUE, ( void * ) 0, sysTimerTask);
	if(xTimers != NULL)
	{	if( xTimerStart(xTimers, 0) != pdPASS )
		{	/* The timer could not be set into the Active state. */
		}
	}
}

int setSensminiTimer(void *taskQHandler, unsigned int msgId, void *ctx, unsigned int timerId, unsigned int interval, unsigned char mode)
{	if(timerId >= SENSMINI_TIMER_MAX)
		return -1;
		
	gSystemTimer[timerId].taskQHandler = taskQHandler;
	gSystemTimer[timerId].msgId = msgId;
	gSystemTimer[timerId].ctx = ctx;
	gSystemTimer[timerId].interval = interval / SENSMINI_TIMER_BASE;
	if(interval % SENSMINI_TIMER_BASE)
		gSystemTimer[timerId].interval++;
	gSystemTimer[timerId].currentTick = gSystemTimer[timerId].interval;
	gSystemTimer[timerId].mode = mode;
	gSystemTimer[timerId].enable = 1;
	
	if(xTimers == NULL)
		initialSysTimer();	
	return 0;
}

unsigned int getTimerInterval(unsigned int timerId)
{	return gSystemTimer[timerId].interval * SENSMINI_TIMER_BASE;
}

int getTimerActivity(unsigned int timerId)
{	return gSystemTimer[timerId].enable;
}

void killSensminiTimer(unsigned int timerId)
{	gSystemTimer[timerId].taskQHandler = NULL;
	gSystemTimer[timerId].enable = 0;
}

void sysTimerTask(xTimerHandle xTimer)
{	struct TaskQMsg msg;
	int timerId;
	void *taskHandler;
	
	for(timerId=0;timerId<SENSMINI_TIMER_MAX;timerId++)
	{	if(gSystemTimer[timerId].enable)
		{	if(gSystemTimer[timerId].currentTick)
				gSystemTimer[timerId].currentTick--;
			if(!gSystemTimer[timerId].currentTick)	//timeout issue task q
			{	//dbg_msg("[TIMER] timer %d timeout!!\r\n", timerId);
				msg.msgId = gSystemTimer[timerId].msgId;
				msg.ptr = gSystemTimer[timerId].ctx;
				taskHandler = gSystemTimer[timerId].taskQHandler;
				if(gSystemTimer[timerId].mode == TIMER_MODE_LOOPING)
				{	gSystemTimer[timerId].currentTick = gSystemTimer[timerId].interval;
				}
				else
				{	killSensminiTimer(timerId);
				}
				if(SENS_Q_FULL == SENS_MSG_Q_SEND(taskHandler, (uint32_t *)&msg, 0)) 
				{	//dbg_msg("[TIMER] timer %d fail to send msg q!!\r\n", timerId);
				}
			}
		}
	}
}

