#ifndef __SERVER_CMD_PROCESS_TASK_H__
#define __SERVER_CMD_PROCESS_TASK_H__

#include "sensminiCfg.h"

#define SERV_CMD_PROCESS_NUM_MESSAGES	32	                                        //max queue size

#define servCmdProcessQSize SERV_CMD_PROCESS_NUM_MESSAGES * SENS_TASKQ_GRANM * sizeof(uint32_t)


extern QueueHandle_t	servCmdProcessQ;
extern void serverCmdProcessTask(void *param);

#endif


