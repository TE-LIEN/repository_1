/* mqttnet.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfMQTT.
 *
 * wolfMQTT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfMQTT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* Include the autoconf generated config.h */
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include "network/mqtt/mqttConfig.h"

#ifdef SUPPORT_MQTT
#include "sensminiCfg.h"
#include "network/mqtt/mqtt_client.h"
#include "network/mqtt/mqttnet.h"
#include "network/mqtt/mqttConfig.h"
//#include "gprs.h"
#include "network/netapp.h"
//#include "mobileTask.h"

#ifdef OS_MQX
#define FREESCALE_MQX
#endif
#define WOLFMQTT_NO_TIMEOUT


/* Freescale MQX / RTCS */
#if defined(FREESCALE_MQX) || defined(FREESCALE_KSDK_MQX)
	#if defined(FREESCALE_MQX)
		#include <posix.h>
	#endif
	#include "rtcs.h"

  #include "addrinfo.h"
	/* Note: Use "RTCS_geterror(sock->fd);" to get error number */
	#define SOCKET_INVALID  RTCS_SOCKET_ERROR
	#define SOCKET_T        int32_t
	//#define SOCK_CLOSE      closesocket
	#define SOCK_OPEN       RTCS_socket
	
	#define MQX_SOCK_CONNECT		connect
#endif

/* Setup defaults */
#ifndef SOCK_OPEN
	#define SOCK_OPEN       socket
#endif
#ifndef SOCKET_T
	#define SOCKET_T        int
#endif
#ifndef SOERROR_T
	#define SOERROR_T       int
#endif
#ifndef SELECT_FD
	#define SELECT_FD(fd)   ((fd) + 1)
#endif
#ifndef SOCKET_INVALID
	#define SOCKET_INVALID  ((SOCKET_T)0)
#endif

//#ifndef SOCK_CONNECT
//	#define SOCK_CONNECT    connect
//#endif

#if 0
	#ifndef SOCK_SEND
		#define SOCK_SEND(s,b,l,f) send((s), (b), (size_t)(l), (f))
	#endif
#endif

#ifndef SOCK_RECV
	#define SOCK_RECV(s,b,l,f) recv((s), (b), (size_t)(l), (f))
#endif

#ifndef SOCK_CLOSE
	#define SOCK_CLOSE      close
#endif

#ifndef SOCK_ADDR_IN
	#define SOCK_ADDR_IN    struct sockaddr_in
#endif
#ifdef SOCK_ADDRINFO
	#define SOCK_ADDRINFO   struct addrinfo
#endif


//uint_32 socks[TCP_MAX_SOCKETS];
extern void CloseSkt(int skt);

/* Private functions */

#ifndef WOLFMQTT_NO_TIMEOUT
static void setup_timeout(struct timeval* tv, int timeout_ms)
{	
#if 0
	tv->tv_sec = timeout_ms / 1000;
	tv->tv_usec = (timeout_ms % 1000) * 1000;

	/* Make sure there is a minimum value specified */
	if(tv->tv_sec < 0 || (tv->tv_sec == 0 && tv->tv_usec <= 0)) 
	{	tv->tv_sec = 0;
		tv->tv_usec = 100;
	}
#endif
}

#ifdef WOLFMQTT_NONBLOCK
static void tcp_set_nonblocking(SOCKET_T* sockfd)
{
#ifdef USE_WINDOWS_API
	unsigned long blocking = 1;
	int ret = ioctlsocket(*sockfd, FIONBIO, &blocking);
	if(ret == SOCKET_ERROR)
		PRINTF("ioctlsocket failed!");
#else
	int flags = fcntl(*sockfd, F_GETFL, 0);
	if(flags < 0)
		PRINTF("fcntl get failed!");
	flags = fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK);
	if(flags < 0)
		PRINTF("fcntl set failed!");
#endif
}
#endif /* WOLFMQTT_NONBLOCK */
#endif /* !WOLFMQTT_NO_TIMEOUT */

static int NetConnect(void *context, char* host, word16 port, int timeout_ms)
{	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)context;
	int rc = -1;
  struct TaskQMsg msg;

	/* Get address information for host and locate IPv4 */
	switch(serverInst->socketState) 
	{	case SOCK_BEGIN_MQTT:	//create SOCKET
					{	//char ipaddr[4];
						//PRINTF("NetConnect: Host %s, Port %u, Timeout %d ms, Use TLS %d", host, port, timeout_ms, mqttCtx->use_tls);
						/* Create socket */ 
						serverInst->socketState = SOCK_CREATE_WAIT;
						msg.msgId = NETWORK_Q_MSG_TCP_SOCK_CREATE;
						msg.ptr = (char *)serverInst;
						if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
						{
						}
						rc = MQTT_CODE_WAIT;
						break;
						//FALL_THROUGH;
					}
		case SOCK_CREATE_WAIT:
					rc = MQTT_CODE_WAIT;
					break;
		case SOCK_CREATE_DONE:
					serverInst->socketState = SOCK_CONN;
		case SOCK_CONN:
					{
#ifndef WOLFMQTT_NO_TIMEOUT
						fd_set fdset;
						struct timeval tv;
						
						/* Setup timeout and FD's */
						setup_timeout(&tv, timeout_ms);
						FD_ZERO(&fdset);
						FD_SET(sock->fd, &fdset);
#endif /* !WOLFMQTT_NO_TIMEOUT */

#if !defined(WOLFMQTT_NO_TIMEOUT) && defined(WOLFMQTT_NONBLOCK)
						if(mqttCtx->useNonBlockMode) 
						{	/* Set socket as non-blocking */
							tcp_set_nonblocking(&sock->fd);
						}
#endif

						/* Start connect */
						serverInst->socketState = SOCK_CONN_WAIT;
						dbg_msg("%s, sock connect!!\r\n", __func__);
						msg.msgId = NETWORK_Q_MSG_TCP_SOCK_CONNECT;
						msg.ptr = (char *)serverInst;
						if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
						{
						}
						rc = MQTT_CODE_WAIT;
						break;
					}
		case SOCK_CONN_WAIT:
					rc = MQTT_CODE_WAIT;
					break;
		case SOCK_CONN_DONE:
					rc = MQTT_CODE_SUCCESS;
					break;
    case SOCK_CONN_FAIL:
		default:
          dbg_msg("sock bad arg:%d\r\n", serverInst->socketState);
					rc = MQTT_CODE_ERROR_BAD_ARG;
	} /* switch */

	(void)timeout_ms;
	return rc;
}

static int NetWrite(void *context, /*const */byte* buf, int buf_len, int timeout_ms)
{	//return 0;	
	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)context;
	int rc;
	//SOERROR_T so_error = 0;
#ifndef WOLFMQTT_NO_TIMEOUT
	struct timeval tv;
#endif

	if(context == NULL || buf == NULL || buf_len <= 0) 
	{	return MQTT_CODE_ERROR_BAD_ARG;
	}

	if(serverInst->fd == SOCKET_INVALID)
		return MQTT_CODE_ERROR_BAD_ARG;

#ifndef WOLFMQTT_NO_TIMEOUT
	/* Setup timeout */
	setup_timeout(&tv, timeout_ms);
	setsockopt(serverInst->fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
#endif

	rc = networkSocketWrite(serverInst, buf, buf_len, timeout_ms);
	if(rc == -1) 
	{	/* Get error */
#if 0
		socklen_t len = sizeof(so_error);
		getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &so_error, (uint_32 *)&len);
		if(so_error == 0) 
		{
	#if defined(USE_WINDOWS_API) && defined(WOLFMQTT_NONBLOCK)
			/* assume non-blocking case */
			rc = MQTT_CODE_CONTINUE;
	#else
			rc = 0; /* Handle signal */
	#endif
		}
		else 
		{
	#ifdef WOLFMQTT_NONBLOCK
			if(so_error == EWOULDBLOCK || so_error == EAGAIN) 
			{	return MQTT_CODE_CONTINUE;
			}
	#endif
#endif
			rc = MQTT_CODE_ERROR_NETWORK;
#if 0
			PRINTF("NetWrite: Error %d", so_error);
		}
#endif
	}
	(void)timeout_ms;
	return rc;
}

static int NetRead_ex(void *context, byte* buf, int buf_len, int timeout_ms, byte peek)
{	CLOUD_SERVER_INSTANCE *serverInst = (CLOUD_SERVER_INSTANCE *)context;
	MQTTCtx* mqttCtx = (MQTTCtx *)serverInst->protCtx;
	int rc = -1, timeout = 0;
	//SOERROR_T so_error = 0;
	int bytes = 0;
	//int flags = 0;
#ifndef WOLFMQTT_NO_TIMEOUT
	fd_set recvfds;
	fd_set errfds;
	struct timeval tv;
#endif

	(void)mqttCtx;

	if(context == NULL || buf == NULL || buf_len <= 0) 
	{	return MQTT_CODE_ERROR_BAD_ARG;
	}

	if(serverInst->fd == SOCKET_INVALID)
		return MQTT_CODE_ERROR_BAD_ARG;


#ifndef WOLFMQTT_NO_TIMEOUT
	/* Setup timeout and FD's */
	setup_timeout(&tv, timeout_ms);
	FD_ZERO(&recvfds);
	FD_SET(serverInst->fd, &recvfds);
	FD_ZERO(&errfds);
	FD_SET(serverInst->fd, &errfds);

	#ifdef WOLFMQTT_ENABLE_STDIN_CAP
	if(!mqttCtx->test_mode) 
	{	FD_SET(STDIN, &recvfds);
	}
	#endif
#else
	(void)timeout_ms;
#endif /* !WOLFMQTT_NO_TIMEOUT && !WOLFMQTT_NONBLOCK */

	/* Loop until buf_len has been read, error or timeout */
	while(bytes < buf_len) 
	{	int do_read = 0;

#ifndef WOLFMQTT_NO_TIMEOUT		//true
	#ifdef WOLFMQTT_NONBLOCK
		if(mqttCtx->useNonBlockMode) 
		{	do_read = 1;
		}
		else
	#endif
		{	/* Wait for rx data to be available */
			rc = select((int)SELECT_FD(serverInst->fd), &recvfds, NULL, &errfds, &tv);
			if(rc > 0)
			{	if(FD_ISSET(serverInst->fd, &recvfds))
				{	do_read = 1;
				}
				/* Check if rx or error */
	#ifdef WOLFMQTT_ENABLE_STDIN_CAP
				else if(!mqttCtx->test_mode && FD_ISSET(STDIN, &recvfds)) 
				{	return MQTT_CODE_STDIN_WAKE;
				}
	#endif
				if(FD_ISSET(serverInst->fd, &errfds)) 
				{	rc = -1;
					break;
				}
			}
			else 
			{	timeout = 1;
				break; /* timeout or signal */
			}
		}
#else
		do_read = 1;
#endif /* !WOLFMQTT_NO_TIMEOUT */

		if(do_read) 
		{	/* Try and read number of buf_len provided, minus what's already been read */
			rc = networkSocketRead(serverInst, &buf[bytes], buf_len - bytes, timeout_ms);
		}
		if(rc < 0) 
		{	if(rc == MQTT_CODE_WAIT)
			{	return rc;
			}
			rc = -1;
			goto exit; /* Error */
		}
		else 
		{	bytes += rc; /* Data */
		}

		/* no timeout and non-block should always exit loop */
#ifdef WOLFMQTT_NONBLOCK
		if(mqttCtx->useNonBlockMode) 
		{	break;
		}
#endif
#ifdef WOLFMQTT_NO_TIMEOUT
		break;
#endif
	} /* while */
exit:
	if(rc == 0 && timeout) 
	{	rc = MQTT_CODE_ERROR_TIMEOUT;
	}
	else if(rc < 0) 
	{	/* Get error */
#if 0
		socklen_t len = sizeof(so_error);
		getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &so_error, (uint_32 *)&len);

		if(so_error == 0) 
		{	rc = 0; /* Handle signal */
		}
		else 
		{
#ifdef WOLFMQTT_NONBLOCK
			if(so_error == EWOULDBLOCK || so_error == EAGAIN) 
			{	return MQTT_CODE_CONTINUE;
			}
#endif
			rc = MQTT_CODE_ERROR_NETWORK;
			PRINTF("NetRead: Error %d", so_error);
		}
#endif
	}
	else 
	{	rc = bytes;
	}

	return rc;
}

static int NetRead(void *context, byte* buf, int buf_len, int timeout_ms)
{	return NetRead_ex(context, buf, buf_len, timeout_ms, 0);
}

static int NetDisconnect(void *context)
{	
#if 0
	SocketContext *sock = (SocketContext*)context;
	if(sock) 
	{	if(sock->fd != SOCKET_INVALID) 
		{	SOCK_CLOSE(sock->fd);
			sock->fd = -1;
		}
		sock->stat = SOCK_BEGIN;
	}
#endif
	return 0;
}


/* Public Functions */
//int MqttClientNet_Init(MqttNet* net, MQTTCtx* mqttCtx)
//int MqttClientNet_Init(SocketContext *sockCtx)
int MqttClientNet_Init(CLOUD_SERVER_INSTANCE *serverInst)
{	MQTTCtx *mqttCtx = (MQTTCtx *)serverInst->protCtx;
	MqttNet *net = &mqttCtx->net;
	
	if(net) 
	{	XMEMSET(net, 0, sizeof(MqttNet));
		net->wlofConnect = NetConnect;
		net->mobileRead = NetRead;
		net->mobileWrite = NetWrite;
		net->disconnect = NetDisconnect;
		net->context = serverInst;
		serverInst->socketState = SOCK_BEGIN_MQTT;
	}
	return MQTT_CODE_SUCCESS;
}


int MqttClientNet_DeInit(MqttNet* net)
{	if(net)
	{	/*if(net->context) 
		{	WOLFMQTT_FREE(net->context);
		}
		XMEMSET(net, 0, sizeof(MqttNet));*/
	}
	return 0;
}
#endif
