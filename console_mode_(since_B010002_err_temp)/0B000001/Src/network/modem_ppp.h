#ifndef __MODEM_PPP_H__
#define __MODEM_PPP_H__

#include "sensminiCfg.h"
#include "network/netApp.h"

#ifdef SUPPORT_PPP 
#include "netif/ppp/pppos.h"

#ifndef __sio_fd_t_defined
#define __sio_fd_t_defined
typedef UART_CTX * sio_fd_t;
#endif

//extern void sio_open()

#include "lwip/sio.h"

#define MODEM_PPP_CONNECTION_TIMEOUT_S     20000
extern void modemDisconnect(MOBILE_INSTANCE *wlInst);
extern int wirelessPppInit(MOBILE_INSTANCE *wlInst);
#endif	//#ifdef SUPPORT_PPP

#endif	//#ifndef __MODEM_PPP_H__
