#ifndef __USBH_UVC_H__
#define __USBH_UVC_H__

//#include "NuMicro.h"
#include "sensminiCfg.h"
#include "usb.h"

#define UVC_CLASS_CODE 					0x0E
#define UVC_SUBCALSS_CODE_CTRL			0x01
#define UVC_SUBCALSS_CODE_STREAMING		0x02
#define UVC_SUBCALSS_CODE_IF_COLLECTION	0x03

#define UVC_PROTOCOL_CODE_PROT_15		0x01


#define BL_3RU_T_CAMERA_VID	0x0BDA
#define BL_3RU_T_CAMERA_PID	0x3035

#endif