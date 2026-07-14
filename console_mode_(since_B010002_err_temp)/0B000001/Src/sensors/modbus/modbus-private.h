/*
 * Copyright © Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef MODBUS_PRIVATE_H
#define MODBUS_PRIVATE_H

/*#ifndef _MSC_VER
#include <stdint.h>
#include <sys/time.h>
#else
#include "stdint.h"
#include <time.h>*/
//typedef int ssize_t;
//#endif
//#include <sys/types.h>
//#include <config.h>

#include "sensminiCfg.h"
#include "sensors/modbus/modbus.h"

//#define fd_set void*
#ifdef NET_RTCS

#define fd_set 		rtcs_fd_set
#define FD_ZERO		RTCS_FD_ZERO
#define FD_SET		RTCS_FD_SET


#endif

#ifdef OS_MQX	//add err no

#define ECONNRESET 		(POSIX_ERROR_BASE | 104)
#define ECONNREFUSED	(POSIX_ERROR_BASE | 111)
#define ENOPROTOOPT 	(POSIX_ERROR_BASE | 92)
#endif

#ifndef OS_FREERTOS
struct timeval
{	uint32_t tv_sec;
	uint32_t tv_usec;
};
#endif

#define ENOTSUP	134

MODBUS_BEGIN_DECLS

/* It's not really the minimal length (the real one is report slave ID
 * in RTU (4 bytes)) but it's a convenient size to use in RTU or TCP
 * communications to read many values or write a single one.
 * Maximum between :
 * - HEADER_LENGTH_TCP (7) + function (1) + address (2) + number (2)
 * - HEADER_LENGTH_RTU (1) + function (1) + address (2) + number (2) + CRC (2)
 */
#define _MIN_REQ_LENGTH 12
#define MAX_MESSAGE_LENGTH 260

#define _REPORT_SLAVE_ID 180

#define _MODBUS_EXCEPTION_RSP_LENGTH 5

/* Timeouts in microsecond (0.5 s) */
#define _RESPONSE_TIMEOUT    500000
#define _BYTE_TIMEOUT        500000

typedef enum 
{	_MODBUS_BACKEND_TYPE_RTU=0,
	_MODBUS_BACKEND_TYPE_TCP
}modbus_backend_type_t;

/*
 *  ---------- Request     Indication ----------
 *  | Client | ---------------------->| Server |
 *  ---------- Confirmation  Response ----------
 */
typedef enum 
{	/* Request message on the server side */
	MSG_INDICATION,
	/* Request message on the client side */
	MSG_CONFIRMATION
}msg_type_t;

/* This structure reduces the number of params in functions and so
 * optimizes the speed of execution (~ 37%). */
typedef struct _sft 
{	int slave;
	int function;
	int t_id;
}sft_t;

typedef struct _modbus_backend 
{	unsigned int backend_type;
	unsigned int header_length;
	unsigned int checksum_length;
	unsigned int max_adu_length;
	int			(*set_slave) 							(modbus_t *ctx, int slave);
	int 		(*build_request_basis)		(modbus_t *ctx, int function, int addr, int nb, uint8_t *req);
	int 		(*build_response_basis)		(sft_t *sft, uint8_t *rsp);
	int 		(*prepare_response_tid)		(const uint8_t *req, int *req_length);
	int 		(*send_msg_pre)						(uint8_t *req, int req_length);
#if defined NET_LWIP
	ssize_t (*sendModbus)										(modbus_t *ctx, const uint8_t *req, int req_length);
#else
	ssize_t (*send)										(modbus_t *ctx, const uint8_t *req, int req_length);
#endif
	int 		(*receive)								(modbus_t *ctx, uint8_t *req);
#if defined NET_LWIP
	ssize_t (*modbusRecv)										(modbus_t *ctx, uint8_t *rsp, int rsp_length);
#else
	ssize_t (*recv)										(modbus_t *ctx, uint8_t *rsp, int rsp_length);
#endif
	int 		(*check_integrity)				(modbus_t *ctx, uint8_t *msg, const int msg_length);
	int 		(*pre_check_confirmation)	(modbus_t *ctx, const uint8_t *req, const uint8_t *rsp, int rsp_length);
#if defined NET_LWIP
	int 		(*modbusConnect)								(modbus_t *ctx);
	void 		(*modbusClose)									(modbus_t *ctx);
#else
	int 		(*connect)								(modbus_t *ctx);
	void 		(*close)									(modbus_t *ctx);
#endif
	
	int 		(*flush)									(modbus_t *ctx);
#if defined NET_LWIP
	int 		(*modbusSelect) 								(modbus_t *ctx, fd_set *rset, struct timeval *tv, int msg_length);
#else
	int 		(*select) 								(modbus_t *ctx, fd_set *rset, struct timeval *tv, int msg_length);
#endif
	void 		(*free)										(modbus_t *ctx);
}modbus_backend_t;

struct _modbus 
{	/* Slave address */
	int slave;
	/* Socket or file descriptor */
	int s;
	void *sendDev;
	void *recvDev;
	int debug;
	int error_recovery;
	struct timeval response_timeout;
	struct timeval byte_timeout;
	struct timeval indication_timeout;
	const modbus_backend_t *backend;
	void *backend_data;
  uint8_t rsp[MAX_MESSAGE_LENGTH];
	uint8_t req[_MIN_REQ_LENGTH];
};

void _modbus_init_common(modbus_t *ctx);
void _error_print(modbus_t *ctx, const char *context);
int _modbus_receive_msg(modbus_t *ctx, uint8_t *msg, msg_type_t msg_type);

#ifndef HAVE_STRLCPY
size_t strlcpy1(char *dest, const char *src, size_t dest_size);
#endif

MODBUS_END_DECLS

#endif  /* MODBUS_PRIVATE_H */
