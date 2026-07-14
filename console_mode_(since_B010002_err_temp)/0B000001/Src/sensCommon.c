#include "sensCommon.h"
#include "sensSystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "sdCardTask.h"

#ifndef PLATFORM_FSL
#include "network/httpsrv/httpsrv_base64.h"
#endif

#ifdef NUVOTON 
extern volatile uint32_t rtcIncTick;
#endif
#ifndef PLATFORM_FSL
//------------------------------------------------------------------------------ 
uint8_t *base64Encode(char *str)
{	uint8_t *base64EncodeValue;
	uint32_t base64OutLen;
	
	base64OutLen = 4 * ceil(strlen(str)/3);
	base64EncodeValue = SENS_MEM_ZALLOC(base64OutLen + 1);
	base64_encode(str, (char *)base64EncodeValue);
	
	return base64EncodeValue;
}
//------------------------------------------------------------------------------ 
uint8_t *base64Decode(char *code)
{	char *base64DecodeValue;
	
	uint32_t decodedLength;
	decodedLength = (strlen(code) / 4) * 3 + 1;
	base64DecodeValue = SENS_MEM_ZALLOC(decodedLength);
	base64_decode(base64DecodeValue, code, decodedLength);
	
	return (uint8_t *)base64DecodeValue;
}
#endif
//------------------------------------------------------------------------------ 
char *strrep(char *str1, char symbol, char blank, int length)
{	int i;
  for( i = 0; i < length; i++ )
  {	if( str1[i] == symbol )
    {	str1[i] = blank;
    }
  }
  return str1;
}
//------------------------------------------------------------------------------
uint16_t hwCrc16(uint8_t *buf, uint32_t start, uint32_t cnt)
{	int i;
	uint16_t temp;
	
	SYS_UnlockReg();
	CLK_EnableModuleClock(CRC_MODULE);
	CRC_Open(CRC_16, CRC_WDATA_RVS | CRC_CHECKSUM_RVS, 0xFFFF, CRC_CPU_WDATA_8);
	
	for(i =0;i<cnt;i++)
	{	CRC->DAT = buf[start + i];
	}
	
	temp = (uint16_t)CRC_GetChecksum();
	
	
	CLK_DisableModuleClock(CRC_MODULE);
	return temp;
}
//------------------------------------------------------------------------------
uint16_t swCrc16(uint8_t *buf, uint32_t start, uint32_t cnt)
{	/*int      i, j;
	uint16_t temp, flag;*/
	return hwCrc16(buf, start, cnt);
#if 0
	temp = 0xFFFF;
	//dbgMsg("start is %X, buf at %p, cnt:%d\r\n", start, buf, cnt);
	
	for (i = start; i < start+cnt; i++) 
	{	//dbgMsg("temp %X, buf:%X\r\n", temp, buf[i]);

		temp = temp ^ buf[i];
		
		for (j = 1; j <= 8; j++) 
		{	flag = temp & 0x0001;
			temp = temp >> 1;
			if(flag) 
				temp = temp ^ 0xA001;
		}
	}
	return (temp);
#endif
}
//------------------------------------------------------------------------------
void initialTaskStruct(TASK_INFO *taskInf)
{	if(SENS_SEM_INIT(taskInf->lock, 1) != MQX_OK)
	{	taskInf->id = SENS_NULL_TASK_ID;
	}
	taskInf->id = SENS_NULL_TASK_ID;
}
//------------------------------------------------------------------------------
void debugLock(void)
{	SENS_SEM_LOCK(sensSys->sysSem.dbgUart);
}
//------------------------------------------------------------------------------
void debugUnlock(void)
{	SENS_SEM_UNLOCK(sensSys->sysSem.dbgUart);
}
//------------------------------------------------------------------------------
void strtokLock(void)
{	SENS_SEM_LOCK(sensSys->sysSem.strtok);
}
//------------------------------------------------------------------------------
void strtokUnLock(void)
{	SENS_SEM_UNLOCK(sensSys->sysSem.strtok);
}
//------------------------------------------------------------------------------
void sdLock(void)
{	SENS_SEM_LOCK(sensSys->sysSem.sdLock);
}
//------------------------------------------------------------------------------
void sdUnlock(void)
{	SENS_SEM_UNLOCK(sensSys->sysSem.sdLock);
}
//------------------------------------------------------------------------------
void webDbgMsgLock(void)
{	SENS_SEM_LOCK(sensSys->sysSem.webDbgMsgLock);
}
//------------------------------------------------------------------------------
void webDbgMsgUnlock(void)
{	SENS_SEM_UNLOCK(sensSys->sysSem.webDbgMsgLock);
}
//------------------------------------------------------------------------------
void sdWriteLock(void)
{	SENS_SEM_LOCK(sensSys->sysSem.sdWriteLock);
}
//------------------------------------------------------------------------------
void sdWriteUnlock(void)
{	SENS_SEM_UNLOCK(sensSys->sysSem.sdWriteLock);
}
//------------------------------------------------------------------------------
void fsLock(void)
{	
#ifdef FILESYSY_USE_LOCK
	SENS_SEM_LOCK(sensSys->sysSem.fsLock);
#endif
}
//------------------------------------------------------------------------------
void fsUnlock(void)
{	
#ifdef FILESYSY_USE_LOCK
	SENS_SEM_UNLOCK(sensSys->sysSem.fsLock);
#endif
}
#ifdef SPI_FILE_SYSTEM
//------------------------------------------------------------------------------
void spiFsLock(void)
{	SENS_SEM_LOCK(sensSys->sysSem.spiFsLock);
}
//------------------------------------------------------------------------------
void spiFsUnLock(void)
{	SENS_SEM_UNLOCK(sensSys->sysSem.spiFsLock);
}
#endif
//------------------------------------------------------------------------------ 
char *sensDtoa(char *dest, double val, int precision)
{	long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};

	char strTmp[30];
  char *ret = dest;
  char *temp = &strTmp[0];
  int heiltal = (int)val;
  int idx;
  int desimal;
  
  memset(strTmp, 0, 30);
  if(isnan(val) || isinf(val))
  {	SENS_SPRINTF(strTmp, "%s", "NaN");
  	memcpy(dest, strTmp, strlen(strTmp));
  	dest[strlen(strTmp)] = '\0';
  	return ret;
  }
  
  if( val < 0 && val > -1)
  {	SENS_SPRINTF(temp, "%s", "-0");
  }
  else
  {	SENS_SPRINTF(temp, "%d", heiltal);
  }
  
  desimal = abs((int)((val - heiltal) * p[precision]));
  if((precision > 0) && desimal)
  {	while (*temp != '\0') temp++;
  	*temp++ = '.';
    
    switch(precision)
    {	case 1:	SENS_SPRINTF(temp, "%1d", desimal);		break;
      case 2:	SENS_SPRINTF(temp, "%02d", desimal);	break;
      case 3:	SENS_SPRINTF(temp, "%03d", desimal);	break;
      case 4:	SENS_SPRINTF(temp, "%04d", desimal);	break;
      case 5:	SENS_SPRINTF(temp, "%05d", desimal);	break;
      case 6:	SENS_SPRINTF(temp, "%06d", desimal);	break;
      case 7:	SENS_SPRINTF(temp, "%07d", desimal);	break;  
      case 8:	SENS_SPRINTF(temp, "%08d", desimal);	break;  
      default:															break;
    }
    temp[precision] = '\0';
    for(idx=precision-1;idx>=0;idx--)
    {	if(temp[idx] == '0')
    		temp[idx] = '\0';
    	else
    		break;
    }
    if(idx==-1 && temp[0] == '\0')
    {	temp--;
    	*temp = '\0';
    }
  }
  memcpy(dest, strTmp, strlen(strTmp));
	dest[strlen(strTmp)] = '\0';
  return ret;
}
//------------------------------------------------------------------------------ 
char * my_ftoa(char *a, float f, int precision)
{	long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};

	char strTmp[30];
	char *ret = a;
	char *temp = &strTmp[0];
	int heiltal = (int)f;
  
	memset(strTmp, 0, 30);
	if(isnan(f) || isinf(f))
	{	SENS_SPRINTF(strTmp, "%s", "NaN");
		memcpy(a, strTmp, strlen(strTmp));
		a[strlen(strTmp)] = '\0';
		return ret;
	}
  
	if( f < 0 && f > -1)
	{	SENS_SPRINTF(temp, "%s", "-0");
	}
	else
	{	SENS_SPRINTF(temp, "%d", heiltal);
	}
  
	if(precision > 0)
	{	while (*temp != '\0') temp++;
		*temp++ = '.';
		int desimal = abs((int)((f - heiltal) * p[precision]));
		switch(precision)
		{	case 1:	SENS_SPRINTF(temp, "%1d", desimal);		break;
			case 2:	SENS_SPRINTF(temp, "%02d", desimal);	break;
			case 3:	SENS_SPRINTF(temp, "%03d", desimal);	break;
			case 4:	SENS_SPRINTF(temp, "%04d", desimal);	break;
			case 5:	SENS_SPRINTF(temp, "%05d", desimal);	break;
			case 6:	SENS_SPRINTF(temp, "%06d", desimal);	break;
			case 7:	SENS_SPRINTF(temp, "%07d", desimal);	break;  
			case 8:	SENS_SPRINTF(temp, "%08d", desimal);	break;  
			default:										break;
		}
		temp[precision] = '\0';
	}
	memcpy(a, strTmp, strlen(strTmp));
	a[strlen(strTmp)] = '\0';
	return ret;
}
//------------------------------------------------------------------------------
void printTime(void)
{	dbgMsg1("[%04d/%02d/%02d %02d:%02d:%02d <%d>] ", sensSys->dateTime.u32Year, sensSys->dateTime.u32Month, sensSys->dateTime.u32Day, sensSys->dateTime.u32Hour, sensSys->dateTime.u32Minute, sensSys->dateTime.u32Second, rtcIncTick);
}
//------------------------------------------------------------------------------
void displayBufInHex(unsigned char *buf, int len, const char *callFunc, int callLine)
{	int idx;
	char temp[32];
	char hex[64];
	int remainByte;
	int x;

	debugLock();
	
	dbgMsg1("buf(%d) call from :%s(%d) at %p\r\n", len, callFunc, callLine, buf);
	for(idx = 0; idx < len ; idx+=16)
	{	memset(hex, 0, 64);
		if((idx + 16) > len)	remainByte = len - idx;
		else									remainByte = 16;
		SENS_SPRINTF(hex, "%04X ", idx);
		memset(temp, 0, 32);
		for( x=0;x<16;x++)
		{	if(x < remainByte)
			{	SENS_SPRINTF(hex+strlen(hex), "%02X ", buf[idx+x]);
				if((buf[idx+x] >= 0x20) && (buf[idx+x] < 0x7F))
				{	temp[x] = buf[idx+x];
				}
				else
				{	temp[x] = '.';
				}
			}
			else
				strcat(hex, "   ");
		}
		dbgMsg1("%s |%s\r\n", hex, temp);
	}
	debugUnlock();
}
//------------------------------------------------------------------------------
void dbgPrint1(char *format, ...)
{	va_list args;
	int argStrLen;
	char *bigBuf = NULL;
	char *logMsgPtr;
	
	va_start(args, format);
	if(strlen(format))
	{	memset((void *)sensSys->dbg.msg, 0, DEBUG_MSG_BUF);
		//vsprintf((char *)sensSys->dbg.msg, format, args);
		argStrLen = vsnprintf((char *)sensSys->dbg.msg, DEBUG_MSG_BUF, format, args);
		logMsgPtr = (char *)sensSys->dbg.msg;
		if((argStrLen > DEBUG_MSG_BUF) && (argStrLen < 2048))	//MAX re alloc 2K size
		{	bigBuf = SENS_MEM_ZALLOC(argStrLen+1);
			vsprintf((char *)bigBuf, format, args);
			logMsgPtr = bigBuf;
		}
		UART_Write(CONSOLE_UART_CTX, (uint8_t *)logMsgPtr, strlen((const char *)logMsgPtr));
		while(!UART_IS_TX_EMPTY(CONSOLE_UART_CTX)) __NOP();
		if(bigBuf)
		{	SENS_MEM_FREE(bigBuf);
		}
	}
	va_end(args);
}
//------------------------------------------------------------------------------
void dbgPrint(char *format, ...)
{	va_list args;
	int argStrLen;
	char *bigBuf = NULL;
	char *logMsgPtr;
	DEBUG_STRUCT *dbg = (DEBUG_STRUCT *)&sensSys->dbg;
#ifndef FILESYSTEM_USE_TASK
	int writeToSdLength;
	int writeToUartLength;
#endif
	char *webDbgPtr;

	if(sensSys && !dbg->dbgUartDisable)
	{	va_start(args, format);
		if(strlen(format))
		//argStrLen = strlen((char *)&args) + strlen(format);
		//if(argStrLen < DEBUG_MSG_BUF)
		{	memset((void *)dbg->msg, 0, DEBUG_MSG_BUF);
			//vsprintf((char *)sensSys->dbg.msg, format, args);
			argStrLen = vsnprintf((char *)dbg->msg, DEBUG_MSG_BUF, format, args);
			logMsgPtr = (char *)sensSys->dbg.msg;
			if((argStrLen > DEBUG_MSG_BUF) && (argStrLen < 2048))	//MAX re alloc 2K size
			{	bigBuf = SENS_MEM_ZALLOC(argStrLen+1);
				vsprintf((char *)bigBuf, format, args);
				logMsgPtr = bigBuf;
			}
			if(sysCfg && sysCfg->sensSysCfg.logToSd)
			{	
#if defined FILESYSTEM_USE_TASK
				writeLog(logMsgPtr);
#else
				writeToSdLength = strlen((char *)sensSys->dbg.sdLogMsg);
				writeToUartLength = strlen((char *)sensSys->dbg.msg);
				if((writeToSdLength < (DEBUG_MSG_BUF *3)) && ((writeToSdLength + writeToUartLength) < (DEBUG_MSG_BUF *3)))
				{	memcpy(sensSys->dbg.sdLogMsg + writeToSdLength, sensSys->dbg.msg, writeToUartLength);
					if(sysParams && sysParams->logToSd && sensSys->logIsReady)
					{	writeLog((char *)sensSys->dbg.sdLogMsg);
						//sensSys->dbg.sdLogMsg[0] = '\0';
						memset(sensSys->dbg.sdLogMsg, 0, strlen((char *)sensSys->dbg.sdLogMsg));
					}
				}
				else
					memset(sensSys->dbg.sdLogMsg, 0, strlen((char *)sensSys->dbg.sdLogMsg));
#endif
			}
			UART_Write(CONSOLE_UART_CTX, (uint8_t *)logMsgPtr, strlen((const char *)logMsgPtr));
			while(!UART_IS_TX_EMPTY(CONSOLE_UART_CTX)) __NOP();
			
			webDbgPtr = strstr(logMsgPtr, "\r\n");
			while(webDbgPtr)
			{	webDbgPtr[0] = ' ';
				webDbgPtr[1] = '&';
				webDbgPtr = strstr(webDbgPtr, "\r\n");
			}
			webDbgMsgLock();
			dbg->webDbgMsgLength += strlen(logMsgPtr);
			if(dbg->webDbgMsgLength < 1020)
			{	if(dbg->webDbgMsg)
				{	strcat((char *)dbg->webDbgMsg, logMsgPtr);
				}
				else
					dbg->webDbgMsgLength = 0;
			}
			webDbgMsgUnlock();
			
			if(bigBuf)
			{	SENS_MEM_FREE(bigBuf);
			}
		}
		va_end(args);
	}
}
//------------------------------------------------------------------------------
uint32_t sendMsgWithErrHandle(enum task_define taskDef, uint32_t msgSts, char const *func, int line)
{	char msg[100];
	
#if defined OS_MQX
	if(!msgSts)
		return msgSts;
#elif defined OS_FREERTOS
	if(msgSts == pdPASS)
		return 0;
#endif

	memset(msg, 0, 50);
	if(taskDef == SERV_CMD_PROCESS_TASK)
	{	SENS_SPRINTF(msg, "send SERV_CMD_PROCESS_TASK Q MSG Fail: ");
	}
	/*else if(taskDef == MOBILE_SEND_TASK)
	{	SENS_SPRINTF(msg, "send MOBILE Q MSG Fail: ");
	}*/
	else if(taskDef == NETWORK_PROCESS_TASK)
	{	SENS_SPRINTF(msg, "send NETWORK Q MSG Fail: ");
	}
#if EN_PERIPHERAL_TASK
	else if(taskDef == PERIPHERAL_HANDLE_TASK)
	{	SENS_SPRINTF(msg, "send PERIPHERAL Q MSG Fail: ");
	}
#endif
	else if(taskDef == SDCARD_TASK)
	{	SENS_SPRINTF(msg, "send SD Q MSG Fail: ");
	}
#ifdef SUPPORT_MQTT
	else if(taskDef == MQTT_TASK)
	{	SENS_SPRINTF(msg, "send MQTT Q MSG Fail: ");
	}
#endif
	
	if(msgSts == SENS_Q_FULL)
	{	SENS_SPRINTF(msg+strlen(msg), "FULL");
//#if N_TEMP_REMOVE
		sysCtrl->isReadyToReboot = 1;
//#endif
	}
	else 
		SENS_SPRINTF(msg+strlen(msg), "UNKNOWN");
	SENS_SPRINTF(msg+strlen(msg), ", call from %s(%d)", func, line);
	dbg_msg("%s\r\n", msg);
	return msgSts;
}

#ifdef OS_FREERTOS
//------------------------------------------------------------------------------
void delayms( const TickType_t ms )
{	vTaskDelay(ms/portTICK_RATE_MS);
}
//------------------------------------------------------------------------------
long systemTickGet(void)
{	uint32_t ret = xTaskGetTickCount();
	ret = ret;
	return (long)ret;
}

#endif

#if SUPPORT_NUVOTON_USB_HOST
//------------------------------------------------------------------------------
uint32_t get_ticks(void)
{	return (systemTickGet()/10);
}
//------------------------------------------------------------------------------
void delay_us(int usec)
{	CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_TMR0SEL_Msk)) | CLK_CLKSEL1_TMR0SEL_HXT;
	CLK->APBCLK0 |= CLK_APBCLK0_TMR0CKEN_Msk;
	TIMER0->CTL = 0;        /* disable timer */
	TIMER0->INTSTS = (TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk);   /* write 1 to clear for safety */
	TIMER0->CMP = (uint32_t)usec;
	TIMER0->CTL = (11 << TIMER_CTL_PSC_Pos) | TIMER_ONESHOT_MODE | TIMER_CTL_CNTEN_Msk;

	while(!TIMER0->INTSTS);
}
#endif
//------------------------------------------------------------------------------
int SENS_IS_NAN(float val)
{	if(val == val)
		return 0;
	else
		return 1;
}
//------------------------------------------------------------------------------
float SENS_SET_VAL_NAN(void)
{	return -0.0f;
}
//------------------------------------------------------------------------------