#include "sensors/sensorApp.h"
#include "sensDev.h"
#include "sensSystem.h"
#include "sensCommon.h"
#include "network/netApp.h"
#include "rtcDateTime.h"
#include "sdCardTask.h"

struct ig_buff_t  _ig[IMAGE_BUFF_CNT];

//uint8_t  image_buff_pool[IMAGE_BUFF_CNT][IMAGE_MAX_SIZE] __attribute__((aligned(32)));

/*uint8_t *imgBufPool[IMAGE_BUFF_CNT];
uint8_t *snapshotBufPool;
*/
int   _idx_usb = 0, _idx_post = 0;
volatile int gWriteJpg2File = 0;
//volatile int gSnapshotLen = 0;
//volatile int gIgnoreImgFrameCnt=0;
extern int gSkipFrame;

extern int   _total_frame_count;

volatile UVC_CMERA_INSTANCE *uvcCamInst = NULL;
//------------------------------------------------------------------------------
void initImgBuffers(void)
{	UVC_CAMERA_CONTEXT *ctx = (UVC_CAMERA_CONTEXT *)uvcCamInst->ctx;
    int   i;
	for(i=0;i<IMAGE_BUFF_CNT;i++)
	{	if(ctx->imgBufPool[i] == NULL)
			ctx->imgBufPool[i] = SENS_MEM_ALLOC(IMAGE_MAX_SIZE);
	}
	if(ctx->snapshotBufPool == NULL)
		ctx->snapshotBufPool = SENS_MEM_ALLOC(IMAGE_MAX_SIZE);
	
    for (i = 0; i < IMAGE_BUFF_CNT; i++)
    {
        _ig[i].buff   = (uint8_t *)ctx->imgBufPool[i];
        _ig[i].len    = 0;
        _ig[i].state  = IMAGE_BUFF_FREE;
    }
    _idx_usb = 0;
    _idx_post = 0;
}
//------------------------------------------------------------------------------
void deinitImageBuf(void)
{	int   i;
	UVC_CAMERA_CONTEXT *ctx = (UVC_CAMERA_CONTEXT *)uvcCamInst->ctx;
	
	for(i=0;i<IMAGE_BUFF_CNT;i++)
	{	if(ctx->imgBufPool[i])
		{	SENS_MEM_FREE(ctx->imgBufPool[i]);
			ctx->imgBufPool[i] = NULL;
		}
	}
#if 0	//remove, when save file done release
	if(ctx->snapshotBufPool)
	{	SENS_MEM_FREE(ctx->snapshotBufPool);
		ctx->snapshotBufPool = NULL;
	}
#endif
	
    for (i = 0; i < IMAGE_BUFF_CNT; i++)
    {	_ig[i].buff   = NULL;
        _ig[i].len    = 0;
        _ig[i].state  = IMAGE_BUFF_FREE;
    }
    _idx_usb = 0;
    _idx_post = 0;
}
//------------------------------------------------------------------------------
int  uvc_rx_callbak(UVC_DEV_T *vdev, uint8_t *data, int len)
{	int  next_idx;

	_total_frame_count++;

	next_idx = (_idx_usb+1) % IMAGE_BUFF_CNT;

    if(_ig[next_idx].state != IMAGE_BUFF_FREE)
    {
        /*
         *  Next image buffer is in used.
         *  Just drop this newly received image and reuse the same image buffer.
         */
        usbh_uvc_set_video_buffer(vdev, _ig[_idx_usb].buff, IMAGE_MAX_SIZE);
    }
    else
    {	_ig[_idx_usb].state = IMAGE_BUFF_READY;   /* mark the current buffer as ready for decode/display */
        _ig[_idx_usb].len   = len;                /* length of this newly received image   */

        /* proceed to the next image buffer */
        _idx_usb = next_idx;
        _ig[_idx_usb].state = IMAGE_BUFF_USB;     /* mark the next image as used by USB    */

        /* assign the next image buffer to receive next image from USB */
        usbh_uvc_set_video_buffer(vdev, _ig[_idx_usb].buff, IMAGE_MAX_SIZE);
    }
    return 0;
}
//------------------------------------------------------------------------------
void addUvcCamFormatInfo(IMAGE_FORMAT_E  format, int width, int height)
{	UVC_CAM_FMT_INF		*fmtInfo;
	UVC_CAM_FMT_INF		*newFmtInfo;
	if(format != UVC_FORMAT_MJPEG)
		return;
	
	for(fmtInfo = uvcCamInst->fmtInfos;fmtInfo;fmtInfo=fmtInfo->next)
	{	if((fmtInfo->width == width) && (fmtInfo->height == height))
		{	return;
		}
	}
	
	newFmtInfo = SENS_MEM_ZALLOC(sizeof(UVC_CAM_FMT_INF));
	newFmtInfo->format = UVC_FORMAT_MJPEG;
	newFmtInfo->width = width;
	newFmtInfo->height = height;
	if(uvcCamInst->fmtInfos == NULL)
	{	uvcCamInst->fmtInfos = newFmtInfo;
	}
	else
	{	fmtInfo = uvcCamInst->fmtInfos;
		while(fmtInfo->next)
		{	fmtInfo = fmtInfo->next;
		}
		fmtInfo->next = newFmtInfo;
	}
}
//------------------------------------------------------------------------------
void findBestResolution(UVC_CAMERA_CONTEXT *ctx, int *width, int *height)
{	int pixel, uvcPixel, maxPixel, minPixel;
	UVC_CAM_FMT_INF *fmtInfo;
	UVC_CAM_FMT_INF *mixFmtInfo;
	UVC_CAM_FMT_INF *maxFmtInfo;
	
	switch(ctx->streamRes)
	{	case CAMERA_RESOLUTION_VGA:		pixel = 640*480;	break;
		case CAMERA_RESOLUTION_QVGA:	pixel = 320*240;	break;
		case CAMERA_RESOLUTION_HD:		pixel = 1280*720;	break;
		case CAMERA_RESOLUTION_FHD:		pixel = 1920*1080;	break;
		case CAMERA_RESOLUTION_300M:	pixel = 3*1048576;	break;
		case CAMERA_RESOLUTION_500M:	pixel = 5*1048576;	break;
	}
	for(fmtInfo = uvcCamInst->fmtInfos;fmtInfo;fmtInfo=fmtInfo->next)
	{	uvcPixel = fmtInfo->width * fmtInfo->height;
		
		if(uvcPixel == pixel)
		{	*width = fmtInfo->width;
			*height = fmtInfo->height;
			return;
		}
	}
	
	minPixel = 100*1048576;
	for(fmtInfo = uvcCamInst->fmtInfos;fmtInfo;fmtInfo=fmtInfo->next)
	{	uvcPixel = fmtInfo->width * fmtInfo->height;
		
		if(uvcPixel == pixel)
		{	*width = fmtInfo->width;
			*height = fmtInfo->height;
			return;
		}
	}
	
	
}
//------------------------------------------------------------------------------
int uvcCameraStart(UVC_DEV_T *vdev)
{	int width, height;
	IMAGE_FORMAT_E  format;
	int ret;
	UVC_CAMERA_CONTEXT *ctx = (UVC_CAMERA_CONTEXT *)uvcCamInst->ctx;
	
	ctx->doSnapshot = 0;
	
	for(int idx=0;;idx++)
	{	ret = usbh_get_video_format((UVC_DEV_T *)vdev, idx, &format, &width, &height);
		if(ret != 0)
			break;
		addUvcCamFormatInfo(format, width, height);
		//dbgMsg("[%d] %s, %d x %d\r\n", idx, (format == UVC_FORMAT_MJPEG ? "MJPEG":"YUYV"), width, height);
	}
	findBestResolution(ctx, &width, &height);
	ret = usbh_set_video_format((UVC_DEV_T *)vdev, UVC_FORMAT_MJPEG, width, height);
	if(ret != 0)
	{	dbgMsg("usbh set video format failed!- 0x%x\r\n", ret);
		return UVC_STREAM_ERR_RESOLUTION;
	}
	ctx->skipStreamImgCount = 0;
	initImgBuffers();
	usbh_uvc_set_video_buffer((UVC_DEV_T *)vdev, _ig[_idx_usb].buff, IMAGE_MAX_SIZE);
	_ig[_idx_usb].state = IMAGE_BUFF_USB;
	dbgMsg("free run time memory:%lu\r\n", xPortGetFreeHeapSize());
	
	ret = usbh_uvc_start_streaming((UVC_DEV_T *)vdev, uvc_rx_callbak);
	if (ret != 0)
	{	dbgMsg("usbh_uvc_start_streaming failed! - %d\r\n", ret);
		dbgMsg("%s", "Please re-connect UVC device...\r\n");
		return UVC_STREAM_ERR_STREAM;
	}
	ctx->imgReady = 0;
	ctx->doSnapshot = 1;
	
	return UVC_STREAM_ERR_OK;
}
//------------------------------------------------------------------------------
int uvcCameraWaitImgReady(UVC_DEV_T *vdev)
{	int postSnapshotTime = 0;
	UVC_CAMERA_CONTEXT *ctx = (UVC_CAMERA_CONTEXT *)uvcCamInst->ctx;
	uint8_t *snapshotBuf = (uint8_t *)ctx->snapshotBufPool;
	int imgReady = 0;
	
	if (_ig[_idx_post].state == IMAGE_BUFF_READY)
	{	dbgMsg("One JPG ready, buf idx:%d, length = %d, skip frame Cnt:%d\r\n", _idx_post, _ig[_idx_post].len, gSkipFrame);
		_ig[_idx_post].state = IMAGE_BUFF_POST;
		ctx->skipStreamImgCount++;
		if(ctx->doSnapshot && (ctx->skipStreamImgCount > IGNORE_IMG_FRAME_CNT))
		{	if(_ig[_idx_post].len < IMAGE_MAX_SIZE)
			{	memcpy(snapshotBuf, _ig[_idx_post].buff, _ig[_idx_post].len);
				ctx->shapshotLength = _ig[_idx_post].len;
				ctx->doSnapshot = 0;
				ctx->imgReady = 1;
				imgReady = 1;
				
				usbh_uvc_stop_streaming((UVC_DEV_T *)vdev);
			}
			else
			{	dbgMsg("length over IMAGE_MAX_SIZE!! %d - %d\r\n", _ig[_idx_post].len, IMAGE_MAX_SIZE);
				imgReady = -1;
				//down resolution and retry.
			}
		}
		if(postSnapshotTime != 0)
		{
		}
		_ig[_idx_post].state = IMAGE_BUFF_FREE;
		_idx_post = (_idx_post + 1) % IMAGE_BUFF_CNT;
	}
	
	return imgReady;
}
//------------------------------------------------------------------------------
void uvcCameraPowerOff(void)
{	SYS_UnlockReg();
	
	//turn off HS VBUS
	SET_GPIO_PB10();
	PIN_USB_HS_VBUS_EN = 0;
	GPIO_SetMode(PB, BIT10, GPIO_MODE_OUTPUT);
}
//------------------------------------------------------------------------------
void uvcCameraStop(UVC_DEV_T *vdev)
{	uvcCameraPowerOff();
	//GPIO_SetMode(PB, BIT11, GPIO_MODE_INPUT);
	deinitImageBuf();
	//SET_GPIO_PB11();
	//SET_HSUSB_VBUS_EN_PB10();
	//SET_HSUSB_VBUS_ST_PB11();
}
//------------------------------------------------------------------------------
static void createJpgFileName(void)
{	CAMERA_WEB_MANAGER	*camWebMng = (CAMERA_WEB_MANAGER *)&sysCtrl->camWebMng;
	IMAGE_UPLOAD_INSTANCE *imgUploadInst = (IMAGE_UPLOAD_INSTANCE *)networkCtx->imgUploadInst;;
#ifdef NUVOTON
	S_RTC_TIME_DATA_T fileDt;
#else
	DATE_STRUCT fileDate;
	TIME_STRUCT fileRtcTime;
#endif
	
	if(uvcCamInst->saveFileName)
		SENS_MEM_FREE(uvcCamInst->saveFileName);
	uvcCamInst->saveFileName = SENS_MEM_ZALLOC(128);
	
#if defined NUVOTON
	memcpy((char *)&fileDt, (char *)&sensSys->dateTime, sizeof(S_RTC_TIME_DATA_T));
	if((camWebMng->status == CWS_STS_IDLE) && (!imgUploadInst->isAlarmUploadImgMode))
		fileDt.u32Second = 0;
#else
	
#endif
	
	
#if defined NUVOTON
	SENS_SPRINTF(uvcCamInst->saveFileName, "DCIM/IMG%04d%02d%02d_%02d%02d%02d.jpg", fileDt.u32Year, fileDt.u32Month, fileDt.u32Day, fileDt.u32Hour, fileDt.u32Minute, fileDt.u32Second);
#else
	SENS_SPRINTF(uvcCamInst->saveFileName, "DCIM/IMG%04d%02d%02d_%02d%02d%02d.jpg", sensSys->date_time[0], sensSys->date_time[1], sensSys->date_time[2], sensSys->date_time[3], sensSys->date_time[4], sensSys->date_time[5]);
#endif
	dbgMsg("[UVC] save File name: %s\r\n", uvcCamInst->saveFileName);
	
	if(camWebMng->status != CWS_STS_IDLE)
	{	memset(camWebMng->fileName, 0, IMG_FILE_SIZE);
		strcat(camWebMng->fileName, uvcCamInst->saveFileName);
	}
	
	uvcCamInst->imgUnixTime = dateToTimestamp(&fileDt);
}
//------------------------------------------------------------------------------
void uvcCameraPowerOn(void)
{
	//SYS_UnlockReg();
	//turn on HS VBUS
	SET_HSUSB_VBUS_EN_PB10();
	dbgMsg("%s", "[HS-USB] Power on!\r\n");
	createJpgFileName();	
	//SET_HSUSB_VBUS_ST_PB11();
}
//------------------------------------------------------------------------------
static void updateRecImgUploadSts(void)
{	IMAGE_UPLOAD_INSTANCE *imgUploadInst = (IMAGE_UPLOAD_INSTANCE *)networkCtx->imgUploadInst;
	IMAGE_UPLOAD_REC_HEADER *imgUploadHeader;
	IMAGE_UPLOAD_INFO *imgUploadInfo;
	char deleteOldestInfo=0;
	
	SENS_SEM_LOCK(imgUploadInst->imgUploadSem);
	imgUploadHeader = imgUploadInst->imgUploadRecHeader;
	imgUploadHeader->numOfUnSendImage++;
	if(imgUploadHeader->numOfUnSendImage > MAX_REC_IMG_COUNT)
	{	imgUploadHeader->numOfUnSendImage = MAX_REC_IMG_COUNT;
		deleteOldestInfo = 1;
	}
	
	imgUploadInfo = SENS_MEM_ZALLOC(sizeof(IMAGE_UPLOAD_INFO));
	imgUploadInfo->unixTime = uvcCamInst->imgUnixTime;
	memset(imgUploadInfo->fileName, 0, IMG_FILE_SIZE);
	strcpy(imgUploadInfo->fileName, uvcCamInst->saveFileName);
	imgUploadInfo->valid = 1;
	sdWriteFile(IMG_REC_FILE_NAME, (char *)imgUploadHeader, sizeof(IMAGE_UPLOAD_REC_HEADER), 0, OVER_WRITE_MODE);
	if(imgUploadInst->imgUploadRecInfoBufLen)
	{	if(deleteOldestInfo)
		{	imgUploadInst->imgUploadRecInfoBufLen -= sizeof(IMAGE_UPLOAD_INFO);
			for(int idx=0;idx<imgUploadInst->imgUploadRecInfoBufLen;idx++)
			{	imgUploadInst->imgUploadRecInfoBuf[idx] = imgUploadInst->imgUploadRecInfoBuf[idx + sizeof(IMAGE_UPLOAD_INFO)];
			}
		}
		sdWriteFile(IMG_REC_FILE_NAME, (char *)imgUploadInst->imgUploadRecInfoBuf, imgUploadInst->imgUploadRecInfoBufLen, sizeof(IMAGE_UPLOAD_REC_HEADER), OVER_WRITE_MODE);
	}
	sdWriteFile(IMG_REC_FILE_NAME, (char *)imgUploadInfo, sizeof(IMAGE_UPLOAD_INFO), sizeof(IMAGE_UPLOAD_REC_HEADER) + imgUploadInst->imgUploadRecInfoBufLen, OVER_WRITE_MODE);
	memcpy(&imgUploadInst->imgUploadRecInfoBuf[imgUploadInst->imgUploadRecInfoBufLen], (char *)imgUploadInfo, sizeof(IMAGE_UPLOAD_INFO));
	imgUploadInst->imgUploadRecInfoBufLen += sizeof(IMAGE_UPLOAD_INFO);
	SENS_MEM_FREE(imgUploadInfo);
	SENS_SEM_UNLOCK(imgUploadInst->imgUploadSem);
	
}
//------------------------------------------------------------------------------
void saveImgFile(void)
{	UVC_CAMERA_CONTEXT *ctx = (UVC_CAMERA_CONTEXT *)uvcCamInst->ctx;
	uint8_t *snapshotBuf = (uint8_t *)ctx->snapshotBufPool;
	CAMERA_WEB_MANAGER	*camWebMng = (CAMERA_WEB_MANAGER *)&sysCtrl->camWebMng;
	
	createImgFile(uvcCamInst->saveFileName);
	sdWriteFile((char *)uvcCamInst->saveFileName, (char *)snapshotBuf, ctx->shapshotLength, 0, OVER_WRITE_MODE);
	
	SENS_MEM_FREE(ctx->snapshotBufPool);
	ctx->snapshotBufPool = NULL;
	
	if(camWebMng->status == CWS_STS_RUNNING)
	{	camWebMng->status = CWS_STS_DONE;
		if(camWebMng->sendToCloud)
		{	camWebMng->fileLength = ctx->shapshotLength;
			camWebMng->readyToSendImgToCloud = 1;
			updateRecImgUploadSts();
			camWebMng->sendToCloud = 0;
		}
	}
	else
		updateRecImgUploadSts();
}
//------------------------------------------------------------------------------
void initUvcCamDevice(void)
{	UVC_CAMERA_CONTEXT *ctx;
	CAMERA_CONFIG *cameraCfg;
	//UVC_CAM_UPLOAD_CONTEXT	*camUploadCtx;
	IMAGE_UPLOAD_INSTANCE *imgUploadInst;
	CAMERA_WEB_MANAGER	*camWebMng = (CAMERA_WEB_MANAGER *)&sysCtrl->camWebMng;
	
	sensDev->uvcCamInst = SENS_MEM_ZALLOC(sizeof(UVC_CMERA_INSTANCE));
	
	uvcCamInst = (volatile UVC_CMERA_INSTANCE *)sensDev->uvcCamInst;
	
	uvcCamInst->cfg = sysCfg->cameraCfg;
	uvcCamInst->ctx = SENS_MEM_ZALLOC(sizeof(UVC_CAMERA_CONTEXT));
	
	ctx = uvcCamInst->ctx;
	cameraCfg = uvcCamInst->cfg;
	ctx->streamRes = cameraCfg->resolution;	
	
	//ctx->camUploadCtx = SENS_MEM_ZALLOC(sizeof(UVC_CAM_UPLOAD_CONTEXT));
	
	
	networkCtx->imgUploadInst = SENS_MEM_ZALLOC(sizeof(IMAGE_UPLOAD_INSTANCE));
	imgUploadInst = networkCtx->imgUploadInst;
	if(SENS_SEM_INIT(imgUploadInst->imgUploadSem, 1) != MQX_OK)
	{	
	}
	imgUploadInst->imgUploadRecHeader = SENS_MEM_ZALLOC(sizeof(IMAGE_UPLOAD_REC_HEADER));
	imgUploadInst->imgUploadRecInfoBuf = SENS_MEM_ZALLOC(sizeof(IMAGE_UPLOAD_INFO) * MAX_REC_IMG_COUNT);
	camWebMng->isSupported = 1;
	checkUnSendImg(imgUploadInst);
}
//------------------------------------------------------------------------------