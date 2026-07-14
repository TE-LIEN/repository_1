#ifndef __CAMERA_UVC_SENSOR_H__
#define __CAMERA_UVC_SENSOR_H__

#include "sensminiCfg.h"
#include "usbh_lib.h"
#include "usbh_uvc.h"
#include "usbh_cdc.h"
#include "hub.h"

#define SELECT_RES_WIDTH     2592	//2592 //1920	//2592	//1280
#define SELECT_RES_HEIGHT    1944	//1944 //1080	//1944	//720

//#define IMAGE_MAX_SIZE       (SELECT_RES_WIDTH*SELECT_RES_HEIGHT*2)
#define IMAGE_MAX_SIZE       1048576	//1572864 //(1*1048576)


#define IMAGE_BUFF_CNT       2

#define IGNORE_IMG_FRAME_CNT	5

#define UVC_STREAM_ERR_OK			0
#define UVC_STREAM_ERR_RESOLUTION	-1
#define UVC_STREAM_ERR_STREAM		-2

#define USB_UVC_INIT_TIMEOUT	5000


enum CAMERA_RESOLUTION_ENUM
{	CAMERA_RESOLUTION_VGA,
	CAMERA_RESOLUTION_QVGA,
	CAMERA_RESOLUTION_HD,
	CAMERA_RESOLUTION_FHD,
	CAMERA_RESOLUTION_300M,
	CAMERA_RESOLUTION_500M,
	/*CAMERA_RESOLUTION_640_480,
	CAMERA_RESOLUTION_320_240,
	CAMERA_RESOLUTION_160_120,
	CAMERA_RESOLUTION_800_600,
	CAMERA_RESOLUTION_1024_768,
	CAMERA_RESOLUTION_1280_720,
	CAMERA_RESOLUTION_1280_960,
	CAMERA_RESOLUTION_1920_1080,
	CAMERA_RESOLUTION_2048_1536,
	CAMERA_RESOLUTION_2592_1944,
	CAMERA_RESOLUTION_1600_1200,
	CAMERA_RESOLUTION_2304_1296,
	CAMERA_RESOLUTION_2560_1440,
	CAMERA_RESOLUTION_2560_1920,
	CAMERA_RESOLUTION_3200_1800,
	CAMERA_RESOLUTION_3520_1980,
	CAMERA_RESOLUTION_3840_2160,*/
	CAMERA_RESOLUTION_MAX
};


enum
{	IMAGE_BUFF_FREE,
    IMAGE_BUFF_USB,
    IMAGE_BUFF_READY,
    IMAGE_BUFF_POST
};

struct ig_buff_t
{	uint8_t   *buff;
    int       len;
    int       state;
};

/*typedef struct _UVC_CAM_UPLOAD_CONTEXT
{	IMAGE_UPLOAD_REC_CONTEXT 	*imageUploadRec;
	char						*imgUploadInfBuf;
}UVC_CAM_UPLOAD_CONTEXT;*/

typedef struct _UVC_CAMERA_CONTEXT
{	uint32_t	streamRes;
	uint32_t	streamWidth;
	uint32_t	streamHeight;
	uint8_t		*imgBufPool[IMAGE_BUFF_CNT];	//alloc 1M bytes
	uint8_t		*snapshotBufPool;
	uint32_t	streamImgSize;
	uint32_t	stillWidth;
	uint32_t	stillHeight;
	uint32_t	shapshotLength;
	uint8_t		skipStreamImgCount;
	uint8_t		doSnapshot;
	uint8_t		imgReady;
	uint8_t		captureFailCount;
	uint8_t		initTimeoutCnt;
	uint8_t		uvcActive;
	//uint8_t		snapshotDone;
	//UVC_CAM_UPLOAD_CONTEXT	*camUploadCtx;
}UVC_CAMERA_CONTEXT;

typedef struct _UVC_CAM_FMT_INF
{	struct _UVC_CAM_FMT_INF *next;
	IMAGE_FORMAT_E	format;
	uint32_t		width;
	uint32_t		height;
}UVC_CAM_FMT_INF;

typedef struct _UVC_CAMERA_INSTANCE
{	void				*cfg;
	UVC_CAMERA_CONTEXT	*ctx;	
	UVC_CAM_FMT_INF		*fmtInfos;
	char				*saveFileName;
	uint64_t 			imgUnixTime;
}UVC_CMERA_INSTANCE;

extern volatile UVC_CMERA_INSTANCE *uvcCamInst;

extern void uvcCameraPowerOn(void);
extern int uvcCameraStart(UVC_DEV_T *vdev);
extern int uvcCameraWaitImgReady(UVC_DEV_T *vdev);
extern void uvcCameraStop(UVC_DEV_T *vdev);
extern void initUvcCamDevice(void);
extern void saveImgFile(void);
extern void uvcCameraPowerOff(void);


#endif

