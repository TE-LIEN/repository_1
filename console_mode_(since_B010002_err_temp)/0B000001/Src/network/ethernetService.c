//#include "network/ethernetService.h"
#include "network/netApp.h"
#include "sensSystem.h"
#include "sensCommon.h"
#include "network/httpsrv/httpsrv.h"

#if HTTPSRV_CFG_MBEDTLS_ENABLE
#include "mbedtls/certs.h"
static HTTPSRV_TLS_PARAM_STRUCT tls_params;
#endif

#if defined(SENSMINIA4_PRO) || defined(SENSMINIA4_NEO)
#define DEFAULT_MAC0_ADDRESS {0x00, 0x55, 0x7B, 0xB5, 0x7D, 0xF7}
static unsigned char sMacAddr[6] = DEFAULT_MAC0_ADDRESS;
#endif

#ifdef NET_LWIP

extern err_t ethernetif_init(struct netif *netif);
//------------------------------------------------------------------------------
static void tcpipInitDone(void *arg)
{	if (arg) 
	{	*((uint8_t *)arg) = 1;
	}
}
//------------------------------------------------------------------------------
void initLwip(void)
{	volatile int	setup = 0;
	DNS_CONFIG		*dnsCfgs;
	NET_CONFIG		*netCfg;
	//lwip_init();
	
	tcpip_init(tcpipInitDone, (void *)&setup);
	while(!setup) 
	{	SENS_TIME_DELAY(2);
	}
	
	//dns_init();
	ip_addr_t dnsserver;
#define DNS_SERVER_ADDRESS(ipaddr, dnsAddrStr) (ip4_addr_set_u32(ipaddr, ipaddr_addr(dnsAddrStr)))
	
	netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	dnsCfgs = (DNS_CONFIG *)netCfg->dnsCfgs;
	DNS_SERVER_ADDRESS(&dnsserver, dnsCfgs->dnsServerStr);
	dns_setserver(0, &dnsserver);
}
//------------------------------------------------------------------------------
#if LWIP_NETIF_LINK_CALLBACK
void ethNetLinkStatus(struct netif *netif)
{	struct netif *netifBack;
	int findOtherDefaultNetIf = 0;
	
	if(netif_is_flag_set(netif, NETIF_FLAG_LINK_UP))
	{	if(netif_default == NULL)	//prev is mobile
		{	netif_set_default(netif);
			dbgMsg("%s", "eth set to default 2\r\n");
		}
		if(netif_is_up(netif))
		{	dbg_msg("%s", "[Ethernet] eth already up!!\r\n");
		}
		else
		{	netif_set_up(netif);
			dbg_msg("%s", "[Ethernet] set eth up!!\r\n");
		}
	}
	else	//link down
	{	netif_set_down(netif);
		dbg_msg("%s", "[Ethernet] set eth Down!!\r\n");

		if(netif_default && (netif_default == netif))
		{	for(netifBack = netif_list;netifBack != NULL;netifBack = netifBack->next)
			{	if(netifBack != netif)
				{	netif_set_default(netifBack);
					findOtherDefaultNetIf = 1;
					dbg_msg("[Ethernet] set to other default, %p\r\n", netifBack);
				}
			}
		}
		if(!findOtherDefaultNetIf)
		{	netif_set_default(NULL);
		}
	}
}
#endif
//------------------------------------------------------------------------------
static int lwipEthInit(void)
{	NET_CONFIG	*netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	WIRED_CONFIG	*wiredCfg = (WIRED_CONFIG *)&netCfg->wiredCfg;
	NET_INSTANCE	*lanInst = networkCtx->lanInst;
	WIRED_INSTANCE	*wiredInst = lanInst->wiredInst;
	DNS_CONFIG		*dnsCfgs;
	struct netif	*ethNetif;
	
	
	IP4_ADDR(&wiredInst->ipAddr, (((wiredCfg->ipAddr)>>24)&0xFF),(((wiredCfg->ipAddr)>>16)&0xFF),(((wiredCfg->ipAddr)>> 8)&0xFF),((wiredCfg->ipAddr)&0xFF));
	IP4_ADDR(&wiredInst->ipMask, (((wiredCfg->maskAddr)>>24)&0xFF),(((wiredCfg->maskAddr)>>16)&0xFF),(((wiredCfg->maskAddr)>> 8)&0xFF),((wiredCfg->maskAddr)&0xFF));
	IP4_ADDR(&wiredInst->ipGw, (((wiredCfg->gwAddr)>>24)&0xFF),(((wiredCfg->gwAddr)>>16)&0xFF),(((wiredCfg->gwAddr)>> 8)&0xFF),((wiredCfg->gwAddr)&0xFF));
	
	dbgMsg("Wired LAN IP: %d.%d.%d.%d\r\n", IPBYTES(wiredCfg->ipAddr));
	dbgMsg("Wired LAN MASK: %d.%d.%d.%d\r\n", IPBYTES(wiredCfg->maskAddr));
	dbgMsg("Wired LAN Gateway: %d.%d.%d.%d\r\n", IPBYTES(wiredCfg->gwAddr));
	
	initLwip();
	
	//MAC OUI use A4 : 00-00-5E, A4 Neo : 00-55-7B
	//MAC SN use CHIP SN -00-01-58
	sensSys->guid.ucid[3] = FMC_ReadUID(2);
	//sMacAddr[0] = 0x00;
	//sMacAddr[1] = 0x00;
	//sMacAddr[2] = 0x5E;
	sMacAddr[3] = (sensSys->guid.ucid[3] >> 16) & 0xFF;
	sMacAddr[4] = (sensSys->guid.ucid[3] >> 8) & 0xFF;
	sMacAddr[5] = (sensSys->guid.ucid[3] >> 0) & 0xFF;
	
	ethNetif = &wiredInst->ethNetif;
	SMEMCPY(ethNetif->hwaddr, sMacAddr, sizeof(ethNetif->hwaddr));

	// dbgMsg("[Ethernet] MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n", sMacAddr[0], sMacAddr[1], sMacAddr[2], sMacAddr[3], sMacAddr[4], sMacAddr[5]);

	netif_add(&wiredInst->ethNetif, &wiredInst->ipAddr, &wiredInst->ipMask, &wiredInst->ipGw, NULL, ethernetif_init, tcpip_input);
	ethNetif = &wiredInst->ethNetif;
	ethNetif->name[0] = 'e';
	ethNetif->name[1] = '0';

#if LWIP_NETIF_LINK_CALLBACK
    netif_set_link_callback(ethNetif, ethNetLinkStatus);
#endif
	
	if(netif_default == NULL)
    {	netif_set_default(ethNetif);
		dbgMsg("%s", "eth set to default 1\r\n");
	}
	if(netif_is_link_up(ethNetif))
    {	netif_set_up(ethNetif);
		dbgMsg("%s", "eth set to Up 1\r\n");
	}
	return 0;
}	

#endif

#ifdef NET_RTCS

#endif
extern const HTTPSRV_CGI_LINK_STRUCT cgi_lnk_tbl[];
extern const HTTPSRV_FS_DIR_ENTRY tfs_data[];
//------------------------------------------------------------------------------
#ifdef NET_LWIP
#define HTTP_SRV_USE_TLS	0

void httpServerInit(void)
{	HTTPSRV_PARAM_STRUCT params;
	uint32_t httpsrv_handle;
	
	HTTPSRV_FS_init(tfs_data);
	memset(&params, 0, sizeof(params));
	params.root_dir    = "";
	params.index_page  = "/Master.html";
	params.auth_table  = NULL;
	params.cgi_lnk_tbl = cgi_lnk_tbl;
	params.ssi_lnk_tbl = NULL;
#if HTTPSRV_CFG_WEBSOCKET_ENABLED
    params.ws_tbl = ws_tbl;
#endif
#if HTTPSRV_CFG_MBEDTLS_ENABLE && HTTP_SRV_USE_TLS
    tls_params.certificate_buffer      = (const unsigned char *)mbedtls_test_srv_crt;
    tls_params.certificate_buffer_size = mbedtls_test_srv_crt_len;
    tls_params.private_key_buffer      = (const unsigned char *)mbedtls_test_srv_key;
    tls_params.private_key_buffer_size = mbedtls_test_srv_key_len;

    /*tls_params.certificate_buffer      = (const unsigned char *)YUSHUN_CERIFICATE;
	tls_params.certificate_buffer_size = sizeof(YUSHUN_CERIFICATE);
	tls_params.private_key_buffer      = (const unsigned char *)YUSHUN_KEY;
	tls_params.private_key_buffer_size = sizeof(YUSHUN_KEY);*/

    params.tls_param = &tls_params;
#endif
    httpsrv_handle = HTTPSRV_init(&params);
	if (httpsrv_handle == 0)
	{	//LWIP_PLATFORM_DIAG(("HTTPSRV_init() is Failed"));
		dbg_msg("%s", "http Server Init Fail!!\r\n");
	}
}
#endif
//------------------------------------------------------------------------------
int ethernetInit(void)
{	int ret = -1;
	
#ifndef SUPPORT_WIRED_LAN
	initLwip();
	ret = 0;
#else
	NET_INSTANCE	*lanInst = networkCtx->lanInst;
	if(lanInst->wiredInst == NULL)
		lanInst->wiredInst = SENS_MEM_ZALLOC(sizeof(WIRED_INSTANCE));	
	PIN_ETH_PWR_EN = 1;
	SENS_TIME_DELAY(500);
	dbgMsg("%s", "start initial Ethernet\r\n");
	#ifdef NET_RTCS
	
	#endif
	
	#ifdef NET_LWIP
	ret = lwipEthInit();
	httpServerInit();
	#endif
#endif
	return ret;
}

//------------------------------------------------------------------------------
int setupEthernetComm(void)
{	NET_INSTANCE *lanInst;
	
	//networkCtx->lanInst = SENS_MEM_ZALLOC(sizeof(NET_INSTANCE));
	lanInst = networkCtx->lanInst;
	lanInst->netType = NETWORK_TYPE_ETH;
	//lanInst->netMdvpnType = sensSys->wlMdvpnType;
	return 0;
}
//------------------------------------------------------------------------------
void addDnsServerCfg(DNS_CONFIG *dnsCfg)
{	DNS_CONFIG *dnsCfgTemp;
	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	
	if(netCfg->dnsCfgs == NULL)
		netCfg->dnsCfgs = dnsCfg;
	else 
	{	dnsCfgTemp = netCfg->dnsCfgs;
		while(dnsCfgTemp->next)
		{	dnsCfgTemp = dnsCfgTemp->next;
		}
		dnsCfgTemp->next = dnsCfg;
	}
}
//------------------------------------------------------------------------------