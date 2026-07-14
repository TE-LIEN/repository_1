#include "cgi.h"
#include "math.h"

#include "sensCommon.h"
#include "sensSystem.h"
#include "param.h"
#include "sensDev.h"
#include "physicalQuantity.h"
#include "sdCardTask.h"
#include "sensors/sensorApp.h"
#include "driver/ad7124Adc.h"
#include "rtcDateTime.h"
#if AUTO_DATA_SYNC
#include "AutoDataSync.h"
#endif
#include "sensors/sensorApp.h"

#if defined PLATFORM_FSL
#include "FSL_MMCAU/ana_base64.h"
#include "APP/FSL/Boot.h"
#endif

#include "network/netApp.h"

#ifdef PLATFORM_XILINX
#include "FSL_MMCAU/ana_base64.h"
#include "APP/XILINX/RTC.h"
#include "trng.h"	//must use V02 wrapper
#define HW_TRNG
#endif

#ifdef SENSMINIA4_NEO
#include <stddef.h>
//#include "network/httpsrv/httpsrv_base64.h"
#include "cryptoMisc.h"
#define NUVOTON_TRNG
#endif

#ifdef NATIVE_HTTP
//#include "lwip/apps/fs.h"
#include "NET_APP/HTTPd/fs.h"
//#include "NET_APP/HTTPd/httpd_ana.h"
#endif

#include "network/protocol/serverProtocol.h"

CONNECT_CTX *connextCtx = NULL;

#ifdef PLATFORM_FSL
#if AUTO_CHECK_SRAM
extern uint8_t isGoodSram;
#endif
#endif

#if defined FSL_HTTP

#if defined NEW_HTTP_STRUCT
const HTTPSRV_CGI_LINK_STRUCT cgi_lnk_tbl[] = {
#else
const HTTPD_CGI_LINK_STRUCT cgi_lnk_tbl[] = {
#endif
#elif defined NATIVE_HTTP
const tCGI_ana ppcURLs[23] = {
#endif
  { "SaveModifiedParam"	,	cgi_SaveModifiedParam	},                            // save-1
  { "SaveParam"			,	cgi_SaveModifiedParam	},                            // save-2
  { "SaveNodeParam"		,	cgi_SaveModifiedParam	},                            // save-3
  { "SaveDOParam"		,	cgi_SaveModifiedParam	},                            // save-4
  { "SaveFormulaParam"	,	cgi_SaveModifiedParam	},                            // save-7
  { "SavePassword"		,	cgi_SaveModifiedParam	},                            // save-8
  { "SaveDOMap"			, 	cgi_SaveModifiedParam	},							  // save-10
  { "GetCurrParam"		,	cgi_GetCurrParam		},                            // none
  { "GetDeviceParam"	,	cgi_GetDeviceParam		},                            // 1-Device Param
  { "GetSDData"			,	cgi_GetSDData			},                            // 1-history 2-parameter 3-pin map
  { "GetCoef"			,	cgi_GetCoef				},                            // 1-formula coef
  { "ResetDevice"		,	cgi_ResetDevice			},                            // none
  { "GetRealValue"		,	cgi_GetRealValue		},                            // none
  { "SetDeviceTime"		,	cgi_SetDeviceTime		},                            // none
  { "GetDeviceTime"		,	cgi_GetDeviceTime		},                            // none
  { "GetNodeParam"		,	cgi_GetNodeParam		},                            // none
  { "GetFWVersion"		,	cgi_GetFWVersion		},                            // none
  { "SetBootParam"		,	cgi_SetBootParam		},                            // none
  { "GetDebug"			,	cgi_GetDebug			},                            // none
  { "SetDeviceTime1"	,	cgi_SetDeviceTime		},                            // none
  { "Login"				,	cgi_Login				},
  { "Extend"			,	cgi_TimeExtend			},
  { "UnExtend"			,	cgi_UnTimeExtend		},
#if NCC_VERSION
  { "TestModeCgi"		,	cgi_TestMode			},
  { "getTmSts"			,	cgi_GetTmSts			},
#endif
	{"avdsCalibration"	,	cgi_avdsCalibration		},
	{"checkImg"			,	cgi_checkImg			},
	{"sendGetImage"		,	cgi_sendGetImage		},
	{"temporaryDisSlp"	,	cgi_temporaryDisSlp		},
#if (NCC_VERSION && EMI_TEST_VERSION) || defined TEST_FORCE_SLEEP
  { "forceSleep"		,	cgi_ForceSleep			},
#endif
	{"chgSta"			,	cgi_chgSta				},
	{"chgSensor"		,	cgi_chgSensor			},
	{"setNewCoord"		,	cgi_setNewCoord			},
	{ 0					,	0						}                             // end of table
};


enum Function_Id_enum
{	GET_CUR_PARAM,       // 0
	GET_DEV_TIME,        // 1
	GET_EXT_DEV_PARAM,   // 2
	GET_DATA_ARRAY_TP,   // 3
	GET_SD_DATA,         // 4
	GET_SD_PARAM_FILE,   // 5
	GET_NODE_MAP,        // 6
	GET_REAL_VALUE,      // 7
	GET_COEF_VALUE,      // 8
	GET_DEVICE_PARAM,    // 9
	GET_PASSWORD,        // 10
	GET_FW_VERSION,      // 11
	GET_COEF1_VALUE,     // 12
	GET_DEBUG,           // 13
	GET_NODE_MAP1,       // 14
	GET_NODE_MAP2,       // 15
	GET_NODE_MAP3,       // 16
	GET_NODE_MAP4,       // 17
	GET_CUR_PARAM2,      // 18
	GET_LOG,						 //19
	GET_SAVE,							// 20
	GET_TIME_EXTEND,			// 21
//#if NCC_VERSION
	GET_TEST_MODE_STS,		// 22
	GET_TEST_MODE_PROCESS,	// 23
//#endif
	GET_AVDS_CALIBRATION,			// 24
	GET_CHECK_IMAGE,			// 25
	GET_SEND_IMAGE,			// 26
	GET_TEMPORARY_DIS_SLP,		// 27
	GET_FORCE_SLEEP,
	GET_CHG_STA,
	GET_CHG_SENSOR,
	UN_AUTHENTICATED = 99,
	TIME_EXPIRE,
};
//------------------------------------------------------------------------------
void initialHttpConnectInfo(void)
{	int idx;
	SESSION_KEY *sessionKey;
	
	connextCtx = SENS_MEM_ZALLOC(sizeof(CONNECT_CTX));
	for(idx=0;idx<MAX_SESSION_KEY;idx++)
	{	connextCtx->sessionKeys[idx] = SENS_MEM_ZALLOC(sizeof(SESSION_KEY));
		sessionKey = connextCtx->sessionKeys[idx];
		sessionKey->key =  SENS_MEM_ZALLOC(KEY_SIZE);
	}	
}
//------------------------------------------------------------------------------
void removeExpireSessionKey(void)
{	SESSION_KEY *sessionKey;
	int idx;
	int currentTime;
	int timeout;
	
	currentTime = GetTimeTicks();
	
	for(idx=0;idx<MAX_SESSION_KEY;idx++)
	{	sessionKey = connextCtx->sessionKeys[idx];
			
		if(sessionKey->isValid)
		{	if(sessionKey->timeExtend)
				timeout = EXT_CGI_TIMEOUT;
			else
				timeout = NORMAL_CGI_TIMEOUT;
			if((currentTime - sessionKey->startTime) > timeout)
			{	sessionKey->isValid = 0;
				sessionKey->timeExtend = 0;
			}
		}
	}
}
//------------------------------------------------------------------------------
SESSION_KEY *replaceOldestSessionKey(void)
{	SESSION_KEY *sessionKey;
	int idx;
	//int currentTime;
	int minValue = 0xFFFFFFFF, minIdx;
	
	//currentTime = GetTimeTicks();
	
	for(idx=0;idx<MAX_SESSION_KEY;idx++)
	{	sessionKey = connextCtx->sessionKeys[idx];
		if(sessionKey->isValid && (sessionKey->startTime < minValue))
		{	minIdx = idx;
			minValue = sessionKey->startTime;
		}
	}
	
	sessionKey = connextCtx->sessionKeys[minIdx];
	sessionKey->isValid = 0;
	sessionKey->timeExtend = 0;
	return sessionKey;
}
//------------------------------------------------------------------------------
SESSION_KEY *findEmptySessionKey(void)
{	SESSION_KEY *sessionKey;
	int idx;
	
	for(idx=0;idx<MAX_SESSION_KEY;idx++)
	{	sessionKey = connextCtx->sessionKeys[idx];
		if(!sessionKey->isValid)
		{	break;
		}
	}
	
	if(idx < MAX_SESSION_KEY)
		return sessionKey;
	else
		return NULL;
}
//------------------------------------------------------------------------------
void generateKey(SESSION_KEY *sessionKey)
{	unsigned int keySeed = 0;
	int idx;
	int currentTime;
	unsigned char randomStr[KEY_RANDON_BYTES+1];
	unsigned char keyStr[KEY_BYTES+1];
	unsigned char dateStr[KEY_DATE_BYTES+1];
	char *base64EncodeValue;

#ifdef NUVOTON_TRNG
	uint8_t trngArray[KEY_RANDON_BYTES/2];
	size_t trngOutLen;
#endif
	
#if defined PLATFORM_XILINX
	DATE_STRUCT date1;
	TIME_STRUCT rtctime;
#endif
#ifndef NUVOTON_TRNG
	SYS_SEM *sysSem = (SYS_SEM *)&sensSys->sysSem;
	SENS_SEM_LOCK(sysSem->rngSem);
#endif
	currentTime = GetTimeTicks();
#if defined PLATFORM_FSL
#if (defined IS_MK64)
	SIM_SCGC6 |= SIM_SCGC6_RNGA_MASK;
#endif
	SIM_SCGC3 |= SIM_SCGC3_RNGA_MASK;                                             // Turn on RNGA module
  RNG_CR &= ~RNG_CR_SLP_MASK;                                                   // RNGA is not in sleep mode
  RNG_CR |= RNG_CR_HA_MASK;                                                     // security violations masked
  RNG_CR |= RNG_CR_GO_MASK;                                                     // output register loaded with data
  while((RNG_SR & RNG_SR_OREG_LVL(0xF)) == 0) {}
  
  keySeed = RNG_OR & 0x7FFFFFFF;
#elif defined PLATFORM_XILINX
#if defined HW_TRNG
	TRNG_get_valid(XPAR_TRNG_0_S00_AXI_BASEADDR);
#else
	get_date(&date1);
	rtc_time_from_date(&date1, &rtctime);
	get_date(&date1);
	rtc_time_from_date(&date1, &rtctime);
	keySeed = (unsigned int)rtctime.SECONDS;
#endif
#endif
#if !defined HW_TRNG && !defined NUVOTON_TRNG
	srand(keySeed);
#endif
	
	memset(sessionKey->key, 0, KEY_SIZE);
	memset(randomStr,  0, KEY_RANDON_BYTES+1);
	memset(keyStr,  0, KEY_BYTES+1);
	memset(dateStr,  0, KEY_DATE_BYTES+1);
	//randomStr = SENS_MEM_ZALLOC(KEY_RANDON_BYTES+1);
	//keyStr = SENS_MEM_ZALLOC(KEY_BYTES+1);
	//dateStr = SENS_MEM_ZALLOC(KEY_DATE_BYTES+1);
	SENS_SPRINTF((char *)dateStr, "%04X", currentTime);

#if defined NUVOTON_TRNG
	mbedtls_hardware_poll(NULL, trngArray, KEY_RANDON_BYTES/2, &trngOutLen);
#endif
	for(idx=0;idx<(KEY_RANDON_BYTES/2);idx++)
	{
#if defined NUVOTON_TRNG
		SENS_SPRINTF((char *)randomStr + strlen((char const *)randomStr), "%02X", trngArray[idx]);
#elif defined HW_TRNG
		SENS_SPRINTF((char *)randomStr + strlen((char const *)randomStr), "%02X", TRNG_get_data(XPAR_TRNG_0_S00_AXI_BASEADDR) % 255);
#else
		SENS_SPRINTF((char *)randomStr + strlen((char const *)randomStr), "%02X", rand() % 255);
#endif
	}
	
	strcat((char *)keyStr, (char *)randomStr);
	strcat((char *)keyStr, (char *)dateStr);
	
	base64EncodeValue = (char *)base64Encode((char *)keyStr);
	if(base64EncodeValue == NULL)
	{	dbg_msg("%s", "base 64 encode NULL\r\n");
	}

	memcpy(sessionKey->key, base64EncodeValue, KEY_TO_BASE64_BYTES);
	SENS_MEM_FREE(base64EncodeValue);
	//SENS_MEM_FREE(randomStr);
	//SENS_MEM_FREE(keyStr);
	//SENS_MEM_FREE(dateStr);
#ifdef PLATFORM_FSL
	SIM_SCGC3 &= ~SIM_SCGC3_RNGA_MASK;
	//dbg_msg("generate new Key: %s\r\n", sessionKey->key);
#if (defined IS_MK64)
	SIM_SCGC6 &= (~SIM_SCGC6_RNGA_MASK);
#endif
#endif
#ifndef NUVOTON_TRNG
	SENS_SEM_UNLOCK(sysSem->rngSem);
#endif
}

#if CGI_CHECK_SID
//------------------------------------------------------------------------------
SESSION_KEY *findConnectInfoFromId(char *sessionId)
{	SESSION_KEY	*sessionKey;
	int idx;
	
	if(connextCtx == NULL)
		return NULL;
	for(idx=0;idx<MAX_SESSION_KEY;idx++)
	{	sessionKey = connextCtx->sessionKeys[idx];
		if(sessionKey->isValid && (!memcmp(sessionKey->sessionId, sessionId, strlen(sessionId))))
		{	return sessionKey;
		}
	}
	
	return NULL;
}

#else
//------------------------------------------------------------------------------
SESSION_KEY *findConnectInfo(char *key)
{	SESSION_KEY	*sessionKey;
	int idx;
	int currentTime;
	int timeout;
	
	if(connextCtx == NULL)
		return NULL;
	currentTime = GetTimeTicks();
	for(idx=0;idx<MAX_SESSION_KEY;idx++)
	{	sessionKey = connextCtx->sessionKeys[idx];
		if(sessionKey->isValid && (!memcmp(sessionKey->key, key, KEY_TO_BASE64_BYTES)))
		{	if(sessionKey->timeExtend)
				timeout = EXT_CGI_TIMEOUT;
			else
				timeout = NORMAL_CGI_TIMEOUT;
			if((currentTime - sessionKey->startTime) > timeout)
			{	sessionKey->isValid = 0;
				dbg_msg("%s\r\n", "key expire!!");
				return NULL;
			}
			return sessionKey;
		}
	}
	return NULL;
}
#endif
//------------------------------------------------------------------------------
/*int checkConnectExpire(CONNECT_CTX *ctx)
{	int currentTime;
	
	currentTime = GetTimeTicks();
	
	if((currentTime - ctx->startTime) > 30000)
	{	return -1;
	}
	return 0;
}*/
//------------------------------------------------------------------------------
void updateExpireTime(SESSION_KEY *sessionKey)
{	sessionKey->startTime = GetTimeTicks();
}
//------------------------------------------------------------------------------
SESSION_KEY *addConnectInfo(int sock)
{	SESSION_KEY *sessionKey;
	//int idx;
	int currentTime;
	
	if(connextCtx == NULL)
		initialHttpConnectInfo();
	
	sessionKey = findEmptySessionKey();
	if(sessionKey == NULL)
	{	removeExpireSessionKey();
		sessionKey = findEmptySessionKey();
		if(sessionKey == NULL)
		{	sessionKey = replaceOldestSessionKey();
		}
	}
	currentTime = GetTimeTicks();
	
	sessionKey->startTime = currentTime;
	sessionKey->isValid = 1;
	generateKey(sessionKey);
	connextCtx->vaildCnt++;
	
	return sessionKey;
}
//------------------------------------------------------------------------------
SESSION_KEY *checkConnectValid(char *key, int *status, char *sessionId)
{	SESSION_KEY	*sessionKey = NULL;
	int currentTime;
	int timeout;
	
	if(connextCtx == NULL)
		return NULL;
	*status = 0;		
#if CGI_CHECK_SID
	
	if(sessionId != NULL)
	{	sessionKey = findConnectInfoFromId(sessionId);
		if(sessionKey == NULL)
		{	
#ifdef DISABLE_KEY_CHK
	#if defined NEW_HTTP_STRUCT || defined NATIVE_HTTP
			sessionKey = addConnectInfo(0);
	#else
    	//sessionKey = addConnectInfo(session->sock);
	#endif
			return sessionKey;
#else
			*status = -1;
			return NULL;
#endif
		}
		else
		{	currentTime = GetTimeTicks();
			if(sessionKey->authType == LOGIN_ID_SU)
			{	*status = 0;
				return sessionKey;
			}
			
#ifdef DISABLE_KEY_CHK
			*status = 0;
			return sessionKey;
#else
			if(!memcmp(sessionKey->key, key, KEY_TO_BASE64_BYTES))
			{	if(sessionKey->timeExtend)
					timeout = EXT_CGI_TIMEOUT;
				else
					timeout = NORMAL_CGI_TIMEOUT;
				if((currentTime - sessionKey->startTime) > timeout)
				{	sessionKey->isValid = 0;
					*status = -1;
					dbg_msg("%s\r\n", "key expire!!");
					return NULL;
				}
			}
#endif
		}
		*status = 0;
	}
#else
	sessionKey = findConnectInfo(key);
	if(!sessionKey)
	{	*status = -1;
	}
#endif

	return sessionKey;
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
int checkCgiIsValid(HTTPD_SESSION_STRUCT *session, int timeExtend, char *sendData, SESSION_KEY **sesKey, const char *func)
#elif defined NATIVE_HTTP
int checkCgiIsValid(char *param, int timeExtend, char *sendData, const char *func)
#endif
{	int connectState = -1;
	char key[KEY_SIZE];
#if CGI_CHECK_SID
	char sessionId[10];
	char *sidStart, *sidEnd;
	int sidLength;
#endif
	SESSION_KEY	*sessionKey;
	char *keyStart;
	char *keyEnd;
	int keyLength;
#if defined FSL_HTTP
	char *param;
	#if defined NEW_HTTP_STRUCT
	param = session->query_string;
	#else
	param = session->request.urldata;
	#endif
#endif
	keyStart = strstr(param, P_KEY);
	keyEnd = strstr(param, PSTART);
#if CGI_CHECK_SID
	sidStart = strstr(param, SID_STR);
	sidEnd = strstr(param, P_KEY);
	
	if(sidEnd != NULL && sidStart != NULL)
	{	sidLength = ((int)sidEnd - (int)sidStart - strlen(SID_STR));
		memset(sessionId, 0, 10);
	 	memcpy(sessionId, sidStart+strlen(SID_STR), sidLength);
	}
	
#endif

	if(keyStart != NULL && keyEnd != NULL)
	{	keyLength = ((int)keyEnd - (int)keyStart - strlen(P_KEY));
  
		if(keyLength == KEY_TO_BASE64_BYTES)
		{	memset(key, 0, KEY_SIZE);
			memcpy(key, keyStart+strlen(P_KEY), keyLength);
			sessionKey = checkConnectValid(key, &connectState,
#if CGI_CHECK_SID
																		sessionId
#else
																		NULL
#endif
  																	);
		}
		else
		{	dbg_msg("error key length:%d\r\n", keyLength);
		}
	}

	if(connectState == -1)
	{	dbg_msg("Un-authorized from func:%s\r\n", func);
#if defined FSL_HTTP
#if defined NEW_HTTP_STRUCT
		SENS_SPRINTF(sendData, "Unauthorized\n");
#else
		CGI_SEND_STR("Unauthorized");
#endif
#elif defined NATIVE_HTTP
		SENS_SPRINTF(sendData, "Unauthorized\n");
#endif
		//return session->request.content_len;
	}
	else
	{	generateKey(sessionKey);
		sessionKey->startTime = GetTimeTicks();
		if(timeExtend == 1)
			sessionKey->timeExtend = 1;
		else if(timeExtend == 2)
			sessionKey->timeExtend = 0;
		//httpd_sendstr(session->sock, P_KEY);
		//CGI_SEND_STR(sessionKey->key);
		SENS_SPRINTF(sendData, "%s%s\n", P_KEY, sessionKey->key);
	}
	*sesKey = sessionKey;
	return connectState;
}
#if defined NEW_HTTP_STRUCT
//------------------------------------------------------------------------------
void responseCgi(HTTPSRV_CGI_RES_STRUCT *response, HTTPD_SESSION_STRUCT *session, char *sendData)
{	response->ses_handle = session->ses_handle;
  response->content_type = HTTPSRV_CONTENT_TYPE_PLAIN;
  response->status_code = HTTPSRV_CODE_OK;
  response->data = sendData;
  response->data_length = strlen(sendData);
  response->content_length = response->data_length;
  HTTPSRV_cgi_write(response);
  SENS_MEM_FREE(sendData);
}
#endif
//------------------------------------------------------------------------------
#if defined FSL_HTTP
//------------------------------------------------------------------------------
_mqx_int cgi_SaveModifiedParam(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_SaveModifiedParam(char *param)
#endif
{
#ifdef FSL_HTTP
	char *param;
#endif
	SESSION_KEY *sesKey = NULL;
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char *sendData;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	// Find position after PSTART("&D="), after which the parameter begins.
	
	dbg_msg("%s", "SaveModifiedParam.cgi process!!\r\n");
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);

	if(sesKey->authType != LOGIN_ID_GUEST)
	{	if(UpdateAndSaveParam(param))
		{	dbg_msg("[==WEB==] %s\r\n", "RESET");
			sysCtrl->isReadyToReboot = 1;
		}
	}
	//dbg_msg("%s", "SaveModifiedParam.cgi done!!\r\n");
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_SAVE);
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
static void setSysCfgToWeb(char *sendData, char *sep)
{	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	char a[30];
	
	strcat(sendData, my_ftoa(a, sensSysCfg->recordSec, 0));	//0
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensSysCfg->autoSendInterval, 0));	//1
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensSysCfg->WakeUpInterval, 0));	//2
	strcat(sendData, sep);
	//strcat(sendData, sensSys->bat_off_string);	//3
	strcat(sendData, "11.0");	//4
	strcat(sendData, sep);
	//strcat(sendData, sensSys->bat_on_diff_string);	//4
	strcat(sendData, "1.0");	//5
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensSysCfg->psm, 0));	//5
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensSysCfg->logToSd, 0));	//6
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensSysCfg->alertEnable, 0));	//7
	strcat(sendData, sep);
	//strcat(sendData, sensDev->Alert_Value_string);	//9
	strcat(sendData, my_ftoa(a, sensSysCfg->alertValue, 3));	//8
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensSysCfg->sysIsInstallMode, 0));	//9
	strcat(sendData, sep);
#if SUPPORT_EXT_LOW_VOLT
	strcat(sendData, my_ftoa(a, sysParams->extLvdEnable, 0));	//10
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sysParams->extLvdLevel, 3));	//11
#else
	strcat(sendData, "0");	//10
	strcat(sendData, sep);
	strcat(sendData, "0");	//11
#endif
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensSysCfg->autoFotaChk, 0));	//12
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensSysCfg->imgServerType, 0));	//13
	strcat(sendData, sep);
	strcat(sendData, "\n");
}
//------------------------------------------------------------------------------
static void setWirelessCfgToWeb(char *sendData, char *sep)
{	NET_CONFIG		*netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	WIRELESS_CONFIG *wirelessCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
	MOBILE_CONFIG *lteCfg, *nbCfg;
	char a[30];
	uint32_t base64OutLen;
	char *base64EncodeValue;
	char *tempStr;
	
	//wireless
	strcat(sendData, my_ftoa(a, wirelessCfg->channelType[0], 0));	//0
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, wirelessCfg->channelType[1], 0));	//1
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, wirelessCfg->mobileInterval, 0));	//2
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, wirelessCfg->mdvpnType, 0));	//3
	strcat(sendData, sep);
	lteCfg = wirelessCfg->lteCfg;
	nbCfg = wirelessCfg->nbCfg;
	if((lteCfg == NULL) || (lteCfg->apn == NULL))
		strcat(sendData, "internet");	//4
	else
		strcat(sendData, lteCfg->apn);	//4
	strcat(sendData, sep);
	if((nbCfg == NULL) || ((nbCfg->apn == NULL)))
		strcat(sendData, "internet.iot");	//5
	else
		strcat(sendData, nbCfg->apn);	//5
	strcat(sendData, sep);
	if(nbCfg == NULL)
		strcat(sendData, "46692");	//6
	else
		strcat(sendData, my_ftoa(a, nbCfg->plmn, 0));	//6
	strcat(sendData, sep);
	if(lteCfg && lteCfg->simAccount)
	{	tempStr = SENS_MEM_ZALLOC(strlen(lteCfg->simAccount) + 1);
		strcpy(tempStr, lteCfg->simAccount);
	}
	else
	{	tempStr = SENS_MEM_ZALLOC(strlen("mobile") + 1);
		strcpy(tempStr, "mobile");
	}
	base64EncodeValue = (char *)base64Encode(tempStr);
    strcat(sendData, base64EncodeValue);	//7
    SENS_MEM_FREE(base64EncodeValue);
	SENS_MEM_FREE(tempStr);
    strcat(sendData, sep);
	if(lteCfg && lteCfg->simPassword)
	{	tempStr = SENS_MEM_ZALLOC(strlen(lteCfg->simPassword) + 1);
		strcpy(tempStr, lteCfg->simPassword);
	}
	else
	{	tempStr = SENS_MEM_ZALLOC(strlen("mobile") + 1);
		strcpy(tempStr, "mobile");
	}
    base64EncodeValue = (char *)base64Encode(tempStr);

    strcat(sendData, base64EncodeValue);	//8
    SENS_MEM_FREE(base64EncodeValue);
	SENS_MEM_FREE(tempStr);
    strcat(sendData, sep);
	if(lteCfg == NULL)
		strcat(sendData, "0");
	else
		strcat(sendData, my_ftoa(a, lteCfg->simAuth, 0));	//9
    strcat(sendData, sep);
	#if defined SUPPORT_ONBOARD_WISUN
    strcat(sendData, my_ftoa(a, sensSys->enOnboardMesh, 0));	//10
	#else
    strcat(sendData, "0");	//10
	#endif
	strcat(sendData, sep);
	
	//AGPS Enable
	strcat(sendData, my_ftoa(a, wirelessCfg->agpsEnable, 0));	//11
    strcat(sendData, sep);
}
//------------------------------------------------------------------------------
static void setAgpsCfgToWeb(char *sendData, char *sep)
{	NET_CONFIG		*netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	WIRELESS_CONFIG *wirelessCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
	LTE_AGPS_CONFIG	*lteAgpsCfg = wirelessCfg->lteAgpsCfg;
	NB_AGPS_CONFIG	*nbAgpsCfg = wirelessCfg->nbAgpsCfg;
	char a[30];
	
	if(nbAgpsCfg)
	{	strcat(sendData, my_ftoa(a, nbAgpsCfg->me310AgpsType, 0));	//0
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, nbAgpsCfg->me310ColdStart, 0));	//1
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, nbAgpsCfg->me310HotStart, 0));	//2
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, nbAgpsCfg->me310PollInterval, 0));	//3
		strcat(sendData, sep);
	}
	else
	{	strcat(sendData, "0");	//0
		strcat(sendData, sep);
		strcat(sendData, "0");	//1
		strcat(sendData, sep);
		strcat(sendData, "0");	//2
		strcat(sendData, sep);
		strcat(sendData, "0");	//3
		strcat(sendData, sep);
	}
	if(lteAgpsCfg)
    {	strcat(sendData, my_ftoa(a, lteAgpsCfg->le910AgpsType, 0));	//4
		strcat(sendData, sep);
		strcat(sendData, (char *)lteAgpsCfg->suplServer);	//5
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, lteAgpsCfg->suplPort, 0));	//6
		strcat(sendData, sep);
	}
	else
	{	strcat(sendData, "0");	//4
		strcat(sendData, sep);
		strcat(sendData, "supl.google.com");	//5
		strcat(sendData, sep);
		strcat(sendData, "7276");	//6
		strcat(sendData, sep);
	}
}
//------------------------------------------------------------------------------
static void setMeshCfgToWeb(char *sendData, char *sep)
{	//lorawan
#if defined SUPPORT_MESH
	SENS_SPRINTF(sendData + strlen(sendData), "%d", sensSys->loraWanCh0Freq);	//0
	strcat(sendData, sep);
	if(sensSys->loraWanDevEui)	strcat(sendData, sensSys->loraWanDevEui);	//1
	else												strcat(sendData, "0");
    strcat(sendData, sep);
    if(sensSys->loraWanAppEui)	strcat(sendData, sensSys->loraWanAppEui);	//2
    else												strcat(sendData, "0");
    strcat(sendData, sep);
    if(sensSys->loraWanAppKey)	strcat(sendData, sensSys->loraWanAppKey);	//3
    else												strcat(sendData, "0");
    strcat(sendData, sep);
    if(sensSys->loraWanDevAddr)	strcat(sendData, sensSys->loraWanDevAddr);	//4
    else												strcat(sendData, "0");
    strcat(sendData, sep);
    
    strcat(sendData, my_ftoa(a, sensSys->meshNodeCnt, 0));	//5
    strcat(sendData, sep);
    strcat(sendData, my_ftoa(a, sensSys->pqCntPerNode, 0));	//6
    strcat(sendData, sep);
    if(sensSys->meshNetName)	strcat(sendData, sensSys->meshNetName);	//7
	else											strcat(sendData, "0");	//7
    strcat(sendData, sep);
    strcat(sendData, my_ftoa(a, sensSys->meshFreq, 0));	//8
#else
	strcat(sendData, "0");	//0, loraWanCh0Freq	
	strcat(sendData, sep);
	strcat(sendData, "0");	//1, loraWanDevEui
	strcat(sendData, sep);	
	strcat(sendData, "0");	//2, loraWanAppEui
	strcat(sendData, sep);	
	strcat(sendData, "0");	//3, loraWanAppKey
	strcat(sendData, sep);
	strcat(sendData, "0");	//4, loraWanDevAddr
	strcat(sendData, sep);
	strcat(sendData, "0");	//5, meshNodeCnt
	strcat(sendData, sep);
	strcat(sendData, "0");	//6, pqCntPerNode
	strcat(sendData, sep);
	strcat(sendData, "0");	//7, meshNetName
	strcat(sendData, sep);
	strcat(sendData, "0");	//8, meshFreq
#endif
}
//------------------------------------------------------------------------------
static void setNetworkCfgToWeb(char *sendData, char *sep)
{	NET_CONFIG		*netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	WIRED_CONFIG	*wiredCfg = (WIRED_CONFIG *)&netCfg->wiredCfg;
	DNS_CONFIG		*dnsCfgs = (DNS_CONFIG *)netCfg->dnsCfgs;
	OTA_CONFIG		*otaCfg;
	char a[30];
	
	strcat(sendData, my_ftoa(a, netCfg->connPriority, 0));	//0
	strcat(sendData, sep);
    //wired
	SENS_SPRINTF(sendData+ strlen(sendData), "%ld", wiredCfg->ipAddr);	//1
	strcat(sendData, sep);
	SENS_SPRINTF(sendData+ strlen(sendData), "%ld", wiredCfg->maskAddr);	//2
	strcat(sendData, sep);
	SENS_SPRINTF(sendData+ strlen(sendData), "%ld", wiredCfg->gwAddr);	//3
	strcat(sendData, sep);
	SENS_SPRINTF(sendData+ strlen(sendData), "%ld", dnsCfgs->dnsSvrAddr);	//4
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, wiredCfg->mdvpnType, 0));	//5
	strcat(sendData, sep);
	strcat(sendData, "\n");
	//wirelwss
	setWirelessCfgToWeb(sendData, sep);
	if(netCfg->otaCfg)
	{	otaCfg = netCfg->otaCfg;
		strcat(sendData, otaCfg->domainname);	//12
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, otaCfg->port, 0));	//13
		strcat(sendData, sep);
	}
	else
	{	strcat(sendData, "0");	//12
		strcat(sendData, sep);
		strcat(sendData, "0");	//13
		strcat(sendData, sep);
	}
	strcat(sendData, "\n");
	setAgpsCfgToWeb(sendData, sep);
	strcat(sendData, "\n");
	setMeshCfgToWeb(sendData, sep);
}
//------------------------------------------------------------------------------
static inline void setEmptyMqttItemCfg(char *sendData, char *sep)
{	strcat(sendData, "TmFO");
	strcat(sendData, sep);
}
//------------------------------------------------------------------------------
static inline void setEmptyMqttCfg(char *sendData, char *sep)
{	setEmptyMqttItemCfg(sendData, sep);	//6, 11, clientId		IOA_equipId
	setEmptyMqttItemCfg(sendData, sep);	//7, 12, mqttUserName	IOA_apiKey
	setEmptyMqttItemCfg(sendData, sep);	//8, 13, mqttPassword
	setEmptyMqttItemCfg(sendData, sep);	//9, 14, mqttPubTopic
	setEmptyMqttItemCfg(sendData, sep);	//10, 15, mqttSubTopic
}
//------------------------------------------------------------------------------
static inline void setEmptyServerCfg(char *sendData, char *sep)
{	strcat(sendData, "0");	//0
	strcat(sendData, sep);
	strcat(sendData, "0");	//1
	strcat(sendData, sep);
	strcat(sendData, "99");	//2
	strcat(sendData, sep);
}
//------------------------------------------------------------------------------
static void setMqttItemCfgToWeb(char *cfgStr, char *sendData, char *sep)
{	char *base64EncodeValue;
	uint32_t base64OutLen;
	base64EncodeValue = (char *)base64Encode(cfgStr);
    strcat(sendData, base64EncodeValue);																		//6
	strcat(sendData, sep);
    SENS_MEM_FREE(base64EncodeValue);
}
//------------------------------------------------------------------------------
static void setMqttCfgToWeb(MQTT_CONNECT_INFO *mqttConnInfo, char *sendData, char *sep)
{	MQTT_TOPIC_CONFIG *topicCfg;
	
	if(mqttConnInfo && mqttConnInfo->clientId)
		setMqttItemCfgToWeb(mqttConnInfo->clientId, sendData, sep);
	else
		setEmptyMqttItemCfg(sendData, sep);
	if(mqttConnInfo && mqttConnInfo->userName)
		setMqttItemCfgToWeb(mqttConnInfo->userName, sendData, sep);
	else
		setEmptyMqttItemCfg(sendData, sep);
	if(mqttConnInfo && mqttConnInfo->password)
		setMqttItemCfgToWeb(mqttConnInfo->password, sendData, sep);
	else
		setEmptyMqttItemCfg(sendData, sep);
	if(mqttConnInfo && mqttConnInfo->pubTopics)
	{	topicCfg = mqttConnInfo->pubTopics;
		setMqttItemCfgToWeb(topicCfg->topic, sendData, sep);
	}
	else
		setEmptyMqttItemCfg(sendData, sep);
	if(mqttConnInfo && mqttConnInfo->subTopics)
	{	topicCfg = mqttConnInfo->subTopics;
		setMqttItemCfgToWeb(topicCfg->topic, sendData, sep);
	}
	else
		setEmptyMqttItemCfg(sendData, sep);
}
//------------------------------------------------------------------------------
/*static void setIoaCfgToWeb(IOA_CONNECT_INFO *ioaConnInfo, char *sendData, char *sep)
{	if(ioaConnInfo && ioaConnInfo->equipId)
		setMqttItemCfgToWeb(ioaConnInfo->equipId, sendData, sep);
	else
		setEmptyMqttItemCfg(sendData, sep);
}*/
//------------------------------------------------------------------------------
static void setServCfgToWeb(char *sendData, char *sep)
{	CLOUD_SERVER_INFO *servInfo;
	char a[30];
	int idx;
	
	for(idx=0;idx<UPLOAD_SERVER_CNT;idx++)
	{	servInfo = getServerInfoById(idx);
		if(servInfo && (servInfo->serverProtocol != MQTT_YS_BROKER) && (servInfo->serverProtocol != MQTTS_YS_BROKER))
		{	strcat(sendData, servInfo->serverDomainName);				//0
			strcat(sendData, sep);
			strcat(sendData, my_ftoa(a, servInfo->serverPort, 0));	//1
			strcat(sendData, sep);
			strcat(sendData, my_ftoa(a, servInfo->serverProtocol, 0));	//2
			strcat(sendData, sep);
		}
		else
			setEmptyServerCfg(sendData, sep);
	}
		
	for(idx=0;idx<UPLOAD_SERVER_CNT;idx++)
	{	servInfo = getServerInfoById(idx);
		if(servInfo && (servInfo->serverProtocol != MQTT_YS_BROKER) && (servInfo->serverProtocol != MQTTS_YS_BROKER))
		{
#if SUPPORT_IOA_WEB_API
			if(servInfo->serverProtocol == IOA_WEB_API_PROTOCOL)
			{	setMqttCfgToWeb((MQTT_CONNECT_INFO *)servInfo->servConnectInfo, sendData, sep);	//ioa and mqtt info is same
			}
			else
#endif
			{	setMqttCfgToWeb((MQTT_CONNECT_INFO *)servInfo->servConnectInfo, sendData, sep);
			}
		}
		else
		{	setMqttCfgToWeb(NULL, sendData, sep);
		}
	}
	strcat(sendData, sep);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_GetCurrParam(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_GetCurrParam(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	int parameter_num;
	char modbus_com[20], none[2], sep[2], a[20];
#ifdef FSL_HTTP
	char *param;
#endif
	char *p;
	char *base64EncodeValue;
  	char *sendData;
	SESSION_KEY *sesKey = NULL;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	memset(modbus_com, 0, sizeof(char)*20);
	strcpy(none, "");
	
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
	strcpy(sep, "|");
  
	strtokLock();
	p = strtok(param, sep);
	strtokUnLock();
	parameter_num = _io_atoi(p);

	dbg_msg("%s", "[==WEB==] Get CurrentParam Command\r\n");	

	if(parameter_num == 1)
	{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_CUR_PARAM);
		//SYSTEM
		setSysCfgToWeb(sendData, sep);
		//network
		setNetworkCfgToWeb(sendData, sep);
		strcat(sendData, sep);
		strcat(sendData, "\n");		
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
#else
		//CGI_SEND_NUM(GET_CUR_PARAM);
		CGI_SEND_STR(sendData);
#endif
#endif
	}
	else if(parameter_num == 2)
	{	COMPOSITE_OSA_CONFIG	*compositeOsaCfg;
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_CUR_PARAM2);
		//server
		setServCfgToWeb(sendData, sep);
		strcat(sendData, "\n");
		//speciall function 
		SPEC_FEATURE_CONFIG	*specFeas = (SPEC_FEATURE_CONFIG *)&sysCfg->specFeas;
		strcat(sendData, my_ftoa(a, specFeas->isPressureWaterLevel, 0));	//0
		strcat(sendData, sep);
#if SUPPORT_GATE_CONTROL
		strcat(sendData, my_ftoa(a, specFeas->isGateCtrlMode, 0));	//1
#else
		strcat(sendData, "0");	//1
#endif
		strcat(sendData, sep);
#if SUPPORT_YL_PUMP
		strcat(sendData, my_ftoa(a, specFeas->isYlPumpMode, 0));	//2
#else
		strcat(sendData, "0");	//2
#endif
		strcat(sendData, sep);
#if SUPPORT_CY_PUMP
		strcat(sendData, my_ftoa(a, specFeas->isCyPumpMode, 0));	//3
#else
		strcat(sendData, "0");		//3
#endif
		strcat(sendData, sep);
		//compositeOsaCfg = (COMPOSITE_OSA_CONFIG *)getExtDevSpeConfig(EXT_DEV_MODEL_COMPOSITE_OSA24G);	//sysCfg->compositeOsaCfg;
		if(sysCfg->compositeOsaCfg)
		{	compositeOsaCfg = sysCfg->compositeOsaCfg;
			if(compositeOsaCfg->radarDevNo && compositeOsaCfg->wlsDevNo)
				strcat(sendData, "0");	//4
			else if(compositeOsaCfg->radarDevNo)
				strcat(sendData, "1");	//4
			else if(compositeOsaCfg->wlsDevNo)
				strcat(sendData, "2");	//4
		}
		else
			strcat(sendData, "0");	//4
		strcat(sendData, sep);
#if SUPPORT_CY_PUMP
		strcat(sendData, my_ftoa(a, sysParams->gpsPositionRstTime[0], 0));	//5
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, sysParams->gpsPositionRstTime[1], 0));	//6
#else
		strcat(sendData, "0|0");	//5 6
#endif
		strcat(sendData, sep);
		strcat(sendData, "\n");
		//cy pump config
		
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
#else
		CGI_SEND_STR(sendData);
#endif
#endif
	}
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_GetDeviceParam(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_GetDeviceParam(char *param)
#endif
{	VW_SENSOR_CONFIG	*vwCfg;
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char sep[2];
#ifdef FSL_HTTP
	char *param;
#endif
	char *p, select_number;
	int device_count, parameter_num;
	char a[20];
	OSA24G_CONFIG *osa24gCfg;
	int commIfProtocol[5];
	int idx;
	CAMERA_WEB_MANAGER	*camWebMng;
	CAMERA_CONFIG *cameraCfg = sysCfg->cameraCfg;
	
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}

	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
	strcpy(sep, "|");

	dbg_msg("%s", "[==WEB==] Get DeviceParam Command\r\n");

	strtokLock();
	p = strtok(param, sep);
	parameter_num = _io_atoi(p);

	p = strtok(NULL, sep);
	strtokUnLock();
	select_number = _io_atoi(p);

	if(parameter_num == 1)                                                        // Get GetDeviceParam
	{	AR77_RADAR_CONFIG *ar77Cfg;
		AVDS_RADAR_CONFIG *avdsCfg;
		EXT_DEV_CONFIG *extDevCfg;
		COMM_IF_CONFIG *commIfCfg;
		COMPOSITE_OSA_CONFIG	*compositeOsaCfg;
		VW_SENSOR_CONFIG *vwCfg;
		
		AI_CONFIG *aiCfg = (AI_CONFIG *)&sysCfg->aiCfg;
		for(commIfCfg = sysCfg->commIfCfgs;commIfCfg;commIfCfg= commIfCfg->next)
		{	commIfProtocol[commIfCfg->channelNo] = commIfCfg->protocol;
		}
		
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_DEVICE_PARAM);
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n", commIfProtocol[EXT_IF_CHANNEL_RS485_1], commIfProtocol[EXT_IF_CHANNEL_RS485_2], commIfProtocol[EXT_IF_CHANNEL_RS232]);
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", commIfProtocol[EXT_IF_CHANNEL_ETH], commIfProtocol[EXT_IF_CHANNEL_USB_HS]);
		for(device_count = 1 + 5*select_number; device_count <= 5 + 5*select_number; device_count++)
		{	extDevCfg = getExtDevCfgByIdx(device_count);
			if(extDevCfg == NULL)
				strcat(sendData, "0\n0\n0\n0\n0\n0\n");
			else
			{	//SENS_SPRINTF(sendData+strlen(sendData), "1\n%ld\n", extDevCfg->ifProtocol);
				SENS_SPRINTF(sendData+strlen(sendData), "1\n%ld\n", (extDevCfg->ifChannel + 1));	//web start from None, but interface channel from RS485-1
				if(extDevCfg->command)
					SENS_SPRINTF(sendData+strlen(sendData), "%s\n", extDevCfg->command);
				else
					strcat(sendData, "0\n");
				SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", extDevCfg->startParseByte);
				SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", extDevCfg->dataType, extDevCfg->dataOrder);
			}
		}
		//SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n%ld\n%ld\n%ld\n", aiCfg->enable, aiCfg->differential, aiCfg->type[0], aiCfg->type[1], aiCfg->type[2], aiCfg->type[3]);
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", aiCfg->enable, aiCfg->differential);
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n%ld\n%ld\n", aiCfg->sensorType[0], aiCfg->sensorType[1], aiCfg->sensorType[2], aiCfg->sensorType[3] - (int)AI_TYPE_500MV, aiCfg->sensorType[4] - (int)AI_TYPE_500MV);
		

		ar77Cfg = (AR77_RADAR_CONFIG *)getExtDevSpeConfig(EXT_DEV_MODEL_OSA77G);
		if(ar77Cfg)	//current is not suuport this device
		{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", ar77Cfg->mode, ar77Cfg->maxDistance, ar77Cfg->minDistance, ar77Cfg->xRange);
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n", ar77Cfg->autoResolution, ar77Cfg->offset, ar77Cfg->isRadarWithWaterDet);	
		}
		else
		{	strcat(sendData, "0\n0\n0\n0\n0\n0\n0\n");
		}

		avdsCfg = (AVDS_RADAR_CONFIG *)getExtDevSpeConfig(EXT_DEV_MODEL_AVDS);
		if(avdsCfg)
		{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n", avdsCfg->mode, avdsCfg->verticalStartAzimuth, avdsCfg->verticalEndAzimuth);	
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n", avdsCfg->verticalStartRange, avdsCfg->verticalEndRange, avdsCfg->distOf2Radar);	
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", avdsCfg->tiltAngle, avdsCfg->tiltBinRangeL, avdsCfg->waterLevel, avdsCfg->installHeight);
		}
		else
		{	strcat(sendData, "0\n0\n0\n0\n0\n0\n0\n0\n0\n0\n");
		}
		camWebMng = (CAMERA_WEB_MANAGER *)&sysCtrl->camWebMng;
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", camWebMng->isSupported);

		osa24gCfg = (OSA24G_CONFIG *)getExtDevSpeConfig(EXT_DEV_MODEL_OSA24G);
		if(osa24gCfg)
		{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", osa24gCfg->alertMode, osa24gCfg->wlSensor);
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", my_ftoa(a, osa24gCfg->alertThreshold, 4));
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n", osa24gCfg->maxDistance, osa24gCfg->minDistance, osa24gCfg->slaveId);
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n", osa24gCfg->devNo, osa24gCfg->pwrSrc, osa24gCfg->pollingInterval);
		}
		else
		{	strcat(sendData, "0\n0\n0\n0\n0\n0\n0\n2\n0\n");
		}
		
#ifdef SUPPORT_AUDIO
		//if(sysParams->audioParams)
		//	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", sysParams->audioParams->devNo, sysParams->audioParams->slaveId, sysParams->audioParams->doSrc, sysParams->audioParams->diWakeUp);
		//else
#endif
		//	strcat(sendData, "0\n0\n0\n0\n");
		strcat(sendData, "0\n0\n");	//for ikw
		strcat(sendData, "0\n0\n0\n0\n");	//for ikw
		//compositeOsaCfg = (COMPOSITE_OSA_CONFIG *)getExtDevSpeConfig(EXT_DEV_MODEL_COMPOSITE_OSA24G);
		if(sysCfg->compositeOsaCfg)
		{	compositeOsaCfg = sysCfg->compositeOsaCfg;
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", compositeOsaCfg->radarDevNo, compositeOsaCfg->radarPwrSrc);
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", compositeOsaCfg->wlsDevNo, compositeOsaCfg->wlsPwrSrc);
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->alertTrigVal, 3));
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->alertRecoveryVal, 3));
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->installHeight, 3));
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->distance, 3));
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->altitude, 3));
		}
		else
			strcat(sendData, "0\n0\n0\n0\n0\n0\n0\n0\n0\n");
		
		compositeOsaCfg = (COMPOSITE_SIEMENS_CONFIG *)getExtDevSpeConfig(EXT_DEV_MODEL_COMPOSITE_SIEMENS);
		if(compositeOsaCfg)
		{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", compositeOsaCfg->radarDevNo, compositeOsaCfg->radarPwrSrc);
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", compositeOsaCfg->wlsDevNo, compositeOsaCfg->wlsPwrSrc);
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->alertTrigVal, 3));
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->alertRecoveryVal, 3));
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->installHeight, 3));
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->distance, 3));
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->altitude, 3));
			SENS_SPRINTF(sendData+strlen(sendData), "%s\n", sensDtoa(a, compositeOsaCfg->radarBlindArea, 3));
		}
		else
			strcat(sendData, "-1\n0\n-1\n0\n0\n0\n0\n0\n0\n0\n");
		commIfCfg = getCommIfConfigByIfNo(0);
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", commIfCfg->baudrate, commIfCfg->format);
		commIfCfg = getCommIfConfigByIfNo(1);
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", commIfCfg->baudrate, commIfCfg->format);
		commIfCfg = getCommIfConfigByIfNo(2);
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", commIfCfg->baudrate, commIfCfg->format);
		if(cameraCfg)
		{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", sysCfg->sensSysCfg.cameraRecInterval / sysTimer->recTimeSlotInterval, sysCfg->sensSysCfg.cameraAlertInterval);
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n0\n", cameraCfg->resolution);
		}
		else
		{	strcat(sendData, "1\n600\n0\n0\n");
		}
		
		vwCfg = (VW_SENSOR_CONFIG *)getExtDevSpeConfig(EXT_DEV_MODEL_VW);
		if(vwCfg)
		{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n%ld\n%ld\n", vwCfg->devNo, vwCfg->slaveId, vwCfg->pwrSrc, vwCfg->pwrStableTime, vwCfg->pollingInterval);
		}
		else
		{	strcat(sendData, "0\n0\n2\n0\n0\n");
		}
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", sysCfg->diCfg.diStatusRecord, sysCfg->diCfg.diWakeup[0], sysCfg->diCfg.diWakeup[1], sysCfg->diCfg.diWakeupRecInterval);
		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", sysCfg->diCfg.diWakeup[2], sysCfg->diCfg.diWakeup[3]);
		//SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n%ld\n%ld\n", sysParams->diCfg.cyPumpMode, sysParams->diCfg.diTaskSel[0], sysParams->diCfg.diTaskSel[1], sysParams->diCfg.diTaskSel[2], sysParams->diCfg.diTaskSel[3]);			
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
		
#else
		CGI_SEND_STR(sendData);
#endif
#endif
	}
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_GetNodeParam(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_GetNodeParam(char *param)
#endif
{
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char sep[2], a[20];
#ifdef FSL_HTTP
	char *param;
#endif
	char *p, select_number;
	int channel_count, parameter_num;
	PQ_CONFIG *pqCfg;
  
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
	strcpy(sep, "|");

	strtokLock();
	p = strtok(param, sep);
	parameter_num = _io_atoi(p);

	p = strtok(NULL, sep);
	strtokUnLock();
	select_number = _io_atoi(p);
  
	dbg_msg("%s", "[==WEB==] Get NodeParam Command\r\n");

	if(parameter_num == 1)                                                        // Get GetNodeParam
	{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", GET_NODE_MAP, GET_NODE_MAP1);
		for(channel_count = 8*select_number; channel_count <= (7+8*select_number); channel_count++)
		{	pqCfg = getPqConfig(channel_count);
			if(pqCfg)
			{	strcat(sendData, "1|");
				strcat(sendData, my_ftoa(a, pqCfg->pqSrc.devNo, 0));
				strcat(sendData, sep);
				strcat(sendData, my_ftoa(a, pqCfg->pqSrc.devChanNo, 0));
				strcat(sendData, sep);
				strcat(sendData, my_ftoa(a, pqCfg->formulaId, 0));
				strcat(sendData, sep);
				strcat(sendData, my_ftoa(a, pqCfg->filterType, 0));
				strcat(sendData, sep);
				if(pqCfg->alias[0])	strcat(sendData, pqCfg->alias[0]);
				else				strcat(sendData, "0");
				strcat(sendData, sep);
				if(pqCfg->alias[1])	strcat(sendData, pqCfg->alias[1]);
				else				strcat(sendData, "0");
				strcat(sendData, sep);
			}
			else
			{	strcat(sendData, "0|0|0|0|0|0|0|");
			}
		}
		strcat(sendData, "\n");
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
#else
		//CGI_SEND_NUM(GET_NODE_MAP);
		//CGI_SEND_NUM(GET_NODE_MAP1);
		CGI_SEND_STR(sendData);
#endif
#endif
	}
	else if(parameter_num == 5)
	{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_NODE_MAP4);
		/*for(channel_count = 0; channel_count < 2; channel_count++)
		{	
#if TEST_N_REMOVE
			strcat(sendData, my_ftoa(a, sensSys->DO_enable[channel_count], 0));
			strcat(sendData, sep);
			strcat(sendData, my_ftoa(a, sensSys->DO_software[channel_count], 0));
			strcat(sendData, sep);
			strcat(sendData, sensSys->DO_batlowstr[channel_count]);
			strcat(sendData, sep);
			strcat(sendData, sensSys->DO_bathighstr[channel_count]);
			strcat(sendData, sep);
			strcat(sendData, my_ftoa(a, sensSys->DO_mode[channel_count], 0));
			strcat(sendData, sep);
#else
			strcat(sendData, "0|0|0|0|0|");
#endif
		}*/

		/*for(channel_count = 5; channel_count < 7; channel_count++)
		{	
#if TEST_N_REMOVE
			strcat(sendData, my_ftoa(a, sensSys->DO_enable[channel_count], 0));
			strcat(sendData, sep);
			strcat(sendData, my_ftoa(a, sensSys->DO_software[channel_count], 0));
			strcat(sendData, sep);
			strcat(sendData, sensSys->DO_batlowstr[channel_count]);
			strcat(sendData, sep);
			strcat(sendData, sensSys->DO_bathighstr[channel_count]);
			strcat(sendData, sep);
			strcat(sendData, my_ftoa(a, sensSys->DO_mode[channel_count], 0));
			strcat(sendData, sep);
#else
			strcat(sendData, "0|0|0|0|0|");
#endif
		}*/

		strcat(sendData, my_ftoa(a, sysCfg->doCfg.powerCtrl[0], 0));
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, sysCfg->doCfg.powerCtrl[1], 0));
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, sysCfg->doCfg.mode, 0));
		strcat(sendData, sep);
		//strcat(sendData, my_ftoa(a, sysCfg->doCfg.powerCtrl[2], 0));
		strcat(sendData, my_ftoa(a, sysCfg->doCfg.DO24VPwr, 0));
		strcat(sendData, sep);
		EEPROM_CONTEXT *eepromCtx = (EEPROM_CONTEXT *)sensSys->eepromBuf;
		strcat(sendData, my_ftoa(a, eepromCtx->doMap[0] + 1, 0));
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, eepromCtx->doMap[1] + 1, 0));
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, eepromCtx->doMap[2] + 1, 0));
		strcat(sendData, sep);
		strcat(sendData, my_ftoa(a, eepromCtx->doMap[3] + 1, 0));
		strcat(sendData, sep);
		
		strcat(sendData, "\n");
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
#else
		//CGI_SEND_NUM(GET_NODE_MAP4);
		CGI_SEND_STR(sendData);
#endif
#endif
	}
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
static void getHistoryRecData(char *sendData, char **finalSendData, int timeYear, int timeMonth, int timeDay, int timeHour)
{	int startSlot, slotIdx, endSlot;
	int slotOffset;
	int bmpIdx, bmpBitNo;
	int currSize, recordHour, dataSizeHour;
	char *sendData1=NULL;
	char minStep[20];
	char str[20], num_index[6];
	char time_buf[50];
	char *textdata;
#if AUTO_DATA_SYNC
	REC_HEADER *recHeader;
#endif
	char *historyData;
	float *historyDataFPtr;
	int length;
	int min;
	int index;
	char filename[MAX_FILE_PATH_LEN];
	char *sendBuf;
	
	memset(filename, 0, MAX_FILE_PATH_LEN);
	SENS_SPRINTF(filename, "%04d/%02d/%02d%02dn%03d.bin", timeYear, timeMonth, timeMonth, timeDay, sensPq->pqNumber);
	startSlot = timeHour * 60;
	endSlot = startSlot + 60;
	slotOffset = startSlot * sensPq->pqNumber * sizeof(float);
	recordHour = 60 * sensPq->pqNumber;				// XXX record each hour
	dataSizeHour = recordHour * sizeof(float);	// how many bytes it occupy each hour
	
	minStep[0] = '\0';
		
	recHeader = (REC_HEADER *)SENS_MEM_ZALLOC(sizeof(REC_HEADER));
	length = sdReadFile(filename, (char *)recHeader, sizeof(REC_HEADER), 0, NORMAL_READ_MODE);

	historyData = SENS_MEM_ALLOC(dataSizeHour);
	if(length == 0) 
	{	
FILL_EMPTY_DATA:
		memset(historyData, 0xFF, dataSizeHour);
	}
	else
	{	if(recHeader->headerTag.tag != 0x55AA)
		{	goto FILL_EMPTY_DATA;
		}
		if(recHeader->headerTag.ver == FIXED_REC_INTERVAL_VERSION)
		{
		}
    	
		length = sdReadFile(filename, historyData, dataSizeHour, sizeof(REC_HEADER) + slotOffset, BIN_READ_MODE);
		if(length == 0)
		{	goto FILL_EMPTY_DATA;
		}	
	}
	dataSizeHour = recordHour * (11+3);
	textdata = SENS_MEM_ZALLOC(dataSizeHour);
	//textdata[0] = '\0';                                                      // be careful that the number don't be less than the words printed in the text file
	min = 0;
	currSize = 0;
	//minute_count = 3600 / sd.record_sec;
	historyDataFPtr = (float *)historyData;
    
	for(slotIdx = startSlot;slotIdx<endSlot;slotIdx++)
	{	bmpIdx = slotIdx / 8;
		bmpBitNo = slotIdx % 8;
		if(recHeader->recSlotBmpFlag[bmpIdx] & (0x01 << bmpBitNo) )
		{	if(slotIdx != startSlot)
			{	SENS_SPRINTF(minStep, "&(%02d minutes),", min);
				strcat(textdata, minStep);
			}
			for(index = 0; index < sensPq->pqNumber; index++)
			{	my_ftoa(str, *historyDataFPtr, 2);
				strcat(textdata, str);
				strcat(textdata, ",");
				historyDataFPtr++;
			}
			min++;
		}
		else
		{	historyDataFPtr += sensPq->pqNumber;
			min++;
		}
	}
	strcat(textdata, "&&");

	SENS_SPRINTF(time_buf, "%4d/%02d/%02d/H%02d,", timeYear, timeMonth, timeDay, timeHour);
	for(index = 0; index < sensPq->pqNumber; index++)
	{	SENS_SPRINTF(num_index, "%d,", index);
		strcat(time_buf, num_index);
	}

	sendBuf = SENS_MEM_ZALLOC(strlen(time_buf)+1 + strlen(textdata)+1 + 2+11+1);
	SENS_SPRINTF(sendBuf, "%ld\n", GET_SD_DATA);
	SENS_SPRINTF(sendBuf+strlen(sendBuf), "%s\n%s\n", time_buf, textdata);
	SENS_SPRINTF(sendBuf+strlen(sendBuf), "%ld\n", (timeHour + 1));
    
	sendData1 = SENS_MEM_ZALLOC(strlen(sendData)+strlen(sendBuf)+2);
	strcat(sendData1, sendData);
	strcat(sendData1, sendBuf);
	
	SENS_MEM_FREE(recHeader);
	SENS_MEM_FREE(historyData);
	SENS_MEM_FREE(textdata);
	
	*finalSendData = sendData1;
}
//------------------------------------------------------------------------------
static void getParameterData(char *sendData, char **finalSendData)
{	char iniFilename[64];
	char *textdata;
	char *sendBuf;
	int length;
	char *sendData1;
		
	memset(iniFilename, 0,64);
	if(chkFileExist(INI_BAK_FILE))
		memcpy(iniFilename, INI_BAK_FILE, strlen(INI_BAK_FILE));
	else
		memcpy(iniFilename, INIFILE, strlen(INIFILE));
		
	length = getFileLength(iniFilename);
	textdata = (char*)SENS_MEM_ZALLOC(length+10);                 // be careful that the number can't be less than the words printed in the text file
	sdReadFile(iniFilename, textdata, length, 0, BIN_READ_MODE);
	
	textdata = strrep(textdata, '\n', '&', length);
	
	sendBuf = SENS_MEM_ZALLOC(strlen(textdata)+1 + 3);
		
	SENS_SPRINTF(sendBuf, "%ld\n", GET_SD_PARAM_FILE);
	SENS_SPRINTF(sendBuf+strlen(sendBuf), "%s\n", textdata);
    
	sendData1 = SENS_MEM_ZALLOC(strlen(sendData)+strlen(sendBuf)+2);
	strcat(sendData1, sendData);
	strcat(sendData1, sendBuf);
	SENS_MEM_FREE(sendBuf);
	SENS_MEM_FREE(textdata);
	
	*finalSendData = sendData1;
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_GetSDData(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char * cgi_GetSDData(char *param)
#endif
{
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
	//HTTPSRV_SESSION_STRUCT *sessionNew;
#endif
	char *sendData1=NULL;
#ifdef FSL_HTTP
	char *param;
#endif
	char *p;
	int timeYear, timeMonth, timeDay, timeHour, cmd, index, minute_count;
	char sep[2], time_buf[50], num_index[6], buf[MAX_FILE_PATH_LEN], str[20]; //, fbuffer[4];
	float sd_value;
	int length;


	char *sendData;
	SESSION_KEY *sesKey = NULL;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
	
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
	strcpy(sep, "|");

	strtokLock();
	p = strtok(param, sep);
	cmd = _io_atoi(p);

	if(cmd == 1 && (sesKey->authType != LOGIN_ID_GUEST))                              // Get SD Binary record data
	{	p = strtok(NULL, sep);
		if(p)	timeYear = _io_atoi(p);
		p = strtok(NULL, sep);
		if(p)	timeMonth = _io_atoi(p);
		p = strtok(NULL, sep);
		if(p)	timeDay = _io_atoi(p);
		p = strtok(NULL, sep);
		if(p)	timeHour = _io_atoi(p);
		strtokUnLock();
		getHistoryRecData(sendData, &sendData1, timeYear, timeMonth, timeDay, timeHour);
		
		SENS_MEM_FREE(sendData);
		//SENS_MEM_FREE(sendBuf);
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
#else
		CGI_SEND_STR(sendData1);
#endif
#endif
		SENS_TIME_DELAY(100);
	}
	else if((cmd == 2) && (sesKey->authType != LOGIN_ID_GUEST))                         // Get Parameter file from SD card
	{	strtokUnLock();
		
		getParameterData(sendData, &sendData1);
		SENS_MEM_FREE(sendData);
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
		
#else
		CGI_SEND_STR(sendData1);
#endif
#endif
		SENS_TIME_DELAY(100);
	}
#if !SPI_FILE_SYSTEM
	else if((cmd == 4) && (sesKey->authType != LOGIN_ID_GUEST))
	{	strtokUnLock();
#if 0
		reset_Param_In_SD();
		dbg_msg("[==WEB==] %s\r\n", "Reset configuraion");
		SENS_TIME_DELAY(100);
		sensSys->IsReboot = 1;
#endif
		sendData1 = sendData;
	}
#endif
	else
	{	strtokUnLock();
		SENS_MEM_FREE(sendData);
	}

	CGI_RETURN(sendData1);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_Login(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_Login(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char sep[2];
#ifdef FSL_HTTP
	char *param;
#endif
	char *p, *q;
	int parameter_num, authPass;
	char *sendData;
	
#if CGI_CHECK_SID
	char authType;
	char *sid;
	SYS_AUTH_INFO *sysAuthInfo = sensSys->sysAuthInf;
	SYS_AUTH_STRUCT *sysAuth;
	
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
	param = session->query_string;
#else
	param = session->request.urldata;
#endif
#endif
	p = strstr(param, SID_STR);
	q = strstr(param, P_KEY);
	sid = SENS_MEM_ZALLOC(10);
	memcpy(sid, p+strlen(SID_STR), q-p-strlen(SID_STR));
#endif
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
	strcpy(sep, "|");
  
	strtokLock();
	p = strtok(param, sep);
	parameter_num = _io_atoi(p);
  
	if(parameter_num == 2)                                                   // Get password
	{	authPass = 2;
		p = strtok(NULL, sep);                                                      // username
		q = strtok(NULL, sep);                                                      // password
		strtokUnLock();

		p = (char *)base64Decode(p);
		q = (char *)base64Decode(q);

		for(sysAuth = sysAuthInfo->authTable;sysAuth;sysAuth = sysAuth->next)
		{	if((strlen(p) == strlen(sysAuth->username)) && (strlen(q) == strlen(sysAuth->psw)))
			{	if(!strcmp(p, sysAuth->username) && !strcmp(q, sysAuth->psw))
				{	authPass = 1;
					authType = sysAuth->authType;
					break;
				}
			}
		}
		
		if(authPass == 1)
			dbgMsg("[WEB] Login ID: %s\r\n", (authType == LOGIN_ID_SU)? "SU": (authType == LOGIN_ID_ADMIN)? "ADMIN":"GUEST");
		else
			dbgMsg("%s", "[WEB] Login ERROR\r\n");
		
		SENS_MEM_FREE(p);
		SENS_MEM_FREE(q);
    
#if defined NEW_HTTP_STRUCT || defined NATIVE_HTTP
		SESSION_KEY *sessionKey = addConnectInfo(0);
#else
		SESSION_KEY *sessionKey = addConnectInfo(session->sock);
#endif
#if CGI_CHECK_SID
		sessionKey->authType = authType;
		sessionKey->sessionId = sid;
#endif
		SENS_SPRINTF(sendData, "%s%s\n", P_KEY, sessionKey->key);


		SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n%ld\n", GET_PASSWORD, authPass, authType);
		//if(sensSys->check_SDparam <= 2)
		//	SENS_SPRINTF(sendData+strlen(sendData), "%s\n", (sensSys->check_SDparam == 0)? "ini_OK": (sensSys->check_SDparam == 1)?"ini_NG":"TCP_NG");
		SENS_SPRINTF(sendData+strlen(sendData), "%s\n", "ini_OK");
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
		
#else
		CGI_SEND_STR(sendData);
#endif
#endif
	}
	else
		strtokUnLock();
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_GetCoef(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_GetCoef(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char sep[2];
#ifdef FSL_HTTP
  char *param;
#endif
  char *p;  //, *q;
  int idx, parameter_num;

	char *sendData;
	SESSION_KEY *sesKey = NULL;
	FORMULA_STRUCT	*formula;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}

	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
	strcpy(sep, "|");

	strtokLock();
	p = strtok(param, sep);
	parameter_num = _io_atoi(p);

	if(parameter_num == 1)                                                        // Get Coef (y = ax^2 + bx + c)
	{	p = strtok(NULL, sep);
		strtokUnLock();
		if(p)
		{	idx = _io_atoi(p);
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_COEF_VALUE);
			for(formula = sysCfg->formula;formula;formula=formula->next)
			{	if(formula->formulaIdx == idx)
				{	SENS_SPRINTF(sendData+strlen(sendData), "%s\n%s\n%s\n", formula->aStr, formula->bStr, formula->cStr);
					break;
				}
			}
			if(formula == NULL)
				strcat(sendData, "0\n1\n0\n");
#ifdef NETRTCS
	#if defined NEW_HTTP_STRUCT
	#else
			CGI_SEND_STR(sendData);
	#endif
#endif
			dbg_msg("[==WEB==] %s\r\n", "Get formula1");
		}
	}
	else if(parameter_num == 3)                                                   // Get Coef_1 (y = ax^b)
	{	p = strtok(NULL, sep);
		strtokUnLock();
		if(p)
		{	idx = _io_atoi(p);
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_COEF1_VALUE);
			for(formula=sysCfg->formula;formula;formula=formula->next)
			{	if(formula->formulaIdx == idx)
				{	SENS_SPRINTF(sendData+strlen(sendData), "%s\n%s\n", formula->aStr, formula->bStr);
					break;
				}
			}
			if(formula == NULL)
				strcat(sendData, "1\n1\n");
			
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
			CGI_SEND_STR(sendData);
	#endif
#endif
			dbg_msg("[==WEB==] %s\r\n", "Get formula2");
		}
	}
	else
		strtokUnLock();
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_ResetDevice(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_ResetDevice(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	dbg_msg("[==WEB==] %s\r\n", "RESET device");

	if(sesKey->authType != LOGIN_ID_GUEST)
	{	SENS_TIME_DELAY(100);
		sysCtrl->isReadyToReboot = 1;
	}
	strcat(sendData, "\n");
  
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
static void putWlSts(char *sendData, char *sep, char mode)
{	char stsFail=0;
	MOBILE_INSTANCE *wlInst;
	if(networkCtx && networkCtx->hsCommInst && networkCtx->hsCommInst->wlInst->mobileCellType == MOBILE_CELL_TYPE_LTE)
	{	switch(networkCtx->hsCommInst->wlInst->wlStatus)
		{	case WL_REG_STS_NOT_REGISTER:
  					strcat(sendData, "not registered");
  					break;
			case WL_REG_STS_GPRS_2G:
  					strcat(sendData, "2G, GPRS");
  					break;
			case WL_REG_STS_EDGE_2G:
  					strcat(sendData, "2G, EDGE");
  					break;
			case WL_REG_STS_WCDMA_3G:
  					strcat(sendData, "3G, WCDMA");
  					break;
			case WL_REG_STS_HSDPA_3G:
  					strcat(sendData, "3G, HSDPA");
  					break;
			case WL_REG_STS_HSUPA_3G:
  					strcat(sendData, "3G, HSUPA");
  					break;
			case WL_REG_STS_HSDPA_HSUPA:
  					strcat(sendData, "HSDPA/HSUPA");
  					break;
			case WL_REG_STS_LTE_4G:
  					strcat(sendData, "4G, LTE");
  					break;
			case WL_REG_STS_GPRS_2G_1:
  					strcat(sendData, "2G, GPRS");
  					break;
			case WL_REG_STS_EDGE_2G_1:
  					strcat(sendData, "2G, EDGE");
  					break;
			//case WL_REG_STS_WAIT_READY_FAIL:
  					//strcat(sendData, "2G, GPRS");
  					//break;
			case WL_REG_STS_WAIT_READY_FAIL:
  					strcat(sendData, "Module Fail");
  					stsFail = 1;
  					break;
			case WL_REG_STS_SIM_NOT_READY:
  					strcat(sendData, "SIM Card Fail");
  					stsFail = 1;
  					break;
			default:
  					break;
		}
	}
	else
		strcat(sendData, "not registered");
	if(!mode)
	{	strcat(sendData, "/");
	}
	else
	{	strcat(sendData, sep);
	}
	if(sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] <= -51 && sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] >= -113)
	{	if(sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] == -51)
			strcat(sendData, "-51dBm or greater");
		else if(sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] == -113)
			strcat(sendData, "-113dBm or less");
		else
			SENS_SPRINTF(sendData+strlen(sendData), "%ddBm", (int)sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH]);
	}
	else if(!stsFail)
		strcat(sendData, "not detectable");
	strcat(sendData, sep);
	stsFail = 0;
	if(networkCtx && 
  		((networkCtx->hsCommInst && networkCtx->hsCommInst->wlInst->mobileCellType == MOBILE_CELL_TYPE_NBIOT) || 
  		 (networkCtx->lsCommInst && networkCtx->lsCommInst->wlInst->mobileCellType == MOBILE_CELL_TYPE_NBIOT)))
	{	if((networkCtx->hsCommInst && networkCtx->hsCommInst->wlInst->mobileCellType == MOBILE_CELL_TYPE_NBIOT))
		{	wlInst = networkCtx->hsCommInst->wlInst;
		}
		else //if(mPcie->lsPciInf && mPcie->lsPciInf->mobileCellType == MOBILE_CELL_NBIOT)
		{	wlInst = networkCtx->lsCommInst->wlInst;
		}
		switch(wlInst->wlStatus)
		{	case WL_REG_STS_NOT_REGISTER:
  					strcat(sendData, "not registered");
  					break;
			case WL_REG_STS_LTE_CAT_NB_M1:
  					if(wlInst->simCardTech == SIM_TYPE_CAT_M1)
  					{	strcat(sendData, "CAT-M1");
  					}
  					else
  					{	strcat(sendData, "NB-IoT");
  					}
  					break;
			case WL_REG_STS_WAIT_READY_FAIL:
  					strcat(sendData, "Module Fail");
  					stsFail = 1;
  					break;
			case WL_REG_STS_SIM_NOT_READY:
  					strcat(sendData, "SIM Card Fail");
  					stsFail = 1;
  					break;
			default:
					break;
		}
	}
	else
		strcat(sendData, "not registered");
	if(!mode)
	{	strcat(sendData, "/");
	}
	else
	{	strcat(sendData, sep);
	}
	if(sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] <= -51 && sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] >= -113)
	{	if(sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] == -51)
			strcat(sendData, "-51dBm or greater");
		else if(sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] == -113)
			strcat(sendData, "-113dBm or less");
		else
			SENS_SPRINTF(sendData+strlen(sendData), "%ddBm", (int)sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH]);
	}
	else if(!stsFail)
		strcat(sendData, "not detectable");
	strcat(sendData, sep);
}

//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_GetRealValue(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_GetRealValue(char *param)
#endif
{
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char sep[2], a[20];
	int index;
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	AI_CONFIG *aiCfg = (AI_CONFIG *)&sysCfg->aiCfg;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	//realValue = SENS_MEM_ZALLOC(400);
	//memset(sd.real_value, 0, sizeof(char)*400);

	strcpy(sep, "|");                                                             // separator between parameters
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_REAL_VALUE);
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", sensPq->pqNumber, ON_BOARD_PQ_MAX);
  
	if(aiCfg->enable)
	{	for(index = 0;index < 3; index++)
		{	//(isnan(sensDev->onboard_ai[index]) == 0)? strcat(sendData, my_ftoa(a, sensDev->onboard_ai[index], 2)): strcat(sendData, "NaN");             // 2 decimal places
			strcat(sendData, my_ftoa(a, sensPq->onboardPq[index], 2));
			strcat(sendData,  " mA");
			strcat(sendData, sep);
		}
		switch(aiCfg->differential)
		{	case DIFFERENTIAL_MODE:
				strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_AI_VOLT_0], 2));
				strcat(sendData, " V");
				strcat(sendData, sep);
				strcat(sendData, "-");
				strcat(sendData, sep);
				break;
			case SINGLE_END_MODE:
				strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_AI_VOLT_0], 2));
				strcat(sendData, " V");
				strcat(sendData, sep);
				strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_AI_VOLT_1], 2));
				strcat(sendData, " V");
				strcat(sendData, sep);
				break;
		}
		//ON_BOARD_PQ_EMPTY_IDX_5
		strcat(sendData, "-");	strcat(sendData, sep);
		strcat(sendData, "-");	strcat(sendData, sep);
		strcat(sendData, "-");	strcat(sendData, sep);
	}
	else
	{	for(index=0;index<8;index++)
		{	strcat(sendData, "-");	strcat(sendData, sep);
		}
	}
	//(sensDev->OnboardCounter[0]!=sensDev->OnboardCounter[0])?strcat(sendData, "NaN"):strcat(sendData, my_ftoa(a, sensDev->OnboardCounter[0], 0));   // DIO counter
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_0], 0));
	strcat(sendData, sep);
	//(sensDev->OnboardCounter[1]!=sensDev->OnboardCounter[1])?strcat(sendData, "NaN"):strcat(sendData, my_ftoa(a, sensDev->OnboardCounter[1], 0));   // DI1 counter
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_1], 0));
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_2], 0));
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_3], 0));
	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DI_STS_0], 0));   // DI0 status	
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DI_STS_1], 0));   // DI1 status
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DI_STS_2], 0));
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DI_STS_3], 0));
	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DO_STS_0], 0));                 // DO0 status
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DO_STS_1], 0));                 // DO1 status
	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_DO24_STS], 0));
	strcat(sendData, sep);
	strcat(sendData, "-");	strcat(sendData, sep);
	
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_ERROR_CODE], 0));
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_TEMPERATURE], 2));   // Temperature
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_BATTERY_VOLT], 2));	// Bat. volt
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_EXT_VOLT], 2));	// Ext. volt
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_SOLAR_CHARGE_CURRENT], 3));
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_LOAD_CURRENT], 0));
	strcat(sendData, sep);
	putWlSts(sendData, sep, 1);	
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_REBOOT_TIMES], 0));
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_LATITUDE], 6));
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_LONGITUDE], 6));
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_GPS_STATUS], 0));
	strcat(sendData, sep);
	strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_GPS_SPEED], 2));
	strcat(sendData, sep);

	for(index = 0 ;index < sensPq->pqNumber; index++)
	{	strcat(sendData, my_ftoa(a, sensPq->pqVal[index], 2));                         // 2 decimal places
		strcat(sendData, sep);
	}
	strcat(sendData, "\n");
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);
}

//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_SetDeviceTime(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_SetDeviceTime(char *param)
#endif
{
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
#ifdef FSL_HTTP
	char *param;
#endif
	char *p, sep[2];
#if defined NUVOTON
	S_RTC_TIME_DATA_T nuDateTime;
#endif
	uint16_t dateTime[6];
	SESSION_KEY *sesKey = NULL;
	int compilerYear;
	
	compilerYear = ((__DATE__[7] - 0x30) * 1000) + ((__DATE__[8] - 0x30) * 100) + ((__DATE__[9] - 0x30) * 10) + ((__DATE__[10] - 0x30) * 1);

	char *sendData;
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}

	// Find position after PSTART("&D="), after which the parameter begins.
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);

	strcpy(sep, "|");                                                             // separator between parameters

	strtokLock();
	p = strtok(param, sep);
	if (p) dateTime[0] = (uint16_t)_io_atoi(p);
	p = strtok(NULL, sep);
	if (p) dateTime[1] = (uint16_t)_io_atoi(p);
	p = strtok(NULL, sep);
	if (p) dateTime[2] = (uint16_t)_io_atoi(p);
	p = strtok(NULL, sep);
	if (p) dateTime[3] = (uint16_t)_io_atoi(p);
	p = strtok(NULL, sep);
	if (p) dateTime[4] = (uint16_t)_io_atoi(p);
	p = strtok(NULL, sep);
	strtokUnLock();
	if (p) dateTime[5] = (uint16_t)_io_atoi(p);

	dbg_msg("[==WEB==] Set Date(UTC): %d/%02d/%02d %02d:%02d:%02d\r\n", dateTime[0], dateTime[1], dateTime[2], dateTime[3], dateTime[4], dateTime[5]);
#ifdef NUVOTON
	nuDateTime.u32Year = dateTime[0];
	nuDateTime.u32Month = dateTime[1];
	nuDateTime.u32Day = dateTime[2];
	nuDateTime.u32Hour = dateTime[3];
	nuDateTime.u32Minute = dateTime[4];
	nuDateTime.u32Second = dateTime[5];
#endif

	if(dateTime[0] >= compilerYear && sesKey->authType != LOGIN_ID_GUEST)
	{
#if defined NUVOTON
		setTime(&nuDateTime);
#else
		set_time(dateTime);
#endif
		SENS_TIME_DELAY(100);
		//sensSys->IsReboot = 1;
	}
  
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
#else
	CGI_SEND_STR(sendData);
#endif
#endif
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_GetDeviceTime(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_GetDeviceTime(char *param)
#endif
{
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char sep[2], a[20];
	//char stsFail=0;
#if SUPPORT_CY_PUMP
	CY_PUMP_WORK_INSTANCE	*cyPumpInst = &sensSys->cyPumpInst;
	GPS_MOVE_REC_CTX *gpsMoveRecCtx;
#endif
	NET_INSTANCE *workingInst;
	MOBILE_INSTANCE *wlInst;
	SYS_POWER_MANAGER *pwrManager = (SYS_POWER_MANAGER *)&sysCtrl->pwrManager;
	SENS_SYS_CONFIG *sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}

	strcpy(sep, "|");                                                             // separator between parameters
  
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_DEV_TIME);  // Func Id
	SENS_SPRINTF(sendData + strlen(sendData), "%ld\n%ld\n%ld\n", sensSys->dateTime.u32Year, sensSys->dateTime.u32Month, sensSys->dateTime.u32Day);
	SENS_SPRINTF(sendData + strlen(sendData), "%ld\n%ld\n%ld\n", sensSys->dateTime.u32Hour, sensSys->dateTime.u32Minute, sensSys->dateTime.u32Second);
	
	workingInst = networkCtx->workingInst;
	if(networkCtx && workingInst && (workingInst->netType != NETWORK_TYPE_ETH) && (workingInst->wlInst->mobileCellType == MOBILE_CELL_TYPE_LTE))
	{	wlInst = workingInst->wlInst;
		SENS_SPRINTF(sendData + strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", wlInst->ip[0], wlInst->ip[1], wlInst->ip[2], wlInst->ip[3]);
	}
	else
		strcat(sendData, "0\n0\n0\n0\n");
	if(networkCtx && workingInst && (workingInst->netType != NETWORK_TYPE_ETH) && (workingInst->wlInst->mobileCellType == MOBILE_CELL_TYPE_NBIOT))
	{	wlInst = workingInst->wlInst;
		SENS_SPRINTF(sendData + strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", wlInst->ip[0], wlInst->ip[1], wlInst->ip[2], wlInst->ip[3]);
	}
	else
		strcat(sendData, "0\n0\n0\n0\n");

	putWlSts(sendData, sep, 0);
	//dbg_msg("wl info :%s\r\n", sd.first_page);
	strcat(sendData, "\n0\n");
	strcat(sendData, "0\n0\n0\n");
	if(networkCtx && workingInst && (workingInst->netType != NETWORK_TYPE_ETH) && (workingInst->wlInst->mobileCellType == MOBILE_CELL_TYPE_LTE))
	{	
#if TEST_N_REMOVE
		if(sensDev->ledStatus == LED_STS_CONNECTING)
			strcat(sendData, "  [Connecting]\n");
		else if(sensDev->ledStatus == LED_STS_STANDBY)
			strcat(sendData, "  [Standby]\n");
		else if(sensDev->ledStatus == LED_STS_CONNECTED)
			strcat(sendData, "  [Connected]\n");
		else if(sensDev->ledStatus == LED_STS_IDLE)
			strcat(sendData, "  [Wait Connecting]\n");
#else
		strcat(sendData, "  \n");
#endif
	}
	else
		strcat(sendData, "  \n");
    if(networkCtx && (networkCtx->workingInst) && (networkCtx->workingInst->netType != NETWORK_TYPE_ETH) && (networkCtx->workingInst->wlInst->mobileCellType == MOBILE_CELL_TYPE_NBIOT))
	{	
#if TEST_N_REMOVE
		if(sensDev->ledStatus == LED_STS_CONNECTING)
			strcat(sendData, "  [Connecting]\n");
		else if(sensDev->ledStatus == LED_STS_STANDBY)
			strcat(sendData, "  [Standby]\n");
		else if(sensDev->ledStatus == LED_STS_CONNECTED)
			strcat(sendData, "  [Connected]\n");
		else if(sensDev->ledStatus == LED_STS_IDLE)
			strcat(sendData, "  [Wait Connecting]\n");
#else
		strcat(sendData, "  \n");
#endif
	}
	else
		strcat(sendData, "  \n");
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n%ld\n", sysCtrl->enTemporaryDisSleep, sysCtrl->temporaryDisSleepremainTime);
	if(pwrManager->currPsm == 0)
	{	strcat(sendData, "Disable");
		if(sensSysCfg->psm)
			SENS_SPRINTF(sendData+strlen(sendData), "(%X H)[%lu]", pwrManager->disablePsmBmp, xPortGetFreeHeapSize());
		strcat(sendData, "\n");
	}
	else if(pwrManager->currPsm == 1)
		//strcat(sendData, "Normal Power Saving\n");	
		//SENS_SPRINTF(sendData+strlen(sendData), "Normal Power Saving(%X H)\n", pwrManager->psmChkBmp);
		SENS_SPRINTF(sendData+strlen(sendData), "Normal Power Saving(%X H)[%lu]\n", pwrManager->disablePsmBmp, xPortGetFreeHeapSize());
	else if(pwrManager->currPsm == 2)
		//strcat(sendData, "Adv. Power Saving\n");	
		//SENS_SPRINTF(sendData+strlen(sendData), "Adv. Power Saving(%X H)\n", pwrManager->psmChkBmp);
		SENS_SPRINTF(sendData+strlen(sendData), "Adv. Power Saving(%X H) [%lu]\n", pwrManager->disablePsmBmp, xPortGetFreeHeapSize());
#if SUPPORT_CY_PUMP
	if(cyPumpInst->gpsMoveRecBufIsValid)
	{	strcat(sendData, my_ftoa(a, sysParams->gpsPositionPoint[0], 6));
		strcat(sendData, "\n");	
		strcat(sendData, my_ftoa(a, sysParams->gpsPositionPoint[1], 6));
		strcat(sendData, "\n");
	}
	else
#endif
		strcat(sendData, "0\n0\n");
	if(!isnan(sensPq->onboardPq[ON_BOARD_PQ_LATITUDE]))
		strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_LATITUDE], 6));
	else
		strcat(sendData, "360");	
	strcat(sendData, "\n");	
	if(!isnan(sensPq->onboardPq[ON_BOARD_PQ_LONGITUDE]))
		strcat(sendData, my_ftoa(a, sensPq->onboardPq[ON_BOARD_PQ_LONGITUDE], 6));
	else
		strcat(sendData, "360");
	strcat(sendData, "\n");	
#if TEST_N_REMOVE
	if(cyPumpInst->gpsMoveRecBufIsValid)
		strcat(sendData, my_ftoa(a, cyPumpInst->locationMoveDist, 3));
	else
#endif
		strcat(sendData, " ");	
	strcat(sendData, "\n");	
	
#ifdef FSL_HTTP
#if defined NEW_HTTP_STRUCT
#else
	CGI_SEND_STR(sendData);
#endif
#endif
	CGI_RETURN(sendData);
}

//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_GetFWVersion(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_GetFWVersion(char *param)
#endif
{
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	//dbg_msg("%s enter, session info:%d, %d at %p\r\n", __func__, session->isAuthenticated, session->sock, session);
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
	
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_FW_VERSION);
	SENS_SPRINTF(sendData + strlen(sendData), "%ld\n", (uint32_t)FW_VERSION);
	SENS_SPRINTF(sendData + strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", sensSys->guid.guid[0], sensSys->guid.guid[1], sensSys->guid.guid[2], sensSys->guid.guid[3]);
	SENS_SPRINTF(sendData + strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", sensSys->guid.guid[4], sensSys->guid.guid[5], sensSys->guid.guid[6], sensSys->guid.guid[7]);
	SENS_SPRINTF(sendData + strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", sensSys->guid.guid[8], sensSys->guid.guid[9], sensSys->guid.guid[10], sensSys->guid.guid[11]);
	SENS_SPRINTF(sendData + strlen(sendData), "%ld\n%ld\n%ld\n%ld\n", sensSys->guid.guid[12], sensSys->guid.guid[13], sensSys->guid.guid[14], sensSys->guid.guid[15]);
	SENS_SPRINTF(sendData + strlen(sendData), " [Build %s %s] ", __DATE__, __TIME__); 
	strcat(sendData, "[Senstalk2");
#ifdef SUPPORT_MQTT
	strcat(sendData, ", MQTT");
#endif
	strcat(sendData, ", UTC");
#ifdef PLATFORM_FSL
	if(isGoodSram)
		strcat(sendData, ", ISSI");
	else
		strcat(sendData, ", LYONTEK");
#endif
	if(IS_BETA_VERSION)
		strcat(sendData, ", BETA version");
	strcat(sendData, "]\n");
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);
}

//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_SetBootParam(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char * cgi_SetBootParam(char *param)
#endif
{	//dbg_msg("%s enter, session info:%d, %d at %p\r\n", __func__, session->isAuthenticated, session->sock, session);
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
	
	if(sesKey->authType != LOGIN_ID_GUEST)
	{	dbg_msg("%s", "[==WEB==] Bootloader OK.\r\n");
		sysStsFlag1.execHramFlag = EXEC_HRAM_FLAG_TFTP_UPDATE;
		setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1, sysStsFlag1.sysStatusFlags);
		sysCtrl->isReadyToReboot = 1;
	}
  
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_GetDebug(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_GetDebug(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	DEBUG_STRUCT *dbg = (DEBUG_STRUCT *)&sensSys->dbg;
	
	sendData = SENS_MEM_ZALLOC(2048);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
	
	if(sesKey->authType != LOGIN_ID_GUEST)
	{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_DEBUG);
		webDbgMsgLock();
		if(dbg->webDbgMsgLength > 0 && dbg->webDbgMsg)
		{	strcat(sendData, (char *)dbg->webDbgMsg);
			dbg->webDbgMsgLength = 0;
			memset(dbg->webDbgMsg, 0, 1024);
		}
		else
		{	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_DEBUG);
		}
		webDbgMsgUnlock();
	}
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_TimeExtend(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_TimeExtend(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_TIME_EXTEND);
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_UnTimeExtend(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_UnTimeExtend(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_TIME_EXTEND);
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_avdsCalibration(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_avdsCalibration(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_AVDS_CALIBRATION);
	avdsCalibration();
	//radar2PwrCtl(&timeTick, 0, 0, 0);
	//radar2PwrCtl(&timeTick, 0, 1, 0);
	//dualRadarRunningIndex = 0;
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);	
}
//------------------------------------------------------------------------------
#if 1
uint8_t inBuf[3], outBuf[4];

static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//------------------------------------------------------------------------------
static void base64Encodeblock(unsigned char *in, unsigned char *out, int len)
{	out[0] = (unsigned char)cb64[(int)(in[0] >> 2)];
	out[1] = (unsigned char)cb64[(int)(((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4))];
	out[2] = (unsigned char)(len > 1 ? cb64[(int)(((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6))] : '=');
	out[3] = (unsigned char)(len > 2 ? cb64[(int)(in[2] & 0x3f)] : '=');
}
//------------------------------------------------------------------------------
int hexBase64Encode(uint8_t *source, uint8_t *destination, int srcLength, int offset)
{	int i, n, p, len;
	i = 0;
	n = 0;
	p = 0;
	
	while(n < srcLength) 
	{	len = 0;
		for(i = 0; i < 3; i++) 
		{	inBuf[i] = 0;
			if(n < srcLength)
			{	inBuf[i] = source[n++];
				len++;
			}
		}
		if(len) 
		{	base64Encodeblock(inBuf, outBuf, len);
			for(i = 0; i < 4; i++) 
			{	destination[p]= outBuf[i];
				p++;
			}			
		}
	}
	return p;
}
#endif
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_sendGetImage(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_sendGetImage(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
	char *sendData;
	char *imgBin;
	char *imgBase64;
#ifdef FSL_HTTP
  char *param;
#endif
	char sep[2];
	int startPos;
	char *fileName;
	int fileSize;
	int length;
	char *p;
	int base64Length;
	SESSION_KEY *sesKey = NULL;
	CAMERA_WEB_MANAGER	*camWebMng;
	int	imgBufSize;

	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
	strcpy(sep, "|");
	fileName = SENS_MEM_ZALLOC(IMG_FILE_SIZE);
	strtokLock();
	p = strtok(param, sep);
	startPos = _io_atoi(p);
	p = strtok(NULL, sep);
	memcpy(fileName, p, strlen(p));
	p = strtok(NULL, sep);
	strtokUnLock();
	fileSize = _io_atoi(p);
  
	camWebMng = (CAMERA_WEB_MANAGER *)&sysCtrl->camWebMng;
	//dbgMsg("Web get image, filename:%s, startPos:%d, fileSize:%d\r\n", fileName, startPos, fileSize);
	if(startPos == 0)
	{	//camWebMng->base64StartPos = 0;
		camWebMng->unalignSize = 0;
		/*if(isFileExist("DCIM/test.bin"))
		{	sdFileDelete("DCIM/test.bin");
		}*/
		//dbgMsg("%s", "debug\r\n");
	}
	//camWebMng->unalignSize = fileSize % 3;
	//SENS_TIME_DELAY(10);
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_SEND_IMAGE);
	
	imgBin = SENS_MEM_ZALLOC(ROUNDUP(fileSize, 3) + camWebMng->unalignSize);
	if(camWebMng->unalignSize)
		memcpy(imgBin, camWebMng->unalignBuf, camWebMng->unalignSize);
	
	length = sdReadFile(fileName, &imgBin[camWebMng->unalignSize], fileSize, startPos, BIN_READ_MODE);
	if(length == 0)
	{	dbg_msg("Web get image, read size 0, must read:%d\r\n", fileSize);
	}
	
	if((fileSize+startPos) >= camWebMng->fileLength)
	{	camWebMng->imgUploadToWebDone = 1;
		imgBufSize = fileSize + camWebMng->unalignSize;
		camWebMng->unalignSize = 0;
	}
	else
	{	imgBufSize = ROUNDDOWN(fileSize + camWebMng->unalignSize, 3);
		camWebMng->unalignSize = fileSize + camWebMng->unalignSize - imgBufSize;
		if(camWebMng->unalignSize)
			memcpy(camWebMng->unalignBuf, &imgBin[imgBufSize], camWebMng->unalignSize);
	}

	base64Length = 4 * ceil(imgBufSize / 3);
	//dbgMsg("base64 alloc size %d, imgBuf Size: %d, un-align size:%d\r\n", base64Length, imgBufSize, camWebMng->unalignSize);
	//imgBase64 = SENS_MEM_ZALLOC(fileSize*2 + strlen(sendData)+256);
	imgBase64 = SENS_MEM_ZALLOC(base64Length+1);
	base64Length = hexBase64Encode((uint8_t *)imgBin, (uint8_t *)imgBase64, imgBufSize, startPos);
	SENS_MEM_FREE(imgBin);
	//dbg_msg("Web get image, base64Length:%d\r\n", base64Length);
	imgBin = SENS_MEM_ZALLOC(base64Length + strlen(sendData)+256);
	//sdWriteFile("DCIM/test.bin", imgBase64, base64Length, camWebMng->base64StartPos, OVER_WRITE_MODE);
	//camWebMng->base64StartPos += base64Length;
	strcat(imgBin, sendData);
	//strcat(imgBin, imgBase64);
	memcpy(&imgBin[strlen(sendData)], imgBase64, base64Length);
		
	//imgBase64 = SENS_MEM_ZALLOC(fileSize*2);
	//hexBase64Encode(imgBase64, fileSize*2, &base64Length, imgBin, fileSize);
	
	//imgBase64 = (char *)base64Encode((char *)imgBin);
	//SENS_MEM_FREE(imgBin);
	//imgBin = SENS_MEM_ZALLOC(base64Length + strlen(sendData)+2);
	
	
	/*imgBase64 = SENS_MEM_ZALLOC(fileSize + strlen(sendData)+2);
	strcat(imgBase64, sendData);
	base64Length =strlen(imgBase64);
	memcpy(&imgBase64[strlen(imgBase64)], imgBin, fileSize);
	base64Length += fileSize;*/
	
	
	//strcat(imgBin, sendData);
	//strcat(imgBin, imgBase64);
	//imgBase64 = SENS_MEM_ZALLOC(fileSize*2);
	SENS_MEM_FREE(sendData);
	//SENS_MEM_FREE(imgBin);
	SENS_MEM_FREE(imgBase64);
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(imgBin);
	//CGI_SEND_STR(imgBase64);
	//SENS_MEM_FREE(imgBase64);
	//return session->request.content_len;
	#endif
#endif
	CGI_RETURN(imgBin);	
	//CGI_RETURN(imgBase64);	
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_checkImg(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_checkImg(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
#ifdef FSL_HTTP
	char *param;
#endif
	int mode;
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	CAMERA_WEB_MANAGER	*camWebMng = (CAMERA_WEB_MANAGER *)&sysCtrl->camWebMng;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
  
	mode = _io_atoi(param);
  
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_CHECK_IMAGE);

	if(mode == 0)
	{	camWebMng->status = CWS_STS_TRIG;
		dbg_msg("%s", "web get img, mode 0, shot only\r\n");
	}
	else if(mode == 1)
	{	camWebMng->status = CWS_STS_TRIG;
		camWebMng->sendToCloud = 1;
		camWebMng->imgUploadToWebDone = 0;
		camWebMng->readyToSendImgToCloud = 0;
		dbg_msg("%s", "web get img, mode 1, send to cloud\r\n");
	}
	else if(mode == 3)
	{	camWebMng->status = CWS_STS_IDLE;
		camWebMng->sendToCloud = 0;
		sdFileDelete(camWebMng->fileName);
	}
  
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", camWebMng->status);
  
	if(mode == 2)
	{	if(camWebMng->status == CWS_STS_DONE)
		{	SENS_SPRINTF(sendData+strlen(sendData), "%s\n", camWebMng->fileName);
			camWebMng->fileLength = getFileLength(camWebMng->fileName);
			SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", camWebMng->fileLength);
		}
	}
	
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);	
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_temporaryDisSlp(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_temporaryDisSlp(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
#ifdef FSL_HTTP
	char *param;
#endif
	int mode;
	int currentTime;
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
  
	mode = _io_atoi(param);
  
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_TEMPORARY_DIS_SLP);
	
	if(mode == 0)
	{	sysCtrl->enTemporaryDisSleep = 0;
		setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_TEMPORARY_DIS_PSM);
		dbg_msg("%s", "Disable temporary disable sleep\r\n");
	}
	else if(mode == 1)
	{	currentTime = GetTimeTicks();
		setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_TEMPORARY_DIS_PSM);
		sysCtrl->temporaryDisSleepEndTimer = currentTime + TEMPORARY_DISABLE_TIME;
		sysCtrl->temporaryDisSleepremainTime = TEMPORARY_DISABLE_TIME;
		sysCtrl->enTemporaryDisSleep = 1;
		dbg_msg("%s", "Enable temporary disable sleep\r\n");
	}
  
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);	
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_chgSta(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_chgSta(char *param)
#endif
{	
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
#ifdef FSL_HTTP
	char *param;
#endif
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
  
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_SAVE);
 #if N_TEMP_REMOVE
	stationChangeProcess();	//??¡ςˮ?Í
#endif
	sysCtrl->isReadyToReboot = 1;
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);	
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_chgSensor(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_chgSensor(char *param)
#endif
{
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
#ifdef FSL_HTTP
	char *param;
#endif
	int sensorType;
	char *sendData;
	SESSION_KEY *sesKey = NULL;
	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
  
	sensorType = _io_atoi(param);
  
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_SAVE);
#if N_TEMP_REMOVE
	changeSensorType(sensorType);	//??¡ςˮ?Í
#endif
	sysCtrl->isReadyToReboot = 1;
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);	
}
//------------------------------------------------------------------------------
#if defined FSL_HTTP
_mqx_int cgi_setNewCoord(HTTPD_SESSION_STRUCT *session)
#elif defined NATIVE_HTTP
const char *cgi_setNewCoord(char *param)
#endif
{
#if defined NEW_HTTP_STRUCT
	HTTPSRV_CGI_RES_STRUCT response;
#endif
#ifdef FSL_HTTP
  char *param;
#endif
	char *p;
	char *sendData;
	SESSION_KEY *sesKey = NULL;

	sendData = SENS_MEM_ZALLOC(HTTP_NODE_MAX_SIZE);
	if(CHK_CGI_IS_VALID())
	{	CGI_RETURN(sendData);
	}
  
	param = GET_START_DATA_PTR();
	param += strlen(PSTART);
  
	SENS_SPRINTF(sendData+strlen(sendData), "%ld\n", GET_SAVE);
#if N_TEMP_REMOVE
	setNewInitCoordinates();	//?΁x҆?铃
#endif
#ifdef FSL_HTTP
	#if defined NEW_HTTP_STRUCT
	#else
	CGI_SEND_STR(sendData);
	#endif
#endif
	CGI_RETURN(sendData);	
}
//------------------------------------------------------------------------------
#ifdef NATIVE_HTTP
void *fs_state_init(struct fs_file *file, const char *name)
{
  char *ret;
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(name);
  ret = (char *)mem_malloc(MAX_CGI_LEN);
  if (ret) {
    *ret = 0;
  }
  return ret;
}

void fs_state_free(struct fs_file *file, void *state)
{
  LWIP_UNUSED_ARG(file);
  if (state != NULL) {
    mem_free(state);
  }
}

void httpd_cgi_handler(struct fs_file *file, const char* uri, int iNumParams, char **pcParam, char **pcValue
#if defined(LWIP_HTTPD_FILE_STATE) && LWIP_HTTPD_FILE_STATE
                                     , void *connection_state
#endif /* LWIP_HTTPD_FILE_STATE */
                                     )
{	LWIP_UNUSED_ARG(file);
	LWIP_UNUSED_ARG(uri);
	if(connection_state != NULL)
	{	char *start = (char *)connection_state;
		char *end = start + MAX_CGI_LEN;
		int i;
		memset(start, 0, MAX_CGI_LEN);
		/* print a string of the arguments: */
		for (i = 0; i < iNumParams; i++)
		{	size_t len;
			len = end - start;
			if (len)
			{	size_t inlen = strlen(pcParam[i]);
				size_t copylen = LWIP_MIN(inlen, len);
				memcpy(start, pcParam[i], copylen);
				start += copylen;
				len -= copylen;
			}
			if (len)
			{	*start = '=';
				start++;
				len--;
			}
			if (len)
			{	size_t inlen = strlen(pcValue[i]);
				size_t copylen = LWIP_MIN(inlen, len);
				memcpy(start, pcValue[i], copylen);
				start += copylen;
				len -= copylen;
			}
			if (len)
			{	*start = ';';
				len--;
			}
			/* ensure NULL termination */
			end--;
			*end = 0;
		}
	}
}
#endif
