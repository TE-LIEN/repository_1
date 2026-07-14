#ifndef __SYS_STRUCT_H__
#define __SYS_STRUCT_H__

#include "sensminiCfg.h"
#include "sysConfig.h"
#include "sysEnum.h"

#ifndef PARAM_IS_JSON
typedef struct _SectionData
{	char                *key;
	char                *value;
	struct _SectionData *next;
}SectionData, INI_SECTION_DATA;

typedef struct _Section
{	char            	*name;
	INI_SECTION_DATA    *data;
	struct _Section 	*next;
}Section, INI_SECTION;

typedef struct _MiniFile
{	char *file_name;
	INI_SECTION *section;
}MiniFile;
#endif

#ifdef SUPPORT_WIRED_LAN
typedef struct _WIRED_CONFIG
{	char 		ipAddrStr[16];
	char		maskAddrStr[16];
	char		gwAddrStr[16];
	uint32_t	ipAddr;
	uint32_t	maskAddr;
	uint32_t	gwAddr;
	uint32_t	orgIpAddr;
	uint32_t	orgMaskAddr;
	uint32_t	orgGwAddr;
	uint8_t		mdvpnType;
}WIRED_CONFIG;
#endif

typedef struct _DNS_SERVER_CONFIG
{	char 		dnsServerStr[16];
	uint32_t	dnsSvrAddr;
}DNS_SERVER_CONFIG;

typedef struct _DNS_CONFIG
{	struct _DNS_CONFIG *next;
	char 		dnsServerStr[16];
	uint32_t	dnsSvrAddr;
}DNS_CONFIG;

typedef struct _NTP_CONFIG
{	struct _NTP_CONFIG *next;
	char 	*ip;
	int 	port;
}NTP_CONFIG;

typedef struct _MOBILE_CONFIG
{	char 			*apn;	//32 byte
	char 			*simAccount;	//32 byte
	char 			*simPassword;	//32 byte
	uint8_t			simAuth;
	uint8_t			channel;
	int32_t			plmn;	//6 byte
	//uint8_t		enAgps;
}MOBILE_CONFIG;

typedef struct _NB_BAND_CFG
{	uint8_t		nbBandSel;
	uint32_t	nbLteBand;
	uint32_t	nbLteBandOver64;
	uint8_t		nbTxPwr;
}NB_BAND_CFG;

typedef struct _LTE_AGPS_CONFIG
{	uint8_t		le910AgpsType;
	uint16_t	suplPort;
	uint8_t		*suplServer;
}LTE_AGPS_CONFIG;

typedef struct _NB_AGPS_CONFIG
{	uint8_t		me310AgpsType;
	uint32_t	me310ColdStart;
	uint32_t	me310HotStart;
	uint32_t	me310PollInterval;
}NB_AGPS_CONFIG;

typedef struct _WIRELESS_CONFIG
{	int8_t				channelType[2];
	MOBILE_CONFIG		*mobileCfgs[2];
	MOBILE_CONFIG		*lteCfg;
	MOBILE_CONFIG		*nbCfg;
	LTE_AGPS_CONFIG		*lteAgpsCfg;
	NB_AGPS_CONFIG		*nbAgpsCfg;
	uint32_t			mobileInterval;
	uint8_t				mdvpnType;
	uint8_t				agpsEnable;
}WIRELESS_CONFIG;

typedef struct _OTA_CONFIG
{	char		*domainname;
	uint16_t	port;
}OTA_CONFIG;

typedef struct _NET_CONFIG
{	uint8_t			connPriority;
#ifdef SUPPORT_WIRED_LAN
	WIRED_CONFIG	wiredCfg;
#endif
	DNS_CONFIG		*dnsCfgs;
	NTP_CONFIG		*ntpCfgs;
	WIRELESS_CONFIG	wirelessCfg;
	OTA_CONFIG		*otaCfg;
}NET_CONFIG;

typedef struct _SENS_SYS_CONFIG
{	uint32_t	recordSec;
	uint32_t	autoSendInterval;
	uint8_t		psm;
	uint8_t 	logToSd;
	uint8_t		sysIsInstallMode;
	uint8_t		alertEnable;
	uint8_t		autoFotaChk;
	uint8_t		imgServerType;
	int32_t		WakeUpInterval;
	float		alertValue;
	float		alertRecoveryMargin;
	uint32_t	cameraRecInterval;
	uint32_t	cameraAlertInterval;
}SENS_SYS_CONFIG;


typedef struct _FORMULA_STRUCT
{	struct _FORMULA_STRUCT *next;
	uint8_t	formulaIdx;
	uint8_t type;
	//char *formulaStr;
	char *aStr;
	char *bStr;
	char *cStr;
	float a;
	float b;
	float c;
}FORMULA_STRUCT;

typedef struct _MQTT_TOPIC_CONFIG
{	struct _MQTT_TOPIC_CONFIG	*next;
	char						*topic;
	int							topicType;	//only for senslink mqtt
}MQTT_TOPIC_CONFIG;

typedef struct _MQTT_CONNECT_INFO
{	//struct _MQTT_CONNECT_INFO	*next;
	//uint8_t					servIdx;
	//uint8_t						infoIdx;
	union
	{	char			*clientId;	//client ID
		char			*equipId;
	};
	union
	{	char			*userName;
		char 			*apiKey;
	};
	char 				*password;
	MQTT_TOPIC_CONFIG	*pubTopics;
	MQTT_TOPIC_CONFIG	*subTopics;
}MQTT_CONNECT_INFO, IOA_CONNECT_INFO;

//#if SUPPORT_IOA_WEB_API
/*typedef struct _IOA_CONNECT_INFO
{	//struct _IOA_CONNECT_INFO *next;
	//uint8_t 				servIdx;
	char 					*equipId;
	char 					*apiKey;
}IOA_CONNECT_INFO;*/
//#endif

typedef struct _CLOUD_SERVER_INFO
{	struct _CLOUD_SERVER_INFO	*next;
	char 		serverDomainName[128];
	uint16_t	serverPort;
	uint16_t	serverProtocol;
	int8_t		servIdx;
	//for ctrl
	int8_t		connectionFail;
	uint32_t	waitForConnectionTimer;
	void 		*servInst;
	void		*servConnectInfo;
}CLOUD_SERVER_INFO;

typedef struct _DO_CONFIG
{	uint8_t 	mode;
	uint8_t		powerCtrl[8];
	uint8_t		DO24VPwr;
}DO_CONFIG;

typedef struct _SPEC_FEATURE_CONFIG
{	uint8_t	isPressureWaterLevel;
	uint8_t	isGateCtrlMode;
	uint8_t	isYlPumpMode;
	uint8_t isCyPumpMode;
}SPEC_FEATURE_CONFIG;

typedef struct _COMPOSITE_OSA_CONFIG
{	//int 	enable;
	int		radarDevNo;
	int		radarPwrSrc;
	int		radarPwrSts;
	int		wlsDevNo;
	int		wlsPwrSrc;
	int		wlsPwrSts;
	IKW_FP	alertTrigVal;
	IKW_FP	alertRecoveryVal;
	IKW_FP	installHeight;
	IKW_FP	radarBlindArea;
	IKW_FP	distance;
	IKW_FP	altitude;
}COMPOSITE_DEV_CONFIG, COMPOSITE_OSA_CONFIG, COMPOSITE_SIEMENS_CONFIG;

typedef struct _VW_SENSOR_CONFIG
{	int 		devNo;
	int 		slaveId;
	int 		pwrSrc;
	uint32_t	pwrStableTime;
	uint32_t	pollingInterval;
}VW_SENSOR_CONFIG;

typedef struct _OSA24G_CONFIG
{	uint16_t 	devNo;
	uint8_t		slaveId;
	uint8_t		alertMode;
	uint8_t		wlSensor;
	uint8_t		pwrSrc;
	uint8_t		wlsDiNo;
	uint8_t		wlsDoNo;
	uint32_t	maxDistance;
	uint32_t	minDistance;
	float		alertThreshold;
	uint32_t	pollingInterval;
}OSA24G_CONFIG;

typedef struct _AR77_RADAR_CONFIG
{	//char  		isValid;
	char		mode;
	char		autoResolution;
	uint32_t 	maxDistance;
	uint32_t 	minDistance;
	int 		xRange;
	int			offset;
	int			isRadarWithWaterDet;
}AR77_RADAR_CONFIG;

typedef struct _AVDS_RADAR_CONFIG
{	//uint8_t			isValid;
	uint8_t			mode;
	int8_t			verticalStartAzimuth;
	int8_t			verticalEndAzimuth;
	uint16_t		verticalStartRange;
	uint16_t		verticalEndRange;
	uint8_t			tiltAngle;
	uint8_t 		tiltAngleCalibration;
	//uint16_t		tiltRange;
	uint16_t		waterLevel;
	uint16_t		distOf2Radar;
	uint8_t			tiltBinRangeL;
	uint8_t			tiltBinRangeH;
	uint8_t			unit;
	uint32_t		installHeight;
}AVDS_RADAR_CONFIG;

typedef struct _CAMERA_CONFIG
{	int resolution;
	int quality;
}CAMERA_CONFIG;

typedef struct _EXT_DEV_CONFIG
{	struct _EXT_DEV_CONFIG	*next;
	int						ifChannel;	//RS485-1, RS485-2, RS232?, Ethernet
	int						ifProtocol;
	char					*command;
	int						startParseByte;
	int						dataType;
	int						dataOrder;
	int 					devIdx;
	int						devModel;	//OSA24, AVDS, OSA77G, VW, camera...
	int8_t					enable;
	int8_t					pwrSrc;
	void					*speDevCfg;
}EXT_DEV_CONFIG;

typedef struct _COMM_IF_CONFIG
{	struct _COMM_IF_CONFIG	*next;
	int						channelNo;
	int						protocol;
	int						baudrate;
	int						format;	//8-N-1...
}COMM_IF_CONFIG;

typedef struct _PQ_SRC
{	int interface;
	int devNo;
	int devChanNo;	//measure point
}PQ_SRC;

typedef struct _PQ_ALARM_CONFIG
{
}PQ_ALARM_CONFIG;

typedef struct _PQ_CONFIG
{	struct _PQ_CONFIG		*next;
	int						pqIdx;
	PQ_SRC					pqSrc;
	//PQ_FORMULA			*pqFormulas;
	int						formulaId;
	//PQ_LOGIC_TRIG			*pqLogic;
	int8_t					filterType;
	char					*alias[UPLOAD_SERVER_CNT];	//for send to cloud name
	PQ_ALARM_CONFIG			*pqAlarmCfg;	//feature use
	float					lowLimit;	//type? float/int32/int16??
	float					highLimit;
}PQ_CONFIG;

typedef struct _AI_CONFIG
{	uint8_t		enable;
	uint8_t		sensorType[6];	//4~20mA, 0~20mA, +-20mA, 500mV, 5V, 10V, +-6V, 0~2 Current, 3~4 Volt, 5 current
	uint8_t 	differential;	//only for Volt channel
	//uint8_t		type[6];	//current, volt
}AI_CONFIG;

typedef struct _DI_CONFIG
{	char		diWakeup[6];
	//uint8_t   diStatus;
	uint8_t   	cyPumpMode;
	//char			diTaskSel[4];
	uint8_t		diStatusRecord;
	uint32_t	diWakeupRecInterval;
}DI_CONFIG;

typedef struct _SYS_CONFIG
{	SENS_SYS_CONFIG			sensSysCfg;
	NET_CONFIG				netCfg;
	FORMULA_STRUCT			*formula;
	CLOUD_SERVER_INFO		*servInfos;
	MQTT_CONNECT_INFO		mqttConnInfoTemp[UPLOAD_SERVER_CNT];	//for parser use, parser done free
	DO_CONFIG				doCfg;
	AI_CONFIG				aiCfg;
	DI_CONFIG				diCfg;
	SPEC_FEATURE_CONFIG		specFeas;
	
	PQ_CONFIG				*pqCfgs;
	COMM_IF_CONFIG			*commIfCfgs;
	EXT_DEV_CONFIG			*extDevCfgs;
	
	//put to EXT_DEV_CONFIG->speDevCfg;
	COMPOSITE_SIEMENS_CONFIG *compositeSiemensCfg;
	COMPOSITE_OSA_CONFIG	*compositeOsaCfg;	//radar wls + pressuer wls
	VW_SENSOR_CONFIG		*vwCfg;
	OSA24G_CONFIG			*osa24gCfg;
	AR77_RADAR_CONFIG		*ar77Cfg;
	AVDS_RADAR_CONFIG		*avdsCfg;
	CAMERA_CONFIG			*cameraCfg;
	
}SYS_CONFIG;

typedef struct _SYS_TIMER
{	uint64_t currTick;
	uint64_t currTimeSec;
	uint16_t currAutosendTimeSlot;
	uint16_t currHistoryRecTimeSlot;
#if REC_INTERVAL_FIXED
	uint16_t recTimeSlotInterval;
	uint16_t alertTimeSlotInverval;
#endif
	uint16_t alertCurrSendImgSlot;
	uint16_t diWakeupRecSlot;
	uint32_t currentTimeStamp;
	uint32_t startTick;
	uint32_t testPsmTick;
	uint32_t sdRecTick;
	uint32_t activeTimer;
}SYS_TIMER;

typedef struct _SYS_SEM
{	SENS_SEM_STRUCT	dbgUart;
	SENS_SEM_STRUCT	strtok;
	SENS_SEM_STRUCT	pqLock;
	//SENS_SEM_STRUCT	imgRecSem;
	SENS_SEM_STRUCT	sdLock;
	SENS_SEM_STRUCT	sdWriteLock;
	SENS_SEM_STRUCT	fsLock;
#if SPI_FILE_SYSTEM
	SENS_SEM_STRUCT	spiFsLock;
#endif
	SENS_SEM_STRUCT webDbgMsgLock;
#if AUTO_DATA_SYNC
	SENS_SEM_STRUCT	autoDataSyncLock;
#endif
#ifdef AUTO_SEND_WITH_INFO
	SENS_SEM_STRUCT	autoSendCtxLock;
#endif
	//SENS_SEM_STRUCT	rtcRamLock;
	SENS_SEM_STRUCT	disablePsmLock;
	//SENS_SEM_STRUCT	rngSem;
}SYS_SEM;

typedef struct _SYS_POWER_MANAGER
{	uint8_t		currPsm;
	uint32_t 	extendWorkingTime;
	uint32_t	disablePsmBmp;
	uint32_t	disablePsmBmpBak;
	uint32_t	psmChkBmp;
}SYS_POWER_MANAGER;

typedef struct _CAMERA_WEB_MANAGER
{	enum WEB_CAM_CAPTURE_FSM	fsm;
	enum WEB_CAM_CAPTURE_MODE	mode;
	enum WEB_CAM_CAPTURE_STS	status;
	int8_t		sendToCloud;
	int8_t		readyToSendImgToCloud;
	int8_t		readyToDeleteFile;
	uint8_t		isSupported;
	uint8_t		imgUploadToWebDone;
	char		fileName[IMG_FILE_SIZE];
	int			fileLength;
	int			base64StartPos;
	char		unalignBuf[3];
	char		unalignSize;
}CAMERA_WEB_MANAGER;

typedef struct _OTA_MANAGER
{	uint8_t 	prevOtaNotFinish;
	uint8_t 	runningAutoFota;
	uint8_t 	fotaStatus;
}OTA_MANAGER;

typedef struct _MAINTENANCE_COTEXT
{	uint8_t		isValid;
}MAINTENANCE_COTEXT;

typedef struct _SENS_LOG_STRUCT
{	struct _SENS_LOG_STRUCT	*next;
	struct _SENS_LOG_STRUCT	*prev;
	uint8_t					isValid;
	uint8_t					isTop;
	uint8_t					isProcessing;
	uint8_t					*buf;
	uint32_t				len;
	uint32_t				count;
	char 					*logFileName;
	SENS_SEM_STRUCT			logLock;
}SENS_LOG_STRUCT;

typedef struct _SYS_CTRL
{	SYS_POWER_MANAGER	pwrManager;
	CAMERA_WEB_MANAGER	camWebMng;
	OTA_MANAGER			otaMng;
	MAINTENANCE_COTEXT  maintenanceCtx;
	SENS_LOG_STRUCT		logDataArray;
	uint8_t				isReadyToReboot;
	
	uint8_t 			srvConnectBmp;
	//uint8_t				cloudActiveBmp;
	uint8_t				srvActiveBmp;	//server active from param
	uint32_t 			servIdleXmitTime[MAX_SERVER_COUNT];
	uint8_t				logIsReady;
	
	uint8_t				sendToCloudDone;
	
	uint8_t				sysIsInstallMode;
	uint8_t				bakupFlashIsPresent;
	uint8_t				paramInSd;
	uint8_t				sdCardError;
	
	uint8_t				interDi4Active;
	uint8_t				interDi4PrevActive;
	uint8_t				interDi4ReleasePrepare;
	uint8_t				interDi5Active;
	uint8_t				interDi5PrevActive;
	uint8_t				interDi5ReleasePrepare;
	
	uint32_t 			sysStandbyTime;
	
	uint32_t 			enTemporaryDisSleep;
	uint32_t			temporaryDisSleepremainTime;
	uint32_t			temporaryDisSleepEndTimer;
	
	uint32_t			sysWorkingStartTime;
	uint32_t			everydayRebootSecondTime;
#if TIME_SHIFT_OPERATE	
	uint32_t			tsoValue;
#endif
	
}SYS_CTRL;

typedef struct _DEBUG_STRUCT
{	uint8_t			*msg;
	char 			*modbusLibMsg;
	uint8_t			*webDbgMsg;
	uint32_t		webDbgMsgLength;
	uint8_t			dbgUartDisable;
}DEBUG_STRUCT;

typedef struct _SD_FILE_CTRL
{	char *fileName;
	char overwrite;
	char *buffer;
	int fileLength;
	int offset;
	int rwLength;
	int realRwLength;
	int rwMode;
	SENS_EVENT_STRUCT lwevent;
#ifndef PARAM_IS_JSON
	MiniFile *mIniFile;
#endif
}SD_FILE_CTRL;

typedef struct _SD_INSTANCE
{	uint8_t			sdPresence;
	
}SD_INSTANCE;

typedef struct _SYS_GUID
{	uint8_t guid[16];
	//uint8_t uniqueId[16];
	uint32_t ucid[4];
	uint16_t crc;
	uint8_t isDefault;
	char 	*guidString;
	char 	*ikwGuidString;
}SYS_GUID;

typedef struct _SYSTEM_ALARM
{	enum ALERT_FSM alertFsm;
	//struct _SYSTEM_ALARM *next;
	int8_t 		waitAlarmSendFinalData;
	uint8_t 	prevIsAlarm;
	uint8_t		alarmFlagIsSet;
	//int8_t	alarmUploadFinalImg;
	//uint8_t	isAlarmUploadImgMode;
}SYSTEM_ALARM;
	
typedef struct _SYS_AUTH_STRUCT
{	struct _SYS_AUTH_STRUCT *next;
	struct _SYS_AUTH_STRUCT *prev;
	char *username;
	char *psw;
	char *usernameBase64;
	char *pswBase64;
	char authType;	//admin, guest, su, invalid
}SYS_AUTH_STRUCT;

typedef struct _SYS_AUTH_INFO
{	SYS_AUTH_STRUCT *authTable;
	char adminCount;
	char userCount;
	char suCount;	//only 1, anasystem
}SYS_AUTH_INFO;

typedef struct _AUTO_DATA_SYNC_INSTANCE
{	uint8_t *srvHistoryBuf[UPLOAD_SERVER_CNT];
}AUTO_DATA_SYNC_INSTANCE;


typedef struct _SENS_SYSTEM_STRUCT
{	SYS_SEM					sysSem;
	SYS_GUID				guid;
	SYS_CONFIG				sysCfg;
	S_RTC_TIME_DATA_T		dateTime;
	SYS_TIMER				sysTimer;
	SYS_CTRL				sysCtrl;
	DEBUG_STRUCT			dbg;
	SD_INSTANCE				sdInst;
	SYSTEM_ALARM			sysAlarm;
	SYS_AUTH_INFO 			*sysAuthInf;
	uint8_t					*eepromBuf;	//EEPROM_CONTEXT
	AUTO_DATA_SYNC_INSTANCE	autoDataSyncInst;
}SENS_SYSTEM_STRUCT;


typedef struct _TASK_INFO
{	uint32_t					id;
	enum TASK_WORKING_STATUS	sts;
	SENS_SEM_STRUCT				lock;
	long						runningStartTime;
}TASK_INFO;

typedef struct _TASK_ACTIVE
{	//int				taskWorkingCount;
	//TASK_INFO	mobileSendTask;
#ifdef PPP_USE_CMUX
	TASK_INFO	cMuxTaskAct;
#endif	
	TASK_INFO	iotAtRecvTaskAct;
	TASK_INFO	servCmdProcessTaskAct;
	TASK_INFO	networkProcessTaskAct;	//networkProcessTask
	TASK_INFO	networkRecvTaskAct;	//networkRecvTask
	TASK_INFO	usbHostTaskAct;
#ifdef PLATFORM_FSL
	TASK_INFO	wireLanRecvTask;
#endif
	TASK_INFO	sensorTaskAct;
	TASK_INFO	uvcCameraTaskAct;
#ifdef SUPPORT_MQTT
	TASK_INFO	mqttTaskAct;
#endif
#ifdef SUPPORT_MODBUS_TCP 
	TASK_INFO	modbusTaskAct;
#endif
	TASK_INFO	expanderTaskAct;
}TASK_ACTIVE;



#endif
