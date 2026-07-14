#ifndef __NETWORK_PROCESS_TASK_H__
#define __NETWORK_PROCESS_TASK_H__

#include "sensminiCfg.h"
//#include "network/netApp.h"


#define LAN_NUM_MESSAGES	32	//max queue size
#define NETWORK_PROCESS_Q_SIZE	LAN_NUM_MESSAGES * SENS_TASKQ_GRANM * sizeof(_mqx_max_type)
	

#define TCP_RECV_BUFFER_SIZE		5120

extern QueueHandle_t networkProcessQ;

extern void networkProcessTask(void *param);
extern void networkRecvTask(void *param);
extern int networkDeInit(void);

extern volatile NETWORK_CONTEXT	*networkCtx;

#endif
