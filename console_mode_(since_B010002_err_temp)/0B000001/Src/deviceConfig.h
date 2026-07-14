#ifndef __DEVICE_CONFIG_H__
#define __DEVICE_CONFIG_H__

#define CRC16_SIZE      2
#define TIMESPAN_SIZE   2
#define HEADER_SIZE     8

#define ON_BOARD_DEV	0

//#define DIQUANTITY		4
#define DIQUANTITY		6	//total 6, iso count 4, 
#define DOQUANTITY		4	//sink2 same with source 2, sink3 same with source 3
#define DO_WIRED_QUANTITY	2

#define RS485_RECEIVE_MODE			0
#define RS485_TRANSMIT_MODE			1

#define RS485_CHANNEL_1				0
#define RS485_CHANNEL_2				1


#define MODBUS_COM1				1
#define DCON_COM1				2
#define TOUCH_PAD				3
#define RADAR_COM1				4

#define MODBUS_COM2				5
#define DCON_COM2				6

#define RADAR_COM2				8
#define GPS_COM3				9
#define DISPLAY_PANEL_COM2		10
#define DCON_COM3				11
#define MODBUS_COM3				12
#define MAXSONAR_SENSOR_COM3	13
#define EUREKA2_COM3			14
#define SDI_12_COM1				15
#define SDI_12_COM2				16

#define CAMERA_COM1				17
#define CAMERA_COM2				18

#define RADAR_77AG_COM1			20
#define RADAR_77AG_COM2			21

#define G_SENSOR_COM1			23
#define G_SENSOR_COM2			24

#define OSA_24G_RADAR_COM1		25
#define OSA_24G_RADAR_COM2		26

#define AUDIO_COM1				27
#define AUDIO_COM2				28

#define CONSOLE_COM3			30

#define ETH_MODBUS_TCP			90

#define NONE_ETH				96
#define NONE_COM1				97
#define NONE_COM2				98
#define NONE_COM3				99


#if defined SPECIAL_MODBUS_SENSOR_FILTER_TYPE_AVG
	#define SPECIALL_MODBUS_SENSOR_FILTER_MAX_COUNT	5
#elif defined SPECIAL_MODBUS_SENSOR_FILTER_TYPE_STD
	#define SPECIALL_MODBUS_SENSOR_FILTER_MAX_COUNT	30
#else 
	#define SPECIALL_MODBUS_SENSOR_FILTER_MAX_COUNT	1
#endif

#define MAX_REC_IMG_COUNT		100

#define PSM_METHOD_WAKEUP_TIMER	1
#define PSM_METHOD_MCU_DPD		2

#define DI_KEEP_ON_STS_KEEP_ON	0
#define DI_KEEP_ON_STS_RELEASE	-1

#define DI_RELEASE_FLAG_REC		0x01
#define DI_RELEASE_FLAG_SEND	0x02
#define DI_RELEASE_FLAG_ALL		0x04

#endif