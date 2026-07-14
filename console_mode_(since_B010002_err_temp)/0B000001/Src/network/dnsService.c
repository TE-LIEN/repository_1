#include "dnsService.h"
#include "sensCommon.h"
#include "network/netApp.h"

volatile uint8_t	dnsResolveIpV4[4];
volatile uint8_t	dnsResolveDone = 1;

static void dnsCallback(const char *name, ip_addr_t *ipaddr, void *arg)
{	dnsResolveIpV4[0] = ipaddr->addr >> 24;
	dnsResolveIpV4[1] = ipaddr->addr >> 16;
	dnsResolveIpV4[2] = ipaddr->addr >> 8;
	dnsResolveIpV4[3] = ipaddr->addr >> 0;
	dnsResolveDone = 1;
}

void dnsQuery(char *domainName, uint8_t *ipV4)
{	ip_addr_t cipaddr_DNS;
	dnsResolveDone = 0;
	if(dns_gethostbyname(domainName, &cipaddr_DNS, (void *)dnsCallback, NULL) != 0)
	{	while(1)
		{	SENS_TIME_DELAY(50);
			if(dnsResolveDone)
			{	break;
			}
		}
		dnsResolveDone = 0;
		ipV4[0] = dnsResolveIpV4[3];
		ipV4[1] = dnsResolveIpV4[2];
		ipV4[2] = dnsResolveIpV4[1];
		ipV4[3] = dnsResolveIpV4[0];
	}
	else
	{	dbgMsg("ip in lookup table is :%X\r\n", cipaddr_DNS.addr);
		ipV4[0] = cipaddr_DNS.addr & 0xFF;
		ipV4[1] = (cipaddr_DNS.addr >> 8) & 0xFF;
		ipV4[2] = (cipaddr_DNS.addr >> 16) & 0xFF;
		ipV4[3] = (cipaddr_DNS.addr >> 24) & 0xFF;
	}	
	dbg_msg("DNS resolve %s to ip %zd.%zd.%zd.%zd\r\n", domainName, ipV4[0], ipV4[1], ipV4[2], ipV4[3]);
}