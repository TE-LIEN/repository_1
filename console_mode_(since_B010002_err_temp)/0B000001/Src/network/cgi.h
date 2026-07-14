#ifndef __CGI_H__
#define __CGI_H__

#include "sensminiCfg.h"

#define NORMAL_CGI_TIMEOUT	600000
#define EXT_CGI_TIMEOUT		600000
#define HTTP_NODE_MAX_SIZE	2048

#if defined NET_RTCS
	#define FSL_HTTP
	#include <httpsrv.h>
	#include <httpsrv_prv.h>  //locate at 
	#define NEW_HTTP_STRUCT
	#define HTTPD_SESSION_STRUCT HTTPSRV_CGI_REQ_STRUCT
#elif defined NET_LWIP
	#if defined HTTP_USE_FSL_LWIP	//XILINX use pre-process
	
	#define FSL_HTTP
	#define NEW_HTTP_STRUCT
	#include "network/httpsrv/httpsrv.h"
	#define HTTPD_SESSION_STRUCT HTTPSRV_CGI_REQ_STRUCT
	
	#elif defined HTTP_USE_NATIVE_LWIP
	
	#define NATIVE_HTTP
	#include "NET_APP/HTTPd/httpd_ana.h"
	//#define HTTPD_SESSION_STRUCT	char
	#endif
#endif

#define PSTART "&D="
#define P_KEY	 "&T="
#define SID_STR	"sid="

#if NCC_VERSION
#define DISABLE_KEY_CHK
#endif

#ifdef CGI_DISABLE_KEY_CHK
#define DISABLE_KEY_CHK
#endif

#define TEMPORARY_DISABLE_TIME	(10 * 60 * 1000)

#if defined DISABLE_KEY_CHK && (!defined NEW_HTTP_STRUCT)
#error "DISABLE_KEY_CHK must define NEW_HTTP_STRUCT"
#endif

#ifdef NATIVE_HTTP
extern const tCGI_ana ppcURLs[];

const char *cgi_GetCurrParam(char *param);
const char *cgi_GetCoef(char *param);
const char *cgi_GetDeviceParam(char *param);
const char *cgi_GetSDData(char *param);
const char *cgi_SaveModifiedParam(char *param);
const char *cgi_SaveModbusParam(char *param);
const char *cgi_SaveDCONParam(char *param);
const char *cgi_SetDeviceTime(char *param);
const char *cgi_SetDeviceTime1(char *param);
const char *cgi_GetDeviceTime(char *param);
const char *cgi_GetNodeParam(char *param);
const char *cgi_GetRealValue(char *param);
const char *cgi_ResetDevice(char *param);
const char *cgi_GetFWVersion(char *param);
const char *cgi_SetBootParam(char *param);
const char *cgi_GetDebug(char *param);
const char *cgi_Login(char *param);
const char *cgi_TimeExtend(char *param);
const char *cgi_UnTimeExtend(char *param);
#endif

#ifdef FSL_HTTP
_mqx_int cgi_GetCurrParam(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_GetCoef(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_GetDeviceParam(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_GetSDData(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_SaveModifiedParam(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_SavePassword(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_SaveModbusParam(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_SaveDCONParam(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_SetDeviceTime(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_SetDeviceTime1(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_GetDeviceTime(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_GetNodeParam(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_GetRealValue(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_ResetDevice(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_GetFWVersion(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_SetBootParam(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_GetDebug(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_Login(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_TimeExtend(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_UnTimeExtend(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_sendGetImage(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_checkImg(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_temporaryDisSlp(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_ForceSleep(HTTPD_SESSION_STRUCT *session);
#if NCC_VERSION
_mqx_int cgi_TestMode(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_GetTmSts(HTTPD_SESSION_STRUCT *session);
#endif
_mqx_int cgi_chgSta(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_chgSensor(HTTPD_SESSION_STRUCT *session);
_mqx_int cgi_setNewCoord(HTTPD_SESSION_STRUCT *session);

_mqx_int cgi_avdsCalibration(HTTPD_SESSION_STRUCT *session);

#ifndef NEW_HTTP_STRUCT
#define CGI_SEND_NUM(val)                         \
{                                                 \
    char str[20];                                 \
    SENS_SPRINTF((char_ptr)&str, "%ld\n", val);        \
    httpd_sendstr(session->sock, str);            \
}

#define CGI_SEND_STR(val)                         \
{                                                 \
    httpd_sendstr(session->sock, val);            \
    /*httpd_sendstr(session->sock, "\n");*/           \
}

#define CGI_SEND_STR2(val)                        \
{                                                 \
    httpd_sendstr(session->sock, val);            \
}
#endif

#endif

#define CGI_USE_METHOD1			1
#define KEY_SIZE		17
#define KEY_TO_BASE64_BYTES	16
#define KEY_BYTES						12
#define KEY_RANDON_BYTES		8
#define KEY_DATE_BYTES			4
#define MAX_SESSION_KEY			20

#define CGI_CHECK_SID	1

typedef struct _SessionKey
{	char *key;
	char isValid;
	int  startTime;
	int timeExtend;
#if CGI_CHECK_SID
	char *sessionId;
	//char isSuperUser;
	char authType;
#endif
}SessionKey, SESSION_KEY;

typedef struct _CONNECT_CTX
{	SessionKey *sessionKeys[MAX_SESSION_KEY];
	int vaildCnt;
}CONNECT_CTX;


#if defined FSL_HTTP
	#define	CHK_CGI_IS_VALID()		checkCgiIsValid(session, 0, sendData, &sesKey, __func__)
#if defined NEW_HTTP_STRUCT
	#define GET_START_DATA_PTR()	strstr(session->query_string, PSTART)
	#define	CGI_RETURN(x)			responseCgi(&response, session, x);	\
									return response.content_length
#else
	#define GET_START_DATA_PTR()	strstr(session->request.urldata, PSTART)
	#define	CGI_RETURN(x)			SENS_MEM_FREE(x);	\
									return session->request.content_len;
#endif
#elif defined NATIVE_HTTP
	#define	CHK_CGI_IS_VALID()		checkCgiIsValid(param, 0, sendData, __func__)
	#define GET_START_DATA_PTR()	strstr(param, PSTART)
	#define	CGI_RETURN(x)			return x

#define MAX_CGI_LEN	32
//extern void *fs_state_init(struct fs_file *file, const char *name);
//extern void fs_state_free(struct fs_file *file, void *state);
#endif

#endif // __CGI_H__
