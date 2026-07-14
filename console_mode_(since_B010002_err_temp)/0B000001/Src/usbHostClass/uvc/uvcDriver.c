#include "usbhUvc.h"


static int uvc_probe(IFACE_T *iface)
{	UDEV_T *udev = iface->udev;
	ALT_IFACE_T *aif = iface->aif;
	DESC_IF_T *ifd;
	
	
	ifd = aif->ifd;
	
	if(ifd->bInterfaceClass == UVC_CLASS_CODE)
	{
	}
	
	
}

static void uvc_disconnect(IFACE_T *iface)
{
}


static UDEV_DRV_T uvc_driver = 
{	uvc_probe,
	uvc_disconnect,
	NULL,
	NULL
};





void usbhUvcInit(void)
{	usbh_register_driver(&uvc_driver);
}