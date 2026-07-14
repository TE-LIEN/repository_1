#ifndef __SEBS_COMMON_H__
#define __SEBS_COMMON_H__

#include "sensminiCfg.h"

extern void printTime(void);
extern void dbgPrint(char *format, ...);
extern void dbgPrint1(char *format, ...);


extern void debugLock(void);
extern void debugUnlock(void);
extern void strtokLock(void);
extern void strtokUnLock(void);
extern void sdLock(void);
extern void sdUnlock(void);
extern void fsLock(void);
extern void fsUnlock(void);
extern void webDbgMsgLock(void);
extern void webDbgMsgUnlock(void);
extern void sdWriteLock(void);
extern void sdWriteUnlock(void);

extern char *strrep(char *str1, char symbol, char blank, int length);
extern uint16_t swCrc16(uint8_t *buf, uint32_t start, uint32_t cnt);
extern uint16_t hwCrc16(uint8_t *buf, uint32_t start, uint32_t cnt);
extern int SENS_IS_NAN(float val);
extern float SENS_SET_VAL_NAN(void);

#ifdef SPI_FILE_SYSTEM
extern void spiFsLock(void);
extern void spiFsUnLock(void);
#endif

#ifdef OS_FREERTOS
extern void delayms( const TickType_t ms );
extern long systemTickGet(void);
#endif

#if SUPPORT_NUVOTON_USB_HOST
extern void delay_us(int usec);

extern uint32_t get_ticks(void);
#endif
extern char *sensDtoa(char *dest, double val, int precision);
extern char * my_ftoa(char *a, float f, int precision);
extern uint32_t sendMsgWithErrHandle(enum task_define taskDef, uint32_t msgSts, char const *func, int line);
extern void initialTaskStruct(TASK_INFO *taskInf);
extern void displayBufInHex(unsigned char *buf, int len, const char *callFunc, int callLine);

#ifndef PLATFORM_FSL
extern uint8_t *base64Encode(char *str);
extern uint8_t *base64Decode(char *code);
#endif


struct TaskQMsg
{	int	msgId;
  char *ptr;
};


#define dbgMsg(fmt, ...)		do                    					\
								{	debugLock();									\
									printTime();									\
									dbgPrint(fmt, __VA_ARGS__); 	\
									debugUnlock();								\
								}while(0)
														
#define dbgMsg1(fmt, ...)		do															\
								{	dbgPrint(fmt, __VA_ARGS__); 	\
								}while(0)
														
#define dbgMsg2(fmt, ...)		do															\
								{	dbgPrint1(fmt, __VA_ARGS__); 	\
								}while(0)

#define dbg_msg					dbgMsg
#define dbg_msg1 				dbgMsg1
#define dbg_msg2				dbgMsg2
								
#define dbgMsgFs							printf

#endif