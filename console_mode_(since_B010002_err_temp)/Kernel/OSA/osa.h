#ifndef __OSA_H__
#define __OSA_H__

#include "sensminiCfg.h"

typedef struct taskTemplate
{	int				TASK_TEMPLATE_INDEX;
	void 			*TASK_ADDRESS;
	int				TASK_STACKSIZE;
	int				TASK_PRIORITY;
	char			*TASK_NAME;
	int				TASK_ATTRIBUTES;
	int				CREATION_PARAMETER;
	int				DEFAULT_TIME_SLICE;
} TASK_TEMPLATE;

//memory
#define SENS_MEM_FREE										OSA_MEM_FREE
#define SENS_MEM_ALLOC									pvPortMalloc
#define SENS_MEM_ZALLOC									MEMORY_ALLOC_ZERO
#define SENS_MEM_REALLOC								pvPortRealloc

//msg queue
#define SENS_MSG_Q_DEL									vQueueDelete
#define SENS_MSG_Q_SEND									xQueueSend
#define SENS_MSG_Q_SEND_TO_FRONT				xQueueSendToFront
#define SENS_MSG_Q_RECV									OSA_MSG_Q_RECV
#define LWMSGQ_RECEIVE_BLOCK_ON_EMPTY		0
#define SENS_MSG_Q_INIT(a, b, c)				OSA_MSG_Q_INIT(&a, b, c)
#define SENS_Q_FULL											errQUEUE_FULL
//#define SENS_TASKQ_GRANM			sizeof(struct TaskQMsg)

//event
#define SENS_EVENT_STRUCT								EventGroupHandle_t
#define SENS_EVENT_STRUCT_PTR						EventGroupHandle_t
#define SENS_EVENT_CREATE(a, b)					OSA_EVENT_CREATE(&a, b)
#define SENS_EVENT_SET									OSA_EVENT_SET
#define SENS_EVENT_WAIT									OSA_EVENT_WAIT
#define SENS_EVENT_DESTROY							vEventGroupDelete
#define LWEVENT_AUTO_CLEAR							0

//semaphore
#define SENS_SEM_STRUCT									xSemaphoreHandle
#define SENS_SEM_INIT(a, b)							freertos_semaInit((SENS_SEM_STRUCT)&a, b)

#define SENS_SEM_LOCK										freertos_semaLock
#define SENS_SEM_UNLOCK									freertos_semaUnlock

//TASK
#define SENS_NULL_TASK_ID								FREERTOS_NULL_TASK_ID
#define SENS_TASK_BLOCK()								vTaskSuspend(NULL)
#define SENS_TASK_TEMPLATE							TASK_TEMPLATE
#define MQX_AUTO_START_TASK							1

#define SENS_SSCANF						__io_sscanf
#define SENS_SPRINTF					_io_sprintf


#define SENS_TIME_DELAY									delayms
#define GetTimeTicks										systemTickGet

#define SENS_TASKQ_GRANM			sizeof(struct TaskQMsg)

extern void *OSA_MEMORY_ALLOC(size_t xWantedSize);
extern void *OSA_MEMORY_ALLOC_ZERO(size_t xWantedSize);

#define SYS_RET_OK	0

#define _mqx_uint								uint32_t
#define _mqx_int								int32_t


extern void *OSA_MEM_CALLOC(size_t nmemb, size_t xWantedSize);
extern void *MEMORY_ALLOC_ZERO(size_t xWantedSize);
extern void OSA_MEM_FREE(void *ptr);
extern uint32_t OSA_MSG_Q_INIT(QueueHandle_t *msgQ, uint32_t queueCount, uint32_t queueSize);
extern uint32_t OSA_MSG_Q_RECV(void *msgQ, uint32_t *msg, uint32_t flag, uint32_t timeout, void *flag2);
extern uint32_t OSA_TASK_CREATE(uint32_t taskId);

extern uint32_t OSA_EVENT_CREATE(SENS_EVENT_STRUCT *event, uint32_t flag);
extern uint32_t OSA_EVENT_SET(SENS_EVENT_STRUCT_PTR event, uint32_t setBit);
extern uint32_t OSA_EVENT_WAIT(SENS_EVENT_STRUCT_PTR event, uint32_t waitBits, uint32_t waitAll, uint32_t waitTimeout);

extern int __io_sscanf(char  *str_ptr, const char  *format_ptr, ...);
extern int32_t _io_sprintf(char *str_ptr, const char *fmt_ptr, ...);


extern int freertos_semaInit(SENS_SEM_STRUCT *sema, int count);
extern BaseType_t freertos_semaLock(SemaphoreHandle_t sema);
extern BaseType_t freertos_semaUnlock(SemaphoreHandle_t sema);

#endif
