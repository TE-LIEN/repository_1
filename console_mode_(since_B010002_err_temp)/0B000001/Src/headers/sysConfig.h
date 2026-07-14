#ifndef __SYS_CONFIG_H__
#define __SYS_CONFIG_H__


#define DEBUG_MSG_BUF		300

#define	ROUNDUP(x, y)		((((x)+((y)-1))/(y))*(y))                       //find one number >= x, and x is multiple of y
#define	ROUNDDOWN(x, y)		(((x)/(y)) * (y))                               //find one number <= x, and x is multiple of y
#define MAX(a, b)       	(((a) > (b)) ? (a) : (b))
#define MIN(a, b)       	(((a) < (b)) ? (a) : (b))


#define RED					"\033[31m"
#define GREEN				"\033[32m"
#define YELLOW				"\033[33m"
#define BLUE				"\033[34m"
#define PURPLE				"\033[35m"
#define DARK_GREEN			"\033[36m"
#define ORG_COLOR			"\033[0m"
#define STROBE				"\033[5m"


#define EMPTY_CHAR			"\x00"


#define MD5_RESULT_LENGTH   16
#define MD5_RESULT_STR		"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
#define MD5_RESULT_PRINT(x)	x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], x[9], x[10], x[11], x[12], x[13], x[14], x[15]
#define MD5_RESULT_SCAN(x)	&x[0], &x[1], &x[2], &x[3], &x[4], &x[5], &x[6], &x[7], &x[8], &x[9], &x[10], &x[11], &x[12], &x[13], &x[14], &x[15]

#define SD_PROCESS_SUCCESS	0
#define SD_PROCESS_FAIL			-1
#define	SD_PROCESS_TIMEOUT	-2

#define SD_RW_LENGTH_ERROR	-5
#define SD_RW_OPEN_FILE_FAIL	-6

#define LW_EVENT_FILE_PROCESS_SUCCESS				0x01
#define LW_EVENT_FILE_OPEN_FAIL							0x02
#define LW_EVENT_FILE_READ_FAIL							0x04
#define LW_EVENT_FILE_WRITE_FAIL						0x08
#define LW_EVENT_FILE_DELETE_FAIL						0x10
#define LW_EVENT_FILE_CREATE_FILE_FAIL			0x20
#define LW_EVENT_FILE_PARSER_INI_FAIL				0x40
#define LW_EVENT_FILE_SAVE_INI_FAIL					0x80

#define LW_EVENT_WAIT_FLAG						0xFF

#define LW_EVENT_FILE_CHK_OPEN		LW_EVENT_FILE_OPEN_FAIL | LW_EVENT_FILE_PROCESS_SUCCESS
#define LW_EVENT_FILE_CHK_READ		LW_EVENT_FILE_CHK_OPEN | LW_EVENT_FILE_READ_FAIL
#define LW_EVENT_FILE_CHK_WRITE		LW_EVENT_FILE_CHK_OPEN | LW_EVENT_FILE_WRITE_FAIL


#define SD_FS_DEV		0x01
#define SPI_FS_DEV	0x02

/*
 * File system
 */
#define OVER_WRITE_MODE		0
#define CONTINUE_WRITE_MODE	1
#define WRITE_NAN_DATA_MODE	2

#define READ_LENGTH_MODE	0
#define NORMAL_READ_MODE	1
#define BIN_READ_MODE		2
#define BIN_READ_MODE2		3


#define IMG_FILE_SIZE	40


#define HS_PCIE_CH					0
#define LS_PCIE_CH					1
#define HS_PCIE_CH_BAK				2
#define LS_PCIE_CH_BAK				3
	#define PCIE_NO_MODULE			99
	#define PCIE_3G_LTE				0
	#define PCIE_3G_LTE_MODBUS		1
	#define PCIE_SARA_N200			2
	#define PCIE_3G_PURE_DATA		3
	//#define PCIE_LS_ROLA_AS923	3
	#define PCIE_SARA_R410			4
	#define PCIE_GPS_L76			5
	#define PCIE_TELIT_ME310G		6
	#define PCIE_TELIT_LE910C1		7
	#define PCIE_WI_SUN				8
	#define PCIE_LORA_WAN			9
	#define PCIE_LORA_P2P			10
	#define PCIE_LTE_USB			11


#define ALGORITHM_NONE					0x00
#define ALGORITHM_MOVING				0x01
#define ALGORITHM_STD					0x02
#define ALGORITHM_KALMAN				0x04
#define ALGORITHM_UNIQUE				0x80

#define FROM_NONE		0
#define FROM_SD_CARD	1
#define FROM_SPI_NOR	2

#define INIFILE 			"Param.ini"
#define INI_BAK_FILE		"ParamBak.ini"
	#define COMPLETE_KEY	"COMPLETE"

#define JSON_PARAM_FILE		"param.json"
#define IKW_PARAM_FILE		"ikwParam.json"
#define IKW_PARAM_FILE_BAK	"ikwParamBak.json"
#define AUTH_JSON_FILE		"auth.json"

#define SECTION 				"SensMiniA4Neo"

#define RED						"\033[31m"
#define GREEN					"\033[32m"
#define YELLOW					"\033[33m"
#define BLUE					"\033[34m"
#define ORG_COLOR				"\033[0m"


#define SET_PSM_STATUS		1
#define CLR_PSM_STATUS		0

#define DISABLE_PSM_BMP_TEMPORARY_DIS_PSM		0	//when dis psm use web button, turn off psm, ok
#define DISABLE_PSM_BMP_PSM_DISABLE				1	//cfg is turn off psm, ok
#define DISABLE_PSM_BMP_WORKING_TIMER			2	//when network active, must work over 65 sec, ok
#define DISABLE_PSM_BMP_CAMERA_ACTIVE			3	//when camera active, turn off psm, ok
#define DISABLE_PSM_BMP_OTA_ACTIVE				4	//when ota active, turn off psm
#define DISABLE_PSM_BMP_IMAGE_UPLOAD			5	//when image uploading, turn off psm, ok
#define DISABLE_PSM_BMP_SRV1_RECEIVE_PROCESSING	6	//receive socket data, turn off psm, when send done, turn on
#define DISABLE_PSM_BMP_SRV2_RECEIVE_PROCESSING	7	//receive socket data, turn off psm, when send done, turn on
#define DISABLE_PSM_BMP_SRV1_DATA_SYNC			8	//auto data sync, turn off psm
#define DISABLE_PSM_BMP_SRV2_DATA_SYNC			9	//auto data sync, turn off psm
#define DISABLE_PSM_BMP_ALERT_FINAL_SEND_IMG	10	//alert send data turn off psm
#define DISABLE_PSM_BMP_DI_WAKEUP				11	//di wakeup, turn off psm, when di release, turn on
#define DISABLE_PSM_BMP_REC_TIME_LESS_3MIN		12	//rec interval less 3 min, turn off psm, ok
#define DISABLE_PSM_BMP_INSTALL_MODE			13	//when install mode, turn off psm
#define DISABLE_PSM_BMP_RECORD_DATA				14	//before save history data, turn off psm, ok
#define DISABLE_PSM_BMP_YL_PUMP_ATTENDANCE		15	//YL pump mode, attendance, turn off psm
#define DISABLE_PSM_BMP_YL_PUMP_VIBRATION		16	//YL pump mode, vibration active, turn off psm
#define DISABLE_PSM_BMP_CY_PUMP_ATTENDANCE		17	//CY pump mode, attendance, turn off psm
#define DISABLE_PSM_BMP_SRV1_DATA_SEND			18	//server 1 PQ not send done, turn off psm
#define DISABLE_PSM_BMP_SRV2_DATA_SEND			19	//server 2 PQ not send done, turn off psm
#define DISABLE_PSM_BMP_ALERT_ACTIVE			20	//alert avtive, turn off psm
#define DISABLE_PSM_BMP_DI4_ACTIVE				21	//maintenance port insert
#define DISABLE_PSM_BMP_DI5_ACTIVE				22	//
#define DISABLE_PSM_BMP_ALERT_SEND_SRV1_DATA	23
#define DISABLE_PSM_BMP_ALERT_SEND_SRV2_DATA	24
//#define DISABLE_PSM_BMP_DI_RELEASE_SEND_SRV1_DATA	25
//#define DISABLE_PSM_BMP_DI_RELEASE_SEND_SRV2_DATA	26

#define SET_BMP(x, y)			x |=  ((uint32_t)1 << y)
#define CLR_BMP(x, y)			x &= ~((uint32_t)1 << y)
#define CHK_BMP(x, y)			(x & (uint32_t)1 << y)

#define PSM_CHK_BMP_TEMPORARY_DISABLE_BMP				0	//temporaryDis
#define PSM_CHK_BMP_PSM_DISABLE							1
#define PSM_CHK_BMP_WORKING_TIMER						2
#define PSM_CHK_BMP_CAMERA_ACTIVE						3
#define PSM_CHK_BMP_OTA_ACTIVE							4
#define PSM_CHK_BMP_IMAGE_UPLOAD						5
#define PSM_CHK_BMP_SRV1_RECEIVE_PROCESSING				6
#define PSM_CHK_BMP_SRV2_RECEIVE_PROCESSING				7
#define PSM_CHK_BMP_SRV1_DATA_SYNCING					8
#define PSM_CHK_BMP_SRV2_DATA_SYNCING					9
#define PSM_CHK_BMP_ALERT_FINAL_SEND					10
#define PSM_CHK_BMP_SRV_ACTIVE_CONN_NOT_MATCH			11
#define PSM_CHK_BMP_SRV_ACTIVE_SEND_NOT_MATCH			12
#define PSM_CHK_BMP_GPS_NOT_READY						13
#define PSM_CHK_BMP_YS_SRV_SEND_DONE					14
#define PSM_CHK_BMP_YS_SRV_TIMER						15
#define PSM_CHK_BMP_ALERT_FINAL_SEND_IMG				16


#define BACKUP_PARAM_COMPARE_SUCCESS	0
#define BACKUP_PARAM_COMPARE_NO_SRC		-1
#define BACKUP_PARAM_COMPARE_NO_DEST	-2
#define BACKUP_PARAM_COMPARE_ERROR		-3

#if EN_WATCHDOG
#define WATCHDOG_TIMEOUT_TICK (30000)
#else
#define WATCHDOG_TIMEOUT_TICK (1800000)
#endif



#define PQ_REC_CONDITION_NONE		0
#define PQ_REC_CONDITION_IM			1
#define PQ_REC_CONDITION_REC_TIME	2
#define PQ_REC_CONDITION_ALARM_TIME	3
#define PQ_REC_CONDITION_DI_TIME	4

#endif 
