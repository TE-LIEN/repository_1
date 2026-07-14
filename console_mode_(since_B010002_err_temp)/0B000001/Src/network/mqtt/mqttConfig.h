/* mqttexample.h
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

#ifndef WOLFMQTT_EXAMPLE_H
#define WOLFMQTT_EXAMPLE_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "sensminiCfg.h"
//#define FREESCALE_MQX

//#include <mqx.h>
//#include <lwevent.h>
#ifdef SUPPORT_MQTT

#include "network/mqtt/mqtt_client.h"

/* Compatibility Options */
#ifdef NO_EXIT
	#undef exit
	#define exit(rc) return rc
#endif

#ifndef MY_EX_USAGE
#define MY_EX_USAGE 2 /* Exit reason code */
#endif

#define MQTT_CONNECT_ACCEPT 						0
#define MQTT_CONNECT_ERR_PROTOCOL_VER 	1
#define MQTT_CONNECT_IDENTIFIER_REJECT 	2
#define MQTT_CONNECT_SERVER_UNAVAILABLE 3
#define MQTT_CONNECT_BAD_USER_NAME_PASS 4
#define MQTT_CONNECT_NO_AUTHORIZED 			5


/* STDIN / FGETS for examples */
#ifndef WOLFMQTT_NO_STDIO
    /* For Linux/Mac */
    #if !defined(FREERTOS) && !defined(USE_WINDOWS_API) && \
        !defined(FREESCALE_MQX) && !defined(FREESCALE_KSDK_MQX) && \
        !defined(MICROCHIP_MPLAB_HARMONY)
        /* Make sure its not explicitly disabled and not already defined */
        #if !defined(WOLFMQTT_NO_STDIN_CAP) && \
            !defined(WOLFMQTT_ENABLE_STDIN_CAP)
            /* Wake on stdin activity */
            #define WOLFMQTT_ENABLE_STDIN_CAP
        #endif
    #endif

    #ifdef WOLFMQTT_ENABLE_STDIN_CAP
        #ifndef XFGETS
            #define XFGETS     fgets
        #endif
        #ifndef STDIN
            #define STDIN 0
        #endif
    #endif
#endif /* !WOLFMQTT_NO_STDIO */


/* Default Configurations */
//#define DEFAULT_CMD_TIMEOUT_MS  15000
#define DEFAULT_CMD_TIMEOUT_MS  8000
#define DEFAULT_CON_TIMEOUT_MS  5000
#define DEFAULT_MQTT_QOS        MQTT_QOS_0
#define DEFAULT_KEEP_ALIVE_SEC  90
//#define DEFAULT_CLIENT_ID       "sensminiMQTTClient"
#define DEFAULT_CLIENT_ID       "sensminiA4-"
#define WOLFMQTT_TOPIC_NAME     "wolfMQTT/example/"
#define DEFAULT_TOPIC_NAME      WOLFMQTT_TOPIC_NAME"testTopic"
#define DEFAULT_AUTH_METHOD    "EXTERNAL"
#define PRINT_BUFFER_SIZE       80


#define AZURE_HOST							"anaiottest.azure-devices.net"
#define AZURE_DEV								"dev0"
#define AZURE_API_VERSION				"api-version=2019-11-04"
#define AZURE_USER_NAME					AZURE_HOST"/"AZURE_DEV"/"AZURE_API_VERSION
#define AZURE_PASSWORD					"SharedAccessSignature sr=anaiottest.azure-devices.net%2Fdevices%2Fdev0&sig=EA9oPguv3sqfGYGka0cyJCCkf3No8fZCWcBYcLho5Vg%3D&se=1614255792"
//#define AZURE_PASSWORD					""
#define AZURE_SEND_TOPIC 				"devices/"AZURE_DEV"/messages/events/"
#define AZURE_SUBSCRIBE_TOPIC		"devices/"AZURE_DEV"/messages/devicebound/#"

#define AZURE_SUB_TOPIC					"devices/%s/messages/devicebound/#"
#define AZURE_PUB_TOPIC					"devices/%s/messages/events/"

#define AZURE_TYPE_PUB_TOPIC		"devices//messages/events/"

//AVNET this info get from https get and post cmd 
#define AVNET_HOST							"poc-iotconnect-iothub-eu.azure-devices.net"		//fixed
#define AVNET_CPID							"MjY1QUUzRDQtMUY5Mi00MkQ4LTk3MEEtQjc3N0QwREUwREMzLWFjY2Vzc0tFWS1kOGdzY3hocnVr"	//fixed
#define AVNET_DEV								"SENSMINI0002"	//vcariable
#define AVNET_USER_NAME					"poc-iotconnect-iothub-eu.azure-devices.net/MjY1QUUzRDQtMUY5Mi00MkQ4LTk3MEEtQjc3N0QwREUwREMzLWFjY2Vzc0tFWS1kOGdzY3hocnVr-SENSMINI0002/?api-version=2018-06-30"	//variable
#define AVNET_PASSWORD					"SharedAccessSignature sr=poc-iotconnect-iothub-eu.azure-devices.net%2Fdevices%2FMjY1QUUzRDQtMUY5Mi00MkQ4LTk3MEEtQjc3N0QwREUwREMzLWFjY2Vzc0tFWS1kOGdzY3hocnVr-SENSMINI0002&sig=O%2BQRNef%2F0lHU5%2BJwfu%2BA70JYgPkzuz8pg0c2Asg%2FaIs%3D&se=1614331171"	//variable
#define AVNET_PUBLISH_TOPIC			"devices/MjY1QUUzRDQtMUY5Mi00MkQ4LTk3MEEtQjc3N0QwREUwREMzLWFjY2Vzc0tFWS1kOGdzY3hocnVr-SENSMINI0002/messages/events/"
#define AVNET_SUBSCRIBE_TOPIC		"devices/MjY1QUUzRDQtMUY5Mi00MkQ4LTk3MEEtQjc3N0QwREUwREMzLWFjY2Vzc0tFWS1kOGdzY3hocnVr-SENSMINI0002/messages/devicebound/#"

#define AVNET_DTG								"e52121c1-cd2e-461a-a4ea-13cfdb6eabed"
#define AVNET_TEMPLETE					"SENSMINI"

#define AVNET_CLIENT_ID					AVNET_CPID"-%s"
#define AVNET_SUB_TOPIC					"devices/"AVNET_CLIENT_ID"/messages/devicebound/#"
#define AVNET_PUB_TOPIC					"devices/"AVNET_CLIENT_ID"/messages/events/"

#define AZURE_TYPE_PWD_HDR					"SharedAccessSignature"
#define AZURE_TYPE_PWD_HDR_W_SPACE	"SharedAccessSignature %s"

#define AVNET_CPID1									"MjY1QUUzRDQtMUY5Mi00MkQ4LTk3MEEtQjc3N0QwREUwREMzLWFjY2Vzc0tFWS1kOGdzY3hocnVr-"

	#define AVNET_MSGTYPE_RPT									0
	#define AVNET_MSGTYPE_FLT									1
	#define AVNET_MSGTYPE_RPTEDGE							2
	#define AVNET_MSGTYPE_RMEdge							3
	#define AVNET_MSGTYPE_LOG									4
	#define AVNET_MSGTYPE_ACK									5
	#define AVNET_MSGTYPE_OTA									6
	#define AVNET_MSGTYPE_DEVICE_CREATED			9
	#define AVNET_MSGTYPE_DEVICE_CONNECTED		10
	#define AVNET_MSGTYPE_FIRMWARE						11


#ifdef WOLFMQTT_V5
#define DEFAULT_MAX_PKT_SZ      768 /* The max MQTT control packet size the
                                       client is willing to accept. */
#endif

#define SEND_TOPIC		"SensMini/Recv"

#define SENSTALK_LWT_MSG "{\"Ver\":3,\"Cmd\":\"DCnt\"}"
#define SENSCFG_LWT_MSG "{\"Disconnect:\": true}"

#define ANIOT_LWT_TOPIC	"device_sts"
//#define ANIOT_LWT_MSG	""
//SENS_SPRINTF(jsonBuf, "{\"Ver\":3,\"Cmd\":\"%s\"", (senstalkCmd->cmdType == STK_REBOOT)? "Rbt":"Slp" );


#define IKW_SEND_TOPIC	"upload/AnaMQTT2023"
#define IKW_ACK_TOPIC		"ack/AnaMQTT2023/"
#define IKW_REQ_TOPIC		"config/AnaMQTT2023/"	//add sn number

#define IKW_TIME_JSON_STR						"TIME"
#define IKW_SN_JSON_STR							"SN"
#define IKW_ALARM1_JSON_STR					"A1"
#define IKW_ALARM2_JSON_STR					"A2"
#define IKW_ALARM3_JSON_STR					"A3"
#define IKW_DATALOG_PERIOD_JSON_STR	"DP"
#define IKW_INSTALLHEIGHT_JSON_STR	"IH"
#define IKW_DI_JSON_STR							"DI"


/*
 *	SENS SEWER MQTT
 */
#define SENSSEWER_SEND_TOPIC	"SensSewer/"

/* MQTT Client state */
typedef enum _MQTTCtxState {
    WMQ_WAIT_MOBILE_INITIAL_DONE = 0,
    //WMQ_BEGIN = 0,
    WMQ_BEGIN,
    WMQ_NET_INIT,
    WMQ_INIT,
    WMQ_TCP_CONN,
    WMQ_MQTT_CONN,
    WMQ_MQTT_CONN_WAIT,
    WMQ_MQTT_CONN_DONE,
    WMQ_SUB,
    WMQ_SUB_WAIT,
    WMQ_SUB_DONE,
    WMQ_PUB,
    WMQ_PUB_WAIT,
    WMQ_WAIT_MSG,
    WMQ_PING,
    WMQ_PING_WAIT,
    WMQ_UNSUB,
    WMQ_UNSUB_WAIT,
    WMQ_DISCONNECT,
    WMQ_NET_DISCONNECT,
    WMQ_DONE
} MQTTCtxState;

/* MQTT Client context */
/* This is used for the examples as reference */
/* Use of this structure allow non-blocking context */
typedef struct _MQTTCtx 
{	char				sendPolling;
	char 				isValid;
	char				eventCreated;
	SENS_SEM_STRUCT 	mqttSem;
	SENS_EVENT_STRUCT	writeDoneEvent;
	SENS_EVENT_STRUCT	publishDoneEvent;
	int					errorRetryCnt;
	char 				subscribeCnt;
	char 				currentSubscribeIdx;
	MQTTCtxState		stat;
	void* 				app_ctx; /* For storing application specific data */
	/* client and net containers */
	MqttClient 			client;
	MqttNet 			net;
    /* temp mqtt containers */
	MqttConnect 		connect;
	MqttMessage 		lwt_msg;
	MqttSubscribe 		subscribe;
	MqttUnsubscribe 	unsubscribe;
	MqttTopic 			topics[1];
	MqttTopic  			*topic;
	MqttPublish 		publish;
	MqttDisconnect 		disconnect;

	/* configuration */
	MqttQoS 			qos;
	const char* 		app_name;
	char* 				host;
	char* 				username;
	char* 				password;
	char* 				topic_name;
#if SUPPORT_SENSTALK_MQTT_COMPRESSION
	char* topic_name_compression;
#endif
	//const char* pub_file;
	/*const*/ char* 	client_id;
	byte 				*tx_buf;
	byte 				*rx_buf;
	int 				return_code;
	int 				use_tls;
	int 				retain;
	int 				enable_lwt;
#ifdef WOLFMQTT_V5
	int      			max_packet_size;
#endif
	word32 				cmd_timeout_ms;
#if defined(WOLFMQTT_NONBLOCK)
	word32  			start_sec; /* used for keep-alive */
#endif
	word16 				keep_alive_sec;
	word16 				port;
#ifdef WOLFMQTT_V5
	word16  			topic_alias;
	word16  			topic_alias_max; /* Server property */
#endif
	byte    			clean_session;
	byte    			test_mode;
#ifdef WOLFMQTT_V5
	byte    			subId_not_avail; /* Server property */
	byte    			enable_eauth; /* Enhanced authentication */
#endif
	unsigned int 		dynamicTopic:1;
	unsigned int 		dynamicClientId:1;
	unsigned int		dynamicPassword:1;
#ifdef WOLFMQTT_NONBLOCK
	unsigned int 		useNonBlockMode:1; /* set to use non-blocking mode. network callbacks can return MQTT_CODE_CONTINUE to indicate "would block" */
#endif
	long   				pingTimeout;
} MQTTCtx;


void mqtt_show_usage(MQTTCtx* mqttCtx);
//void mqtt_init_ctx(MQTTCtx* mqttCtx, char *brokerIp, int mobileType);
//void mqtt_init_ctx(CLOUD_SERVER_INSTANCE *serverInst, int type);
void mqtt_init_ctx(CLOUD_SERVER_INSTANCE *serverInst);
void mqtt_free_ctx(MQTTCtx* mqttCtx);
int mqtt_parse_args(MQTTCtx* mqttCtx, int argc, char** argv);
//int err_sys(const char* msg);

int mqtt_tls_cb(MqttClient* client);
uint16_t mqtt_get_packetid(void);

#ifdef WOLFMQTT_NONBLOCK
int mqtt_check_timeout(int rc, word32* start_sec, word32 timeout_sec);
#endif


#endif
#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* WOLFMQTT_EXAMPLE_H */
