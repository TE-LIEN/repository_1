#ifndef __SENS_DEVICE_H__
#define __SENS_DEVICE_H__

#include "sensminiCfg.h"

#define OTP_GUID_START_ADDR		240	//

#define IMG_REC_FILE_NAME	"imgUpload.bin"
#define DI_NO_WIRED		-1

#define DO_NO_WIRED		-1

#define MAX_WAKEUP_IO	18

#define EEPROM_STRUCT_VERSION	1
#define EEPROM_STRUCT_TAG		0x55AA
#define RTC_IO_CTRL_BY_GPIO		0
#define RTC_IO_CTRL_BY_VBAT		1

#define EEPROM_GUID_ADDR		0x80	//δ?핾҆??OTP
#define EEPROM_CONTEXT_ADDR		0

#define MAX_EEPROM_BUF_SIZE		256
#define FORCE_WRITE_EEPROM_INFO_WHEN_ERROR	1

enum EXEC_HRAM_FLAG
{	EXEC_HRAM_FLAG_NONE,
	EXEC_HRAM_FLAG_TFTP_UPDATE,		//load boot zone 2
	EXEC_HRAM_FLAG_FOTA,			//load boot zone 2
	EXEC_HRAM_FLAG_RS232_UPDATE,	//load boot zone 2
	EXEC_HRAM_FLAG_ZONE_3,
	EXEC_HRAM_FLAG_ZONE_4,
	EXEC_HRAM_FLAG_ZONE_5,
	EXEC_HRAM_FLAG_ZONE_6,
	EXEC_HRAM_FLAG_MAX
};

typedef struct _EEPROM_GUID_CONTEXT
{	uint8_t		guid[16];
	uint16_t 	guidCrc16;
	uint16_t	pad;
}EEPROM_GUID_CONTEXT;	//20 Byte

typedef struct _EEPROM_CONTEXT
{	uint16_t	version;	//V1.0
	uint16_t	tag;
	uint8_t		ipAddr[4];
	uint8_t		ipMask[4];
	uint8_t		ipRouter[4];
	int8_t		diMap[8];
	int8_t		doMap[8];
	uint8_t	 	extDiCnt;
	uint8_t	 	extDoCnt;
	uint16_t 	eepromCrc16;
}EEPROM_CONTEXT;


enum IRQ_WAKEUP_FLAGS
{
#if BOARD_VERSION == 1
	WAKEUP_DI0 = 0,
	WAKEUP_DI1,
	WAKEUP_DI2,
#else
	WAKEUP_DI2 = 0,
#endif
	WAKEUP_DI3,
	WAKEUP_DI4,
	WAKEUP_DI5,
#if BOARD_VERSION == 2
	WAKEUP_DI0,
	WAKEUP_DI1,
#else
	WAKEUP_PAD6,
	WAKEUP_PAD7,
#endif
	WAKEUP_MK1_GPIO0,
	WAKEUP_MK1_GPIO1,
	WAKEUP_MK2_GPIO0,
	WAKEUP_TEMP_ALERT,
	WAKEUP_I2C0_IRQ,
	WAKEUP_I2C1_IRQ,
	WAKEUP_I2C2_IRQ,
	WAKEUP_MK2_GPIO1,
	WAKEUP_IOT1,
	WAKEUP_IOT2,
	WAKEUP_MAX
};

#if BOARD_VERSION == 1
#define DEFAULT_IRQ_WAKEUP_FLAGS	(~((1 << WAKEUP_DI4) | (1 << WAKEUP_DI5))) >> 2
#elif BOARD_VERSION == 2
#define DEFAULT_IRQ_WAKEUP_FLAGS	~((1 << WAKEUP_DI4) | (1 << WAKEUP_DI5))
#endif


enum RTC_SPARE_REG_ITEM
{	RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG = 0,
	RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1,	//save SYS_STATUS_FLAG1
	RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG2,	//save SYS_STATUS_FLAG2
	RTC_SPARE_REG_ITEM_GPS_LATITUDE,
	RTC_SPARE_REG_ITEM_GPS_LONGITUDE,
	RTC_SPARE_REG_ITEM_NEXT_WAKEUP_TIMESTAMP,
	//RTC_SPARE_REG_ITEM_REBOOT_TIMES,	//move to 0 high word
	RTC_SPARE_REG_ITEM_SYS_CTRL_FLAG,
	
	
	//RTC_SPARE_REG_ITEM_ISP_UART_INFO,
	RTC_SPARE_REG_ITEM_DI0_COUNTER = 16,
	RTC_SPARE_REG_ITEM_DI1_COUNTER,
	RTC_SPARE_REG_ITEM_DI2_COUNTER,
	RTC_SPARE_REG_ITEM_DI3_COUNTER,
	RTC_SPARE_REG_ITEM_MAX = 20
};

typedef union _REC_SLOT_FLAG
{	uint32_t	recSlotFlag;
	struct
	{	uint32_t	currHistoryRecSlot:16;
		uint32_t	rebootCnt:16;
	};
}REC_SLOT_FLAG;

typedef union _SYS_STATUS_FLAG1	//for app <-> boot loader use
{	uint32_t sysStatusFlags;
	struct
	{	uint32_t sysStatusLoWord:16;
		uint32_t sysStatusHiWord:16;
	};
#if BOARD_VERSION == 1
	struct
	{	uint32_t noneIrqSts:14;
		uint32_t irqSts:18;
	};
#else
	struct
	{	uint32_t noneIrqSts:16;
		uint32_t irqSts:16;
	};
#endif
	struct
	{	uint32_t	diCounterEnableInPsm:1;
		uint32_t	di0CntEn:1;	//wakeup then 
		uint32_t	di1CntEn:1;
		uint32_t	di2CntEn:1;
		uint32_t	di3CntEn:1;
		uint32_t	execHramFlag:3;
		//uint32_t	runTftpUpdate:1;
		//uint32_t	runFotaUpdate:1;
		//uint32_t	runRs232Update:1;
		uint32_t	otaFlag:2;		//
		uint32_t	psmMode:2;		//0: PSM NONE, 1: Normal PSM, 2: Advance PSM
		uint32_t	psmMethod:2;	//0, 1: Wakeup Timer, 2: MCU DPD - use Boot zone2 to enter DPD
#if BOARD_VERSION == 1
		uint32_t	di0IrqSts:1;
		uint32_t	di1IrqSts:1;
#else
		//uint32_t	pad14:2;
		uint32_t	loadHramInfo:2;	//load info, when use boot zone 3~n
#endif
		uint32_t	extGpioIn0IrqSts:1;	//DI2
		uint32_t	extGpioIn1IrqSts:1;	//DI3
		uint32_t	extGpioIn2IrqSts:1;	//DI4
		uint32_t	extGpioIn3IrqSts:1;	//DI5
		uint32_t	extGpioIn4IrqSts:1;	//DI6	//board ver 2 --> DI0
		uint32_t	extGpioIn5IrqSts:1;	//NC	//board ver 2 --> DI1
		uint32_t	extGpioIn6IrqSts:1;	//MK1 GPIO0
		uint32_t	extGpioIn7IrqSts:1;	//MK1 GPIO1
		uint32_t	extGpioIn8IrqSts:1;	//MK2 GPIO0
		uint32_t	extGpioIn9IrqSts:1;	//TEMP ALERT
		uint32_t	extGpioIn10IrqSts:1;	//I2C0 IRQ
		uint32_t	extGpioIn11IrqSts:1;	//I2C1 IRQ
		uint32_t	extGpioIn12IrqSts:1;	//I2C2 IRQ
		uint32_t	extGpioIn13IrqSts:1;	//MK2 GPIO1
		uint32_t	extGpioIn14IrqSts:1;	//IOT1_nWAKE
		uint32_t	extGpioIn15IrqSts:1;	//IOT2 nWAKE
	};
}SYS_STATUS_FLAG1;

typedef union _SYS_STATUS_FLAG2
{	uint32_t sysStatusFlags;
	struct
	{	uint32_t sysStatusLoWord:16;
		uint32_t sysStatusHiWord:16;
	};
#if BOARD_VERSION == 1
	struct
	{	uint32_t	unTriggerVal:14;
		uint32_t	triggerVal:18;
	};
#else
	struct
	{	uint32_t	unTriggerVal:16;
		uint32_t	triggerVal:16;
	};
#endif
	struct
	{	uint32_t	sdRecDone:1;
		uint32_t	cameraUploadDone:1;
		uint32_t	pqUploadDone:1;
		uint32_t	alarmActive:1;
		uint32_t	turnOnMobileForAlarm:1;
		uint32_t	logFileClearSts:1;
		uint32_t	pad6:1;
		uint32_t	pad7:1;
		uint32_t	pad8:1;
		uint32_t	pad9:1;
		uint32_t	pad10:1;
		uint32_t	pad11:1;
		uint32_t	pad12:1;
		uint32_t	pad13:1;
#if BOARD_VERSION == 1
		uint32_t	di0Val:1;
		uint32_t	di1Val:1;
#else
		uint32_t	pad14:2;
#endif
		uint32_t	extGpioIn0Val:1;	//DI2
		uint32_t	extGpioIn1Val:1;	//DI3
		uint32_t	extGpioIn2Val:1;	//DI4	// force wakeup system
		uint32_t	extGpioIn3Val:1;	//DI5	// force wakeup system
		uint32_t	extGpioIn4Val:1;	//DI6	//board ver 2 --> DI0
		uint32_t	extGpioIn5Val:1;	//NC	//board ver 2 --> DI1
		uint32_t	extGpioIn6Val:1;	//MK1 GPIO0
		uint32_t	extGpioIn7Val:1;	//MK1 GPIO1
		uint32_t	extGpioIn8Val:1;	//MK2 GPIO0
		uint32_t	extGpioIn9Val:1;	//TEMP ALERT
		uint32_t	extGpioIn10Val:1;	//I2C0 IRQ
		uint32_t	extGpioIn11Val:1;	//I2C1 IRQ
		uint32_t	extGpioIn12Val:1;	//I2C2 IRQ
		uint32_t	extGpioIn13Val:1;	//MK2 GPIO1
		uint32_t	extGpioIn14Val:1;	//IOT1_nWAKE
		uint32_t	extGpioIn15Val:1;	//IOT2 nWAKE
		
	};
}SYS_STATUS_FLAG2;

typedef union _SYS_CTRL_FLAG
{	uint32_t	sysCtrlFlags;
	struct
	{	uint32_t sysCtrlLoWord:16;
		uint32_t sysCtrlHiWord:16;
	};
	struct
	{	uint32_t	otherCtrl:16;
		uint32_t	wakeupKeepOn0:2;	//DI2 & DI3
		uint32_t	otherCtrl1:2;
		uint32_t	wakeupKeepOn1:2;	//DI0 & DI1
		uint32_t	otherCtrl2:10;
	};
	struct
	{	uint32_t	wdtActiveCnt:4;
		uint32_t	diWakeupFromPsm:1;	//from boot loader
		uint32_t	pad5:1;
		uint32_t	pad6:1;
		uint32_t	pad7:1;
		uint32_t	pad8:1;
		uint32_t	pad9:1;
		uint32_t	pad10:1;
		uint32_t	pad11:1;
		uint32_t	pad12:1;
		uint32_t	pad13:1;
#if BOARD_VERSION == 1
		uint32_t	di0WakeupEn:1;	//version 1 always disable
		uint32_t	di1WakeupEn:1;	//version 1 always disable
#else
		uint32_t	pad14:2;
#endif
		uint32_t	extGpioIn0WakeupEn:1;	//DI2
		uint32_t	extGpioIn1WakeupEn:1;	//DI3
		uint32_t	extGpioIn2WakeupEn:1;	//DI4	// force wakeup system
		uint32_t	extGpioIn3WakeupEn:1;	//DI5	// force wakeup system
		uint32_t	extGpioIn4WakeupEn:1;	//DI6	//board ver 2 --> DI0
		uint32_t	extGpioIn5WakeupEn:1;	//NC	//board ver 2 --> DI1
		uint32_t	extGpioIn6WakeupEn:1;	//MK1 GPIO0
		uint32_t	extGpioIn7WakeupEn:1;	//MK1 GPIO1
		uint32_t	extGpioIn8WakeupEn:1;	//MK2 GPIO0
		uint32_t	extGpioIn9WakeupEn:1;	//TEMP ALERT
		uint32_t	extGpioIn10WakeupEn:1;	//I2C0 IRQ
		uint32_t	extGpioIn11WakeupEn:1;	//I2C1 IRQ
		uint32_t	extGpioIn12WakeupEn:1;	//I2C2 IRQ
		uint32_t	extGpioIn13WakeupEn:1;	//MK2 GPIO1
		uint32_t	extGpioIn14WakeupEn:1;	//IOT1_nWAKE
		uint32_t	extGpioIn15WakeupEn:1;	//IOT2 nWAKE
	};
}SYS_CTRL_FLAG;

typedef union _SYS_WAKE_UP_SRC
{	uint32_t wakeUpSrc;
	struct
	{	uint32_t gpc0Wk:1;	//PINWK0, PC.0
		uint32_t tmrWk:1;	//TMRWK
		uint32_t rtcWk:1;	//RTCWK	
		uint32_t gpb0Wk:1;	//PINWK1, PB.0
		uint32_t gpb2Wk:1;	//PINWK2, PB.2
		uint32_t gpb12Wk:1;	//PINWK12, PB.12
		uint32_t gpf6Wk:1;	//PF.6
#if defined SENSMINIS2
		uint32_t reserved7:1;
#elif defined SENSMINIA4_NEO
		uint32_t vBusWk:1;	//PA.12
#endif
		uint32_t gpaWk:1;	//GPIO-A
		uint32_t gpbWk:1;	//GPIO-B
		uint32_t gpcWk:1;	//GPIO-C
		uint32_t gpdWk:1;	//GPIO-D
		uint32_t lvrWk:1;	//LVR 
		uint32_t bodWk:1;	//BOD
#ifdef SENSMINIA4_NEO
		uint32_t reserved14:1;
		uint32_t rstWk:1;
#endif
		uint32_t acmpWk:1;
#ifdef SENSMINIA4_NEO
		uint32_t acmp1Wk:1;
		uint32_t acmp2Wk:1;
		uint32_t acmp3Wk:1;
#endif
#ifdef SENSMINIS2
		uint32_t tamperWk:1;
#endif
	};
}SYS_WAKE_UP_SRC;

typedef union _SYS_RESET_SRC
{	uint32_t resetSrc;
	struct
	{	uint32_t por:1;
		uint32_t nRstPin:1;
		uint32_t wdt:1;
		uint32_t lvr:1;
		uint32_t bod:1;
#if defined SENSMINIS2
		uint32_t sys:1;
#elif defined SENSMINIA4_NEO
		uint32_t mcuRst:1;
		uint32_t hrstPin:1;
#endif
		uint32_t cpu:1;
		uint32_t cpuLockUp:1;
	};
}SYS_RESET_SRC;


typedef union _VBAT_IO_CTL
{	uint32_t regVal;
	struct
	{	uint32_t io0Sts:6;
		uint32_t io0Res:2;
		uint32_t io1Sts:6;
		uint32_t io1Res:2;
		uint32_t io2Sts:6;
		uint32_t io2Res:2;
		uint32_t io3Sts:6;
		uint32_t io3Res:2;
	};
	struct
	{	uint32_t io0Mode:2;
		uint32_t io0OutVal:1;
		uint32_t io0DinEn:1;
		uint32_t io0PuSel:2;
		uint32_t io0Pad:2;
		uint32_t io1Mode:2;
		uint32_t io1OutVal:1;
		uint32_t io1DinEn:1;
		uint32_t io1PuSel:2;
		uint32_t io1Pad:2;
		uint32_t io2Mode:2;
		uint32_t io2OutVal:1;
		uint32_t io2DinEn:1;
		uint32_t io2PuSel:2;
		uint32_t io2Pad:2;
		uint32_t io3Mode:2;
		uint32_t io3OutVal:1;
		uint32_t io3DinEn:1;
		uint32_t io3PuSel:2;
		uint32_t io3Pad:2;
	};
}VBAT_IO_CTL;

extern volatile SENS_DEV_STRUCT *sensDev;
extern volatile SYS_CTRL_FLAG	sysCtrlFlag;
extern volatile SYS_WAKE_UP_SRC wakeupSrc;
extern volatile SYS_RESET_SRC sysResetSrc;
extern volatile REC_SLOT_FLAG	sysRecSlotFlag;	
extern volatile SYS_STATUS_FLAG1 sysStsFlag1;
extern volatile SYS_STATUS_FLAG2 sysStsFlag2;

extern volatile SYS_STATUS_FLAG2 inputExpVal;

extern void getGuidFromOtp(void);
extern void addUartCfg(UART_CONFIG *uartCfg);
extern void delUartCfg(UART_CONFIG *uartCfg);
extern void *getExtDevSpeConfig(int devModel);
extern COMM_IF_CONFIG *getCommIfConfigByIfNo(int iFaceNo);
//extern EXT_DEV_CONFIG *getExtDevConfig(int devIdx);

extern void addExtDevConfig(EXT_DEV_CONFIG *extDevCfg);
extern EXT_DEV_CONFIG *getExtDevCfgByIdx(int idx);
extern void specExtDevBind(void);
extern int setSwPq(DEV_INSTANCE *devInst, SENSOR_HW_PQ_STRUCT *topHwPqInf);

extern int clearImgRecInfo(IMAGE_UPLOAD_INFO *imageInfo);
extern int checkUnSendImg(IMAGE_UPLOAD_INSTANCE *imgUploadInst);


extern uint32_t getVBatRegValue(uint32_t num);
extern void setVBatRegValue(uint32_t num, uint32_t value);

extern int getEepromInfo(void);
extern int findDiMap(int diNumber);
extern void setEepromInfo(uint32_t startAddr, uint8_t *buf);
#endif

