#include "param.h"
#include "sensSystem.h"
#include "network/netApp.h"
#include "physicalQuantity.h"
#include "ini2.h"
#include "sensCommon.h"
#include "sdCardTask.h"
#if SPI_FILE_SYSTEM
#include "littleFs/littleFsPort.h"
#endif
#include "sensDev.h"

#if defined NET_LWIP && defined HTTP_USE_FSL_LWIP
#include "network/httpsrv/Httpsrv_base64.h"
#endif

#include "cJSON/cJSON.h"

#define TEST_IOT_CH_0	1
#define TEST_LTE		0

#if TEST_LTE
#define TEST_APN_NAME	"internet"
#else
#define TEST_APN_NAME	"internet.iot"
#endif

#define MATCH(s, n) 			strcmp(section, s) == 0 && strcmp(name, n) == 0
#define SECTION_MATCH(s)		strcmp(section, s) == 0
#define SECTION_NOT_MATCH(s)	strcmp(section, s) != 0
#define NAME_MATCH(n)			strcmp(name, n) == 0

#define _MAX_CMD_LINE_ARGS   32
char *gArgv[_MAX_CMD_LINE_ARGS+1];

//------------------------------------------------------------------------------
/*void setSystemCfg(void)
{	SENS_SYS_CONFIG	*sensSysCfg;
	
	sensSysCfg = (SENS_SYS_CONFIG *)&sysCfg->sensSysCfg;
	
	sensSysCfg->recordSec = 600;
	sensSysCfg->autoSendInterval = 60;
	sensSysCfg->psm = SYS_CFG_PSM_NONE;
	sensSysCfg->WakeUpInterval = 30;
	sensSysCfg->logToSd = 0;
	sensSysCfg->alertEnable = 0;
	sensSysCfg->autoFotaChk = 1;
	sensSysCfg->imgServerType = 0;
	
}*/
//------------------------------------------------------------------------------
void setMobileCfg(NET_CONFIG *netCfg)
{	WIRELESS_CONFIG	*wirelessCfg;
	MOBILE_CONFIG	*lteCfg;
	
	wirelessCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
	wirelessCfg->mobileInterval = 1;
#if TEST_IOT_CH_0
	wirelessCfg->channelType[HS_PCIE_CH] = PCIE_3G_LTE;
	wirelessCfg->channelType[LS_PCIE_CH] = PCIE_NO_MODULE;
#else
	wirelessCfg->channelType[HS_PCIE_CH] = PCIE_NO_MODULE;
	wirelessCfg->channelType[LS_PCIE_CH] = PCIE_3G_LTE;
#endif
#if TEST_LTE
	wirelessCfg->lteCfg = SENS_MEM_ZALLOC(sizeof(MOBILE_CONFIG));
	lteCfg = wirelessCfg->lteCfg;
#else
	wirelessCfg->nbCfg = SENS_MEM_ZALLOC(sizeof(MOBILE_CONFIG));
	lteCfg = wirelessCfg->nbCfg;
#endif
	lteCfg->apn = SENS_MEM_ZALLOC(strlen(TEST_APN_NAME) + 1);
	strcpy(lteCfg->apn, TEST_APN_NAME);
	lteCfg->simAuth = SIM_AUTH_NONE;
	lteCfg->plmn = 46692;
}
//------------------------------------------------------------------------------
void setEthCfg(NET_CONFIG *netCfg)
{	WIRED_CONFIG	*wiredCfg = (WIRED_CONFIG *)&netCfg->wiredCfg;
	NET_INSTANCE	*lanInst = networkCtx->lanInst;
	WIRED_INSTANCE	*wiredInst = lanInst->wiredInst;
	
	wiredCfg->ipAddr = IPADDR(192, 168, 255, 1);
	wiredCfg->maskAddr = IPADDR(255, 255, 255, 0);
	wiredCfg->gwAddr = IPADDR(192, 168, 255, 254);
}
//------------------------------------------------------------------------------
/*void setNetworkCfg(void)
{	NET_CONFIG		*netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	DNS_CONFIG		*dnsCfgs;
	
	netCfg->connPriority = COMM_PRIO_WIRELESS_LAN;
	setEthCfg(netCfg);
	setMobileCfg(netCfg);
	dnsCfgs = SENS_MEM_ZALLOC(sizeof(DNS_CONFIG));
	strcpy(dnsCfgs->dnsServerStr, "168.95.1.1");
	
	netCfg->dnsCfgs = dnsCfgs;
}*/
//------------------------------------------------------------------------------
void addServInfo(CLOUD_SERVER_INFO	*servInfo)
{	CLOUD_SERVER_INFO	*servInfoTemp;
	
	if(sysCfg->servInfos == NULL)
	{	sysCfg->servInfos = servInfo;
	}
	else
	{	servInfoTemp = sysCfg->servInfos;
		while(servInfoTemp->next)
		{	servInfoTemp = servInfoTemp->next;
		}
		servInfoTemp->next = servInfo;
	}
}
//------------------------------------------------------------------------------
void addYsMqttCloudServerCfg(void)
{	CLOUD_SERVER_INFO	*servInfo;
	
	servInfo = SENS_MEM_ZALLOC(sizeof(CLOUD_SERVER_INFO));
	servInfo->serverProtocol = MQTT_YS_BROKER;
	strcpy(servInfo->serverDomainName, "59.120.52.21");
	servInfo->serverPort = 1883;	//test TLS
	servInfo->servIdx = 2;
	addServInfo(servInfo);
}
//------------------------------------------------------------------------------
void setAuthTable(SYS_AUTH_STRUCT *authTable, char *username, char *psw)
{	char *base64EncodeStr;
	
	authTable->username = SENS_MEM_ZALLOC(strlen(username) + 1);
	strcpy(authTable->username, username);
	
	authTable->psw = SENS_MEM_ZALLOC(strlen(psw) + 1);
	strcpy(authTable->psw, psw);
	
	base64EncodeStr = (char *)base64Encode(username);
	authTable->usernameBase64 = base64EncodeStr;
	
	base64EncodeStr = (char *)base64Encode(psw);
	authTable->pswBase64 = base64EncodeStr;
}
//------------------------------------------------------------------------------
void addAuthTable(SYS_AUTH_STRUCT *authTable)
{	SYS_AUTH_INFO	*sysAuthInf;
	SYS_AUTH_STRUCT *authTableTemp;
	
	sysAuthInf = sensSys->sysAuthInf;
	if(sysAuthInf->authTable == NULL)
	{	sysAuthInf->authTable = authTable;
	}
	else
	{	authTableTemp = sysAuthInf->authTable;
		while(authTableTemp->next)
		{	authTableTemp = authTableTemp->next;
		}
		authTableTemp->next = authTable;
	}
}
//------------------------------------------------------------------------------
void writeAuthFile(void)
{	SYS_AUTH_INFO	*sysAuthInf = sensSys->sysAuthInf;
	SYS_AUTH_STRUCT *authTable;
	cJSON *authArrays;
	cJSON *authObj;
	cJSON *authItem;
	uint8_t *jsonStrBuf;
	
	if(sysAuthInf == NULL)
		return;
	
	authArrays = cJSON_CreateArray();
	
	for(authTable = sysAuthInf->authTable;authTable;authTable=authTable->next)
	{	if(authTable->authType != LOGIN_ID_SU)
		{	authObj = cJSON_CreateObject();
			authItem = cJSON_CreateString(authTable->username);
			cJSON_AddItemToObject(authObj, "name", authItem);
			authItem = cJSON_CreateString(authTable->psw);
			cJSON_AddItemToObject(authObj, "psw", authItem);
			authItem = cJSON_CreateNumber(authTable->authType);
			cJSON_AddItemToObject(authObj, "type", authItem);
			cJSON_AddItemToArray(authArrays, authObj);
		}
	}
	
	jsonStrBuf = (uint8_t *)cJSON_PrintUnformatted(authArrays);
	SENS_SPI_FS_WRITE_FILE(AUTH_JSON_FILE, jsonStrBuf, strlen((char *)jsonStrBuf), 0, OVER_WRITE_MODE);
}
//------------------------------------------------------------------------------
void getUserId(void)
{	/*	
	[	{	"name": "xxx",
			"psw": "xxx"
			"type": 0
		},
	]
	*/
	SYS_AUTH_INFO	*sysAuthInf;
	SYS_AUTH_STRUCT *authTable;
	cJSON *authArrays;
	cJSON *authObj;
	cJSON *authItem;
	char *jsonBuf;
	int fileSize;
	int readLength;
	int items, idx;
	char *username;
	char *psw;
	int type;
	
	
	sensSys->sysAuthInf = SENS_MEM_ZALLOC(sizeof(SYS_AUTH_INFO));
	sysAuthInf = sensSys->sysAuthInf;
	
	authTable = SENS_MEM_ZALLOC(sizeof(SYS_AUTH_STRUCT));
	setAuthTable(authTable, "anasystem", "80210014");
	authTable->authType = LOGIN_ID_SU;
	addAuthTable(authTable);
	sysAuthInf->suCount++;
	
	if(SENS_SPI_FS_FILE_EXIST(AUTH_JSON_FILE) == SPI_FILE_IS_EXIST)
	{	fileSize = SENS_SPI_FS_GET_FILE_SIZE(AUTH_JSON_FILE);
		jsonBuf = SENS_MEM_ZALLOC(fileSize);
		readLength = SENS_SPI_FS_READ_FILE(AUTH_JSON_FILE, jsonBuf, fileSize, 0, NORMAL_READ_MODE);
		if(readLength != fileSize)
		{	SENS_MEM_FREE(jsonBuf);
			goto USE_DEFAULT_AUTH;
		}
		authArrays = cJSON_Parse(jsonBuf);
		SENS_MEM_FREE(jsonBuf);
		if((authArrays == NULL) || (!cJSON_IsArray(authArrays)))
		{	if(authArrays)
				cJSON_Delete(authArrays);
			goto USE_DEFAULT_AUTH;
		}
		items = cJSON_GetArraySize(authArrays);
		for(idx=0;idx<items;idx++)
		{	authObj = cJSON_GetArrayItem(authArrays, idx);
			if(cJSON_IsObject(authObj))
			{	username = NULL;
				psw = NULL;
				type = LOGIN_ID_MAX;
				authItem = cJSON_GetObjectItem(authObj, "name");
				if(authItem)
					username = cJSON_GetStringValue(authItem);
				authItem = cJSON_GetObjectItem(authObj, "psw");
				if(authItem)
					psw = cJSON_GetStringValue(authItem);
				authItem = cJSON_GetObjectItem(authObj, "type");
				if(authItem)
					type = (int)cJSON_GetNumberValue(authItem);
				if((type == LOGIN_ID_MAX) || (username == NULL) || (psw == NULL))
					continue;
				authTable = SENS_MEM_ZALLOC(sizeof(SYS_AUTH_STRUCT));
				setAuthTable(authTable, username, psw);
				authTable->authType = type;
				addAuthTable(authTable);
				if(type == LOGIN_ID_GUEST)
					sysAuthInf->userCount++;
				else if(type == LOGIN_ID_ADMIN)
					sysAuthInf->adminCount++;
				else if(type == LOGIN_ID_SU)
					sysAuthInf->suCount++;
			}
		}
		cJSON_Delete(authArrays);
		return;
	}
USE_DEFAULT_AUTH:
	authTable = SENS_MEM_ZALLOC(sizeof(SYS_AUTH_STRUCT));
	setAuthTable(authTable, "user", "123456");
	authTable->authType = LOGIN_ID_ADMIN;
	addAuthTable(authTable);
	sysAuthInf->adminCount++;
	authTable = SENS_MEM_ZALLOC(sizeof(SYS_AUTH_STRUCT));
	setAuthTable(authTable, "guest", "guest");
	authTable->authType = LOGIN_ID_GUEST;
	addAuthTable(authTable);
	sysAuthInf->userCount++;
	
	/*
	 * write to File
	 */
	writeAuthFile();
}
//------------------------------------------------------------------------------
void addExtInterfaceCfg(COMM_IF_CONFIG	*commIfCfg)
{	COMM_IF_CONFIG	*commIfCfgTemp;
	
	if(sysCfg->commIfCfgs == NULL)
		sysCfg->commIfCfgs = commIfCfg;
	else
	{	commIfCfgTemp = sysCfg->commIfCfgs;
		while(commIfCfgTemp->next)
		{	commIfCfgTemp = commIfCfgTemp->next;
		}
		commIfCfgTemp->next = commIfCfg;
	}
}
//------------------------------------------------------------------------------
void setExtInterfaceCfg(void)
{	COMM_IF_CONFIG	*commIfCfg;
	
	commIfCfg = SENS_MEM_ZALLOC(sizeof(COMM_IF_CONFIG));
	commIfCfg->channelNo = EXT_IF_CHANNEL_RS485_1;
	commIfCfg->baudrate = 9600;
	commIfCfg->format = N_8_1;
	commIfCfg->protocol = EXT_IF_PROTO_NONE;
	addExtInterfaceCfg(commIfCfg);
	
	commIfCfg = SENS_MEM_ZALLOC(sizeof(COMM_IF_CONFIG));
	commIfCfg->channelNo = EXT_IF_CHANNEL_RS485_2;
	commIfCfg->baudrate = 9600;
	commIfCfg->format = N_8_1;
	commIfCfg->protocol = EXT_IF_PROTO_NONE;
	addExtInterfaceCfg(commIfCfg);
	
	commIfCfg = SENS_MEM_ZALLOC(sizeof(COMM_IF_CONFIG));
	commIfCfg->channelNo = EXT_IF_CHANNEL_RS232;
	commIfCfg->baudrate = 115200;
	commIfCfg->format = N_8_1;
	commIfCfg->protocol = EXT_IF_PROTO_NONE;
	addExtInterfaceCfg(commIfCfg);
	
	commIfCfg = SENS_MEM_ZALLOC(sizeof(COMM_IF_CONFIG));
	commIfCfg->channelNo = EXT_IF_CHANNEL_ETH;
	commIfCfg->protocol = EXT_IF_PROTO_NONE;
	addExtInterfaceCfg(commIfCfg);
	
	commIfCfg = SENS_MEM_ZALLOC(sizeof(COMM_IF_CONFIG));
	commIfCfg->channelNo = EXT_IF_CHANNEL_USB_HS;
	commIfCfg->protocol = EXT_IF_PROTO_NONE;
	addExtInterfaceCfg(commIfCfg);
}
#if SPI_FILE_SYSTEM
//------------------------------------------------------------------------------
static uint8_t *getFileBufFrom(int disk, char *filename, int *fileLengh)
{	uint8_t *fileBuf = NULL;
	int fileSize;
	int readLength;
	//SENS_FILE_PTR fd_ptr;
	
	
	if(disk == FROM_SD_CARD)
	{	if(isFileExist(filename))
		{	fileSize = getFileLength(filename);
			if(fileSize)
			{	fileBuf = SENS_MEM_ZALLOC(fileSize);
				readLength = sdReadFile(filename, (char *)fileBuf, fileSize, 0, NORMAL_READ_MODE);
			}
		}
		else
		{	fileSize = 0;
			readLength = 0;
		}
	}	
	else if(disk == FROM_SPI_NOR)
	{	if(SENS_SPI_FS_FILE_EXIST(filename) == SPI_FILE_IS_EXIST)
		{	fileSize = SENS_SPI_FS_GET_FILE_SIZE(filename);
			if(fileSize)
			{	fileBuf = SENS_MEM_ZALLOC(fileSize);
				readLength = SENS_SPI_FS_READ_FILE(filename, (char *)fileBuf, fileSize, 0, NORMAL_READ_MODE);
			}
		}
		else
		{	fileSize = 0;
			readLength = 0;
		}
	}
	
	*fileLengh = fileSize;
	if(readLength != fileSize)
	{	if(fileBuf)
		{	SENS_MEM_FREE(fileBuf);
			fileBuf = NULL;
		}
		*fileLengh = 0;
	}
	return fileBuf;
}
//------------------------------------------------------------------------------
static int writeFileBufFrom(int destDisk, char *filename, uint8_t *writeBuf, int writeSize)
{	int ret = -1;
	int writeLen;
	
	if(destDisk == FROM_SD_CARD)
	{	writeLen = sdWriteFile(filename, (char *)writeBuf, writeSize, 0, OVER_WRITE_MODE);
		if(writeLen == writeSize)
			ret = 0;
	}
	else if(destDisk == FROM_SPI_NOR)
	{	writeLen = SENS_SPI_FS_WRITE_FILE(filename, writeBuf, writeSize, 0, OVER_WRITE_MODE);
		if(writeLen == writeSize)
			ret = 0;
	}
	return ret;
}
//------------------------------------------------------------------------------
int backupParamFile(int srcDisk, int destDisk, int compare, int directCopy, int copySrc)
{	uint8_t *srcBuf;
	uint8_t	*destBuf;
	uint8_t *tempBufptr;
	int srcFileLength, destFileLength;
	int compareErr = 0;
	int idx;
	int ret;
	
	srcBuf = getFileBufFrom(srcDisk, INIFILE, &srcFileLength);
	//if(srcBuf == NULL)
	//	return BACKUP_PARAM_COMPARE_NO_SRC;
	destBuf = getFileBufFrom(destDisk, INIFILE, &destFileLength);
	//if(destBuf == NULL)
	//	return BACKUP_PARAM_COMPARE_NO_DEST;
	if(compare /*&& destBuf && srcBuf*/)
	{	if(srcFileLength != destFileLength)
		{	compareErr = 1;
			if(copySrc == FROM_NONE)
			{	return BACKUP_PARAM_COMPARE_ERROR;
			}
		}
		if(!compareErr)
		{	for(idx=0;idx<srcFileLength;idx++)
			{	if(srcBuf[idx] != destBuf[idx])
				{	compareErr = 1;
					break;
				}
			}
		}
		if(compareErr)
		{	if(copySrc == FROM_NONE)
				return BACKUP_PARAM_COMPARE_ERROR;
			
			directCopy = 1;
		}
	}
	
	if(directCopy)
	{	if(copySrc == FROM_SD_CARD)
		{	if(srcDisk == FROM_SPI_NOR)
			{	tempBufptr = srcBuf;
				srcBuf = destBuf;
				destBuf = tempBufptr;
				srcFileLength = destFileLength;
				srcDisk = FROM_SD_CARD;
				destDisk = FROM_SPI_NOR;
			}
		}
		else if(copySrc == FROM_SPI_NOR)
		{	if(srcDisk == FROM_SD_CARD)
			{	tempBufptr = srcBuf;
				srcBuf = destBuf;
				destBuf = tempBufptr;
				srcFileLength = destFileLength;
				srcDisk = FROM_SPI_NOR;
				destDisk = FROM_SD_CARD;
			}
		}
		if(srcBuf)
		{	dbg_msg("Backup Param Src:%s, Dest:%s\r\n", srcDisk==FROM_SPI_NOR? "NOR":"SD", destDisk == FROM_SPI_NOR? "NOR":"SD");
			ret = writeFileBufFrom(destDisk, INIFILE, srcBuf, srcFileLength);
		}
	}
	else
		ret = 0;
		
	if(destBuf)
		SENS_MEM_FREE(destBuf);
	if(srcBuf)
		SENS_MEM_FREE(srcBuf);
	return ret;
}
//------------------------------------------------------------------------------
static void chkParamInSpi(void)
{	if(sysCtrl->bakupFlashIsPresent)
	{	if(SENS_SPI_FS_FILE_EXIST(INIFILE) == SPI_FILE_IS_EXIST)
		{	if(backupParamFile(FROM_SPI_NOR, FROM_SD_CARD, 0, 1, FROM_SPI_NOR) == 0)
			{	sysCtrl->paramInSd = 1;
			}
		}
		else
		{	dbg_msg("%s", "[SD CARD] SPI FLASH NO PARAM FILE!!\r\n");
			sysCtrl->sdCardError = 1;
		}
	}
	else
	{	dbg_msg("%s", "[SD CARD] NO SPI FLASH FS!!\r\n");
		sysCtrl->sdCardError = 1;
	}
}
#endif
//------------------------------------------------------------------------------
static int cmdLineToArgcArgv(char *cmdLine)
{	int argc;
	char *pszCmdLine;

	gArgv[0] = 0;
	pszCmdLine = cmdLine;
	if('"' == *pszCmdLine)
	{	pszCmdLine++;
		gArgv[0] = pszCmdLine;

		while (*pszCmdLine && (*pszCmdLine != '"')) pszCmdLine++;
		if(*pszCmdLine)
			*pszCmdLine++ = 0;
		else
			return 0;
	}
	else
	{	gArgv[0] = pszCmdLine;

		while (*pszCmdLine && (' ' != *pszCmdLine) && ('\t' != *pszCmdLine))
			pszCmdLine++;
		if(*pszCmdLine)
			*pszCmdLine++ = 0;
	}
	argc = 1;
	while (1)
	{	while(*pszCmdLine && ((' ' == *pszCmdLine) || ('\t' == *pszCmdLine)))
			pszCmdLine++;
    
		if(0 == *pszCmdLine)
			return argc;
    
		if ('"' == *pszCmdLine)
		{	pszCmdLine++;
			gArgv[argc++] = pszCmdLine;
			gArgv[argc] = 0;

			while(*pszCmdLine && (*pszCmdLine != '"'))
				pszCmdLine++;
			if(0 == *pszCmdLine)
				return argc;
			if (*pszCmdLine)
				*pszCmdLine++ = 0;
		}
		else
		{	gArgv[argc++] = pszCmdLine;
			gArgv[argc] = 0;
			
			while(*pszCmdLine && (' '!=*pszCmdLine) && ('\t'!=*pszCmdLine))
				pszCmdLine++;
      
			if(0 == *pszCmdLine)
				return argc;
			if(*pszCmdLine)
				*pszCmdLine++ = 0;
		}
		if(argc >= (_MAX_CMD_LINE_ARGS)) 
			return argc;
	}
}
//------------------------------------------------------------------------------
static int cmdLineToArgcArgv1(char *cmd, const char *value)
{	memset(cmd, 0, KEY_VAL_LEN);
	strcpy(cmd, value);
	return cmdLineToArgcArgv(cmd);
}
//------------------------------------------------------------------------------
static int handler1(void* user, const char* section, const char* name, const char* value)
{	int idx, tmp_i, device_count, channel_count;
	char *end;
	char cmd[KEY_VAL_LEN], key_name[36], channel_key_name[36];                    // in ini2.h
	memset(key_name, 0, sizeof(char)*36);
	memset(channel_key_name, 0, sizeof(char)*36);
	char argcNum;
	char sep[2], ip[4];
	char *IP_add, *p;
	char *mdvpnPosStart;
	char *mdvpnPosEnd;
	int enable;
	
	if (value == NULL || strlen(value) == 0) return 0;
	
	if(SECTION_NOT_MATCH(SECTION))
		return 1;
	if (memcmp(name, "device_", strlen("device_")) == 0)
	{	SENS_SSCANF((char *)name, "device_%d", &idx);
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		enable = _io_atoi(gArgv[0]);
		if(enable)
		{	EXT_DEV_CONFIG *extDevCfg;
			//SENS_SPRINTF(device_parameter, "1 %d %d %s %d %d %d", extIfIdx, protocol, device_command, start, type, sequence);
			extDevCfg = SENS_MEM_ZALLOC(sizeof(EXT_DEV_CONFIG));
			extDevCfg->devModel = EXT_DEV_MODEL_NORMAL;
			extDevCfg->devIdx = idx;
			extDevCfg->ifChannel = _io_atoi(gArgv[1]);
			extDevCfg->ifChannel--;	//web 0 is None, bit interface channel start from RS485_1
			extDevCfg->ifProtocol = _io_atoi(gArgv[2]);
			extDevCfg->command = SENS_MEM_ZALLOC(strlen(gArgv[3]));
			strcpy(extDevCfg->command, gArgv[3]);
			extDevCfg->startParseByte = _io_atoi(gArgv[4]);
			extDevCfg->dataType = _io_atoi(gArgv[5]);
			extDevCfg->dataOrder = _io_atoi(gArgv[6]);
			addExtDevConfig(extDevCfg);
		}
	}
	else if(memcmp(name, "FORMULA_COEF", strlen("FORMULA_COEF")) == 0)
	{	FORMULA_STRUCT	*formula;
		SENS_SSCANF((char *)name, "FORMULA_COEF%d", &idx);
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0)
			return 0;
		formula = SENS_MEM_ZALLOC(sizeof(FORMULA_STRUCT));
		formula->formulaIdx = _io_atoi(gArgv[0]);
		formula->aStr = SENS_MEM_ZALLOC(strlen(gArgv[1]) + 1);
		strcpy(formula->aStr, gArgv[1]);
		formula->bStr = SENS_MEM_ZALLOC(strlen(gArgv[2]) + 1);
		strcpy(formula->bStr, gArgv[2]);
		formula->type = 1;
		if(argcNum > 3)
		{	formula->cStr = SENS_MEM_ZALLOC(strlen(gArgv[3]) + 1);
			strcpy(formula->cStr, gArgv[3]);
			formula->type = 0;
		}
		formula->a = (float)_io_strtod((char*)formula->aStr, &end);
		formula->b = (float)_io_strtod((char*)formula->bStr, &end);
		
		if(formula->type == 0)
			formula->c = (float)_io_strtod((char*)formula->cStr, &end);
		
		addFormula(formula);
	}
	else if(memcmp(name, "channel_", strlen("channel_")) == 0)
	{	//SENS_SPRINTF(device_parameter, "1 %d %d %d %d %s %s", deviceNO, item, eqn, filter, alias, alias2);
		PQ_CONFIG *pqCfg;
		SENS_SSCANF((char *)name, "channel_%d", &idx);
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0)
			return 0;
		enable = _io_atoi(gArgv[0]);
		if(enable)
		{	pqCfg = SENS_MEM_ZALLOC(sizeof(PQ_CONFIG));
			pqCfg->pqIdx = idx;
			pqCfg->pqSrc.devNo = _io_atoi(gArgv[1]);
			pqCfg->pqSrc.devChanNo = _io_atoi(gArgv[2]);
			pqCfg->formulaId = _io_atoi(gArgv[3]);
			pqCfg->filterType = _io_atoi(gArgv[4]);
			if((argcNum > 5) && (strlen(gArgv[5]) > 1))
			{	pqCfg->alias[0] = SENS_MEM_ZALLOC(strlen(gArgv[5]));
				strcpy(pqCfg->alias[0], gArgv[5]);
			}
			if((argcNum > 6) && (strlen(gArgv[6]) > 1))
			{	pqCfg->alias[1] = SENS_MEM_ZALLOC(strlen(gArgv[6]));
				strcpy(pqCfg->alias[1], gArgv[6]);
			}
			addPqCfg(pqCfg);
		}
	}
	else if(NAME_MATCH(COM_PROTOCOL_str))
	{	COMM_IF_CONFIG *commifCfg;
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		commifCfg = getCommIfConfigByIfNo(EXT_IF_CHANNEL_RS485_1);
		commifCfg->protocol = _io_atoi(gArgv[0]);
		commifCfg = getCommIfConfigByIfNo(EXT_IF_CHANNEL_RS485_2);
		commifCfg->protocol = _io_atoi(gArgv[1]);
		commifCfg = getCommIfConfigByIfNo(EXT_IF_CHANNEL_RS232);
		commifCfg->protocol = _io_atoi(gArgv[2]);
		commifCfg = getCommIfConfigByIfNo(EXT_IF_CHANNEL_ETH);
		commifCfg->protocol = _io_atoi(gArgv[3]);
		commifCfg = getCommIfConfigByIfNo(EXT_IF_CHANNEL_USB_HS);
		commifCfg->protocol = _io_atoi(gArgv[4]);
	}
	else if(NAME_MATCH(ADC_CFG_str))
	{	AI_CONFIG *aiCfg = (AI_CONFIG *)&sysCfg->aiCfg;
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		aiCfg->enable = _io_atoi(gArgv[0]);
		aiCfg->differential = _io_atoi(gArgv[1]);
		aiCfg->sensorType[0] = AI_TYPE_4_20MA;
		aiCfg->sensorType[1] = AI_TYPE_4_20MA;
		aiCfg->sensorType[2] = AI_TYPE_4_20MA;
		aiCfg->sensorType[3] = AI_TYPE_500MV;
		aiCfg->sensorType[4] = AI_TYPE_500MV;
		if(argcNum > 2)	aiCfg->sensorType[0] = _io_atoi(gArgv[2]);
		if(argcNum > 3)	aiCfg->sensorType[1] = _io_atoi(gArgv[3]);
		if(argcNum > 4)	aiCfg->sensorType[2] = _io_atoi(gArgv[4]);
		if(argcNum > 5)	aiCfg->sensorType[3] = _io_atoi(gArgv[5]);
		if(argcNum > 6)	aiCfg->sensorType[4] = _io_atoi(gArgv[6]);
	}
	else if(NAME_MATCH(TRANSFER_PROTOCOL_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		wirelessCfg->channelType[0] = _io_atoi(gArgv[0]);
	}
	else if(NAME_MATCH(TRANSFER_PROTOCOL1_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		wirelessCfg->channelType[1] = _io_atoi(gArgv[0]);
	}
	else if(NAME_MATCH(RS485_1_str))
	{	COMM_IF_CONFIG *commIfCfg;
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		commIfCfg = getCommIfConfigByIfNo(EXT_IF_CHANNEL_RS485_1);
		commIfCfg->baudrate = _io_atoi(gArgv[0]);
		commIfCfg->format = _io_atoi(gArgv[4]);
	}
	else if(NAME_MATCH(RS485_2_str))
	{	COMM_IF_CONFIG *commIfCfg;
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		commIfCfg = getCommIfConfigByIfNo(EXT_IF_CHANNEL_RS485_2);
		commIfCfg->baudrate = _io_atoi(gArgv[0]);
		commIfCfg->format = _io_atoi(gArgv[4]);
	}
	else if(NAME_MATCH(RS232_1_str))
	{	COMM_IF_CONFIG *commIfCfg;
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		commIfCfg = getCommIfConfigByIfNo(EXT_IF_CHANNEL_RS232);
		commIfCfg->baudrate = _io_atoi(gArgv[0]);
		commIfCfg->format = _io_atoi(gArgv[4]);
	}
	else if(NAME_MATCH(SYSTEM_INSTALL_MODE_STR))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->sensSysCfg.sysIsInstallMode = _io_atoi(gArgv[0]);
	}
	else if(NAME_MATCH(AUTO_FOTA_CHK_MODE_STR))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->sensSysCfg.autoFotaChk = _io_atoi(gArgv[0]);
	}
	else if(NAME_MATCH(IMG_SERV_TYPE_STR))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->sensSysCfg.imgServerType = _io_atoi(gArgv[0]);
	}
	else if(NAME_MATCH(SD_REC_SEC_str))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->sensSysCfg.recordSec = _io_atoi(gArgv[0]);
	}
	else if(NAME_MATCH(CAMERA_PARAM_STR))
	{	CAMERA_CONFIG *cameraCfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;

		sysCfg->sensSysCfg.cameraRecInterval = _io_atoi(gArgv[0]);
		sysCfg->sensSysCfg.cameraAlertInterval = _io_atoi(gArgv[1]);
		cameraCfg = SENS_MEM_ZALLOC(sizeof(CAMERA_CONFIG));
		cameraCfg->resolution = _io_atoi(gArgv[2]);
		cameraCfg->quality = _io_atoi(gArgv[3]);
		sysCfg->cameraCfg = cameraCfg;
	}
	else if(NAME_MATCH(VW_PARAM_STR))
	{	VW_SENSOR_CONFIG	*vwCfg;
		int devNo;
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		devNo = _io_atoi(gArgv[0]);
		if(devNo)
		{	vwCfg = SENS_MEM_ZALLOC(sizeof(VW_SENSOR_CONFIG));
			vwCfg->devNo 				= devNo;
			vwCfg->slaveId 			= _io_atoi(gArgv[1]);
			vwCfg->pwrSrc 				= _io_atoi(gArgv[2]);
			vwCfg->pwrStableTime	= _io_atoi(gArgv[3]);
			vwCfg->pollingInterval	= _io_atoi(gArgv[4]);
			sysCfg->vwCfg = vwCfg;
		}
	}
	else if (NAME_MATCH(AUTOSEND_SEC_str))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->sensSysCfg.autoSendInterval = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(MOBILE_INTERVAL_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG	*)&netCfg->wirelessCfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		wirelessCfg->mobileInterval = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(PIC_POWERDOWN_str))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->sensSysCfg.psm = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(DI_WAKEUP_str))
	{	argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		sysCfg->diCfg.diWakeup[0] = _io_atoi(gArgv[0]);
		sysCfg->diCfg.diWakeup[1] = _io_atoi(gArgv[1]);
		if(argcNum > 2)
		{	sysCfg->diCfg.diWakeup[2] = _io_atoi(gArgv[2]);
			sysCfg->diCfg.diWakeup[3] = _io_atoi(gArgv[3]);
		}
	}
	else if(NAME_MATCH(DI_WAKEUP_REC_INTERVAL_STR))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->diCfg.diWakeupRecInterval = _io_atoi(gArgv[0]);
	}
#if SUPPORT_CY_PUMP		
	else if (NAME_MATCH(DI_FUNCTION_str))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->diCfg.cyPumpMode = _io_atoi(gArgv[0]);
	}
#endif		
	else if (NAME_MATCH(PRESSURE_WATER_LEVEL_GAUGE_STR))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->specFeas.isPressureWaterLevel = _io_atoi(gArgv[0]);     
	}
	else if (NAME_MATCH(WAKE_UP_INTERVAL_str))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->sensSysCfg.WakeUpInterval = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(IP_ADDRESS_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRED_CONFIG *wiredCfg = (WIRED_CONFIG *)&netCfg->wiredCfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
			
		mdvpnPosStart = strstr(gArgv[0], "[");
		if(mdvpnPosStart)
		{	mdvpnPosStart++;
			mdvpnPosEnd = strstr(mdvpnPosStart, "]");
			if(mdvpnPosEnd)
			{	*mdvpnPosEnd = '\0';
				if(!strcmp(mdvpnPosStart, "MDVPN"))
					wiredCfg->mdvpnType = LTE_MDVPN_TYPE_SENSLINK_MDVPN;
				else if(!strcmp(mdvpnPosStart, "mdvpn"))
					wiredCfg->mdvpnType = LTE_MDVPN_TYPE_OTHER_MDVPN;
				mdvpnPosStart = mdvpnPosEnd + 1;
			}
			else
				mdvpnPosStart = gArgv[0];
		}
		else
			mdvpnPosStart = gArgv[0];
		
		strcpy(sep, ".");
		IP_add = mdvpnPosStart;
		p = strtok(IP_add, sep);
		if(p)	{	ip[0] = _io_atoi(p);	}
		p = strtok(NULL, sep);
		if(p)	{	ip[1] = _io_atoi(p);	}
		p = strtok(NULL, sep);
		if(p)	{	ip[2] = _io_atoi(p);	}
		p = strtok(NULL, sep);
		if(p)	{	ip[3] = _io_atoi(p);	}
		wiredCfg->ipAddr = IPADDR(ip[0], ip[1], ip[2], ip[3]);//ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
	}
	else if (NAME_MATCH(IP_MASK_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRED_CONFIG *wiredCfg = (WIRED_CONFIG *)&netCfg->wiredCfg;
		
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		strcpy(sep, ".");
		IP_add = gArgv[0];
		p = strtok(IP_add, sep);
		if(p)	{	ip[0] = _io_atoi(p);	}
		p = strtok(NULL, sep);
		if(p)	{	ip[1] = _io_atoi(p);	}
		p = strtok(NULL, sep);
		if(p)	{	ip[2] = _io_atoi(p);	}
		p = strtok(NULL, sep);
		if(p)	{	ip[3] = _io_atoi(p);	}
		wiredCfg->maskAddr = IPADDR(ip[0], ip[1], ip[2], ip[3]);	//ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
		//tcp.curr_ip_mask = tcp.ip_mask;
	}
	else if (NAME_MATCH(IP_GATEWAY_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRED_CONFIG *wiredCfg = (WIRED_CONFIG *)&netCfg->wiredCfg;
		
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		strcpy(sep, ".");
		IP_add = gArgv[0];
		p = strtok(IP_add, sep);
		if(p)	{	ip[0] = _io_atoi(p);	}
		p = strtok(NULL, sep);
		if(p)	{	ip[1] = _io_atoi(p);	}
		p = strtok(NULL, sep);
		if(p)	{	ip[2] = _io_atoi(p);	}
		p = strtok(NULL, sep);
		if(p)	{	ip[3] = _io_atoi(p);	}
		wiredCfg->gwAddr = IPADDR(ip[0], ip[1], ip[2], ip[3]);//ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
		//tcp.curr_ip_gateway = tcp.ip_gateway;
	}
	else if (NAME_MATCH(DNS_ADDRESS_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		DNS_CONFIG *dnsCfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		//------------------------------------DNS Address---------------------------
		tmp_i = strlen(gArgv[0]);
		if(tmp_i >= IP_MAX_COMMAND_LEN) return 0;
		if(gArgv[0][0] != '0')
		{	dnsCfg = SENS_MEM_ZALLOC(sizeof(DNS_CONFIG));
			memcpy(dnsCfg->dnsServerStr, gArgv[0], tmp_i);
			strcpy(sep, ".");
			IP_add = gArgv[0];
			p = strtok(IP_add, sep);
			if(p)	{	ip[0] = _io_atoi(p);	}
			p = strtok(NULL, sep);
			if(p)	{	ip[1] = _io_atoi(p);	}
			p = strtok(NULL, sep);
			if(p)	{	ip[2] = _io_atoi(p);	}
			p = strtok(NULL, sep);
			if(p)	{	ip[3] = _io_atoi(p);	}
			dnsCfg->dnsSvrAddr = ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
			addDnsServerCfg(dnsCfg);
		}
		//------------------------------------DNS Address---------------------------
	}
	else if (NAME_MATCH(PRIORITY_CONNECT_str))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->netCfg.connPriority = (uint8_t)_io_atoi(value);
	}
	else if (NAME_MATCH(SENSLINK_ADDRESS_str))
	{	CLOUD_SERVER_INFO *servInfo;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		tmp_i = strlen(gArgv[0]);
		if (tmp_i >= 128) return 0;
		servInfo = getServerInfoById(0);
		if(servInfo == NULL)
		{	servInfo = SENS_MEM_ZALLOC(sizeof(CLOUD_SERVER_INFO));
			servInfo->servIdx = 0;
			addServInfo(servInfo);
		}
		memcpy(servInfo->serverDomainName, gArgv[0], tmp_i);
	}
	else if (NAME_MATCH(SENSLINK_PORT_str))
	{	CLOUD_SERVER_INFO *servInfo;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		servInfo = getServerInfoById(0);
		if(servInfo == NULL)
		{	servInfo = SENS_MEM_ZALLOC(sizeof(CLOUD_SERVER_INFO));
			servInfo->servIdx = 0;
			addServInfo(servInfo);
		}
		servInfo->serverPort = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(SERV1_TYPE_str))
	{	CLOUD_SERVER_INFO *servInfo;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		servInfo = getServerInfoById(0);
		if(servInfo == NULL)
		{	servInfo = SENS_MEM_ZALLOC(sizeof(CLOUD_SERVER_INFO));
			servInfo->servIdx = 0;
			addServInfo(servInfo);
		}
		servInfo->serverProtocol = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(SENSLINK_ADDRESS2_str))
	{	CLOUD_SERVER_INFO *servInfo;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		tmp_i = strlen(gArgv[0]);
		if (tmp_i >= 128) return 0;
		servInfo = getServerInfoById(1);
		if(servInfo == NULL)
		{	servInfo = SENS_MEM_ZALLOC(sizeof(CLOUD_SERVER_INFO));
			servInfo->servIdx = 1;
			addServInfo(servInfo);
		}
		memcpy(servInfo->serverDomainName, gArgv[0], tmp_i);
	}
	else if (NAME_MATCH(SENSLINK_PORT2_str))
	{	CLOUD_SERVER_INFO *servInfo;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		servInfo = getServerInfoById(1);
		if(servInfo == NULL)
		{	servInfo = SENS_MEM_ZALLOC(sizeof(CLOUD_SERVER_INFO));
			servInfo->servIdx = 1;
			addServInfo(servInfo);
		}
		servInfo->serverPort = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(SERV2_TYPE_str))
	{	CLOUD_SERVER_INFO *servInfo;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		servInfo = getServerInfoById(1);
		if(servInfo == NULL)
		{	servInfo = SENS_MEM_ZALLOC(sizeof(CLOUD_SERVER_INFO));
			servInfo->servIdx = 1;
			addServInfo(servInfo);
		}
		servInfo->serverProtocol = _io_atoi(gArgv[0]);
	}
	else if(memcmp(name, MQTT_DEVICE_NAME_str, strlen(MQTT_DEVICE_NAME_str)) == 0)
	{	MQTT_CONNECT_INFO *mqttConnInfoTemp;
		
		SENS_SSCANF((char *)name, MQTT_DEVICE_NAME_str"%d", &channel_count);
		if(channel_count < UPLOAD_SERVER_CNT)
		{	if(cmdLineToArgcArgv1(cmd, value) == 0)	return 0;
			tmp_i = strlen(gArgv[0]);
			mqttConnInfoTemp = (MQTT_CONNECT_INFO *)&sysCfg->mqttConnInfoTemp[channel_count];
				
			if(mqttConnInfoTemp->clientId != NULL)
				SENS_MEM_FREE(mqttConnInfoTemp->clientId);
			mqttConnInfoTemp->clientId = (char *)SENS_MEM_ZALLOC(tmp_i+1);
			strcpy(mqttConnInfoTemp->clientId, gArgv[0]);
		}
	}
	else if(memcmp(name, MQTT_USERNAME_str, strlen(MQTT_USERNAME_str)) == 0)
	{	MQTT_CONNECT_INFO *mqttConnInfoTemp;
		SENS_SSCANF((char *)name, MQTT_USERNAME_str"%d", &channel_count);
		if(channel_count < UPLOAD_SERVER_CNT)
		{	if(cmdLineToArgcArgv1(cmd, value) == 0)
				return 0;
			tmp_i = strlen(gArgv[0]);
			mqttConnInfoTemp = (MQTT_CONNECT_INFO *)&sysCfg->mqttConnInfoTemp[channel_count];
			if(mqttConnInfoTemp->userName != NULL)
				SENS_MEM_FREE(mqttConnInfoTemp->userName);
			mqttConnInfoTemp->userName = (char *)SENS_MEM_ZALLOC(tmp_i+1);
			strcpy(mqttConnInfoTemp->userName, gArgv[0]);
		}
	}
	else if(memcmp(name, MQTT_PASSWORD_str, strlen(MQTT_PASSWORD_str)) == 0)
	{	MQTT_CONNECT_INFO *mqttConnInfoTemp;
		SENS_SSCANF((char *)name, MQTT_PASSWORD_str"%d", &channel_count);
		if(channel_count < UPLOAD_SERVER_CNT)
		{	if(cmdLineToArgcArgv1(cmd, value) == 0)
				return 0;
			tmp_i = strlen(gArgv[0]);
			mqttConnInfoTemp = (MQTT_CONNECT_INFO *)&sysCfg->mqttConnInfoTemp[channel_count];
				
			if(mqttConnInfoTemp->password != NULL)
				SENS_MEM_FREE(mqttConnInfoTemp->password);
			mqttConnInfoTemp->password = (char *)SENS_MEM_ZALLOC(tmp_i+1);
			strcpy(mqttConnInfoTemp->password, gArgv[0]);
		}
	}
	else if(memcmp(name, MQTT_PUB_TOPIC_str, strlen(MQTT_PUB_TOPIC_str)) == 0)
	{	MQTT_CONNECT_INFO *mqttConnInfoTemp;
		MQTT_TOPIC_CONFIG *topic;
		MQTT_TOPIC_CONFIG *topicTemp;
		SENS_SSCANF((char *)name, MQTT_PUB_TOPIC_str"%d", &channel_count);
		if(channel_count < UPLOAD_SERVER_CNT)
		{	if(cmdLineToArgcArgv1(cmd, value) == 0)
				return 0;
			tmp_i = strlen(gArgv[0]);
			mqttConnInfoTemp = (MQTT_CONNECT_INFO *)&sysCfg->mqttConnInfoTemp[channel_count];
			topic = SENS_MEM_ZALLOC(sizeof(MQTT_TOPIC_CONFIG));
			topic->topic = (char *)SENS_MEM_ZALLOC(tmp_i+1);
			strcpy(topic->topic, gArgv[0]);
				
			if(mqttConnInfoTemp->pubTopics == NULL)
				mqttConnInfoTemp->pubTopics = topic;
			else 
			{	topicTemp = mqttConnInfoTemp->pubTopics;
				while(topicTemp->next)
				{	topicTemp = topicTemp->next;
				}
				topicTemp->next = topic;
			}
		}
	}
	else if(memcmp(name, MQTT_SUB_TOPIC_str, strlen(MQTT_SUB_TOPIC_str)) == 0)
	{	MQTT_CONNECT_INFO *mqttConnInfoTemp;
		MQTT_TOPIC_CONFIG *topic;
		MQTT_TOPIC_CONFIG *topicTemp;
		SENS_SSCANF((char *)name, MQTT_SUB_TOPIC_str"%d", &channel_count);
		if(channel_count < UPLOAD_SERVER_CNT)
		{	if(cmdLineToArgcArgv1(cmd, value) == 0)
				return 0;
			tmp_i = strlen(gArgv[0]);
			mqttConnInfoTemp = (MQTT_CONNECT_INFO *)&sysCfg->mqttConnInfoTemp[channel_count];
			topic = SENS_MEM_ZALLOC(sizeof(MQTT_TOPIC_CONFIG));
			topic->topic = (char *)SENS_MEM_ZALLOC(tmp_i+1);
			strcpy(topic->topic, gArgv[0]);
			
			if(mqttConnInfoTemp->subTopics == NULL)
				mqttConnInfoTemp->subTopics = topic;
			else 
			{	topicTemp = mqttConnInfoTemp->subTopics;
				while(topicTemp->next)
				{	topicTemp = topicTemp->next;
				}
				topicTemp->next = topic;
			}
		}
	}
	else if (NAME_MATCH(SENSLINK_APN_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG	*)&netCfg->wirelessCfg;
		MOBILE_CONFIG *lteCfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		tmp_i = strlen(gArgv[0]);
		if (tmp_i >= IP_MAX_COMMAND_LEN) return 0;
		gArgv[0][tmp_i] = '\0';
		mdvpnPosStart = strstr(gArgv[0], "[");
		if(mdvpnPosStart)
		{	mdvpnPosStart++;
			mdvpnPosEnd = strstr(mdvpnPosStart, "]");
			if(mdvpnPosEnd)
			{	*mdvpnPosEnd = '\0';
				if(!strcmp(mdvpnPosStart, "MDVPN"))
					wirelessCfg->mdvpnType = LTE_MDVPN_TYPE_SENSLINK_MDVPN;
				else if(!strcmp(mdvpnPosStart, "mdvpn"))
					wirelessCfg->mdvpnType = LTE_MDVPN_TYPE_OTHER_MDVPN;
				mdvpnPosStart = mdvpnPosEnd + 1;
			}
			else
				mdvpnPosStart = gArgv[0];
		}
		else
			mdvpnPosStart = gArgv[0];
		if(wirelessCfg->lteCfg == NULL)
			wirelessCfg->lteCfg = SENS_MEM_ZALLOC(sizeof(MOBILE_CONFIG));
		lteCfg = wirelessCfg->lteCfg;
		if(lteCfg->apn)
			SENS_MEM_FREE(lteCfg->apn);
		lteCfg->apn = SENS_MEM_ZALLOC(strlen(mdvpnPosStart) + 1);
		strcpy(lteCfg->apn, mdvpnPosStart);
	}
	else if (NAME_MATCH(SIM_AUTH_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG	*)&netCfg->wirelessCfg;
		MOBILE_CONFIG *lteCfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		if(wirelessCfg->lteCfg == NULL)
			wirelessCfg->lteCfg = SENS_MEM_ZALLOC(sizeof(MOBILE_CONFIG));
		lteCfg = wirelessCfg->lteCfg;
		lteCfg->simAuth = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(SIM_ACCOUNT_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG	*)&netCfg->wirelessCfg;
		MOBILE_CONFIG *lteCfg;
		
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		tmp_i = strlen(gArgv[0]);
		if (tmp_i >= IP_MAX_COMMAND_LEN) return 0;
		if(wirelessCfg->lteCfg == NULL)
			wirelessCfg->lteCfg = SENS_MEM_ZALLOC(sizeof(MOBILE_CONFIG));
		
		lteCfg = wirelessCfg->lteCfg;
		if(lteCfg->simAccount)
			SENS_MEM_FREE(lteCfg->simAccount);
		lteCfg->simAccount = SENS_MEM_ZALLOC(strlen(gArgv[0]) + 1);
		strcpy(lteCfg->simAccount, gArgv[0]);
	}
	else if (NAME_MATCH(SIM_PASSWORD_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG	*)&netCfg->wirelessCfg;
		MOBILE_CONFIG *lteCfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		if(strlen(gArgv[0]) >= IP_MAX_COMMAND_LEN) return 0;
		if(wirelessCfg->lteCfg == NULL)
			wirelessCfg->lteCfg = SENS_MEM_ZALLOC(sizeof(MOBILE_CONFIG));
		lteCfg = wirelessCfg->lteCfg;
		if(lteCfg->simPassword)
			SENS_MEM_FREE(lteCfg->simPassword);
		lteCfg->simPassword = SENS_MEM_ZALLOC(strlen(gArgv[0]) + 1);
		strcpy(lteCfg->simPassword, gArgv[0]);
	}
	else if (NAME_MATCH(SENS_APN_NB_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG	*)&netCfg->wirelessCfg;
		MOBILE_CONFIG *nbCfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		tmp_i = strlen(gArgv[0]);
		if(tmp_i >= IP_MAX_COMMAND_LEN) return 0;
		if(wirelessCfg->nbCfg == NULL)
			wirelessCfg->nbCfg = SENS_MEM_ZALLOC(sizeof(MOBILE_CONFIG));
		nbCfg = wirelessCfg->nbCfg;
		if(nbCfg->apn)
			SENS_MEM_FREE(nbCfg->apn);
		nbCfg->apn = SENS_MEM_ZALLOC(strlen(gArgv[0]) + 1);
		memcpy(nbCfg->apn, gArgv[0], tmp_i);
	}
	else if (NAME_MATCH(SENS_PLMN_NB_str))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG	*)&netCfg->wirelessCfg;
		MOBILE_CONFIG *nbCfg;
		
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		tmp_i = strlen(gArgv[0]);
		if (tmp_i > 6) return 0;
		if(wirelessCfg->nbCfg == NULL)
			wirelessCfg->nbCfg = SENS_MEM_ZALLOC(sizeof(MOBILE_CONFIG));
		nbCfg = wirelessCfg->nbCfg;
		nbCfg->plmn = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(DI_STATUS_str))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->diCfg.diStatusRecord = _io_atoi(gArgv[0]);
	}
	else if (NAME_MATCH(DO_OUTPUT_POWER_str))
	{	argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		
		sysCfg->doCfg.powerCtrl[0] = _io_atoi(gArgv[0]);
		sysCfg->doCfg.powerCtrl[1] = _io_atoi(gArgv[1]);
		//sysCfg->doCfg.powerCtrl[2] = _io_atoi(gArgv[2]);
		sysCfg->doCfg.DO24VPwr = _io_atoi(gArgv[2]);
		sysCfg->doCfg.mode = _io_atoi(gArgv[3]);
	}
	else if (NAME_MATCH(ALERT_THRE_str))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->sensSysCfg.alertEnable = _io_atoi(gArgv[0]);
		sysCfg->sensSysCfg.alertValue = (float)_io_strtod(gArgv[1], NULL);
		sysCfg->sensSysCfg.alertRecoveryMargin = sysCfg->sensSysCfg.alertValue * 0.1;
	}
	else if (NAME_MATCH(SD_HISTORY_str))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->sensSysCfg.logToSd = _io_atoi(gArgv[0]);
	}
	else if(NAME_MATCH(AR77_PARAM_STR))
	{	AR77_RADAR_CONFIG *ar77Cfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->ar77Cfg = SENS_MEM_ZALLOC(sizeof(AR77_RADAR_CONFIG));
		ar77Cfg = sysCfg->ar77Cfg;
		ar77Cfg->mode = _io_atoi(gArgv[0]);
		ar77Cfg->maxDistance = _io_atoi(gArgv[1]);
		ar77Cfg->minDistance = _io_atoi(gArgv[2]);
		ar77Cfg->xRange = _io_atoi(gArgv[3]);
		ar77Cfg->autoResolution = _io_atoi(gArgv[4]);
		ar77Cfg->offset = _io_atoi(gArgv[5]);
		//sensSys->isRadarWithWaterDet = _io_atoi(gArgv[6]);     
		//ar77Cfg->isValid = 1;
	}
	else if(NAME_MATCH(AVDS_PARAMS_STR))
	{	AVDS_RADAR_CONFIG	*avdsCfg;
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
		sysCfg->avdsCfg = SENS_MEM_ZALLOC(sizeof(AVDS_RADAR_CONFIG));
		avdsCfg = sysCfg->avdsCfg;
		avdsCfg->mode = _io_atoi(gArgv[0]);
		avdsCfg->verticalStartAzimuth = _io_atoi(gArgv[1]);
		avdsCfg->verticalEndAzimuth = _io_atoi(gArgv[2]);
		avdsCfg->verticalStartRange = _io_atoi(gArgv[3]);
		avdsCfg->verticalEndRange = _io_atoi(gArgv[4]);
		avdsCfg->distOf2Radar = _io_atoi(gArgv[5]);
		avdsCfg->tiltAngle = _io_atoi(gArgv[6]);
		avdsCfg->tiltBinRangeL = _io_atoi(gArgv[7]);
		avdsCfg->waterLevel = _io_atoi(gArgv[8]);
		avdsCfg->installHeight = _io_atoi(gArgv[9]);
		avdsCfg->tiltBinRangeH = avdsCfg->tiltBinRangeL;
		avdsCfg->tiltAngleCalibration = avdsCfg->tiltAngle;
		avdsCfg->tiltAngleCalibration = _io_atoi(gArgv[10]);
		//avdsCfg->isValid = 1;
	}
	else if(NAME_MATCH(OSA24_PARAMS_STR))
	{	OSA24G_CONFIG *osa24gCfg;
			
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) return 0;
				
		osa24gCfg = SENS_MEM_ZALLOC(sizeof(OSA24G_CONFIG));
		osa24gCfg->devNo = _io_atoi(gArgv[0]);
		osa24gCfg->slaveId = _io_atoi(gArgv[1]);
		osa24gCfg->alertMode = _io_atoi(gArgv[2]);
		osa24gCfg->alertThreshold = (float)_io_strtod((char*)gArgv[3], NULL);
		osa24gCfg->wlSensor = _io_atoi(gArgv[4]);
		osa24gCfg->maxDistance = _io_atoi(gArgv[5]);
		osa24gCfg->minDistance = _io_atoi(gArgv[6]);
		osa24gCfg->pwrSrc = _io_atoi(gArgv[7]);
		if(argcNum > 8)
			osa24gCfg->pollingInterval = _io_atoi(gArgv[8]);
		if(osa24gCfg->devNo)
		{	if(osa24gCfg->wlSensor)
			{	osa24gCfg->wlsDiNo = (osa24gCfg->wlSensor & 0x7F) - 1;
				if(osa24gCfg->wlSensor & 0x80)
					osa24gCfg->wlsDoNo = 1;
				else
					osa24gCfg->wlsDoNo = 0;
			}
			sysCfg->osa24gCfg = osa24gCfg;
		}
		else
			SENS_MEM_FREE(osa24gCfg);
	}
	else if(NAME_MATCH(COMPOSITE_OSA_PARAM_STR))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		COMPOSITE_OSA_CONFIG	*compositeOsaCfg;
		compositeOsaCfg = SENS_MEM_ZALLOC(sizeof(COMPOSITE_OSA_CONFIG));
		compositeOsaCfg->radarDevNo = _io_atoi(gArgv[0]);
		compositeOsaCfg->radarPwrSrc = _io_atoi(gArgv[1]);
		compositeOsaCfg->wlsDevNo = _io_atoi(gArgv[2]);
		compositeOsaCfg->wlsPwrSrc = _io_atoi(gArgv[3]);
		compositeOsaCfg->alertTrigVal = (IKW_FP)_io_strtod((char*)gArgv[4], NULL);
		compositeOsaCfg->alertRecoveryVal = (IKW_FP)_io_strtod((char*)gArgv[5], NULL);
		compositeOsaCfg->distance = (IKW_FP)_io_strtod((char*)gArgv[6], NULL);
		compositeOsaCfg->installHeight = (IKW_FP)_io_strtod((char*)gArgv[7], NULL);
		compositeOsaCfg->altitude = (IKW_FP)_io_strtod((char*)gArgv[8], NULL);
		if((compositeOsaCfg->radarDevNo == 0) && (compositeOsaCfg->wlsDevNo == 0))
			SENS_MEM_FREE(compositeOsaCfg);
		else
			sysCfg->compositeOsaCfg = compositeOsaCfg;
	}
	else if(NAME_MATCH(COMPOSITE_SIEMENS_PARAM_STR))
	{	COMPOSITE_SIEMENS_CONFIG	*compositeSiemensParmas;
		//COMPOSITE_SENSOR_CTRL_STRUCT *compositeSensorCtrl;
			
		argcNum = cmdLineToArgcArgv1(cmd, value);
		if(argcNum == 0) 
			return 0;
		compositeSiemensParmas = SENS_MEM_ZALLOC(sizeof(COMPOSITE_SIEMENS_CONFIG));
		compositeSiemensParmas->radarDevNo = _io_atoi(gArgv[0]);
		compositeSiemensParmas->radarPwrSrc = _io_atoi(gArgv[1]);
		compositeSiemensParmas->wlsDevNo = _io_atoi(gArgv[2]);
		compositeSiemensParmas->wlsPwrSrc = _io_atoi(gArgv[3]);
		compositeSiemensParmas->alertTrigVal = (IKW_FP)_io_strtod((char*)gArgv[4], NULL);
		compositeSiemensParmas->alertRecoveryVal = (IKW_FP)_io_strtod((char*)gArgv[5], NULL);
		compositeSiemensParmas->distance = (IKW_FP)_io_strtod((char*)gArgv[6], NULL);
		compositeSiemensParmas->installHeight = (IKW_FP)_io_strtod((char*)gArgv[7], NULL);
		compositeSiemensParmas->altitude = (IKW_FP)_io_strtod((char*)gArgv[8], NULL);
		if(argcNum > 9)
			compositeSiemensParmas->radarBlindArea = (IKW_FP)_io_strtod((char*)gArgv[9], NULL);
		
		if((compositeSiemensParmas->radarDevNo != -1) || (compositeSiemensParmas->wlsDevNo != -1))
		{	sysCfg->compositeSiemensCfg = compositeSiemensParmas;
			/*compositeSensorCtrl = SENS_MEM_ZALLOC(sizeof(COMPOSITE_SENSOR_CTRL_STRUCT));
			compositeSensorCtrl->isAiRadar = 1;
			getLinearEquationParam(compositeSiemensParmas->radarBlindArea, compositeSiemensParmas->installHeight, &compositeSensorCtrl->linearEquationX, &compositeSensorCtrl->linearEquationY);
			sensDev->compositeSensorCtrl = compositeSensorCtrl;*/
		}
		else
			SENS_MEM_FREE(compositeSiemensParmas);
	}
	else if(NAME_MATCH(OTA_PARAM_STR))
	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		OTA_CONFIG *otaCfg;
		
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		tmp_i = strlen(gArgv[0]);
		if(netCfg->otaCfg)
		{	SENS_MEM_FREE(netCfg->otaCfg);
			netCfg->otaCfg = NULL;
		}
		if(tmp_i >= 1)
		{	netCfg->otaCfg = SENS_MEM_ZALLOC(sizeof(OTA_CONFIG));
			otaCfg = netCfg->otaCfg;
			otaCfg->domainname = SENS_MEM_ZALLOC(tmp_i + 1);
			strcpy(otaCfg->domainname, gArgv[0]);
			otaCfg->port = _io_atoi(gArgv[1]);
		}
	}
	else if(NAME_MATCH(AGPS_ENABLE_STR))
  	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG	*)&netCfg->wirelessCfg;
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		wirelessCfg->agpsEnable = _io_atoi(gArgv[0]);
  	}
  	else if(NAME_MATCH(AGPS_PARAMS_STR))
  	{	NET_CONFIG *netCfg = (NET_CONFIG *)&sysCfg->netCfg;
		WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG	*)&netCfg->wirelessCfg;
		LTE_AGPS_CONFIG *lteAgpsCfg;
		NB_AGPS_CONFIG *nbAgpsCfg;
		
		if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
    	lteAgpsCfg = SENS_MEM_ZALLOC(sizeof(LTE_AGPS_CONFIG));
		nbAgpsCfg = SENS_MEM_ZALLOC(sizeof(NB_AGPS_CONFIG));
		nbAgpsCfg->me310AgpsType = _io_atoi(gArgv[0]);
		nbAgpsCfg->me310ColdStart = _io_atoi(gArgv[1]);
		nbAgpsCfg->me310HotStart = _io_atoi(gArgv[2]);
		nbAgpsCfg->me310PollInterval = _io_atoi(gArgv[3]);
		
		lteAgpsCfg->le910AgpsType = _io_atoi(gArgv[4]);
		
		tmp_i = strlen(gArgv[5]);
		if(tmp_i > 64)	
		{	lteAgpsCfg->suplServer = SENS_MEM_ZALLOC(strlen("supl.google.com") + 1);
			strcpy((char *)lteAgpsCfg->suplServer, "supl.google.com");
		}
		else			
		{	lteAgpsCfg->suplServer = SENS_MEM_ZALLOC(strlen(gArgv[5]) + 1);
			strcpy((char *)lteAgpsCfg->suplServer, gArgv[5]);
		}
		lteAgpsCfg->suplPort = _io_atoi(gArgv[6]);
		
		wirelessCfg->lteAgpsCfg = lteAgpsCfg;
		wirelessCfg->nbAgpsCfg = nbAgpsCfg;
  	}
	else if(NAME_MATCH(GATE_CTRL_STR))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->specFeas.isGateCtrlMode = _io_atoi(gArgv[0]);;
	}
	else if(NAME_MATCH(PUMP_MODE_STR))
	{	if(cmdLineToArgcArgv1(cmd, value) == 0) return 0;
		sysCfg->specFeas.isYlPumpMode = _io_atoi(gArgv[0]);
	}
	
	
	return 0;
}
//------------------------------------------------------------------------------
static int iniParseNew(void)
{	MiniFile *iniFileParsePtr;
	Section	*iniSection;
	SectionData *sectionData;
	int findA4Section = 0;
	
	iniFileParsePtr = mini_parse_file(INIFILE);
	if(iniFileParsePtr == NULL)
	{	return -1;
	}
	
	for(iniSection = iniFileParsePtr->section;iniSection;iniSection = iniSection->next)
	{	if(!strcmp(iniSection->name, SECTION))
		{	findA4Section = 1;
			for(sectionData = iniSection->data;sectionData;sectionData = sectionData->next)
			{	handler1(NULL, iniSection->name, sectionData->key, sectionData->value);
			}
		}
	}
	mini_file_free(iniFileParsePtr);
	return findA4Section? 0:-1;
}
//------------------------------------------------------------------------------
static void connectInfoBind(void)
{	CLOUD_SERVER_INFO *serverInfo;
	int idx;
	
	for(idx=0;idx<UPLOAD_SERVER_CNT;idx++)
	{	serverInfo = getServerInfoById(idx);
		if(serverInfo)
		{	if(strlen(serverInfo->serverDomainName) > 1)
			{	sysCtrl->srvActiveBmp |= (1 << idx);
				if((serverInfo->serverProtocol >= MQTT_SENSTALK_BROKER) && (serverInfo->serverProtocol < MQTT_PLATFORM_MAX))
				{	serverInfo->servConnectInfo = (void *)&sysCfg->mqttConnInfoTemp[idx];
				}
#if SUPPORT_IOA_WEB_API
				else if((serverInfo->serverProtocol >= IOA_WEB_API_PROTOCOL) && (serverInfo->serverProtocol < IOA_PLATFORM_MAX))
				{	MQTT_CONNECT_INFO *mqttConnInfo = (MQTT_CONNECT_INFO *)&sysCfg->mqttConnInfoTemp[idx];
					serverInfo->servConnectInfo = (void *)mqttConnInfo;
				}
#endif
			}
		}
	}
}
//------------------------------------------------------------------------------
static void setDiWakeupFunction(void)
{	int idx;
	int startIdx = 0;

#if BOARD_VERSION == 1
	startIdx = 2;
#endif	

	for(idx=startIdx;idx<4;idx++)
	{	if(sysCfg->diCfg.diWakeup[idx])
		{	
#if BOARD_VERSION == 2
			if(idx == 0)
				sysCtrlFlag.extGpioIn4WakeupEn = 1;
			else if(idx == 1)
				sysCtrlFlag.extGpioIn5WakeupEn = 1;
			else 
#endif
				if(idx == 2)
				sysCtrlFlag.extGpioIn0WakeupEn = 1;
			else if(idx == 3)
				sysCtrlFlag.extGpioIn1WakeupEn = 1;
		}
		else
		{
#if BOARD_VERSION == 2
			if(idx == 0)
				sysCtrlFlag.extGpioIn4WakeupEn = 0;
			else if(idx == 1)
				sysCtrlFlag.extGpioIn5WakeupEn = 0;
			else 
#endif
				if(idx == 2)
				sysCtrlFlag.extGpioIn0WakeupEn = 0;
			else if(idx == 3)
				sysCtrlFlag.extGpioIn1WakeupEn = 0;
		}
	}
	setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_CTRL_FLAG, sysCtrlFlag.sysCtrlFlags);
}
//------------------------------------------------------------------------------
int getSysParameter(void)
{	/*if(sensSys->sdInst.sdPresence)
	{
	}*/
	MiniFile *miniFileCtx;
	
	if(!isFileExist(INIFILE) && !isFileExist(INI_BAK_FILE))
	{	dbgMsg("%s", "[SD CARD] no parameter file detect!\r\n");
#if SPI_FILE_SYSTEM
		chkParamInSpi();
#else
		sysCtrl->sdCardError = 1;
#endif
	}
	else if(isFileExist(INIFILE) || isFileExist(INI_BAK_FILE))
	{	dbg_msg("%s", "[SD CARD] SD ini file exist, check params\r\n");
#if SPI_FILE_SYSTEM
		sysCtrl->paramInSd = 1;
#endif
		miniFileCtx = mini_parse_file(INIFILE);
		if(miniFileCtx)
		{	mini_file_free(miniFileCtx);
#if SPI_FILE_SYSTEM
			sysCtrl->paramInSd = 1;
			if(sysCtrl->bakupFlashIsPresent)
			{	if(SENS_SPI_FS_FILE_EXIST(INIFILE) != SPI_FILE_IS_EXIST)
				{	if(backupParamFile(FROM_SD_CARD, FROM_SPI_NOR, 1, 0, FROM_SD_CARD) == 0)
					{
					}
				}
			}
#endif
		}
		else
		{	
#if SPI_FILE_SYSTEM
			sysCtrl->paramInSd = 0;
			chkParamInSpi();
			if(sysCtrl->paramInSd)
			{
			}
#else
			sysCtrl->sdCardError = 1;
#endif
		}
	}
	
	if(sysCtrl->sdCardError)
		return -1;
	
	setExtInterfaceCfg();
	
	iniParseNew();
#if REC_INTERVAL_FIXED
	sysTimer->recTimeSlotInterval = sysCfg->sensSysCfg.recordSec/60;
#endif
	if(sysCfg->sensSysCfg.psm == 0)
		setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_PSM_DISABLE);
	else
		setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_PSM_DISABLE);
	
	sysStsFlag1.psmMode = sysCfg->sensSysCfg.psm;
	setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1, sysStsFlag1.sysStatusFlags);
	
	if(sysCfg->sensSysCfg.recordSec < 180)
		setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_REC_TIME_LESS_3MIN);
	else
		setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_REC_TIME_LESS_3MIN);
	
	setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_RECORD_DATA);
	
	sysCtrl->sysIsInstallMode = sysCfg->sensSysCfg.sysIsInstallMode;
	sysTimer->alertTimeSlotInverval = 1;	//1 min slot record in alert mode
	if(sysCfg->diCfg.diWakeupRecInterval)
		sysTimer->diWakeupRecSlot = sysCfg->diCfg.diWakeupRecInterval / 60;	//sysCfg->diCfg.diWakeupRecInterval is sec
	else
		sysTimer->diWakeupRecSlot = 1;	//default
	sysCfg->sensSysCfg.cameraRecInterval = sysCfg->sensSysCfg.cameraRecInterval * sysTimer->recTimeSlotInterval;
		
	
	sensPq->pqNumber = getTotalPq();
	//setSystemCfg();
	//setNetworkCfg();
	addYsMqttCloudServerCfg();
	//setUserId();
	getUserId();
	connectInfoBind();
	specExtDevBind();
	
	setDiWakeupFunction();
	
	return 0;
}
//------------------------------------------------------------------------------
int UpdateAndSaveParam(char *param)
{	char *p;
	char sep[2];
	int parameter_num = 0, PDS_sel = 0;
	uint32_t ip_addr;
#ifdef PLATFORM_XILINX
	EEPROM_DATA *eepromCtx = (EEPROM_DATA *)sensDev->buffer_r;
#endif
#ifdef SENSMINIA4_NEO
	EEPROM_CONTEXT *eepromCtx = (EEPROM_CONTEXT *)sensSys->eepromBuf;
#endif
	
#if defined PLATFORM_XILINX || defined SENSMINIA4_NEO
	char changeEepromData = 0;
#endif
	MiniFile *mini_file;
	//IKW_CMD_PARAM_STRUCT	*ikwCmdParam = NULL;

	mini_file = mini_parse_file(INIFILE);
	if(mini_file == NULL)
	{	dbg_msg("[WARNING] Updata: Can't parse '%s' file!\r\n", INIFILE);
		return 1;
	}

	strcpy(sep, "|");                                                             // separator between parameters
	strtokLock();
	p = strtok(param, sep);
	parameter_num = _io_atoi(p);

	if(parameter_num == PARAM_SETTING_BASIC_PARAM_VAL_1)			// basic parameter
	{	char *rs485_1_param, *gprs_lan_on, *ip_address;
		rs485_1_param = SENS_MEM_ZALLOC(128);
		gprs_lan_on = SENS_MEM_ZALLOC(5);
		ip_address = SENS_MEM_ZALLOC(20);
  	
		p = strtok(NULL, sep);
		if(p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SD_REC_SEC_str, p);
		}
		p = strtok(NULL, sep);
		if(p)		mini_file_update_value_for_key(mini_file, SECTION, AUTOSEND_SEC_str, p);
		
		p = strtok(NULL, sep);
		if (p)
		{	ip_addr = _io_atoi(p);
			SENS_SPRINTF(ip_address, "%d.%d.%d.%d", (ip_addr>>24)&0xFF, (ip_addr>>16)&0xFF, (ip_addr>>8)&0xFF, ip_addr&0xFF);
			mini_file_update_value_for_key(mini_file, SECTION, IP_ADDRESS_str, ip_address);
		}
#if defined PLATFORM_XILINX || defined SENSMINIA4_NEO
		if((eepromCtx->ipAddr[0] != ((ip_addr>>24)&0xFF)) || (eepromCtx->ipAddr[1] != ((ip_addr>>16)&0xFF)) || (eepromCtx->ipAddr[2] != ((ip_addr>>8)&0xFF)) || (eepromCtx->ipAddr[3] != (ip_addr&0xFF)))
		{	changeEepromData = 1;
			eepromCtx->ipAddr[0] = (ip_addr>>24)&0xFF;
			eepromCtx->ipAddr[1] = (ip_addr>>16)&0xFF;
			eepromCtx->ipAddr[2] = (ip_addr>>8)&0xFF;
			eepromCtx->ipAddr[3] = ip_addr&0xFF;
		}
#endif
		memset(ip_address,  0, sizeof(char)*20);
		p = strtok(NULL, sep);
		if (p)
		{	ip_addr = _io_atoi(p);
			SENS_SPRINTF(ip_address, "%d.%d.%d.%d", (ip_addr>>24)&0xFF, (ip_addr>>16)&0xFF, (ip_addr>>8)&0xFF, ip_addr&0xFF);
      //dbg_msg("ip_address:%s, ip_addr:%x\r\n", ip_address, ip_addr);
			mini_file_update_value_for_key(mini_file, SECTION, IP_MASK_str, ip_address);
		}
#if defined PLATFORM_XILINX || defined SENSMINIA4_NEO
		if((eepromCtx->ipMask[0] != ((ip_addr>>24)&0xFF)) || (eepromCtx->ipMask[1] != ((ip_addr>>16)&0xFF)) || (eepromCtx->ipMask[2] != ((ip_addr>>8)&0xFF)) || (eepromCtx->ipMask[3] != (ip_addr&0xFF)))
		{	changeEepromData = 1;
			eepromCtx->ipMask[0] = (ip_addr>>24)&0xFF;
			eepromCtx->ipMask[1] = (ip_addr>>16)&0xFF;
			eepromCtx->ipMask[2] = (ip_addr>>8)&0xFF;
			eepromCtx->ipMask[3] = ip_addr&0xFF;
		}
#endif

		memset(ip_address,  0, sizeof(char)*20);

		p = strtok(NULL, sep);
		if (p)
		{	ip_addr = _io_atoi(p);
			SENS_SPRINTF(ip_address, "%d.%d.%d.%d", (ip_addr>>24)&0xFF, (ip_addr>>16)&0xFF, (ip_addr>>8)&0xFF, ip_addr&0xFF);
			mini_file_update_value_for_key(mini_file, SECTION, IP_GATEWAY_str, ip_address);
		}
#if defined PLATFORM_XILINX || defined SENSMINIA4_NEO
		if((eepromCtx->ipRouter[0] != ((ip_addr>>24)&0xFF)) || (eepromCtx->ipRouter[1] != ((ip_addr>>16)&0xFF)) || (eepromCtx->ipRouter[2] != ((ip_addr>>8)&0xFF)) || (eepromCtx->ipRouter[3] != (ip_addr&0xFF)))
		{	changeEepromData = 1;
			eepromCtx->ipRouter[0] = (ip_addr>>24)&0xFF;
			eepromCtx->ipRouter[1] = (ip_addr>>16)&0xFF;
			eepromCtx->ipRouter[2] = (ip_addr>>8)&0xFF;
			eepromCtx->ipRouter[3] = ip_addr&0xFF;
		}
#endif
		memset(ip_address,  0, sizeof(char)*20);

		/*p = strtok(NULL, sep);	//yushun remove
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, DEVICE_DISCONNECT_SEC_str, p);
		}*/

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, PIC_POWERDOWN_str, p);
		}

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, WAKE_UP_INTERVAL_str, p);
		}

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SENSLINK_APN_str, p);
		}
		//SET_VBAT_REG_LTE_APN_SET(1);
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SENSLINK_ADDRESS_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SENSLINK_PORT_str, p);
		}

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SENSLINK_ADDRESS2_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SENSLINK_PORT2_str, p);
		}

		//mini_file_update_value_for_key(mini_file, SECTION, ID_Client_str, "1 1");	//yushun remove

		memset(ip_address,  0, sizeof(char)*20);
		p = strtok(NULL, sep);
		if (p)
		{	
#ifdef PLATFORM_XILINX
			SENS_SSCANF(p, "%d.%d.%d.%d", &ip_address[0], &ip_address[1], &ip_address[2], &ip_address[3]);

			if((eepromData->ipDns[0] != ip_address[0]) || (eepromData->ipDns[1] != ip_address[1]) || (eepromData->ipDns[2] != ip_address[2]) || (eepromData->ipDns[3] != ip_address[3]))
			{	changeEepromData = 1;
				eepromData->ipDns[0] = ip_address[0];
				eepromData->ipDns[1] = ip_address[1];
				eepromData->ipDns[2] = ip_address[2];
				eepromData->ipDns[3] = ip_address[3];
			}
#endif
			mini_file_update_value_for_key(mini_file, SECTION, DNS_ADDRESS_str, p);
		}

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, BAT_OFF_str, p);
		}

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, BAT_ON_DIFF_str, p);
		}

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, PRIORITY_CONNECT_str, p);
		}

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, PRESSURE_WATER_LEVEL_GAUGE_STR, p); 
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, GATE_CTRL_STR, p); 
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, PUMP_MODE_STR, p); 
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SYSTEM_INSTALL_MODE_STR, p); 
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, AUTO_FOTA_CHK_MODE_STR, p); 
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, IMG_SERV_TYPE_STR, p); 
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MOBILE_INTERVAL_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, TRANSFER_PROTOCOL_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, TRANSFER_PROTOCOL1_str, p);
		}

		/*p = strtok(NULL, sep);	//yushun remove
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SLEEP_REC_TIME_str, p);
		}*/

		/*p = strtok(NULL, sep);	//yushun remove
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MOBILE_BR_str, p);
		}*/

		memset(ip_address,  0, sizeof(char)*20);
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(ip_address, "%s", p);
		}

		p = strtok(NULL, sep);
		if (p)
		{	if(ip_address[0] == '0')
			{	SENS_SPRINTF(ip_address, "%s %s", ip_address, "0");
				mini_file_update_value_for_key(mini_file, SECTION, ALERT_THRE_str, ip_address);
			}
			else
			{	SENS_SPRINTF(ip_address, "%s %s", ip_address, p);
				mini_file_update_value_for_key(mini_file, SECTION, ALERT_THRE_str, ip_address);
			}
		}
		else
		{	SENS_SPRINTF(ip_address, "%s %s", ip_address, "0");
			mini_file_update_value_for_key(mini_file, SECTION, ALERT_THRE_str, ip_address);
		}

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SENS_APN_NB_str, p);
		}
		//SET_VBAT_REG_NB_APN_SET(1);
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SENS_PLMN_NB_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SIM_AUTH_str, p);
		}
		p = strtok(NULL, sep);
		
		if(p)
		{	char *base64DecodeValue;
			base64DecodeValue = (char *)base64Decode(p);
			mini_file_update_value_for_key(mini_file, SECTION, SIM_ACCOUNT_str, base64DecodeValue);
			SENS_MEM_FREE(base64DecodeValue);
		}
		p = strtok(NULL, sep);
		if (p)
		{	char *base64DecodeValue;
			base64DecodeValue = (char *)base64Decode(p);
			mini_file_update_value_for_key(mini_file, SECTION, SIM_PASSWORD_str, base64DecodeValue);
			SENS_MEM_FREE(base64DecodeValue);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SD_HISTORY_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, NUM_WISUN_NODE_STR, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, PQ_NODE_STR, p);
		}
		/*
		*	LORA WAN
		*/
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, LORA_WAN_CH0_FREQ_STR, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, LORA_WAN_DEV_EUI_STR, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, LORA_WAN_APP_EUI_STR, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, LORA_WAN_APP_KEY_STR, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, LORA_WAN_DEV_ADDR_STR, p);
		}
		p = strtok(NULL, sep);
		if(p)
		{	mini_file_update_value_for_key(mini_file, SECTION, AGPS_ENABLE_STR, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SERV1_TYPE_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MQTT1_DEVICE_NAME_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MQTT1_USERNAME_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	char *base64DecodeValue;
			base64DecodeValue = (char *)base64Decode(p);
			mini_file_update_value_for_key(mini_file, SECTION, MQTT1_PASSWORD_str, base64DecodeValue);
			//mini_file_update_value_for_key(mini_file, SECTION, MQTT1_PASSWORD_str, p);
			SENS_MEM_FREE(base64DecodeValue);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MQTT1_PUB_TOPIC_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MQTT1_SUB_TOPIC_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, SERV2_TYPE_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MQTT2_DEVICE_NAME_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MQTT2_USERNAME_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	char *base64DecodeValue;
			base64DecodeValue = (char *)base64Decode(p);
			mini_file_update_value_for_key(mini_file, SECTION, MQTT2_PASSWORD_str, base64DecodeValue);
			//mini_file_update_value_for_key(mini_file, SECTION, MQTT2_PASSWORD_str, p);
			SENS_MEM_FREE(base64DecodeValue);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MQTT2_PUB_TOPIC_str, p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, MQTT2_SUB_TOPIC_str, p);
		}
		//agps params
		memset(rs485_1_param, 0, 128);
		
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(rs485_1_param, "%s ", p);	//nb Agps system
		}
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(rs485_1_param+strlen(rs485_1_param), "%s ", p);	//cold start
		}
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(rs485_1_param+strlen(rs485_1_param), "%s ", p);	//hot start
		}
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(rs485_1_param+strlen(rs485_1_param), "%s ", p);	//agps interval
		}
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(rs485_1_param+strlen(rs485_1_param), "%s ", p);	//lte agps sys
		}
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(rs485_1_param+strlen(rs485_1_param), "%s ", p);	//suplServ
		}
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(rs485_1_param+strlen(rs485_1_param), "%s ", p);	//suplServPort
		}
		
		mini_file_update_value_for_key(mini_file, SECTION, AGPS_PARAMS_STR, rs485_1_param); 
		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, WISUN_NET_NAME_STR, p);
		}

		p = strtok(NULL, sep);
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, WISUN_FREQ_STR, p);
		}

#ifdef SUPPORT_ONBOARD_WISUN
		p = strtok(NULL, sep);
		if(p)	mini_file_update_value_for_key(mini_file, SECTION, WISUN_ONBOARD_EN_STR, p);
#endif
		memset(rs485_1_param, 0, 128);
		for(parameter_num = 0;parameter_num<OTA_PARAM_SIZE;parameter_num++)
		{	p = strtok(NULL, sep);
			if(p)
			{	strcat(rs485_1_param, p);
				strcat(rs485_1_param, " ");
			}
			else
				break;
		}
		mini_file_update_value_for_key(mini_file, SECTION, OTA_PARAM_STR, rs485_1_param);
		p = strtok(NULL, sep);
		if(p)	mini_file_update_value_for_key(mini_file, SECTION, DI_FUNCTION_str, p);	//CY PUMP MODE
		memset(rs485_1_param, 0, 128);
		for(parameter_num = 0;parameter_num<GPS_POS_RST_TIME_SIZE;parameter_num++)
		{	p = strtok(NULL, sep);
			if(p)
			{	strcat(rs485_1_param, p);
				strcat(rs485_1_param, " ");
			}
			else
				break;
		}
		mini_file_update_value_for_key(mini_file, SECTION, GPS_POS_RST_TIME_str, rs485_1_param);
#if SUPPORT_EXT_LOW_VOLT
		memset(rs485_1_param, 0, 128);
		for(parameter_num = 0;parameter_num<EXT_LVD_SIZE;parameter_num++)
		{	p = strtok(NULL, sep);
			if(p)
			{	strcat(rs485_1_param, p);
				strcat(rs485_1_param, " ");
			}
			else
				break;
		}
		mini_file_update_value_for_key(mini_file, SECTION, EXT_LVD_STR, rs485_1_param);
#endif
		SENS_MEM_FREE(rs485_1_param);
		SENS_MEM_FREE(gprs_lan_on);
		SENS_MEM_FREE(ip_address);
		/*if(ikwParam)
			SENS_MEM_FREE(ikwParam);*/
		dbg_msg ("%s", "Save general parameter settings\r\n");
	}
	else if(parameter_num == PARAM_SETTING_DEVICE_SETTING_VAL_2)	// device parameter
	{	char device_count, device_count_select;
		char enable, protocol, start, /*point, */type, sequence/*, receive*/;
		char *para_name, *first_total_name, *device_parameter, *second_total_name,  *device_channel_parm, *ar77Params;
		char *device_command;
		char extDevIdx;
		char extIfIdx;
		uint8_t	cameraParamSize;
		int extDevProtocol[EXT_IF_CHANNEL_MAX];

		para_name = SENS_MEM_ZALLOC(10);
		first_total_name = SENS_MEM_ZALLOC(16);
		device_parameter = SENS_MEM_ZALLOC(70);
		second_total_name = SENS_MEM_ZALLOC(16);
		device_channel_parm = SENS_MEM_ZALLOC(16);
		ar77Params = SENS_MEM_ZALLOC(70);
		
		p = strtok(NULL, sep);                                                        // device count
		if (p)
		{	device_count_select = _io_atoi(p);
		}
		p = strtok(NULL, sep);                                                        // COM1 Protocol
		if (p)
		{	extDevProtocol[EXT_IF_CHANNEL_RS485_1] = _io_atoi(p);
		}
		p = strtok(NULL, sep);                                                        // COM2 Protocol
		if (p)
		{	extDevProtocol[EXT_IF_CHANNEL_RS485_2] = _io_atoi(p);
		}
		p = strtok(NULL, sep);                                                        // COM3 Protocol
		if (p)
		{	extDevProtocol[EXT_IF_CHANNEL_RS232] = _io_atoi(p);
		}
		p = strtok(NULL, sep);                                                        // Ether Protocol
		if (p)
		{	extDevProtocol[EXT_IF_CHANNEL_ETH] = _io_atoi(p);
		}
		p = strtok(NULL, sep);                                                        // HS USB Protocol
		if (p)
		{	extDevProtocol[EXT_IF_CHANNEL_USB_HS] = _io_atoi(p);
		}
		SENS_SPRINTF(device_parameter, "%d %d %d %d %d",	extDevProtocol[EXT_IF_CHANNEL_RS485_1], 
															extDevProtocol[EXT_IF_CHANNEL_RS485_2], 
															extDevProtocol[EXT_IF_CHANNEL_RS232], 
															extDevProtocol[EXT_IF_CHANNEL_ETH], 
															extDevProtocol[EXT_IF_CHANNEL_USB_HS]);

		mini_file_update_value_for_key(mini_file, SECTION, COM_PROTOCOL_str, device_parameter);
		device_parameter[0] = '\0';

		p = strtok(NULL, sep);                                                        // device 0 enable
		if (p)
		{	//sensDev->DEMulti->device_enable[0] = _io_atoi(p);
			mini_file_update_value_for_key(mini_file, SECTION, "device_0", "1 0 0 0 0 0 0 0");
		}

		for(device_count = 1 + device_count_select*5; device_count <= 5 + device_count_select*5; device_count++)
		{	memset(device_parameter, 0, 70);
			memset(para_name, 0, 10);
			strcat(para_name, "device_");
			SENS_SPRINTF(first_total_name, "%s%d", para_name, device_count);                 // create prefix
			p = strtok(NULL, sep);
			if (p)
			{	enable = _io_atoi(p);                                                     // device 1 enable
			}

			p = strtok(NULL, sep);                                                      // device 1 Protocol
			if (p)
			{	extIfIdx = _io_atoi(p);
				if(extIfIdx && (extIfIdx <= EXT_IF_CHANNEL_MAX))	//web 0 is None
				{	//extIfIdx--;
					protocol = extDevProtocol[extIfIdx-1];
				}
				else
					protocol = EXT_IF_PROTO_NONE;
				dbg_msg("ext dev idx:%d, protocol:%d\r\n", extIfIdx, protocol);
			}

			if( protocol == EXT_IF_PROTO_DCON)
			{	p = strtok(NULL, sep);
				if(p)	device_command = p;
				if(device_command[0] == '*') device_command[0] = '#';
				if(device_command[0] == '/') device_command[0] = '%';
				if(device_command[0] == '^') device_command[0] = '~';
			}
			else
			{	p = strtok(NULL, sep);                                                    // device 1 command
				if(p)	device_command = p;
			}
			p = strtok(NULL, sep);                                                      // device 1 start byte
			if(p)
				start = _io_atoi(p);

			//p = strtok(NULL, sep);	//yushun remove			// device 1 number item (how many points you want?)
			//if(p)	
			//	point = _io_atoi(p);
			p = strtok(NULL, sep);                                                      // device 1 type (calculate byte)
			if(p)
				type = _io_atoi(p);

			p = strtok(NULL, sep);                                                      // device 1 sequence
			if( protocol == EXT_IF_PROTO_MODBUS)	// MODBUS
			{	if(p)
					sequence = _io_atoi(p);
			}
			else
			{	if(p)
					sequence = 0;
			}

			//p = strtok(NULL, sep);	//yushun remove			// device 1 receive data or not
			//if(p)
			//	receive = _io_atoi(p);
			if( enable == 1)
			{	SENS_SPRINTF(device_parameter, "1 %d %d %s %d %d %d", extIfIdx, protocol, device_command, start, type, sequence);
				mini_file_update_value_for_key(mini_file, SECTION, first_total_name, device_parameter);
				device_parameter[0] = '\0';
			}
			else if(enable == 0)
			{	mini_file_update_value_for_key(mini_file, SECTION, first_total_name, "0 0 0 0 0 0 0");
			}
		}
		
		uint8_t differential, aiTypeArrays[5];	//yushun remove, aiArrays[4];
		
		p = strtok(NULL, sep);
		if(p)	enable = _io_atoi(p);
			//mini_file_update_value_for_key(mini_file, SECTION, ADC_POWER_str, p);
		p = strtok(NULL, sep);                                                      // onboard ai volt diff/single
		if(p)	differential = _io_atoi(p);
		
		p = strtok(NULL, sep);
		if(p)	aiTypeArrays[0] = _io_atoi(p);
		else	aiTypeArrays[0] = 0;
		p = strtok(NULL, sep);
		if(p)	aiTypeArrays[1] = _io_atoi(p);
		else	aiTypeArrays[1] = 0;
		p = strtok(NULL, sep);
		if(p)	aiTypeArrays[2] = _io_atoi(p);
		else	aiTypeArrays[2] = 0;
		p = strtok(NULL, sep);
		if(p)	aiTypeArrays[3] = _io_atoi(p);
		else	aiTypeArrays[3] = 0;	
		aiTypeArrays[3] += AI_TYPE_500MV;
		p = strtok(NULL, sep);
		if(p)	aiTypeArrays[4] = _io_atoi(p);
		else	aiTypeArrays[4] = 0;	
		if(differential == 0)	//differential
			aiTypeArrays[4] = aiTypeArrays[3];
		else
			aiTypeArrays[4] += AI_TYPE_500MV;
		/*p = strtok(NULL, sep);                                                      // onboard ai 1 current/voltage
		if(p)	aiArrays[0] = _io_atoi(p);
		p = strtok(NULL, sep);                                                      // onboard ai 2 current/voltage
		if(p)	aiArrays[1] = _io_atoi(p);
		p = strtok(NULL, sep);                                                      // onboard ai 3 current/voltage
		if(p)	aiArrays[2] = _io_atoi(p);
		p = strtok(NULL, sep);                                                      // onboard ai 4 current/voltage
		if(p)	aiArrays[3] = _io_atoi(p);*/
		
		memset(device_parameter, 0, 70);
		SENS_SPRINTF(device_parameter, "%d %d %d %d %d %d %d", enable, differential, aiTypeArrays[0], aiTypeArrays[1], aiTypeArrays[2], aiTypeArrays[3], aiTypeArrays[4]);
		mini_file_update_value_for_key(mini_file, SECTION, ADC_CFG_str, device_parameter);
		
		//AR77 params
		memset(ar77Params, 0, 70);
		for(extDevIdx = 0;extDevIdx<(AR77_PARAM_SIZE-1);extDevIdx++)
		{	p = strtok(NULL, sep);
			if(p)
			{	strcat(ar77Params, p);
				strcat(ar77Params, " ");
			}
			else
				break;
		}
		p = strtok(NULL, sep);
		if (p)
		{	
#if TEST_N_REMOVE
			char newState = RADAR_WLS_DISABLE;
			//mini_file_update_value_for_key(mini_file, SECTION, RADAR_WATER_LEVEL_SENSOR_STR, p); 
			SENS_SPRINTF(ar77Params+strlen(ar77Params), "%s ", p);	//water detect sensor
			if(strstr(p, "1"))
				newState = RADAR_WLS_3_STAGE;
			else if(strstr(p, "2"))
				newState = RADAR_WLS_1_STAGE;
			if(sensSys->isRadarWithWaterDet != newState)
			{	SET_VBAT_REG_WL3S_STATE(0x3F);	//set to invalid
			}
#else
			SENS_SPRINTF(ar77Params+strlen(ar77Params), "%s ", p);	//water detect sensor
#endif
		}
		mini_file_update_value_for_key(mini_file, SECTION, AR77_PARAM_STR, ar77Params); 
    
		//AVDS RADAR
		memset(ar77Params, 0, 70);
		for(extDevIdx = 0;extDevIdx<AVDS_PARAMS_SIZE;extDevIdx++)
		{	p = strtok(NULL, sep);
			if(p)
			{	//SENS_SPRINTF(ar77Params, "%s ", p);	//mode
				strcat(ar77Params, p);
				strcat(ar77Params, " ");
			}
			else
				break;
		}
		mini_file_update_value_for_key(mini_file, SECTION, AVDS_PARAMS_STR, ar77Params); 
		//OSA RADAR
		memset(ar77Params, 0, 70);
		for(extDevIdx = 0;extDevIdx<OSA24_PARAMS_SIZE;extDevIdx++)
		{	p = strtok(NULL, sep);
			if(p)
			{	//SENS_SPRINTF(ar77Params, "%s ", p);	//dev no
				strcat(ar77Params, p);
				strcat(ar77Params, " ");
			}
			else
				break;
		}
		mini_file_update_value_for_key(mini_file, SECTION, OSA24_PARAMS_STR, ar77Params); 
		
		memset(ar77Params, 0, 70);
		for(extDevIdx = 0;extDevIdx<COMPOSITE_OSA_PARAM_SIZE;extDevIdx++)
		{	p = strtok(NULL, sep);
			if(p)
			{	//SENS_SPRINTF(ar77Params, "%s ", p);	//dev no
				strcat(ar77Params, p);
				strcat(ar77Params, " ");
			}
			else
				break;
		}
		mini_file_update_value_for_key(mini_file, SECTION, COMPOSITE_OSA_PARAM_STR, ar77Params); 
		
		memset(ar77Params, 0, 70);
		for(extDevIdx = 0;extDevIdx<COMPOSITE_SIEMENS_PARAM_SIZE;extDevIdx++)
		{	p = strtok(NULL, sep);
			if(p)
			{	//SENS_SPRINTF(ar77Params, "%s ", p);	//dev no
				strcat(ar77Params, p);
				strcat(ar77Params, " ");
			}
			else
				break;
		}
		mini_file_update_value_for_key(mini_file, SECTION, COMPOSITE_SIEMENS_PARAM_STR, ar77Params); 
		
		//RS485-1
		char parity, stopBit;
		memset(ar77Params, 0, 70);
		p = strtok(NULL, sep);                                                      // rs485_baudrate
		if(p)
			SENS_SPRINTF(ar77Params, "%s", p);
		p = strtok(NULL, sep);
		if(p)
		{	PDS_sel = _io_atoi(p);                                                      // Parity, Digital number, Stop Bit
			if(PDS_sel == 0)	//N_8_1
			{	parity = 1;
				stopBit = 1;
			}
			else if(PDS_sel == 1)	//N_8_2
			{	parity = 1;
				stopBit = 2;
			}
			else if(PDS_sel == 2)	//E_8_1
			{	parity = 3;
				stopBit = 1;
			}
			else if(PDS_sel == 3)	//O_8_1
			{	parity = 2;
				stopBit = 1;
			}
			SENS_SPRINTF(ar77Params+strlen(ar77Params), " 8 %d %d %d", parity, stopBit, PDS_sel);
			mini_file_update_value_for_key(mini_file, SECTION, RS485_1_str, ar77Params);
		}
		//RS485-2
		memset(ar77Params, 0, 70);
		p = strtok(NULL, sep);                                                      // rs485_baudrate
		if(p)
			SENS_SPRINTF(ar77Params, "%s", p);
		p = strtok(NULL, sep);
		if(p)
		{	PDS_sel = _io_atoi(p);                                                      // Parity, Digital number, Stop Bit
			if(PDS_sel == 0)	//N_8_1
			{	parity = 1;
				stopBit = 1;
			}
			else if(PDS_sel == 1)	//N_8_2
			{	parity = 1;
				stopBit = 2;
			}
			else if(PDS_sel == 2)	//E_8_1
			{	parity = 3;
				stopBit = 1;
			}
			else if(PDS_sel == 3)	//O_8_1
			{	parity = 2;
				stopBit = 1;
			}
			SENS_SPRINTF(ar77Params+strlen(ar77Params), " 8 %d %d %d", parity, stopBit, PDS_sel);
			mini_file_update_value_for_key(mini_file, SECTION, RS485_2_str, ar77Params);
		}
		
		//RS232
		memset(ar77Params, 0, 70);
		p = strtok(NULL, sep);                                                        // rs232_1_baudrate
		if (p)
		{	SENS_SPRINTF(ar77Params, "%s", p);
		}
		p = strtok(NULL, sep);
		if (p)
		{	PDS_sel = _io_atoi(p);                                                      // Parity, Digital number, Stop Bit
			if(PDS_sel == 0)	//N_8_1
			{	parity = 1;
				stopBit = 1;
			}
			else if(PDS_sel == 1)	//N_8_2
			{	parity = 1;
				stopBit = 2;
			}
			else if(PDS_sel == 2)	//E_8_1
			{	parity = 3;
				stopBit = 1;
			}
			else if(PDS_sel == 3)	//O_8_1
			{	parity = 2;
				stopBit = 1;
			}
			SENS_SPRINTF(ar77Params+strlen(ar77Params), " 8 %d %d %d", parity, stopBit, PDS_sel);
			mini_file_update_value_for_key(mini_file, SECTION, RS232_1_str, ar77Params);
		}
		/*p = strtok(NULL, sep);	//yushun remove
		if (p)
		{	mini_file_update_value_for_key(mini_file, SECTION, RS232_POWER_str, p);
		}*/
    
		memset(ar77Params, 0, 70);
		for(cameraParamSize=0;cameraParamSize<CAMERA_PARAM_SIZE;cameraParamSize++)
		{	p = strtok(NULL, sep);
			if(p)
			{	strcat(ar77Params, p);
				strcat(ar77Params, " ");
			}
			else
				break;
		}
		mini_file_update_value_for_key(mini_file, SECTION, CAMERA_PARAM_STR, ar77Params);
		
		memset(ar77Params, 0, 70);
		for(cameraParamSize=0;cameraParamSize<VW_PARAM_SIZE;cameraParamSize++)
		{	p = strtok(NULL, sep);
			if(p)
			{	strcat(ar77Params, p);
				strcat(ar77Params, " ");
			}
			else
				break;
		}
		mini_file_update_value_for_key(mini_file, SECTION, VW_PARAM_STR, ar77Params);
		
		p = strtok(NULL, sep);
		if(p)
		{	mini_file_update_value_for_key(mini_file, SECTION, DI_STATUS_str, p);
		}
		
		memset(ar77Params, 0, 70);
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(ar77Params, "%s", p);	//DI0_WAKEUP
		}
		
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(ar77Params+strlen(ar77Params), " %s", p);	//DI1_WAKEUP
		}
		
		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(ar77Params+strlen(ar77Params), " %s", p);	//DI2_WAKEUP
		}

		p = strtok(NULL, sep);
		if (p)
		{	SENS_SPRINTF(ar77Params+strlen(ar77Params), " %s", p);	//DI3_WAKEUP
			mini_file_update_value_for_key(mini_file, SECTION, DI_WAKEUP_str, ar77Params);
		}
    
		p = strtok(NULL, sep);
		if(p)
		{	mini_file_update_value_for_key(mini_file, SECTION, DI_WAKEUP_REC_INTERVAL_STR, p);
		}
	
		SENS_MEM_FREE(para_name);
		SENS_MEM_FREE(first_total_name);
		SENS_MEM_FREE(device_parameter);
		SENS_MEM_FREE(second_total_name);
		SENS_MEM_FREE(device_channel_parm);
		SENS_MEM_FREE(ar77Params);
		//SENS_MEM_FREE(device_command);
		dbg_msg ("%s", "Save device settings\r\n");
	}
	else if(parameter_num == PARAM_SETTING_PQ_VAL_3)				// channel parameter
	{	char channel_count, channel_count_select;
		char enable, deviceNO, item, eqn, filter, p_do_0, p_do_1;
		char *para_name, *first_total_name, *device_parameter;
		char *alias;
		char *alias2;
    
		para_name = SENS_MEM_ZALLOC(10);
		first_total_name = SENS_MEM_ZALLOC(16);
		device_parameter = SENS_MEM_ZALLOC(128);
		alias = SENS_MEM_ZALLOC(128);
		alias2 = SENS_MEM_ZALLOC(128);
    
		p = strtok(NULL, sep);                                                      // channel enable
		if (p)
		{	channel_count_select = _io_atoi(p);
		}
		for(channel_count = 0 + channel_count_select*8; channel_count <= (7+channel_count_select*8); channel_count++)
		{	SENS_SPRINTF(para_name, "%s", "channel_");
			SENS_SPRINTF(first_total_name, "%s%d", para_name, channel_count);                // create prefix
			p = strtok(NULL, sep);                                                      // channel enable
			if(p)	enable = _io_atoi(p);
			p = strtok(NULL, sep);                                                      // channel deviceno
			if(p)	deviceNO = _io_atoi(p);
			p = strtok(NULL, sep);                                                      // channel item number
			if(p)	item = _io_atoi(p);
			p = strtok(NULL, sep);                                                      // channel EQN
			if(p)	eqn = _io_atoi(p);
			p = strtok(NULL, sep);                                                      // channel filter
			if(p)	filter = _io_atoi(p);
			p = strtok(NULL, sep);                                                      //alias
			if(p)
			{	memset(alias, 0, 128);
				memcpy(alias, p, strlen(p));
			}
			p = strtok(NULL, sep);                                                      //alias2
			if(p)
			{	memset(alias2, 0, 128);
				memcpy(alias2, p, strlen(p));
			}
			
			if( enable == 1)
			{	SENS_SPRINTF(device_parameter, "1 %d %d %d %d %s %s", deviceNO, item, eqn, filter, alias, alias2);
				mini_file_update_value_for_key(mini_file, SECTION, first_total_name, device_parameter);
				device_parameter[0] = '\0';
			}
			else if( enable == 0 )
			{	mini_file_update_value_for_key(mini_file, SECTION, first_total_name, "0 0 0 0 0 0 0");
			}
		}
		SENS_MEM_FREE(para_name);
		SENS_MEM_FREE(first_total_name);
		SENS_MEM_FREE(device_parameter);
		SENS_MEM_FREE(alias);
		SENS_MEM_FREE(alias2);
		dbg_msg ("%s", "Save node settings\r\n");
	}
	else if(parameter_num == PARAM_SETTING_DO_VAL_4)				// DO parameter
	{	char doStatus[3];
		char doMode;
		//char para_name[20], first_total_name[26], Mode_write[50];
		char *str_value;
		char /**para_name, *first_total_name,*/ *Mode_write;
    
		//para_name = SENS_MEM_ZALLOC(20);
		//first_total_name = SENS_MEM_ZALLOC(26);
		Mode_write = SENS_MEM_ZALLOC(10);

		/*for(channel_count = 0; channel_count < 2; channel_count++)	//yushun remove
		{	SENS_SPRINTF(para_name, "%s", "CONDITION_DEFI_");
			SENS_SPRINTF(first_total_name, "%s%d", para_name, channel_count);                // create prefix
			p = strtok(NULL, sep);                                                      // channel enable
			if (p)
			{	enable = _io_atoi(p);
			}
			p = strtok(NULL, sep);
			if (p)
			{	length = strlen(p);
				str_value = strrep(p, '!', ' ', length);
			}
			SENS_SPRINTF(Mode_write, "%d %s", enable, str_value);

			if( enable == 1)
			{	mini_file_update_value_for_key(mini_file, SECTION, first_total_name, Mode_write);
				Mode_write[0] = '\0';
			}
			else if( enable == 0 )
			{	mini_file_update_value_for_key(mini_file, SECTION, first_total_name, "0 0 0 0 0");
			}
		}

		for(channel_count = 5; channel_count < 7; channel_count++)
		{	SENS_SPRINTF(para_name, "%s", "CONDITION_DEFI_");
			SENS_SPRINTF(first_total_name, "%s%d", para_name, channel_count);                // create prefix

			p = strtok(NULL, sep);                                                      // channel enable
			if (p)	enable = _io_atoi(p);
			p = strtok(NULL, sep);
			if (p)
			{	length = strlen(p);
				str_value = strrep(p, '!', ' ', length);
			}
			SENS_SPRINTF(Mode_write, "%d %s", enable, str_value);
			if( enable == 1)
			{	mini_file_update_value_for_key(mini_file, SECTION, first_total_name, Mode_write);
				Mode_write[0] = '\0';
			}
			else if( enable == 0 )
			{	mini_file_update_value_for_key(mini_file, SECTION, first_total_name, "0 0 0 0 0");
			}
		}*/
		p = strtok(NULL, sep);		// DO-0 Output High/Low
		if (p)
		{	doStatus[0] = _io_atoi(p);
		}
		p = strtok(NULL, sep);		// DO-1 Output High/Low
		if (p)
		{	doStatus[1] = _io_atoi(p);
		}
		p = strtok(NULL, sep);		// DO-5V Output High/Low
		if (p)
		{	doStatus[2] = _io_atoi(p);
		}
    
		p = strtok(NULL, sep);                                                      // DO_0 MODE
		if (p)
		{	doMode = _io_atoi(p);
		}
		memset(Mode_write, 0, 10);
		//SENS_SPRINTF(Mode_write, "%d", enable);
		SENS_SPRINTF(Mode_write, "%d %d %d %d", doStatus[0], doStatus[1], doStatus[2], doMode);
		mini_file_update_value_for_key(mini_file, SECTION, DO_OUTPUT_POWER_str, Mode_write);
		//mini_file_update_value_for_key(mini_file, SECTION, CONDITION_DEFIM_str, Mode_write);

		//SENS_MEM_FREE(para_name);
		//SENS_MEM_FREE(first_total_name);
		SENS_MEM_FREE(Mode_write);

		dbg_msg ("%s", "Save DO settings\r\n");
	}
	/*else if(parameter_num == 6) 
	{	char *gps_param, *gps_lat, *gps_long, *gps_time;

		gps_param = SENS_MEM_ZALLOC(20);
		gps_time = SENS_MEM_ZALLOC(20);
		gps_long = SENS_MEM_ZALLOC(10);
		gps_lat = SENS_MEM_ZALLOC(10);	  
		p = strtok(NULL, sep);   
		if(p)	memcpy(gps_time, p, strlen(p));
	
		p = strtok(NULL, sep);   
		if(p)	memcpy(gps_long, p, strlen(p));

		p = strtok(NULL, sep);   
		if(p)	memcpy(gps_lat, p, strlen(p));
	
		SENS_SPRINTF(gps_param, "%s %s", gps_long, gps_lat);
		mini_file_update_value_for_key(mini_file, SECTION, GPS_START_Time_str, gps_time);
		mini_file_update_value_for_key(mini_file, SECTION, GPS_POSITION_str, gps_param);

		dbg_msg ("Save GPS data, %s\r\n", gps_param);
	
		SENS_MEM_FREE(gps_time);
		SENS_MEM_FREE(gps_lat);
		SENS_MEM_FREE(gps_long);
		SENS_MEM_FREE(gps_param);
	}
	else if(parameter_num == 10) 
	{	char *gps_rstparam, *gps_day, *gps_hour;

		gps_rstparam = SENS_MEM_ZALLOC(10);
		gps_day = SENS_MEM_ZALLOC(6);
		gps_hour = SENS_MEM_ZALLOC(6);	  
		p = strtok(NULL, sep);   
		if(p)	memcpy(gps_day, p, strlen(p));
		p = strtok(NULL, sep);   
		if(p)	memcpy(gps_hour, p, strlen(p));
	
		SENS_SPRINTF(gps_rstparam, "%s %s", gps_day, gps_hour);	
		mini_file_update_value_for_key(mini_file, SECTION, GPS_POS_RST_TIME_str, gps_rstparam);

		dbg_msg ("Save GPS Position Reset Time, %s\r\n", gps_rstparam);
	
		SENS_MEM_FREE(gps_day);
		SENS_MEM_FREE(gps_hour);
		SENS_MEM_FREE(gps_rstparam);
	}*/
	else if((parameter_num == PARAM_SETTING_FORMULA_1_VAL_7) || (parameter_num == PARAM_SETTING_FORMULA_2_VAL_9))			// formula1 parameter
	{	int idx;
		char length;
		char *Mode_write, *formula;
		char *str_value;
    
		Mode_write = SENS_MEM_ZALLOC(50);
		formula = SENS_MEM_ZALLOC(14);
		p = strtok(NULL, sep);
		if (p)	idx = _io_atoi(p);
		SENS_SPRINTF(formula, "%s%d", "FORMULA_COEF", idx);
		p = strtok(NULL, sep);
		if(p)
		{	length = strlen(p);
			str_value = strrep(p, '!', ' ', length);
		}
		SENS_SPRINTF(Mode_write, "%d %s", idx, str_value);
		mini_file_update_value_for_key(mini_file, SECTION, formula, Mode_write);
		
		SENS_MEM_FREE(Mode_write);
		SENS_MEM_FREE(formula);
		dbg_msg ("%s", "Save formula1 coefficients\r\n");
	}
	else if(parameter_num == PARAM_SETTING_PASSWORD_VAL_8)			// password parameter
	{	char *base64DecodeValue;
		SYS_AUTH_INFO *sysAuthInf = sensSys->sysAuthInf;
		SYS_AUTH_STRUCT *authTable;
		
		for(authTable = sysAuthInf->authTable;authTable;authTable=authTable->next)	//current only 1 account
		{	if(authTable->authType == LOGIN_ID_ADMIN)
			{	break;
			}
		}
		if(authTable == NULL)
			return 0;
		
		p = strtok(NULL, sep);
		if (p)
		{	base64DecodeValue = (char *)base64Decode(p);
			//mini_file_update_value_for_key(mini_file, SECTION, USERNAME_str, base64DecodeValue);
			SENS_MEM_FREE(authTable->username);
			authTable->username = SENS_MEM_ZALLOC(strlen(base64DecodeValue) + 1);
			strcpy(authTable->username, base64DecodeValue);
			SENS_MEM_FREE(base64DecodeValue);
		}
		p = strtok(NULL, sep);
		if (p)
		{	base64DecodeValue = (char *)base64Decode(p);
			//mini_file_update_value_for_key(mini_file, SECTION, PASSWORD_str, base64DecodeValue);
			SENS_MEM_FREE(authTable->psw);
			authTable->psw = SENS_MEM_ZALLOC(strlen(base64DecodeValue) + 1);
			strcpy(authTable->psw, base64DecodeValue);
			SENS_MEM_FREE(base64DecodeValue);
		}
		strtokUnLock();
		dbg_msg ("%s", "Save account/password. Reboot!!\r\n");
		//mini_save_to_file(mini_file, NULL, INI_BAK_FILE);
		writeAuthFile();
		//mini_file_free(mini_file);
		return 1;
	}
	else if(parameter_num == PARAM_SETTING_DO_MAP_VAL_10)
	{	int8_t doMap[4];
		int32_t val = 0;
		uint8_t *eepromBuf;
		EEPROM_CONTEXT *eepromCtx;
		uint16_t crc16;
		
		eepromBuf = SENS_MEM_ZALLOC(sizeof(EEPROM_CONTEXT));
		memcpy(eepromBuf, sensSys->eepromBuf, sizeof(EEPROM_CONTEXT));
		eepromCtx = (EEPROM_CONTEXT *)eepromBuf;
		
		p = strtok(NULL, sep);
		if(p)	val = _io_atoi(p);
		else	val = 0;
		doMap[0] = val - 1;
		p = strtok(NULL, sep);
		if(p)	val = _io_atoi(p);
		else	val = 0;
		doMap[1] = val - 1;
		p = strtok(NULL, sep);
		if(p)	val = _io_atoi(p);
		else	val = 0;
		doMap[2] = val - 1;
		p = strtok(NULL, sep);
		if(p)	val = _io_atoi(p);
		else	val = 0;
		doMap[3] = val - 1;
		eepromCtx->doMap[0] = doMap[0];
		eepromCtx->doMap[1] = doMap[1];
		eepromCtx->doMap[2] = doMap[2];
		eepromCtx->doMap[3] = doMap[3];
		eepromCtx->eepromCrc16 = 0;
		crc16 = swCrc16(eepromBuf, 0, sizeof(EEPROM_CONTEXT)-2);
		eepromCtx->eepromCrc16 = crc16;
		setEepromInfo(EEPROM_CONTEXT_ADDR, eepromBuf);
		SENS_MEM_FREE(eepromBuf);
	}
	strtokUnLock();
	mini_save_to_file(mini_file, NULL, INI_BAK_FILE);
#if defined PLATFORM_XILINX || defined SENSMINIA4_NEO
	if(changeEepromData)
	{	
#ifdef SENSMINIA4_NEO
		uint16_t crc16;
		eepromCtx->eepromCrc16 = 0;
		crc16 = swCrc16(sensSys->eepromBuf, 0, sizeof(EEPROM_CONTEXT)-2);
		eepromCtx->eepromCrc16 = crc16;
		setEepromInfo(EEPROM_CONTEXT_ADDR, sensSys->eepromBuf);
#endif
#ifdef PLATFORM_XILINX
		writeEepromData();
#endif
	}
#endif
	mini_file_free(mini_file);
	return 0;
}
//------------------------------------------------------------------------------