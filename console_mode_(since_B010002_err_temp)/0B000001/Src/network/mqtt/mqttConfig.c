/* mqttexample.c
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

#include "network/mqtt/mqtt_client.h"
#include "network/mqtt/mqttConfig.h"
#include "network/mqtt/mqttnet.h"
#include "sensSystem.h"


//extern sens_sys_struct *sensSys;
/* locals */
static volatile word16 mPacketIdLast;
static /*const*/ char* kDefTopicName = DEFAULT_TOPIC_NAME;
//static /*const*/ char* kDefClientId =  DEFAULT_CLIENT_ID;
static char *ikwDefaultTopicName = IKW_SEND_TOPIC;
/* argument parsing */
//static int myoptind = 0;
//static char* myoptarg = NULL;


#if 0
static int mygetopt(int argc, char** argv, const char* optstring)
{	static char* next = NULL;

	char  c;
	char* cp;

	if(myoptind == 0)
		next = NULL;   /* we're starting new/over */

	if(next == NULL || *next == '\0') 
	{	if(myoptind == 0)
			myoptind++;

		if(myoptind >= argc || argv[myoptind][0] != '-' || argv[myoptind][1] == '\0') 
		{	myoptarg = NULL;
			if(myoptind < argc)
				myoptarg = argv[myoptind];
			return -1;
		}

		if(XSTRNCMP(argv[myoptind], "--", 2) == 0) 
		{	myoptind++;
			myoptarg = NULL;
			if(myoptind < argc)
				myoptarg = argv[myoptind];
			return -1;
		}
		next = argv[myoptind];
		next++;                  /* skip - */
		myoptind++;
	}

	c  = *next++;
	/* The C++ strchr can return a different value */
	cp = (char*)XSTRCHR(optstring, c);

	if(cp == NULL || c == ':')
		return '?';

	cp++;

	if(*cp == ':') 
	{	if(*next != '\0') 
		{	myoptarg = next;
			next     = NULL;
		}
		else if(myoptind < argc) 
		{	myoptarg = argv[myoptind];
			myoptind++;
		}
		else
		return '?';
	}
	return c;
}
#endif

/* used for testing only, requires wolfSSL RNG */

#ifndef TEST_RAND_SZ
#define TEST_RAND_SZ 4
#endif

void mqtt_show_usage(MQTTCtx* mqttCtx)
{	PRINTF("%s:", mqttCtx->app_name);
	PRINTF("-?          Help, print this usage");
	PRINTF("-h <host>   Host to connect to, default %s", mqttCtx->host);
	PRINTF("-p <num>    Port to connect on, default: %d", MQTT_DEFAULT_PORT);
	PRINTF("-q <num>    Qos Level 0-2, default %d", mqttCtx->qos);
	PRINTF("-s          Disable clean session connect flag");
	PRINTF("-k <num>    Keep alive seconds, default %d", mqttCtx->keep_alive_sec);
	PRINTF("-i <id>     Client Id, default %s", mqttCtx->client_id);
	PRINTF("-l          Enable LWT (Last Will and Testament)");
	PRINTF("-u <str>    Username");
	PRINTF("-w <str>    Password");
	PRINTF("-n <str>    Topic name, default %s", mqttCtx->topic_name);
	PRINTF("-r          Set Retain flag on publish message");
	PRINTF("-C <num>    Command Timeout, default %dms", mqttCtx->cmd_timeout_ms);
#ifdef WOLFMQTT_V5
	PRINTF("-P <num>    Max packet size the client will accept, default: %d", DEFAULT_MAX_PKT_SZ);
#endif
	PRINTF("-T          Test mode");
	/*if(mqttCtx->pub_file) 
	{	PRINTF("-f <file>   Use file for publish, default %s", mqttCtx->pub_file);
	}*/
}

void mqtt_init_ctx(CLOUD_SERVER_INSTANCE *serverInst)
{	int allocSize;
	MQTTCtx *mqttCtx = (MQTTCtx *)serverInst->protCtx;
#if defined NET_LWIP
	struct
#endif
		sockaddr_in	*socketAddr;
	MQTT_CONNECT_INFO *mqttConnectInfo = serverInst->mqttConnectInfo;
	
	//mqttCtx->stat = WMQ_WAIT_MOBILE_INITIAL_DON;
	socketAddr = &serverInst->socketAddr;
	mqttCtx->isValid = 1;
	mqttCtx->host = serverInst->serverName;
	mqttCtx->qos = DEFAULT_MQTT_QOS;
	mqttCtx->clean_session = 1;
	mqttCtx->port = socketAddr->sin_port;
	mqttCtx->keep_alive_sec = DEFAULT_KEEP_ALIVE_SEC;
	if((serverInst->serverType >= MQTTS_AZURE_BROKER) && (serverInst->serverType < MQTT_PLATFORM_MAX))	//Azure or Avnet or Google or aws
	{	if(serverInst->serverType == MQTTS_AVNET_BROKER)	//Avnet
		{	mqttCtx->dynamicClientId = 1;
			allocSize = strlen(AVNET_CPID1) + strlen(mqttConnectInfo->clientId)+1;
			mqttCtx->client_id = SENS_MEM_ZALLOC(allocSize);
			SENS_SPRINTF(mqttCtx->client_id, AVNET_CLIENT_ID, mqttConnectInfo->clientId);
		}
		else if(serverInst->serverType == MQTTS_AZURE_BROKER)	//azure
		{	mqttCtx->dynamicClientId = 1;
			allocSize = strlen(mqttConnectInfo->clientId)+1;
			mqttCtx->client_id = SENS_MEM_ZALLOC(allocSize);
			SENS_SPRINTF(mqttCtx->client_id, "%s", mqttConnectInfo->clientId);
		}
		else if(serverInst->serverType == MQTT_THINGSBOARD_BROKER)	//things board
		{	mqttCtx->client_id = mqttConnectInfo->clientId;
			//mqttCtx->dynamicClientId = 0;
		}
#if defined SUPPORT_MQTT_SENSSEWER || defined SUPPORT_MQTTS_SENSSEWER
		else if((serverInst->serverType == MQTT_SENSSEWER_BROKER) || (serverInst->serverType == MQTTS_SENSSEWER_BROKER))
		{	goto ANASYSTEM_MQTT_PROCESS;
		}
#endif
		else	//google or aws
		{	//mqttCtx->client_id = serverInst->mqttDevName;
			mqttCtx->client_id = SENS_MEM_ZALLOC(strlen(sensSys->guid.guidString)+1);
			memcpy(mqttCtx->client_id, sensSys->guid.guidString, strlen(sensSys->guid.guidString));
		}
		mqttCtx->username = mqttConnectInfo->userName;
		if(serverInst->serverType <= MQTTS_AVNET_BROKER)	//
		{	if(strstr(mqttConnectInfo->password, AZURE_TYPE_PWD_HDR))
			{	mqttCtx->password = mqttConnectInfo->password;
				//dbg_msg("mqtt password %s\r\n", mqttCtx->password);
			}
			else
			{	mqttCtx->dynamicPassword = 1;
				allocSize = strlen(mqttConnectInfo->password) + strlen(AZURE_TYPE_PWD_HDR) + 1/*space char*/ + 1;	//must include space char
				mqttCtx->password = SENS_MEM_ZALLOC(allocSize);
				SENS_SPRINTF(mqttCtx->password, AZURE_TYPE_PWD_HDR_W_SPACE, mqttConnectInfo->password);
			}
			
			allocSize = strlen(AZURE_TYPE_PUB_TOPIC) + strlen(mqttCtx->client_id)+1;
			mqttCtx->dynamicTopic = 1;
			mqttCtx->topic_name = SENS_MEM_ZALLOC(allocSize);
			
			if(serverInst->serverType == MQTTS_AVNET_BROKER)
			{	SENS_SPRINTF(mqttCtx->topic_name, AVNET_PUB_TOPIC, mqttConnectInfo->clientId);
			}
			else
			{	SENS_SPRINTF(mqttCtx->topic_name, AZURE_PUB_TOPIC, mqttConnectInfo->clientId);
			}
		}
		else if(serverInst->serverType == MQTT_THINGSBOARD_BROKER)
		{	MQTT_TOPIC_CONFIG *pubTopics = mqttConnectInfo->pubTopics;
			
			mqttCtx->password = mqttConnectInfo->password;
			mqttCtx->topic_name = pubTopics->topic;
		}
		else if((serverInst->serverType == MQTT_I_KNOW_WATER_BROKER) || (serverInst->serverType == MQTTS_I_KNOW_WATER_BROKER))
		{	mqttCtx->password = mqttConnectInfo->password;
			mqttCtx->topic_name = ikwDefaultTopicName;			
		}
		else	//other aws or google iot core
		{	mqttCtx->password = mqttConnectInfo->password;
			mqttCtx->topic_name = kDefTopicName;
		}
	}
	else	//Senslink MQTT/MQTTS/ AnIoT MQTT
	{	
ANASYSTEM_MQTT_PROCESS:
		mqttCtx->dynamicClientId = 1;
		mqttCtx->client_id = SENS_MEM_ZALLOC(strlen(sensSys->guid.guidString)+1);
		memcpy(mqttCtx->client_id, sensSys->guid.guidString, strlen(sensSys->guid.guidString));
		//mqttCtx->client_id = DEFAULT_CLIENT_ID;
		if((mqttConnectInfo == NULL) || (mqttConnectInfo->userName == NULL) || (strlen(mqttConnectInfo->userName) == 0))
		{	mqttCtx->username = SENS_MEM_ZALLOC(strlen(sensSys->guid.guidString)+1);
			memcpy(mqttCtx->username, sensSys->guid.guidString, strlen(sensSys->guid.guidString));
		}
		else
			mqttCtx->username = mqttConnectInfo->userName;
		if((mqttConnectInfo == NULL) || (mqttConnectInfo->password == NULL) || (strlen(mqttConnectInfo->password) == 0))
		{	mqttCtx->password = SENS_MEM_ZALLOC(strlen("anasystem")+1);
			SENS_SPRINTF(mqttCtx->password, "anasystem");
		}
		else
			mqttCtx->password = mqttConnectInfo->password;
		
		if(serverInst->serverType < MQTTS_ANIOT_BROKER)
		{	allocSize = strlen("SensMini/") + strlen(sensSys->guid.guidString) + 1 + strlen("/Rsp");
			mqttCtx->dynamicTopic = 1;
			mqttCtx->topic_name = SENS_MEM_ZALLOC(allocSize);
			SENS_SPRINTF(mqttCtx->topic_name, "SensMini/%s/Rsp", sensSys->guid.guidString);
	#if SUPPORT_SENSTALK_MQTT_COMPRESSION
			mqttCtx->topic_name_compression = SENS_MEM_ZALLOC(allocSize+1);
			SENS_SPRINTF(mqttCtx->topic_name_compression, "SensMini/%s/cRsp", sensSys->guid.guidString);
	#endif
			mqttCtx->subscribeCnt = 2;
		}
	#if defined SUPPORT_MQTT_SENSSEWER || defined SUPPORT_MQTTS_SENSSEWER
		else if((serverInst->serverType >= MQTT_SENSSEWER_BROKER) && (serverInst->serverType <= MQTTS_SENSSEWER_BROKER))
		{	allocSize = strlen("SensSewer/") + strlen(sensSys->guid.ikwGuidString) + strlen("/upload") + 1;
			mqttCtx->dynamicTopic = 1;
			mqttCtx->topic_name = SENS_MEM_ZALLOC(allocSize);
			SENS_SPRINTF(mqttCtx->topic_name, "SensSewer/%s/upload", sensSys->guid.ikwGuidString);
			mqttCtx->subscribeCnt = 1;
		}
	#endif
		else	//ANIOT, YS MQTT
		{	allocSize = strlen(sensSys->guid.guidString) + 1 + strlen("/disconnect") + strlen("SensCfg/");
			mqttCtx->dynamicTopic = 1;
			mqttCtx->topic_name = SENS_MEM_ZALLOC(allocSize);
			SENS_SPRINTF(mqttCtx->topic_name, "SensCfg/%s/disconnect", sensSys->guid.guidString);
			mqttCtx->subscribeCnt = 1;
			mqttCtx->enable_lwt = 1;
		}

#if MQTT_ENABLE_LWT
		//mqttCtx->enable_lwt = 1;
#endif
	}
		
	mqttCtx->cmd_timeout_ms = DEFAULT_CMD_TIMEOUT_MS;
#ifdef WOLFMQTT_V5
	mqttCtx->max_packet_size = DEFAULT_MAX_PKT_SZ;
	mqttCtx->topic_alias = 1;
	mqttCtx->topic_alias_max = 1;
#endif
	//mqttCtx->pingTimeout = (long)DEFAULT_KEEP_ALIVE_SEC*1000;
}

void mqtt_free_ctx(MQTTCtx* mqttCtx)
{	if(mqttCtx == NULL) 
	{	return;
	}

#if 0
	if(mqttCtx->dynamicTopic && mqttCtx->topic_name) 
	{	WOLFMQTT_FREE((char*)mqttCtx->topic_name);
		mqttCtx->topic_name = NULL;
	}
	if(mqttCtx->dynamicClientId && mqttCtx->client_id) 
	{	WOLFMQTT_FREE((char*)mqttCtx->client_id);
		mqttCtx->client_id = NULL;
	}
	
	if(mqttCtx->dynamicPassword && mqttCtx->password)
	{	WOLFMQTT_FREE((char*)mqttCtx->password);
		mqttCtx->password = NULL;
	}
#endif
}

#if defined(__GNUC__) && !defined(NO_EXIT)
	//__attribute__ ((noreturn))
#endif

uint16_t mqtt_get_packetid(void)
{	/* Check rollover */
	if(mPacketIdLast >= MAX_PACKET_ID) 
	{	mPacketIdLast = 0;
	}

	return (++mPacketIdLast);
}

#ifdef WOLFMQTT_NONBLOCK
	#if defined(MICROCHIP_MPLAB_HARMONY)
	
	#include <system/tmr/sys_tmr.h>
	
	#else
	
	#include <time.h>
	
	#endif

static word32 mqtt_get_timer_seconds(void)
{	word32 timer_sec = 0;

#if defined(MICROCHIP_MPLAB_HARMONY)
	timer_sec = (word32)(SYS_TMR_TickCountGet() / SYS_TMR_TickCounterFrequencyGet());
#else
	/* Posix style time */
	timer_sec = (word32)time(0);
#endif

	return timer_sec;
}

int mqtt_check_timeout(int rc, word32* start_sec, word32 timeout_sec)
{	word32 elapsed_sec;

	/* if start seconds not set or is not continue */
	if(*start_sec == 0 || rc != MQTT_CODE_CONTINUE) 
	{	*start_sec = mqtt_get_timer_seconds();
		return rc;
	}

	elapsed_sec = mqtt_get_timer_seconds();
	if(*start_sec < elapsed_sec) 
	{	elapsed_sec -= *start_sec;
		if(elapsed_sec >= timeout_sec) 
		{	*start_sec = mqtt_get_timer_seconds();
			PRINTF("Timeout timer %d seconds", timeout_sec);
			return MQTT_CODE_ERROR_TIMEOUT;
		}
	}
	return rc;
}
#endif /* WOLFMQTT_NONBLOCK */

#ifdef ENABLE_MQTT_TLS
static int mqtt_tls_verify_cb(int preverify, WOLFSSL_X509_STORE_CTX* store)
{	char buffer[WOLFSSL_MAX_ERROR_SZ];
	MQTTCtx *mqttCtx = NULL;
	char appName[PRINT_BUFFER_SIZE] = {0};

	if(store->userCtx != NULL) 
	{	/* The client.ctx was stored during MqttSocket_Connect. */
		mqttCtx = (MQTTCtx *)store->userCtx;
		XSTRNCPY(appName, " for ", PRINT_BUFFER_SIZE-1);
		XSTRNCAT(appName, mqttCtx->app_name, PRINT_BUFFER_SIZE-XSTRLEN(appName)-1);
	}

	PRINTF("MQTT TLS Verify Callback%s: PreVerify %d, Error %d (%s)", appName, preverify, store->error, store->error != 0 ? wolfSSL_ERR_error_string(store->error, buffer) : "none");
	PRINTF("  Subject's domain name is %s", store->domain);

	if(store->error != 0) 
	{	/* Allowing to continue */
		/* Should check certificate and return 0 if not okay */
		PRINTF("  Allowing cert anyways");
	}

	return 1;
}

/* Use this callback to setup TLS certificates and verify callbacks */
int mqtt_tls_cb(MqttClient* client)
{	int rc = WOLFSSL_FAILURE;

	client->tls.ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
	if(client->tls.ctx) 
	{	wolfSSL_CTX_set_verify(client->tls.ctx, WOLFSSL_VERIFY_PEER, mqtt_tls_verify_cb);

		/* default to success */
		rc = WOLFSSL_SUCCESS;

#if !defined(NO_CERT)
	#if !defined(NO_FILESYSTEM)
		if(mTlsCaFile) 
		{	/* Load CA certificate file */
			rc = wolfSSL_CTX_load_verify_locations(client->tls.ctx, mTlsCaFile, NULL);
		}

		/* If using a client certificate it can be loaded using: */
		/* rc = wolfSSL_CTX_use_certificate_file(client->tls.ctx,
		 *                              clientCertFile, WOLFSSL_FILETYPE_PEM);*/
	#else
		if(mTlsCaFile) 
		{
		#if 0
			/* As example, load file into buffer for testing */
			long  caCertSize = 0;
			byte  caCertBuf[10000];
			FILE* file = fopen(mTlsCaFile, "rb");
			if(!file) 
			{	err_sys("can't open file for CA buffer load");
			}
			fseek(file, 0, SEEK_END);
			caCertSize = ftell(file);
			rewind(file);
			fread(caCertBuf, sizeof(caCertBuf), 1, file);
			fclose(file);

			/* Load CA certificate buffer */
			rc = wolfSSL_CTX_load_verify_buffer(client->tls.ctx, caCertBuf, caCertSize, WOLFSSL_FILETYPE_PEM);
		#endif
		}

		/* If using a client certificate it can be loaded using: */
		/* rc = wolfSSL_CTX_use_certificate_buffer(client->tls.ctx,
		 *               clientCertBuf, clientCertSize, WOLFSSL_FILETYPE_PEM);*/
	#endif /* !NO_FILESYSTEM */
#endif /* !NO_CERT */
	}

	PRINTF("MQTT TLS Setup (%d)", rc);

	return rc;
}
#else
int mqtt_tls_cb(MqttClient* client)
{	(void)client;
	return 0;
}
#endif /* ENABLE_MQTT_TLS */
#endif
