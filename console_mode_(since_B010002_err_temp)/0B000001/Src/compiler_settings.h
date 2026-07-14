#ifndef __COMPILER_SETTINGS_H__
#define __COMPILER_SETTINGS_H__

#define TEST_LOOP 1//[DL]
#define TEST_CODE_SD 1
#define TEST_CODE_USB_UVC 1
#define TEST_ADC 1

//#define TEST_CODE_SD
//#define TEST_CODE_USB_UVC
//#define TEST_ADC
//	#define TEST_MODE		0	//DIFFERENTIAL_MODE	0
//	#define TEST_ADC_TYPE	1	//0:current, 1:volt
#define TIME_SHIFT_OPERATE	0

#define ADC_USE_OLD_CIRCUIT		0
#define DI_IRQ_NEW_CIRCUIT		1
#define EN_LOAD_BOOT_ZONE2		0//[DL]20260611
//#define EN_LOAD_BOOT_ZONE2		1

#define TEST_ETH_MASS_XMIT		1//[DL]20260611
//#define TEST_ETH_MASS_XMIT		0
#if HBI_ENABLE == 1 
#define USE_NUVOTON_EVB	0
#else
#define USE_NUVOTON_EVB	1
#endif
#define GPIO_DEBUG		0

#define TEST_SPI	0

#if USE_NUVOTON_EVB
	#define EN_WATCHDOG	0
#else
#define EN_WATCHDOG	0//1
#endif

#define TEST_IOT_DEVICE	1

#define TEST_N_REMOVE	0
#define N_TEMP_REMOVE	0

#if DI_IRQ_NEW_CIRCUIT == 1
	#define DI_USE_IRQ		0
#else
	#define DI_USE_IRQ		0
#endif

#define SPI_FILE_SYSTEM				1
#define HTTPS_IMG_SERVER			1
#define HTTPS_OTA_SERVER			1
#define YS_MQTT_BROKER				1
	#define USE_MQTTS				0
#define UPLOAD_SERVER_CNT			2
#define MULTI_SOCK_QUEUE			2
#define MAX_SERVER_COUNT			UPLOAD_SERVER_CNT + YS_MQTT_BROKER
#define MAX_CLOUD_SERVER			MAX_SERVER_COUNT + HTTPS_IMG_SERVER + HTTPS_OTA_SERVER

#define SUPPORT_IOA_WEB_API		1
#define IOA_USE_LOCAL_TIME	1
#define IOA_FIXED_LOCAL_TIME_ZONE_480	1
#define FOR_MCU_OR_CARRIER_BOARD 0//1

#ifdef SUPPORT_USB_HOST
#define SUPPORT_NUVOTON_USB_HOST	1
//#define SUPPORT_USB_CDC_ECM			1	//for CDC-ECM ETHERNET don't use
#endif

#if SUPPORT_NUVOTON_USB_HOST || SUPPORT_FSL_USB_HOST
#define SUPPORT_USB					1
#else
#ifdef SUPPORT_USB
#define SUPPORT_USB			0
#endif
#endif


#define SUPPORT_PPP					1
	//#define PPP_USE_CMUX			1	//A4 NEO only use USB HOST
	//#define UART_RECV_INTER_LOCK	1	//for CMUX

#define SUPPORT_MQTT
#define SUPPORT_HTTP_CLIENT

/*
 * Little FS define
 */
//#define SPI_FILE_SYSTEM	1
//#define SPI_DUAL_PARTITION	1	//first partition reserved for firmware code

#define FILESYSTEM_USE_TASK

#define IS_BETA_VERSION	0
#define AUTO_DATA_SYNC	1
#define REC_INTERVAL_FIXED	1
#define REC_FILE_2_SERV_DATA_SYNC	1

//#define TEST_WAKEUP_TIMER	1

/*
* version define
*/
#ifdef SENSMINIA4_PRO
	#define DEV_MODEL_A4PRO	1
#if defined RELEASE_VERSION
	#define VER_BOARD	0x0AUL
#else
	#define VER_BOARD	0x8AUL
#endif
#define DEV_NAME	"A4Pro"

#ifdef MP_TEST_CODE
	#define VER_TYPE	0xFFUL
#else
	#define VER_TYPE	0x01UL
#endif

#define VER_MAJOR	0x03UL	//01: pcie UART mode, 02: pcie usb host mode, 03: pcie usb combo mode
#define VER_MINOR	0x0AUL

#define CPU_MODEL		5

#define VER_BOARD_SHIFT	24
#define VER_TYPE_SHIFT	16
#define VER_MAJOR_SHIFT	8
#define VER_MINOR_SHIFT	0

#define FW_VERSION		((VER_BOARD << VER_BOARD_SHIFT) | (VER_TYPE << VER_TYPE_SHIFT) | (VER_MAJOR << VER_MAJOR_SHIFT) | (VER_MINOR << VER_MINOR_SHIFT))

#define VALID_YEAR	2025

#elif defined SENSMINIA4_NEO

/*
*		board version 
*		A4		-> 04
*		A4 Plus -> 07
*		S2 		-> 0A
*		A4 Neo	-> 0B
*
*/
#define DEV_MODEL_A4NEO	1
#if defined RELEASE_VERSION
	#define VER_BOARD	0x0BUL
#else
	#define VER_BOARD	0x8AUL
#endif
#define DEV_NAME	"A4Neo"

#ifdef MP_TEST_CODE
	#define VER_TYPE	0xFFUL
#else
	#define VER_TYPE	0x01UL	
#endif

#define VER_MAJOR	0x00UL	//00: for develop beta version
#define VER_MINOR	0x02UL

#define CPU_MODEL		5

#define VER_BOARD_SHIFT	24
#define VER_TYPE_SHIFT	16
#define VER_MAJOR_SHIFT	8
#define VER_MINOR_SHIFT	0

#define FW_VERSION		((VER_BOARD << VER_BOARD_SHIFT) | (VER_TYPE << VER_TYPE_SHIFT) | (VER_MAJOR << VER_MAJOR_SHIFT) | (VER_MINOR << VER_MINOR_SHIFT))

#define VALID_YEAR	2026
#endif

#define IKW_USE_DOUBLE


#ifdef IKW_USE_DOUBLE
typedef double IKW_FP;
typedef double SENS_FP;
#endif

#ifndef IKW_USE_DOUBLE
typedef float IKW_FP;	
typedef float SENS_FP;
#endif

/*
 *	AUTO FOTA define
 */
#define AUTO_FOTA_HOURS_1			(19)	// 0-23
#define AUTO_FOTA_MINS_1			(0)	// 0-59
#define AUTO_FOTA_SECONDS_1			(5)	// 0-59

#define AUTO_FOTA_HOURS_2			(6)	// 0-23
#define AUTO_FOTA_MINS_2			(0)	// 0-59
#define AUTO_FOTA_SECONDS_2			(5)	// 0-59

#define FOTA_TIME_UP_1				((AUTO_FOTA_HOURS_1*3600)+(AUTO_FOTA_MINS_1*60)+AUTO_FOTA_SECONDS_1)		//UTC 19:00:30 --> AM03:00:05
#define FOTA_TIME_UP_2    			((AUTO_FOTA_HOURS_2*3600)+(AUTO_FOTA_MINS_2*60)+AUTO_FOTA_SECONDS_2)		//UTC 06:00:30 --> PM14:00:05

#define AUTO_FOTA_HOURS_1_TEST		(10)	// 0-23
#define AUTO_FOTA_MINS_1_TEST		(00)	// 0-59
#define AUTO_FOTA_SECONDS_1_TEST	(0)
#define FOTA_TIME_UP_1_TEST			((AUTO_FOTA_HOURS_1_TEST*3600)+(AUTO_FOTA_MINS_1_TEST*60)+AUTO_FOTA_SECONDS_1_TEST)		//UTC 19:00:30 --> AM03:00:05
#define FOTA_TIME_UP_2_TEST			((AUTO_FOTA_HOURS_2*3600)+(AUTO_FOTA_MINS_2*60)+AUTO_FOTA_SECONDS_2)		//UTC 06:00:30 --> PM14:00:05

#define TEST_AUTO_FOTA_FIX_TIME		0

#define SUPPORT_FOTA	1
	#define SUPPORT_FOTA_MD5	1
	#define USE_MBEDTLS_MD5		1



#endif