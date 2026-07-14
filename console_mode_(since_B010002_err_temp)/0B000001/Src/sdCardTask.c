#include "sdCardTask.h"
#include "ff.h"
#include "string.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "FS/fsCtrl.h"
#include "physicalQuantity.h"
//#include "physicalQuantity.h"
#include "rtcDateTime.h"
#include "ini2.h"
#include "sensDev.h"
#include "network/netApp.h"
//#include "param.h"
//#include "sensors/sensorTask.h"
//#include "sensDev.h"
#ifdef SPI_FILE_SYSTEM
#include "littleFs/littleFsPort.h"
#endif
#ifdef AUTO_SEND_WITH_INFO
#include "network/AppnetworkProcess.h"
#endif

#if AUTO_DATA_SYNC
#include "AutoDataSync.h"
#endif

#ifdef AUTO_SEND_WITH_INFO
//AUTO_SEND_INFO	*autoSendCtx = NULL;
#endif

#ifdef FILESYSTEM_USE_TASK
QueueHandle_t	sdSendQ = NULL;

//------------------------------------------------------------------------------
static void watchDogExtend(uint8_t extend)
{
#if EN_WATCHDOG
	if(extend)
	{
#if defined DEBUG_WATCHDOG
		WDT_Open(WDT_TIMEOUT_2POW20, WDT_RESET_DELAY_18CLK, FALSE, FALSE);	//32s, not auto reset
#else
		WDT_Open(WDT_TIMEOUT_2POW20, WDT_RESET_DELAY_18CLK, TRUE, FALSE);	//32s, with auto reset
#endif
	}
	else
	{	
#if defined DEBUG_WATCHDOG
		WDT_Open(WDT_TIMEOUT_2POW18, WDT_RESET_DELAY_18CLK, FALSE, FALSE);	//8s, not auto reset
#else
		WDT_Open(WDT_TIMEOUT_2POW18, WDT_RESET_DELAY_18CLK, TRUE, FALSE);	//8s, with auto reset
#endif
	}	
	watchDogClr();
#endif
}

#endif
//------------------------------------------------------------------------------
void SDH0_IRQHandler(void)
{	uint32_t volatile u32Isr;
#if defined SENSMINIA4_PRO || defined SENSMINIA4_NEO
	uint32_t volatile u32Ier;
#endif
	
	/* FMI data abort interrupt */
	if(SDH0->GINTSTS & SDH_GINTSTS_DTAIF_Msk)
	{	/* ResetAllEngine() */
		SDH0->GCTL |= SDH_GCTL_GCTLRST_Msk;
	}

	/* ----- SD interrupt status */
	u32Isr = SDH0->INTSTS;
#if defined SENSMINIA4_PRO || defined SENSMINIA4_NEO
	u32Ier = SDH0->INTEN;
#endif
	if(u32Isr & SDH_INTSTS_BLKDIF_Msk)
	{	/* Block down */
#if defined SENSMINIS2
		g_u8SDDataReadyFlag = TRUE;
#elif defined SENSMINIA4_PRO || defined SENSMINIA4_NEO
		SD0.DataReadyFlag = TRUE;
#endif
		SDH0->INTSTS = SDH_INTSTS_BLKDIF_Msk;
	}

	if(
#if defined SENSMINIA4_PRO || defined SENSMINIA4_NEO
		(u32Ier & SDH_INTEN_CDIEN_Msk) &&
#endif
		(u32Isr & SDH_INTSTS_CDIF_Msk))    // card detect
	{	/* ----- SD interrupt status */
		/* it is work to delay 50 times for SD_CLK = 200KHz */
		{	int volatile i;
			for(i = 0; i < 0x500; i++); /* delay to make sure got updated value from REG_SDISR. */
			u32Isr = SDH0->INTSTS;
		}
#if (DEF_CARD_DETECT_SOURCE==CardDetect_From_DAT3)
		if(!(u32Isr & SDH_INTSTS_CDSTS_Msk))
#else
		if(u32Isr & SDH_INTSTS_CDSTS_Msk)
#endif
		{	//printf("\n***** card remove !\n");
			SD0.IsCardInsert = FALSE;   /* SDISR_CD_Card = 1 means card remove for GPIO mode */
			memset(&SD0, 0, sizeof(SDH_INFO_T));
		}
		else
		{	//printf("***** card insert !\n");
			SDH_Open(SDH0, CardDetect_From_GPIO);
			//SDH_Open(SDH0, CardDetect_From_DAT3);
			SDH_Probe(SDH0);
		}
		SDH0->INTSTS = SDH_INTSTS_CDIF_Msk;
	}

	/* CRC error interrupt */
	if(u32Isr & SDH_INTSTS_CRCIF_Msk)
	{	if(!(u32Isr & SDH_INTSTS_CRC16_Msk))
		{	//printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
			// handle CRC error
		}
		else if(!(u32Isr & SDH_INTSTS_CRC7_Msk))
		{	
#if defined SENSMINIA4_PRO || defined SENSMINIA4_NEO
			if(!SD0.R3Flag)
#else
			if(!g_u8R3Flag)
#endif
			{	//printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
				// handle CRC error
			}
		}
		/* Clear interrupt flag */
		SDH0->INTSTS = SDH_INTSTS_CRCIF_Msk;
	}

	if(u32Isr & SDH_INTSTS_DITOIF_Msk)
	{	//printf("***** ISR: data in timeout !\n");
		SDH0->INTSTS |= SDH_INTSTS_DITOIF_Msk;
	}

	/* Response in timeout interrupt */
	if(u32Isr & SDH_INTSTS_RTOIF_Msk)
	{	//printf("***** ISR: response in timeout !\n");
		SDH0->INTSTS |= SDH_INTSTS_RTOIF_Msk;
	}
}
//------------------------------------------------------------------------------
int sdInit(void)
{	sensSys->sdInst.sdPresence = 0;
	//NVIC_EnableIRQ(SDH0_IRQn);
#ifdef SENSMINIA4_NEO
	NVIC_SetPriority(SDH0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
#endif
	//SD_nCD = 0;
	//SDH_Open_Disk(SDH0, CardDetect_From_DAT3);
	SDH_Open_Disk(SDH0, CardDetect_From_GPIO);
	//f_chdrive(MSD_TF); 
#ifdef FILESYSTEM_USE_TASK 
	if(sensSys->sdInst.sdPresence)
	{	OSA_TASK_CREATE(SDCARD_TASK);
		SENS_TIME_DELAY(2);
	}
#endif
	return 0;
}
//------------------------------------------------------------------------------
#ifdef FILESYSTEM_USE_TASK
void filenameToAbsFilepath(SD_FILE_CTRL *sdCtrl, char *fileName)
{	char *pos;

	if(sdCtrl->fileName == NULL)
		sdCtrl->fileName = SENS_MEM_ZALLOC(MAX_FILE_PATH_LEN);
	pos = strstr(fileName, MSD_TF);
	if(pos)
		memcpy(sdCtrl->fileName, fileName, strlen(fileName));
	else
		SENS_SPRINTF(sdCtrl->fileName, "%s%s", MSD_TF, fileName);
}
//------------------------------------------------------------------------------
void clearSdCtrlVariable(SD_FILE_CTRL *sdCtrl)
{	if(sdCtrl)
	{	if(sdCtrl->fileName)
			SENS_MEM_FREE(sdCtrl->fileName);
		//if(sdCtrl->buffer)
		//	SENS_MEM_FREE(sdCtrl->buffer);
		SENS_EVENT_DESTROY(sdCtrl->lwevent);
		SENS_MEM_FREE(sdCtrl);
	}
}
//------------------------------------------------------------------------------
int waitSDProcessDone(SD_FILE_CTRL *sdCtrl, char flag, int timeInSec)
{	int result, mask = 0;

	if(timeInSec <= 2)
		timeInSec = 8;
	
	watchDogExtend(1);
	result = SENS_EVENT_WAIT(sdCtrl->lwevent, flag, FALSE, 200*timeInSec);
	watchDogExtend(0);
	mask = result;
	if(mask & flag)
	{	if(mask == LW_EVENT_FILE_PROCESS_SUCCESS)
			return SD_PROCESS_SUCCESS;
		else if(mask == LW_EVENT_FILE_OPEN_FAIL)
			return SD_PROCESS_FAIL;
	}
	
	return SD_PROCESS_FAIL;
}
//------------------------------------------------------------------------------
void setOpenFail(SD_FILE_CTRL *sdCtrl)
{	dbg_msg(RED"%s(%d) open file(%s) NULL\r\n"ORG_COLOR, __func__, __LINE__, sdCtrl->fileName);
	SENS_EVENT_SET(sdCtrl->lwevent, LW_EVENT_FILE_OPEN_FAIL);
}
//------------------------------------------------------------------------------
void setReadFail(SD_FILE_CTRL *sdCtrl)
{	dbg_msg(RED"%s(%d) read file(%s) length error(%d, %d)\r\n"ORG_COLOR, __func__, __LINE__, sdCtrl->fileName, sdCtrl->realRwLength, sdCtrl->rwLength);
	SENS_EVENT_SET(sdCtrl->lwevent, LW_EVENT_FILE_READ_FAIL);
}
//------------------------------------------------------------------------------
void setWriteFail(SD_FILE_CTRL *sdCtrl)
{	dbg_msg(RED"%s(%d) Write file(%s) length error(%d, %d)\r\n"ORG_COLOR, __func__, __LINE__, sdCtrl->fileName, sdCtrl->realRwLength, sdCtrl->rwLength);
	SENS_EVENT_SET(sdCtrl->lwevent, LW_EVENT_FILE_WRITE_FAIL);
}
#endif
//------------------------------------------------------------------------------
int isFileExist(char *filename)
{	
#ifdef SPI_FILE_SYSTEM 
	#ifdef ONLY_ONE_STORAGE
	int err;
	if(!sensSys->sdInst.sdPresence)
	{	err = SENS_SPI_FS_FILE_EXIST(filename);
		if(err == 0)
			return 1;
		else
			return 0;
	}
	#endif
#endif
	if(!sensSys->sdInst.sdPresence)
		return 0;
	
#if defined FILESYSTEM_USE_TASK
	SD_FILE_CTRL *sdFileCtrl;
	struct TaskQMsg msg;
	int rsp;
	
	sdFileCtrl = SENS_MEM_ZALLOC(sizeof(SD_FILE_CTRL));
	filenameToAbsFilepath(sdFileCtrl, filename);
	if(SENS_EVENT_CREATE(sdFileCtrl->lwevent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{
	}
	msg.msgId = SD_Q_MSG_CHK_FILE_EXIST;
	msg.ptr = (char *)sdFileCtrl;
	if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	clearSdCtrlVariable(sdFileCtrl);
		return 0;
	}
	rsp = waitSDProcessDone(sdFileCtrl, LW_EVENT_FILE_CHK_OPEN, 2);
	clearSdCtrlVariable(sdFileCtrl);
	if(rsp == SD_PROCESS_SUCCESS)
	{	return 1;
	}
	else
		return 0;
#else
	SENS_FILE_PTR fd_ptr;
	int error;
	
	fsLock();
	fd_ptr = SENS_FILE_OPEN(FS_TF, filename, "r", &error);
	if(error)
	{	fsUnlock();
		return 0;
	}
	SENS_FILE_CLOSE(fd_ptr);
	fsUnlock();
	return 1;
#endif
}
//------------------------------------------------------------------------------
#ifndef FILESYSTEM_USE_TASK 
int8_t isExistDir(char *absDirName)
{	char path[MAX_FILE_PATH_LEN];
	
	nameToAbsolutePath(FS_TF, absDirName, path);
	return (SENS_FILE_DIR_EXIST(path) == 0);
}
#endif
//------------------------------------------------------------------------------
int8_t createDir(char *absDirName)
{	
#ifdef SPI_FILE_SYSTEM 
	#ifdef ONLY_ONE_STORAGE
	int err;
	if(!sensSys->sdInst.sdPresence)
	{	err = SENS_SPI_FS_CREATE_DIR(absDirName);
		if(err == 0)
			return 1;
		else
			return 0;
	}
	#endif
#endif
	if(!sensSys->sdInst.sdPresence)
		return 0;
#if defined FILESYSTEM_USE_TASK
	SD_FILE_CTRL *sdCtrl;
	struct TaskQMsg msg;
  int rsp;
	
	sdCtrl = SENS_MEM_ZALLOC(sizeof(SD_FILE_CTRL));

	filenameToAbsFilepath(sdCtrl, absDirName);
	if(SENS_EVENT_CREATE(sdCtrl->lwevent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{
	}
	
	msg.msgId = SD_Q_MSG_CREATE_DIR;
	msg.ptr = (char *)sdCtrl;
	if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	clearSdCtrlVariable(sdCtrl);
		return 0;
	}
	rsp = waitSDProcessDone(sdCtrl, LW_EVENT_FILE_CHK_OPEN, 2);
	clearSdCtrlVariable(sdCtrl);
	if(rsp == SD_PROCESS_SUCCESS)
	{	return 1;
	}
	else
		return 0;
#else
	if(isFileExist(absDirName))
		return 1;
	return (SENS_FILE_DIR_CREATE(absDirName) == 0);
#endif
}
//------------------------------------------------------------------------------
static void createFileBySize(char *fileName, long fileSize
#if defined REC_INTERVAL_FIXED
	, REC_HEADER *recHeader
#endif
	)
{	
#if defined REC_INTERVAL_FIXED
	uint8_t *buffer;
	uint32_t offset = 0;
	uint32_t totalWriteSize;
	uint32_t perWriteSize;
	uint32_t allocSize;
	
	
	allocSize = 4096;
	if(allocSize > fileSize)
		allocSize = fileSize;
	if(allocSize < sizeof(REC_HEADER))
		allocSize = sizeof(REC_HEADER);
		
	buffer = SENS_MEM_ALLOC(allocSize);
	
	memcpy(buffer, (void *)recHeader, sizeof(REC_HEADER));
	//memset(buffer, 0x00, sizeof(REC_HEADER));
	sdWriteFile(fileName, (char *)buffer, sizeof(REC_HEADER), 0, OVER_WRITE_MODE);

	totalWriteSize = fileSize;	// + sizeof(REC_HEADER);
	offset += sizeof(REC_HEADER);
	//rec_per_day = 86400L/60;
	while(totalWriteSize)
	{	perWriteSize = allocSize;
		if(perWriteSize > totalWriteSize)
			perWriteSize = totalWriteSize;
		
		memset((void *)buffer, 0xFF, perWriteSize);
		sdWriteFile(fileName, (char *)buffer, perWriteSize, offset, CONTINUE_WRITE_MODE);
		offset += perWriteSize;
		totalWriteSize -= perWriteSize;
	}
	
#else
#if N_TEMP_REMOVE
	long rec_per_day, bytes_per_day;
	rec_per_day = 86400L/sensSys->sysCfg.sensSysCfg.recordSec;
	bytes_per_day = sensPq->bytesPerRec * rec_per_day;

	dbg_msg("[SD CARD] rec_sec: %d, ai_number: %d\r\n", sensSys->sysCfg.sensSysCfg.recordSec, sensPq->pqNumber);
	dbg_msg("                     [SD CARD] bytes_per_rec: %d, rec_per_day: %d\r\n", sensPq->pqNumber*sizeof(float), rec_per_day);
	dbg_msg("                     [SD CARD] bytes_per_day(File size): %d (%d KB)\r\n", bytes_per_day, bytes_per_day/1024);	
	sdWriteFile(fileName, NULL, fileSize, 0, WRITE_NAN_DATA_MODE);
#endif
#endif
}
//------------------------------------------------------------------------------
int getFileLength(char *fileName)
{	int fileLength=0;
	
#ifdef SPI_FILE_SYSTEM 
	#ifdef ONLY_ONE_STORAGE
	if(!sensSys->sdInst.sdPresence)
	{	return SENS_SPI_FS_GET_FILE_SIZE(fileName);
	}
	#endif
#endif
	if(!sensSys->sdInst.sdPresence)
		return 0;
#if defined FILESYSTEM_USE_TASK
	SD_FILE_CTRL *sdFileCtrl;
	struct TaskQMsg msg;
	
	sdFileCtrl = SENS_MEM_ZALLOC(sizeof(SD_FILE_CTRL));
	filenameToAbsFilepath(sdFileCtrl, fileName);
	if(SENS_EVENT_CREATE(sdFileCtrl->lwevent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{
	}
	msg.msgId = SD_Q_MSG_GET_FILE_LENGTH;
	msg.ptr = (char *)sdFileCtrl;
	if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	clearSdCtrlVariable(sdFileCtrl);
		return 0;
	}

	if(waitSDProcessDone(sdFileCtrl, LW_EVENT_FILE_CHK_OPEN, 2) == SD_PROCESS_SUCCESS)
		fileLength = sdFileCtrl->fileLength;

	clearSdCtrlVariable(sdFileCtrl);
	return fileLength;
#else
	//char path[MAX_FILE_PATH_LEN];
	SENS_FILE_PTR fd_ptr;
	int error;
	
	//nameToAbsolutePath(FS_TF, fileName, path);
	fsLock();
	
	fd_ptr = SENS_FILE_OPEN(FS_TF, fileName, "r", &error);
	if(error)
	{	fsUnlock();
		return -1;
	}
	fileLength = f_size(fd_ptr);
	SENS_FILE_CLOSE(fd_ptr);
	fsUnlock();
#endif
	return fileLength;
}
//------------------------------------------------------------------------------
int sdReadFile(char *fileName, char *buf, int length, int offset, char mode)
{	int readLen = 0;
	int rsp;
#ifdef SPI_FILE_SYSTEM 
	#ifdef ONLY_ONE_STORAGE
	if(!sensSys->sdInst.sdPresence)
	{	rsp = SENS_SPI_FS_READ_FILE(fileName, buf, length, offset, mode);
		if(rsp == -1)
			return SD_RW_OPEN_FILE_FAIL;
		else if(rsp == -3)
			return SD_RW_LENGTH_ERROR;
		return rsp;
	}
	#endif
#endif
	if(!sensSys->sdInst.sdPresence)
		return 0;
#if defined FILESYSTEM_USE_TASK
	SD_FILE_CTRL *sdFileCtrl;
	struct TaskQMsg msg;
	
	if(mode == READ_LENGTH_MODE)
	{	return getFileLength(fileName);
	}
	//dbgMsg("%s", "--1--\r\n");
	sdFileCtrl = SENS_MEM_ZALLOC(sizeof(SD_FILE_CTRL));
	filenameToAbsFilepath(sdFileCtrl, fileName);
	sdFileCtrl->buffer = buf;
	sdFileCtrl->rwLength = length;
	sdFileCtrl->offset = offset;
	sdFileCtrl->rwMode = mode;
	if(SENS_EVENT_CREATE(sdFileCtrl->lwevent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{
	}
	//dbgMsg("%s", "--2--\r\n");
	msg.msgId = SD_Q_MSG_READ;
	msg.ptr = (char *)sdFileCtrl;
	if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	clearSdCtrlVariable(sdFileCtrl);
		return SD_PROCESS_FAIL;
	}
	//SENS_TIME_DELAY(1);
	//dbgMsg("%s", "--3--\r\n");
	rsp = waitSDProcessDone(sdFileCtrl, LW_EVENT_FILE_CHK_READ, 6);
	//dbgMsg("%s", "--4--\r\n");
	clearSdCtrlVariable(sdFileCtrl);
	if(rsp == SD_PROCESS_SUCCESS)
	{	readLen = sdFileCtrl->realRwLength;
		return readLen;
	}
	return rsp;
#else
	
	SENS_FILE_PTR fd_ptr;
	int error;

	if(mode == READ_LENGTH_MODE)
	{	return getFileLength(fileName);
	}
	
	fsLock();
	if( mode == NORMAL_READ_MODE)
		fd_ptr = SENS_FILE_OPEN(FS_TF, fileName, "r", &error);
	else if(mode == BIN_READ_MODE)
		fd_ptr = SENS_FILE_OPEN(FS_TF, fileName, "rb", &error);
	else if(mode == BIN_READ_MODE2)
		fd_ptr = SENS_FILE_OPEN(FS_TF, fileName, "r+", &error);
	if(error)
	{	fsUnlock();
		return SD_RW_OPEN_FILE_FAIL;
	}
	
	SENS_FILE_SEEK(fd_ptr, offset, IO_SEEK_SET);
	SENS_FILE_READ(fd_ptr, (uint8_t *)buf, length, &rsp);
	SENS_FILE_CLOSE(fd_ptr);
	fsUnlock();
	if(rsp != length)
	{	return SD_RW_LENGTH_ERROR;
	}
	else
		return rsp;
#endif
}
//------------------------------------------------------------------------------
int sdWriteFile(char *fileName, char *buf, int len, int offset, int mode)
{	int readLen = 0;
	int rsp;

#ifdef SPI_FILE_SYSTEM 
	#ifdef ONLY_ONE_STORAGE
	if(!sensSys->sdInst.sdPresence)
	{	return SENS_SPI_FS_WRITE_FILE(fileName, (uint8_t *)buf, len, offset, mode);
	}
	#endif
#endif
	if(!sensSys->sdInst.sdPresence)
		return 0;
#if defined FILESYSTEM_USE_TASK
	SD_FILE_CTRL *sdFileCtrl;
	struct TaskQMsg msg;
	sdFileCtrl = SENS_MEM_ZALLOC(sizeof(SD_FILE_CTRL));
	filenameToAbsFilepath(sdFileCtrl, fileName);
	sdFileCtrl->buffer = buf;
	sdFileCtrl->rwLength = len;
	sdFileCtrl->offset = offset;
	sdFileCtrl->rwMode = mode;
	if(mode == WRITE_NAN_DATA_MODE)
	{	sdFileCtrl->fileLength = len;
	}
	if(SENS_EVENT_CREATE(sdFileCtrl->lwevent, LWEVENT_AUTO_CLEAR) != MQX_OK )
  {
  }
	msg.msgId = SD_Q_MSG_WRITE;
	msg.ptr = (char *)sdFileCtrl;
	if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	clearSdCtrlVariable(sdFileCtrl);
		return SD_PROCESS_FAIL;
	}
	rsp = waitSDProcessDone(sdFileCtrl, LW_EVENT_FILE_CHK_READ, 12);
	clearSdCtrlVariable(sdFileCtrl);
	if(mode == WRITE_NAN_DATA_MODE)
	{	if(sdFileCtrl->buffer)
			SENS_MEM_FREE(sdFileCtrl->buffer);
	}
	if(rsp == SD_PROCESS_SUCCESS)
	{	readLen = sdFileCtrl->realRwLength;
		return readLen;
	}
	return rsp;
#else
	SENS_FILE_PTR fd_ptr;
	int error;
	
	fsLock();

	if(mode == WRITE_NAN_DATA_MODE)
	{	int chunkSize = 8192;
		int remainSize = len;
		int bytesWritten, writeError=0;
		
		if(buf == NULL)
		{	buf = SENS_MEM_ALLOC(chunkSize);
			memset(buf, 0xFF, chunkSize);
		}
		fd_ptr = SENS_FILE_OPEN(FS_TF, fileName, "w", &error);
								
		while(remainSize)
		{	if(remainSize < chunkSize)
			{	chunkSize = remainSize;
			}
			SENS_FILE_WRITE(fd_ptr, (uint8_t *)buf, chunkSize, &bytesWritten);
			if(bytesWritten != chunkSize)
			{	writeError = 1;
				break;
			}
			remainSize -= chunkSize;
		}
		SENS_FILE_CLOSE(fd_ptr);
		SENS_MEM_FREE(buf);
		fsUnlock();
		if(writeError)
		{	return -1;
		}
		return len;
	}
	else
	{	fd_ptr = SENS_FILE_OPEN(FS_TF, fileName, "w", &error);	//internal auto create file
	
		if(mode == OVER_WRITE_MODE)
		{	SENS_FILE_SEEK(fd_ptr, offset, IO_SEEK_SET);
		}
		else if(mode == CONTINUE_WRITE_MODE)
		{	offset = f_size(fd_ptr);
			//sdCtrl->fileLength = sdCtrl->offset;
			SENS_FILE_SEEK(fd_ptr, offset, IO_SEEK_SET);
		}
		SENS_FILE_WRITE(fd_ptr, (uint8_t *)buf, len, &rsp);
		SENS_FILE_CLOSE(fd_ptr);
		fsUnlock();
		if(rsp != len)
		{	return -1;
		}
		return rsp;
	}
#endif
}
//------------------------------------------------------------------------------
int sdFileDelete(char *filename)
{	
#ifdef SPI_FILE_SYSTEM 
	#ifdef ONLY_ONE_STORAGE
	if(!sensSys->sdInst.sdPresence)
	{	return SENS_SPI_FS_FILE_DELETE(filename);
	}	
	#endif
#endif
	if(!sensSys->sdInst.sdPresence)
		return 0;
#if defined FILESYSTEM_USE_TASK
	SD_FILE_CTRL *sdCtrl;
	struct TaskQMsg msg;
  int rsp;
	
	sdCtrl = SENS_MEM_ZALLOC(sizeof(SD_FILE_CTRL));
	filenameToAbsFilepath(sdCtrl, filename);
	if(SENS_EVENT_CREATE(sdCtrl->lwevent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{
	}
	msg.msgId = SD_Q_MSG_DELETE_FILE;
	msg.ptr = (char *)sdCtrl;
	if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	clearSdCtrlVariable(sdCtrl);
		return -1;
	}
	rsp = waitSDProcessDone(sdCtrl, LW_EVENT_FILE_CHK_OPEN | LW_EVENT_FILE_DELETE_FAIL, 2);
	clearSdCtrlVariable(sdCtrl);
	return rsp;
#else
	SENS_FILE_DELETE(FS_TF, filename);
	return 0;
#endif
}
//------------------------------------------------------------------------------
void readRecNByte(int datetime[7], int recSec, int pqNum, int bytePerRec, long NByte, char* readBuf, int *readBytes)
{	char f_name[30];
	long idx, idxMax;
	int length;

	memset(readBuf, 0xFF, NByte);

	idx = (((long)datetime[3]*60 + (long)datetime[4])*60 + (long)datetime[5])/(long)recSec*(long)bytePerRec;
	idxMax = ((long)86400)/(long)recSec*(long)bytePerRec;

	if( (idx+NByte) > idxMax) NByte = idxMax - idx;
	*readBytes = NByte;
	SENS_SPRINTF(f_name, "%d/%02d/i%02ds%04d.d%02d", datetime[0], datetime[1], pqNum, recSec, datetime[2]);

	length = sdReadFile(f_name, readBuf, NByte, idx, NORMAL_READ_MODE);
	if(length == SD_RW_LENGTH_ERROR)
	{	//*readBytes = 0;
		dbg_msg("%s", "[WARNING] SD: Read Error\r\n");
	}
	else if(length < 0)
	{	dbg_msg("[SD CARD] No File: %s\r\n",f_name);
	}
	dbg_msg("[SD CARD] history-%d Byte\r\n",(*readBytes));
}
//------------------------------------------------------------------------------
int writeDiHistory(DI_CHG_CONTEXT *diChgCtx, uint8_t *diBuf)
{	S_RTC_TIME_DATA_T dateTime;
	char fileName[MAX_FILE_PATH_LEN];
	int writeLen;
	
	timestampToDateTime(diChgCtx->unixTime, &dateTime);
	if(dateTime.u32Year < 2026)
		return 0;
	
	memset(fileName, 0, MAX_FILE_PATH_LEN);
	SENS_SPRINTF(fileName, "%d", dateTime.u32Year);
	if(createDir(fileName) != SD_PROCESS_SUCCESS)
	{	dbgMsg("[SD] Can not create Directory %d\r\n", fileName);
		sysCtrl->isReadyToReboot = 1;
		return -1;
	}
	SENS_SPRINTF(fileName, "%d/%02d", dateTime.u32Year, dateTime.u32Month);
	if(createDir(fileName) != SD_PROCESS_SUCCESS)
	{	dbgMsg("[SD] Can not create Directory %d\r\n", fileName);
		sysCtrl->isReadyToReboot = 1;
		return -1;
	}
	SENS_SPRINTF(fileName, "%d/%02d/d%02di%02d.h%02d", dateTime.u32Year, dateTime.u32Month, dateTime.u32Day, DIQUANTITY, dateTime.u32Hour);     // ex. "sd:/2012/11/d12i02.h18"
	writeLen = sdWriteFile(fileName, (char *)diBuf, 10, 0, CONTINUE_WRITE_MODE);
	if(writeLen != 10)
	{	dbgMsg("%s", "[SD] Failed to write DI Data\r\n");
	}
	return 0;
}
//------------------------------------------------------------------------------
int readDiHistory(int *dateTime, int diNumber, char *readBuf, int readBytes, char mode, int offset)
{	char fileName[MAX_FILE_PATH_LEN];
	int length;
	
	
	memset(fileName, 0, MAX_FILE_PATH_LEN);
	SENS_SPRINTF(fileName, "%d/%02d/d%02di%02d.h%02d", dateTime[0], dateTime[1], dateTime[2], diNumber, dateTime[3]);

	length = getFileLength(fileName);
	if(mode == READ_LENGTH_MODE)
	{
	}
	else if((mode == NORMAL_READ_MODE) && length)
	{	length = sdReadFile(fileName, readBuf, readBytes, offset, NORMAL_READ_MODE);
		if(length != readBytes)
		{	dbg_msg("%s", "[WARNING] SD: read DI record one hour error\r\n");
			length = 0;
		}
	}
	dbgMsg("[SD] DI History %d byte\r\n", length);
	
	return length;
}
//------------------------------------------------------------------------------
void writeHistoryRecDataToTf(void)
{	char filename[MAX_FILE_PATH_LEN];
	uint32_t bytesPerDay, recPerDay;
	uint32_t offset, length = 0;
	uint32_t currHistoryRecTimeSlot;
	S_RTC_TIME_DATA_T recDateTime;
	uint32_t currentTimeStamp;
	int fileExistResult;
	
#if REC_INTERVAL_FIXED
	uint8_t *recHeaderBuf;
	REC_HEADER *recHeader = NULL;
	uint8_t slotBmpIdx;
	uint8_t slotBmpBit;
	uint8_t slotBmpVal;
#endif
	uint8_t recSizeForV1 = 0;
	
	if(sensSys->dateTime.u32Year < VALID_YEAR)
		return;
	
	currentTimeStamp = sensSys->sysTimer.currentTimeStamp;
#if TIME_SHIFT_OPERATE
	if(sysCtrl->tsoValue)
		currentTimeStamp -= (sysCtrl->tsoValue % sysCfg->sensSysCfg.recordSec);
#endif
	timestampToDateTime(currentTimeStamp, &recDateTime);
	
#if !REC_INTERVAL_FIXED
	#if N_TEMP_REMOVE
	memcpy(&recDateTime, (void *)&sensSys->dateTime, sizeof(S_RTC_TIME_DATA_T));
	if(sysCtrl->pwrManager.currPsm == PARAM_PSM_ADVANCE)
	{	
		currHistoryRecTimeSlot = getVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG);
		if(currHistoryRecTimeSlot != sensSys->sysTimer.currHistoryRecTimeSlot)	//current time is not next time slot
		{	currentTimeStamp = dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
			currentTimeStamp = ROUNDUP(currentTimeStamp, sysParams->recordSec);
			timestampToDateTime(currentTimeStamp, &recDateTime);
		}
	}
	else
	#endif
#endif
		currHistoryRecTimeSlot = sensSys->sysTimer.currHistoryRecTimeSlot;
	dbgMsg("curr rec time:%02d:%02d:%02d, slot:%d\r\n", recDateTime.u32Hour, recDateTime.u32Minute, recDateTime.u32Second, currHistoryRecTimeSlot);
	
	if(sysCfg->cameraCfg)
		checkAndTriggerCamera(currHistoryRecTimeSlot);
	
	if(checkOta())
	{	startTrigOta();
	}

#ifndef ONLY_ONE_STORAGE
	#if N_TEMP_REMOVE
	if(!sensSys->sdInst.sdPresence)
	{	sysCtrlFlag2.sdRecDone = 1;
		sysCtrlFlag2.pqSendDone = 0;
		setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_CTRL_FLAG, sysCtrlFlag.sysCtrlFlags);
		return;
	}
	#endif
#endif
	
	memset(filename, 0, MAX_FILE_PATH_LEN);
	SENS_SPRINTF(filename, "%d", recDateTime.u32Year);
	if(!createDir(filename))
	{	sysCtrl->isReadyToReboot = 1;
		sysRecSlotFlag.currHistoryRecSlot = currHistoryRecTimeSlot - 1;
		setVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG, sysRecSlotFlag.recSlotFlag);
		return;
	}
	SENS_SPRINTF(filename+strlen(filename), "/%02d", recDateTime.u32Month);
	if(!createDir(filename))
	{	sysCtrl->isReadyToReboot = 1;
		sysRecSlotFlag.currHistoryRecSlot = currHistoryRecTimeSlot - 1;
		setVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG, sysRecSlotFlag.recSlotFlag);
		return;
	}
#if REC_INTERVAL_FIXED
	SENS_SPRINTF(filename+strlen(filename), "/%02d%02dn%03d.bin", recDateTime.u32Month, recDateTime.u32Day, sensPq->pqNumber);
	recHeaderBuf = SENS_MEM_ALLOC(sizeof(REC_HEADER));	
#else
	SENS_SPRINTF(filename+strlen(filename), "/i%02ds%04d.d%02d", sensPq->pqNumber, sysParams->recordSec, recDateTime.u32Day);
#endif
	if(!isFileExist(filename))
	{	
#if REC_INTERVAL_FIXED
		recPerDay = 86400 / 60;
		bytesPerDay = sensPq->bytesPerRec * recPerDay;
			
		memset(recHeaderBuf, 0, sizeof(REC_HEADER));
		recHeader = (REC_HEADER *)recHeaderBuf;
		recHeader->headerTag.tag = 0x55AA;
	#if REC_FILE_2_SERV_DATA_SYNC
		recHeader->headerTag.ver = FIXED_REC_INTERVAL_VERSION_2;
	#else
		recHeader->headerTag.ver = FIXED_REC_INTERVAL_VERSION;
	#endif
		recHeader->headerTag.recCnt = 0;
		recHeader->headerTag.sendCnt = 0;
		recHeader->headerTag.compareStartSlot = currHistoryRecTimeSlot;
		recHeader->headerTag.checkSendSlot = currHistoryRecTimeSlot;
		recHeader->headerTag.endChkSlot = currHistoryRecTimeSlot + 1;
		recHeader->headerTag.currTimestamp = currentTimeStamp;
		recHeader->headerTag.lastRecSlot = currHistoryRecTimeSlot;
		createFileBySize(filename, bytesPerDay, recHeader);
	#if AUTO_DATA_SYNC
		addRecordToHistoryFile(filename, currentTimeStamp, -1);
	#endif
#else
		recPerDay = 86400 / sysParams->recordSec;
		if(86400 % sysParams->recordSec)
			recPerDay++;
		bytesPerDay = sensPq->bytesPerRec * recPerDay;
		createFileBySize(filename, bytesPerDay);
#endif
		//sdWriteFile(filename, NULL, bytesPerDay, 0, WRITE_NAN_DATA_MODE);
	}
#if REC_INTERVAL_FIXED
	slotBmpIdx = currHistoryRecTimeSlot / 8;
	slotBmpBit = currHistoryRecTimeSlot % 8;
	if(recHeader == NULL)
		sdReadFile(filename, (char *)recHeaderBuf, sizeof(REC_HEADER), 0, NORMAL_READ_MODE);
	recHeader = (REC_HEADER *)recHeaderBuf;
	if(recHeader->headerTag.ver == FIXED_REC_INTERVAL_VERSION)
		recSizeForV1 = 180;
	
	recHeader->recSlotBmpFlag[slotBmpIdx] |= (1<<slotBmpBit);
	recHeader->headerTag.recCnt++;
	recHeader->headerTag.endChkSlot = currHistoryRecTimeSlot + 1;
	recHeader->headerTag.lastRecSlot = currHistoryRecTimeSlot;
	
	length = sdWriteFile(filename, (char *)recHeaderBuf, sizeof(REC_HEADER) - recSizeForV1, 0, OVER_WRITE_MODE);
	if(length != (sizeof(REC_HEADER) - recSizeForV1))
	{	dbg_msg("%s", "[WARNING] SD: Failed to write data_2\r\n");
		sysCtrl->isReadyToReboot = 1;
		sysRecSlotFlag.currHistoryRecSlot = currHistoryRecTimeSlot - 1;
		setVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG, sysRecSlotFlag.recSlotFlag);
		return;
	}
	offset = currHistoryRecTimeSlot * sensPq->bytesPerRec;
	offset += (sizeof(REC_HEADER) - recSizeForV1);
	SENS_MEM_FREE(recHeaderBuf);
#else
	offset = 0;
	if(sysParams->recordSec < 86400)
		offset = ((((recDateTime.u32Hour * 60) + recDateTime.u32Minute)*60)+recDateTime.u32Second) / sysParams->recordSec * sensPq->bytesPerRec;
#endif
	length = sdWriteFile(filename, (char *)sensPq->pqVal, sensPq->bytesPerRec, offset, OVER_WRITE_MODE);
	if(length != sensPq->bytesPerRec)
	{	dbg_msg("%s", "[WARNING] SD: Failed to write data_2\r\n");
		sysCtrl->isReadyToReboot = 1;
		sysRecSlotFlag.currHistoryRecSlot = currHistoryRecTimeSlot - 1;
		setVBatRegValue(RTC_SPARE_REG_ITEM_REC_TO_SD_FLAG, sysRecSlotFlag.recSlotFlag);
	}
	else
	{	sysStsFlag2.sdRecDone = 1;
		sysStsFlag2.pqUploadDone = 0;
		clearDiReleaseFlag(DI_RELEASE_FLAG_REC);
		setPsmStatus(CLR_PSM_STATUS, DISABLE_PSM_BMP_RECORD_DATA);
		sysTimer->sdRecTick = GetTimeTicks();
		setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG2, sysStsFlag2.sysStatusFlags);
		
		//ǥ?? DI Counter, ȧӪͲ???y?郿10?֧?Ӌˣһ?΍
		for(int diIdx=0;diIdx<DIQUANTITY;diIdx++)
		{	sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_0 + diIdx] = 0;
			if(diIdx< 4)
				setVBatRegValue(RTC_SPARE_REG_ITEM_DI0_COUNTER+diIdx, sensPq->onboardPq[ON_BOARD_PQ_DI_COUNTER_0 + diIdx]);
		}
	}
}
//------------------------------------------------------------------------------
void recordPqToTf(void)
{	int idx;
	int currentGetPqCnt = 0;
#if SUPPORT_IOA_WEB_API
	CLOUD_SERVER_INSTANCE	*serverInst;
#endif
	
	if(sensPq->pqNumber)
	{	for(idx=0;idx<sensPq->pqNumber;idx++)
		{	if(sensPq->pqGetMap[idx/32] & (1 << (idx % 32)))
			{	currentGetPqCnt++;
			}
		}
		
		if(currentGetPqCnt != sensPq->pqNumber)
		{	if((GetTimeTicks() - sensPq->startTime) < 30000)
			{	return;
			}
		}
		if(isTimeToRecPq())
		{	//dbgMsg("PQ Ready Map:%X \r\n", sensPq->pqGetMap[0]);
			writeHistoryRecDataToTf();
#if SUPPORT_IOA_WEB_API
			for(idx=0;idx<MAX_CLOUD_SERVER;idx++)
			{	serverInst = (CLOUD_SERVER_INSTANCE *)&networkCtx->cloudServers[idx];
				if(serverInst->isUsing && serverInst->xmitProtocolType == PROTOCOL_IOA_WEB_API)
				{	
#if AUTO_DATA_SYNC
					SENS_SEM_LOCK(serverInst->autoDataSyncCtxLock);
					if(serverInst->autoDataSyncCtx == NULL)
					{	SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
						checkRecordFromHistoryFile(serverInst, __func__, __LINE__);
					}
					else
						SENS_SEM_UNLOCK(serverInst->autoDataSyncCtxLock);
#endif
				}
			}
#endif
		}
	}
}
//------------------------------------------------------------------------------
void clearAllJpegFile(void)
{	char dirPath[20];
	
	memset(dirPath, 0, 20);
	strcat(dirPath, "DCIM");
	SENS_FILE_DEL_ALL_FILE_IN_DIR(FS_TF, dirPath, 0);
}
//------------------------------------------------------------------------------
int createImgFile(char *filename)
{	
#ifdef SPI_FILE_SYSTEM 
	#ifdef ONLY_ONE_STORAGE
	if(!sensSys->sdInst.sdPresence)
	{	
	}	
	#endif
#endif
	if(!sensSys->sdInst.sdPresence)
		return 0;
#if defined FILESYSTEM_USE_TASK
	SD_FILE_CTRL *sdCtrl;
	struct TaskQMsg msg;
  int rsp;
	
	sdCtrl = SENS_MEM_ZALLOC(sizeof(SD_FILE_CTRL));
	filenameToAbsFilepath(sdCtrl, filename);
	
	if(SENS_EVENT_CREATE(sdCtrl->lwevent, LWEVENT_AUTO_CLEAR) != MQX_OK )
	{
	}
  
	msg.msgId = SD_Q_MSG_CREATE_JPG_FILE;
	msg.ptr = (char *)sdCtrl;
	if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	clearSdCtrlVariable(sdCtrl);
		return -1;
	}
	rsp = waitSDProcessDone(sdCtrl, LW_EVENT_FILE_CHK_OPEN | LW_EVENT_FILE_CREATE_FILE_FAIL, 12);
	clearSdCtrlVariable(sdCtrl);
	return rsp;
#else
	char path[20] = {0};
	SENS_FILE_PTR file;
	int error;

	strcat(path, "DCIM");
	if(!createDir(path))
	{	sysCtrl->isReadyToReboot = 1;
		return -1;
	}
	
	if(!isFileExist(filename))
	{	
createNewFile:
		file = SENS_FILE_OPEN(FS_TF, filename, "a", &error);
		if(!error)
			SENS_FILE_CLOSE(file);
	}
	else
	{	error = SENS_FILE_DELETE(FS_TF, filename);
		if(error)
		{	sysCtrl->isReadyToReboot = 1;
			return -1;
		}
		goto createNewFile;
	}
	return 0;
#endif
}
//------------------------------------------------------------------------------
#if 1
void deleteOtaInfoFile(void)
{	char buf[MAX_FILE_PATH_LEN];
	memset(buf, 0, sizeof(buf));

	strcat(buf, "otaInf.inf");
	//SENS_FILE_DELETE(FS_TF, buf);
	sdFileDelete(buf);
}
//------------------------------------------------------------------------------
void sdWriteOtaInfo(OTA_REC_CTX *otaRecCtx, char *ackId, char type)
{	char buf[MAX_FILE_PATH_LEN];
	int writeLen1;
	char write1Error=0;

	memset(buf, 0, sizeof(buf));
	strcat(buf, "otaInf.inf");

	writeLen1 = sdWriteFile(buf, (char *)otaRecCtx, sizeof(OTA_REC_CTX), 0, OVER_WRITE_MODE);
	if(writeLen1 == SD_RW_LENGTH_ERROR)
	{	write1Error = 1;
	}
	
  if(write1Error)
  {	dbg_msg("[WARNING] SD: Failed to write data write len is %d, real Write Len:%d\r\n", sizeof(OTA_REC_CTX), writeLen1);
  }
}
//------------------------------------------------------------------------------
OTA_REC_CTX *sdReadOtaInfo(void)
{	OTA_REC_CTX *otaRecCtx;
	char f_name[30];
	char *buf;
	int rdLen;

	memset(f_name, 0, 30);
	strcat(f_name, "otaInf.inf");
	buf = SENS_MEM_ZALLOC(sizeof(OTA_REC_CTX));
	
	rdLen = sdReadFile(f_name, buf, sizeof(OTA_REC_CTX), 0, NORMAL_READ_MODE);
	if(rdLen == SD_RW_LENGTH_ERROR)
	{	dbg_msg("[WARNING] SD: OTA file len fail, %d, %d\r\n", sizeof(OTA_REC_CTX), rdLen);
		SENS_MEM_FREE(buf);
		return NULL;
	}
	else if(rdLen < 0)
	{	dbg_msg("%s", "[WARNING] SD: OTA file doesn't exist.\r\n");
		SENS_MEM_FREE(buf);
		return NULL;
	}
	else
	{	otaRecCtx = (OTA_REC_CTX *)buf;
		return otaRecCtx;
	}
}
#endif
//------------------------------------------------------------------------------
/*void getLogFileName(void)
{	SENS_SPRINTF((char *)sensSys->sysLog.logFileName, "LOG%02d/%d%02d%02d.log", sensSys->dateTime.u32Month, sensSys->dateTime.u32Year, sensSys->dateTime.u32Month, sensSys->dateTime.u32Day);
}*/
#if defined FILESYSTEM_USE_TASK
//------------------------------------------------------------------------------
static SENS_LOG_STRUCT *createNewLogQueue(char *buf, int len)
{	SENS_LOG_STRUCT *queue;
	SENS_LOG_STRUCT *newQueue;
	
	newQueue = (SENS_LOG_STRUCT *)SENS_MEM_ZALLOC(sizeof(SENS_LOG_STRUCT));
	if(newQueue == NULL)
		return NULL;

	queue = (SENS_LOG_STRUCT *)&sysCtrl->logDataArray;
	
	while(1)
	{	if(queue->next == NULL)
			break;
		else
			queue = queue->next;
	}
	
	/*queue = (SENS_LOG_STRUCT *)&sensSys->sysLog;
	while(1)
	{	if(queue->next == NULL)
		{	break;
		}
		queue = queue->next;
	}*/
	newQueue->buf = SENS_MEM_ZALLOC(len);
	if(newQueue->buf == NULL)
	{	SENS_MEM_FREE(newQueue);
		return NULL;
	}
	
	newQueue->prev = queue;
	queue->next = newQueue;
	memcpy(newQueue->buf, buf, len);
	
	queue = (SENS_LOG_STRUCT *)&sysCtrl->logDataArray;
	queue->count++;
	newQueue->len = len;
	newQueue->isValid = 1;
	return newQueue;
}
//------------------------------------------------------------------------------
static SENS_LOG_STRUCT *findOldestLogQueue(void)
{	SENS_LOG_STRUCT *topLog;
	SENS_LOG_STRUCT *queue;
	
	topLog = (SENS_LOG_STRUCT *)&sysCtrl->logDataArray;
	queue = topLog->next;
		
	if(queue != NULL)
	{	if(queue->isValid != 1)
		{	return NULL;
		}
	}
	return queue;
}
//------------------------------------------------------------------------------
static int removeLogQueue(SENS_LOG_STRUCT *removeQueue)
{	SENS_LOG_STRUCT *topLog;
	SENS_LOG_STRUCT *queue;
	SENS_LOG_STRUCT *next;
	
	topLog = (SENS_LOG_STRUCT *)&sysCtrl->logDataArray;
	
	SENS_MEM_FREE(removeQueue->buf);
	SENS_MEM_FREE(removeQueue->logFileName);
	queue = topLog;

	while(1)
	{	if(queue->next == removeQueue)
		{	break;
		}
		if(queue->next == NULL)
		{	return -1;
		}
		queue = queue->next;
	}
	queue->next = removeQueue->next;
	next = removeQueue->next;
	
	if(next != NULL)
	{	next->prev = removeQueue->prev;
	}
	topLog->count--;
	SENS_MEM_FREE(removeQueue);
	return 0;
}
#endif
//------------------------------------------------------------------------------
void writeLog(char *msg)
{	S_RTC_TIME_DATA_T currDateTime;
#if defined FILESYSTEM_USE_TASK
	struct TaskQMsg msgQ;
	SENS_LOG_STRUCT *queue;
	SENS_LOG_STRUCT *topLog;
	
	topLog = (SENS_LOG_STRUCT *)&sysCtrl->logDataArray;
	if(topLog->isTop == 0)
	{	topLog->isTop = 1;
		SENS_SEM_INIT(topLog->logLock, 1);
	}
	
	SENS_SEM_LOCK(topLog->logLock);
	queue = createNewLogQueue(msg, strlen(msg));
	if(queue == NULL)
	{	SENS_SEM_UNLOCK(topLog->logLock);
		return;
	}
	memcpy(&currDateTime, (void *)&sensSys->dateTime, sizeof(S_RTC_TIME_DATA_T));
	queue->logFileName = SENS_MEM_ZALLOC(MAX_FILE_PATH_LEN);
	SENS_SPRINTF(queue->logFileName, "LOG%02d/%d%02d%02d.log", currDateTime.u32Month, currDateTime.u32Year, currDateTime.u32Month, currDateTime.u32Day);
	
	if(sysCtrl->logIsReady)
	{	if(!topLog->isProcessing)
		{	msgQ.msgId = SD_Q_MSG_WRITE_LOG;
			msgQ.ptr = (char *)NULL;
			if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msgQ, 0), __func__, __LINE__))
			{	SENS_SEM_UNLOCK(topLog->logLock);
				return;
			}
			topLog->isProcessing = 1;
		}
	}
	
	SENS_SEM_UNLOCK(topLog->logLock);
#endif
	/*char filename[MAX_FILE_PATH_LEN];
	int writeRetLen;
	
	//getLogFileName();
	if(sensSys->dateTime.u32Year < VALID_YEAR)
		return;
	memcpy(&currDateTime, (void *)&sensSys->dateTime, sizeof(S_RTC_TIME_DATA_T));
	memset(filename, 0, MAX_FILE_PATH_LEN);
	SENS_SPRINTF(filename, "LOG%02d", currDateTime.u32Month);
	if(!createDir(filename))
	{	sysCtrl->isReadyToReboot = 1;
		return;
	}
	SENS_SPRINTF(filename+strlen(filename), "/%d%02d%02d.log", currDateTime.u32Year, currDateTime.u32Month, currDateTime.u32Day);
	writeRetLen = sdWriteFile(filename, msg, strlen(msg), 0, CONTINUE_WRITE_MODE);*/
}
//------------------------------------------------------------------------------
void clearOldestLog(void)
{	S_RTC_TIME_DATA_T currDateTime;
	int currMonth, delStartMonth, delEndMonth;
	char dirPath[20];
	
	memcpy(&currDateTime, (void *)&sensSys->dateTime, sizeof(S_RTC_TIME_DATA_T));
	currMonth = currDateTime.u32Month;
	
	if(currDateTime.u32Day != 1)
	{	//prev is clear done, set to 1 for DAY 1 to clr
		sysStsFlag2.sysStatusFlags = getVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG2);//GET_VBAT_REG_SYS_STS_FLAG();
		if(sysStsFlag2.logFileClearSts == 1)
		{	sysStsFlag2.logFileClearSts = 0;
			setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG2, sysStsFlag2.sysStatusFlags);
		}
		return;
	}
	else
	{	//already clear done, return
		sysStsFlag2.sysStatusFlags = getVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG2);
		if(sysStsFlag2.logFileClearSts == 1)
			return;
	}
	
	delStartMonth = currMonth+1;
	if(delStartMonth > 12)
		delStartMonth = 1;

	delEndMonth = currMonth;
	for(int idx=0;idx<4;idx++)	//Keep 3 Month log
	{	if(delEndMonth <= 0)
		{	delEndMonth = 12;
		}
		else
		{	delEndMonth--;
			if(delEndMonth <= 0)
				delEndMonth = 12;
		}
	}
	
	while(1)
	{	SENS_SPRINTF(dirPath, "LOG%02d", delStartMonth);
		//dbg_msg("delete Path %s\r\n", dirPath);
		SENS_FILE_DEL_ALL_FILE_IN_DIR(FS_TF, dirPath, 1);
		delStartMonth++;
		if(delStartMonth > 12)
			delStartMonth = 1;
		if(delStartMonth == delEndMonth)
			break;
	}
	sysStsFlag2.logFileClearSts = 1;
	setVBatRegValue(RTC_SPARE_REG_ITEM_SYS_STATUS_FLAG2, sysStsFlag2.sysStatusFlags);
}
//------------------------------------------------------------------------------
int iniParserFile(char *fileName, struct _MiniFile *mIniFile)
{	SD_FILE_CTRL *sdCtrl;
	struct TaskQMsg msg;
  int rsp;
	
	sdCtrl = SENS_MEM_ZALLOC(sizeof(SD_FILE_CTRL));
	filenameToAbsFilepath(sdCtrl, fileName);
	if(SENS_EVENT_CREATE(sdCtrl->lwevent, LWEVENT_AUTO_CLEAR) != MQX_OK )
  {
  }
  
  sdCtrl->mIniFile = mIniFile;
  
  msg.msgId = SD_Q_MSG_PARSER_INI_FILE;
	msg.ptr = (char *)sdCtrl;
	if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	clearSdCtrlVariable(sdCtrl);
		return -1;
	}
	rsp = waitSDProcessDone(sdCtrl, LW_EVENT_FILE_CHK_OPEN | LW_EVENT_FILE_PARSER_INI_FAIL, 6);
	clearSdCtrlVariable(sdCtrl);
  return rsp;
}
//------------------------------------------------------------------------------
int mIniSaveToTf(char *fileName, struct _MiniFile *mIniFile)
{	SD_FILE_CTRL *sdCtrl;
	struct TaskQMsg msg;
  int rsp;
	
	sdCtrl = SENS_MEM_ZALLOC(sizeof(SD_FILE_CTRL));
	filenameToAbsFilepath(sdCtrl, fileName);
	if(SENS_EVENT_CREATE(sdCtrl->lwevent, LWEVENT_AUTO_CLEAR) != MQX_OK )
  {
  }
  
  sdCtrl->mIniFile = mIniFile;
  
  msg.msgId = SD_Q_MSG_SAVE_INI_FILE;
	msg.ptr = (char *)sdCtrl;
	if(sendMsgWithErrHandle(SDCARD_TASK, SENS_MSG_Q_SEND(sdSendQ, (uint32_t *)&msg, 0), __func__, __LINE__))
	{	clearSdCtrlVariable(sdCtrl);
		return -1;
	}
	rsp = waitSDProcessDone(sdCtrl, LW_EVENT_FILE_CHK_OPEN | LW_EVENT_FILE_SAVE_INI_FAIL, 10);
	clearSdCtrlVariable(sdCtrl);
  return rsp;
}
//------------------------------------------------------------------------------
#ifdef FILESYSTEM_USE_TASK
static void sdTaskGetFileLength(SD_FILE_CTRL* sdFileCtrl)
{	SENS_FILE_PTR fd_ptr;
	int error;
	
	fd_ptr = SENS_FILE_OPEN(FS_TF, sdFileCtrl->fileName, "r", &error);
	if(error)
	{	setOpenFail(sdFileCtrl);
		return;
	}
	sdFileCtrl->fileLength = f_size(fd_ptr);
	SENS_FILE_CLOSE(fd_ptr);
	SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PROCESS_SUCCESS);
}
//------------------------------------------------------------------------------
static void sdTaskReadFile(SD_FILE_CTRL* sdFileCtrl)
{	SENS_FILE_PTR fd_ptr;
	int error;
	
	//dbgMsg("%s", "===1===\r\n");
	if( sdFileCtrl->rwMode == NORMAL_READ_MODE)
		fd_ptr = SENS_FILE_OPEN(FS_TF, sdFileCtrl->fileName, "r", &error);
	else if(sdFileCtrl->rwMode == BIN_READ_MODE)
		fd_ptr = SENS_FILE_OPEN(FS_TF, sdFileCtrl->fileName, "rb", &error);
	else if(sdFileCtrl->rwMode == BIN_READ_MODE2)
		fd_ptr = SENS_FILE_OPEN(FS_TF, sdFileCtrl->fileName, "r+", &error);
	//dbgMsg("%s", "===2===\r\n");
	if(error)
	{	setOpenFail(sdFileCtrl);
		return;
	}
	
	//dbgMsg("%s", "===3===\r\n");
	SENS_FILE_SEEK(fd_ptr, sdFileCtrl->offset, IO_SEEK_SET);
	//dbgMsg("%s", "===4===\r\n");
	SENS_FILE_READ(fd_ptr, (uint8_t *)sdFileCtrl->buffer, sdFileCtrl->rwLength, &sdFileCtrl->realRwLength);
	//dbgMsg("%s", "===5===\r\n");
	SENS_FILE_CLOSE(fd_ptr);
	//dbgMsg("%s", "===6===\r\n");
	if(sdFileCtrl->realRwLength != sdFileCtrl->rwLength)
	{	setReadFail(sdFileCtrl);
		return;
	}
	
	SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PROCESS_SUCCESS);
	//dbgMsg("%s", "===7===\r\n");
}
//------------------------------------------------------------------------------
static void sdTaskWriteFile(SD_FILE_CTRL* sdFileCtrl)
{	SENS_FILE_PTR fd_ptr;
	int error;

#ifndef REC_INTERVAL_FIXED
	if(sdFileCtrl->rwMode == WRITE_NAN_DATA_MODE)
	{	int chunkSize = 8192;
		int remainSize = sdFileCtrl->fileLength;
		int bytesWritten, writeError=0;
		if(sdFileCtrl->buffer == NULL)
		{	sdFileCtrl->buffer = SENS_MEM_ALLOC(chunkSize);
			memset(sdFileCtrl->buffer, 0xFF, chunkSize);
		}
		fd_ptr = SENS_FILE_OPEN(FS_TF, sdFileCtrl->fileName, "w", &error);
		while(remainSize)
		{	if(remainSize < chunkSize)
			{	chunkSize = remainSize;
			}
			SENS_FILE_WRITE(fd_ptr, (uint8_t *)sdFileCtrl->buffer, chunkSize, &bytesWritten);
			if(bytesWritten != chunkSize)
			{	writeError = 1;
				break;
			}
			remainSize -= chunkSize;
		}
		SENS_FILE_CLOSE(fd_ptr);
		if(writeError)
		{	setWriteFail(sdFileCtrl);
			return;
		}
	}
	else
#endif
	{	fd_ptr = SENS_FILE_OPEN(FS_TF, sdFileCtrl->fileName, "w", &error);	//internal auto create file
	
		if(sdFileCtrl->rwMode == OVER_WRITE_MODE)
		{	SENS_FILE_SEEK(fd_ptr, sdFileCtrl->offset, IO_SEEK_SET);
		}
		else if(sdFileCtrl->rwMode == CONTINUE_WRITE_MODE)
		{	sdFileCtrl->offset = f_size(fd_ptr);
			SENS_FILE_SEEK(fd_ptr, sdFileCtrl->offset, IO_SEEK_SET);
		}
		SENS_FILE_WRITE(fd_ptr, (uint8_t *)sdFileCtrl->buffer, sdFileCtrl->rwLength, &sdFileCtrl->realRwLength);
		SENS_FILE_CLOSE(fd_ptr);
		if(sdFileCtrl->realRwLength != sdFileCtrl->rwLength)
		{	setWriteFail(sdFileCtrl);
			return;
		}
	}
	SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PROCESS_SUCCESS);
}
//------------------------------------------------------------------------------
static int8_t isExistDirInternal(char *absDirName)
{	char path[MAX_FILE_PATH_LEN];
	
	nameToAbsolutePath(FS_TF, absDirName, path);
	return (SENS_FILE_DIR_EXIST(path) == 0);
}
//------------------------------------------------------------------------------
static int8_t createDirInternal(char *absDirName)
{	if(isExistDirInternal(absDirName))
		return 1;
	return (SENS_FILE_DIR_CREATE(absDirName) == 0);
}
//------------------------------------------------------------------------------
static int isFileExistInternal(char *filename)
{	SENS_FILE_PTR fd_ptr;
	int error;
	
	fd_ptr = SENS_FILE_OPEN(FS_TF, filename, "r", &error);
	if(error)
	{	if(fd_ptr)
			SENS_MEM_FREE(fd_ptr);
		//fsUnlock();	//need check
		return 0;
	}
	SENS_FILE_CLOSE(fd_ptr);
	return 1;
}
//------------------------------------------------------------------------------
static void sdTaskCreateImgFile(SD_FILE_CTRL *sdFileCtrl)
{	int error_code;
	SENS_FILE_PTR fd_ptr;
	int error;
	char path[20] = {0};
	SENS_FILE_PTR file;

	strcat(path, "DCIM");
	if(!createDirInternal(path))
	{	
#if N_TEMP_REMOVE
		sysCtrl->isReadyToReboot = 1;
#endif
		SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_CREATE_FILE_FAIL);
		return;
	}
	
	if(!isFileExistInternal(sdFileCtrl->fileName))
	{	fd_ptr = SENS_FILE_OPEN(FS_TF, sdFileCtrl->fileName, "a", &error);
		if(!error)
			SENS_FILE_CLOSE(fd_ptr);
	}
	else
	{	error_code = SENS_FILE_DELETE(FS_TF, sdFileCtrl->fileName);
		//error_code = ioctl(g_fs, IO_IOCTL_DELETE_FILE, sdCtrl->fileName);
		if(error_code != MQX_OK)
		{	SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_CREATE_FILE_FAIL);
			return;
		}
		else
		{	fd_ptr = SENS_FILE_OPEN(FS_TF, sdFileCtrl->fileName, "a", &error);
			SENS_FILE_CLOSE(fd_ptr);
		}
	}
	SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PROCESS_SUCCESS);
}
//------------------------------------------------------------------------------
static void writeLogProcess(void)
{	SENS_LOG_STRUCT *topLog;
	SENS_LOG_STRUCT *queue;
	uint32_t logQueueCnt;
	int idx;
	char *pos;
	int startMonth;
	SENS_FILE_PTR fd_ptr;
	char dirName[10];
	int error;
	
	topLog = (SENS_LOG_STRUCT *)&sysCtrl->logDataArray;
	SENS_SEM_LOCK(topLog->logLock);
	
	logQueueCnt = topLog->count;
	if(logQueueCnt == 0)
	{	topLog->isProcessing = 0;
		SENS_SEM_UNLOCK(topLog->logLock);
		return;
	}
	
	sdWriteLock();
	clearOldestLog();
	
	for(idx=0;idx<logQueueCnt;idx++)
	{	queue = findOldestLogQueue();
		if(queue != NULL)
		{	pos = strstr(queue->logFileName, "LOG");
			SENS_SSCANF(pos, "LOG%d", &startMonth);
			memset(dirName, 0, 10);
			SENS_SPRINTF(dirName, "%sLOG%02d", MSD_TF, startMonth);
			if(!isExistDirInternal(dirName))
			{	if(!createDirInternal(dirName))
				{	//break;
					SENS_MEM_FREE(dirName);
					removeLogQueue(queue);
					continue;
				}
			}
			if(!isFileExistInternal(queue->logFileName))
			{	fd_ptr = SENS_FILE_OPEN(FS_TF, queue->logFileName, "a", &error);
				SENS_FILE_CLOSE(fd_ptr);
			}
			fd_ptr = SENS_FILE_OPEN(FS_TF, queue->logFileName, "w", &error);
			if(error)
			{	removeLogQueue(queue);
				continue;
			}
			SENS_FILE_SEEK(fd_ptr, 0, IO_SEEK_END);
			SENS_FILE_WRITE(fd_ptr, queue->buf, queue->len, NULL);
			SENS_FILE_CLOSE(fd_ptr);
			removeLogQueue(queue);
		}
	}
	topLog->isProcessing = 0;
	SENS_SEM_UNLOCK(topLog->logLock);
	sdWriteUnlock();
}
//------------------------------------------------------------------------------
void sdcardTask(void *param)
{	//if only detect function, don't create
	struct TaskQMsg msg;
	SD_FILE_CTRL *sdFileCtrl;
		
	if(MQX_OK != SENS_MSG_Q_INIT(sdSendQ, SDTASK_NUM_MESSAGES, SENS_TASKQ_GRANM))
	{	return ;
  }
	while(1)
	{	//if(!(SDH_CardDetection(SDH0)))
		SENS_MSG_Q_RECV(sdSendQ, (uint32_t *)&msg, LWMSGQ_RECEIVE_BLOCK_ON_EMPTY, 0, 0);
		switch(msg.msgId)
		{	case SD_Q_MSG_GET_FILE_LENGTH:
						{	sdFileCtrl = (SD_FILE_CTRL *)msg.ptr;
							sdTaskGetFileLength(sdFileCtrl);
						}
						break;
			case SD_Q_MSG_READ:
						{	sdFileCtrl = (SD_FILE_CTRL *)msg.ptr;
							sdTaskReadFile(sdFileCtrl);
						}
						break;
			case SD_Q_MSG_WRITE:
						{	sdFileCtrl = (SD_FILE_CTRL *)msg.ptr;
							sdWriteLock();
							sdTaskWriteFile(sdFileCtrl);
							sdWriteUnlock();
						}
						break;
			case SD_Q_MSG_CREATE_DIR:
						{	sdFileCtrl = (SD_FILE_CTRL *)msg.ptr;
							
							sdWriteLock();
							if(!isExistDirInternal(sdFileCtrl->fileName))
							{	if(!createDirInternal(sdFileCtrl->fileName))
								{	SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_OPEN_FAIL);
								}
							}
							sdWriteUnlock();
							SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PROCESS_SUCCESS);
						}
						break;
			case SD_Q_MSG_CHK_FILE_EXIST:
						{	sdFileCtrl = (SD_FILE_CTRL *)msg.ptr;
							
							if(isFileExistInternal(sdFileCtrl->fileName))
								SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PROCESS_SUCCESS);
							else
								SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_OPEN_FAIL);
						}
						break;
			case SD_Q_MSG_DELETE_FILE:
						{	sdFileCtrl = (SD_FILE_CTRL *)msg.ptr;
							int error_code;
							
							sdWriteLock();
							if(isFileExistInternal(sdFileCtrl->fileName))
							{	error_code = SENS_FILE_DELETE(FS_TF, sdFileCtrl->fileName);
								//error_code = ioctl(g_fs, IO_IOCTL_DELETE_FILE, sdCtrl->fileName);
								if(error_code != MQX_OK)
								{	SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_DELETE_FAIL);
									sdWriteUnlock();
									break;
								}
							}
							sdWriteUnlock();
							SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PROCESS_SUCCESS);
						}
						break;
			case SD_Q_MSG_CREATE_JPG_FILE:
						{	sdFileCtrl = (SD_FILE_CTRL *)msg.ptr;
							sdWriteLock();
							sdTaskCreateImgFile(sdFileCtrl);
							sdWriteUnlock();
						}
						break;
			case SD_Q_MSG_PARSER_INI_FILE:
						{	int rsp;
							sdFileCtrl = (SD_FILE_CTRL *)msg.ptr;
							rsp = mIniParserFile(sdFileCtrl->fileName, sdFileCtrl->mIniFile);
							if(rsp == 0)
								SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PROCESS_SUCCESS);
							else
								SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PARSER_INI_FAIL);
						}
						break;
			case SD_Q_MSG_SAVE_INI_FILE:
						{	int rsp;
							sdFileCtrl = (SD_FILE_CTRL *)msg.ptr;
							sdWriteLock();
							rsp = mIniSave(sdFileCtrl->fileName, sdFileCtrl->mIniFile);
							sdWriteUnlock();
							if(rsp == 1)
								SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_PROCESS_SUCCESS);
							else
								SENS_EVENT_SET(sdFileCtrl->lwevent, LW_EVENT_FILE_SAVE_INI_FAIL);
						}
						break;
			case SD_Q_MSG_WRITE_LOG:
						writeLogProcess();
						break;
		}
		//SENS_TIME_DELAY(1000);
	}
}
#endif