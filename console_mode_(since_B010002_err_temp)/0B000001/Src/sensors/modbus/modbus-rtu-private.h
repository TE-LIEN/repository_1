/*
 * Copyright © Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef MODBUS_RTU_PRIVATE_H
#define MODBUS_RTU_PRIVATE_H

/*#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdint.h"
#endif*/

//#include <termios.h>

#define _MODBUS_RTU_HEADER_LENGTH      1
#define _MODBUS_RTU_PRESET_REQ_LENGTH  6
#define _MODBUS_RTU_PRESET_RSP_LENGTH  2

#define _MODBUS_RTU_CHECKSUM_LENGTH    2

typedef struct _modbus_rtu 
{	/*char *device;
	int baud;
	uint8_t data_bit;
	uint8_t stop_bit;
	char parity;*/
	//struct termios old_tios;
	/* To handle many slaves on the same link */
	uint8_t	uartChannel;
	int confirmation_to_ignore;
} modbus_rtu_t;

#endif /* MODBUS_RTU_PRIVATE_H */
