#ifndef __RADAR_SENSOR_H__
#define __RADAR_SENSOR_H__

#include "sensminiCfg.h"


#define RADAR_RX_FRAME_START_TAG			"\x88\x77\x66\x55\x44\x33\x22\x20"
#define RADAR_RX_FRAME_END_TAG				"\x55\xAA"
#define RADAR_RX_PROFILE_FRAME_START_TAG	"\x88\x77\x66\x55\x44\x33\x22\x21"
#define RADAR_RX_FAST_SCAN_FRAME_START_TAG	"\x88\x77\x66\x55\x44\x33\x22\x26"

#define RADAR_DISTANCE			0
#define RADAR_SNR				1
#define RADAR_POWER				2
#define RADAR_ANGLE				3
#define RADAR_STATUS			4
#define RADAR_RESOLUTION		5

#define RADAR_TILT_DISTANCE		0
#define RADAR_TILT_SNR			1
#define RADAR_TILT_POWER		2
#define RADAR_TILT_ANGLE		3
#define RADAR_TILT_STATUS		4
#define RADAR_TILT_RESOLUTION	5

#define RADAR_PARAM_AZIMUTH_START	0
#define RADAR_PARAM_AZIMUTH_END		1
#define RADAR_PARAM_RANGE_START		2
#define RADAR_PARAM_RANGE_END		3
#define RADAR_PARAM_DIF				4
#define RADAR_PARAM_PWR_SAVING		5
#define RADAR_PARAM_SNR_VALID_VAL	6

#define RADAR_TILT_MAX_BUF_SIZE			20
#define AR77_RADAR_TILT_MAX_BUF_SIZE	20

#define RADAR_MAX_PARAM					6
#define RADAR_TILT_MAX_PARAM			6

#define AVDS_MODE_SINGLE_VERTICAL	0x00
#define AVDS_MODE_DUAL				0x01
#define AVDS_MODE_SINGLE_TILT		0x02

#define AVDS_UNIT_CM				0x01
#define AVDS_UNIT_MM				0x00

#define RSP_BUF_SIZE_RADAR			4096

SENS_PACK_IAR typedef struct _RADAR_NORMAL_FRAME
{	unsigned char 	frameStart[8];	//0
	unsigned int 	packetLength;	//8
	unsigned short	NumOfRangeBin;	//12
	unsigned short	rangeStart;		//14
	unsigned short	rangeEnd;		//16
	short			azimuthStart;	//18
	short			azimuthEnd;		//20
	unsigned short	reserved0;		//22
	float			rangeResolution;//24
	unsigned short	peakIndex;		//28
	unsigned short	zoomIndex;		//30
	short			angleIndex;		//32
	unsigned short	reserved1;		//34
	float			snr;			//36
	float			fineRange;		//40
	float			power;			//44
	unsigned short	reserved2;
	unsigned char	frameEnd[2];	
}SENS_PACK_GCC RADAR_NORMAL_FRAME;

SENS_PACK_IAR typedef struct _RADAR_FAST_SCAN_FRAME
{	unsigned char 	frameStart[8];
	unsigned int 	packetLength;
	unsigned short	NumOfRangeBin;
	unsigned short	rangeStart;
	unsigned short	rangeEnd;
	short			azimuthStart;
	short			azimuthEnd;
	unsigned short	reserved0;
	float			rangeResolution;
	unsigned short	peakIndex;
	unsigned short	zoomIndex;
	short			angleIndex;
	unsigned short	reserved1;
	float			snr;
	float			fineRange;
	float			focusRangePower;
	//char 			emptyData[120];
	//float			focusRangePower[121];
	//unsigned short	heatMap;
	//unsigned char		frameEnd[2];
}SENS_PACK_GCC RADAR_FAST_SCAN_FRAME;

extern void radarOperation(void);
extern void avdsCalibration(void);

#endif
