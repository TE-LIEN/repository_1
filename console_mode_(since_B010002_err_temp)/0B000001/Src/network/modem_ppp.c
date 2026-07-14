#include "network/modem_ppp.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "lwip/dns.h"
#include "network/netApp.h"

#ifdef SUPPORT_PPP
//#include "lwip/tcpip.h"

//#include "NET_APP/netApp.h"

//#define PPPOS_RX_USE_THREAD

#if SUPPORT_NUVOTON_USB_HOST
#include "usb.h"
#include "hub.h"
#include "usbh_lib.h"
#include "usbh_cdc.h"
#include "usbHost/usbHostTask.h"
#endif

#ifdef NET_LWIP

//#define PPP_RECV_USE_MASS_DATA

int gPppMru /*= PPP_MRU*/;

uint8_t pppLinkup = 0;
uint8_t pppConnectionState = 3;

enum PPP_CONN_STATE
{	PPP_CONN_OK,
	PPP_CONN_DISCONNECT,
	PPP_CONN_DISCONNECT_DONE,
	PPP_CONN_MAX
};

#if defined PPPOS_RX_USE_THREAD || SUPPORT_NUVOTON_USB_HOST
void ppposRxThread(void *arg)
{	UART_CTX *dev;
	uint32_t len;
	uint8_t *buffer;
	uint32_t recvLen;
	MOBILE_USB_INSTANCE *mobUsbInst;
	MOBILE_INSTANCE *wlInst;
	wlInst = (MOBILE_INSTANCE *)arg;

	mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
	
	dbgMsg("%s", "pppos Thread Start!!\r\n");
	if(mobUsbInst->usbIsMultiIfDev)
	{	if(mobUsbInst->usbIsCdcAcm)
		{	if(wlInst->usbDevAt1)
			{	CDC_DEV_T *cdev = (CDC_DEV_T *)wlInst->usbDevAt1;
				UART_CONFIG *uartCfg = cdev->uartCfg;
				dev = uartCfg->ctx;
			}
		}
		else
		{	if(wlInst->usbDevAt1)
			{	USB_COMPOSITE_DEVICE_IF_CTX *usbDevIfCtx = (USB_COMPOSITE_DEVICE_IF_CTX *)wlInst->usbDevAt1;
				dev = usbDevIfCtx->usbCdcAtUartCfg->ctx;
			}
		}
	}
	else
		dev = wlInst->dev;
	
	buffer = SENS_MEM_ZALLOC(4096);
	/* Please read the "PPPoS input path" chapter in the PPP documentation. */
	
	while (1)
	{	//recvLen = SENS_UART_STATUS(dev);
		//if(recvLen)
		while(1)
		{	recvLen = SENS_UART_STATUS(dev);
			if(recvLen)
			{	SENS_SEM_LOCK(dev->sema);
				recvLen = SENS_UART_READ(dev, buffer, recvLen);
				SENS_SEM_UNLOCK(dev->sema);
				pppos_input_tcpip(wlInst->ppp, buffer, recvLen);
			}
			else
				break;
		}
		SENS_TIME_DELAY(5);

#if 0
		len = sio_read(ppp_sio, buffer, sizeof(buffer));
		if (len > 0)
		{	/* Pass received raw characters from PPPoS to be decoded through lwIP
		 	 * TCPIP thread using the TCPIP API. This is thread safe in all cases
		 	 * * but you should avoid passing data byte after byte. */
			pppos_input_tcpip(ppp, buffer, len);
		}
#endif
	}
}
#endif

void pppLinkStatusCb(ppp_pcb *pcb, int err_code, void *ctx)
{	struct netif *pppif = ppp_netif(pcb);
	LWIP_UNUSED_ARG(ctx);

	switch(err_code)
	{	case PPPERR_NONE:               /* No error. */
				{
#if LWIP_DNS
					const ip_addr_t *ns;
#endif /* LWIP_DNS */
					dbg_msg("%s", "PPP Link up.\r\n");
#if LWIP_IPV4
					dbg_msg("   our_ip4addr = %s\r\n", ip4addr_ntoa(netif_ip4_addr(pppif)));
					dbg_msg("   his_ipaddr  = %s\r\n", ip4addr_ntoa(netif_ip4_gw(pppif)));
					dbg_msg("   netmask     = %s\r\n", ip4addr_ntoa(netif_ip4_netmask(pppif)));
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
					dbg_msg("   our_ip6addr = %s\r\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* LWIP_IPV6 */

#if LWIP_DNS
					ns = dns_getserver(0);
					dbg_msg("   dns1        = %s\r\n", ipaddr_ntoa(ns));
					ns = dns_getserver(1);
					dbg_msg("   dns2        = %s\r\n", ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#if PPP_IPV6_SUPPORT
					dbg_msg("   our6_ipaddr = %s\r\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
					pppLinkup = 1;
				}
				break;
		case PPPERR_PARAM:             /* Invalid parameter. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_PARAM\r\n");
				break;
		case PPPERR_OPEN:              /* Unable to open PPP session. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_OPEN\r\n");
				break;
		case PPPERR_DEVICE:            /* Invalid I/O device for PPP. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_DEVICE\r\n");
				break;
		case PPPERR_ALLOC:             /* Unable to allocate resources. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_ALLOC\r\n");
				break;
		case PPPERR_USER:              /* User interrupt. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_USER\r\n");
				pppConnectionState = PPP_CONN_DISCONNECT_DONE;
				break;
		case PPPERR_CONNECT:           /* Connection lost. */
				{	struct TaskQMsg msg;
					
					pppLinkup = 0;
					dbg_msg("%s", "ppp_link_status_cb: PPPERR_CONNECT\r\n");
					msg.msgId = NETWORK_Q_MSG_PPP_DISCONNECTED;
					msg.ptr = (char *)ctx;
					if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
					{	
					}
				}
				break;
		case PPPERR_AUTHFAIL:          /* Failed authentication challenge. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_AUTHFAIL\r\n");
				break;
		case PPPERR_PROTOCOL:          /* Failed to meet protocol. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_PROTOCOL\r\n");
				break;
		case PPPERR_PEERDEAD:          /* Connection timeout. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_PEERDEAD\r\n");
				break;
		case PPPERR_IDLETIMEOUT:       /* Idle Timeout. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_IDLETIMEOUT\r\n");
				break;
		case PPPERR_CONNECTTIME:       /* PPPERR_CONNECTTIME. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_CONNECTTIME\r\n");
				break;
		case PPPERR_LOOPBACK:          /* Connection timeout. */
				dbg_msg("%s", "ppp_link_status_cb: PPPERR_LOOPBACK\r\n");
				break;
		default:
				dbg_msg("ppp_link_status_cb: unknown errCode %d\r\n", err_code);
				break;
	}
}

u32_t pppOutputCb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{	MOBILE_INSTANCE *wlInst = (MOBILE_INSTANCE *)ctx;
	UART_CTX		*dev;
#if SUPPORT_NUVOTON_USB_HOST
	MOBILE_USB_INSTANCE *mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
#endif
	
	LWIP_UNUSED_ARG(pcb);
	//LWIP_UNUSED_ARG(ctx);
#if SUPPORT_NUVOTON_USB_HOST
	if(mobUsbInst->isUsbHost)
	{	if(mobUsbInst->usbIsCdcAcm)
		{	CDC_DEV_T *cdev;
			cdev = (CDC_DEV_T *)wlInst->usbDev;
			usbh_cdc_send_data(cdev, (uint8_t *)data, len);
			return len; 
		}
		else
		{	USB_COMPOSITE_DEVICE_IF_CTX *usbDevIf;
			usbDevIf = (USB_COMPOSITE_DEVICE_IF_CTX *)wlInst->usbDev;
			usbhVendorBulkWrite(usbDevIf, (uint8_t *)data, len);
			return len; 
		}
	}
	else
#endif
	{	if(wlInst->isNotSupportCmuxMode)
			dev = wlInst->dev;
		else
			dev = wlInst->pppDev;
		return SENS_UART_WRITE(dev, data, len);
	}
}

#if LWIP_NETIF_STATUS_CALLBACK
void netif_status_callback(struct netif *nif)
{
	debugLock();
	dbg_msg1("PPP Link %s.\r\n", netif_is_up(nif) ? "UP" : "DOWN");

	//dbg_msg("PPPNETIF: %c%c%d is %s\r\n", nif->name[0], nif->name[1], nif->num, netif_is_up(nif) ? "UP" : "DOWN");
#if LWIP_IPV4
	dbg_msg1("IPV4: Host at %s ", ip4addr_ntoa(netif_ip4_addr(nif)));
	dbg_msg1("mask %s ", ip4addr_ntoa(netif_ip4_netmask(nif)));
	dbg_msg1("gateway %s\r\n", ip4addr_ntoa(netif_ip4_gw(nif)));
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
	dbg_msg1("IPV6: Host at %s\r\n", ip6addr_ntoa(netif_ip6_addr(nif, 0)));
#endif /* LWIP_IPV6 */
#if LWIP_NETIF_HOSTNAME
	dbg_msg1("FQDN: %s\r\n", netif_get_hostname(nif));
#endif /* LWIP_NETIF_HOSTNAME */
	debugUnlock();
}
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#endif	//#ifdef NET_LWIP

int wirelessPppInit(MOBILE_INSTANCE *wlInst)
{	UART_CTX	*dev;
	//uint8_t 	pppLinkup=0;
	uint32_t	pppLinkTimer;
	uint8_t ipV4[4];
#if SUPPORT_NUVOTON_USB_HOST
	MOBILE_USB_INSTANCE *mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
#endif
	
	if(!wlInst->isNotSupportCmuxMode
	#if SUPPORT_NUVOTON_USB_HOST
			&& (!mobUsbInst->isUsbHost)
	#endif
		)
	{	if(wlInst->module == MOBILE_TYPE_LE910C1)
			gPppMru = 110;
		else //if(wlInst->module == MOBILE_TYPE_ME310G)
			gPppMru = 110;
		//else //if(wlInst->module == MOBILE_TYPE_ME310G)
			//gPppMru = 100;
	}
	else
	{	gPppMru = 1500;
	}

	pppLinkup=0;
	pppConnectionState = PPP_CONN_MAX;
	wlInst->ppposNetif = (struct netif *)SENS_MEM_ZALLOC(sizeof(struct netif));
	wlInst->ppp = (void *)pppos_create(wlInst->ppposNetif, pppOutputCb, pppLinkStatusCb, (void *)wlInst);

	if(!wlInst->isNotSupportCmuxMode
#if SUPPORT_NUVOTON_USB_HOST
			&& (!mobUsbInst->isUsbHost)
#endif
		)
	{	SENS_SEM_LOCK(iotSys.iotSem);
		wlInst->dev = wlInst->pppDev;
		SENS_SEM_UNLOCK(iotSys.iotSem);
	}
	if(setPppMode(wlInst))
		return 0;

	/*
	ppp_set_auth(ppp, PPPAUTHTYPE_CHAP, "lwip", "mysecret");
	*/
	ppp_connect((ppp_pcb *)wlInst->ppp, 0);

#if LWIP_NETIF_STATUS_CALLBACK
	netif_set_status_callback(wlInst->ppposNetif, netif_status_callback);
#endif
#if defined PPPOS_RX_USE_THREAD || SUPPORT_NUVOTON_USB_HOST
	//sys_thread_new("ppposRxThread", ppposRxThread, wlInst, 4096, DEFAULT_THREAD_PRIO);
	#ifdef SUPPORT_NUVOTON_USB_HOST
	if(mobUsbInst->isUsbHost)
	#endif
		sys_thread_new("ppposRxThread", ppposRxThread, wlInst, 4096, PPPOS_RX_THREAD_PRIORITY);
	else
		wlInst->startUsePpp = 1;
#else
	wlInst->startUsePpp = 1;
#endif
	dbg_msg("%s", "[PPP]Attempting to establish connection. Please wait...");
	pppLinkTimer = GetTimeTicks();
	while(1)
	{	SENS_TIME_DELAY(1000);
		//if(netif_is_up(wlInst->ppposNetif))
		if(pppLinkup)
		{	//pppLinkup = 1;
			/*debugLock();
			dbg_msg1("%s", "PPP Link up.\r\n");
			debugUnlock();*/
			//ip_addr_t dnsserver;
			//inet_pton(AF_INET, sysParams->dnsServer, &dnsserver);
			//dns_setserver(0, &dnsserver);

			netif_set_default(wlInst->ppposNetif);
			//netif_set_mtu(wlInst->ppposNetif, 96);
			
			//dnsQuery("device.senslink.net", ipV4);
			//dbgMsg("device.senslink.net is %zd.%zd.%zd.%zd\r\n", ipV4[0], ipV4[1], ipV4[2], ipV4[3]);
			
			//ppp_send_config(wlInst->ppp, PPP_MRU, 0, 0, 0);
			break;
    }
		if(sysCtrl->isReadyToReboot)
			break;
		if((GetTimeTicks() - pppLinkTimer) > MODEM_PPP_CONNECTION_TIMEOUT_S)
		{	dbg_msg("%s", "\r\nppp link timeout\r\n");
			break;
		}
		debugLock();
		dbg_msg1("%s", ".");
		debugUnlock();
	}
	
	//pppLinkup = 0;	//for test ppp fail
	
	if(!pppLinkup)
	{	modemDisconnect(wlInst);
		wlInst->ppp = NULL;
	}
	else
	{	
	}
	
	return 1;
}

void modemDisconnect(MOBILE_INSTANCE *wlInst)
{	ppp_pcb *ppp = (ppp_pcb *)wlInst->ppp;
	TaskHandle_t handle = NULL;
	uint32_t pppCloseTimeout;

	ppp_close(ppp, 0);
	
	pppCloseTimeout = systemTickGet();
	while(1)
	{	SENS_TIME_DELAY(100);
		if(pppConnectionState == PPP_CONN_DISCONNECT_DONE)
			break;
		if((systemTickGet() - pppCloseTimeout) > 10000)
		{	sysCtrl->isReadyToReboot = 1;
			break;
		}
		
	}
	ppp_free(ppp);
	
	if(wlInst->ppposNetif)
	{	netif_set_down(wlInst->ppposNetif);
		SENS_MEM_FREE(wlInst->ppposNetif);
		wlInst->ppposNetif = NULL;
	}
	
	wlInst->startUsePpp = 0;
	pppLinkup = 0;
	dbg_msg("%s", "[PPP]PPP connection closed\r\n");

#if defined PPPOS_RX_USE_THREAD || SUPPORT_NUVOTON_USB_HOST
	handle = xTaskGetHandle("ppposRxThread");
	if(handle != NULL)
		vTaskDelete(handle);
#endif
}

int isPppLink(MOBILE_INSTANCE *wlInst)
{
	/*ppp_pcb *ppp = (ppp_pcb *)wlInst->ppp;
	struct netif *pppif;

	if(ppp == NULL)
		return 0;
	pppif = ppp_netif(ppp);
	return netif_is_up(pppif);*/
	return pppLinkup;
}

#endif	//#ifdef SUPPORT_PPP
