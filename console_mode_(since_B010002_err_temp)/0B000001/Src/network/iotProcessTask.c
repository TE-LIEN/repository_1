#include "network/netApp.h"
#include "sensSystem.h"
#include "sensCommon.h"
#include "rtcDateTime.h"

#if SUPPORT_NUVOTON_USB_HOST
#include "usbh_lib.h"
#include "usbh_uvc.h"
#include "usbh_cdc.h"
#include "hub.h"
#include "usbHost/usbHostTask.h"
#endif

//------------------------------------------------------------------------------
void iotAtRecvTask(void *param)
{	char OneByteN;
	AT_CMD_RECV *atCmdRecv = (AT_CMD_RECV *)&iotSys.atCmdRecv;
	char *strPos;
	struct TaskQMsg msg;
	int validAscii = 0;
	MOBILE_INSTANCE *wlInst;
	NET_INSTANCE	*workingInst;
#if SUPPORT_NUVOTON_USB_HOST
	//uint32_t unmRead;
	MOBILE_USB_INSTANCE *mobUsbInst;
#endif

	//SENS_SEM_LOCK(taskAct.mobileRecvTask.lock);
	SENS_SEM_LOCK(taskAct.iotAtRecvTaskAct.lock);
#ifdef OS_FREERTOS
	SENS_TIME_DELAY(2);
#endif

	atCmdRecv->receiveBuf = SENS_MEM_ALLOC(512);
	atCmdRecv->receivePos = 0;
	//atCmdRecv->receiveTemp = NULL;
	//dbg_msg("mobileSys at %p, atCmdRecv at %p, buf at %p, pos at %p, mobileSys size:%X\r\n", &mobileSys, atCmdRecv, atCmdRecv->receiveBuf, &atCmdRecv->receivePos, sizeof(struct MobileSystem));
	dbg_msg("%s start\r\n", __func__);

	while(1)
	{	//taskAct.mobileRecvTask.sts = TASK_STS_BLOCKED;
		SENS_SEM_LOCK(iotSys.iotSem);
		//taskAct.mobileRecvTask.sts = TASK_STS_RUNNING;
		extendTaskTimer((TASK_INFO *)&taskAct.iotAtRecvTaskAct);
		while(1)
		{	if(networkCtx == NULL)
				break;
			if(!networkCtx->mobileUartStartRecv)
				break;	
			if(networkCtx->workingInst == NULL)
				break;
			workingInst = networkCtx->workingInst;
			wlInst = workingInst->wlInst;
			if(wlInst == NULL)
				break;
			if(wlInst->dev == NULL)
				break;
			
			if(SENS_UART_STATUS(wlInst->dev))
			{	SENS_UART_READ(wlInst->dev, (uint8_t *)&OneByteN, 1);
				atCmdRecv->receiveBuf[atCmdRecv->receivePos++] = OneByteN;
				atCmdRecv->receiveBuf[atCmdRecv->receivePos] = '\0';
        
				if((atCmdRecv->receivePos > 3) && (atCmdRecv->receiveBuf[atCmdRecv->receivePos-2] == 0x0D) && (atCmdRecv->receiveBuf[atCmdRecv->receivePos-1] == 0x0A))
				{	//displayBufInHex((uint8_t *)atCmdRecv->receiveBuf, atCmdRecv->receivePos, __func__, __LINE__);
					atCmdRecv->receiveBuf[atCmdRecv->receivePos-1] = '\0';
					strPos = strstr(atCmdRecv->receiveBuf, "+CTZE: ");
					if(strPos != NULL)
					{	displayCrrentSendAtCmd(0, strPos, __func__, __LINE__);
						char *pos;
						pos = strstr(atCmdRecv->receiveBuf, "-");
						if(pos == NULL)
						{	
#if defined NUVOTON
							SENS_SSCANF(strPos, "+CTZE: +%d,%d,\"%d/%d/%d,%d:%d:%d\"", &networkCtx->timezonediff, &networkCtx->daylightsaving, &networkCtx->timezone.u32Year, &networkCtx->timezone.u32Month, &networkCtx->timezone.u32Day, &networkCtx->timezone.u32Hour, &networkCtx->timezone.u32Minute, &networkCtx->timezone.u32Second);
#else
							SENS_SSCANF(strPos, "+CTZE: +%d,%d,\"%d/%d/%d,%d:%d:%d\"", &networkCtx->timezonediff, &networkCtx->daylightsaving, &networkCtx->timezone[0], &networkCtx->timezone[1], &networkCtx->timezone[2], &networkCtx->timezone[3], &networkCtx->timezone[4], &networkCtx->timezone[5]);
#endif
						}
						else
						{	
#if defined NUVOTON
							SENS_SSCANF(strPos, "+CTZE: -%d,%d,\"%d/%d/%d,%d:%d:%d\"", &networkCtx->timezonediff, &networkCtx->daylightsaving, &networkCtx->timezone.u32Year, &networkCtx->timezone.u32Month, &networkCtx->timezone.u32Day, &networkCtx->timezone.u32Hour, &networkCtx->timezone.u32Minute, &networkCtx->timezone.u32Second);
#else
							SENS_SSCANF(strPos, "+CTZE: -%d,%d,\"%d/%d/%d,%d:%d:%d\"", &networkCtx->timezonediff, &networkCtx->daylightsaving, &networkCtx->timezone[0], &networkCtx->timezone[1], &networkCtx->timezone[2], &networkCtx->timezone[3], &networkCtx->timezone[4], &networkCtx->timezone[5]);
#endif	
							networkCtx->timezonediff = (-1)*networkCtx->timezonediff;
						}
#if defined NUVOTON
						networkCtx->timezone.u32Year += 2000;
						if(networkCtx->timezone.u32Year > 2026 && networkCtx->timezone.u32Year < 2080)
#else
						networkCtx->timezone += 2000;
						if(networkCtx->timezone[0] > 2026 && networkCtx->timezone[0] < 2080)
#endif
						{	dbg_msg2("%s", "Update time clock.\r\n");
							timezoneDiffMobile((S_RTC_TIME_DATA_T *)&networkCtx->timezone, networkCtx->timezonediff, networkCtx->daylightsaving);
							setTime((S_RTC_TIME_DATA_T *)&networkCtx->timezone);
						}
						atCmdRecv->receivePos = 0;
						break;
					}
					strPos = strstr(atCmdRecv->receiveBuf, "+UUPSDA: ");
					if(strPos != NULL)
					{	atCmdRecv->receivePos = 0;
						//goto RECEIVE_END;
						displayCrrentSendAtCmd(0, strPos, __func__, __LINE__);
						break;
						//continue;
					}
					//$GPSSLSR: SUPL ERROR,7	//TELIT LE910 gps supl server error
					strPos = strstr(atCmdRecv->receiveBuf, "$GPSSLSR: ");
					if(strPos)
					{	atCmdRecv->receivePos = 0;
						//goto RECEIVE_END;
						displayCrrentSendAtCmd(0, strPos, __func__, __LINE__);
						break;
					}
						
#ifdef SUPPORT_PPP
					strPos = strstr(atCmdRecv->receiveBuf, "CONNECT");
					if(strPos)
					{	wlInst->usePPP = 1;
						if(wlInst->isNotSupportCmuxMode)
						{	
		#if SUPPORT_NUVOTON_USB_HOST
							mobUsbInst = (MOBILE_USB_INSTANCE *)&wlInst->usbInst;
	
							if(mobUsbInst->usbIsMultiIfDev)
							{	if(mobUsbInst->usbIsCdcAcm)
								{	if(wlInst->usbDevAt2)
									{	CDC_DEV_T *cdev = (CDC_DEV_T *)wlInst->usbDevAt2;
										UART_CONFIG *uartCfg = cdev->uartCfg;
										wlInst->dev = uartCfg->ctx;
									}
									else
										networkCtx->mobileUartStartRecv = 0;	// ppp mode, stop receive UART data	
								}
								else
								{	if(wlInst->usbDevAt2)
									{	USB_COMPOSITE_DEVICE_IF_CTX *usbDevIfCtx = (USB_COMPOSITE_DEVICE_IF_CTX *)wlInst->usbDevAt2;
										wlInst->dev = usbDevIfCtx->usbCdcAtUartCfg->ctx;
									}
									else
										networkCtx->mobileUartStartRecv = 0;	// ppp mode, stop receive UART data	
								}
							}
							else
								networkCtx->mobileUartStartRecv = 0;	// ppp mode, stop receive UART data
		#elif SUPPORT_FSL_USB_HOST
							if(((!wlInst->isUsbMode) || (wlInst->isUsbMode && wlInst->isCdcAcm)))
							{	networkCtx->mobileUartStartRecv = 0;	// ppp mode, stop receive UART data
							}
							else
							{	wlInst->dev = wlInst->atCmdDev;
							}
		#endif
						}
						else
							wlInst->dev = wlInst->atCmdDev;
						dbg_msg("%s", "[PPP] receive CONNECT\r\n");
					}
#endif

					strPos = strstr(atCmdRecv->receiveBuf, "+CEREG: ");
					if(strPos != NULL && wlInst->atType == MOBILE_AT_TYPE_TELIT)
					{	char *pos1 = strstr(atCmdRecv->receiveBuf, "\"");
						char *pos2 = strstr(atCmdRecv->receiveBuf, "+CEREG: 2");
						char tac[5], ci[9];
						char act;
						int status, modeConfiguration;
						displayCrrentSendAtCmd(0, strPos, __func__, __LINE__);
						if(pos1 != NULL)
						{	pos1 = strstr(atCmdRecv->receiveBuf, " ");
							if(pos2)
							{	SENS_SSCANF(pos1+1, "2,%d,\"%s\",\"%s\",%d", &status, tac, ci, &act);
							}
							else
							{	SENS_SSCANF(pos1+1, "%d,\"%s\",\"%s\",%d", &status, tac, ci, &act);
							}
							if(status == 1)	//home network
								wlInst->ceregRdy = 1;
							else if(status == 5)	//roam network
								wlInst->ceregRdy = 1;
						}
						if(!wlInst->ceregRdy)
						{	pos2 = strstr(atCmdRecv->receiveBuf, " ");
							if(pos2)
							{	SENS_SSCANF(pos2, " %d,%d", &modeConfiguration, &status);
								if(status == 3)
								{	wlInst->ceregRdy = status;
								}
							}
						}
						atCmdRecv->receivePos = 0;
					}
					else
					{
//PARSE_NORMAL:
						for(validAscii=0;validAscii<atCmdRecv->receivePos;validAscii++)
						{	if((atCmdRecv->receiveBuf[validAscii] >= 0x20) && (atCmdRecv->receiveBuf[validAscii] <= 0x7F))
							{	break;
							}
						}
						if(validAscii == atCmdRecv->receivePos)
						{	atCmdRecv->receivePos = 0;
							break;
						}
						AT_CMD_RECV_BACK_BUF *atCmdRecvBakBuf;
						atCmdRecvBakBuf = SENS_MEM_ALLOC(sizeof(AT_CMD_RECV_BACK_BUF));
						atCmdRecvBakBuf->len = atCmdRecv->receivePos - validAscii;
						atCmdRecvBakBuf->buf = SENS_MEM_ALLOC(atCmdRecvBakBuf->len);
						memcpy(atCmdRecvBakBuf->buf, &atCmdRecv->receiveBuf[validAscii], atCmdRecvBakBuf->len);
						atCmdRecv->receivePos = 0;
						msg.msgId = NETWORK_Q_MSG_MOBILE_AT_CMD_NORMAL_RSP;
						msg.ptr = (char *)atCmdRecvBakBuf;
						//dbg_msg("get Receive data(%d)\r\n", validAscii);
						if(sendMsgWithErrHandle(NETWORK_PROCESS_TASK, SENS_MSG_Q_SEND(networkProcessQ, (uint32_t *)&msg, 0), __func__, __LINE__))
						{	SENS_MEM_FREE(atCmdRecvBakBuf->buf);
							SENS_MEM_FREE(atCmdRecvBakBuf);
						}
						break;
					}
				}
			}
			else
				break;
		}
		SENS_SEM_UNLOCK(iotSys.iotSem);
		SENS_TIME_DELAY(2);
	}
}