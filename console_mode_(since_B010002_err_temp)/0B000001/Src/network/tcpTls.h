#ifndef _TCP_TLS_H_
#define _TCP_TLS_H_

#include "sensminiCfg.h"

#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cache.h"
#include "mbedtls/debug.h"

//typedef mbedtls_ssl_context *  tcpTlsSock_t;

typedef enum TLS_STATE_TAG
{	TLS_STATE_NOT_INITIALIZED,
	TLS_STATE_INITIALIZED,
	TLS_STATE_CLOSING,
}TLS_STATE;

typedef struct _TCP_TLS_CTX
{	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_config config;
	mbedtls_ssl_context ssl;
	mbedtls_x509_crt cliCert;
	mbedtls_x509_crt ownCert;
	mbedtls_pk_context pkey;
	mbedtls_ssl_session session;
	int 	tlsStatus;
	int		tlsUseLowFragment;
	char *hostname;
	char *x509Certificate;
	char *x509PrivateKey;
	int 	x509CertificateLen;
	int 	x509PrivateKeyLen;
}TCP_TLS_CTX;


typedef enum TCP_SSL_TYPE
{	TCP_SSL_SERVER,
	TCP_SSL_CLIENT
}TCP_SSL_TYPE;

#if 0
typedef struct _TCP_TLS_CLIENT_PARAMS_STRUCT
{	char*              certFile;       /* Client or Server Certificate file.*/
	char*              privKeyFile;   /* Client or Server private key file.*/
	char*              caFile;         /* CA (Certificate Authority) certificate file.*/
	unsigned long			 certSize;	//yushun add for mbedtls
	unsigned long			 keySize;	//yushun add for mbedtls
	//TCP_SSL_TYPE 			init_type;
	TCP_TLS_CTX				tcpTlsCtx;
}TCP_TLS_CLIENT_PARAMS_STRUCT;
#endif

extern void mbedtlsUninit(TCP_TLS_CTX *tcpTlsCtx);
extern int mbedtlsInit(TCP_TLS_CTX *tcpTlsCtx);
extern int tcpTlsConnect(TCP_TLS_CTX *tcpTlsCtx, int sockFd);
extern int tcpTlsRead(mbedtls_ssl_context *sslSock, void *buf, size_t len);
extern int tcpTlsWrite(mbedtls_ssl_context *sslSock, void *buf, size_t len);


#endif

#endif