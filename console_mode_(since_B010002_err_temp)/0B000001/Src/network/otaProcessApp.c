#include "network/netApp.h"
#include "systemTimer.h"
#include "math.h"
#include "physicalQuantity.h"
#include "sdCardTask.h"
#include "sensSystem.h"
#include "sensDev.h"
#include "network/Protocol/serverProtocol.h"
#include "littleFs/littleFsPort.h"
#include "driver/spiNorFlash.h"

#include "headers/otaConfig.h"
#if SUPPORT_FOTA_MD5 && USE_MBEDTLS_MD5

#include "mbedtls/md_internal.h"
enum MD5_STS{	
  MD5_STS_NOT_INIT,
  MD5_STS_INIT_DONE
};

typedef struct md5_inf
{	enum MD5_STS md5Sts;
	//unsigned char md5Result[MD5_RESULT_LENGTH];
	unsigned char *md5Result;
	unsigned int md5Length;
	//mbedtls_md_info_t *mdInfo;
	mbedtls_md_context_t mdCtx;
}md5_inf_t;

md5_inf_t	md5Inf;

#endif


#if SUPPORT_FOTA
//FIRMWARE_INFO chksumEntry;
FIRMWARE_INFO crcEntry;
FIRMWARE_INFO fwEntry;
#endif
//------------------------------------------------------------------------------
#if SUPPORT_FOTA_MD5 && !defined PLATFORM_FSL
//------------------------------------------------------------------------------
static void initMd5(void)
{	if(md5Inf.md5Sts == MD5_STS_NOT_INIT)
	{	mbedtls_md_init(&md5Inf.mdCtx);
		mbedtls_md_init_ctx(&md5Inf.mdCtx, mbedtls_md_info_from_type(MBEDTLS_MD_MD5));
		md5Inf.md5Result = SENS_MEM_ZALLOC(mbedtls_md_get_size(md5Inf.mdCtx.md_info));
		
		md5Inf.md5Sts = MD5_STS_INIT_DONE;
		md5Inf.md5Length = 0;
		mbedtls_md_starts(&md5Inf.mdCtx);
	}
}
//------------------------------------------------------------------------------
void resetMd5(void)
{	md5Inf.md5Sts = MD5_STS_NOT_INIT;
	md5Inf.md5Length = 0;
}
//------------------------------------------------------------------------------
void md5Encode(unsigned char *srcBuf, unsigned int srcLength, char final)
{	initMd5();
	mbedtls_md(md5Inf.mdCtx.md_info, (uint8_t *)srcBuf, srcLength, md5Inf.md5Result);
	mbedtls_md_update(&md5Inf.mdCtx, (uint8_t *)srcBuf, srcLength);
	if(final)
		mbedtls_md_finish(&md5Inf.mdCtx, md5Inf.md5Result);
}
//------------------------------------------------------------------------------
void closeMd5(void)
{	mbedtls_md_free(&md5Inf.mdCtx);
}
#endif
//------------------------------------------------------------------------------
void getAndCreateSpiOtaInfo(void)
{	if(SENS_SPI_FS_FILE_EXIST(OTA_FILE_NAME) == LFS_ERR_OK)
		SENS_SPI_FS_FILE_DELETE(OTA_FILE_NAME);
	
	if(SENS_SPI_FS_FILE_EXIST(OTA_CHKSUM_NAME) == LFS_ERR_OK)
		SENS_SPI_FS_FILE_DELETE(OTA_CHKSUM_NAME);
	
#ifdef PLATFORM_FSL
	memoryReadData(RESOURCE_BASE_ADDRESS, sizeof(anasystem_filesystem_t), (uint8_t *)&spiFsBpb);

  if(strncmp(spiFsBpb.tag, ANASYSTEM_BOOT_INDICATE_STR, strlen(ANASYSTEM_BOOT_INDICATE_STR)))
  {	memcpy(spiFsBpb.tag, ANASYSTEM_BOOT_INDICATE_STR, strlen(ANASYSTEM_BOOT_INDICATE_STR));
    spiFsBpb.reserved[0] = '\0';
    spiFsBpb.structVerIndex = RESOURCE_VERSION_INDEX;
    memcpy(spiFsBpb.signature, ANA_SIG, 2);
    spiFsBpb.resourceCount = 0;
    spiFsBpb.resoruceInfoOffset = ENTRY_MAX_SIZE;
    spiFsBpb.firmwareBakInfoOffset = spiFsBpb.resoruceInfoOffset + RESOURCE_MAX_SIZE + ENTRY_MAX_SIZE;
    spiFsBpb.firmwareInfoOffset = spiFsBpb.firmwareBakInfoOffset + ENTRY_MAX_SIZE;
    memorySetProtection(0);
    SENS_TIME_DELAY(200);
    gForceEraseResource = 1;
    memorySectorErase(RESOURCE_BASE_ADDRESS);
    memoryWriteData(RESOURCE_BASE_ADDRESS, sizeof(anasystem_filesystem_t), (uint8_t *)&spiFsBpb);
    gForceEraseResource = 0;
  }

  dbg_msg("%s, find ota info in spi\r\n", __func__);
  dbg_msg("bpb tag %s\r\n", spiFsBpb.tag);
  dbg_msg("bpb structVerIndex %X\r\n", spiFsBpb.structVerIndex);
  dbg_msg("bpb structVerIndex %02X%02X\r\n", spiFsBpb.signature[0], spiFsBpb.signature[1]);
  dbg_msg("bpb resourceCount %d\r\n", spiFsBpb.resourceCount);
  dbg_msg("bpb resoruceInfoOffset %X\r\n", spiFsBpb.resoruceInfoOffset);
  dbg_msg("bpb firmwareInfoOffset %X\r\n", spiFsBpb.firmwareInfoOffset);
  dbg_msg("bpb firmwareBakInfoOffset %X\r\n", spiFsBpb.firmwareBakInfoOffset);

  memset(&chksumEntry, 0, sizeof(firmware_info_t));
  memset(&fwEntry, 0, sizeof(firmware_info_t));

  chksumEntry.type = RESOURCE_TYPE_FIRMWARE;
  chksumEntry.subtype = SUBTYPE_FIRMWARE_CHECKSUM;
  chksumEntry.isValid = 0x55;
  chksumEntry.startAddr = BACKUP_FW_MAX_SIZE + BACKUP_CHKSUM_MAX_SIZE + spiFsBpb.firmwareInfoOffset + ENTRY_MAX_SIZE;

  fwEntry.type = RESOURCE_TYPE_FIRMWARE;
  fwEntry.subtype = SUBTYPE_FIRMWARE;
  fwEntry.isValid = 0x55;
  fwEntry.startAddr = chksumEntry.startAddr + BACKUP_CHKSUM_MAX_SIZE;
  fwEntry.romStartAddr = 0xC000;
#endif
}
//------------------------------------------------------------------------------
static void setHttpGetSizePerApi(OTA_REC_CTX *otaRecCtx)
{	int remainSize;
	
	if(networkCtx->workingInst->netType == NETWORK_TYPE_ETH)
		otaRecCtx->loadSizePerApi = ETH_OTA_LOAD_SIZE_PER_API;
	else		
	{	if(!isnan(sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH]) && (sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] > -115))
		{	if(sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] > -85)
				otaRecCtx->loadSizePerApi = MOBILE_OTA_LOAD_SIZE_PER_API_HIGH_RSSI;
			else
				otaRecCtx->loadSizePerApi = MOBILE_OTA_LOAD_SIZE_PER_API_LOW_RSSI;
		}
		else if(!isnan(sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH]) && (sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] > -115))
		{	if(sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] > -85)
				otaRecCtx->loadSizePerApi = MOBILE_OTA_LOAD_SIZE_PER_API_HIGH_RSSI;
			else
				otaRecCtx->loadSizePerApi = MOBILE_OTA_LOAD_SIZE_PER_API_LOW_RSSI;
		}
		else
			otaRecCtx->loadSizePerApi = MOBILE_OTA_LOAD_SIZE_PER_API_LOW_RSSI;
	}
	
	remainSize = otaRecCtx->loadsizePerCmd % 4096;
	
	if((remainSize + otaRecCtx->loadSizePerApi) > 4096)
		otaRecCtx->loadSizePerApi = 4096 - remainSize;
}
//------------------------------------------------------------------------------
void stopNetworkOta(CLOUD_SERVER_INSTANCE *serverInst, char delTaskDirect)
{	OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
	OTA_MANAGER *otaMng = (OTA_MANAGER *)&sysCtrl->otaMng;
	//OTA_REC_CTX *otaRecCtx = &otaCtx->otaRecCtx;
	HTTP_CONTEXT *httpCtx = selectOtaSvrHttpCtx();
  
	lanSocketClose(serverInst, 1, __func__, __LINE__);
	httpCtx->running = 0;
	networkCtx->otaRunning = 0;
	otaMng->runningAutoFota = 0;
	//sensSys->sysOta.isOtaRunning = 0;
	//sensSys->sysOta.isOtaTime = 0;
	otaCtx->otaSm = OTA_SM_IDLE;
	if(delTaskDirect)
		deleteOtaInfoFile();
	//networkCtx->enFotaLoadingTimeout = 0;
	setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_OTA_ACTIVE);
	//sysParams->powerSavingMode = networkCtx->powerSavingMode;
	
	sysStsFlag1.otaFlag = OTA_FLAG_IDLE;
	setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1, sysStsFlag1.sysStatusFlags);
	killSensminiTimer(SENSMINI_TIMER_LAN_HTTP_GET);
	dbg_msg("%s", "stop OTA process!!\r\n");
	//gprs.fotaLoadingTimeoutValue = GetTimeTicks();
}
//------------------------------------------------------------------------------
int getOtaFromNetwork(CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx)
{	OTA_CONTEXT *otaCtx;
	OTA_REC_CTX *otaRecCtx;
	int error = 0;
	int getLength;
#if SUPPORT_FOTA_MD5
	//file_md5_t *fileMd5;
#endif

	error = ethHttpConnect(serverInst, httpCtx);
	
	if(error)
		goto OTA_END;
	
	dbg_msg("%s", "tcp Tls socket for OTA connect success\r\n");
	
	if(networkCtx->prevOtaRecCtx)
	{	otaRecCtx = networkCtx->prevOtaRecCtx;
		if(otaRecCtx->otaSm == OTA_SM_DOWNLOADING)
		{	memset(httpCtx->path, 0, HTTP_PATH_STR_MAX_LEN);
			getLength = otaRecCtx->loadSizePerApi;
			if((otaRecCtx->binOffset + getLength) > otaRecCtx->binSize)
				getLength = otaRecCtx->binSize - otaRecCtx->binOffset;
			SENS_SPRINTF(httpCtx->path, "%s/FOTA/%s?ver=%X&start=%d&length=%d" EMPTY_CHAR, SENSMINI_HTTP_SERVER_PATH, sensSys->guid.guidString, FW_VERSION, otaRecCtx->loadOffset, getLength);
#if SUPPORT_FOTA_MD5
	//#ifdef PLATFORM_FSL
			resetMd5();
	//#endif
	#ifdef PLATFORM_FSL
			setMd5Length(otaRecCtx->binOffset);
			setMd5OutputValue(otaRecCtx->currMd5);
	#endif
#endif
#ifdef PLATFORM_FSL
			getAndCreateSpiOtaInfo();
			memorySetProtection(0);
			SENS_TIME_DELAY(200);
#endif
			fwEntry.length = otaRecCtx->binSize;
			crcEntry.length = otaRecCtx->binSize / 4096;
			if(otaRecCtx->binSize % 4096)
				crcEntry.length++;
		}
	}
	else
	{	sdDeleteFileNew("otafwTemp.bin");
	}
	if(httpCtx->httpRequestCtx)
		clearRequestCtx(httpCtx);
	error = httpSendRequest(serverInst, httpCtx);
	
	if(error)
		goto OTA_END;
	httpCtx->running = 1;
	setSensminiTimer((void *)networkProcessQ, NETWORK_Q_MSG_HTTP_GET_TIMEOUT, NULL, SENSMINI_TIMER_LAN_HTTP_GET, HTTP_POST_TIMEOUT, TIMER_MODE_TRIGGER);
	
	if(networkCtx->otaCtx == NULL)
		networkCtx->otaCtx = SENS_MEM_ZALLOC(sizeof(OTA_CONTEXT));
	else
		memset(networkCtx->otaCtx, 0, sizeof(OTA_CONTEXT));
	otaCtx = networkCtx->otaCtx;
	otaRecCtx = &otaCtx->otaRecCtx;
	if(networkCtx->prevOtaRecCtx)
	{	otaCtx->otaSm = networkCtx->prevOtaRecCtx->otaSm;
		/*otaRecCtx->otaSm = networkCtx->prevOtaRecCtx->otaSm;
		otaRecCtx->tag = 0x55AA;	
		otaRecCtx->loadOffset = networkCtx->prevOtaRecCtx->loadOffset;
		otaRecCtx->binOffset = networkCtx->prevOtaRecCtx->binOffset;*/
		memcpy(otaRecCtx, networkCtx->prevOtaRecCtx, sizeof(OTA_REC_CTX));
		SENS_MEM_FREE(networkCtx->prevOtaRecCtx);
		networkCtx->prevOtaRecCtx = NULL;
	}
	else
	{	otaCtx->otaSm = OTA_SM_CHECK_VERSION;
		otaRecCtx->otaSm = OTA_SM_CHECK_VERSION;
		otaRecCtx->tag = 0x55AA;
		setHttpGetSizePerApi(otaRecCtx);
		/*if(networkCtx->workingInst->netType == NETWORK_TYPE_ETH)
			otaRecCtx->loadSizePerApi = ETH_OTA_LOAD_SIZE_PER_API;
		else
			otaRecCtx->loadSizePerApi = MOBILE_OTA_LOAD_SIZE_PER_API;*/
	}
OTA_END:
	if(error)
	{	//httpCtx->running = 0;
		lanSocketClose(serverInst, 1, __func__, __LINE__);
	}
	
	return error;
}
//------------------------------------------------------------------------------
int otaStartProcess(HTTP_CONTEXT *httpCtx)
{	int rsp;
	CLOUD_SERVER_INSTANCE *serverInst;
	NET_INSTANCE *workingInst = networkCtx->workingInst;
	
	//networkCtx->otaRunning = 1;
	
	serverInst = findFreeServerInstance();
	serverInst->isOtaSocket = 1;
	serverInst->isUsing = 1;
	serverInst->isTempUsing = 0;
						
	serverInst->xmitProtocolType = PROTOCOL_HTTP;
	if(httpCtx->url == NULL)
		httpCtx->url = SENS_MEM_ZALLOC(HTTP_URL_STR_MAX_LEN);
	else
		memset(httpCtx->url, 0, HTTP_URL_STR_MAX_LEN);	
	if(httpCtx->path == NULL)
		httpCtx->path = SENS_MEM_ZALLOC(HTTP_PATH_STR_MAX_LEN);
	else
		memset(httpCtx->path, 0, HTTP_PATH_STR_MAX_LEN);
						
	setOtaUrl(httpCtx->url, workingInst->netMdvpnType, 1);
	memset(serverInst->serverName, 0, 256);
	strcat(serverInst->serverName, "device.senslink.net");
	//SENS_SPRINTF(httpCtx->url, "http://211.21.191.26:8080" EMPTY_CHAR);
	SENS_SPRINTF(httpCtx->path, "%s/FOTA/%s/checknewfw?ver=%X", SENSMINI_HTTP_SERVER_PATH, sensSys->guid.guidString, FW_VERSION);
	httpCtx->httpCmd = HTTP_GET;
	//sensSys->runningAutoFota = 1;
	setPsmStatus(SET_PSM_STATUS, DISABLE_PSM_BMP_OTA_ACTIVE);
	return getOtaFromNetwork(serverInst, httpCtx);
}
//------------------------------------------------------------------------------
static int getAndGetNewFirmwareInfo(HTTP_RSP_INFO *httpRspInfo, CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx)
{	char *pos;
	int fileSize;
	int error = -1;
	OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
	OTA_REC_CTX *otaRecCtx = &otaCtx->otaRecCtx;
	
	pos = strstr(httpRspInfo->content, "fwSize");
	if(pos == NULL)
		return error;
	pos = strstr(pos, ":");
	SENS_SSCANF(pos+1, "%d", &fileSize);
	otaCtx->fileSize = fileSize;
	otaCtx->loadSize = fileSize;
						
	otaRecCtx->loadSize = otaCtx->fileSize;
	otaRecCtx->loadOffset = 0;
	otaRecCtx->binOffset = 0;
	otaRecCtx->binSize = otaCtx->fileSize;
						
	memset(httpCtx->path, 0, HTTP_PATH_STR_MAX_LEN);
#if SUPPORT_FOTA_MD5
	SENS_SPRINTF(httpCtx->path, "%s/FOTA/%s?ver=%X&start=0&length=256", SENSMINI_HTTP_SERVER_PATH, sensSys->guid.guidString, FW_VERSION);
	otaCtx->otaSm = OTA_SM_CHECK_INFO;
	otaRecCtx->otaSm = OTA_SM_CHECK_INFO;
#else
	SENS_SPRINTF(httpCtx->path, "%s/FOTA/%s?ver=%X&start=%d&length=%d", SENSMINI_HTTP_SERVER_PATH, sensSys->guidString, FW_VERSION, otaRecCtx->loadOffset, otaRecCtx->loadSizePerApi);
	otaCtx->otaSm = OTA_SM_DOWNLOADING;
	otaRecCtx->otaSm = OTA_SM_DOWNLOADING;
#endif
	
	if(serverInst->fd == -1)
	{	error = ethHttpConnect(serverInst, httpCtx);
		if(error)
		{	return error;
		}
	}
	if(httpCtx->httpRequestCtx)
		clearRequestCtx(httpCtx);
	error = httpSendRequest(serverInst, httpCtx);
	return error;
}
//------------------------------------------------------------------------------
static int checkOtaInfoFromBinHeader(HTTP_RSP_INFO *httpRspInfo, CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx)
{	OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
	OTA_REC_CTX *otaRecCtx = &otaCtx->otaRecCtx;
	
#if SUPPORT_FOTA_MD5
	file_md5_t *fileMd5;
#endif
	int error=0;
	
	getAndCreateSpiOtaInfo();
	//memorySetProtection(0);
	//SENS_TIME_DELAY(200);
	
	fwEntry.length = otaRecCtx->binSize;
	crcEntry.length = otaRecCtx->binSize / 4096;
	if(otaRecCtx->binSize % 4096)
		crcEntry.length++;
	otaRecCtx->checksum = 0;
	crcEntry.currWriteOffset = 0;
	otaRecCtx->checksumWriteOffset = 0;


#if SUPPORT_FOTA_MD5
	fileMd5 = (file_md5_t *)httpRspInfo->content;
	
	if((fileMd5->sig == 0x55AA) && (fileMd5->ver == 0x01) && (fileMd5->cpuModel == CPU_MODEL))
	{	otaRecCtx->loadOffset += 256;
		otaRecCtx->binSize -= 256;
		memcpy(otaRecCtx->srcMd5, fileMd5->md5, 16);
		otaRecCtx->isValidMd5 = 1;
		sdWriteOtaInfo(otaRecCtx, NULL, 0x01);
		resetMd5();
		fwEntry.length = otaRecCtx->binSize;
		crcEntry.length = otaRecCtx->binSize / 4096;

		if(otaRecCtx->binSize % 4096)
			crcEntry.length++;
	}
	else if((fileMd5->sig == 0x55AA) && (fileMd5->ver == 0x01))
	{	error = -1;
		return error;
	}
#endif
	SENS_SPRINTF(httpCtx->path, "%s/FOTA/%s?ver=%X&start=%d&length=%d", SENSMINI_HTTP_SERVER_PATH, sensSys->guid.guidString, FW_VERSION, otaRecCtx->loadOffset, otaRecCtx->loadSizePerApi);
	dbg_msg(RED"load new firmware offset:%d/%d\r\n"ORG_COLOR, otaRecCtx->loadOffset, otaRecCtx->binSize);
	otaCtx->otaSm = OTA_SM_DOWNLOADING;
	otaRecCtx->otaSm = OTA_SM_DOWNLOADING;
	if(serverInst->fd == -1)
	{	error = ethHttpConnect(serverInst, httpCtx);
		if(error)
		{	return error;
		}
	}
	if(httpCtx->httpRequestCtx)
		clearRequestCtx(httpCtx);
	error = httpSendRequest(serverInst, httpCtx);
	return error;
}
//------------------------------------------------------------------------------
void writeToNorFalsh(HTTP_RSP_INFO *httpRspInfo, CLOUD_SERVER_INSTANCE *serverInst)
{	int idx;
	//char checksumBuf[2];
	//unsigned short *chksumWBuf;
	OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
	OTA_REC_CTX *otaRecCtx = &otaCtx->otaRecCtx;
	int tempChksum;
	int mode = OVER_WRITE_MODE;

	if(SENS_SPI_FS_FILE_EXIST(OTA_FILE_NAME) == LFS_ERR_OK)
		mode = CONTINUE_WRITE_MODE;

	SENS_SPI_FS_WRITE_FILE(OTA_FILE_NAME, (uint8_t *)httpRspInfo->content, httpRspInfo->fileSize, 0, mode);
	dbgMsg("[OTA] write %d bytes to OTA file, mode:%d, after write file size is %d\r\n", httpRspInfo->fileSize, mode, SENS_SPI_FS_GET_FILE_SIZE(OTA_FILE_NAME));
	
	if(!(otaRecCtx->binOffset % SPI_SECTOR_SIZE))
	{	otaRecCtx->loadsizePerCmd = 0;
		otaRecCtx->checksum = 0;
		dbgMsg("[OTA] address %X, reset checksum value!!\r\n", otaRecCtx->binOffset);
	}
	/*if(!(otaRecCtx->binOffset % SPI_SECTOR_SIZE))
	{	memorySectorErase(fwEntry.startAddr + otaRecCtx->binOffset + RESOURCE_BASE_ADDRESS);
	}
	memoryWriteData(fwEntry.startAddr + otaRecCtx->binOffset + RESOURCE_BASE_ADDRESS, httpRspSts->fileSize, (uint8_t *)httpRspSts->content);*/
	otaRecCtx->loadsizePerCmd += httpRspInfo->fileSize;
	
	//chksumWBuf = (unsigned short *)httpRspInfo->content;
	//otaRecCtx->checksum = 0;
	/*for(idx = 0; idx < httpRspInfo->fileSize / 2; idx++)
	{	otaRecCtx->checksum += chksumWBuf[idx];
	}*/

	/*if((otaRecCtx->loadsizePerCmd == SPI_SECTOR_SIZE) || ((otaRecCtx->binOffset + httpRspInfo->fileSize) >= otaRecCtx->binSize))
	{	while(otaRecCtx->checksum > 0xFFFF)
		{	tempChksum = otaRecCtx->checksum >> 16;
			otaRecCtx->checksum &= 0xFFFF;
			otaRecCtx->checksum += tempChksum;
		}

		checksumBuf[0] = otaRecCtx->checksum & 0xFF;
		checksumBuf[1] = otaRecCtx->checksum >> 8;
	
		mode = OVER_WRITE_MODE;
		if(SENS_SPI_FS_FILE_EXIST(OTA_CHKSUM_NAME) == LFS_ERR_OK)
			mode = CONTINUE_WRITE_MODE;
		SENS_SPI_FS_WRITE_FILE(OTA_CHKSUM_NAME, (uint8_t *)&checksumBuf, 2, 0, mode);
		dbgMsg("[OTA] write checksum, mode %d, checksum is %X\r\n", mode, otaRecCtx->checksum);
		otaRecCtx->checksum = 0;
	}*/
	
  //memoryWriteData(chksumEntry.startAddr + checksumWriteOffset + RESOURCE_BASE_ADDRESS, 2, (uint8_t *)&checksumBuf);
	//checksumWriteOffset += 2;
	
	//sdWriteFile("otafwTemp.bin", httpRspInfo->content, httpRspInfo->fileSize, otaRecCtx->binOffset, CONTINUE_WRITE_MODE);
	dbg_msg("[OTA] write to file size:%d, offset:%d\r\n", httpRspInfo->fileSize, otaRecCtx->binOffset);
	
	otaRecCtx->loadOffset += httpRspInfo->fileSize;
	otaRecCtx->binOffset += httpRspInfo->fileSize;

#if SUPPORT_FOTA_MD5 && defined PLATFORM_FSL
	if(otaRecCtx->isValidMd5)
	{	if(md5Inf.md5Length != otaRecCtx->binOffset)
		{	//md5Encode((uint8_t *)httpRspSts->content, httpRspSts->fileSize, (otaRecCtx->binOffset >= otaRecCtx->binSize)? 1:0);
			//memcpy(otaRecCtx->currMd5, md5Inf.md5Result, MD5_RESULT_LENGTH);
			//dbg_msg("[OTA]LAN curr MD5 :"MD5_RESULT_STR" \r\n", MD5_RESULT_PRINT(otaRecCtx->currMd5));
		}
		else
		{
		}
		//dbg_msg("%s(%d) encode md5 done\r\n", __func__, __LINE__);
	}
#endif
}
//------------------------------------------------------------------------------
int wireLanFinalWriteFirmwareInfo(void)
{	int offset = 0;
	int size;
	//int idx;
	OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
	OTA_REC_CTX *otaRecCtx = &otaCtx->otaRecCtx;
	int error = 0;
	uint8_t *crc16Buf;
	uint16_t *wCrc16Buf;
	uint8_t *fwDataBuf;
	uint16_t *wFwDataBuf;
	//char checksumBuf[2];
	uint16_t crc16;
	int crcSectorIdx;
  
#if SUPPORT_FOTA_MD5
	if(otaRecCtx->isValidMd5)
	{	int fileLength;
		
		fileLength = SENS_SPI_FS_GET_FILE_SIZE(OTA_FILE_NAME);
		fwDataBuf = SENS_MEM_ZALLOC(SPI_SECTOR_SIZE);
		/*
		 *	CRC FILE: [SECTOR-0 CRC16], [SECTOR-1 CRC16], ......, [SECTOR-N CRC16], [SECTOR-0~N CRC16]
		 */
		crc16Buf = SENS_MEM_ZALLOC((crcEntry.length + 1) * 2);
		wCrc16Buf = (uint16_t *)crc16Buf;
		crcSectorIdx = 0;
		resetMd5();
		while(fileLength)
		{	size = SPI_SECTOR_SIZE;
			if(fileLength < size)
				size = fileLength;
			SENS_SPI_FS_READ_FILE(OTA_FILE_NAME, (char*)fwDataBuf, size, offset, NORMAL_READ_MODE);
			md5Encode((uint8_t *)fwDataBuf, size, (fileLength == size)? 1:0);
			crc16 = swCrc16(fwDataBuf, 0, size);
			wCrc16Buf[crcSectorIdx] = crc16;
			crcSectorIdx++;
			offset += size;
			fileLength -= size;
#ifdef EN_WATCHDOG
			watchDogClr();
#endif
		}
		memcpy(otaRecCtx->currMd5, md5Inf.md5Result, MD5_RESULT_LENGTH);
		dbg_msg("[OTA]LAN curr MD5 :"MD5_RESULT_STR" \r\n", MD5_RESULT_PRINT(otaRecCtx->currMd5));
		closeMd5();
		SENS_MEM_FREE(fwDataBuf);
		fwDataBuf = NULL;
		offset = 0;
		if(memcmp(otaRecCtx->currMd5, otaRecCtx->srcMd5, MD5_RESULT_LENGTH))	//md5 check error
		{	dbg_msg(RED"[OTA]MD5 check error src :"MD5_RESULT_STR", cal :"MD5_RESULT_STR"\r\n"ORG_COLOR, MD5_RESULT_PRINT(otaRecCtx->srcMd5), MD5_RESULT_PRINT(otaRecCtx->currMd5));
			//deleteOtaInfoFile();
			//SENS_SPI_OTA_FILE_DELETE(OTA_FILE_NAME);
			//SENS_SPI_OTA_FILE_DELETE(OTA_CHKSUM_NAME);
			error = -2;
			return error;
		}
		else	//md5 check pass
		{	dbg_msg(GREEN"[OTA]MD5 check pass src :"MD5_RESULT_STR", cal :"MD5_RESULT_STR"\r\n"ORG_COLOR, MD5_RESULT_PRINT(otaRecCtx->srcMd5), MD5_RESULT_PRINT(otaRecCtx->currMd5));
		}
	}
#endif
	dbg_msg("%s", "[OTA]download to SPI success, start check!!\r\n");

	crc16 = swCrc16(crc16Buf, 0, crcEntry.length * 2);
	wCrc16Buf[crcEntry.length] = crc16;
	
	SENS_SPI_FS_WRITE_FILE(OTA_CHKSUM_NAME, (uint8_t *)crc16Buf, (crcEntry.length+1) * 2, 0, OVER_WRITE_MODE);

	SENS_MEM_FREE(crc16Buf);
	//SENS_MEM_FREE(fwData);

	otaRecCtx->otaFinish = 1;
	sysStsFlag1.otaFlag = OTA_FLAG_FINISH;
	sysStsFlag1.execHramFlag = EXEC_HRAM_FLAG_FOTA;
	setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG1, sysStsFlag1.sysStatusFlags);
	
	return error;
}
//------------------------------------------------------------------------------
static int fileToSpi(HTTP_RSP_INFO *httpRspInfo, CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx)
{	OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
	OTA_REC_CTX *otaRecCtx = &otaCtx->otaRecCtx;
	int error;
	int getLength;
	
	writeToNorFalsh(httpRspInfo, serverInst);
	if(otaRecCtx->binOffset >= otaRecCtx->binSize)
	{	error = wireLanFinalWriteFirmwareInfo();
		if(error < 0)
			return error;
		else
		{	otaCtx->otaSm = OTA_SM_DONE;
			otaRecCtx->otaSm = OTA_SM_DONE;
		}
	}
	else
	{	setHttpGetSizePerApi(otaRecCtx);
		getLength = otaRecCtx->loadSizePerApi;
		if((otaRecCtx->binOffset + getLength) > otaRecCtx->binSize)
			getLength = otaRecCtx->binSize - otaRecCtx->binOffset;

		SENS_SPRINTF(httpCtx->path, "%s/FOTA/%s?ver=%X&start=%d&length=%d" EMPTY_CHAR, SENSMINI_HTTP_SERVER_PATH, sensSys->guid.guidString, FW_VERSION, otaRecCtx->loadOffset, getLength);
		dbg_msg(RED"load new firmware offset:%d/%d\r\n"ORG_COLOR, otaRecCtx->loadOffset, otaRecCtx->binSize);
		otaCtx->otaSm = OTA_SM_DOWNLOADING;
		otaRecCtx->otaSm = OTA_SM_DOWNLOADING;
		sdWriteOtaInfo(otaRecCtx, NULL, 0x01);
		if(serverInst->fd == -1)
		{	error = ethHttpConnect(serverInst, httpCtx);
			if(error)
			{	return error;
			}
		}
		if(httpCtx->httpRequestCtx)
			clearRequestCtx(httpCtx);
		error = httpSendRequest(serverInst, httpCtx);
	}
	return error;
}
//------------------------------------------------------------------------------
void otaProcess(HTTP_RSP_INFO *httpRspInfo, CLOUD_SERVER_INSTANCE *serverInst, HTTP_CONTEXT *httpCtx)
{	OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
	
	switch(otaCtx->otaSm)
	{	case OTA_SM_CHECK_VERSION:
				if(!httpRspInfo->isOctetStreamData)
				{	char *pos;
					int error;
					pos = strstr(httpRspInfo->content, "No new firmware");
					if(pos)
					{	stopNetworkOta(serverInst, 1);
						break;
					}
					error = getAndGetNewFirmwareInfo(httpRspInfo, serverInst, httpCtx);
					if(error)
					{	stopNetworkOta(serverInst, 1);
					}
					else
					{	setSensminiTimer((void *)networkProcessQ, NETWORK_Q_MSG_HTTP_GET_TIMEOUT, NULL, SENSMINI_TIMER_LAN_HTTP_GET, HTTP_POST_TIMEOUT, TIMER_MODE_TRIGGER);
					}
				}
				else
				{	stopNetworkOta(serverInst, 1);
				}
				break;
		case OTA_SM_CHECK_INFO:
				{	int error = 0;
					if(httpRspInfo->isOctetStreamData)
					{	error = checkOtaInfoFromBinHeader(httpRspInfo, serverInst, httpCtx);
						if(!error)
							setSensminiTimer((void *)networkProcessQ, NETWORK_Q_MSG_HTTP_GET_TIMEOUT, NULL, SENSMINI_TIMER_LAN_HTTP_GET, HTTP_POST_TIMEOUT, TIMER_MODE_TRIGGER);
					}
					else
					{	stopNetworkOta(serverInst, 1);
					}
				}
				break;
		case OTA_SM_DOWNLOADING:
				{	int error = 0;
					if(httpRspInfo->isOctetStreamData)
						error = fileToSpi(httpRspInfo, serverInst, httpCtx);
					else
					{	stopNetworkOta(serverInst, 1);
						break;
					}
					if(error <= 0)
					{	if(error)
						{	dbg_msg("%s", "HTTP Fail, stop OTA\r\n");
							stopNetworkOta(serverInst, 0);
							break;
						}
						else
						{	if(otaCtx->otaSm != OTA_SM_DONE)
							{	setSensminiTimer((void *)networkProcessQ, NETWORK_Q_MSG_HTTP_GET_TIMEOUT, NULL, SENSMINI_TIMER_LAN_HTTP_GET, HTTP_POST_TIMEOUT, TIMER_MODE_TRIGGER);
								break;
							}
						}
					}
				}
		case OTA_SM_DONE:
				lanSocketClose(serverInst, 1, __func__, __LINE__);
				//httpCtx->running = 0;
				sysCtrl->isReadyToReboot = 1;
				dbg_msg("%s", "ota success, reboot!!\r\n");
				break;
		default:
				break;
	}
}
//------------------------------------------------------------------------------
int triggerNextFota(CLOUD_SERVER_INSTANCE *serverInst)
{	OTA_CONTEXT *otaCtx = networkCtx->otaCtx;
	OTA_REC_CTX *otaRecCtx = &otaCtx->otaRecCtx;
	int error;
	int getLength;
	HTTP_CONTEXT *httpCtx = selectOtaSvrHttpCtx();
	
	setHttpGetSizePerApi(otaRecCtx);
	getLength = otaRecCtx->loadSizePerApi;
	if((otaRecCtx->binOffset + getLength) > otaRecCtx->binSize)
		getLength = otaRecCtx->binSize - otaRecCtx->binOffset;
	SENS_SPRINTF(httpCtx->path, "%s/FOTA/%s?ver=%X&start=%d&length=%d" EMPTY_CHAR, SENSMINI_HTTP_SERVER_PATH, sensSys->guid.guidString, FW_VERSION, otaRecCtx->loadOffset, getLength);
	if(((otaRecCtx->loadOffset - 256) % 409600) == 0)
		lanSocketClose(serverInst, 1, __func__, __LINE__);
		
	dbg_msg(RED"load new firmware offset:%d/%d\r\n"ORG_COLOR, otaRecCtx->loadOffset, otaRecCtx->binSize);
	otaCtx->otaSm = OTA_SM_DOWNLOADING;
	otaRecCtx->otaSm = OTA_SM_DOWNLOADING;
	sdWriteOtaInfo(otaRecCtx, NULL, 0x01);
	SENS_TIME_DELAY(10);
	if(serverInst->fd == -1)
	{	error = ethHttpConnect(serverInst, httpCtx);
		if(error)
		{	otaCtx->sockConnErrorCnt++;
			return FOTA_HTTP_ERR_CONNECT;
		}
	}
	if(httpCtx->httpRequestCtx)
		clearRequestCtx(httpCtx);
	error = httpSendRequest(serverInst, httpCtx);
	if(error == -1)
		error = FOTA_HTTP_ERR_XMIT;
	return error;
}
//------------------------------------------------------------------------------