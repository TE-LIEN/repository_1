#ifndef __SENSMINI_CFG_H__
#define __SENSMINI_CFG_H__

#ifndef SENS_PACK
    #if defined(__GNUC__)
        #define SENS_PACK_GCC __attribute__((__packed__))
        #define SENS_PACK_IAR 
        
    #elif defined (__IAR_SYSTEMS_ICC__)
        #define SENS_PACK_GCC 
        #define SENS_PACK_IAR	__packed
    #endif
#endif

#include "compiler_settings.h"

#if defined SENSMINIA4_PRO || defined SENSMINIS2 || defined SENSMINIA4_NEO
#define OS_FREERTOS
#define NET_LWIP
#define FS_FFS
#endif

#if defined SENSMINIA4_PLUS || defined SENSMINIA4
#define OS_MQX
#define NET_RTCS
#define MQX_FS

#define SUPPORT_WIRED_LAN
#endif


//include OS
#ifdef OS_FREERTOS
	#include "FreeRTOS.h"
	#include "semphr.h"
	#include "event_groups.h"
	#include "queue.h"
	#include "task.h"
	#include "timers.h"
	
	#define	_io_strtod	strtod
	#define	_io_atoi	atoi
	#define MQX_OK	0
	#define FREERTOS_NULL_TASK_ID					0x5A5A

	typedef void*	taskArg;

#endif

#ifdef OS_MQX
typedef uint32_t	taskArg;
#endif

#if defined SENSMINIA4_PRO || defined SENSMINIA4_NEO
#include "m460.h"
#include "drv_emac/m460_emac.h"
#include "drv_emac/m460_mii.h"

#define SUPPORT_WIRED_LAN
#define SUPPORT_QSPI

#endif

#ifdef SENSMINIS2
#include "M2354.h"
#endif

#ifdef SENSMINIA4_NEO
#include "gpioEnum.h"
#endif

#ifdef NUVOTON
#include "boardDef.h"
#endif

#include "taskDef.h"
#include "osa.h"


#ifdef FS_FFS
#include "ff.h"
#include "FS/FSA_FS.h"

#define IO_SEEK_SET 							(1) /* Seek from start */
#define IO_SEEK_CUR 							(2) /* Seek from current location */
#define IO_SEEK_END 							(3) /* Seek from end */
#endif

#ifdef NET_LWIP
	#include "lwip/sockets.h"

	#define IPBYTES(a)            (((a)>>24)&0xFF),(((a)>>16)&0xFF),(((a)>> 8)&0xFF),((a)&0xFF)
	#define IPADDR(a,b,c,d)       ((((uint32_t)(a)&0xFF)<<24)|(((uint32_t)(b)&0xFF)<<16)|(((uint32_t)(c)&0xFF)<<8)|((uint32_t)(d)&0xFF))
	
	#define RTCS_OK		0
	#define RTCS_SOCKET_ERROR	-1
	
	#define NETWORK_USE_SELECT
	#define HTTP_USE_FSL_LWIP
	
	#define LWIP_USE_NONBLOCKING	1
#endif

#define	_io_strtod	strtod
#define	_io_atoi	atoi

#ifdef SENSMINIA4_NEO
#define _mqx_max_type	uint32_t
#define MQX_OK			0

#include "HAL/HAL_CONFIG.h"
#include "HAL/HAL_UART.h"
#endif

#include "headers/sysStruct.h"
#include "headers/deviceStruct.h"
#include "headers/networkStruct.h"

#include "sensCommon.h"

#endif

