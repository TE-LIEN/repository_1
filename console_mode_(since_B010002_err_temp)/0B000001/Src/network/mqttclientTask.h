/* mqttclient.h
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

#ifndef WOLFMQTT_MQTTCLIENT_H
#define WOLFMQTT_MQTTCLIENT_H

#include "sensminiCfg.h"

/* Exposed functions */
#ifdef SUPPORT_MQTT

#include "network/mqtt/mqttConfig.h"

#define MQTT_NUM_MESSAGES	32	//max queue size

#define mqttQSize MQTT_NUM_MESSAGES * SENS_TASKQ_GRANM * sizeof(_mqx_max_type)
	
#define MQTT_MAX_BUFFER_SIZE	4096


#if defined OS_MQX
	extern _mqx_max_type  mqttQ[mqttQSize];
	extern void mqtt_task(uint_32 temp);
#elif defined OS_FREERTOS
	extern QueueHandle_t  mqttQ;
	extern void mqtt_task(void *param);
#endif

extern int sendMqttQ(MQTTCtx *mqttCtx, int id);
extern int mqttClientInitial(CLOUD_SERVER_INSTANCE *serverInst);
extern void mqttClientFreeCtx(MQTTCtx *mqttCtx);
extern void mqttClientSubscribe(MQTTCtx *mqttCtx, char *topic, MqttQoS qos, char num);
extern void mqttClientPublisher(MQTTCtx *mqttCtx, char *topic, MqttQoS qos, int retain, byte *message, int messageSize);

extern CLOUD_SERVER_INSTANCE *getServerInstFromMqtt(MQTTCtx *mqttCtx);
#endif

#endif /* WOLFMQTT_MQTTCLIENT_H */
