#ifndef __PARAM_H__
#define __PARAM_H__

#include "sensminiCfg.h"


extern int getSysParameter(void);
extern int UpdateAndSaveParam(char *param);

#define PARAM_SETTING_BASIC_PARAM_VAL_1		1
#define PARAM_SETTING_DEVICE_SETTING_VAL_2	2
#define PARAM_SETTING_PQ_VAL_3				3
#define PARAM_SETTING_DO_VAL_4				4
#define PARAM_SETTING_FORMULA_1_VAL_7		7
#define PARAM_SETTING_PASSWORD_VAL_8		8
#define PARAM_SETTING_FORMULA_2_VAL_9		9
#define PARAM_SETTING_DO_MAP_VAL_10			10

#define VERSION_STR						"VERSION"

#define AUTOSEND_SECOND_str              "AUTOSEND_SECOND"

// SD Card Parameter ===========================================================
#define RECORD_SECOND_str 				"RECORD_SECOND"                                       // need to think if RECORD_SECOND_str > SYSTEM_TIMEOUT_TICK, some errors maybe happen

// System Parameter ============================================================
#define LOG_TO_SDCARD_str         "LOG_TO_SDCARD"
#define LOG_FILE_NAME_str         "LOG_FILE_NAME"
#define TRANSFER_PROTOCOL_str     "TRANSFER_PROTOCOL"
#define TRANSFER_PROTOCOL1_str    "TRANSFER_PROTOCOL1"

// SensMini_M4 Parameter =======================================================
#define DEVICE_ONBOARD_str               "onboard_ai"
#define COM_PROTOCOL_str                 "com_protocol"

// Formula Parameter ===========================================================
#define FORMULA_NUMBER_str  "FORMULA_NUMBER"
#define FORMULA_COEF_str   	"FORMULA_COEF"
#define FORMULA_COEF0_str   "FORMULA_COEF0"
#define FORMULA_COEF1_str   "FORMULA_COEF1"
#define FORMULA_COEF2_str   "FORMULA_COEF2"
#define FORMULA_COEF3_str   "FORMULA_COEF3"
#define FORMULA_COEF4_str   "FORMULA_COEF4"
#define FORMULA_COEF5_str   "FORMULA_COEF5"
#define FORMULA_COEF6_str   "FORMULA_COEF6"
#define FORMULA_COEF7_str   "FORMULA_COEF7"
#define FORMULA_COEF8_str   "FORMULA_COEF8"
#define FORMULA_COEF9_str   "FORMULA_COEF9"
#define FORMULA_COEF10_str  "FORMULA_COEF10"
#define FORMULA_COEF11_str  "FORMULA_COEF11"
#define FORMULA_COEF12_str  "FORMULA_COEF12"
#define CONDITION_DEFIM_str "CONDITION_DEFI_M"
#define CONDITION_DEFI_str 	"CONDITION_DEFI_"
#define CONDITION_DEFI0_str "CONDITION_DEFI_0"
#define CONDITION_DEFI1_str "CONDITION_DEFI_1"
#define CONDITION_DEFI2_str "CONDITION_DEFI_2"
#define CONDITION_DEFI3_str "CONDITION_DEFI_3"
#define CONDITION_DEFI4_str "CONDITION_DEFI_4"
#define CONDITION_DEFI5_str "CONDITION_DEFI_5"
#define CONDITION_DEFI6_str "CONDITION_DEFI_6"
#define CONDITION_DEFI7_str "CONDITION_DEFI_7"
#define CONDITION_DEFI8_str "CONDITION_DEFI_8"
#define CONDITION_DEFI9_str "CONDITION_DEFI_9"

#define ID_Client_str         "GROUP_SERIAL_ID"
#define SENSMODE_str          "SENSMODE"
#define SENSLINK_ADDRESS_str  "SENSLINK_ADDRESS"
#define SENSLINK_PORT_str     "SENSLINK_PORT"
#define SENSLINK_ADDRESS2_str "SENSLINK_ADDRESS2"
#define SENSLINK_PORT2_str    "SENSLINK_PORT2"
#define CUSTOMER_ADDRESS_str  "CUSTOMER_ADDRESS"
#define CUSTOMER_PORT_str     "CUSTOMER_PORT"

#define SERV1_TYPE_str					"SERVER_TYPE_0"
#define MQTT1_DEVICE_NAME_str		"SERV_MQTT_DEV_NAME_0"
#define MQTT1_USERNAME_str			"SERV_MQTT_USERNAME_0"
#define MQTT1_PASSWORD_str			"SERV_MQTT_PASSWORD_0"
#ifdef SECURITY_TEST
#define NEW_MQTT1_PASSWORD_str	"NEW_SERV_MQTT_PASSWORD_0"
#define NEW1_MQTT1_PASSWORD_str	"SRV_MQTT_PWD_0"
#endif
#define MQTT1_PUB_TOPIC_str			"SERV_MQTT_PUB_TOPIC_0"
#define MQTT1_SUB_TOPIC_str			"SERV_MQTT_SUB_TOPIC_0"

#define SERV2_TYPE_str					"SERVER_TYPE_1"
#define MQTT2_DEVICE_NAME_str		"SERV_MQTT_DEV_NAME_1"
#define MQTT2_USERNAME_str			"SERV_MQTT_USERNAME_1"
#define MQTT2_PASSWORD_str			"SERV_MQTT_PASSWORD_1"
#ifdef SECURITY_TEST
#define NEW_MQTT2_PASSWORD_str	"NEW_SERV_MQTT_PASSWORD_1"
#define NEW1_MQTT2_PASSWORD_str	"SRV_MQTT_PWD_1"
#endif
#define MQTT2_PUB_TOPIC_str			"SERV_MQTT_PUB_TOPIC_1"
#define MQTT2_SUB_TOPIC_str			"SERV_MQTT_SUB_TOPIC_1"

#define SERV_TYPE_str						"SERVER_TYPE_"
#define MQTT_DEVICE_NAME_str		"SERV_MQTT_DEV_NAME_"
#define MQTT_USERNAME_str				"SERV_MQTT_USERNAME_"
#define MQTT_PASSWORD_str				"SERV_MQTT_PASSWORD_"
#ifdef SECURITY_TEST
#define NEW_MQTT_PASSWORD_str		"NEW_SERV_MQTT_PASSWORD_"
#define NEW1_MQTT_PASSWORD_str	"SRV_MQTT_PWD_"
#endif
#define MQTT_PUB_TOPIC_str			"SERV_MQTT_PUB_TOPIC_"
#define MQTT_SUB_TOPIC_str			"SERV_MQTT_SUB_TOPIC_"

#define SENSLINK_APN_str      "SENS_APN"
#define DNS_ADDRESS_str       "DNS_ADDRESS"
#define PRIORITY_CONNECT_str  "PRIORITY_CONNECTION"
#define TIME_ZONES_str        "TIME_ZONES"
#define MOBILE_BR_str         "MOBILE_BAUDRATE"
#define ALERT_THRE_str        "ALERT_THRESHOLD"
#define SENS_APN_NB_str       "SENS_APN_NB"
#if IS_M4_DEV
	#define SENS_PLMN_NB_str      "NB_PLMN"
#else
	#define SENS_PLMN_NB_str      "SENS_PLMN_NB"
#endif
#define SIM_PASSWORD_str      "SIM_PASSWORD"
#ifdef SECURITY_TEST
#define NEW_SIM_PASSWORD_str  "SIMCARD_PASSWORD"
#define NEW1_SIM_PASSWORD_str  "SIM_CARD_PWD"
#endif
#define SIM_ACCOUNT_str       "SIM_ACCOUNT"
#define SIM_AUTH_str          "SIM_AUTH"
#define SD_HISTORY_str        "SD_HISTORY"

// USER NAME ===================================================================
#define USERNAME_str         "USERNAME"
#define PASSWORD_str         "DEVICEDIG"
#ifdef SECURITY_TEST
#define NEW_PASSWORD_STR		 "PASSWORD"
#define NEW1_PASSWORD_STR		 "PWD"
#endif

// BASIC PARAMETER =============================================================
#define RS485_1_str                        "RS485_1"
#define RS485_2_str                        "RS485_2"
#define RS232_1_str                        "RS232_1"
#define SD_REC_SEC_str                     "SD_REC_SEC"
#define SLEEP_REC_TIME_str                 "SLEEP_REC_TIME"
#define GPRS_LOCAL_PORT_str                "GPRS_LOCAL_PORT"
#define AUTOSEND_SEC_str                   "AUTOSEND_SEC"
#define GPRS_LAN_str                       "GPRS_LAN"
#define MOBILE_INTERVAL_str                "MOBILE_INTERVAL"
#define DEVICE_INITIAL_WAIT_SEC_str        "DEVICE_INITIAL_WAIT_SEC"
#define DEVICE_DISCONNECT_SEC_str          "DEVICE_DISCONNECT_SEC"
#define PIC_POWERDOWN_str                  "PIC_POWERDOWN"
#define RS232_POWER_str                    "RS232_POWER"
#define ADC_POWER_str                      "ADC_POWER"
#define ADC_CFG_str							"ADC_CFG"
#define WAKE_UP_INTERVAL_str               "WAKE_UP_INTERVAL"
#define IP_ADDRESS_str                     "IP_ADDRESS"
#define IP_MASK_str                        "IP_MASK"
#define IP_GATEWAY_str                     "IP_GATEWAY"
#define LAN_LOCAL_PORT_str                 "LAN_LOCAL_PORT"
#define DI_STATUS_str                      "DI_STATUS"
#define BAT_ON_DIFF_str                    "BAT_ON_DIFF"
#define BAT_OFF_str                        "BAT_OFF"
#define DO_OUTPUT_POWER_str                "DO_OUTPUT_POWER"
#define DI_WAKEUP_str                      "DI_WAKEUP"
#define DI_WAKEUP_REC_INTERVAL_STR			"DI_WAKE_REC_INTERVAL"
#define DI_FUNCTION_str                     "DI_FUNCTION"	//CY PUMP
#define DI_FUNCTION_size                   	5
#define GPS_START_Time_str                  "GPS_Initial_Rec_Time"
#define GPS_POSITION_str                    "GPS_Initial_Rec_Point"
#define GPS_POS_RST_TIME_str                "GPS_Position_Reset_Time"
	#define GPS_POS_RST_TIME_SIZE			2

#define CAMERA_INTERVAL_STR					"CAMERA_INTERVAL"
#define PRESSURE_WATER_LEVEL_GAUGE_STR		"PRESSURE_WATER"
//#define _3_STAGE_WATER_LEVEL_SENSOR_STR		"WATER_3_STAGE"
#define RADAR_WATER_LEVEL_SENSOR_STR		"RADAR_WLS_STATUS"

#define NB_SIM_TYPE_STR											"NB_SIM_TYPE"
//#define SENS_PLMN_LTE_STR      							"SENS_PLMN_LTE"

#define SNTP_SERVER_STR											"SNTP_SERVER"

#define AGPS_ENABLE_STR											"AGPS"
#define AGPS_PARAMS_STR											"AGPS_PARAMS"

#define PROJECT_str													"PROJECT_NO"

#define GATE_CTRL_STR												"GATE_CTRL"
#define PUMP_MODE_STR												"PUMP_MODE"

#define NUM_WISUN_NODE_STR									"WISUN_NODES"
#define PQ_NODE_STR													"PQ_OF_NODE"

#define LORA_WAN_CH0_FREQ_STR								"LORA_WAN_FREQ"
#define LORA_WAN_DEV_EUI_STR								"LORA_WAN_DEVEUI"
#define LORA_WAN_APP_EUI_STR								"LORA_WAN_APPEUI"
#define LORA_WAN_APP_KEY_STR								"LORA_WAN_APPKEY"
#define LORA_WAN_DEV_ADDR_STR								"LORA_WAN_DEVADDR"

#define WISUN_NET_NAME_STR									"WISUN_NET_NAME"
#define WISUN_FREQ_STR										"WISUN_FREQ"
#define WISUN_ONBOARD_EN_STR								"ONBOARD_WISUN"

#define AR77_PARAM_STR											"AR77_PARAMS"
#define AR77_PARAM_SIZE											7
#define AVDS_PARAMS_STR											"AVDS_PARAMS"
#define AVDS_PARAMS_SIZE										10
#define OSA24_PARAMS_STR										"OSA24_PARAMS"
#define OSA24_PARAMS_SIZE										9

#define EMI_TEST_CTRL_STR									"EMI_TEST_ON_PARAMS_SAFETY"

#define CAMERA_PARAM_STR										"CAMERA_PARAMS"
#define CAMERA_PARAM_SIZE										4

#define AUDIO_PARAM_STR											"AUDIO_PARAMS"
#define AUDIO_PARAM_SIZE										4

#define COMPOSITE_OSA_PARAM_STR							"COMP_OSA_PARAMS"
#define COMPOSITE_OSA_PARAM_SIZE						9

#define COMPOSITE_SIEMENS_PARAM_STR					"COMP_SIEMENS_PARAMS"
#define COMPOSITE_SIEMENS_PARAM_SIZE				10

#define OTA_PARAM_STR												"OTA_PARAMS"
#define OTA_PARAM_SIZE											2	//ip and port

#define SEWER_INSTALL_MODE_STR							"SEWER_IM"

#define VW_PARAM_STR												"VW_PARAMS"
#define VW_PARAM_SIZE												5

#define EXT_LVD_STR													"EXT_LVD_PARAMS"
#define EXT_LVD_SIZE												2

#define SYSTEM_INSTALL_MODE_STR							"SYS_IM"

#define AUTO_FOTA_CHK_MODE_STR							"AUTO_FOTA"

#define IMG_SERV_TYPE_STR										"IMG_SVR"


#endif

