#include "network/netApp.h"
#include "sensCommon.h"
#include "sensSystem.h"
#include "sensDev.h"
#include "powerCtrl.h"
#include "rtcDateTime.h"
#include "physicalQuantity.h"
#include "systemTimer.h"

#if SUPPORT_NUVOTON_USB_HOST
#include "usb.h"
#include "hub.h"
#include "usbh_lib.h"
#include "usbh_cdc.h"
#include "usbHost/usbHostTask.h"
#endif

IOT_SYSTEM iotSys;

#define AT_CMD_MAX_SIZE					256
#define MOBILE_READY_TIMEOUT_CNT		20
#define MOBILE_READY_R410_TIMEOUT_CNT	4

char atCmdStr[AT_CMD_MAX_SIZE];

//------------------------------------------------------------------------------
void displayCrrentSendAtCmd(uint8_t send, char *str, char const *func, int line)
{	debugLock();
	dbg_msg1("[%d/%02d/%02d %02d:%02d:%02d]", sensSys->dateTime.u32Year, sensSys->dateTime.u32Month ,sensSys->dateTime.u32Day ,sensSys->dateTime.u32Hour, sensSys->dateTime.u32Minute, sensSys->dateTime.u32Second);
#if AT_CMD_DISPLAY_FUNC_LINE
	dbg_msg1("%s(%d), %s %s\r\n", func, line, send? networkCtx->workingInst->sendStr : networkCtx->workingInst->recvStr, str);
#else
	dbg_msg1("%s %s\r\n", send? networkCtx->workingInst->sendStr : networkCtx->workingInst->recvStr, str);
#endif
	debugUnlock();
}
//------------------------------------------------------------------------------
static int processUnknownURC(char *buf)
{	char *pos;
	
	pos = strstr(buf, "+CEREG: ");
	if(pos)
	{	return 0;
	}
	/*pos = strstr(buf, "ERROR");
	if(pos)
	{	return -99;
	}*/
	
	return -1;
}
//------------------------------------------------------------------------------
static int waitReceive(int timeInSec, AT_CMD_RECV_BACK_BUF **atCmdRecvBakBuf, MOBILE_INSTANCE	*wlInst, char const *func, int line)
{	//_mqx_uint mask, result;
	struct TaskQMsg msg;
	//int rty=0;
	uint32_t result;
	
	int startTime = GetTimeTicks();
	int endTime;
	
GET_QUEUE_MSG:
	result = SENS_MSG_Q_RECV(networkProcessQ, (uint32_t *)&msg, LWMSGQ_RECEIVE_BLOCK_ON_EMPTY, 200*timeInSec, 0);
	if(result == SYS_RET_OK)
	{	if(msg.msgId == NETWORK_Q_MSG_MOBILE_AT_CMD_NORMAL_RSP)
		{	*atCmdRecvBakBuf = (AT_CMD_RECV_BACK_BUF *)msg.ptr;
			//dbg_msg("%s, wait done\r\n", __func__);
			return NW_INIT_ERR_OK;
		}
		else	//re-queue must delay
		{	
			//SENS_SEM_LOCK(mobileSys.mobileSem);
			SENS_TIME_DELAY(20);
			if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
			{	
			}
			//SENS_SEM_UNLOCK(mobileSys.mobileSem);
			endTime = GetTimeTicks();
			if((endTime - startTime) > (timeInSec * 1000))
			{	AT_CMD_RECV *atCmdRecv = (AT_CMD_RECV *)&iotSys.atCmdRecv;
				dbg_msg("total time over %d sec, call from %s(%d)\r\n", timeInSec, func, line);
#ifdef OS_MQX
				dbg_msg("atCmdRecv->pos:%d, sema value:%d, uart sts:%d, uart ptr:%p, run cnt:%d\r\n", atCmdRecv->receivePos, mobileSys.mobileSem.VALUE, HAL_UART_STATUS(wlInst->dev), wlInst->dev, mobileSys.runningCount);
#endif
				if(atCmdRecv->receivePos)
				{	displayBufInHex((unsigned char *)atCmdRecv->receiveBuf, atCmdRecv->receivePos, __func__, __LINE__);
				}
				return NW_INIT_ERR_AT_RSP_TIMEOUT;
			}
			if(sysCtrl->isReadyToReboot)
			{	return NW_INIT_ERR_SYS_REBOOT;
			}
			
			goto GET_QUEUE_MSG;
		}
	}
	else 
	{	//UART_CONFIG *uartCfg;
		//UART_CONFIG	*lsComm = (UART_CONFIG	*)sensDev->lsComm;
		//UART_CTX *uartCtx = lsComm->ctx;
		//UART_RX_CTX	*rxCtx = uartCtx->rxCtx;
		
		AT_CMD_RECV *atCmdRecv = (AT_CMD_RECV *)&iotSys.atCmdRecv;
		dbg_msg("total time over %d sec, call from %s(%d)\r\n", timeInSec, func, line);
#ifdef OS_MQX
		dbg_msg("atCmdRecv->pos:%d, sema value:%d, uart sts:%d, uart ptr:%p, run cnt:%d\r\n", atCmdRecv->receivePos, mobileSys.mobileSem.VALUE, HAL_UART_STATUS(wlInst->dev), wlInst->dev, mobileSys.runningCount);
#endif
		if(atCmdRecv->receivePos)
		{	displayBufInHex((unsigned char *)atCmdRecv->receiveBuf, atCmdRecv->receivePos, __func__, __LINE__);
		}
		
		//dbg_msg("%s(%d), wait Receive Timeout, call from %s(%d)\r\n", __func__, __LINE__, func, line);
	}
	
	return NW_INIT_ERR_AT_RSP_TIMEOUT;
}
//------------------------------------------------------------------------------
static int waitAtCmdRsp(int timeout, char *str, uint8_t ignoreOtherCmdRsp, int waitStrType, uint8_t keepBuf, char **buf, MOBILE_INSTANCE *wlInst, char const *func, int line)
{	char *strPos;
	int rsp;
	AT_CMD_RECV_BACK_BUF *atCmdRecvBakBuf;
		
	rsp = waitReceive(timeout, &atCmdRecvBakBuf, wlInst, func, line);
	if(rsp == NW_INIT_ERR_OK)
	{	displayCrrentSendAtCmd(0, (char *)atCmdRecvBakBuf->buf, __func__, __LINE__);
		if(waitStrType == WAIT_STR_TYPE_SPECIFY)
			strPos = strstr((char *)atCmdRecvBakBuf->buf, str);
		else if(waitStrType == WAIT_STR_TYPE_OK)
			strPos = strstr((char *)atCmdRecvBakBuf->buf, "OK");
		if(strPos)
		{	if(!keepBuf)
			{	SENS_MEM_FREE(atCmdRecvBakBuf->buf);
				SENS_MEM_FREE(atCmdRecvBakBuf);
			}
			else
			{	*buf = (char *)atCmdRecvBakBuf->buf;
				SENS_MEM_FREE(atCmdRecvBakBuf);
			}
			return NW_INIT_ERR_OK;
		}
		else
		{	if((ignoreOtherCmdRsp == IGNORE_OTHER_AT_CMD_RSP) || !keepBuf)
			{	SENS_MEM_FREE(atCmdRecvBakBuf->buf);
				SENS_MEM_FREE(atCmdRecvBakBuf);
			}
			else
			{	*buf = (char *)atCmdRecvBakBuf->buf;
				SENS_MEM_FREE(atCmdRecvBakBuf);
			}
			if(ignoreOtherCmdRsp == IGNORE_OTHER_AT_CMD_RSP)
				return NW_INIT_ERR_AT_RSP_TIMEOUT;
			else
				return NW_INIT_ERR_RSP_OTHER_CMD;
		}
	}
	return rsp;
}
//------------------------------------------------------------------------------
void iotSendCmd(MOBILE_INSTANCE *wlInst, char *buf)
{	uint32_t len;
	len = strlen(buf);
#if defined SUPPORT_PPP
	#if SUPPORT_NUVOTON_USB_HOST
	CDC_DEV_T *cdev;
	USB_COMPOSITE_DEVICE_IF_CTX *usbDevIf;
	MOBILE_USB_INSTANCE *mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
	
	if(mobUsbInst->isUsbHost)
	{	if(mobUsbInst->usbIsCdcAcm)
		{	if(mobUsbInst->usbIsMultiIfDev)
			{	if(wlInst->usePPP)
				{	cdev = (CDC_DEV_T *)wlInst->usbDevAt2;
					usbh_cdc_send_data(cdev, (uint8_t *)buf, len);
					return;
				}
				else
				{	cdev = (CDC_DEV_T *)wlInst->usbDev;
					usbh_cdc_send_data(cdev, (uint8_t *)buf, len);
					return;
				}
			}
			else
			{	cdev = (CDC_DEV_T *)wlInst->usbDev;
				usbh_cdc_send_data(cdev, (uint8_t *)buf, len);
				return;
			}
		}
		else
		{	if(mobUsbInst->usbIsMultiIfDev)
			{	if(wlInst->usePPP)
				{	usbDevIf = (USB_COMPOSITE_DEVICE_IF_CTX *)wlInst->usbDevAt2;
					usbhVendorBulkWrite(usbDevIf, (uint8_t *)buf, len);
					return;
				}
				else
				{	usbDevIf = (USB_COMPOSITE_DEVICE_IF_CTX *)wlInst->usbDev;
					usbhVendorBulkWrite(usbDevIf, (uint8_t *)buf, len);
					return;
				}
			}
			else
			{	usbDevIf = (USB_COMPOSITE_DEVICE_IF_CTX *)wlInst->usbDev;
				usbhVendorBulkWrite(usbDevIf, (uint8_t *)buf, len);
				return;
			}
		}
	}
	#endif
	if(!wlInst->isNotSupportCmuxMode)
	{	SENS_UART_WRITE(wlInst->dev, (uint8_t *)buf, len);
	}
	else if(!wlInst->usePPP)
		SENS_UART_WRITE(wlInst->dev, (uint8_t *)buf, len);
#else
	SENS_UART_WRITE(wlInst->dev, (uint8_t *)buf, len);
#endif
}
//------------------------------------------------------------------------------
static int waitMobileReady(MOBILE_INSTANCE *wlInst, int mode)
{	int retryCnt=0;
	int rsp;
	int timeout;
	char *buf = NULL;
	NET_INSTANCE *netInst = (NET_INSTANCE *)wlInst->upperCtx;
	int	timer=150;
#if SUPPORT_NUVOTON_USB_HOST
	MOBILE_USB_INSTANCE *mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
#endif
	
#ifdef PLATFORM_FSL
	if(mode == 0)
	{	if(wlInst->flowCtrl == FC_HW)
		{	while(timer)
			{	SENS_TIME_DELAY(400);
				extendTaskTimer(&taskAct.wireLanSendTask);
	#if SUPPORT_USB
				if(wlInst->isUsbMode && wlInst->usbUp)
				{	dbg_msg("%s", "USB mobile is Ready!!\r\n");
					wlInst->flowCtrl = FC_NONE;
					netInst->isUsbModule = 1;
					break;
				}
	#endif
				//if((SENS_GPIO_GET_VALUE(&gprs_cts) == 0) && (wlInst->module != MOBILE_TYPE_USB))
				if((SENS_GPIO_GET_VALUE(&gprs_cts) == 0) && SENS_GPIO_GET_VALUE(&uart_gprs_rx))
  			{	//return NW_INIT_ERR_CONTINUE;
  				if(netInst->netType != NETWORK_TYPE_HS)
	  			{	dbg_msg("%s", "mobile is install at LS!!\r\n");
	  				break;
	  			}
	  			else if(!netInst->isUsbModule)
	  			{	dbg_msg("%s", "mobile is not USB interface!!\r\n");
	  				break;
	  			}
  			}
  			timer--;
  			if(sensSys->IsReboot)
	  			return -1;
  		}
  		dbg_msg("CTS status:%d\r\n", SENS_GPIO_GET_VALUE(&gprs_cts));
		}
#if SUPPORT_USB
		else
		{	if((netInst->netType == NETWORK_TYPE_HS) && netInst->isUsbModule)
			{	timer = 100;
        while(timer)
				{	SENS_TIME_DELAY(400);
					extendTaskTimer(&taskAct.wireLanSendTask);
					if(wlInst->isUsbMode && wlInst->usbUp)
					{	wlInst->flowCtrl = FC_NONE;
						netInst->isUsbModule = 1;
						break;
					}
					timer--;
					if(sensSys->IsReboot)
		  			return -1;
				}
			}
		}
#endif
	}
#endif

#ifdef SENSMINIS2
	if(mode == 0)
	{	if(wlInst->flowCtrl == FC_HW)
		{	while(timer)
			{	if(getUartCtsSts(wlInst->dev) == 0)
				{
					break;
				}
				SENS_TIME_DELAY(400);
				timer--;
				if(sysCtrl->isReadyToReboot)
					return -1;
				if(mobUsbInst->isUsbHost && sysParams->useReyaxUsb)
					break;
			}
			dbg_msg("CTS status:%d\r\n", getUartCtsSts(wlInst->dev));
		}
	}
#endif
#ifdef SENSMINIA4_NEO
	while(1)
	{	if(mobUsbInst->isUsbHost)
		{	break;
		}
		SENS_TIME_DELAY(400);
		timer--;
	}
	if(!mobUsbInst->isUsbHost)
	{	dbgMsg("%s", "NO FS-USB INSERT\r\n");
		return -1;
	}
#endif
	SENS_SPRINTF(atCmdStr, "AT\r" EMPTY_CHAR);
	
#ifdef SENSMINIS2
#if SUPPORT_NUVOTON_USB_HOST
	if(!mobUsbInst->isUsbHost)
	{	PCIE_USB_ON = 0;
		TaskHandle_t handle = NULL;
		handle = xTaskGetHandle("USB_HOST_TASK");
		if(handle)
			vTaskDelete(handle);
	}
#endif
#endif
	while(1)
	{	extendTaskTimer((TASK_INFO *)&taskAct.networkProcessTaskAct);
		displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		SENS_SEM_LOCK(iotSys.iotSem);
		iotSendCmd(wlInst, atCmdStr);
		SENS_SEM_UNLOCK(iotSys.iotSem);
#if defined PLATFORM_FSL || !SUPPORT_NUVOTON_USB_HOST
		if((wlInst->flowCtrl == FC_HW) && (mode ==0))
			timeout = 50;
		else
#endif
			timeout = 2;
		rsp = waitAtCmdRsp(timeout, "AT", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(!rsp)
		{	break;
		}
		else
		{	if(buf)
			{	if(!memcmp(buf, "OK", 2))
				{	SENS_MEM_FREE(buf);
					return 0;
				}
			}
			if(rsp == NW_INIT_ERR_RSP_OTHER_CMD)
			{	//freeReceiveTempBuf();
				return NW_INIT_ERR_AT_RSP_TIMEOUT;
			}
			else
			{	if(
#if !SUPPORT_NUVOTON_USB_HOST
					(
#endif
					mode == 0
#if !SUPPORT_NUVOTON_USB_HOST
					)
					&& (wlInst->flowCtrl != FC_HW)
#endif
				)
				{	retryCnt++;
					if(retryCnt > MOBILE_READY_TIMEOUT_CNT)	//30 sec
						return NW_INIT_ERR_AT_RSP_TIMEOUT;
					if(((retryCnt % 5 ) == MOBILE_READY_R410_TIMEOUT_CNT))
					{	if(wlInst->module == MOBILE_TYPE_AUTO || wlInst->module == MOBILE_TYPE_R410)
						{	if(wlInst->baudrate == 115200)
							{	wlInst->baudrate = 19200;
								dbg_msg("%s, change Baudrate to 19200\r\n", __func__);
#if defined PLATFORM_XILINX
								setBaudrate(wlInst->dev, wlInst->baudrate);
#else
								setBaudRate(wlInst->dev, wlInst->baudrate);
#endif
								SENS_TIME_DELAY(50);
							}
							else if(wlInst->baudrate == 19200)
							{	//change baudrate
								wlInst->baudrate = 115200;
								dbg_msg("%s, change Baudrate to 115200\r\n", __func__);
#if defined PLATFORM_XILINX
								setBaudrate(wlInst->dev, wlInst->baudrate);
#else
								setBaudRate(wlInst->dev, wlInst->baudrate);
#endif
								SENS_TIME_DELAY(50);
							}
						}
					}
				}
				else if(mode == 1)
				{	retryCnt++;
					if(retryCnt > MOBILE_READY_TIMEOUT_CNT)	//30 sec
						return NW_INIT_ERR_AT_RSP_TIMEOUT;
				}
#if SUPPORT_NUVOTON_USB_HOST
				else if(mobUsbInst->isUsbHost)
				{	retryCnt++;
					if(retryCnt > MOBILE_READY_TIMEOUT_CNT)	//30 sec
						return NW_INIT_ERR_AT_RSP_TIMEOUT;
				}
#endif
				else
				{	return NW_INIT_ERR_AT_RSP_TIMEOUT;
				}
			}
		}
	}
	
	return waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
}
//------------------------------------------------------------------------------
static int disableEcho(MOBILE_INSTANCE *wlInst)
{	int rsp;
	int failRtyCnt=2;
	char *buf=NULL;
	char *pos;
	
	//memset(atCmdStr, 0, AT_CMD_MAX_SIZE);
	SENS_SPRINTF(atCmdStr, "ATE0\r" EMPTY_CHAR);
	
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		
		rsp = waitAtCmdRsp(2, "ATE0", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		//rsp = waitAtCmdRsp(100, "ATE0", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(rsp)
		{	pos = strstr(buf, "OK");
			if(pos)
			{	SENS_MEM_FREE(buf);
				return 0;
			}
			pos = strstr(buf, "ERROR");
			if(pos)
			{	SENS_MEM_FREE(buf);
				return 0;
			}
			if(rsp == NW_INIT_ERR_RSP_OTHER_CMD)
			{	//freeReceiveTempBuf();
				return NW_INIT_ERR_AT_RSP_TIMEOUT;
			}
			else
			{	return NW_INIT_ERR_AT_RSP_TIMEOUT;
			}
		}
		if(buf)
		{	SENS_MEM_FREE(buf);
			buf = NULL;
		}
		rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
			break;
		failRtyCnt--;
		SENS_TIME_DELAY(200);
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int getModule(MOBILE_INSTANCE *wlInst)
{	int failRtyCnt = 4;
	char *buf = NULL;
	char changeFlowCtrl = 0;
	int rsp;
	char unknownModule = 0;
	NET_INSTANCE *netInst;
	NET_CONFIG	*netCfg;
	WIRELESS_CONFIG	*wirelessCfg;
	MOBILE_CONFIG	*mobileCfg;
	
	netInst = networkCtx->workingInst;
  
	SENS_SPRINTF(atCmdStr, "AT+CGMM\r" EMPTY_CHAR);
	
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		
		rsp = waitAtCmdRsp(4, "OK", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(rsp)
		{	if(buf)
			{	unknownModule = 0;
				//wlInst->lteMdvpnType = LTE_MDVPN_TYPE_NONE;
				if(!memcmp(buf, "TOBY-L210", 9))
				{	
#ifdef SENSMINIS2
					cfgPcieIOPwr(PCIE_IO_PWR_1V8);
#endif
					wlInst->module = MOBILE_TYPE_L210;
					wlInst->atType = MOBILE_AT_TYPE_UBLOX;
					wlInst->initType = MOBILE_CELL_TYPE_LTE;
					//wlInst->initType = MOBILE_CELL_NBIOT;
					wlInst->mobileCellType = MOBILE_CELL_TYPE_LTE;
					//wlInst->lteMdvpnType = sensSys->netParam.lteMdvpnType;
				}
				else if(!memcmp(buf, "SARA-R410", 9))
				{	wlInst->module = MOBILE_TYPE_R410;
					wlInst->atType = MOBILE_AT_TYPE_UBLOX;
					wlInst->initType = MOBILE_CELL_TYPE_NBIOT;
					wlInst->mobileCellType = MOBILE_CELL_TYPE_NBIOT;
				}
				else if((!memcmp(buf, "ME310G1-WW", 10)) || (!memcmp(buf, "ME910G1-WW", 10)))
				{	wlInst->module = MOBILE_TYPE_ME310G;
					wlInst->atType = MOBILE_AT_TYPE_TELIT;
					wlInst->initType = MOBILE_CELL_TYPE_NBIOT;
					wlInst->mobileCellType = MOBILE_CELL_TYPE_NBIOT;
					changeFlowCtrl = 0;
#if SUPPORT_AGPS
					wlInst->supportAgps = 1;
#endif
				}
				else if(!memcmp(buf, "LE910C1-AP", 10) || !memcmp(buf, "LE910C4-AP", 10))
				{	wlInst->module = MOBILE_TYPE_LE910C1;
					wlInst->atType = MOBILE_AT_TYPE_TELIT;
					wlInst->mobileCellType = MOBILE_CELL_TYPE_LTE;
					wlInst->initType = MOBILE_CELL_TYPE_NBIOT;
					//wlInst->lteMdvpnType = sensSys->netParam.lteMdvpnType;
				}
				else if(!memcmp(buf, "LE910C4-WWX", 11) || !memcmp(buf, "LE910C1-WWX", 11))
				{	wlInst->module = MOBILE_TYPE_LE910_WWX;
					wlInst->atType = MOBILE_AT_TYPE_TELIT;
					wlInst->mobileCellType = MOBILE_CELL_TYPE_LTE;
					wlInst->initType = MOBILE_CELL_TYPE_NBIOT;
					//wlInst->lteMdvpnType = sensSys->netParam.lteMdvpnType;
#if SUPPORT_AGPS
					wlInst->supportAgps = 1;
#endif
				}
				else
				{	unknownModule = 1;
				}
				if(wlInst->mobileCellType == MOBILE_CELL_TYPE_LTE)
				{	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_NB_STS, sensPq->onboardPq[ON_BOARD_PQ_NB_STS]);
					putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_NB_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH]);
				}
				else if(wlInst->mobileCellType == MOBILE_CELL_TYPE_NBIOT)
				{	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LTE_STS, sensPq->onboardPq[ON_BOARD_PQ_LTE_STS]);
					putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LTE_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH]);
				}
				
				SENS_MEM_FREE(buf);
				buf = NULL;
				rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
				if(!rsp)
				{	if(!unknownModule)
					{	if(changeFlowCtrl)
						{	dbg_msg("%s", "[Network] Change to No Flow Ctrl\r\n");
							setUartFlowControl(wlInst->dev, 0);
						}
						netCfg = (NET_CONFIG *)&sysCfg->netCfg;
						wirelessCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
						if(wlInst->mobileCellType == MOBILE_CELL_TYPE_NBIOT)
						{	mobileCfg = (MOBILE_CONFIG *)wirelessCfg->nbCfg;
							memset(netInst->sendStr, 0x00, 16);
							memset(netInst->recvStr, 0x00, 16);
							memset(netInst->stsStr, 0x00, 16);
							
							strcpy(netInst->sendStr, NB_SEND_STR);
							strcpy(netInst->recvStr, NB_RECV_STR);
							strcpy(netInst->stsStr, NB_STATUS_STR);
							
							memset(wlInst->apn, 0x00, 32);
							//memset(wlInst->plmn, 0x00, 6);
							memcpy(wlInst->apn, (char *)mobileCfg->apn, strlen(mobileCfg->apn));
							//memcpy(wlInst->plmn, (char *)mobileCfg->plmn, strlen(wlInst->plmn));
							wlInst->plmn = mobileCfg->plmn;
						}
						else
						{	mobileCfg = (MOBILE_CONFIG *)wirelessCfg->lteCfg;
							memset(wlInst->apn, 0x00, 32);
							memset(wlInst->simAccount, 0x00, 32);
							memset(wlInst->simPass, 0x00, 32);
							
							memcpy(wlInst->apn, mobileCfg->apn, strlen(mobileCfg->apn));
							if(mobileCfg->simAccount)
								memcpy(wlInst->simAccount, mobileCfg->simAccount, strlen(mobileCfg->simAccount));
							if(mobileCfg->simPassword)
								memcpy(wlInst->simPass, mobileCfg->simPassword, strlen(mobileCfg->simPassword));
							wlInst->simAuthType = mobileCfg->simAuth;
						}
						return 0;
					}
					else
						failRtyCnt--;
				}
			}
			else
			{	failRtyCnt--;
			}
		}
	}
	
	return rsp;
}
//------------------------------------------------------------------------------
static int getSimPinStatus(MOBILE_INSTANCE *wlInst)
{	int rsp;
	//int rsp1;
	int failRtyCnt=2;
	char *buf = NULL;
	
	SENS_SPRINTF(atCmdStr, "AT+CPIN?\r" EMPTY_CHAR);
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		//rsp = waitAtCmdRsp(2, "+CPIN: READY", IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 0, NULL, wlInst, __func__, __LINE__);
		rsp = waitAtCmdRsp(2, "+CPIN: READY", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(!rsp)
		{	SENS_MEM_FREE(buf);
			break;
		}
		else
		{	if(buf)
			{	SENS_MEM_FREE(buf);
			}
		}
		//rsp1 = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		//rsp |= rsp1;
		//if(!rsp)
		//	break;
		failRtyCnt--;
	}
	return waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);;
}
//------------------------------------------------------------------------------
static int setSwFlowControl(int type, MOBILE_INSTANCE *wlInst)
{	int failRtyCnt = 2;
	int rsp;
	
	SENS_SPRINTF(atCmdStr, "AT+IFC=%d,%d\r" EMPTY_CHAR, type, type);
	
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
			break;
		failRtyCnt--;
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int getFirmwareVersion(MOBILE_INSTANCE *wlInst)
{	int failRtyCnt = 2;
	int rsp;
	char *buf = NULL;
	char *pos;
	
	if(wlInst->atType == MOBILE_AT_TYPE_TELIT)
		SENS_SPRINTF(atCmdStr, "AT#SWPKGV\r" EMPTY_CHAR);
	else
		SENS_SPRINTF(atCmdStr, "ATI9\r" EMPTY_CHAR);
	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
	iotSendCmd(wlInst, atCmdStr);
	while(failRtyCnt)
	{	//rsp = waitAtCmdRsp(2, NULL, CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		rsp = waitAtCmdRsp(2, "OK", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(!rsp)
		{	SENS_MEM_FREE(buf);
			break;
		}
		else
		{	if(buf)
			{	if(wlInst->module == MOBILE_TYPE_R410)
				{	pos = strstr(buf, "05.06,A.02.01");
					if(pos)
					{	//pciInf->isOldR410FirmwareVer = 1;
						//wlInst->isNotSupportCmuxMode = 1;
					}
				}
				else if(wlInst->module == MOBILE_TYPE_L210)
				{	pos = strstr(buf, "16.19");
					if(pos)
					{	wlInst->isNewL210 = 1;
					}
				}
#if SUPPORT_AGPS
				else if(wlInst->module == MOBILE_TYPE_ME310G)
				{	pos = strstr(buf, "37.00.213");
					if(pos)
						wlInst->supportGtp = 0;
				}
#endif
				SENS_MEM_FREE(buf);
				buf = NULL;
			}
		}
	}
	
	return rsp;
}
//------------------------------------------------------------------------------
static int stePdpAuthenticationParams(MOBILE_INSTANCE *wlInst)
{	int rsp;
	
	if(wlInst->simAuthType)
		SENS_SPRINTF(atCmdStr, "AT#PDPAUTH=1,%d,\"%s\",\"%s\"\r" EMPTY_CHAR, wlInst->simAuthType, wlInst->simAccount, wlInst->simPass);
	else
		SENS_SPRINTF(atCmdStr, "AT#PDPAUTH=1,0\r" EMPTY_CHAR);
	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
	iotSendCmd(wlInst, atCmdStr);
	while(1)
	{	rsp = waitAtCmdRsp(2, NULL, CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
			break;
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int stePppAuthenticationParams(MOBILE_INSTANCE *wlInst)
{	int rsp;
	int failRtyCnt=2;
	
	if(wlInst->simAuthType)
		SENS_SPRINTF(atCmdStr, "AT#GAUTH=%d\r" EMPTY_CHAR, wlInst->simAuthType);
	else
		SENS_SPRINTF(atCmdStr, "AT#GAUTH=0\r" EMPTY_CHAR);
	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
	iotSendCmd(wlInst, atCmdStr);
	while(failRtyCnt)
	{	rsp = waitAtCmdRsp(2, NULL, CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
			break;
		failRtyCnt--;
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int setNbiotCommunicationMode(MOBILE_INSTANCE *wlInst)
{	int failRtyCnt = 2;
	int rsp;
	char *buf=NULL;
	int nbMode;
	char setMode = 1;
	char *pos;
	
	if(wlInst->atType == MOBILE_AT_TYPE_TELIT)
		SENS_SPRINTF(atCmdStr, "AT#WS46?\r" EMPTY_CHAR);
	else
		SENS_SPRINTF(atCmdStr, "AT+URAT?\r" EMPTY_CHAR);
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		if(wlInst->atType == MOBILE_AT_TYPE_TELIT)
			rsp = waitAtCmdRsp(2, "#WS46: ", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		else
			rsp = waitAtCmdRsp(2, "+URAT: ", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(!rsp)
		{	if(wlInst->atType == MOBILE_AT_TYPE_TELIT)
			{	SENS_SSCANF(buf, "#WS46: %d", &nbMode);
				if(nbMode == 2)	//AUTO
				{	setMode = 0;
				}
			}
			else
			{	pos = strstr(buf, ",");
				if(pos)
					setMode = 0;
			}
		}
		else
		{	if(buf)
				SENS_MEM_FREE(buf);
			return -1;
		}
		rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
			break;
		failRtyCnt--;
		SENS_TIME_DELAY(200);
	}
	
	if(!setMode)
		return rsp;
	
	failRtyCnt = 2;
	if(wlInst->atType == MOBILE_AT_TYPE_TELIT)
	{	SENS_SPRINTF(atCmdStr, "AT#WS46=2\r" EMPTY_CHAR);
	}
	else
	{	SENS_SPRINTF(atCmdStr, "AT+URAT=7,8\r" EMPTY_CHAR);
	}
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
			break;
		failRtyCnt--;
		SENS_TIME_DELAY(200);
	}
	return -1;
}
//------------------------------------------------------------------------------
static int automaticTimeZoneUpdate(MOBILE_INSTANCE *wlInst)
{	int failRtyCnt = 2;
	int rsp;
	
	SENS_SPRINTF(atCmdStr, "AT+CTZU=1\r" EMPTY_CHAR);
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
			break;
		failRtyCnt--;
		SENS_TIME_DELAY(200);
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int timeZoneReporting(MOBILE_INSTANCE *wlInst)
{	int failRtyCnt = 2;
	int rsp;
	
	SENS_SPRINTF(atCmdStr, "AT+CTZR=2\r" EMPTY_CHAR);
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
			break;
		failRtyCnt--;
		SENS_TIME_DELAY(200);
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int setModuleFunctionality(int func, MOBILE_INSTANCE *wlInst)
{	int failRtyCnt=2;
	int rsp;
	char *buf=NULL;
	
	if(wlInst->module == MOBILE_TYPE_ME310G)
	{	failRtyCnt = 5;
	}	
	SENS_SPRINTF(atCmdStr, "AT+CFUN=%d\r" EMPTY_CHAR, func);
	
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
SET_MODULE_FUNCTIONALITY_RE_GET:
		rsp = waitAtCmdRsp(4, "OK", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(!rsp)
		{	SENS_MEM_FREE(buf);
			break;
		}
		else
		{	if(buf)
			{	if(!processUnknownURC(buf))
				{	dbg_msg("%s", "get URC, re-wait OK!!\r\n");
					SENS_MEM_FREE(buf);
					buf = NULL;
					goto SET_MODULE_FUNCTIONALITY_RE_GET;
				}
				else
					SENS_MEM_FREE(buf);
			}
		}
		failRtyCnt--;
		SENS_TIME_DELAY(200);
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int pdpContextDefinition(uint8_t set, MOBILE_INSTANCE *wlInst, uint8_t cid)
{	int rsp;
	int failRtyCnt=2;
	char *buf = NULL;
	char *pos;
	char *pos1;
	char profileId;
	
	if(set)
		SENS_SPRINTF(atCmdStr, "AT+CGDCONT=%d,\"IP\",\"%s\"\r" EMPTY_CHAR, cid, wlInst->apn);
	else
		SENS_SPRINTF(atCmdStr, "AT+CGDCONT?\r" EMPTY_CHAR);
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		if(set)
		{	rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
			if(!rsp)
				break;
			failRtyCnt--;
		}
		else
		{	while(1)
			{	rsp = waitAtCmdRsp(2, "OK", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
				if(rsp)
				{	pos = strstr(buf, "\"IP\"");
					pos1 = strstr(buf, "\"IPV4V6\"");
					if(pos || pos1)
					{	pos = strstr(buf, "+CGDCONT: ");
						pos += strlen("+CGDCONT: ");
						strtokLock();
						pos1 = strtok(pos, ",");
						profileId = _io_atoi(pos1);
						pos1 = strtok(NULL, ",");	//"IP"
						pos1 = strtok(NULL, ",");	//"internet"
						pos1 = strtok(NULL, ",");	//"xx.xx.xx.xx"
						strtokUnLock();
						pos1 = strstr(pos1, ".");
						if(pos1)
						{	wlInst->netProfileId = profileId;
						}
						SENS_MEM_FREE(buf);
					}
					else
					{	SENS_MEM_FREE(buf);
						failRtyCnt--;
						break;
					}
					//rsp = waitAtCmdRsp(2, "+CGDCONT: ", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 0, NULL, wlInst, __func__, __LINE__);	
				}	
				else
				{	SENS_MEM_FREE(buf);
					return 0;
				}
			}
		}
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int operatorSelection(int mode, MOBILE_INSTANCE *wlInst)
{	int rsp;
	char *buf=NULL;
	int failRtyCnt = 2;
	
	if(mode == NB_OP_MODE_MANUAL)
		SENS_SPRINTF(atCmdStr, "AT+COPS=1,2,\"%d\"\r" EMPTY_CHAR, wlInst->plmn);
	else
		SENS_SPRINTF(atCmdStr, "AT+COPS=%d\r" EMPTY_CHAR, mode);
	
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		if(mode == NB_OP_MODE_MANUAL)
		{	rsp = waitAtCmdRsp(30, "OK", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
			if(!rsp)
			{	SENS_MEM_FREE(buf);
				return 0;
			}
			else
			{	char *pos;
				pos = strstr(buf, "+CME ERROR: "/*: no network service*/);	//+CME ERROR: no network service
				if(pos)
				{	SENS_MEM_FREE(buf);
					return -7;
				}
				pos = strstr(buf, "ERROR");	//+CME ERROR: no network service
				SENS_MEM_FREE(buf);
				buf = NULL;
				if(pos)
					return -1;
			}
			failRtyCnt--;
		}
		else
		{	rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
			if(!rsp)
				break;
			failRtyCnt--;
		}
	}
  return rsp;
}
//------------------------------------------------------------------------------
static int epsNetworkRegistrationStatus(uint8_t get, char mode, MOBILE_INSTANCE *wlInst)
{	int rsp;
	//struct AtCmdReceive *atCmdRecv = (struct AtCmdReceive *)&mobileSys.atCmdRecv;
	int modeConfiguration, status;
	char rspStr[20];
	char statusGetDone=0;
	
	
	if(get)
	{	if(wlInst->module == MOBILE_TYPE_L210)
			SENS_SPRINTF(atCmdStr, "AT+CREG?\r" EMPTY_CHAR);
		else
			SENS_SPRINTF(atCmdStr, "AT+CEREG?\r" EMPTY_CHAR);
	}
	else	
		//SENS_SPRINTF(atCmdStr, "AT+CEREG=1\r" EMPTY_CHAR);
	{	SENS_SPRINTF(atCmdStr, "AT+CEREG=%d\r" EMPTY_CHAR, mode);
		wlInst->ceregRdy = 0;
	}
	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
	iotSendCmd(wlInst, atCmdStr);
	if(!get)
		return waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
	else
	{	if(wlInst->atType == MOBILE_AT_TYPE_TELIT)
		{	rsp = waitAtCmdRsp(4, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
			if(!rsp)
			{	//dbg_msg("return status:%d\r\n", status);
				if(wlInst->ceregRdy == 1)
					return 1;
				else if(wlInst->ceregRdy == 3)
					return 3;
			}
			return rsp;
		}
#if 1
		else
		{	char *buf=NULL;
			if(wlInst->module == MOBILE_TYPE_L210)
				SENS_SPRINTF(rspStr, "+CREG: " EMPTY_CHAR);
			else
				SENS_SPRINTF(rspStr, "+CEREG: " EMPTY_CHAR);
			rsp = waitAtCmdRsp(2, rspStr, CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
			if(!rsp)
			{	if(!statusGetDone)
				{	char *pos = strstr(buf, " ");
					SENS_SSCANF(pos, " %d,%d", &modeConfiguration, &status);
				}
				SENS_MEM_FREE(buf);

				rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
				if(!rsp)
				{	//dbg_msg("return status:%d\r\n", status);
					if(status == 5)	//roam network
						status = 1;
					return status;
				}
			}
			else
				SENS_MEM_FREE(buf);
			return rsp;
		}
#endif
	}
}
//------------------------------------------------------------------------------
int extendedPacketSwitchedNetworkRegistrationStatus(MOBILE_INSTANCE *wlInst)
{	int rsp;
	int uregN, uregSts, num;
	int failRtyCnt=2;
	char rspStr[20];
	char *buf=NULL;
	char *pos;
	
	memset(rspStr, 0, 20);
	if(wlInst->module == MOBILE_TYPE_L210)
	{	SENS_SPRINTF(atCmdStr, "AT+UREG?\r" EMPTY_CHAR);
		SENS_SPRINTF(rspStr, "+UREG");
	}
	else
	{	SENS_SPRINTF(atCmdStr, "AT+COPS?\r" EMPTY_CHAR);
		SENS_SPRINTF(rspStr, "+COPS");
	}
		
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
RE_GET_STS:
		rsp = waitAtCmdRsp(2, rspStr, CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(!rsp)
		{	pos = strstr(buf, "AT");
			if(pos)
			{	SENS_MEM_FREE(buf);
				goto RE_GET_STS;
			}
			
			if(wlInst->module == MOBILE_TYPE_L210)
			{	SENS_SSCANF(buf, "+UREG: %d,%d", &uregN, &uregSts);
			}
			else
			{	SENS_SSCANF(buf, "+COPS: %d,%d,\"%s\",%d", &uregN, &num, rspStr, &uregSts);
			}
			SENS_MEM_FREE(buf);
			//if(wlInst->module == MOBILE_TYPE_L210)
			if(wlInst->mobileCellType == MOBILE_CELL_TYPE_LTE)
			{	wlInst->wlStatus = (enum WL_REGISTER_STS)uregSts;
				sensPq->onboardPq[ON_BOARD_PQ_LTE_STS] = wlInst->wlStatus;
				putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LTE_STS, sensPq->onboardPq[ON_BOARD_PQ_LTE_STS]);
			}
			else
			{	if(wlInst->module == MOBILE_TYPE_R410)
				{	if(uregSts == 7)
						wlInst->simCardTech = SIM_TYPE_CAT_M1;
					else	//9:NBIOT
						wlInst->simCardTech = SIM_TYPE_NBIOT;
				}
				else 
				{	if(uregSts == 8)
						wlInst->simCardTech = SIM_TYPE_CAT_M1;
					else	//9:NBIOT
						wlInst->simCardTech = SIM_TYPE_NBIOT;
				}
			}
			rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
			if(!rsp)
				return uregSts;
			else
			{	//return rsp;
				failRtyCnt--;
			}
		}
		else
		{	if(buf)
			{	if(!processUnknownURC(buf))
				{	SENS_MEM_FREE(buf);
					goto RE_GET_STS;
				}
				SENS_MEM_FREE(buf);
			}
			failRtyCnt--;
		}
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int packetSwitchedDataConfiguration(int paramTag, MOBILE_INSTANCE *wlInst)
{	int keepBuf;
	int ignoreOtherCmdRsp;
	int rsp;
	int failRtyCnt=2;
	
	if(paramTag == PSD_CFG_PARAM_TAG_APN)
	{	SENS_SPRINTF(atCmdStr, "AT+UPSD=0,1,\"%s\"\r" EMPTY_CHAR, wlInst->apn);
		keepBuf = 0;
		ignoreOtherCmdRsp = IGNORE_OTHER_AT_CMD_RSP;
	}
	else if(paramTag == PSD_CFG_PARAM_TAG_USERNAME)
	{	//SENS_SPRINTF(atCmdStr, "AT+UPSD=0,2,\"%s\"\r" EMPTY_CHAR, "mobile");
		SENS_SPRINTF(atCmdStr, "AT+UPSD=0,2,\"%s\"\r" EMPTY_CHAR, wlInst->simAccount);
		keepBuf = 1;
		ignoreOtherCmdRsp = CHECK_OTHER_AT_CMD_RSP;
	}
	else if(paramTag == PSD_CFG_PARAM_TAG_PASSWORD)
	{	//SENS_SPRINTF(atCmdStr, "AT+UPSD=0,3,\"%s\"\r" EMPTY_CHAR, "mobile");
		SENS_SPRINTF(atCmdStr, "AT+UPSD=0,3,\"%s\"\r" EMPTY_CHAR, wlInst->simPass);
		keepBuf = 1;
		ignoreOtherCmdRsp = CHECK_OTHER_AT_CMD_RSP;
	}
	else if(paramTag == PSD_CFG_PARAM_TAG_AUTH)
	{	//SENS_SPRINTF(atCmdStr, "AT+UPSD=0,6,%d\r" EMPTY_CHAR, 0);
		SENS_SPRINTF(atCmdStr, "AT+UPSD=0,6,%d\r" EMPTY_CHAR, wlInst->simAuthType);
		keepBuf = 1;
		ignoreOtherCmdRsp = CHECK_OTHER_AT_CMD_RSP;
	}
	else if(paramTag == PSD_CFG_PARAM_TAG_MAP_PROFILE_CID)
	{	SENS_SPRINTF(atCmdStr, "AT+UPSD=0,100,%d\r" EMPTY_CHAR, wlInst->netProfileId);
		keepBuf = 0;
		ignoreOtherCmdRsp = IGNORE_OTHER_AT_CMD_RSP;
	}
	
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(3, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		if(!keepBuf)
		{	rsp = waitAtCmdRsp(2, NULL, ignoreOtherCmdRsp, WAIT_STR_TYPE_OK, keepBuf, NULL, wlInst, __func__, __LINE__);
			if(rsp == NW_INIT_ERR_OK)
				break;
			failRtyCnt--;
		}
		else
		{	char *buf = NULL;
			rsp = waitAtCmdRsp(2, NULL, ignoreOtherCmdRsp, WAIT_STR_TYPE_OK, keepBuf, &buf, wlInst, __func__, __LINE__);
			if(!rsp)
			{	SENS_MEM_FREE(buf);
				return rsp;
			}
			else
			{	if(!memcmp(buf, "ERROR", strlen("ERROR")))
				{	SENS_MEM_FREE(buf);
					return -2;
				}
				if(buf)
					SENS_MEM_FREE(buf);
				if(rsp == NW_INIT_ERR_AT_RSP_TIMEOUT)
					failRtyCnt--;
				else
					return rsp;
			}
		}
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int getGprsAttach(MOBILE_INSTANCE *wlInst)
{	int rsp;
	int attach;
	int failRtyCnt=2;
	
	SENS_SPRINTF(atCmdStr, "AT+CGATT?\r" EMPTY_CHAR);
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		char *buf=NULL;
		rsp = waitAtCmdRsp(2, "+CGATT: ", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(!rsp)
		{	SENS_SSCANF(buf, "+CGATT: %d", &attach);
			SENS_MEM_FREE(buf);
		
			rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
			if(!rsp)
				return attach;
			else
			{	failRtyCnt--;
				//return rsp;
			}
		}
		else
		{	if(buf)
				SENS_MEM_FREE(buf);
			failRtyCnt--;
		}
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int setClkMode(MOBILE_INSTANCE *wlInst)
{	int rsp;
	int failRtyCnt=2;
	
	SENS_SPRINTF(atCmdStr, "AT#CCLKMODE=1\r" EMPTY_CHAR);
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
		{	return rsp;
		}
		failRtyCnt--;
		SENS_TIME_DELAY(200);
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int getPdpAddress(MOBILE_INSTANCE *wlInst, int activeCid)
{	int rsp;
	int tmp;
	int failRtyCnt=2;
	char *buf=NULL;
	NET_INSTANCE *workingInst = networkCtx->workingInst;
	//struct AtCmdReceive *atCmdRecv = (struct AtCmdReceive *)&mobileSys.atCmdRecv;
	
	SENS_SPRINTF(atCmdStr, "AT+CGPADDR=%d\r" EMPTY_CHAR, activeCid);
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
RE_GET_CGPADDR:
		rsp = waitAtCmdRsp(2, "+CGPADDR: ", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(!rsp)
		{	if(wlInst->module == MOBILE_TYPE_R410)
			{	SENS_SSCANF(buf, "+CGPADDR: %d,%d.%d.%d.%d",	&tmp, &wlInst->ip[0], &wlInst->ip[1], &wlInst->ip[2], &wlInst->ip[3]);
			}
			else
			{	SENS_SSCANF(buf, "+CGPADDR: %d,\"%d.%d.%d.%d\"",	&tmp, &wlInst->ip[0], &wlInst->ip[1], &wlInst->ip[2], &wlInst->ip[3]);
			}
			SENS_MEM_FREE(buf);	
			dbg_msg("%s channel IP: %d.%d.%d.%d\r\n", workingInst->netType == NETWORK_TYPE_HS ? "HS":"LS", wlInst->ip[0], wlInst->ip[1], wlInst->ip[2], wlInst->ip[3]);
#if 0
			if(wlInst->mobileCellType == MOBILE_CELL_TYPE_LTE)
			{	gprs.client_ip[0] = wlInst->ip[0];
				gprs.client_ip[1] = wlInst->ip[1];
				gprs.client_ip[2] = wlInst->ip[2];
				gprs.client_ip[3] = wlInst->ip[3];
			}
			else
			{	gprs.client_ip_NB[0] = wlInst->ip[0];
				gprs.client_ip_NB[1] = wlInst->ip[1];
				gprs.client_ip_NB[2] = wlInst->ip[2];
				gprs.client_ip_NB[3] = wlInst->ip[3];
			}
#endif
#if TEST_N_REMOVE
			sensDev->ledStatus = LED_STS_STANDBY;
#endif
			if(wlInst->ip[0] == 0 && wlInst->ip[1] == 0 && wlInst->ip[2] == 0 && wlInst->ip[3] == 0)
				wlInst->wlSignalQ = -115;
			rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
			if(!rsp)
				break;
			failRtyCnt--;
		}
		else
		{	if(buf)
			{	if(!processUnknownURC(buf))
				{	SENS_MEM_FREE(buf);
					buf = NULL;
					goto RE_GET_CGPADDR;
				}
				SENS_MEM_FREE(buf);
			}
			failRtyCnt--;
		}
	}
	return rsp;
}
//------------------------------------------------------------------------------
static char checkTimezoneYear(int year)
{	int compilerYear;
	
	compilerYear = /*((__DATE__[7] - 0x30) * 1000) + ((__DATE__[8] - 0x30) * 100) + */((__DATE__[9] - 0x30) * 10) + ((__DATE__[10] - 0x30) * 1);
	dbg_msg("year:%d, compilerYear:%d\r\n", year, compilerYear);
	if(compilerYear != 0x00)
	{	if((year >= (compilerYear - 1)) && (year < (compilerYear+20)))
		{	return 1;
		}
	}
	else
	{	compilerYear = 99;
		if(year == compilerYear  || year < 20)
		{	return 1;
		}
	}
	return 0;
}
//------------------------------------------------------------------------------
int getClock(MOBILE_INSTANCE *wlInst)
{	int rsp;
	char *pos;
	char timeStr[20];
	char *buf=NULL;
	char failRtyCnt = 2;
	
	memset(timeStr, 0, 20);
	SENS_SPRINTF(atCmdStr, "AT+CCLK?\r" EMPTY_CHAR);
	SENS_SPRINTF(timeStr, "+CCLK: ");
	
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		rsp = waitAtCmdRsp(2, timeStr, CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(rsp != NW_INIT_ERR_AT_RSP_TIMEOUT)
		{	pos = strstr(buf, "+CCLK: ");
			if(pos)
			{	//if(!sensSys->getTimeFromSntp)
				{	pos = strstr(buf, "-");
					SENS_SEM_LOCK(networkCtx->timeZoneLock);
					if(pos == NULL)
					{	
#if defined NUVOTON
						SENS_SSCANF(buf+1, "CCLK: \"%d/%d/%d,%d:%d:%d+%d\"", &networkCtx->timezone.u32Year, &networkCtx->timezone.u32Month, &networkCtx->timezone.u32Day, &networkCtx->timezone.u32Hour, &networkCtx->timezone.u32Minute, &networkCtx->timezone.u32Second, &networkCtx->timezonediff);
#else
						SENS_SSCANF(buf+1, "CCLK: \"%d/%d/%d,%d:%d:%d+%d\"", &networkCtx->timezone[0], &networkCtx->timezone[1], &networkCtx->timezone[2], &networkCtx->timezone[3], &networkCtx->timezone[4], &networkCtx->timezone[5], &networkCtx->timezonediff); 
#endif
					}
					else
					{	
#if defined NUVOTON
						SENS_SSCANF(buf+1, "CCLK: \"%d/%d/%d,%d:%d:%d-%d\"", &networkCtx->timezone.u32Year, &networkCtx->timezone.u32Month, &networkCtx->timezone.u32Day, &networkCtx->timezone.u32Hour, &networkCtx->timezone.u32Minute, &networkCtx->timezone.u32Second, &networkCtx->timezonediff); 
#else
						SENS_SSCANF(buf+1, "CCLK: \"%d/%d/%d,%d:%d:%d-%d\"", &networkCtx->timezone[0], &networkCtx->timezone[1], &networkCtx->timezone[2], &networkCtx->timezone[3], &networkCtx->timezone[4], &networkCtx->timezone[5], &networkCtx->timezonediff); 
#endif
						networkCtx->timezonediff = (-1)*networkCtx->timezonediff;
					}
					networkCtx->daylightsaving = 0;
					if(wlInst->module != MOBILE_TYPE_L210)
						networkCtx->timezonediff = 0;
					SENS_MEM_FREE(buf);
#if defined NUVOTON
					if(checkTimezoneYear(networkCtx->timezone.u32Year))
#else
					if(checkTimezoneYear(networkCtx->timezone[0]))
#endif
					{	
#if defined NUVOTON
						networkCtx->timezone.u32Year = networkCtx->timezone.u32Year + 2000;
						if(networkCtx->timezone.u32Year > 2020)
#else
						networkCtx->timezone[0] = networkCtx->timezone[0] + 2000;
						if(networkCtx->timezone[0] > 2020)
#endif
						{	if((wlInst->module != MOBILE_TYPE_L210 && wlInst->wlSignalQ > -115) || (wlInst->module == MOBILE_TYPE_L210))
							{	dbg_msg2("%s", "Update time clock.\r\n");
								timezoneDiffMobile((S_RTC_TIME_DATA_T *)&networkCtx->timezone, networkCtx->timezonediff, networkCtx->daylightsaving);
#ifndef DISABLE_UPDATE_TIME
								setTime((S_RTC_TIME_DATA_T *)&networkCtx->timezone);
#endif
							}
						}
					}
#ifdef TEST_TIME_XMIT
					}
#endif
					SENS_SEM_UNLOCK(networkCtx->timeZoneLock);
				}
				//else
				//	SENS_MEM_FREE(buf);
				return waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
			}
			else
			{	if(!processUnknownURC(buf))
				{	failRtyCnt++;
				}
			}
			SENS_MEM_FREE(buf);
			buf = NULL;
		}
		else
			SENS_MEM_FREE(buf);
		failRtyCnt--;
	}
	return rsp;
}
//------------------------------------------------------------------------------
static int deleteNonActivePdp(MOBILE_INSTANCE *wlInst)
{	int failRtyCnt=2;
	int rsp;
	char *buf=NULL;
	
	SENS_SPRINTF(atCmdStr, "AT+CGDEL=4\r" EMPTY_CHAR);
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		rsp = waitAtCmdRsp(8, "+CGDEL", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
		if(!rsp)
		{	SENS_MEM_FREE(buf);
			rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
			if(!rsp)
			{	break;
			}
		}
		else
		{	if(buf)
				SENS_MEM_FREE(buf);
			buf = NULL;
		}
		failRtyCnt--;
	}
	
	if(!rsp)
	{	AT_CMD_RECV *atCmdRecv = (AT_CMD_RECV *)&iotSys.atCmdRecv;
		setModuleFunctionality(16, wlInst);
		SENS_TIME_DELAY(2000);
		atCmdRecv->receivePos = 0;
		
		waitMobileReady(wlInst, 0);
		disableEcho(wlInst);
	}
	return rsp;
}
//------------------------------------------------------------------------------
#ifdef SUPPORT_PPP
int setPppMode(MOBILE_INSTANCE *wlInst)
{	int rsp;
	int failRtyCnt=20;
	char *buf=NULL;
	char *pos;
	char isReceiveErr = 0;
	
	while(failRtyCnt)
	{	//SENS_SPRINTF(atCmdStr, "ATDT*99***1#\r" EMPTY_CHAR);
		if(wlInst->module == MOBILE_TYPE_L210)
		{	if(!isReceiveErr)
				SENS_SPRINTF(atCmdStr, "ATDT*99***100#\r" EMPTY_CHAR);
			else
				SENS_SPRINTF(atCmdStr, "ATDT*99***4#\r" EMPTY_CHAR);
		}
		else
		{	
/*#if SUPPORT_AGPS
			if(sens_sys->enableGps && wlInst->supportAgps)
				SENS_SPRINTF(atCmdStr, "ATDT*99***2#\r" EMPTY_CHAR);
			else
#endif*/
				SENS_SPRINTF(atCmdStr, "ATDT*99***1#\r" EMPTY_CHAR);
		}
		displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		while(1)
		{	rsp = waitAtCmdRsp(4, "CONNECT", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
			if(!rsp)
			{	SENS_MEM_FREE(buf);
				return rsp;
			}
			else
			{	if(buf)
				{	pos = strstr(buf, "ERROR");
					if(pos)
					{	if(wlInst->module == MOBILE_TYPE_L210)
							isReceiveErr = 1;
					}
					SENS_MEM_FREE(buf);
					buf = NULL;
				}
				else
					break;
			}
			failRtyCnt--;
		}
	}
	return rsp;
}
#endif

#ifdef PPP_USE_CMUX
//------------------------------------------------------------------------------
int setCmuxMode(MOBILE_INSTANCE *wlInst, int mode)
{	int rsp;
	int failRtyCnt=2;
	
	SENS_SPRINTF(atCmdStr, "AT#CMUXMODE=%d\r" EMPTY_CHAR, mode);
	
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		
		rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
		{	break;	
		}
		else
			failRtyCnt--;
	}
	return rsp;
}
//------------------------------------------------------------------------------
int enableCmux(MOBILE_INSTANCE *wlInst)
{	int rsp;
	int failRtyCnt=2;
	
	//memset(atCmdStr, 0, AT_CMD_MAX_SIZE);
	if((wlInst->module == MOBILE_TYPE_LE910C1) || wlInst->module == MOBILE_TYPE_LE910_WWX)
	{	setCmuxMode(wlInst, 5);
		SENS_SPRINTF(atCmdStr, "AT+CMUX=0,0\r" EMPTY_CHAR);
	}
	else //if(wlInst->module == MOBILE_TYPE_ME310G)
	{	//SENS_SPRINTF(atCmdStr, "AT+CMUX=0,0,,1500,50,3,90\r" EMPTY_CHAR);
		SENS_SPRINTF(atCmdStr, "AT+CMUX=0,0,,1500,10,3,30,10,7\r" EMPTY_CHAR);
		//SENS_SPRINTF(atCmdStr, "AT+CMUX=0,0,,,10,3,30,10,7\r" EMPTY_CHAR);
	}
	/*else if(wlInst->module == MOBILE_TYPE_R410)	
		SENS_SPRINTF(atCmdStr, "AT+CMUX=0,0,,1500,50,3,90\r" EMPTY_CHAR);*/
	
	while(failRtyCnt)
	{	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
		iotSendCmd(wlInst, atCmdStr);
		
		rsp = waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
		if(!rsp)
		{	break;	
		}
		else
		{	if(wlInst->module == MOBILE_TYPE_ME310G)
			{	SENS_SPRINTF(atCmdStr, "AT+CMUX=0,0\r" EMPTY_CHAR);
			}
			failRtyCnt--;
		}
	}
	return rsp;
}
#endif
//------------------------------------------------------------------------------
int getSignalQuality(MOBILE_INSTANCE *wlInst)
{	int rsp;
	int signalQuality;
	char *buf=NULL;
	char *pos;
	
	//struct AtCmdReceive *atCmdRecv = (struct AtCmdReceive *)&mobileSys.atCmdRecv;
	
	SENS_SPRINTF(atCmdStr, "AT+CSQ\r" EMPTY_CHAR);
	displayCrrentSendAtCmd(1, atCmdStr, __func__, __LINE__);
	iotSendCmd(wlInst, atCmdStr);
RE_GET_CSQ:
	rsp = waitAtCmdRsp(4, "+CSQ", CHECK_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_SPECIFY, 1, &buf, wlInst, __func__, __LINE__);
	if(!rsp)
	{	//SENS_SSCANF(atCmdRecv->receiveTemp, "+CSQ: %d,", &signalQuality);
		pos = strstr(buf, "AT");
		if(pos)
		{	SENS_MEM_FREE(buf);
			goto RE_GET_CSQ;
		}
		
		
		SENS_SSCANF(buf, "+CSQ: %d,", &signalQuality);
		//dbg_msg2("get Quality: %d\r\n", signalQuality);
		//freeReceiveTempBuf();
		SENS_MEM_FREE(buf);
		if(signalQuality == 99)
		{	if(wlInst->mobileCellType != MOBILE_CELL_TYPE_LTE)
				wlInst->wlStatus = WL_REG_STS_NOT_REGISTER;
			wlInst->wlSignalQ = -115;
			if(wlInst->mobileCellType != MOBILE_CELL_TYPE_LTE)
			{	sensPq->onboardPq[ON_BOARD_PQ_NB_STS] = WL_REG_STS_NOT_REGISTER;
				putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_NB_STS, sensPq->onboardPq[ON_BOARD_PQ_NB_STS]);
				sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] = wlInst->wlSignalQ;
				putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_NB_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH]);
			}
			else
			{	sensPq->onboardPq[ON_BOARD_PQ_LTE_STS] = WL_REG_STS_NOT_REGISTER;
				putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LTE_STS, sensPq->onboardPq[ON_BOARD_PQ_LTE_STS]);
				sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] = wlInst->wlSignalQ;
				putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LTE_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH]);
			}
		}
		else
		{	if(wlInst->mobileCellType != MOBILE_CELL_TYPE_LTE)
				wlInst->wlStatus = WL_REG_STS_LTE_CAT_NB_M1;
			wlInst->wlSignalQ = 2*signalQuality-113;
			if(wlInst->mobileCellType != MOBILE_CELL_TYPE_LTE)
			{	sensPq->onboardPq[ON_BOARD_PQ_NB_STS] = WL_REG_STS_LTE_CAT_NB_M1;
				putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_NB_STS, sensPq->onboardPq[ON_BOARD_PQ_NB_STS]);
				sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] = wlInst->wlSignalQ;
				putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_NB_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH]);
			}
			else
			{	sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] = wlInst->wlSignalQ;
				putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LTE_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH]);
			}
			
			if(getTimerInterval(SENSMINI_TIMER_CSQ) == 2000)
				setSensminiTimer((void *)networkProcessQ, NETWORK_Q_MSG_MOBILE_CSQ_GET, NULL, SENSMINI_TIMER_CSQ, GPRS_CSQ_TIMEOUT, TIMER_MODE_LOOPING);
		}
		return waitAtCmdRsp(2, NULL, IGNORE_OTHER_AT_CMD_RSP, WAIT_STR_TYPE_OK, 0, NULL, wlInst, __func__, __LINE__);
	}
	else
	{	if(buf)
		{	if(!processUnknownURC(buf))
			{	SENS_MEM_FREE(buf);
				buf = NULL;
				goto RE_GET_CSQ;
			}
			SENS_MEM_FREE(buf);
		}
	}
	return rsp;
}
//------------------------------------------------------------------------------
int iotMobileInit(MOBILE_INSTANCE *wlInst)
{	int idx;
	int rsp;
  
	NET_INSTANCE *workingInst;
#ifdef PPP_USE_CMUX
	CMUX_CONTEXT *cmuxCtx;
	CMUX_VCOMM	*vcomm;
#endif
	
#if SUPPORT_NUVOTON_USB_HOST
	MOBILE_USB_INSTANCE *mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
#endif
	int activeCid;
  
	workingInst = networkCtx->workingInst;
  
	workingInst->initError = 0;
	workingInst->connectToCellSite = 0;
#if TEST_N_REMOVE
	sensDev->ledStatus = LED_STS_CONNECTING;
#endif
	sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH] = -115;
	sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH] = -115;
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_NB_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_NB_SIGNAL_STRENGTH]);
	putValToMapPqData(ON_BOARD_DEV, ON_BOARD_PQ_LTE_SIGNAL_STRENGTH, sensPq->onboardPq[ON_BOARD_PQ_LTE_SIGNAL_STRENGTH]);

RE_INIT:
	wlInst->wlStatus = WL_REG_STS_NOT_REGISTER;
	rsp = waitMobileReady(wlInst, 0);
	if(rsp)	//issue AT each 2 sec and wait 1 min, if no dev ready chang to other
	{	wlInst->wlStatus = WL_REG_STS_WAIT_READY_FAIL;
		goto RESET_MODULE;
	}
	
	if(disableEcho(wlInst))	//ATE0
	{	wlInst->wlStatus = WL_REG_STS_UART_COMM_TIMEOUT_FAIL;
		goto RESET_MODULE;
	}
#if SUPPORT_NUVOTON_USB_HOST
	if(mobUsbInst && mobUsbInst->isUsbHost && mobUsbInst->usbIsMultiIfDev)
	{	if(mobUsbInst->usbIsCdcAcm)
		{	if(wlInst->usbDev == wlInst->usbDevAt1)
			{	CDC_DEV_T *cdev = (CDC_DEV_T *)wlInst->usbDevAt2;
				wlInst->usbDev = wlInst->usbDevAt2;
				UART_CONFIG *uartCfg = cdev->uartCfg;
				SENS_SEM_LOCK(iotSys.iotSem);
				wlInst->dev = uartCfg->ctx;
				SENS_SEM_UNLOCK(iotSys.iotSem);
				goto RE_INIT;
			}
			else
			{	CDC_DEV_T *cdev = (CDC_DEV_T *)wlInst->usbDevAt1;
				UART_CONFIG *uartCfg = cdev->uartCfg;
				wlInst->usbDev = wlInst->usbDevAt1;
				SENS_SEM_LOCK(iotSys.iotSem);
				wlInst->dev = uartCfg->ctx;
				SENS_SEM_UNLOCK(iotSys.iotSem);
			}
		}
		else
		{	if(wlInst->usbDev == wlInst->usbDevAt1)
			{	USB_COMPOSITE_DEVICE_IF_CTX *usbDevIfCtx = (USB_COMPOSITE_DEVICE_IF_CTX *)wlInst->usbDevAt2;
				if(usbDevIfCtx)
				{	wlInst->usbDev = wlInst->usbDevAt2;
					SENS_SEM_LOCK(iotSys.iotSem);
					wlInst->dev = usbDevIfCtx->usbCdcAtUartCfg->ctx;
					SENS_SEM_UNLOCK(iotSys.iotSem);
					goto RE_INIT;
				}
			}
			else
			{	USB_COMPOSITE_DEVICE_IF_CTX *usbDevIfCtx = (USB_COMPOSITE_DEVICE_IF_CTX *)wlInst->usbDevAt1;
				wlInst->usbDev = wlInst->usbDevAt1;
				SENS_SEM_LOCK(iotSys.iotSem);
				wlInst->dev = usbDevIfCtx->usbCdcAtUartCfg->ctx;
				SENS_SEM_UNLOCK(iotSys.iotSem);
			}
		}
	}
#endif
	
	if(wlInst->module == MOBILE_TYPE_AUTO)
	{	SENS_TIME_DELAY(500);
		if(getModule(wlInst))
		{	
		}
	}
	
	if(getSimPinStatus(wlInst))
	{	wlInst->wlStatus = WL_REG_STS_SIM_NOT_READY;
		goto RESET_MODULE;
	}
	
	if(wlInst->atType == MOBILE_AT_TYPE_UBLOX)
	{	if(wlInst->module == MOBILE_TYPE_R410)
		{
		#ifdef PLATFORM_FSL
			if(wlInst->baudrate == 19200)
			{	wlInst->baudrate = 115200;
				dbg_msg("%s\r\n", "change baudrate to 115200");
				uartDataRateConfiguration(wlInst, wlInst->baudrate);
				setBaudRate(wlInst->dev, wlInst->baudrate);
				SENS_TIME_DELAY(1000);
				struct AtCmdReceive *atCmdRecv = (struct AtCmdReceive *)&mobileSys.atCmdRecv;
				atCmdRecv->receivePos = 0;
			}
		#endif
		}
#if defined PLATFORM_XILINX
		if(setSwFlowControl(0, wlInst))	//0: disable flow control, 1: sw flow control, 2: hw flow control
#else
		if(setSwFlowControl(wlInst->flowCtrl, wlInst))	//0: disable flow control, 1: sw flow control, 2: hw flow control
#endif
		{	wlInst->wlStatus = WL_REG_STS_UART_COMM_TIMEOUT_FAIL;
			goto RESET_MODULE;
		}
		getFirmwareVersion(wlInst);
	}
	else
	{
#ifdef SUPPORT_PPP
		if(wlInst->module != MOBILE_TYPE_ME310G)
			stePppAuthenticationParams(wlInst);
#endif
		if(wlInst->module == MOBILE_TYPE_ME310G)
		{	if(setNbiotCommunicationMode(wlInst))
			{	wlInst->wlStatus = WL_REG_STS_RE_INIT;
				goto RESET_MODULE;
			}
		}
		getFirmwareVersion(wlInst);
		
		if((wlInst->atType == MOBILE_AT_TYPE_TELIT) && (wlInst->mobileCellType == MOBILE_CELL_TYPE_LTE))
		{	stePdpAuthenticationParams(wlInst);
		}
	}
	
	if(wlInst->initType == MOBILE_CELL_TYPE_LTE)
	{	if(automaticTimeZoneUpdate(wlInst))
		{	goto RESET_MODULE;
		}
		
		if(wlInst->module == MOBILE_TYPE_L210)
		{	if(timeZoneReporting(wlInst))
			{	goto RESET_MODULE;
			}
		}
	}
	
	if((wlInst->initType == MOBILE_CELL_TYPE_NBIOT) && (wlInst->module != MOBILE_TYPE_L210))
	{	if(setModuleFunctionality(1, wlInst))
		{	goto RESET_MODULE;
		}
	}
	
	if(wlInst->initType == MOBILE_CELL_TYPE_NBIOT)
	{	if(automaticTimeZoneUpdate(wlInst))
		{	goto RESET_MODULE;
		}
		activeCid = 1;
		if(wlInst->module == MOBILE_TYPE_L210)
			activeCid = 4;
		if(pdpContextDefinition(1, wlInst, activeCid))
		{	goto RESET_MODULE;
		}
		
		if(wlInst->mobileCellType != MOBILE_CELL_TYPE_LTE)
		{	rsp = operatorSelection(NB_OP_MODE_MANUAL, wlInst);
			if(rsp == -7)
			{	
			}
		}
		epsNetworkRegistrationStatus(0, 2, wlInst);
      
		if(rsp)
		{	SENS_TIME_DELAY(5000);
			extendTaskTimer((TASK_INFO *)&taskAct.networkProcessTaskAct);
		}
		for(idx=0;idx<10;idx++)
		{	int rsp = epsNetworkRegistrationStatus(1, 0, wlInst);
			if((rsp == 1) /*|| ((rsp == 2) && (idx > 2))*/)
			{	break;
			}
			if(rsp == 3)
			{	
			}
			int count = 50;
			while(1)
			{	if((wlInst->ceregRdy == 1) || !count)
					break;
				SENS_TIME_DELAY(100);
				count--;
			}
			//SENS_TIME_DELAY(5000);
			extendTaskTimer((TASK_INFO *)&taskAct.networkProcessTaskAct);
		}
		if(idx == 10)
		{	goto RESET_MODULE;
		}
		workingInst->connectToCellSite = 1;
		if(extendedPacketSwitchedNetworkRegistrationStatus(wlInst) <= 0)
		{	goto RESET_MODULE;
		}
	}
	else
	{	if(packetSwitchedDataConfiguration(PSD_CFG_PARAM_TAG_APN, wlInst))
		{	goto RESET_MODULE;
		}
		
		if(wlInst->simAuthType)
		{	int lteLoginSts;
			lteLoginSts = packetSwitchedDataConfiguration(PSD_CFG_PARAM_TAG_USERNAME, wlInst);
			if(lteLoginSts == 0)
			{	lteLoginSts = packetSwitchedDataConfiguration(PSD_CFG_PARAM_TAG_PASSWORD, wlInst);
				if(!lteLoginSts)
				{	lteLoginSts = packetSwitchedDataConfiguration(PSD_CFG_PARAM_TAG_AUTH, wlInst);
					if(lteLoginSts == -1)
					{	goto RESET_MODULE;
					}
				}
			}
			else if(lteLoginSts == -1)
			{	goto RESET_MODULE;
			}
		}
		wlInst->netProfileId = 4;
		if(packetSwitchedDataConfiguration(PSD_CFG_PARAM_TAG_MAP_PROFILE_CID, wlInst))
		{	goto RESET_MODULE;
		}
		
		if(setModuleFunctionality(1, wlInst))
		{	goto RESET_MODULE;
		}
		
		SENS_TIME_DELAY(1000);
		for(idx=0;idx<15;idx++)
		{	if(epsNetworkRegistrationStatus(1, 0, wlInst) == 1)
			{	break;
			}
			extendTaskTimer((TASK_INFO *)&taskAct.networkProcessTaskAct);
			SENS_TIME_DELAY(5000);
		}
		if(idx == 15)
		{	goto RESET_MODULE;
		}
		workingInst->connectToCellSite = 1;
		if(getGprsAttach(wlInst) != 1)
		{	//goto RESET_MODULE;
		}
		if(extendedPacketSwitchedNetworkRegistrationStatus(wlInst) <= 0)
		{	//goto RESET_MODULE;
		}
	}
	
	if(wlInst->initType != MOBILE_CELL_TYPE_LTE)
	{	if(wlInst->atType == MOBILE_AT_TYPE_UBLOX)
		{	if(pdpContextDefinition(0, wlInst, 1))
			{	goto RESET_MODULE;
			}
		}
		else
		{	if(setClkMode(wlInst))
				goto RESET_MODULE;
		}
		
		if(getPdpAddress(wlInst, activeCid))
		{	goto RESET_MODULE;
		}
		
		getClock(wlInst);
	}
	else
	{	wlInst->netProfileId = 4;
		pdpContextDefinition(0, wlInst, 1);
		if(wlInst->netProfileId != 4)
		{	deleteNonActivePdp(wlInst);
			goto RE_INIT;
		}
	}
	
	if(wlInst->module != MOBILE_TYPE_L210)
		epsNetworkRegistrationStatus(0, 0, wlInst);

#if SUPPORT_AGPS
	workingInst->currentIsGpsOnlyMode = 0;
	//sensDev->gpsMoveOverDistFlag = 0;
	if(wlInst->module == MOBILE_TYPE_ME310G)
	{	int rebootForAgpsSet=0;
		int getRsp=0;
		getRsp = gpsGetConfig(wlInst, GPS_PARAM_STARTUP_PRIORITY, GPS_PRIORITY_WWAN);
		if(getRsp == 1)
		{	rebootForAgpsSet = 1;
			gpsSetConfig(wlInst, GPS_PARAM_STARTUP_PRIORITY, GPS_PRIORITY_WWAN);
		}
		getRsp = gpsGetConfig(wlInst, GPS_PARAM_CONSTELLATION, sensSys->nbAgpsSys);
		if(getRsp == 1)
		{	rebootForAgpsSet = 1;
			gpsSetConfig(wlInst, GPS_PARAM_CONSTELLATION, sensSys->nbAgpsSys);
		}
		
		if(rebootForAgpsSet)
		{	wlInst->wlStatus = WL_REG_STS_RE_INIT;
			goto RESET_MODULE;
		}
	}
#endif

#ifdef PPP_USE_CMUX
	if(!wlInst->isNotSupportCmuxMode 
#if SUPPORT_NUVOTON_USB_HOST
			&& (!mobUsbInst->isUsbHost)
#endif
		&& !enableCmux(wlInst))
	{	wlInst->cMuxCtx = (void *)SENS_MEM_ZALLOC(sizeof(CMUX_CONTEXT));
		cmuxCtx = (CMUX_CONTEXT *)wlInst->cMuxCtx;
		cmuxCtx->wlInst = (void *)wlInst;
		SENS_SEM_LOCK(iotSys.iotSem);
#if defined UART_RECV_INTER_LOCK_IRQ
		xSemaphoreGive(wlInst->dev->sema);
		portYIELD();
#endif
		wlInst->devTemp = wlInst->dev;
		//wlInst->dev = NULL;
		cmuxCtx->dev = wlInst->devTemp;
		SENS_SEM_UNLOCK(taskAct.cMuxTask.lock);
		cmuxInit(cmuxCtx, CMUX_MAX_PORT);
		cmuxStart(cmuxCtx);
		if(wlInst->module == MOBILE_TYPE_L210)
			SENS_TIME_DELAY(1000);
		//cmuxAttach(cmuxCtx, CMUX_AT_PORT, CMUX_AT_NAME, 512);
		cmuxAttach(cmuxCtx, CMUX_AT_PORT, CMUX_AT_NAME, 512);
		if(wlInst->module == MOBILE_TYPE_L210)
			SENS_TIME_DELAY(1000);
		//cmuxAttach(cmuxCtx, CMUX_PPP_PORT, CMUX_PPP_NAME, 4096+1024);
		cmuxAttach(cmuxCtx, CMUX_PPP_PORT, CMUX_PPP_NAME, 4096+1024);
		if(wlInst->module == MOBILE_TYPE_L210)
			SENS_TIME_DELAY(1000);
		vcomm = &cmuxCtx->vComms[CMUX_AT_PORT];
		wlInst->dev = vcomm->dev;
		wlInst->atCmdDev = vcomm->dev;
		vcomm = &cmuxCtx->vComms[CMUX_PPP_PORT];
		wlInst->pppDev = vcomm->dev;
		
#if 1
		//if((wlInst->module == MOBILE_TYPE_L210) || (wlInst->module == MOBILE_TYPE_LE910C1))
		{	wlInst->dev = wlInst->pppDev;
			SENS_SEM_UNLOCK(mobileSys.mobileSem);
		
			waitMobileReady(wlInst, 1);
			
			if(disableEcho(wlInst))	//ATE0
			{	wlInst->wlStatus = WL_REG_STS_UART_COMM_TIMEOUT_FAIL;
				goto RESET_MODULE;
			}
	#if SUPPORT_AGPS
			if(sens_sys->enableGps && wlInst->supportAgps && wlInst->module == MOBILE_TYPE_ME310G)
				configAgps(wlInst);
	#endif
			SENS_SEM_LOCK(mobileSys.mobileSem);
			wlInst->dev = wlInst->atCmdDev;
		}
#endif
		SENS_SEM_UNLOCK(mobileSys.mobileSem);
		
#if 1
		//if((wlInst->module == MOBILE_TYPE_L210) || (wlInst->module == MOBILE_TYPE_LE910C1))
		{	if(disableEcho(wlInst))	//ATE0
			{	wlInst->wlStatus = WL_REG_STS_UART_COMM_TIMEOUT_FAIL;
				goto RESET_MODULE;
			}
		}
#endif
	}
#endif
	getSignalQuality(wlInst);
	
#if SUPPORT_AGPS
	if(sensSys->enableGps && wlInst->supportAgps)
	{	if((wlInst->module == MOBILE_TYPE_LE910C1) || (wlInst->module == MOBILE_TYPE_LE910C4))
		{	int rebootForLteGps=0;
			wlInst->gpsRunMode = GPS_LOOP_MODE;
			
			rebootForLteGps = gpsChkSetSystem(wlInst, sensSys->lteAgpsSys);
			if(rebootForLteGps == 1)
			{	wlInst->wlStatus = WL_REG_STS_RE_INIT;
				goto RESET_MODULE;
			}
			//gpsStartLocationServiceReq(wlInst);
			//setSensminiTimer((pointer)wireLanQ, 		NETWORK_Q_MSG_AGPS_LOCATION_GET,	NULL, SENSMINI_TIMER_GPS_GET_POSITION, 		GPS_GET_TIMEOUT, 	TIMER_MODE_LOOPING);
		}
	}
#endif
	
	if(!wlInst->isNotSupportCmuxMode
#if SUPPORT_NUVOTON_USB_HOST
		|| (mobUsbInst->isUsbHost && !mobUsbInst->usbIsCdcAcm)
#endif
		)
	{	if((wlInst->module == MOBILE_TYPE_L210) || (wlInst->module == MOBILE_TYPE_LE910C1) || (wlInst->module == MOBILE_TYPE_LE910C4))
		{	setSensminiTimer((void *)networkProcessQ,	NETWORK_Q_MSG_MOBILE_UREG_GET,	NULL, SENSMINI_TIMER_UREG,	GPRS_UREG_TIMEOUT,	TIMER_MODE_LOOPING);
		}
		//setSensminiTimer((void *)networkProcessQ,	NETWORK_Q_MSG_MOBILE_CSQ_GET, 	NULL, SENSMINI_TIMER_CSQ, 		GPRS_CSQ_TIMEOUT, 				TIMER_MODE_LOOPING);
		if(wlInst->wlSignalQ == -115)
			setSensminiTimer((void *)networkProcessQ,	NETWORK_Q_MSG_MOBILE_CSQ_GET, 	NULL, SENSMINI_TIMER_CSQ,	2000,				TIMER_MODE_LOOPING);
		else
			setSensminiTimer((void *)networkProcessQ,	NETWORK_Q_MSG_MOBILE_CSQ_GET, 	NULL, SENSMINI_TIMER_CSQ,	GPRS_CSQ_TIMEOUT,	TIMER_MODE_LOOPING);
	}
	
#if SUPPORT_AGPS
	if(!sensSys->enableGps || !wlInst->supportAgps /*|| wlInst->module == MOBILE_TYPE_LE910C4*/)
#endif
	{
#ifdef SUPPORT_PPP
		wirelessPppInit(wlInst);
	#if defined NET_RTCS
		if(!wlInst->pppHandle)
	#else
		if(wlInst->ppp == NULL)
	#endif
		{	wlInst->wlStatus = WL_REG_STS_PPP_FAIL;
			goto RESET_MODULE;
		}
#endif
	}

#if SUPPORT_AGPS
	if(sensSys->enableGps && wlInst->supportAgps)
	{	extendTaskTimer(&taskAct.wireLanSendTask);
		if((wlInst->module == MOBILE_TYPE_LE910C1) || (wlInst->module == MOBILE_TYPE_LE910C4))
		{	if(wlInst->supportGtp)
			{	if(configGtp(wlInst))
				{	goto RESET_MODULE;
				}
			}
			configAgps(wlInst);
			setContextActivation(wlInst, 1);
			gpsStartLocationServiceReq(wlInst);
			setSensminiTimer((pointer)wireLanQ, 		NETWORK_Q_MSG_AGPS_LOCATION_GET,	NULL, SENSMINI_TIMER_GPS_GET_POSITION, 		GPS_ME310_GET_TIMEOUT, 	TIMER_MODE_LOOPING);
		}
		else
		{	if(wlInst->supportGtp)
			{	if(configGtp(wlInst))
				{	goto RESET_MODULE;
				}
			}
			if(!wlInst->isAgnssMode)
      {	if(gpsSetAgnss(wlInst) == -2)
      	{	goto RESET_MODULE;
      	}
      }
			
			setContextActivation(wlInst, 1);
			configAgps(wlInst);
			
			wlInst->gpsRunMode = GPS_LOOP_MODE;
#if FORCE_CLOSE_AGNSS
			sensDev->enAgpsGetPositionTime = 1;
#endif
			switchGpsWan(wlInst, 1);
			setSensminiTimer((pointer)wireLanQ, 		NETWORK_Q_MSG_AGPS_LOCATION_GET,	NULL, SENSMINI_TIMER_GPS_GET_POSITION, 		GPS_ME310_GET_TIMEOUT, 	TIMER_MODE_LOOPING);
			if(!wlInst->setExtendWorkingTime)
			{	sensSys->extendWorkingTime += (sensSys->gpsColdStartTimeout);	//for next wakeup early...
				wlInst->setExtendWorkingTime = 1;
			}
		}
	}
#endif

	wlInst->status = WL_INIT_STS_SUCCESS;
	return 0;
RESET_MODULE:
	wlInst->errorRetryCnt++;
	if(wlInst->errorRetryCnt >= 3)
	{	workingInst->initError = 1;
	}
	else
	{	if(wlInst->wlStatus != WL_REG_STS_RE_INIT)
			workingInst->initError = 1;
	}
	
#if TEST_N_REMOVE
	if(wlInst->atType == MOBILE_AT_UBLOX)
	{	if(!powerOffUblox(wlInst))
		{
		}
	}
#endif
	if(workingInst->netType == NETWORK_TYPE_HS)
		iotPwrCtrl(IOT_IF_SEL_IOT1_USB, 0);
	else
		iotPwrCtrl(IOT_IF_SEL_IOT2_USB, 0);

	return -1;
}
//------------------------------------------------------------------------------
int initLsIo(NET_INSTANCE *netInst)
{	MOBILE_INSTANCE *wlInst = netInst->wlInst;
#if !defined SENSMINIA4_NEO
	UART_CONFIG *lsComm;
#endif
	
	//dbg_msg("%s", "[LSPOWER] Power off\r\n");
	
#ifdef SENSMINIA4_PLUS
	RS485_close();
#endif
	
#if !defined SENSMINIA4_NEO	//A4 neo only use USB 
// #else
	if(sensDev->lsComm == NULL)
		sensDev->lsComm = SENS_MEM_ZALLOC(sizeof(UART_CONFIG));
	lsComm = sensDev->lsComm;
	lsComm->baudrate = wlInst->baudrate;
	dbg_msg("set LS baudrate:%d\r\n", wlInst->baudrate);
	lsComm->mode = N_8_1;
	lsComm->dataLen = 8;
	lsComm->parity = 1;
	lsComm->stopbit = 1;
#ifndef PLATFORM_FSL
	lsComm->ctx = SENS_MEM_ZALLOC(sizeof(SENS_UART_CTX));
#endif
#ifdef NUVOTON
	lsComm->flowCtrl = wlInst->flowCtrl;
	lsComm->id = IOT_UART_NO;
	lsComm->uartType = UT_UART;
	lsComm->bufferLength = MOBILE_UART_BUFFER_SIZE;
	addUartCfg(lsComm);
#endif
	SENS_UART_INIT(lsComm->ctx, IOT_UART_NO, lsComm->baudrate, lsComm->mode, MOBILE_UART_BUFFER_SIZE, UT_UART, wlInst->flowCtrl);
#endif
	//SENS_UART_FLUSH_INPUT(lsComm->ctx);
#if defined PLATFORM_FSL
  //RS485_InitializeIO();
  RS485_Enable(0);
#elif defined PLATFORM_XILINX
	setCommPortPwer(UART_COMM2, 1);
#elif defined SENSMINIS2
	setPcieEn(POWER_ON);
#elif defined SENSMINIA4_NEO
	iotPwrCtrl(IOT_IF_SEL_IOT2_USB, 1);
	SENS_TIME_DELAY(1000);
#endif

#if !defined SENSMINIA4_NEO
	iotPwrCtrl(IOT_IF_SEL_IOT2_UART, 1);
	SENS_UART_FLUSH_INPUT(lsComm->ctx);
#endif

	dbg_msg("%s", "[LSPOWER] Power on\r\n");

#ifdef PLATFORM_FSL
	SENS_GPIO_SET_VALUE(&uart_485_1_ls, SENS_GPIO_VALUE_HIGH);
#endif
  return 0;
}
//------------------------------------------------------------------------------
int initHsIo(void)
{	int ret = 1;
#if defined PLATFORM_FSL
	// Define 4G Power, 4G Powerkey, 4G TX, 4G RX, 4G RTS, 4G CTS
	PORT_MemMapPtr  pctl;
	pctl = (PORT_MemMapPtr)PORTE_BASE_PTR;

	pctl->PCR[10] = PORT_PCR_MUX(ALT1);                                                         // 4G Power
	pctl->PCR[12] = PORT_PCR_MUX(ALT1);                                                         // 4G Powerkey
	pctl->PCR[24] = PORT_PCR_MUX(ALT3);                                                         // 4G TX
	pctl->PCR[25] = PORT_PCR_MUX(ALT3);                                                         // 4G RX
	//pctl->PCR[27] = PORT_PCR_MUX(ALT3)|PORT_PCR_PS_MASK|PORT_PCR_PE_MASK|PORT_PCR_DSE_MASK;     // 4G RTS
	pctl->PCR[27] = PORT_PCR_MUX(ALT3) | PORT_PCR_DSE_MASK;
	pctl->PCR[26] = PORT_PCR_MUX(ALT3)| PORT_PCR_DSE_MASK | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;		// 4G CTS

	// 4G Power
	ret = SENS_GPIO_INIT(&gprs_pwron, EN_GPRS, SENS_GPIO_DIR_OUT, SENS_GPIO_VALUE_NONCHG, 0, NULL);
	if (!ret) return ret;
	SENS_GPIO_SET_FUNCTION(&gprs_pwron, LWGPIO_MUX_GPIO);

	// 4G Powerkey
	ret = SENS_GPIO_INIT(&gprs_pwrkey, GPRS_KEY, SENS_GPIO_DIR_OUT, SENS_GPIO_VALUE_NONCHG, 0, NULL);
	if (!ret) return ret;
	SENS_GPIO_SET_FUNCTION(&gprs_pwrkey, LWGPIO_MUX_GPIO);

	ret = SENS_GPIO_INIT(&gprs_cts, GPRS_CTS, SENS_GPIO_DIR_IN, SENS_GPIO_VALUE_NONCHG, 0, NULL);
	if (!ret) return ret;

	ret = SENS_GPIO_INIT(&uart_gprs_rx, RX_GPRS, SENS_GPIO_DIR_IN, SENS_GPIO_VALUE_NONCHG, 0, NULL);
	if (!ret) return ret;
#elif defined PLATFORM_XILINX
	ret = 1;
#elif defined NUVOTON
#endif
  return ret;
}
//------------------------------------------------------------------------------
#ifndef NUVOTON
void gprsPowerOn(int flag)
{	
#if defined PLATFORM_FSL
	showtime();
  if(flag)
  {	SENS_GPIO_SET_VALUE(&gprs_pwron, SENS_GPIO_VALUE_HIGH);
  	dbg_msg("%s", "[HSPOWER] Power On\r\n");
    showtime();
  	dbg_msg("%s", "[Network] Power KEY: LOW\r\n");
  	PIO_STATUS_PWRKEY = 1;
  	SENS_TIME_DELAY(200);

  	showtime();
  	dbg_msg("%s", "[Network] Power KEY: HIGH\r\n");

		PIO_STATUS_PWRKEY = 0;
  	SENS_TIME_DELAY(200);
  }
  else
  {	SENS_GPIO_SET_VALUE(&gprs_pwron, SENS_GPIO_VALUE_LOW);
  	dbg_msg("%s", "[HSPOWER] Power Off\r\n");
    showtime();
  	dbg_msg("%s", "[Network] Power KEY: HIGH\r\n");
  	PIO_STATUS_PWRKEY = 0;
  	SENS_TIME_DELAY(200);

  	showtime();
  	dbg_msg("%s", "[Network] Power KEY: LOW\r\n");

		PIO_STATUS_PWRKEY = 1;
  	SENS_TIME_DELAY(200);
  }
#elif defined PLATFORM_XILINX
	setCommPortPwer(UART_COMM1, flag);
#endif
}
#endif
//------------------------------------------------------------------------------
int initialHsIo(NET_INSTANCE *netInst)
{	int rc = 0;
	MOBILE_INSTANCE *wlInst = netInst->wlInst;
	UART_CONFIG *hsComm;
	
	if(!initHsIo())
	{	dbg_msg("%s", "[WARNING] Module: Initialize IO Failed.\r\n");
		return -1;
	}
	
#if !defined NUVOTON
	gprsPowerOn(1);
#elif !defined SENSMINIA4_NEO
	if(sensDev->hsComm == NULL)
		sensDev->hsComm = SENS_MEM_ZALLOC(sizeof(UART_CONFIG));
	hsComm = sensDev->hsComm;
	hsComm->baudrate = wlInst->baudrate;
	dbg_msg("set LS baudrate:%d\r\n", wlInst->baudrate);
	hsComm->mode = N_8_1;
	hsComm->dataLen = 8;
	hsComm->parity = 1;
	hsComm->stopbit = 1;
	if(hsComm->ctx == NULL)
		hsComm->ctx = SENS_MEM_ZALLOC(sizeof(SENS_UART_CTX));
	hsComm->flowCtrl = wlInst->flowCtrl;
	hsComm->id = IOT_UART_NO;
	hsComm->uartType = UT_UART;
	hsComm->bufferLength = MOBILE_UART_BUFFER_SIZE;
	addUartCfg(hsComm);
	SENS_UART_INIT(hsComm->ctx, IOT_UART_NO, hsComm->baudrate, hsComm->mode, MOBILE_UART_BUFFER_SIZE, UT_UART, wlInst->flowCtrl);
	
	iotPwrCtrl(IOT_IF_SEL_IOT1_UART, 1);
	SENS_TIME_DELAY(1000);
	
	SENS_UART_FLUSH_INPUT(hsComm->ctx);
#else
	iotPwrCtrl(IOT_IF_SEL_IOT1_USB, 1);
	SENS_TIME_DELAY(1000);
#endif
	dbgMsg("%s", "[HS COMM] Power on\r\n");
	return 0;
}
//------------------------------------------------------------------------------
void freeNetworkInst(NET_INSTANCE *netInst)
{	if(netInst == NULL)
		return;
	
	if(netInst->sendStr)
		SENS_MEM_FREE(netInst->sendStr);
	if(netInst->recvStr)
		SENS_MEM_FREE(netInst->recvStr);
	if(netInst->stsStr)
		SENS_MEM_FREE(netInst->stsStr);
		
	if(netInst->wlInst)
	{	SENS_MEM_FREE(netInst->wlInst);
		netInst->wlInst = NULL;
	}
	
	SENS_MEM_FREE(netInst);
	netInst = NULL;
}
//------------------------------------------------------------------------------
static int setWirelessInfo(NET_INSTANCE *netInst, enum NETWORK_TYPE netType)
{	int chan;
	MOBILE_INSTANCE		*wlInst;
	NET_CONFIG	*netCfg = (NET_CONFIG *)&sysCfg->netCfg;
	WIRELESS_CONFIG	*wirelessCfg = (WIRELESS_CONFIG *)&netCfg->wirelessCfg;
	MOBILE_CONFIG	*mobileCfg;

#if IS_M4_DEV
	if(netType == NETWORK_TYPE_LS)
		return -1;
#endif

	netInst->netType = netType;
	if(netType == NETWORK_TYPE_HS)
		chan = 0;
	else
		chan = 1;

	if(netInst->wlInst == NULL)
		netInst->wlInst = SENS_MEM_ZALLOC(sizeof(MOBILE_INSTANCE));
	wlInst = netInst->wlInst;
	
	wlInst->upperCtx = netInst;
	wlInst->module = MOBILE_TYPE_NONE;
	wlInst->protocol = MOBILE_PROTOCOL_NORMAL;
	wlInst->baudrate = 115200;

	if((wirelessCfg->channelType[chan] == PCIE_3G_LTE) || (wirelessCfg->channelType[chan] == PCIE_3G_PURE_DATA) ||
		 (wirelessCfg->channelType[chan] == PCIE_SARA_N200) ||
		 (wirelessCfg->channelType[chan] == PCIE_SARA_R410) ||
		 (wirelessCfg->channelType[chan] == PCIE_TELIT_ME310G) ||
		 (wirelessCfg->channelType[chan] == PCIE_TELIT_LE910C1)
		)
	{
#ifdef PLATFORM_FSL
		if(netType == NETWORK_TYPE_HS)
			wlInst->flowCtrl = FC_HW;
#endif
#ifdef NUVOTON
		wlInst->flowCtrl = FC_HW;	//for test UART, 
#endif
		wlInst->module = MOBILE_TYPE_AUTO;
	}
#if SUPPORT_YL_PUMP
	else if(sensDev->transfer_protocol[chan] == PCIE_GPS_L76)
	{	wlInst->module = MOBILE_TYPE_GPS_L76;
		wlInst->baudrate = 9600;
	}
#endif
	
	if(wlInst->module == MOBILE_TYPE_NONE)
	{	freeNetworkInst(netInst);
		return -2;
	}

	if(wlInst->module == MOBILE_TYPE_AUTO  || wlInst->module == MOBILE_TYPE_USB)
	{	netInst->sendStr = SENS_MEM_ZALLOC(16);
		netInst->recvStr = SENS_MEM_ZALLOC(16);
		netInst->stsStr = SENS_MEM_ZALLOC(16);
		
		/*if(wirelessCfg->lteCfg)
		{	memcpy(wlInst->apn, gprs.apn, 32);
			memcpy(wlInst->simPass, gprs.sim_pass, 32);
			memcpy(wlInst->simAccount, gprs.sim_account, 32);
		}*/
		strcpy(netInst->sendStr, LTE_SEND_STR);
		strcpy(netInst->recvStr, LTE_RECV_STR);
		strcpy(netInst->stsStr, LTE_STATUS_STR);
	}
  return 0;
}
//------------------------------------------------------------------------------
int setupMobileComm(void)
{	int rsp;
	networkCtx->hsCommInst = SENS_MEM_ZALLOC(sizeof(NET_INSTANCE));
	networkCtx->lsCommInst = SENS_MEM_ZALLOC(sizeof(NET_INSTANCE));
	
	rsp = setWirelessInfo(networkCtx->hsCommInst, NETWORK_TYPE_HS);
	if(rsp == -2)
	{	//SENS_MEM_FREE(networkCtx->hsCommInst);
		networkCtx->hsCommInst = NULL;
	}
	rsp = setWirelessInfo(networkCtx->lsCommInst, NETWORK_TYPE_LS);
	if(rsp == -2)
	{	//SENS_MEM_FREE(networkCtx->lsCommInst);
		networkCtx->lsCommInst = NULL;
	}
	
	if((networkCtx->hsCommInst && (networkCtx->hsCommInst->wlInst->module != MOBILE_TYPE_NONE)) && 
		 (networkCtx->lsCommInst && ((networkCtx->lsCommInst->wlInst->module != MOBILE_TYPE_NONE) && (networkCtx->lsCommInst->wlInst->module != MOBILE_TYPE_GPS_L76)))
		)
	{	sysCtrl->pwrManager.extendWorkingTime += 30;
	}
  
  return 0;
}
//------------------------------------------------------------------------------
int mobileDeinit(MOBILE_INSTANCE	*wlInst)
{	
#ifdef PPP_USE_CMUX
	CMUX_CONTEXT *cmuxCtx;
#endif
#ifdef SUPPORT_NUVOTON_USB_HOST
	MOBILE_USB_INSTANCE *mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
#endif
	NET_INSTANCE *workingInst = networkCtx->workingInst;
	
	killSensminiTimer(SENSMINI_TIMER_CSQ);
	killSensminiTimer(SENSMINI_TIMER_CLK);
	killSensminiTimer(SENSMINI_TIMER_UREG);
#if SUPPORT_AGPS
	killSensminiTimer(SENSMINI_TIMER_GPS_GET_POSITION);
	killSensminiTimer(SENSMINI_TIMER_GPS_RESTART);
#endif
	
#ifdef SUPPORT_PPP
	#if defined NET_RTCS
	if(wlInst->pppHandle)
	{	modemDisconnect(wlInst->pppHandle);
		wlInst->pppHandle = 0;
	}

	if(wlInst->modemParams)
	{	SENS_MEM_FREE(wlInst->modemParams);
		wlInst->modemParams = NULL;
	}
	if(wlInst->pppParams)
	{	SENS_MEM_FREE(wlInst->pppParams);
		wlInst->pppParams = NULL;
	}
	#else
	if(wlInst->ppp)
		modemDisconnect(wlInst);
	#endif
#endif
#ifdef PPP_USE_CMUX 
	cmuxCtx = wlInst->cMuxCtx;
	if(cmuxCtx)
	{	cmuxDetach(cmuxCtx, 2);
		wlInst->pppDev = NULL;
		cmuxDetach(cmuxCtx, 1);
		wlInst->atCmdDev = NULL;
		wlInst->dev = NULL;
		cmuxDetach(cmuxCtx, 0);
		SENS_TIME_DELAY(500);
		//SENS_SEM_LOCK(taskAct.cMuxTask.lock);
		SENS_MEM_FREE(cmuxCtx->buffer);
		SENS_MEM_FREE(cmuxCtx->vComms);
		SENS_MEM_FREE(cmuxCtx);
		SENS_SEM_LOCK(taskAct.cMuxTask.lock);
		fclose(wlInst->devTemp);
		wlInst->devTemp = NULL;
		wlInst->cMuxCtx = NULL;
	}
#endif
	
	/*if(wlInst->atType == MOBILE_AT_UBLOX)
	{	if(!powerOffUblox(wlInst))
		{
		}
	}*/
#ifdef SUPPORT_NUVOTON_USB_HOST
	mobUsbInst->usbIsCdcAcm = 0;
	mobUsbInst->usbIsMultiIfDev = 0;
	mobUsbInst->isUsbHost = 0;
#endif
#ifdef SUPPORT_FSL_USB_HOST
	if(wlInst->isUsbMode && wlInst->usbUp)
	{	removeUsbDevice();
		wlInst->isUsbMode = 0;
		wlInst->usbUp = 0;
	}
#endif
	wlInst->isNotSupportCmuxMode = 0;
	if(workingInst->netType == NETWORK_TYPE_HS)
	{	sensDev->hsComm = NULL;
		iotPwrCtrl(IOT_IF_SEL_IOT1_USB, 0);
		//GPRS_PowerOn(FALSE);
	}
	else
	{	sensDev->lsComm = NULL;
		iotPwrCtrl(IOT_IF_SEL_IOT2_USB, 0);
		//LSCOM_Power(0);
	}

	return 0;
}