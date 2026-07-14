#include "usbHost/usbHostTask.h"
#include "sensDev.h"
#include "network/netApp.h"
#include "sensors/sensorTask.h"
#include "sensSystem.h"

QueueHandle_t usbHostQ;

extern portBASE_TYPE xInsideISR;

#define USB_XFER_TIMEOUT             100

#ifdef SUPPORT_USB_CDC_ECM
struct netif ethernetNetif;
#endif

typedef struct __attribute__((__packed__)) _CDC_NOTIFICATION
{	uint8_t bmRequestType;
	uint8_t bNotification;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
}CDC_NOTIFICATION;

volatile USBH_DEVICE	*gUsbhDev;
volatile uint8_t		*ecmRxPbufStorage;
volatile uint32_t 		gUsbUartCfgBmp;

#ifdef SUPPORT_USB_CDC_ECM
PBUF_QUEUE *pQueue = NULL;

//------------------------------------------------------------------------------
void pqCreate()
{	//PBUF_QUEUE *q = NULL;
	
	if(pQueue == NULL)
	{	pQueue = SENS_MEM_ZALLOC(sizeof(PBUF_QUEUE));
	}
}
//------------------------------------------------------------------------------
int pqEnqueue(PBUF_QUEUE *q, void *p)
{	if(q->len == PBUF_QUEUE_SIZE)
		return -1;
	
	q->data[q->head] = p;
	q->head = (q->head+1) % PBUF_QUEUE_SIZE;
	q->len++;
	
	return 0;
}
//------------------------------------------------------------------------------
void *pqDequeue(PBUF_QUEUE *q)
{	int ptail;
	if(q->len == 0)
		return NULL;
	
	ptail = q->tail;
	q->tail = (q->tail + 1) % PBUF_QUEUE_SIZE;
	taskENTER_CRITICAL();
	q->len--;
	taskEXIT_CRITICAL();
	return q->data[ptail];
}
//------------------------------------------------------------------------------
int pqQlength(PBUF_QUEUE *q)
{	return q->len;
}
#endif


//------------------------------------------------------------------------------
//			UVC
//------------------------------------------------------------------------------
volatile UVC_DEV_T   *g_vdev = NULL;
int   _total_frame_count = 0;



//------------------------------------------------------------------------------
//		composite
//------------------------------------------------------------------------------
void usbCdcAcmSerialStateNotifyProcess(CDC_DEV_T *cdev, MOBILE_INSTANCE *wlInst);

//------------------------------------------------------------------------------
void cdcCompositeDevNotify(void *ctx, int sts, uint8_t *rdata, int data_len)
{	CDC_NOTIFICATION *cdcNotify;
	USB_COMPOSITE_DEVICE_IF_CTX *compositeDevIf = (USB_COMPOSITE_DEVICE_IF_CTX *)ctx;
	uint16_t *value;
	uint8_t idx;
		
	cdcNotify = (CDC_NOTIFICATION *)&rdata[0];
	if(data_len && cdcNotify->bmRequestType == 0xA1)
	{	dbgMsg2("%s", "[VCOM NOTIFY] ");
		for(idx = 0; idx < data_len; idx++)
			dbgMsg2("0x%02x ", rdata[idx]);
		dbgMsg2("%s", "\r\n");
		if(cdcNotify->bNotification == CDC_NOTIFY_CODE_NETWORK_CONNECTION)
		{	dbgMsg2("[VCOM NOTIFY] network connection %s\r\n", cdcNotify->wValue? "connected":"disconnect");
			if(cdcNotify->wValue == NETWORK_CONNECTION_CONNECTED)
			{	
			}
		}
		else if(cdcNotify->bNotification == CDC_NOTIFY_CODE_CONNECTION_SPEED_CHANGE)
		{	uint32_t *speedBitRate;
			speedBitRate = (uint32_t *)&cdcNotify[1];
			dbgMsg2("[VCOM NOTIFY] speed change dl:%d, ul:%d\r\n", speedBitRate[0], speedBitRate[1]);
		}
		else if(cdcNotify->bNotification == CDC_NOTIFY_CODE_SERIAL_STATE)
		{	value = (uint16_t *)&cdcNotify[1];
			compositeDevIf->currDsrSignal = (*value >> 1) & 0x01;
			if(compositeDevIf->currDsrSignal && (compositeDevIf->currDsrSignal != compositeDevIf->prevDsrSignal))
			{	//AT is Ready
			}
		}
	}
}
//------------------------------------------------------------------------------
void intInIrqDone(UTR_T *utr)
{	int ret;
	USB_COMPOSITE_DEVICE_IF_CTX *compositeDevIf = (USB_COMPOSITE_DEVICE_IF_CTX *)utr->context;
	
	if(compositeDevIf->intInCbFunc == NULL)
		goto ERR_OUT;
	if(utr->status != 0)	
	{	compositeDevIf->intInCbFunc(compositeDevIf, utr->status, utr->buff, (int)utr->xfer_len);
		goto ERR_OUT;
	}
	if(utr->xfer_len)
		compositeDevIf->intInCbFunc(compositeDevIf, utr->status, utr->buff, (int)utr->xfer_len);
	
	utr->xfer_len = 0;
	ret = usbh_int_xfer(utr);
	if(ret != 0)
	{	compositeDevIf->intInCbFunc(compositeDevIf, utr->status, utr->buff, (int)utr->xfer_len);
		goto ERR_OUT;
	}
	return;
ERR_OUT:
	if(xPortIsInsideInterrupt())
	{	//in irq don't free
	}
	else
	{	if(utr == compositeDevIf->utr_int_in[0])
		{	free_utr(compositeDevIf->utr_int_in[0]);
			compositeDevIf->utr_int_in[0] = NULL;
		}
		else if(utr == compositeDevIf->utr_int_in[1])
		{	free_utr(compositeDevIf->utr_int_in[1]);
			compositeDevIf->utr_int_in[1] = NULL;
		}
	}
}
//------------------------------------------------------------------------------
//static volatile USB_VENDOR_DEVICE *multiVendorDev;
int usbhCdcStartPollingStatus(void *ctx, CDC_NOTIFY_CB_FUNC *func)
{	EP_INFO_T   *ep;
	UTR_T       *utr;
	int         ret;
	USB_COMPOSITE_DEVICE_IF_CTX *compositeDevIf = (USB_COMPOSITE_DEVICE_IF_CTX *)ctx;

	if((compositeDevIf == NULL) || (compositeDevIf->ep_int_in == NULL))
		return USBH_ERR_NOT_FOUND;

	if(!func || compositeDevIf->utr_int_in[0])
		return USBH_ERR_INVALID_PARAM;

	ep = compositeDevIf->ep_int_in;
	if(ep == NULL)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("%s", "Interrupt-in endpoint not found in this CDC device!\r\n");
		USB_DBG_UNLOCK();
		return USBH_ERR_EP_NOT_FOUND;
	}

	utr = alloc_utr(compositeDevIf->udev);
	if(utr == NULL)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("%s", "Failed to allocated UTR!\r\n");
		USB_DBG_UNLOCK();
		return USBH_ERR_MEMORY_OUT;
	}

	utr->buff = (uint8_t *)compositeDevIf->buff_int_in[0];
	utr->context = compositeDevIf;
	utr->ep = ep;
	utr->data_len = ep->wMaxPacketSize;
	if(utr->data_len > CDC_STATUS_BUFF_SIZE)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("Warning! CDC_STATUS_BUFF_SIZE %d is smaller than max. packet size %d!\r\n", CDC_STATUS_BUFF_SIZE, ep->wMaxPacketSize);
		USB_DBG_UNLOCK();
		utr->data_len = CDC_STATUS_BUFF_SIZE;
	}
	utr->xfer_len = 0;
	utr->func = intInIrqDone;

	compositeDevIf->utr_int_in[0] = utr;
	compositeDevIf->intInCbFunc = func;

	ret = usbh_int_xfer(utr);
	if(ret < 0)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("Error - failed to submit interrupt read request (%d)\r\n", ret);
		USB_DBG_UNLOCK();
		free_utr(utr);
		compositeDevIf->utr_int_in[0] = NULL;
		return ret;
	}
	return 0;
}
//------------------------------------------------------------------------------
void addUsbhdev(UDEV_T *udev, int devType)
{	USBH_DEVICE	*usbhDevTemp;
	USBH_DEVICE	*newUsbhDev;
	
	newUsbhDev = SENS_MEM_ZALLOC(sizeof(USBH_DEVICE));
	newUsbhDev->udev = udev;
	newUsbhDev->udevType = devType;
	
	if(gUsbhDev == NULL)
	{	gUsbhDev = newUsbhDev;
	}
	else
	{	usbhDevTemp = (USBH_DEVICE *)gUsbhDev;
		while(usbhDevTemp->next)
		{	usbhDevTemp = usbhDevTemp->next;
		}
		usbhDevTemp->next = newUsbhDev;
		newUsbhDev->prev = usbhDevTemp;
	}
}
//------------------------------------------------------------------------------
USBH_DEVICE	*findAndAddUsbhdev(UDEV_T *udev, int devType)
{	UDEV_T *udevTemp;
	USBH_DEVICE	*usbhDevTemp;
	
	if(gUsbhDev == NULL)
		addUsbhdev(udev, devType);
FIND_USBH_DEV:
	usbhDevTemp = (USBH_DEVICE *)gUsbhDev;
	while(usbhDevTemp)
	{	if(usbhDevTemp->udev == udev)
		{	break;
		}
		usbhDevTemp = usbhDevTemp->next;
	}
	if(usbhDevTemp == NULL)
	{	addUsbhdev(udev, devType);
		goto FIND_USBH_DEV;
	}
	
	return usbhDevTemp;
}
//------------------------------------------------------------------------------
USBH_DEVICE	*findUsbhDev(UDEV_T *udev)
{	UDEV_T *udevTemp;
	USBH_DEVICE	*usbhDevTemp;
	
	if(gUsbhDev == NULL)
		return NULL;
	usbhDevTemp = (USBH_DEVICE *)gUsbhDev;
	while(usbhDevTemp)
	{	if(usbhDevTemp->udev == udev)
		{	break;
		}
		usbhDevTemp = usbhDevTemp->next;
	}
	if(usbhDevTemp == NULL)
	{	return NULL;
	}
	
	return usbhDevTemp;
}
//------------------------------------------------------------------------------
void addUsbHVendorDevice(USB_COMPOSITE_DEVICE_IF_CTX *sVendorDevIfCtx, uint8_t devType)
{	USB_COMPOSITE_DEVICE_IF_CTX *tempVendorDev;
	USB_COMPOSITE_DEVICE_IF_CTX *udevIfCtx;
	UDEV_T       				*udev;
	USBH_DEVICE	*usbhDev;
	udev = sVendorDevIfCtx->udev;
	
	usbhDev = findAndAddUsbhdev(udev, devType);
	udevIfCtx = (USB_COMPOSITE_DEVICE_IF_CTX *)usbhDev->udevIfCtx;
	sVendorDevIfCtx->upperCtx = usbhDev;
	
	if(udevIfCtx == NULL)
	{	usbhDev->udevIfCtx = sVendorDevIfCtx;
	}
	else
	{	tempVendorDev = udevIfCtx;
		while(1)
		{	if(tempVendorDev->next ==  NULL)
				break;
			tempVendorDev = tempVendorDev->next;
		}
		tempVendorDev->next = sVendorDevIfCtx;
		sVendorDevIfCtx->prev = tempVendorDev;
	}
}
//------------------------------------------------------------------------------
void removeUsbHVendorDevice(USB_COMPOSITE_DEVICE_IF_CTX *sVendorDevIfCtx)
{	USB_COMPOSITE_DEVICE_IF_CTX *nextOfRemoveDev;
	USB_COMPOSITE_DEVICE_IF_CTX *prevOfRemoveDev;
	USBH_DEVICE *usbhDev;
	
	usbhDev = (USBH_DEVICE *)sVendorDevIfCtx->upperCtx;
	
	prevOfRemoveDev = sVendorDevIfCtx->prev;
	nextOfRemoveDev = sVendorDevIfCtx->next;
	
	if(prevOfRemoveDev)
		prevOfRemoveDev->next = nextOfRemoveDev;
	if(nextOfRemoveDev)
		nextOfRemoveDev->prev = prevOfRemoveDev;
	if(usbhDev->udevIfCtx == sVendorDevIfCtx)	//first
		usbhDev->udevIfCtx = nextOfRemoveDev;
	SENS_MEM_FREE(sVendorDevIfCtx);
}
//------------------------------------------------------------------------------
void *findDevIfCtx(IFACE_T *iface)
{	UDEV_T *udev;
	USBH_DEVICE	*usbhDev;
	udev = iface->udev;
	
	usbhDev = (USBH_DEVICE *)gUsbhDev;
	if(usbhDev == NULL)
		return NULL;
	else
	{	while(usbhDev)
		{	if(usbhDev->udev == udev)
				break;
			usbhDev = usbhDev->next;
		}
		if(usbhDev == NULL)
			return usbhDev;
	}
	
	return usbhDev->udevIfCtx;
}
//------------------------------------------------------------------------------
void vendor_interrupt_in_stop(USB_COMPOSITE_DEVICE_IF_CTX *sVendorDevIfCtx)
{	int i;

	/* clear <int_in_func> to stop cascading transfers */
	//sVendorDevIfCtx->int_in_func = NULL;
	sVendorDevIfCtx->intInCbFunc = NULL;
	delay_us(32000);

	for(i = 0; i < 2; i++)
	{	if(sVendorDevIfCtx->utr_int_in[i] != NULL)
		{	usbh_quit_utr(sVendorDevIfCtx->utr_int_in[i]);    /* force to stop the transfer   */
			delay_us(32000);
			free_utr(sVendorDevIfCtx->utr_int_in[i]);
			sVendorDevIfCtx->utr_int_in[i] = NULL;
		}
	}
}
//------------------------------------------------------------------------------
void vendor_interrupt_out_stop(USB_COMPOSITE_DEVICE_IF_CTX *sVendorDevIfCtx)
{	int i;

	/* clear <int_in_func> to stop cascading transfers */
	//sVendorDevIfCtx->int_out_func = NULL;
	sVendorDevIfCtx->intOutCbFunc = NULL;
	delay_us(32000);

	for(i = 0; i < 2; i++)
	{	if(sVendorDevIfCtx->utr_int_out[i] != NULL)
		{	usbh_quit_utr(sVendorDevIfCtx->utr_int_out[i]);    /* force to stop the transfer   */
			delay_us(32000);
			free_utr(sVendorDevIfCtx->utr_int_out[i]);
			sVendorDevIfCtx->utr_int_out[i] = NULL;
		}
	}
}
//------------------------------------------------------------------------------
void vendor_isochronous_in_stop(USB_COMPOSITE_DEVICE_IF_CTX *sVendorDevIfCtx)
{	int i;

	/* clear <iso_in_func> to stop cascading transfers */
	sVendorDevIfCtx->iso_in_func = NULL;

	for(i = 0; i < ISO_UTR_NUM; i++)        /* stop all UTRs                              */
	{	if(sVendorDevIfCtx->utr_iso_in[i])
			usbh_quit_utr(sVendorDevIfCtx->utr_iso_in[i]);
	}

	if((sVendorDevIfCtx->utr_iso_in[0] != NULL) && (sVendorDevIfCtx->utr_iso_in[0]->buff != NULL))       /* free audio buffer                */
		usbh_free_mem(sVendorDevIfCtx->utr_iso_in[0]->buff, (int)(sVendorDevIfCtx->utr_iso_in[0]->data_len * ISO_UTR_NUM));

	for(i = 0; i < ISO_UTR_NUM; i++)        /* free all UTRs                              */
	{	if(sVendorDevIfCtx->utr_iso_in[i])
			free_utr(sVendorDevIfCtx->utr_iso_in[i]);
		sVendorDevIfCtx->utr_iso_in[i] = NULL;
	}
}
//------------------------------------------------------------------------------
void vendor_isochronous_out_stop(USB_COMPOSITE_DEVICE_IF_CTX *sVendorDevIfCtx)
{	int i;

	/* clear <iso_out_func> to stop cascading transfers */
	sVendorDevIfCtx->iso_out_func = NULL;

	for(i = 0; i < ISO_UTR_NUM; i++)            /* stop all UTRs                              */
	{	if(sVendorDevIfCtx->utr_iso_out[i])
			usbh_quit_utr(sVendorDevIfCtx->utr_iso_out[i]);
	}

	if((sVendorDevIfCtx->utr_iso_out[0] != NULL) && (sVendorDevIfCtx->utr_iso_out[0]->buff != NULL))       /* free transfer buffer           */
		usbh_free_mem(sVendorDevIfCtx->utr_iso_out[0]->buff, (int)(sVendorDevIfCtx->utr_iso_out[0]->data_len * ISO_UTR_NUM));

	for(i = 0; i < ISO_UTR_NUM; i++)            /* free all UTRs                              */
	{	if(sVendorDevIfCtx->utr_iso_out[i])
			free_utr(sVendorDevIfCtx->utr_iso_out[i]);
		sVendorDevIfCtx->utr_iso_out[i] = NULL;
	}
}
//------------------------------------------------------------------------------
int usbhVendorBulkWrite(USB_COMPOSITE_DEVICE_IF_CTX *devIfCtx, uint8_t *dataBuf, int dataLen)
{	UTR_T *utr;
	uint32_t t0;
	int ret;

	if((devIfCtx->udev == NULL) || (devIfCtx->ep_bulk_out == NULL))
		return -1;

	utr = alloc_utr(devIfCtx->udev);
	if(!utr)
		return USBH_ERR_MEMORY_OUT;

	utr->ep = devIfCtx->ep_bulk_out;
	utr->buff = dataBuf;
	utr->data_len = (uint32_t)dataLen;
	utr->xfer_len = 0;
	utr->func = NULL;
	utr->bIsTransferDone = 0;

	ret = usbh_bulk_xfer(utr);
	if(ret < 0)
	{	free_utr(utr);
		return ret;
	}

	t0 = get_ticks();
	while(utr->bIsTransferDone == 0)
	{	if((get_ticks() - t0) > USB_XFER_TIMEOUT)
		{	usbh_quit_utr(utr);
			free_utr(utr);
			return USBH_ERR_TIMEOUT;
		}
	}
	ret = utr->status;
	free_utr(utr);
	return ret;
}
//------------------------------------------------------------------------------
void vendorBulkInIrq(UTR_T *utr)
{	USB_COMPOSITE_DEVICE_IF_CTX *devIfCtx;
	int ret;
	
	devIfCtx = (USB_COMPOSITE_DEVICE_IF_CTX *)utr->context;
	if(utr->status)
	{	return;
	}
	if(devIfCtx->bulkInCbFunc)
		devIfCtx->bulkInCbFunc(devIfCtx, utr->buff, utr->xfer_len);
	
	utr->xfer_len = 0;
	ret = usbh_bulk_xfer(utr);
	if(ret < 0)
	{	free_utr(utr);
		devIfCtx->utr_rx = NULL;
	}
#if 0
	free_utr(utr);
	devIfCtx->utr_rx = NULL;
	devIfCtx->bulkInBusy = 0;
#endif
}
//------------------------------------------------------------------------------
int usbhVendorStartToReceiveBulkInData(USB_COMPOSITE_DEVICE_IF_CTX *devIfCtx, VENDOR_BULK_CB_FUNC *func)
{	UTR_T *utr;
	uint32_t t0;
	int ret;

	if((devIfCtx->udev == NULL) || (devIfCtx->ep_bulk_in == NULL))
		return -1;

	utr = alloc_utr(devIfCtx->udev);
	if(!utr)
		return USBH_ERR_MEMORY_OUT;

	utr->buff = (uint8_t *)devIfCtx->bulkInBuf;
	utr->context = devIfCtx;
	utr->ep = devIfCtx->ep_bulk_in;
	utr->data_len = devIfCtx->ep_bulk_in->wMaxPacketSize;
	utr->xfer_len = 0;
	//utr->func = NULL;
	//utr->bIsTransferDone = 0;
	utr->func = vendorBulkInIrq;
	
	devIfCtx->bulkInCbFunc = func;
	devIfCtx->utr_rx = utr;
	devIfCtx->bulkInBusy = 1;
	
	ret = usbh_bulk_xfer(utr);
	if(ret < 0)
	{	free_utr(utr);
		devIfCtx->utr_rx = NULL;
		devIfCtx->bulkInBusy = 0;
		return ret;
	}

	return 0;
}
//------------------------------------------------------------------------------
static int vendor_probe(IFACE_T *iface)
{	UDEV_T *udev = iface->udev;
	ALT_IFACE_T *aif = iface->aif;
	DESC_IF_T *ifd;              /* interface descriptor */
	int i;
	uint8_t ifMatch = 0;
	USB_COMPOSITE_DEVICE_IF_CTX *sVendorDevIfCtx;
	uint8_t usbDevType;
	
	ifd = aif->ifd;

#if 0
	if(((udev->descriptor.idVendor != TELIT_VID) && ((udev->descriptor.idProduct != TELIT_ME310_PID) || (udev->descriptor.idProduct != TELIT_LE910_PID))) 
		//||	//for R410
		)
	{	USB_DBG_LOCK();
		VENDOR_DBGMSG("%s", "Not Vendor device. Ignore this device.\r\n");
		USB_DBG_UNLOCK();
		return USBH_ERR_NOT_MATCHED;
	}
#endif	

	if(ifd->bInterfaceClass == 0xFF)
	{	if(udev->descriptor.idVendor == TELIT_VID)
		{	if(udev->descriptor.idProduct == TELIT_ME310_PID)
			{	ifMatch = 1;
				usbDevType = USB_DEVICE_TYPE_CDC_ACM_COMPOSITE;
			}
			else if(udev->descriptor.idProduct == TELIT_LE910_PID)
			{ ifMatch = 1;
				usbDevType = USB_DEVICE_TYPE_CDC_ACM_COMPOSITE;
			}
			else if(udev->descriptor.idProduct == TELIT_LE910C4_WWX_PID_0x1031)
			{	ifMatch = 1;
				usbDevType = USB_DEVICE_TYPE_CDC_ACM_COMPOSITE;
			}	
			
#ifdef LE910_USE_CDC_ECM
			else if(udev->descriptor.idProduct == TELIT_LE910_PID_0x1206)
			{	ifMatch = 1;
				usbDevType = USB_DEVICE_TYPE_CDC_ECM_COMPOSITE;
			}
#endif
		}
		else if((udev->descriptor.idVendor == UBLOX_R410_VID) && (udev->descriptor.idProduct == UBLOX_R410_PID))
		{	ifMatch = 1;
			usbDevType = USB_DEVICE_TYPE_CDC_ACM_COMPOSITE;
		}
#ifdef SUPPORT_USB_CDC_ECM
		else if((udev->descriptor.idVendor == ASIX_VID) && (udev->descriptor.idProduct == ASIX_AX88179_PID))
		{	ifMatch = 1;
			usbDevType = USB_DEVICE_TYPE_CDC_ECM;
		}
		else if((udev->descriptor.idVendor == REALTEK_VID) && (udev->descriptor.idProduct == RTL8153_PID))
		{	ifMatch = 1;
			usbDevType = USB_DEVICE_TYPE_CDC_ECM;
		}
#endif
	}
	/* Is this interface HID class? */
	/*else if((ifd->bInterfaceClass != 0xFF) || (ifd->bInterfaceSubClass != 0xFF))
	{	printf("Not Vendor interface. Ignore this interface.\n");
		return USBH_ERR_NOT_MATCHED;
	}*/
	
	
	if(!ifMatch)
	{	USB_DBG_LOCK();
		VENDOR_DBGMSG("%s", "Not Vendor interface. Ignore this interface.\r\n");
		USB_DBG_UNLOCK();
		return USBH_ERR_NOT_MATCHED;
	}

	
	/*if(sVendorDev.udev != NULL)
	{	printf("A Vendor LBK device is connected, do not support multiple devices!\n");
		return USBH_ERR_NOT_MATCHED;
	}*/
	sVendorDevIfCtx = SENS_MEM_ZALLOC(sizeof(USB_COMPOSITE_DEVICE_IF_CTX));

	USB_DBG_LOCK();
	printf("lbk_probe - device found (vid=0x%x, pid=0x%x), interface %d.\r\n",	udev->descriptor.idVendor, udev->descriptor.idProduct, ifd->bInterfaceNumber);
	USB_DBG_UNLOCK();

	/*
	 *  Find all endpoints of LBK device
	 */
	for(i = 0; i < aif->ifd->bNumEndpoints; i++)
	{	if((aif->ep[i].bmAttributes & EP_ATTR_TT_MASK) == EP_ATTR_TT_BULK)
		{	USB_DBG_LOCK();
			VENDOR_DBGMSG("Bulk-%s endpoint: 0x%x\r\n", ((aif->ep[i].bEndpointAddress & EP_ADDR_DIR_MASK) == EP_ADDR_DIR_IN) ? "in" : "out", aif->ep[i].bEndpointAddress);
			USB_DBG_UNLOCK();
			if((aif->ep[i].bEndpointAddress & EP_ADDR_DIR_MASK) == EP_ADDR_DIR_IN)
			{	sVendorDevIfCtx->ep_bulk_in = &aif->ep[i];
				sVendorDevIfCtx->bulkInBuf = (uint32_t *)SENS_MEM_ZALLOC(aif->ep[i].wMaxPacketSize);
			}
			else
				sVendorDevIfCtx->ep_bulk_out = &aif->ep[i];
		}
		else if((aif->ep[i].bmAttributes & EP_ATTR_TT_MASK) == EP_ATTR_TT_INT)
		{	USB_DBG_LOCK();
			VENDOR_DBGMSG("Interrupt-%s endpoint: 0x%x\r\n", ((aif->ep[i].bEndpointAddress & EP_ADDR_DIR_MASK) == EP_ADDR_DIR_IN) ? "in" : "out", aif->ep[i].bEndpointAddress);
			USB_DBG_UNLOCK();
			if((aif->ep[i].bEndpointAddress & EP_ADDR_DIR_MASK) == EP_ADDR_DIR_IN)
				sVendorDevIfCtx->ep_int_in = &aif->ep[i];
			else
				sVendorDevIfCtx->ep_int_out = &aif->ep[i];
		}
		else if((aif->ep[i].bmAttributes & EP_ATTR_TT_MASK) == EP_ATTR_TT_ISO)
		{	USB_DBG_LOCK();
			VENDOR_DBGMSG("Isochronous-%s endpoint: 0x%x\r\n", ((aif->ep[i].bEndpointAddress & EP_ADDR_DIR_MASK) == EP_ADDR_DIR_IN) ? "in" : "out", aif->ep[i].bEndpointAddress);
			USB_DBG_UNLOCK();
			if((aif->ep[i].bEndpointAddress & EP_ADDR_DIR_MASK) == EP_ADDR_DIR_IN)
				sVendorDevIfCtx->ep_iso_in = &aif->ep[i];
			else
				sVendorDevIfCtx->ep_iso_out = &aif->ep[i];
		}
	}
	sVendorDevIfCtx->udev = udev;
	sVendorDevIfCtx->iface = iface;
	sVendorDevIfCtx->udevType = usbDevType;
	sVendorDevIfCtx->prevDsrSignal = 0x00;
	
	addUsbHVendorDevice(sVendorDevIfCtx, usbDevType);
	
	return 0;
}
//------------------------------------------------------------------------------
static void vendor_disconnect(IFACE_T *iface)
{	int i;
	USB_COMPOSITE_DEVICE_IF_CTX *sVendorDevIfCtx;
	SENS_UART_CTX *uartCtx;
	UART_CONFIG	 *uartCfg;
	int currCfgIdx;
	
	sVendorDevIfCtx = findDevIfCtx(iface);
	
	while(sVendorDevIfCtx)
	{	vendor_interrupt_in_stop(sVendorDevIfCtx);
		vendor_interrupt_out_stop(sVendorDevIfCtx);
		vendor_isochronous_in_stop(sVendorDevIfCtx);
		vendor_isochronous_out_stop(sVendorDevIfCtx);
	
		for(i = 0; i < iface->aif->ifd->bNumEndpoints; i++)
		{	iface->udev->hc_driver->quit_xfer(NULL, &(iface->aif->ep[i]));
		}
		if(sVendorDevIfCtx->usbCdcAtUartCfg)
		{	uartCfg = sVendorDevIfCtx->usbCdcAtUartCfg;
			delUartCfg(uartCfg);
			currCfgIdx = uartCfg->id - UART_USB_VENDOR_AT;
			gUsbUartCfgBmp &= ~(1 << currCfgIdx);
			uartCtx = uartCfg->ctx;
			SENS_UART_DE_INIT(uartCtx);
			SENS_MEM_FREE(sVendorDevIfCtx->usbCdcAtUartCfg);
		}
		if(sVendorDevIfCtx->bulkInBuf)
			SENS_MEM_FREE(sVendorDevIfCtx->bulkInBuf);
		removeUsbHVendorDevice(sVendorDevIfCtx);
		sVendorDevIfCtx = sVendorDevIfCtx->next;
	}
	//memset((void *)((uint32_t)&sVendorDev), 0, sizeof(sVendorDev));
}
//------------------------------------------------------------------------------
static UDEV_DRV_T vendor_driver =
{	vendor_probe,
	vendor_disconnect,
	NULL,                       /* suspend */
	NULL,                       /* resume */
};
//------------------------------------------------------------------------------
void vendorBulkRxCallback(void *ctx, uint8_t *pu8RData, int u8DataLen)
{	int i8Cnt;
	USB_COMPOSITE_DEVICE_IF_CTX *devIf = (USB_COMPOSITE_DEVICE_IF_CTX *)ctx;
	UART_CONFIG *uartCfg;
	
	uartCfg = devIf->usbCdcAtUartCfg;
	//uartFifoPut(1, 8+(uartCfg->id - UART_USB_VENDOR_AT), pu8RData, u8DataLen);
	uartFifoPut(1, uartCfg->id, pu8RData, u8DataLen);
	devIf->isRxReady = 1;
}
//------------------------------------------------------------------------------
void vcomStatusCallback(CDC_DEV_T *cdev, uint8_t *pu8RData, int u8DataLen)
{	int i8Cnt;
	BaseType_t xHigherPriorityTaskWoken;
	struct TaskQMsg msg;
	xHigherPriorityTaskWoken = pdFALSE;
	IFACE_T *iface;
	ALT_IFACE_T  *aif;// = iface->aif;
	DESC_IF_T    *ifd;
	CDC_NOTIFICATION *cdcNotify;
	uint16_t *value;
	uint8_t sendMsg = 0;
	
	iface = cdev->iface_cdc;
	aif = iface->aif;
	ifd = aif->ifd;
	
	cdcNotify = (CDC_NOTIFICATION *)&pu8RData[0];
	if(cdcNotify->bmRequestType == 0xA1)	//bmRequestType, PSTN
	{	if(cdcNotify->bNotification == CDC_NOTIFY_CODE_SERIAL_STATE)
		{	value = (uint16_t *)&cdcNotify[1];
			cdev->currDsrSignal = (*value >> 1) & 0x01;
			//if(cdev->currDsrSignal && (cdev->currDsrSignal != cdev->prevDsrSignal))
			if(cdev->currDsrSignal != cdev->prevDsrSignal)
			{	cdev->prevDsrSignal = cdev->currDsrSignal;
				msg.msgId = USBH_Q_MSG_SERIAL_STATE_NOTIFY;
				sendMsg = 0;
			}
			else if(*value & 0x007C)
			{	msg.msgId = USBH_Q_MSG_SERIAL_STATE_ERROR;
				sendMsg = 1;
			}
			if(sendMsg)
			{	msg.ptr = (char *)cdev;
				xQueueSendFromISR(usbHostQ, &msg, &xHigherPriorityTaskWoken);
			}
		}
		else if(cdcNotify->bNotification == CDC_NOTIFY_CODE_NETWORK_CONNECTION)
		{	//dbgMsg2("[VCOM NOTIFY] network connection %s\r\n", cdcNotify->wValue? "connected":"disconnect");
			if((cdcNotify->wValue == NETWORK_CONNECTION_CONNECTED) && (!cdev->prevNetworkIsConnect))
			{	cdev->prevNetworkIsConnect = 1;
				msg.msgId = USBH_Q_MSG_CDC_ECM_NET_LINKUP;
				msg.ptr = (char *)cdev;
				xQueueSendFromISR(usbHostQ, &msg, &xHigherPriorityTaskWoken);
			}
			else if((cdcNotify->wValue == NETWORK_CONNECTION_DISCONNECT) && (cdev->prevNetworkIsConnect))
			{	cdev->prevNetworkIsConnect = 0;
				msg.msgId = USBH_Q_MSG_CDC_ECM_NET_LINKDOWN;
				msg.ptr = (char *)cdev;
				xQueueSendFromISR(usbHostQ, &msg, &xHigherPriorityTaskWoken);
			}
		}
		else if(cdcNotify->bNotification == CDC_NOTIFY_CODE_CONNECTION_SPEED_CHANGE)
		{	uint32_t *speedBitRate = (uint32_t *)&cdcNotify[1];
			if((speedBitRate[0] != cdev->prevNetworkDlSpeed) || (cdev->prevNetworkUlSpeed != speedBitRate[1]))
			{	cdev->prevNetworkDlSpeed = speedBitRate[0];
				cdev->prevNetworkUlSpeed = speedBitRate[1];
				msg.msgId = USBH_Q_MSG_CDC_ECM_SPEED_CHANGED;
				msg.ptr = (char *)cdev;
				xQueueSendFromISR(usbHostQ, &msg, &xHigherPriorityTaskWoken);
			}
			//dbgMsg2("[VCOM NOTIFY] speed change dl:%d, ul:%d\r\n", speedBitRate[0], speedBitRate[1]);
		}
		return;
	}
	dbgMsg2("%s", "[VCOM STS] ");
	for(i8Cnt = 0; i8Cnt < u8DataLen; i8Cnt++)
		dbgMsg2("0x%02x ", pu8RData[i8Cnt]);
	dbgMsg2("%s", "\r\n");
	//0xa1 0x20 0x00 0x00 0x02 0x00 0x02 0x00 0x00 0x00 
	//debugLock();
	
	//debugUnlock();
}
//------------------------------------------------------------------------------
void vcomRxCallback(CDC_DEV_T *cdev, uint8_t *pu8RData, int u8DataLen)
{	int i8Cnt;
	(void)cdev;
	UART_CONFIG *uartCfg;
	
	uartCfg = cdev->uartCfg;
	//uartFifoPut(1, 8+(uartCfg->id - UART_USB_VENDOR_AT), pu8RData, u8DataLen);
	uartFifoPut(1, uartCfg->id, pu8RData, u8DataLen);
	cdev->isRxReady = 1;
	
	
	/*uartFifoPut(1, 5, pu8RData, u8DataLen);
	isRxReady = 1;*/
	
	/*debugLock();
	printf("[VCOM DATA %d] ", u8DataLen);
	for(i8Cnt = 0; i8Cnt < u8DataLen; i8Cnt++)
	{	//printf("0x%02x ", pu8RData[i8Cnt]);
		dbgMsg1("%c", pu8RData[i8Cnt]);
	}
	debugUnlock();*/
	//printf("\n");

	//s_i8RxReady = 1;
}
//------------------------------------------------------------------------------
void show_line_coding(LINE_CODING_T *lc)
{	/*printf("[CDC device line coding]\n");
	printf("====================================\n");
	printf("Baud rate:  %d bps\n", lc->baud);
	printf("Parity:     ");
	switch(lc->parity)
	{	case 0:		printf("None\n");			break;
		case 1:		printf("Odd\n");			break;
		case 2:		printf("Even\n");			break;
		case 3:		printf("Mark\n");			break;
		case 4:		printf("Space\n");		break;
		default:	printf("Invalid!\n");	break;
	}
	printf("Data Bits:  ");
	switch(lc->data_bits)
	{	case 5 :
		case 6 :
		case 7 :
		case 8 :
		case 16:
					printf("%d\n", lc->data_bits);
					break;
		default:
					printf("Invalid!\n");
					break;
	}
	printf("Stop Bits:  %s\n\n", (lc->stop_bits == 0) ? "1" : ((lc->stop_bits == 1) ? "1.5" : "2"));*/
	(void)lc;
}

#ifdef SUPPORT_USB_CDC_ECM
//------------------------------------------------------------------------------
int32_t usbhCdcEcmSetEthernetPacketFilter(CDC_DEV_T *cdev)
{	uint32_t   xfer_len;
	int        ret;
	uint16_t   ctrlBmp = 0;

	if(cdev == NULL)
		return USBH_ERR_INVALID_PARAM;

	ctrlBmp |= 0x1F;

	ret = usbh_ctrl_xfer(	cdev->udev,
												REQ_TYPE_OUT | REQ_TYPE_CLASS_DEV | REQ_TYPE_TO_IFACE, /* bmRequestType */
												CDC_REQUEST_CODE_SET_ETHERNET_PACKET_FILTER,    /* bRequest                              */
												ctrlBmp,                   /* wValue                                */
												cdev->iface_cdc->if_num,				/* wIndex                                */
												0,                             /* wLength                               */
												NULL,                          /* data buffer                           */
												&xfer_len, CDC_CMD_TIMEOUT);

	if(ret)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("SET_CONTROL_LINE_STATE command failed. %d\r\n", ret);
		USB_DBG_UNLOCK();
		return ret;
	}
	return ret;
}
//------------------------------------------------------------------------------
err_t cdcEcmOutput(struct netif *netif, struct pbuf *p, const ip_addr_t *ipaddr)
{	return etharp_output(netif, p, ipaddr);
}
//------------------------------------------------------------------------------
err_t cdcEcmLowLevelOutput(struct netif *netif, struct pbuf *p)
{	CDC_DEV_T *cdev = (CDC_DEV_T *)netif->state;
	struct pbuf *q;
	//int numPBufs;
	uint32_t maxFrameSize;
	uint32_t offset;
	uint32_t totXmitSize;
	uint32_t xmitSz;
	
	maxFrameSize = cdev->mtu - 14;
	for(q=p/*, numPBufs=0*/;q != NULL; q=q->next)
	{	//numPBufs++;
		if(q->len > maxFrameSize)
		{	totXmitSize = q->len;
			offset = 0;
			while(totXmitSize)
			{	if(totXmitSize > maxFrameSize)
					xmitSz = maxFrameSize;
				else
					xmitSz = totXmitSize;
				usbh_cdc_send_data(cdev, &q->payload[offset], xmitSz);
				offset += xmitSz;
				totXmitSize -= xmitSz;
			}
		}
		else
			usbh_cdc_send_data(cdev, q->payload, q->len);
	}
	return 0;
}
//------------------------------------------------------------------------------
void ecmRxCallback(CDC_DEV_T *cdev, uint8_t *pu8RData, int u8DataLen)
{	int i8Cnt;
	(void)cdev;
	struct pbuf *p;
	
	p = pbuf_alloc(PBUF_RAW, u8DataLen, PBUF_POOL);
	//p = (struct pbuf *)ecmRxPbufStorage;
	xInsideISR++;
	memcpy(p->payload, pu8RData, u8DataLen);
	if(pqEnqueue(pQueue, p) < 0)
	{	pbuf_free(p);
	}
	sys_sem_signal(&cdev->rxSema);
	xInsideISR--;
}

static void cdcEcmBulkInIrq(UTR_T *utr)
{	CDC_DEV_T   *cdev;
	int ret;

	cdev = (CDC_DEV_T *)utr->context;

	if(utr->status)
		return;

	if(cdev->rx_func)
		cdev->rx_func(cdev, utr->buff, utr->xfer_len);
	
	utr->xfer_len = 0;
	ret = usbh_bulk_xfer(utr);	//continue receive
	//free_utr(utr);
	//cdev->utr_rx = NULL;
}
//------------------------------------------------------------------------------
int32_t usbhCdcEcmPollReceiveData(CDC_DEV_T *cdev, CDC_CB_FUNC *func)
{	EP_INFO_T   *ep;
	UTR_T       *utr;
	int         ret;

	if((cdev == NULL) || (cdev->iface_data == NULL))
		return USBH_ERR_NOT_FOUND;

	if(!func)
		return USBH_ERR_INVALID_PARAM;

	ep = cdev->ep_rx;
	if(ep == NULL)
	{	ep = usbh_iface_find_ep(cdev->iface_data, 0, EP_ADDR_DIR_IN | EP_ATTR_TT_BULK);
		if(ep == NULL)
		{	USB_DBG_LOCK();
			CDC_DBGMSG("Bulk-in endpoint not found in this CDC device, cdev at 0x%X\r\n", cdev);
			USB_DBG_UNLOCK();
			return USBH_ERR_EP_NOT_FOUND;
		}
		cdev->ep_rx = ep;
	}

	utr = alloc_utr(cdev->udev);
	if(utr == NULL)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("%s", "Failed to allocated UTR!\r\n");
		USB_DBG_UNLOCK();
		return USBH_ERR_MEMORY_OUT;
	}

	utr->buff = (uint8_t *)cdev->cdcEcmRxBuf;
	utr->context = cdev;
	utr->ep = ep;
	utr->data_len = cdev->mtu;
	//utr->data_len = ep->wMaxPacketSize;
	/*if(utr->data_len > CDC_RX_BUFF_SIZE)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("Warning! CDC_RX_BUFF_SIZE %d is smaller than max. packet size %d!\r\n", CDC_RX_BUFF_SIZE, ep->wMaxPacketSize);
		USB_DBG_UNLOCK();
		utr->data_len = CDC_RX_BUFF_SIZE;
	}*/
	utr->xfer_len = 0;
	utr->func = cdcEcmBulkInIrq;

	cdev->rx_func = func;
	cdev->utr_rx = utr;
	cdev->rx_busy = 1;

	ret = usbh_bulk_xfer(utr);
	if(ret < 0)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("Error - failed to submit bulk in request (%d)\r\n", ret);
		USB_DBG_UNLOCK();
		free_utr(utr);
		cdev->utr_rx = NULL;
		cdev->rx_busy = 0;
		return ret;
	}
	return 0;
}
//------------------------------------------------------------------------------
void  cdcEcmIntInIrq(UTR_T *utr)
{	CDC_DEV_T   *cdev;
	int         ret;

	//CDC_DBGMSG("cdc_int_in_irq. %d\n", utr->xfer_len);

	cdev = (CDC_DEV_T *)utr->context;

	if(utr->status)
	{	//USB_DBG_LOCK();
		//CDC_DBGMSG("cdc_int_in_irq - has error: 0x%x\r\n", utr->status);
		//USB_DBG_UNLOCK();
		return;
	}

	if(cdev->sts_func && utr->xfer_len)
		cdev->sts_func(cdev, utr->buff, utr->xfer_len);

	free_utr(utr);
	cdev->utr_sts = NULL;
}
//------------------------------------------------------------------------------
int usbhCdcEcmGetStatus(CDC_DEV_T *cdev, CDC_CB_FUNC *func)
{	EP_INFO_T   *ep;
	UTR_T       *utr;
	int         ret;

	if((cdev == NULL) || (cdev->iface_cdc == NULL))
		return USBH_ERR_NOT_FOUND;

	if(!func || cdev->utr_sts)
		return USBH_ERR_INVALID_PARAM;

	ep = cdev->ep_sts;
	if(ep == NULL)
	{	ep = usbh_iface_find_ep(cdev->iface_cdc, 0, EP_ADDR_DIR_IN | EP_ATTR_TT_INT);
		if(ep == NULL)
		{	USB_DBG_LOCK();
			CDC_DBGMSG("%s", "Interrupt-in endpoint not found in this CDC device!\r\n");
			USB_DBG_UNLOCK();
			return USBH_ERR_EP_NOT_FOUND;
		}
		cdev->ep_sts = ep;
	}

	utr = alloc_utr(cdev->udev);
	if(utr == NULL)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("%s", "Failed to allocated UTR!\r\n");
		USB_DBG_UNLOCK();
		return USBH_ERR_MEMORY_OUT;
	}

	utr->buff = (uint8_t *)cdev->sts_buff;
	utr->context = cdev;
	utr->ep = ep;
	utr->data_len = ep->wMaxPacketSize;
	if(utr->data_len > CDC_STATUS_BUFF_SIZE)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("Warning! CDC_STATUS_BUFF_SIZE %d is smaller than max. packet size %d!\r\n", CDC_STATUS_BUFF_SIZE, ep->wMaxPacketSize);
		USB_DBG_UNLOCK();
		utr->data_len = CDC_STATUS_BUFF_SIZE;
	}
	utr->xfer_len = 0;
	utr->func = cdcEcmIntInIrq;

	cdev->utr_sts = utr;
	cdev->sts_func = func;

	ret = usbh_int_xfer(utr);
	if(ret < 0)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("Error - failed to submit interrupt read request (%d)\r\n", ret);
		USB_DBG_UNLOCK();
		free_utr(utr);
		cdev->utr_sts = NULL;
		return ret;
	}
	return 0;
}
//------------------------------------------------------------------------------
err_t cdcEcmDriverLowLevelInit(struct netif *netif)
{	CDC_DEV_T *cdev = (CDC_DEV_T *)netif->state;
	//EP_INFO_T   *ep;
	
	cdev->cdcEcmRxBuf = SENS_MEM_ZALLOC(cdev->mtu);
	netif->mtu = cdev->mtu;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
	sys_sem_new(&cdev->rxSema, 0);
	cdev->netIf = netif;
	//ep = usbh_iface_find_ep(cdev->iface_data, 0, EP_ADDR_DIR_IN | EP_ATTR_TT_BULK);
	//ecmRxPbufStorage = SENS_MEM_ZALLOC(ep->wMaxPacketSize);
	//usbh_cdc_start_polling_status(cdev, vcomStatusCallback);
	pqCreate();
	//usbhCdcEcmGetStatus(cdev, vcomStatusCallback);
	usbhCdcEcmSetEthernetPacketFilter(cdev);
	usbhCdcEcmPollReceiveData(cdev, ecmRxCallback);
	setSensminiTimer((void *)usbHostQ, USBH_Q_MSG_CDC_ECM_GET_STATUS, cdev, SENSMINI_TIMER_USB_CDC_ECM_GET_STATUS, 50, TIMER_MODE_TRIGGER);
	return 0;
}
//------------------------------------------------------------------------------
err_t cdcEcmDriverInit(struct netif *netif)
{	netif->name[0] = 'E';
	netif->name[1] = 'T';
	netif->output = cdcEcmOutput;
	netif->linkoutput = cdcEcmLowLevelOutput;
	cdcEcmDriverLowLevelInit(netif);
	return 0;
}
//------------------------------------------------------------------------------
struct netif * cdcEcmNetAdd(struct netif *netif, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw, uint8_t *macAddr, CDC_DEV_T *cdev)
{	int i;
	
	netif->hwaddr_len = 6;
	for(i=0;i<6;i++)
		netif->hwaddr[i] = macAddr[i];
	
	return netif_add(netif, ipaddr, netmask, gw, (void *)cdev, cdcEcmDriverInit, tcpip_input);
}
//------------------------------------------------------------------------------
#if LWIP_NETIF_LINK_CALLBACK
void ethNetLinkStatus(struct netif *netif)
{	struct netif *netifBack;
	
	if(netif_is_flag_set(netif, NETIF_FLAG_LINK_UP))
	{	if(netif_default == NULL)
			netif_set_default(netif);
		if(netif_is_up(netif))
		{	dbgMsg("%s", "[CDC ECM] Eth already up!!\r\n");
		}
		else
		{	netif_set_up(netif);
			dbgMsg("%s", "[CDC ECM] set Eth up!!\r\n");
		}
	}
	else
	{	netif_set_down(netif);
		dbgMsg("%s", "[CDC ECM] set Eth Down!!\r\n");

		if(netif_default && (netif_default == netif))
		{	for(netifBack = netif_list;netifBack != NULL;netifBack = netifBack->next)
			{	if(netifBack != netif)
				{	netif_set_default(netifBack);
				}
			}
		}
	}
}
#endif
//------------------------------------------------------------------------------
void ethernetInit(CDC_DEV_T *cdev)
{	ip_addr_t ipaddr, netmask, gw;
	struct netif *netif;
	
	netif = &ethernetNetif;
	
	IP4_ADDR(&ipaddr,  192, 168, 0, 100);
	IP4_ADDR(&netmask, 255, 255, 255, 0);
	IP4_ADDR(&gw,      192, 168, 0, 254);
	
	cdcEcmNetAdd(netif, &ipaddr, &netmask, &gw, cdev->macAddr, cdev);
	
#if LWIP_NETIF_LINK_CALLBACK
	netif_set_link_callback(netif, ethNetLinkStatus);
#endif
	if(netif_default == NULL)
		netif_set_default(netif);
	
	xTaskCreate( cdcEcmInputThread, "ETH_INP_TASK", USB_HOST_TASK_STACK_SIZE, netif, USB_HOST_TASK_PRIORITY, NULL );
}
#endif
//------------------------------------------------------------------------------
int initCdcDevice(CDC_DEV_T *cdev, MOBILE_INSTANCE *wlInst)
{	int i8Ret;
	LINE_CODING_T line_code;

	if(cdev->isInitialDone)
		return 0;
	
	dbgMsg("%s", "==================================\r\n");
	dbgMsg("  Init CDC device : 0x%x\r\n", (int)cdev);
	dbgMsg("  VID: 0x%x, PID: 0x%x, cdc if: %d, data if:%d\r\n", cdev->udev->descriptor.idVendor, cdev->udev->descriptor.idProduct, cdev->iface_cdc->if_num, cdev->iface_data->if_num);
	
#ifdef SUPPORT_USB_CDC_ECM
	if(cdev->cdcType == CDC_SC_ETHER)
	{	ethernetInit(cdev);
		cdev->isInitialDone = 1;
		//usbh_cdc_start_polling_status(cdev, vcomStatusCallback);
		//usbhCdcEcmSetEthernetPacketFilter(cdev);
		return 0;
	}
#endif
	
	i8Ret = usbh_cdc_get_line_coding(cdev, &line_code);
	if(i8Ret < 0)
	{	dbgMsg("Get Line Coding command failed: %d\r\n", i8Ret);
	}
	else
		show_line_coding(&line_code);

	line_code.baud = 115200;
	line_code.parity = 0;
	line_code.data_bits = 8;
	line_code.stop_bits = 0;

	i8Ret = usbh_cdc_set_line_coding(cdev, &line_code);
	if(i8Ret < 0)
	{	dbgMsg("Set Line Coding command failed: %d\r\n", i8Ret);
	}

	i8Ret = usbh_cdc_get_line_coding(cdev, &line_code);
	if(i8Ret < 0)
	{	dbgMsg("Get Line Coding command failed: %d\r\n", i8Ret);
	}
	else
	{	//printf("New line coding =>\n");
		show_line_coding(&line_code);
	}

	usbh_cdc_set_control_line_state(cdev, 1, 1);
	dbgMsg("%s", "usbh_cdc_start_polling_status...\r\n");
	usbh_cdc_start_polling_status(cdev, vcomStatusCallback);
	dbgMsg("%s", "usbh_cdc_start_to_receive_data...\r\n");
	usbh_cdc_start_to_receive_data(cdev, vcomRxCallback);
	cdev->isInitialDone = 1;
	cdev->prevDsrSignal = 3;
	
	
	usbCdcAcmSerialStateNotifyProcess(cdev, wlInst);
	return 0;
}
//------------------------------------------------------------------------------
void usbConnectFunc(UDEV_T *udev, int i8Param)
{	struct hub_dev_t *parent;
	int i8Cnt;
	struct TaskQMsg msg;
	USBH_DEVICE	*usbhDev;

	(void)i8Param;

	parent = udev->parent;
	USB_DBG_LOCK();
	VENDOR_DBGMSG("Device [0x%x,0x%x] was connected.\r\n", udev->descriptor.idVendor, udev->descriptor.idProduct);
	VENDOR_DBGMSG("    Speed:    %s-speed\r\n", (udev->speed == SPEED_HIGH) ? "high" : ((udev->speed == SPEED_FULL) ? "full" : "low"));
	VENDOR_DBGMSG("%s", "    Location: ");

	if(parent == NULL)
	{	if(udev->port_num == 1)
			VENDOR_DBGMSG("%s", "USB 2.0 port\r\n");
		else
			VENDOR_DBGMSG("%s", "USB 1.1 port\r\n");
	}
	else
	{	if(parent->pos_id[0] == '1')
			VENDOR_DBGMSG("%s", "USB 2.0 port");
		else
			VENDOR_DBGMSG("%s", "USB 1.1 port");

		for(i8Cnt = 1; parent->pos_id[i8Cnt] != 0; i8Cnt++)
		{	VENDOR_DBGMSG(" => Hub port %c", parent->pos_id[i8Cnt]);
		}

		VENDOR_DBGMSG(" => Hub port %d\r\n", udev->port_num);
	}
	USB_DBG_UNLOCK();
	
	usbhDev = findUsbhDev(udev);
	if(usbhDev)
	{	if(usbhDev->udevType == USB_DEVICE_TYPE_CDC_ACM_COMPOSITE)
		{	msg.msgId = USBH_Q_MSG_CDC_ACM_COMPOSITE_DEV_CONNECT;
		}
#ifdef SUPPORT_USB_CDC_ECM
		else if(usbhDev->udevType == USB_DEVICE_TYPE_CDC_ECM)
		{	msg.msgId = USBH_Q_MSG_CDC_ECM_DEV_CONNECT;
		}
#endif
		msg.ptr = (char *)udev;
		SENS_MSG_Q_SEND(usbHostQ, (uint32_t *)&msg, 0);
	}
}
//------------------------------------------------------------------------------
void usbDisconnectFunc(UDEV_T *udev, int i8Param)
{	(void)i8Param;
	struct TaskQMsg msg;
	
	USB_DBG_LOCK();
	VENDOR_DBGMSG("Device [0x%x,0x%x] was disconnected.\r\n", udev->descriptor.idVendor, udev->descriptor.idProduct);
	USB_DBG_UNLOCK();
	msg.msgId = USBH_Q_MSG_CDC_ACM_COMPOSITE_DEV_DISCONNECT;
	msg.ptr = (char *)udev;
	SENS_MSG_Q_SEND(usbHostQ, (uint32_t *)&msg, 0);
}
//------------------------------------------------------------------------------
void usbh_vendor_init(void)
{	//memset((void *)((uint32_t)&sVendorDev), 0, sizeof(sVendorDev));
	usbh_register_driver(&vendor_driver);
}
//------------------------------------------------------------------------------
static int32_t  usbhCompositeCdcAcmSetControlLineState(USB_COMPOSITE_DEVICE_IF_CTX *devIfCtx, int activeCarrier, int dtePesent)
{	uint32_t   xfer_len;
	int        ret;
	uint16_t   ctrlBitmap = 0;
	UDEV_T *udev;

	if(devIfCtx == NULL)
		return USBH_ERR_INVALID_PARAM;

	/*if(cdev->iface_cdc == NULL)
		return USBH_ERR_INVALID_PARAM;*/

	if(activeCarrier)
		ctrlBitmap |= 0x02;

	if(dtePesent)
		ctrlBitmap |= 0x01;

	ret = usbh_ctrl_xfer(	devIfCtx->udev,
												REQ_TYPE_OUT | REQ_TYPE_CLASS_DEV | REQ_TYPE_TO_IFACE, /* bmRequestType */
												CDC_SET_CONTROL_LINE_STATE,    /* bRequest                              */
												ctrlBitmap,                   	/* wValue                                */
												devIfCtx->iface->if_num,       /* wIndex                                */
												0,                             /* wLength                               */
												NULL,                          /* data buffer                           */
												&xfer_len, CDC_CMD_TIMEOUT);

	if(ret)
	{	USB_DBG_LOCK();
		CDC_DBGMSG("SET_CONTROL_LINE_STATE command failed. %d\r\n", ret);
		USB_DBG_UNLOCK();
		return ret;
	}
	return ret;
}
//------------------------------------------------------------------------------
int findEmptyUartCfg(void)
{	int emptyCfg = -1;
	uint8_t idx;
	
	for(idx=0;idx<MAX_UART_USB_VENDOR_AT_CNT;idx++)
	{	if((gUsbUartCfgBmp & (1 << idx)) == 0)
		{	emptyCfg = idx;
			break;
		}
	}
	return emptyCfg;
}
//------------------------------------------------------------------------------
void usbCdcAcmSerialStateNotifyProcess(CDC_DEV_T *cdev, MOBILE_INSTANCE *wlInst)
{	UDEV_T *udev;
	UART_CONFIG *uartCfg;
	uint8_t isReady = 0;
	int emptyCfg;
	MOBILE_USB_INSTANCE *mobUsbInst;
	
	dbgMsg("cdev %x, DSR bit is on\r\n", cdev);
	udev = cdev->udev;
	cdev->upperCtx = wlInst;
	mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
	
	if(cdev->uartCfg == NULL)
	{	cdev->uartCfg = (void *)SENS_MEM_ZALLOC(sizeof(UART_CONFIG));
		uartCfg = (UART_CONFIG *)cdev->uartCfg;
		if(uartCfg->ctx == NULL)
			uartCfg->ctx = SENS_MEM_ZALLOC(sizeof(SENS_UART_CTX));
		emptyCfg = findEmptyUartCfg();
		//if(emptyCfg >= 0)
		uartCfg->id = UART_USB_VENDOR_AT + emptyCfg;
		gUsbUartCfgBmp |= (1 << emptyCfg);
		uartCfg->uartType = UT_USB;
		uartCfg->bufferLength = MOBILE_UART_BUFFER_SIZE;
		addUartCfg(uartCfg);
		SENS_UART_INIT(uartCfg->ctx, uartCfg->id, uartCfg->baudrate, uartCfg->mode, MOBILE_UART_BUFFER_SIZE, UT_USB, 0);
	}
	
	if(wlInst->usbDevAt1 == NULL)
	{	wlInst->usbDev = cdev;
		wlInst->usbDevAt1 = cdev;
		SENS_SEM_LOCK(iotSys.iotSem);
		wlInst->dev = uartCfg->ctx;
		SENS_SEM_UNLOCK(iotSys.iotSem);
		if(cdev->next == NULL)
		{	mobUsbInst->usbIsMultiIfDev = 0;
			isReady = 1;
		}
		else
			mobUsbInst->usbIsMultiIfDev = 1;
	}
	else if(wlInst->usbDevAt2 == NULL && wlInst->usbDevAt1 != cdev)
	{	wlInst->usbDevAt2 = cdev;
		mobUsbInst->usbIsMultiIfDev = 1;
	}
	else if(wlInst->usbDevNmea == NULL && wlInst->usbDevAt1 != cdev && wlInst->usbDevAt2 != cdev)
	{	wlInst->usbDevNmea = cdev;
		mobUsbInst->usbIsMultiIfDev = 1;
		isReady = 1;
	}
	mobUsbInst->usbIsCdcAcm = 1;
	wlInst->isNotSupportCmuxMode = 1;
	mobUsbInst->isUsbHost = isReady;
}
//------------------------------------------------------------------------------
void usbCdcAcmCompositeDevConnectProcess(UDEV_T *udev, MOBILE_INSTANCE *wlInst)
{	USB_COMPOSITE_DEVICE_IF_CTX *devIfCtx;
	uint8_t usbIsMultiIfDev = 0;
	USBH_DEVICE	*usbhDev = findUsbhDev(udev);
	IFACE_T *iface;
	ALT_IFACE_T *aif;// = iface->aif;
	DESC_IF_T *ifd;
	UART_CONFIG *uartCfg;
	int emptyCfg;
	MOBILE_USB_INSTANCE *mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
				
	devIfCtx = (USB_COMPOSITE_DEVICE_IF_CTX *)usbhDev->udevIfCtx;
	while(devIfCtx)
	{	iface = devIfCtx->iface;
		aif = iface->aif;
		ifd = aif->ifd;
		if(((udev->descriptor.idVendor == TELIT_VID) && ((udev->descriptor.idProduct == TELIT_ME310_PID) || (udev->descriptor.idProduct == TELIT_LE910_PID) || (udev->descriptor.idProduct == TELIT_LE910C4_WWX_PID_0x1031)
#ifdef LE910_USE_CDC_ECM
			|| (udev->descriptor.idProduct == TELIT_LE910_PID_0x1206)
#endif
				)) 
			||((udev->descriptor.idVendor == UBLOX_R410_VID) && (udev->descriptor.idProduct == UBLOX_R410_PID)))
		{	if(((udev->descriptor.idProduct == TELIT_ME310_PID) && ((ifd->bInterfaceNumber == 1) || (ifd->bInterfaceNumber == 2))) 
				||((udev->descriptor.idProduct == TELIT_LE910_PID) && ((ifd->bInterfaceNumber == LE910_1201_NMEA_IF) || (ifd->bInterfaceNumber == LE910_1201_MODEM1_IF) || (ifd->bInterfaceNumber == LE910_1201_MODEM2_IF)))
				||((udev->descriptor.idProduct == TELIT_LE910C4_WWX_PID_0x1031) && ((ifd->bInterfaceNumber == LE910C4_WWX_1031_MODEM1_IF) || (ifd->bInterfaceNumber == LE910C4_WWX_1031_MODEM2_IF)))
#ifdef LE910_USE_CDC_ECM
				||((udev->descriptor.idProduct == TELIT_LE910_PID_0x1206) && ((ifd->bInterfaceNumber == LE910_1206_NMEA_IF) || (ifd->bInterfaceNumber == LE910_1206_MODEM1_IF)))
#endif
				||((udev->descriptor.idProduct == UBLOX_R410_PID) && (ifd->bInterfaceNumber == 2)/*((ifd->bInterfaceNumber == 0) || (ifd->bInterfaceNumber == 2))*/))
			{	if(devIfCtx->usbCdcAtUartCfg == NULL)
				{	devIfCtx->usbCdcAtUartCfg = (UART_CONFIG *)SENS_MEM_ZALLOC(sizeof(UART_CONFIG));
					uartCfg = devIfCtx->usbCdcAtUartCfg;
					if(uartCfg->ctx == NULL)
						uartCfg->ctx = SENS_MEM_ZALLOC(sizeof(SENS_UART_CTX));
					emptyCfg = findEmptyUartCfg();
					uartCfg->id = UART_USB_VENDOR_AT + emptyCfg;
					gUsbUartCfgBmp |= (1 << emptyCfg);
					uartCfg->uartType = UT_USB;
					uartCfg->bufferLength = MOBILE_UART_BUFFER_SIZE;
					addUartCfg(uartCfg);
					SENS_UART_INIT(uartCfg->ctx, uartCfg->id, uartCfg->baudrate, uartCfg->mode, MOBILE_UART_BUFFER_SIZE, UT_USB, 0);
				}
				if(udev->descriptor.idProduct == TELIT_ME310_PID)
				{	usbIsMultiIfDev = 1;
					if(ifd->bInterfaceNumber == 1)
					{	wlInst->usbDev = devIfCtx;
						wlInst->usbDevAt1 = devIfCtx;
						SENS_SEM_LOCK(iotSys.iotSem);
						wlInst->dev = uartCfg->ctx;
						SENS_SEM_UNLOCK(iotSys.iotSem);
					}
					else
						wlInst->usbDevAt2 = devIfCtx;
				}
				else if(udev->descriptor.idProduct == TELIT_LE910_PID)
				{	usbIsMultiIfDev = 1;
					if(ifd->bInterfaceNumber == LE910_1201_MODEM1_IF)
					{	wlInst->usbDev = devIfCtx;
						wlInst->usbDevAt1 = devIfCtx;
						SENS_SEM_LOCK(iotSys.iotSem);
						wlInst->dev = uartCfg->ctx;
						SENS_SEM_UNLOCK(iotSys.iotSem);
					}
					else if(ifd->bInterfaceNumber == LE910_1201_MODEM2_IF)
						wlInst->usbDevAt2 = devIfCtx;
					else
						wlInst->usbDevNmea = devIfCtx;
					usbhCompositeCdcAcmSetControlLineState(devIfCtx, 1, 1);
				}
				else if(udev->descriptor.idProduct == TELIT_LE910C4_WWX_PID_0x1031)
				{	usbIsMultiIfDev = 1;
					if(ifd->bInterfaceNumber == LE910C4_WWX_1031_MODEM1_IF)
					{	wlInst->usbDev = devIfCtx;
						wlInst->usbDevAt1 = devIfCtx;
						SENS_SEM_LOCK(iotSys.iotSem);
						wlInst->dev = uartCfg->ctx;
						SENS_SEM_UNLOCK(iotSys.iotSem);
					}
					else if(ifd->bInterfaceNumber == LE910C4_WWX_1031_MODEM2_IF)
						wlInst->usbDevAt2 = devIfCtx;
					usbhCompositeCdcAcmSetControlLineState(devIfCtx, 1, 1);
				}
#ifdef LE910_USE_CDC_ECM
				else if(udev->descriptor.idProduct == TELIT_LE910_PID_0x1206)
				{	usbIsMultiIfDev = 1;
					if(ifd->bInterfaceNumber == LE910_1206_MODEM1_IF)
					{	wlInst->usbDev = devIfCtx;
						wlInst->usbDevAt1 = devIfCtx;
						SENS_SEM_LOCK(mobileSys.mobileSem);
						wlInst->dev = uartCfg->ctx;
						SENS_SEM_UNLOCK(mobileSys.mobileSem);
					}
					/*else if(ifd->bInterfaceNumber == LE910_1201_MODEM2_IF)
						wlInst->usbDevAt2 = devIfCtx;*/
					else
						wlInst->usbDevNmea = devIfCtx;
					usbhCompositeCdcAcmSetControlLineState(devIfCtx, 1, 1);
				}
#endif
				else if(udev->descriptor.idProduct == UBLOX_R410_PID)
				{	wlInst->usbDev = devIfCtx;
					wlInst->usbDevAt1 = devIfCtx;
					SENS_SEM_LOCK(iotSys.iotSem);
					wlInst->dev = uartCfg->ctx;
					SENS_SEM_UNLOCK(iotSys.iotSem);
					usbhCompositeCdcAcmSetControlLineState(devIfCtx, 1, 1);
				}
				devIfCtx->isAtIf = 1;
				usbhCdcStartPollingStatus(devIfCtx, cdcCompositeDevNotify);
				usbhVendorStartToReceiveBulkInData(devIfCtx, vendorBulkRxCallback);
			}
		}
		devIfCtx = devIfCtx->next;
	}
	mobUsbInst->usbIsCdcAcm = 0;
	mobUsbInst->usbIsMultiIfDev = usbIsMultiIfDev;
	wlInst->isNotSupportCmuxMode = 1;
	mobUsbInst->isUsbHost = 1;
}
//------------------------------------------------------------------------------
MOBILE_INSTANCE *getWlInst(void)
{	MOBILE_INSTANCE *wlInst = NULL;
	NET_INSTANCE	*workingInst;
	
	if(networkCtx->workingInst)
	{	workingInst = networkCtx->workingInst;
		wlInst = workingInst->wlInst;
	}
	return wlInst;
}
//------------------------------------------------------------------------------
//			
//------------------------------------------------------------------------------
void usbHostTask(taskArg param)
{	MOBILE_INSTANCE *wlInst = NULL;
	CDC_DEV_T	*cdev;
	UART_CONFIG	*uartCfg;
	uint8_t initial = 0;
	struct TaskQMsg msg;
	BaseType_t result;
	UDEV_T *udev;
	USB_COMPOSITE_DEVICE_IF_CTX *devIfCtx;
	
	UVC_DEV_T       *vdev;
	//int             t_last = 0, cnt_last = 0;
	//int writeJpg2File = 0;
	
	SENS_MSG_Q_INIT(usbHostQ, USB_HOST_NUM_MESSAGES, SENS_TASKQ_GRANM);
	SENS_SEM_LOCK(taskAct.usbHostTaskAct.lock);
	
	usbh_core_init();
	usbh_uvc_init();
	usbh_cdc_init();
	usbh_vendor_init();
	usbh_memory_used();
	
	usbh_install_conn_callback(usbConnectFunc, usbDisconnectFunc);
	//SET_HSUSB_VBUS_EN_PB10();
	//SET_HSUSB_VBUS_ST_PB11();
	//PIN_USB_HS_VBUS_EN = 1;
	
	while(1)
	{	if(usbh_pooling_hubs())
		{	dbgMsg("%s", "USB Hub change!!\r\n");
			vdev = usbh_uvc_get_device_list();
			if((vdev == NULL) && g_vdev)
			{	g_vdev = NULL;
			}
			else if(vdev == NULL)
			{	g_vdev = NULL;
				_total_frame_count = 0;
				//SENS_TIME_DELAY(1);
				//continue;
			}
			else if(vdev != g_vdev)
			{	msg.msgId = SENSOR_Q_MSG_CAMERA_START;
				msg.ptr = (void *)vdev;
				if(sendMsgWithErrHandle(SENSORS_TASK, SENS_MSG_Q_SEND(sensorQ, (uint32_t *)&msg, 0), __func__, __LINE__))
				{
				}
				g_vdev = vdev;
			}
			cdev = usbh_cdc_get_device_list();
			if(cdev)
			{	if(wlInst == NULL)
					wlInst = getWlInst();
				while(cdev != NULL)
				{	initCdcDevice(cdev, wlInst);
					if(cdev != NULL)
						cdev = cdev->next;
				}
			}
			else
			{	//goto CHECK_USBH_Q;
			}
		}
CHECK_USBH_Q:
		result = xQueueReceive(usbHostQ, &msg, 0);
		if(result == pdPASS)
		{	/*if(msg.msgId == USBH_Q_MSG_SERIAL_STATE_NOTIFY)
				usbCdcAcmSerialStateNotifyProcess((CDC_DEV_T *)msg.ptr, wlInst);
			else */if(msg.msgId == USBH_Q_MSG_CDC_ACM_COMPOSITE_DEV_CONNECT)
			{	if(wlInst == NULL)
					wlInst = getWlInst();
				usbCdcAcmCompositeDevConnectProcess((UDEV_T *)msg.ptr, wlInst);
			}
			else if(msg.msgId == USBH_Q_MSG_SET_WLINST)
			{	wlInst = getWlInst();
			}
			else if(msg.msgId == USBH_Q_MSG_CDC_ACM_COMPOSITE_DEV_DISCONNECT)
			{	//usbCdcAcmCompositeDevDisconnectProcess((UDEV_T *)msg.ptr, wlInst);
				wlInst = NULL;
			}
#if 0
			else if(msg.msgId == USBH_Q_MSG_CDC_ECM_DEV_CONNECT)
			{	usbCdcEcmDevConnectProcess((UDEV_T *)msg.ptr);
			}
#endif
#ifdef SUPPORT_USB_CDC_ECM
			else if(msg.msgId == USBH_Q_MSG_CDC_ECM_GET_STATUS)
			{	usbhCdcEcmGetStatus((CDC_DEV_T *)msg.ptr, vcomStatusCallback);
				setSensminiTimer((void *)usbHostQ, USBH_Q_MSG_CDC_ECM_GET_STATUS, msg.ptr, SENSMINI_TIMER_USB_CDC_ECM_GET_STATUS, 50, TIMER_MODE_TRIGGER);
			}
			else if(msg.msgId == USBH_Q_MSG_CDC_ECM_NET_LINKUP)
			{	CDC_DEV_T *cdcEcmDev = (CDC_DEV_T *)msg.ptr;
				struct netif *netif = (struct netif *)cdcEcmDev->netIf;
				if(netif)
				{	netif_set_link_up(netif);
					dbgMsg("%s", "[CDC ECM] Ethernet Link UP\r\n");
				}
			}
			else if(msg.msgId == USBH_Q_MSG_CDC_ECM_NET_LINKDOWN)
			{	CDC_DEV_T *cdcEcmDev = (CDC_DEV_T *)msg.ptr;
				struct netif *netif = (struct netif *)cdcEcmDev->netIf;
				if(netif)
				{	netif_set_link_down(netif);
					dbgMsg("%s", "[CDC ECM] Ethernet Link DOWN\r\n");
				}
			}
			else if(msg.msgId == USBH_Q_MSG_CDC_ECM_SPEED_CHANGED)
			{	CDC_DEV_T *cdcEcmDev = (CDC_DEV_T *)msg.ptr;
				dbgMsg("[CDC ECM] speed change dl:%d, ul:%d\r\n", cdcEcmDev->prevNetworkDlSpeed, cdcEcmDev->prevNetworkUlSpeed);
			}
#endif
		}
		SENS_TIME_DELAY(1);
	}
}


/*
 *	UVC descriptor
 *	[Device Descriptor]
----------------------------------------------
  Length              = 18
  DescriptorType      = 0x01
  USB version         = 2.00
  Vendor:Product      = 0bda:3035
  MaxPacketSize0      = 64
  NumConfigurations   = 1
  Device version      = 0.01
  Device Class:SubClass:Protocol = ef:02:01
[Configuration Descriptor]
----------------------------------------------
  Length              =  9
  DescriptorType      = 02
  wTotalLength        = 990
  bNumInterfaces      = 2
  bConfigurationValue = 1
  iConfiguration      = 4
  bmAttributes        = 0x80
  MaxPower            = 250

!![Unknown Descriptor]
----------------------------------------------
Length              =  8
DescriptorType      = 0b

    [Interface Descriptor]
    ----------------------------------------------
      Length              =  9
      DescriptorType      = 04
      bInterfaceNumber    = 0
      bAlternateSetting   = 0
      bNumEndpoints       = 1
      bInterfaceClass     = 0x0e
      bInterfaceSubClass  = 0x01
      bInterfaceProtocol  = 0x00
      iInterface          = 5

!![Unknown Descriptor]
----------------------------------------------
Length              = 13
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 18
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 11
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              =  9
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 27
DescriptorType      = 24

        [Endpoint Descriptor]
        ----------------------------------------------
          Length              =  7
          DescriptorType      = 05
          bEndpointAddress    = 0x83
          bmAttributes        = 0x03
          wMaxPacketSize      = 16
          bInterval           = 6
          bRefresh            = 5
          bSynchAddress       = 37

!![Unknown Descriptor]
----------------------------------------------
Length              =  5
DescriptorType      = 25

    [Interface Descriptor]
    ----------------------------------------------
      Length              =  9
      DescriptorType      = 04
      bInterfaceNumber    = 1
      bAlternateSetting   = 0
      bNumEndpoints       = 0
      bInterfaceClass     = 0x0e
      bInterfaceSubClass  = 0x02
      bInterfaceProtocol  = 0x00
      iInterface          = 0

!![Unknown Descriptor]
----------------------------------------------
Length              = 15
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 11
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 46
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 50
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              =  6
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 27
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 30
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              = 22
DescriptorType      = 24

!![Unknown Descriptor]
----------------------------------------------
Length              =  6
DescriptorType      = 24

    [Interface Descriptor]
    ----------------------------------------------
      Length              =  9
      DescriptorType      = 04
      bInterfaceNumber    = 1
      bAlternateSetting   = 1
      bNumEndpoints       = 1
      bInterfaceClass     = 0x0e
      bInterfaceSubClass  = 0x02
      bInterfaceProtocol  = 0x00
      iInterface          = 0

        [Endpoint Descriptor]
        ----------------------------------------------
          Length              =  7
          DescriptorType      = 05
          bEndpointAddress    = 0x81
          bmAttributes        = 0x05
          wMaxPacketSize      = 128
          bInterval           = 1
          bRefresh            = 9
          bSynchAddress       = 4

    [Interface Descriptor]
    ----------------------------------------------
      Length              =  9
      DescriptorType      = 04
      bInterfaceNumber    = 1
      bAlternateSetting   = 2
      bNumEndpoints       = 1
      bInterfaceClass     = 0x0e
      bInterfaceSubClass  = 0x02
      bInterfaceProtocol  = 0x00
      iInterface          = 0

        [Endpoint Descriptor]
        ----------------------------------------------
          Length              =  7
          DescriptorType      = 05
          bEndpointAddress    = 0x81
          bmAttributes        = 0x05
          wMaxPacketSize      = 512
          bInterval           = 1
          bRefresh            = 9
          bSynchAddress       = 4

    [Interface Descriptor]
    ----------------------------------------------
      Length              =  9
      DescriptorType      = 04
      bInterfaceNumber    = 1
      bAlternateSetting   = 3
      bNumEndpoints       = 1
      bInterfaceClass     = 0x0e
      bInterfaceSubClass  = 0x02
      bInterfaceProtocol  = 0x00
      iInterface          = 0

        [Endpoint Descriptor]
        ----------------------------------------------
          Length              =  7
          DescriptorType      = 05
          bEndpointAddress    = 0x81
          bmAttributes        = 0x05
          wMaxPacketSize      = 1024
          bInterval           = 1
          bRefresh            = 9
          bSynchAddress       = 4

    [Interface Descriptor]
    ----------------------------------------------
      Length              =  9
      DescriptorType      = 04
      bInterfaceNumber    = 1
      bAlternateSetting   = 4
      bNumEndpoints       = 1
      bInterfaceClass     = 0x0e
      bInterfaceSubClass  = 0x02
      bInterfaceProtocol  = 0x00
      iInterface          = 0

        [Endpoint Descriptor]
        ----------------------------------------------
          Length              =  7
          DescriptorType      = 05
          bEndpointAddress    = 0x81
          bmAttributes        = 0x05
          wMaxPacketSize      = 2816
          bInterval           = 1
          bRefresh            = 9
          bSynchAddress       = 4

    [Interface Descriptor]
    ----------------------------------------------
      Length              =  9
      DescriptorType      = 04
      bInterfaceNumber    = 1
      bAlternateSetting   = 5
      bNumEndpoints       = 1
      bInterfaceClass     = 0x0e
      bInterfaceSubClass  = 0x02
      bInterfaceProtocol  = 0x00
      iInterface          = 0

        [Endpoint Descriptor]
        ----------------------------------------------
          Length              =  7
          DescriptorType      = 05
          bEndpointAddress    = 0x81
          bmAttributes        = 0x05
          wMaxPacketSize      = 3072
          bInterval           = 1
          bRefresh            = 9
          bSynchAddress       = 4

    [Interface Descriptor]
    ----------------------------------------------
      Length              =  9
      DescriptorType      = 04
      bInterfaceNumber    = 1
      bAlternateSetting   = 6
      bNumEndpoints       = 1
      bInterfaceClass     = 0x0e
      bInterfaceSubClass  = 0x02
      bInterfaceProtocol  = 0x00
      iInterface          = 0

        [Endpoint Descriptor]
        ----------------------------------------------
          Length              =  7
          DescriptorType      = 05
          bEndpointAddress    = 0x81
          bmAttributes        = 0x05
          wMaxPacketSize      = 4992
          bInterval           = 1
          bRefresh            = 9
          bSynchAddress       = 4

    [Interface Descriptor]
    ----------------------------------------------
      Length              =  9
      DescriptorType      = 04
      bInterfaceNumber    = 1
      bAlternateSetting   = 7
      bNumEndpoints       = 1
      bInterfaceClass     = 0x0e
      bInterfaceSubClass  = 0x02
      bInterfaceProtocol  = 0x00
      iInterface          = 0

        [Endpoint Descriptor]
        ----------------------------------------------
          Length              =  7
          DescriptorType      = 05
          bEndpointAddress    = 0x81
          bmAttributes        = 0x05
          wMaxPacketSize      = 5120
          bInterval           = 1
          bRefresh            = 0
          bSynchAddress       = 0
ignore descriptor 0xB 8
uvc_probe - device (vid=0xbda, pid=0x3035), control interface 0.
UVC parsing video control interface 0...
VC Interface VC_HEADER
    bcdUVC: 0100
    baInterfaceNr: 01
VC Interface VC_INPUT_TERMINAL
    bTerminalID:    0x1
    wTerminalType:  0x201
    bAssocTerminal: 0x0
VC Interface VC_PROCESSING_UNIT\rn    bUnitID:          0x2
    wMaxMultiplier:   0x0
    bControlSize:     0x2
    bmControls:       0x00177f
    bmVideoStandards: 0x24
VC Interface VC_OUTPUT_TERMINAL
    bTerminalID:    0x3
    wTerminalType:  0x101
    bAssocTerminal: 0x0
    bSourceID:      0x4
UVC USB OT found.
VC Interface VC_EXTENSION_UNIT
    bUnitID:      0x4
    bNumControls: 0x2
    bNrInPins:    0x1
uvc_probe - device (vid=0xbda, pid=0x3035), streaming interface 1.
UVC parsing video streaming interface 1...
VS Interface VS_INPUT_HEADER
    bNumFormats:      0x2
    bEndpointAddress: 0x81
    bTerminalLink:    0x3
VS Interface VS_FORMAT_MJPEG
    bFormatIndex:         0x1
    bNumFrameDescriptors: 0xc
    bAspectRatioX:        0x0
    bAspectRatioY:        0x0
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0x1
    wWidth:       0x780	//1920
    wHeight       0x438	//1080
    dwMinBitRate: 0x31704000
    dwMaxBitRate: 0x31704000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0x2
    wWidth:       0xa0	//160
    wHeight       0x78	//120
    dwMinBitRate: 0x753000
    dwMaxBitRate: 0x753000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0x3
    wWidth:       0x140	//320
    wHeight       0xf0	//240
    dwMinBitRate: 0x1d4c000
    dwMaxBitRate: 0x1d4c000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0x4
    wWidth:       0x160	//352
    wHeight       0x120	//288
    dwMinBitRate: 0x26ac000
    dwMaxBitRate: 0x26ac000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0x5
    wWidth:       0x280	//640
    wHeight       0x1e0	//480
    dwMinBitRate: 0x7530000
    dwMaxBitRate: 0x7530000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0x6
    wWidth:       0x320	//800
    wHeight       0x258	//600
    dwMinBitRate: 0xb71b000
    dwMaxBitRate: 0xb71b000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0x7
    wWidth:       0x400	//1024
    wHeight       0x300	//768
    dwMinBitRate: 0x12c00000
    dwMaxBitRate: 0x12c00000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0x8
    wWidth:       0x500	//1280
    wHeight       0x2d0	//720
    dwMinBitRate: 0x15f90000
    dwMaxBitRate: 0x15f90000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0x9
    wWidth:       0x500	//1280
    wHeight       0x3c0	//960
    dwMinBitRate: 0x1d4c0000
    dwMaxBitRate: 0x1d4c0000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0xa
    wWidth:       0xa20	//2592
    wHeight       0x798	//1944
    dwMinBitRate: 0x99c6000
    dwMaxBitRate: 0x601bc000
    dwDefaultFrameInterval:  0x7a120
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0xb
    wWidth:       0x800	//2048
    wHeight       0x600	//1536
    dwMinBitRate: 0x3c000000
    dwMaxBitRate: 0x3c000000
    dwDefaultFrameInterval:  0x7a120
VS Interface VS_FRAME_MJPEG
    bFormatIndex: 0xc
    wWidth:       0x780	//1920
    wHeight       0x438	//1080
    dwMinBitRate: 0x31704000
    dwMaxBitRate: 0x31704000
    dwDefaultFrameInterval:  0x61a80
Unsupported VS class interface descriptor 0xd! Skip.
VS Interface VS_FORMAT_UNCOMPRESSED
    bFormatIndex:         0x2
    bNumFrameDescriptors: 0x8
    guidFormat:           0x3259555900100000aa000080719b3800
    bBitsPerPixel         0x10
    bDefaultFrameIndex:   0x1
    bAspectRatioX:        0x0
    bAspectRatioY:        0x0
VS Interface VS_FRAME_UNCOMPRESSED
    bFormatIndex: 0x1
    wWidth:       0x280
    wHeight       0x1e0
    dwMinBitRate: 0x7530000
    dwMaxBitRate: 0x7530000
    dwDefaultFrameInterval:  0x61a80
VS Interface VS_FRAME_UNCOMPRESSED
    bFormatIndex: 0x2
    wWidth:       0x780
    wHeight       0x438
    dwMinBitRate: 0x9e34000
    dwMaxBitRate: 0x9e34000
    dwDefaultFrameInterval:  0x1e8480
VS Interface VS_FRAME_UNCOMPRESSED
    bFormatIndex: 0x3
    wWidth:       0x500
    wHeight       0x2d0
    dwMinBitRate: 0x8ca0000
    dwMaxBitRate: 0x8ca0000
    dwDefaultFrameInterval:  0xf4240
VS Interface VS_FRAME_UNCOMPRESSED
    bFormatIndex: 0x4
    wWidth:       0x320
    wHeight       0x258
    dwMinBitRate: 0x927c000
    dwMaxBitRate: 0x927c000
    dwDefaultFrameInterval:  0x7a120
VS Interface VS_FRAME_UNCOMPRESSED
    bFormatIndex: 0x5
    wWidth:       0x3c0
    wHeight       0x21c
    dwMinBitRate: 0x9e34000
    dwMaxBitRate: 0x9e34000
    dwDefaultFrameInterval:  0x7a120
VS Interface VS_FRAME_UNCOMPRESSED
    bFormatIndex: 0x6
    wWidth:       0xa20
    wHeight       0x798
    dwMinBitRate: 0x99c6000
    dwMaxBitRate: 0x99c6000
    dwDefaultFrameInterval:  0x4c4b40
VS Interface VS_FRAME_UNCOMPRESSED
    bFormatIndex: 0x7
    wWidth:       0x500
    wHeight       0x3c0
    dwMinBitRate: 0x9600000
    dwMaxBitRate: 0x9600000
    dwDefaultFrameInterval:  0x1312d0
VS Interface VS_FRAME_UNCOMPRESSED
    bFormatIndex: 0x8
    wWidth:       0x800
    wHeight       0x600
    dwMinBitRate: 0x9000000
    dwMaxBitRate: 0x9000000
    dwDefaultFrameInterval:  0x32dcd5
Unsupported VS class interface descriptor 0xd! Skip.
Parsing VS if 1, alt 1...
  Endpoint wMaxPacketSize = 128
Parsing VS if 1, alt 2...
  Endpoint wMaxPacketSize = 512
Parsing VS if 1, alt 3...
  Endpoint wMaxPacketSize = 1024
Parsing VS if 1, alt 4...
  Endpoint wMaxPacketSize = 2816
Parsing VS if 1, alt 5...
  Endpoint wMaxPacketSize = 3072
Parsing VS if 1, alt 6...
  Endpoint wMaxPacketSize = 4992
Parsing VS if 1, alt 7...
  Endpoint wMaxPacketSize = 5120


----------------------------------------------------------
[Video Streaming interface parsing result dump]
  Alt 1, wMaxPacketSize = 128
  Alt 2, wMaxPacketSize = 512
  Alt 3, wMaxPacketSize = 1024
  Alt 4, wMaxPacketSize = 1536
  Alt 5, wMaxPacketSize = 2048
  Alt 6, wMaxPacketSize = 2688
  Alt 7, wMaxPacketSize = 3072

 */
