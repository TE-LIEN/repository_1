#ifndef __DEVICE_STRUCT__
#define __DEVICE_STRUCT__

#include "sensminiCfg.h"
#include "deviceConfig.h"
#include "deviceEnum.h"

typedef struct _UART_CONFIG
{	UART_CTX 				*ctx;
	uint32_t				baudrate;
	uint8_t 				flowCtrl;
	enum UART_MODE 			mode;
	uint8_t					id;
	uint8_t					dataLen;
	uint8_t					parity;
	uint8_t					stopbit;
	uint32_t				bufferLength;
	uint8_t					uartType;	
#ifdef USE_USB_HOST_LIB
	uint8_t					isUsbHost;
	void					*usbCtx;
#endif
}UART_CONFIG;

typedef struct _IO_PWR_CTL
{	uint32_t 	powerEnableCount;
	uint32_t	currSts;
	uint32_t	onBmp;
}IO_PWR_CTL;

typedef struct _DI_CHG_CONTEXT
{	struct _DI_CHG_CONTEXT *next;
	uint64_t				unixTime;
	uint8_t					sts;
	uint8_t					chg;
}DI_CHG_CONTEXT;

typedef struct _DI_CONTEXT
{	char				diWakeupTimeFlag;
	//long				diLowTimer;
	int8_t				diStsOld[DIQUANTITY];
	int8_t				diStsNew[DIQUANTITY];
	int8_t				diStsChg[DIQUANTITY];
	int8_t				diStsChgTemp[DIQUANTITY];
	int8_t				diWiredMap[DIQUANTITY];
	//int				dateTime[7];
	int					isWakeupFromDi;
	int					isDiKeepOn;
	SENS_SEM_STRUCT		diChgSem;
	DI_CHG_CONTEXT		*diChgCtx;
	enum DI_KEEP_ON_FSM	keepOnFsm;
	int					diReleaseStartTimer;
	uint32_t 			diRelaseFlag;
}DI_CONTEXT;

typedef struct _DI_INSTANCE
{	void		*diCfg;	//from syscfg
	DI_CONTEXT	diCtx;
}DI_INSTANCE;

typedef struct _DO_CONTEXT
{	uint8_t doSts[DOQUANTITY];
	uint32_t doOnCnt[DOQUANTITY];
	uint32_t do24vOnCnt;
}DO_CONTEXT;

typedef struct _DO_INSTANCE
{	void		*doCfg;
	DO_CONTEXT	doCtx;
}DO_INSTANCE;

typedef struct _IMAGE_UPLOAD_REC_HEADER	//struct _ImageSendRec
{	uint16_t 		tag;
	uint8_t			version;
	uint8_t			valid;
	//uint8_t		pad;
	uint32_t		numOfUnSendImage;
	//ImageInfo		imageInfo;
}IMAGE_UPLOAD_REC_HEADER;//IMAGE_UPLOAD_REC_CONTEXT;	//ImageSendRec;

/*typedef struct _CMAERA_DEV
{	enum CAMERA_FSM	cameraFsm;
	enum CAMERA_FSM	cameraFsmNext;
	uint8_t			isValid;
	uint8_t			uartChannel[2];
	uint8_t			isDualMode;
	uint8_t			cameraId[2];	//for dual mode max 2 camera, for Q1 use, A4 power weak
	uint8_t 		isRunning;
	//uint8_t		isCameraWorking;
	//uint8_t		is3MPixelCamera;
	uint8_t			cameraType;
	uint8_t 		isReady;
	uint8_t 		rtyTimes;
	uint8_t 		needResetCamera;
	uint8_t 		currCameraId;
	uint8_t			currResolution;
	uint8_t 		getRawRtyTimes;
	uint8_t			isSuccess;
	char			cameraFileName[IMG_FILE_SIZE];
	UART_CONFIG		*uartCfg;
	uint8_t 		*imgBuf;
	uint32_t		imgBufOffset;
	uint32_t		imgBufLength;
	uint32_t		currQuility;
	uint8_t			*sendBuf;
	uint32_t		jpgRawOffset;
	uint32_t		jpgSize;
	uint32_t		cameraRdyTimer;
	IMAGE_UPLOAD_REC_CONTEXT 	*imageSendRec;
	char 			*recImgSendInfoBuf;
	int				recImgSendInfBufLen;
	char			enablePostImgTimeout;
	long			postImgTimer;	
	uint32_t		fsmWaitTimer;
	uint32_t		fsmWaitTimeout;
#if TEST_N_REMOVE
	Camera_Web_Shot	cameraWebShot;
#endif
}CAMERA_DEV;*/


/*
 *	modbus
 */

typedef struct _MODBUS_REQ_CONTEXT
{	char		*ip;
	uint8_t		dataType;
	uint8_t		dataOrder;
	uint8_t		slaveId;
	uint8_t		funcCode;
	uint16_t	regNo;
	uint16_t 	readLength;
	uint16_t	*writeBuf;
	uint32_t	rspTimer;
	uint32_t	rspTimeout;		
}MODBUS_REQ_CONTEXT;

typedef struct _MODBUS_RSP_CONTEXT
{	uint8_t 	*rspBuf;
	uint8_t		*rspBufBak;
	uint8_t		isDataGet;
	uint8_t		isRspTimeout;
	uint32_t	rspBufSize;
	uint32_t	rspTimeoutCnt;
}MODBUS_RSP_CONTEXT;

typedef struct _MODBUS_INSTANCE
{	struct _MODBUS_INSTANCE		*next;
	struct _MODBUS_INSTANCE		*prev;
	enum EXT_DEV_TYPE			extDevType;
	void 						*devInst;
	enum MODBUS_MODE			mode;
	enum MODBUS_RUN_FSM			fsm;
	uint8_t						channel;
	void						*modbusCtx;	//tcp or rtu
	MODBUS_REQ_CONTEXT			reqCtx;
	MODBUS_RSP_CONTEXT			rspCtx;
}MODBUS_INSTANCE;

typedef struct _DCON_INSTANCE
{	struct _DCON_INSTANCE	*next;
	struct _DCON_INSTANCE	*prev;
	void 					*sensorInst;
	enum DCON_DEV_FSM		fsm;
	enum EXT_DEV_TYPE		extDevType;
	uint8_t					channel;
	UART_CONFIG				*uartCfg;
	char					*sendBuf;
	char					*recvBuf;
	int						recvLength;
	int						startParseByte;
	int						parserLength;
	int						parserType;
	int						rspTimer;
	int						rspTimeout;
	int						isRspTimeout;
	int						isPqReady;
	int						timeoutCount;
}DCON_INSTANCE;

typedef struct _SENSOR_ALGO_STRUCT
{	float	*valArray;
	float	wlVal;
	float	*stdValArray;
	uint8_t valCount;
	uint8_t valPos;
	uint8_t	maxCount;
}SENSOR_ALGO_STRUCT, IKW_SMR_SENSOR_STRUCT;

typedef struct _SENSOR_HW_PQ_STRUCT
{	struct _SENSOR_HW_PQ_STRUCT *next;
	int							hwPqIdx;
	float						rawPqVal;
	float						pqVal;
	float						filterPqVal;
	int							isRawHwPqReady;
	int							isHwPqReady;
	int							isHwFilterPqRdy;
	int							seqNanCnt;
	uint32_t					emptyPqCnt;
	uint32_t					emptyPqError;
	SENSOR_ALGO_STRUCT			*prevFilterAlgo;
}SENSOR_HW_PQ_STRUCT;

typedef struct _SENSOR_PROCESS_CONTEXT
{	enum MODBUS_DEV_FSM	fsm;
	uint32_t 			pollingTimer;
	uint32_t 			pollingTimeout;
	uint32_t			pwrOnDelayTimer;
	uint32_t			pwrOnDelayTimeout;
	uint32_t			pwrOffDelayTimer;
	uint32_t			pwrOffDelayTimeout;
	uint32_t			activeWaitTimer;
	uint32_t			activeWaitTimeout;
	uint8_t				skipZeroValue;
	uint8_t				isNotFirst;
	uint32_t			pollingInterval;
}SENSOR_PROCESS_CONTEXT;

typedef struct _OSA24G_REC_INFO
{	uint32_t recFilterTimestamp;
	uint32_t recRealTimestamp;
	uint32_t recFilterPqVal;
	uint32_t recRealPqVal;
}OSA24G_REC_INFO;

typedef struct _OSA24G_CONTEXT	//_OSA_24G_RADAR_PARAM
{	MODBUS_INSTANCE			*modbusInst;
	SENSOR_PROCESS_CONTEXT	processCtx;
	uint32_t				maxDistance;
	uint32_t				minDistance;
}OSA24G_CONTEXT;	//OSA_24G_RADAR_PARAM;

typedef struct _OSA24G_INSTANCE
{	void				*osa24gCfgPtr;
	OSA24G_CONTEXT		*osa24gCtx;
	SENSOR_HW_PQ_STRUCT	*hwPqInf;
	void				*devInst;
}OSA24G_INSTANCE;

typedef struct _COMPOSITE_SENSOR_CONTEXT
{	MODBUS_INSTANCE			*modbusInst;
	SENSOR_PROCESS_CONTEXT	processCtx;
}COMPOSITE_SENSOR_CONTEXT;

typedef struct _COMPOSITE_SENSOR_INSTANCE
{	uint8_t						isAiDev;
	uint8_t						isRadarDev;
	void						*compositeCfgPtr;
	COMPOSITE_SENSOR_CONTEXT	*compositeCtx;
	SENSOR_HW_PQ_STRUCT			*hwPqInf;
	void						*devInst;
}COMPOSITE_SENSOR_INSTANCE;

typedef struct _COMPOSITE_SENSOR_CTRL_STRUCT	//_COMPOSITE_SENSOR_INFO_STRUCT
{	COMPOSITE_SENSOR_INSTANCE	*radarInst;
	COMPOSITE_SENSOR_INSTANCE	*wlsInst;
	int isAiRadar;	//for siemens
	int compSensorIsReady;
	int compSensorIsPause;
	uint32_t compSensorPollingTimer;
	int alertCheckPqIdx;
	int	enNextPowerCycle;
	int alertRecTimeSlot;
	int alertRecTimeSlotOld;
	int enCheckPsm;
	int psmChkSolt;
	int psmChkSoltOld;
	SENS_FP	radarVal[2];
	SENS_FP	wlsVal[2];
	int sensorPqIsReady;
	int pqIsReady;
	IKW_FP	linearEquationX;
	IKW_FP	linearEquationY;
}COMPOSITE_SENSOR_CTRL_STRUCT;	//COMPOSITE_SENSOR_INFO_STRUCT;

typedef struct _VW_SENSOR_CONTEXT
{	MODBUS_INSTANCE	*modbusInst;
	SENSOR_PROCESS_CONTEXT	processCtx;
}VW_SENSOR_CONTEXT;

typedef struct _VW_SENSOR_INSTANCE
{	void				*devInst;
	void 				*vwCfgPtr;
	VW_SENSOR_CONTEXT	*vwCtx;
	SENSOR_HW_PQ_STRUCT	*hwPqInf;
}VW_SENSOR_INSTANCE;

typedef struct _NORMAL_SENSOR_CONTEXT
{	union
	{	MODBUS_INSTANCE			*modbusInst;
		DCON_INSTANCE			*dconInst;
	};
	SENSOR_PROCESS_CONTEXT	processCtx;
}NORMAL_SENSOR_CONTEXT;

typedef struct _NORMAL_SENSOR_INSTANCE
{	void					*devInst;
	NORMAL_SENSOR_CONTEXT	*ctx;
	SENSOR_HW_PQ_STRUCT		*hwPqInf;
}NORMAL_SENSOR_INSTANCE;

/*
*  RADAR
*/
typedef struct _RADAR_PROCESS_CONTEXT
{	enum RADAR_FSM	fsm;
	enum RADAR_FSM	nextFsm;
	char 			*recvBuf;
	int				isAvds;
	int				recvLen;
	uint32_t		pwrOnDelayTimer;
	uint32_t		pwrOnDelayTimeout;
	uint32_t		pwrOffDelayTimer;
	uint32_t		pwrOffDelayTimeout;
	uint32_t		cmdNoResponseTimer;
	uint32_t		cmdNoResponseTimeout;
	void			*upperCtx;
	uint8_t			forceChangeFsm;
	uint32_t		lowerZoomInTimer;
	uint32_t		lowerZoomInTimeout;
}RADAR_PROCESS_CONTEXT;

typedef struct _DUAL_RADAR_REC_STRUCT
{	uint16_t	tag;
	int 		verticalMeasureDist;
	int 		tiltMeasureDist;
	int			baseIsVertical;
}DUAL_RADAR_REC_STRUCT;

typedef struct _AVDS_RADAR_CONTEXT
{	RADAR_PROCESS_CONTEXT	processCtx;
	
	uint8_t		activeIdx;
	uint8_t		useHeatMapCmd;
	uint8_t		swapRadar;
	uint8_t		dualRadarRecBufIsValid;
	uint8_t		checkZoomIn;
	uint8_t 	lowerZoomInTimeoutFlag;
	int 		dropValue;
	int			radarUnit;
	int  		valueInterval;
	int			startRangeBin;
	int			endRangeBin;
	int			tiltAngle;
	int			tiltAngleCalibration;
	int			tiltStartRange;
	int			tiltEndRange;
	int 		distanceOfTwoRadar;
	int 		verticalRadarMeasureDistance;
	int			waterLevel;	//is for single tilt use, it mean vertical radar measure distance
	int			currRangeStart;
	int			currRangeEnd;
	int			currAngleStart;
	int			currAngleEnd;
	DUAL_RADAR_REC_STRUCT	*dualRadarRecStruct;
}AVDS_RADAR_CONTEXT;

typedef struct _AVDS_RADAR_INSTANCE
{	void				*devInst;
	void				*avdsCfgPtr;
	AVDS_RADAR_CONTEXT	*avdsCtx;
	SENSOR_HW_PQ_STRUCT	*hwPqInf;
}AVDS_RADAR_INSTANCE;

typedef struct _AR77_RADAR_CONTEXT
{	RADAR_PROCESS_CONTEXT	processCtx;
	uint8_t 				checkDistance;
	uint8_t					changeResolution;
}AR77_RADAR_CONTEXT;

typedef struct _AR77_RADAR_INSTANCE
{	void				*devInst;
	void				*ar77CfgPtr;
	AR77_RADAR_CONTEXT	*ar77Ctx;
	SENSOR_HW_PQ_STRUCT	*hwPqInf;
}AR77_RADAR_INSTANCE;

typedef struct _SW_PQ_MAPPING
{	struct _SW_PQ_MAPPING	*next;
	int						swPqIdx;
	int						hwPqMeasureIdx;
	void					*pqData;
}SW_PQ_MAPPING;

typedef struct _DEV_INSTANCE
{	struct _DEV_INSTANCE	*next;
	int						extDevNo;
	int						interface;
	//enum EXT_DEV_PROTOCOL	protocol;
	enum EXT_IF_PROTOCOL	protocol;
	enum EXT_DEV_TYPE		extDevType;
	int						instIsValid;
	void					*extDevInstPtr;	//NORMAL_SENSOR_INSTANCE, VW_SENSOR_INSTANCE, COMPOSITE_SENSOR_INSTANCE, OSA24G_INSTANCE, AVDS_RADAR_INSTANCE
	SW_PQ_MAPPING			*swPqMapping;
}DEV_INSTANCE;

typedef struct _AI_CONTEXT
{	int64_t pollTimer;
	int64_t pollTimeout;
}AI_CONTEXT;

typedef struct _AI_INSTANCE
{	void		*aiCfg;	//from syscfg
	AI_CONTEXT	aiCtx;
}AI_INSTANCE;

typedef struct _TEMPERATURE_CONTEXT
{	int64_t pollTimer;
	int64_t pollTimeout;
}TEMPERATURE_CONTEXT;

typedef struct _INTERNAL_SENSOR_INSTANCE
{	//solar volt
	//bat Volt
	//bat charge Current
	//temperature
	TEMPERATURE_CONTEXT temperatureCtx;
}INTERNAL_SENSOR_INSTANCE;

typedef struct _VI_SENSE_CONTEXT
{	uint32_t	*value;
	//float		sum;
	float		avg;
	uint32_t	count;
	uint8_t		valueIsReady;
	uint8_t		pqIsReady;
	void		*valFilter;
	void		*pqFilter;
}VI_SENSE_CONTEXT;

typedef struct _VI_SENSE_INSTANCE
{	VI_SENSE_CONTEXT 	batV;
	VI_SENSE_CONTEXT 	solarV;
	VI_SENSE_CONTEXT 	solarI;
	VI_SENSE_CONTEXT 	loadI;
	uint8_t				isReady;
	uint8_t				lowBatCount;
	uint8_t				getOnceFlag;
	uint8_t				readyFlag;
}VI_SENSE_INSTANCE;

typedef struct _DISPLAY_PANEL_CONTEXT
{	uint32_t	timer;
	uint32_t	timeout;
	uint16_t	crc16;
	uint16_t	pqVal;
	char		buf[8];
}DISPLAY_PANEL_CONTEXT;

typedef struct _DISPLAY_PANEL_INSTANCE
{	void				 *extDevCfg;
	DISPLAY_PANEL_CONTEXT ctx;
}DISPLAY_PANEL_INSTANCE;

typedef struct _SENS_DEV_STRUCT
{	DEV_INSTANCE	*modbusDevInsts;
	DEV_INSTANCE	*dconDevInsts;
	DEV_INSTANCE	*radarDevInst;
	VI_SENSE_INSTANCE voltCurrSenseInst;	
	
	UART_CONFIG	*uartCfgs[UART_MAX_ID];
	UART_CONFIG	*hsComm;
	UART_CONFIG	*lsComm;
	//UART_CONFIG	*rs485_1_Comm;
	//UART_CONFIG	*rs485_2_Comm;
	UART_CONFIG	*rs485_Comm[2];
	UART_CONFIG	*rs232_Comm;
	
	IO_PWR_CTL	iotVbatt;
	IO_PWR_CTL	rs485PwrCtrl[2];
	DI_INSTANCE	diInst;
	DO_INSTANCE	doInst;
	AI_INSTANCE aiInst;
	INTERNAL_SENSOR_INSTANCE intSensorInst;
	DISPLAY_PANEL_INSTANCE *displayPanelInst;
	//CAMERA_DEV	cameraDev;
	void	*uvcCamInst;	//UVC_CMERA_INSTANCE
	
	MODBUS_INSTANCE	*modbusServerInst;
	void *modbusMapping;
	//IMAGE_UPLOAD_INSTANCE *imgUploadInst;
}SENS_DEV_STRUCT;






#endif
