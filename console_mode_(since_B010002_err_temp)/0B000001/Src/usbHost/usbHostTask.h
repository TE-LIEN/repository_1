#ifndef __USB_HOST_TASK_H__
#define __USB_HOST_TASK_H__

#include "sensminiCfg.h"
#include "sensCommon.h"
#include "usbh_lib.h"
#include "usbh_uvc.h"
#include "usbh_cdc.h"
#include "hub.h"

#define VENDOR_DEBUG

#ifdef VENDOR_DEBUG
#define VENDOR_DBGMSG	dbgMsg2
#else
#define VENDOR_DEBUG(...)
#endif

#define REQ_SET_DATA                   0x01
#define REQ_GET_DATA                   0x12

#define ISO_UTR_NUM       2

#define USB_DEVICE_TYPE_CDC_ACM						0
#define USB_DEVICE_TYPE_CDC_ACM_COMPOSITE	1
#define USB_DEVICE_TYPE_CDC_ECM						2
#define USB_DEVICE_TYPE_CDC_ECM_COMPOSITE	3

typedef int (INT_CB_FUNC)(int status, uint8_t *rdata, int data_len);
typedef int (ISO_CB_FUNC)(uint8_t *rdata, int data_len);
typedef void (VENDOR_BULK_CB_FUNC)(void *ctx, uint8_t *rData, int dataLen);
typedef void (CDC_NOTIFY_CB_FUNC)(void *ctx, int sts, uint8_t *rdata, int data_len);


typedef struct _USB_COMPOSITE_DEVICE_IF_CTX
{	struct _USB_COMPOSITE_DEVICE_IF_CTX	*next;
	struct _USB_COMPOSITE_DEVICE_IF_CTX *prev;
	UDEV_T       *udev;
	IFACE_T      *iface;
	EP_INFO_T    *ep_bulk_in;
	EP_INFO_T    *ep_bulk_out;
	EP_INFO_T    *ep_int_in;
	EP_INFO_T    *ep_int_out;
	EP_INFO_T    *ep_iso_in;
	EP_INFO_T    *ep_iso_out;
	//INT_CB_FUNC  *int_in_func;
	//INT_CB_FUNC  *int_out_func;
	ISO_CB_FUNC  		*iso_in_func;
	ISO_CB_FUNC  		*iso_out_func;
	CDC_NOTIFY_CB_FUNC	*intInCbFunc;
	CDC_NOTIFY_CB_FUNC	*intOutCbFunc;
	VENDOR_BULK_CB_FUNC	*bulkInCbFunc;
	UTR_T        		*utr_int_in[2];
	uint8_t      		buff_int_in[2][512];
	UTR_T        		*utr_int_out[2];
	uint8_t      		buff_int_out[2][512];
	UTR_T        		*utr_iso_in[ISO_UTR_NUM];
	UTR_T        		*utr_iso_out[ISO_UTR_NUM];
	void				*upperCtx;
	UART_CONFIG	 		*usbCdcAtUartCfg;
	uint32_t		 	*bulkInBuf;
	uint8_t				bulkInBusy;
	//UTR_T					*utrInt;
	UTR_T         		*utr_rx;
	uint8_t				isRxReady;
	uint8_t				isAtIf;
	CDC_DEV_T 			*cdev;
	uint32_t			udevType;
	uint8_t				prevDsrSignal;
	uint8_t				currDsrSignal;
}USB_COMPOSITE_DEVICE_IF_CTX;

typedef struct _USBH_DEVICE
{	struct _USBH_DEVICE	*next;
	struct _USBH_DEVICE	*prev;
	UDEV_T       		*udev;
	uint32_t			udevType;
	void				*udevIfCtx;	//cdc / vendor	//uvc
}USBH_DEVICE;

#define USB_HOST_NUM_MESSAGES	32	//max queue size


//UBLOX L210 16.19 VID:0x1546		PID:0x1146, only 1 interface
//UBLOX L210 15.69 VID:0x1546		PID:0x1141
//UBLOX R410 VID:	0x05C6	PID: 0x90B2
//Telit ME310 VID:0x1BC7 PID:0x110A
	//ME310 if 0, Diagnostics, class 0xFF, sub calss 0xFF, protocol 0xFF
	//ME310 if 1, AT, class 0xFF, sub calss 0xFF, protocol 0xFF
	//ME310 if 2, AT, class 0xFF, sub calss 0xFE, protocol 0xFF
	//ME310 if 3, NA, class 0xFF, sub calss 0xFF, protocol 0xFF
//Telit LE910 VID:0x1BC7 PID:0x1201
	//LE910 if 0, Diagnostics, class 0xFF, sub class 0xFF, protocol 0xFF
	//LE910 if 1, 
	//LE910 if 2
	//LE910 if 3, NMEA, class 0xff, sub class 0x00, protocol 0x00
	//LE910 if 4, AT, class 0xff, sub class 0x00, protocol 0x00
	//LE910 if 5, AT, class 0xff, sub class 0x00, protocol 0x00
	//LE910 if 6, SAP, class 0xff, sub class 0x00, protocol 0x00

#define TELIT_VID						0x1BC7
#define TELIT_ME310_PID					0x110A	// if 1 & 2 is AT
#define TELIT_LE910_PID					0x1201	// if 4 & 5
#define TELIT_LE910_PID_0x1203			0x1203
#define TELIT_LE910_PID_0x1206			0x1206
#define TELIT_LE910C4_WWX_PID_0x1031	0x1031

	#define LE910_1201_DIAG_IF			0
	#define LE910_1201_ADB_IF			1
	#define LE910_1201_RMNET_IF			2
	#define LE910_1201_NMEA_IF			3
	#define LE910_1201_MODEM1_IF		4
	#define LE910_1201_MODEM2_IF		5
	#define LE910_1201_SAP_IF			6
	
	#define LE910_1203_RNDIS_IF			0
	#define LE910_1203_DIAG_IF			1
	#define LE910_1203_ADB_IF			2
	#define LE910_1203_NMEA_IF			3
	#define LE910_1203_MODEM1_IF		4
	#define LE910_1203_MODEM2_IF		5
	#define LE910_1203_SAP_IF			6
	
	#define LE910_1206_DIAG_IF			0
	#define LE910_1206_ADB_IF			1
	#define LE910_1206_ECM_IF			2
	#define LE910_1206_NMEA_IF			3
	#define LE910_1206_MODEM1_IF		4
	#define LE910_1206_MODEM2_IF		5
	#define LE910_1206_SAP_IF			6
	
	#define LE910C4_WWX_1031_DIAG_IF	0
	#define LE910C4_WWX_1031_MODEM1_IF	1
	#define LE910C4_WWX_1031_MODEM2_IF	2
	#define LE910C4_WWX_1031_RMNET_IF	3

#define UBLOX_L210_VID	0x1546
#define UBLOX_L210_PID1	0x1141
#define UBLOX_L210_PID2	0x1146

#define UBLOX_R410_VID	0x05C6
#define UBLOX_R410_PID	0x90B2

#define ASIX_VID			0x0B95	//USB ETH Bridge
#define ASIX_AX88179_PID	0x1790

#define REALTEK_VID			0x0BDA
#define RTL8153_PID			0x8153

#define CDC_NOTIFY_CODE_NETWORK_CONNECTION			0x00
	#define NETWORK_CONNECTION_CONNECTED			1
	#define NETWORK_CONNECTION_DISCONNECT			0
#define CDC_NOTIFY_CODE_RESPONSE_AVAILABLE			0x01
#define CDC_NOTIFY_CODE_AUX_JACK_HOOK_STATE			0x08
#define CDC_NOTIFY_CODE_RING_DETECT					0x09
#define CDC_NOTIFY_CODE_SERIAL_STATE				0x20
#define CDC_NOTIFY_CODE_CALL_STATE_CHANGE			0x28
#define CDC_NOTIFY_CODE_LINE_STATE_CHANGE			0x29
#define CDC_NOTIFY_CODE_CONNECTION_SPEED_CHANGE		0x2A

#define CDC_REQUEST_CODE_SEND_ENCAPSULATED_COMMAND	0x00
#define CDC_REQUEST_CODE_GET_ENCAPSULATED_RESPONSE	0x01
#define CDC_REQUEST_CODE_SET_COMM_FEATURE			0x02	//PSTN
#define CDC_REQUEST_CODE_GET_COMM_FEATURE			0x03	//PSTN
#define CDC_REQUEST_CODE_CLEAR_COMM_FEATURE			0x04	//PSTN
#define CDC_REQUEST_CODE_SET_AUX_LINE_STATE			0x10	//PSTN
#define CDC_REQUEST_CODE_SET_HOOK_STATE				0x11	//PSTN
#define CDC_REQUEST_CODE_PULSE_SETUP				0x12	//PSTN
#define CDC_REQUEST_CODE_SEND_PULSE					0x13	//PSTN
#define CDC_REQUEST_CODE_SET_PULSE_TIME				0x14	//PSTN
#define CDC_REQUEST_CODE_RING_AUX_JACK				0x15	//PSTN
#define CDC_REQUEST_CODE_SET_LINE_CODING			0x20	//PSTN
#define CDC_REQUEST_CODE_GET_LINE_CODING			0x21	//PSTN
#define CDC_REQUEST_CODE_SET_CONTROL_LINE_STATE		0x22	//PSTN
#define CDC_REQUEST_CODE_SEND_BREAK					0x23	//PSTN
#define CDC_REQUEST_CODE_SET_RINGER_PARMS			0x30	//PSTN
#define CDC_REQUEST_CODE_GET_RINGER_PARMS			0x31	//PSTN
#define CDC_REQUEST_CODE_SET_OPERATION_PARMS		0x32	//PSTN
#define CDC_REQUEST_CODE_GET_OPERATION_PARMS		0x33	//PSTN
#define CDC_REQUEST_CODE_SET_LINE_PARMS				0x34	//PSTN
#define CDC_REQUEST_CODE_GET_LINE_PARMS				0x35	//PSTN	
#define CDC_REQUEST_CODE_DIAL_DIGITS				0x36	//PSTN
#define CDC_REQUEST_CODE_SET_UNIT_PARAMETER			0x37	//ISDN
#define CDC_REQUEST_CODE_GET_UNIT_PARAMETER			0x38	//ISDN
#define CDC_REQUEST_CODE_CLERA_UNIT_PARAMETER		0x39	//ISDN
#define CDC_REQUEST_CODE_GET_PROFILE				0x3A	//ISDN
#define CDC_REQUEST_CODE_SET_ETHERNET_MULTICAST_FILTERS					0x40	//ECM
#define CDC_REQUEST_CODE_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER	0x41	//ECM
#define CDC_REQUEST_CODE_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER	0x42	//ECM
#define CDC_REQUEST_CODE_SET_ETHERNET_PACKET_FILTER						0x43	//ECM
#define CDC_REQUEST_CODE_GET_ETHERNET_STATISTIC							0x44	//ECM
#define CDC_REQUEST_CODE_SET_ATM_DATA_FORMAT							0x50	//ATM
#define CDC_REQUEST_CODE_GET_ATM_DEVICE_STATISTICS						0x51	//ATM
#define CDC_REQUEST_CODE_SET_ATM_DEFAULT_VC								0x52	//ATM
#define CDC_REQUEST_CODE_GET_ATM_VC_STATISTICS							0x53	//ATM
//#define CDC_REQUEST_CODE_MDLM_SEMANTIC_MODEL_SPECIFIC_REQUESTS		0x60	//0x60~0x67	WMC
#define CDC_REQUEST_CODE_GET_NTB_PARAMETERS								0x80	//NCM
#define CDC_REQUEST_CODE_GET_NET_ADDRESS								0x81	//NCM
#define CDC_REQUEST_CODE_SET_NET_ADDRESS								0x82	//NCM
#define CDC_REQUEST_CODE_GET_NTB_FORMAT									0x83	//NCM
#define CDC_REQUEST_CODE_SET_NTB_FORMAT									0x84	//NCM
#define CDC_REQUEST_CODE_GET_NTB_INPUT_SIZE								0x85	//NCM
#define CDC_REQUEST_CODE_SET_NTB_INPUT_SIZE								0x86	//NCM
#define CDC_REQUEST_CODE_GET_MAX_DATAGRAM_SIZE							0x87	//NCM
#define CDC_REQUEST_CODE_SET_MAX_DATAGRAM_SIZE							0x88	//NCM
#define CDC_REQUEST_CODE_GET_CRC_MODE									0x89	//NCM
#define CDC_REQUEST_CODE_SET_CRC_MODE									0x8A	//NCM

extern void vcomStatusCallback(CDC_DEV_T *cdev, uint8_t *pu8RData, int u8DataLen);
extern void vcomRxCallback(CDC_DEV_T *cdev, uint8_t *pu8RData, int u8DataLen);
//extern int initCdcDevice(CDC_DEV_T *cdev);
extern int initCdcDevice(CDC_DEV_T *cdev, MOBILE_INSTANCE *wlInst);
//extern void usbHostTask(void *param);
extern void usbh_vendor_init(void);

extern int usbhVendorBulkWrite(USB_COMPOSITE_DEVICE_IF_CTX *devIfCtx, uint8_t *dataBuf, int dataLen);
extern void cdcEcmInputThread(void *param);


extern void usbHostTask(taskArg param);

typedef struct _USB_REQUEST_BLOCK
{	uint8_t		bmRequestType;
	uint8_t		bRequest;
	uint16_t	wValue;
	uint16_t	wIndex;
	uint16_t	wLength;
	uint8_t 	*data;
}USB_REQUEST_BLOCK;

#define PBUF_QUEUE_SIZE	1024

typedef struct _PBUF_QUEUE
{	void *data[1024];
	int head;
	int tail;
	int len;
}PBUF_QUEUE;

#endif