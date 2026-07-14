#include "network/protocol/serverProtocol.h"
#include "sensCommon.h"
#include "sensSystem.h"


QueueHandle_t	servCmdProcessQ;
//------------------------------------------------------------------------------
int servCmdProcessTaskInit(void)
{	if(MQX_OK != SENS_MSG_Q_INIT(servCmdProcessQ, SERV_CMD_PROCESS_NUM_MESSAGES, SENS_TASKQ_GRANM))
	{		return -1;
  }
  
#ifdef SUPPORT_MQTT
	//initialIotpsTable();	//for senstalk MQTT protocol
#endif
  
	/*if(SENS_EVENT_CREATE(otaFsReadDoneEvent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{	return -1;
	}*/
	
	return 0;
}

//------------------------------------------------------------------------------
void serverCmdProcessTask(void *param)
{	struct TaskQMsg msg;
	SERVER_CMD_STRUCT *cmdStruct;
	
	servCmdProcessTaskInit();
	
	SENS_SEM_LOCK(taskAct.servCmdProcessTaskAct.lock);
	
	while(1)
	{	taskAct.servCmdProcessTaskAct.sts = TASK_STS_BLOCKED;
		SENS_MSG_Q_RECV(servCmdProcessQ, (uint32_t *)&msg, LWMSGQ_RECEIVE_BLOCK_ON_EMPTY, 0, 0);
		taskAct.servCmdProcessTaskAct.sts = TASK_STS_RUNNING;
		taskAct.servCmdProcessTaskAct.runningStartTime = GetTimeTicks();
		
		switch(msg.msgId)
		{	case SERV_CMD_PROCESS_Q_MSG_SEND_CMD:                                          // Send command(JSON) to broker 
						cmdStruct = (SERVER_CMD_STRUCT *)msg.ptr;
						prepareProtocolPayloadToServer(cmdStruct);
						SENS_MEM_FREE(cmdStruct);
						break;
			case SERV_CMD_PROCESS_Q_MSG_RECV_CMD:
						parserServerPayload((SERV_RECV_CMD_CTX *)msg.ptr);
						break;
			default:
						break;
		}
	}
}
//------------------------------------------------------------------------------