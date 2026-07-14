#ifdef TCP_CLIENT_CFG_MBEDTLS_ENABLE

#include "network/tcpTls.h"
#include "sensCommon.h"
#include "mbedtls/error.h"


//static const int tls_cipher_suites[2] = {MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384, 0};
static const int tls_cipher_suites[3] = {MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384, MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384, 0};


static const int sig_hashes[] = {
    MBEDTLS_MD_SHA256,
    MBEDTLS_MD_SHA384,
    MBEDTLS_MD_NONE
};


//SDK_ALIGN(static uint8_t certBufAligned[2048], 8);


//------------------------------------------------------------------------------
static int tcpCliMbedtlsSend(void *ctx, unsigned char const *buf, size_t len)
{	int result;
	result = send((int)ctx, (void *)buf, len, 0);
	dbg_msg("tls Send len:%d, result:%d\r\n", len, result);
	//displayBufInHex((uint8_t *)buf, result, __func__, __LINE__);
	return result;
}
//------------------------------------------------------------------------------
static int tcpCliMbedtlsRecv(void *ctx, unsigned char *buf, size_t len)
{	int result;
	//int printSize;
	
	result = recv((int)ctx, buf, len, 0);
#if 0
	if(result)
	{	dbg_msg("tls Recv len:%d, result:%d\r\n", len, result);
		/*printSize = result;
		if(printSize > 0x80)
			printSize = 0x80;*/
		//displayBufInHex(buf, result, __func__, __LINE__);
	}
#endif
	return result;
}
//------------------------------------------------------------------------------
void mbedtlsUninit(TCP_TLS_CTX *tcpTlsCtx)
{	int ret=0;
	
	if(tcpTlsCtx->tlsStatus != TLS_STATE_NOT_INITIALIZED)
	{	//mbedtls_ssl_close_notify(&tcpTlsCtx->ssl);
		do
		{	ret = mbedtls_ssl_close_notify(&tcpTlsCtx->ssl);
		}while(ret == MBEDTLS_ERR_SSL_WANT_WRITE);
		dbgMsg(RED"ssl close ret:%d\r\n"ORG_COLOR, ret);
		
		mbedtls_ssl_free(&tcpTlsCtx->ssl);
		mbedtls_ssl_config_free(&tcpTlsCtx->config);
		mbedtls_x509_crt_free(&tcpTlsCtx->cliCert);
		mbedtls_x509_crt_free(&tcpTlsCtx->ownCert);
		mbedtls_ctr_drbg_free(&tcpTlsCtx->ctr_drbg);
		mbedtls_entropy_free(&tcpTlsCtx->entropy);
		mbedtls_pk_free(&tcpTlsCtx->pkey);

		SENS_MEM_FREE(tcpTlsCtx->hostname);
		tcpTlsCtx->tlsStatus = TLS_STATE_NOT_INITIALIZED;
	}
}
//------------------------------------------------------------------------------
int mbedtlsInit(TCP_TLS_CTX *tcpTlsCtx)
{	const char *pers = "ssl_client";
	int idx=0;
	
	dbg_msg("%s", "[MBEDTLS] init\r\n");
	if(tcpTlsCtx->tlsStatus != TLS_STATE_INITIALIZED)
	{	if(tcpTlsCtx->tlsStatus == TLS_STATE_CLOSING)
		{	// The underlying connection has been closed, so here un-initialize first
			mbedtlsUninit(tcpTlsCtx);
		}
        dbg_msg("%s", "[MBEDTLS] crt init\r\n");
		mbedtls_x509_crt_init(&tcpTlsCtx->cliCert);
		mbedtls_x509_crt_init(&tcpTlsCtx->ownCert);
		mbedtls_pk_init(&tcpTlsCtx->pkey);
		mbedtls_entropy_init(&tcpTlsCtx->entropy);
		//mbedtls_entropy_add_source(&tcpTlsCtx->entropy, tlsio_entropy_poll, NULL, MBEDTLS_ENTROPY_MAX_GATHER, MBEDTLS_ENTROPY_SOURCE_WEAK);
		mbedtls_ctr_drbg_init(&tcpTlsCtx->ctr_drbg);
		mbedtls_ssl_config_init(&tcpTlsCtx->config);
		
#ifdef MBEDTLS_V3_1
		if(mbedtls_ssl_config_defaults(&tcpTlsCtx->config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0)
		{	dbgMsg("%s", YELLOW"tcp client TLS ssl config default fail!!\r\n"ORG_COLOR);
			goto INIT_ERROR;
		}
#endif
		
#ifdef MBEDTLS_V3_1
		if(mbedtls_ctr_drbg_seed(&tcpTlsCtx->ctr_drbg, mbedtls_entropy_func, &tcpTlsCtx->entropy, (const unsigned char *)pers, strlen(pers)) != 0)
		{	dbgMsg("%s", YELLOW"tcp client TLS crt drbg seed fail!!\r\n"ORG_COLOR);
			goto INIT_ERROR;
		}
#endif
		//mbedtls_ssl_conf_sig_hashes(&tcpTlsCtx->config, sig_hashes);
		
		dbg_msg("%s", "[MBEDTLS] cipher init\r\n");
		for(idx=0;tls_cipher_suites[idx] != 0;idx++)
		{	if(tls_cipher_suites[idx] == MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384) 
			{	mbedtls_x509_crt_parse(&tcpTlsCtx->cliCert, (const unsigned char *)mbedtls_test_cli_crt, mbedtls_test_cli_crt_len);
#if !defined MBEDTLS_V3_1
				mbedtls_pk_parse_key(&tcpTlsCtx->pkey, (const unsigned char *)mbedtls_test_cli_key, mbedtls_test_cli_key_len, NULL, 0);
#else
				mbedtls_pk_parse_key(&tcpTlsCtx->pkey, (const unsigned char *)mbedtls_test_cli_key, mbedtls_test_cli_key_len, NULL, 0, mbedtls_ctr_drbg_random, &tcpTlsCtx->ctr_drbg);
#endif
				//break;
			} 
			else if(tls_cipher_suites[idx] == MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384) 
			{	//memset(certBufAligned, 0, 2048);
				//memcpy(certBufAligned, mbedtls_test_cli_crt_ec, mbedtls_test_cli_crt_ec_len);
				mbedtls_x509_crt_parse(&tcpTlsCtx->cliCert, (const unsigned char *)mbedtls_test_cli_crt_ec, mbedtls_test_cli_crt_ec_len);
				//mbedtls_x509_crt_parse(&tcpTlsCtx->cliCert, (const unsigned char *)certBufAligned, mbedtls_test_cli_crt_ec_len);
#if !defined MBEDTLS_V3_1
				mbedtls_pk_parse_key(&tcpTlsCtx->pkey, (const unsigned char *)mbedtls_test_cli_key_ec, mbedtls_test_cli_key_ec_len, NULL, 0);
#else
				mbedtls_pk_parse_key(&tcpTlsCtx->pkey, (const unsigned char *)mbedtls_test_cli_key_ec, mbedtls_test_cli_key_ec_len, NULL, 0, mbedtls_ctr_drbg_random, &tcpTlsCtx->ctr_drbg);
#endif
				//break;
			}
		}
		
		mbedtls_ssl_conf_own_cert(&tcpTlsCtx->config, &tcpTlsCtx->cliCert, &tcpTlsCtx->pkey);
		dbg_msg("%s", "[MBEDTLS] cipher init Done\r\n");
		/*if(mbedtls_x509_crt_parse(&tcpTlsCtx->cliCert, (const unsigned char *)tcpTlsCtx->x509Certificate, tcpTlsCtx->x509CertificateLen) != 0)
		{	dbgMsg("%s", YELLOW"tcp client TLS x509 crt parse fail!!\r\n"ORG_COLOR);
			goto INIT_ERROR;
		}*/		

#ifndef MBEDTLS_V3_1
		if(mbedtls_ctr_drbg_seed(&tcpTlsCtx->ctr_drbg, mbedtls_entropy_func, &tcpTlsCtx->entropy, (const unsigned char *)pers, strlen(pers)) != 0)
		{	dbgMsg("%s", YELLOW"tcp client TLS crt drbg seed fail!!\r\n"ORG_COLOR);
			goto INIT_ERROR;
		}
#endif
		
#ifndef MBEDTLS_V3_1
		if(mbedtls_ssl_config_defaults(&tcpTlsCtx->config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0)
		{	dbgMsg("%s", YELLOW"tcp client TLS ssl config default fail!!\r\n"ORG_COLOR);
			goto INIT_ERROR;
		}
#endif
		
		mbedtls_ssl_conf_max_version(&tcpTlsCtx->config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3); // v1.2
		mbedtls_ssl_conf_min_version(&tcpTlsCtx->config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3); // v1.2
		mbedtls_ssl_conf_ciphersuites(&tcpTlsCtx->config, tls_cipher_suites);
		
		mbedtls_ssl_conf_ca_chain(&tcpTlsCtx->config, tcpTlsCtx->cliCert.next, NULL);
		//strictly ensure that certificates are signed by the CA
		//mbedtls_ssl_conf_authmode(&tcpTlsCtx->config, MBEDTLS_SSL_VERIFY_REQUIRED);
		mbedtls_ssl_conf_authmode(&tcpTlsCtx->config, MBEDTLS_SSL_VERIFY_NONE);
		mbedtls_ssl_conf_rng(&tcpTlsCtx->config, mbedtls_ctr_drbg_random, &tcpTlsCtx->ctr_drbg);		
		/*mbedtls_ssl_conf_own_cert(&tcpTlsCtx->config, &tcpTlsCtx->cliCert, &tcpTlsCtx->pkey);
		dbg_msg("%s", "[MBEDTLS] cipher init Done\r\n");*/
		//mbedtls_ssl_set_hostname(&tcpTlsCtx->ssl, tcpTlsCtx->hostname);
		//mbedtls_ssl_session_init(&tcpTlsCtx->session);
		//mbedtls_ssl_set_session(&tcpTlsCtx->ssl, &tcpTlsCtx->ssn);
		tcpTlsCtx->tlsStatus = TLS_STATE_INITIALIZED;
	}
	
	dbgMsg("%s", GREEN"mbedtls init pass\r\n"ORG_COLOR);
	return 0;
INIT_ERROR:
	dbgMsg("%s", RED"mbedtls init pass\r\n"ORG_COLOR);
	mbedtlsUninit(tcpTlsCtx);
	return -1;
}
//------------------------------------------------------------------------------
void tcpTlsShutdown(mbedtls_ssl_context *ssl)
//void tcpTlsShutdown(TCP_TLS_CTX *tcpTlsCtx)
{	if(ssl)
	{	/* Shuts down an active SSL/TLS connection */
		mbedtls_ssl_close_notify(ssl);
		/* Free TLS socket.*/
		mbedtls_ssl_free(ssl);
		//httpsrv_mem_free(tls_sock);
	}
}
//------------------------------------------------------------------------------
int tcpTlsConnect(TCP_TLS_CTX *tcpTlsCtx, int sockFd)
{	int ret;
	mbedtls_ssl_init(&tcpTlsCtx->ssl);
		
	if(mbedtls_ssl_setup(&tcpTlsCtx->ssl, &tcpTlsCtx->config) != 0)
	{	dbgMsg("%s", RED"tcp client TLS ssl setup fail!!\r\n"ORG_COLOR);
		goto CONN_ERROR;
	}
		
	mbedtls_ssl_set_bio(&tcpTlsCtx->ssl, (void *)sockFd, tcpCliMbedtlsSend, tcpCliMbedtlsRecv, NULL);
	mbedtls_ssl_set_hostname(&tcpTlsCtx->ssl, tcpTlsCtx->hostname);
	dbg_msg("host name is %s\r\n", tcpTlsCtx->hostname);
	
	ret = mbedtls_ssl_handshake(&tcpTlsCtx->ssl);
	if(ret < 0)
	{	//tcpTlsShutdown(&tcpTlsCtx);
		char err_buf[128];
		mbedtls_strerror(ret, err_buf, sizeof(err_buf));
		dbg_msg("TLS handshake failed: -0x%x (%s)\r\n", -ret, err_buf);
	}
	return ret;
CONN_ERROR:
	
	return -1;
}
//------------------------------------------------------------------------------
int tcpTlsRead(mbedtls_ssl_context *sslSock, void *buf, size_t len)
{	return mbedtls_ssl_read(sslSock, buf, len);
}
//------------------------------------------------------------------------------
int tcpTlsWrite(mbedtls_ssl_context *sslSock, void *buf, size_t len)
{	return mbedtls_ssl_write(sslSock, buf, len);
}
#ifdef MBEDTLS_V3_1
void myMbedTlsExit(int status)
{	(void)status;
}
#endif


#endif