#ifndef __SYSTEM_TIMER_H__
#define __SYSTEM_TIMER_H__

#include "sensminiCfg.h"

#define SENSMINI_TIMER_BASE						5	//10 ms

#define SENSMINI_TIMER_CSQ						0
#define SENSMINI_TIMER_CLK						1
#define SENSMINI_TIMER_UREG						2
//#define SENSMINI_TIMER_TPM					3
//#define SENSMINI_TIMER_COM_PORT				4
//#define SENSMINI_TIMER_ID_CAMERA				3
#define SENSMINI_TIMER_LAN_HTTP_GET				4
#define SENSMINI_TIMER_LAN_HTTP_POST			5
//#define SENSMINI_TIMER_HTTPS_GET				5
//#define SENSMINI_TIMER_HTTPS_POST				6
//#define SENSMINI_TIMER_PERIPHERAL_POLLING	7
//#define SENSMINI_TIMER_SD_CARD_DETECT			8
#define SENSMINI_TIMER_AI_SENSOR				6
#define SENSMINI_TIMER_GPS_GET_POSITION			9
#define SENSMINI_TIMER_GPS_RESTART				10
#define SENSMINI_TIMER_I2C_THERMOSENSOR			12

#define SENSMINI_TIMER_MODBUS_POLLING			13
//#define SENSMINI_TIMER_WISUN_POLLING_DEV	14
#define SENSMINI_TIMER_MAX						16
#define SENSMINI_TIMER_USB_CDC_ECM_GET_STATUS	7
#define SENSMINI_TIMER_USB_UVC_INIT_TIMEOUT		8

#define TIMER_MODE_TRIGGER	0
#define TIMER_MODE_LOOPING	1

typedef struct SysTimer
{	//struct TaskQMsg taskQMsg;
	unsigned char 		mode;					//1:looping, 0:trigger once
	unsigned char		enable;
	//_mqx_max_type_ptr taskQHandler;
	void 				*taskQHandler;
	void				*ctx;
	unsigned int		msgId;
	unsigned int		interval;
	unsigned int		currentTick;
}SysTimer_t;


//extern int setSensminiTimer(_mqx_max_type_ptr taskQHandler, unsigned int msgId, unsigned int timerId, unsigned int interval, unsigned char mode);
extern int setSensminiTimer(void *taskQHandler, unsigned int msgId, void *ctx, unsigned int timerId, unsigned int interval, unsigned char mode);
extern void killSensminiTimer(unsigned int timerId);
extern unsigned int getTimerInterval(unsigned int timerId);
extern int getTimerActivity(unsigned int timerId);

#endif
