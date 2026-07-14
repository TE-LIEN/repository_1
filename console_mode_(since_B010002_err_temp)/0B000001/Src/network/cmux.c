#include "network/cmux.h"

#ifdef PPP_USE_CMUX
#include "network/netApp.h"
#include "sensCommon.h"
#include "sensSystem.h"

//#define CMUX_DEBUG
//#define CMUX_DEBUG_1

#define CMUX_RECV_USE_MASS_DATA	

#ifdef CMUX_DEBUG_1
	#ifdef CMUX_DEBUG
	#undef CMUX_DEBUG
	#endif
#endif

#ifdef CMUX_DEBUG
	#ifdef CMUX_DEBUG_1
	#undef CMUX_DEBUG_1
	#endif
#define	MAX_UART_RECV_BUF_SIZE		256
uint8_t cmuxRecvBuf[MAX_UART_RECV_BUF_SIZE];
uint32_t cmuxRecvLength;
#endif

#if defined CMUX_DEBUG || defined CMUX_DEBUG_1
uint8_t cmuxDebug=0;
#endif


#ifdef PLATFORM_FSL
typedef MQX_FILE FILE_DEVICE_STRUCT, * FILE_DEVICE_STRUCT_PTR;

#define MEM_TYPE_VCOMM_DEVICE_STRUCT		1200
#define IO_IOCTL_PUT_DATA								_IO(IO_TYPE_MQX, 0x80)
#define IO_IOCTL_SET_INIT_DATA_PTR			_IO(IO_TYPE_MQX, 0x81)
//------------------------------------------------------------------------------
_mqx_int vCommOpen(FILE_DEVICE_STRUCT_PTR fd_ptr, char *open_name_ptr, char *flags)
{	IO_DEVICE_STRUCT_PTR		io_dev_ptr;
	IO_VCOMM_DEVICE_STRUCT	*vcommDev;
	_mqx_uint								result = MQX_OK;
	//CMUX_VCOMM							*vcommCtx;
	//CMUX_CONTEXT						*cmuxCtx;
	
	io_dev_ptr = fd_ptr->DEV_PTR;
	vcommDev = (void *)io_dev_ptr->DRIVER_INIT_PTR;
	if(vcommDev->COUNT)
	{	vcommDev->COUNT++;
		fd_ptr->FLAGS = vcommDev->FLAGS;
		return (_mqx_int)(result);
	}
	vcommDev->bufPtr = SENS_MEM_ZALLOC(vcommDev->QUEUE_SIZE);
	vcommDev->recvPos = vcommDev->bufPtr;
	vcommDev->getPos = vcommDev->bufPtr;
	vcommDev->startPos = vcommDev->bufPtr;
	vcommDev->endPos = vcommDev->bufPtr + vcommDev->QUEUE_SIZE;
	vcommDev->dataLength = 0;
	vcommDev->dropInputCount = 0;
	fd_ptr->FLAGS      = (_mqx_uint)flags;
	
	//vcommCtx = vcommDev->DEV_INIT_DATA_PTR;
	//cmuxCtx = (CMUX_CONTEXT	*)vcommCtx->cmuxCtx;
	
	//cmuxSendData(cmuxCtx, vcommCtx->linkPort, CMUX_FRAME_SABM | CMUX_CONTROL_PF, NULL, 0);
	
	return (_mqx_int)(result);
}
//------------------------------------------------------------------------------
_mqx_int vCommClose(FILE_DEVICE_STRUCT_PTR fd_ptr)
{	IO_DEVICE_STRUCT_PTR		io_dev_ptr;
	IO_VCOMM_DEVICE_STRUCT	*vcommDev;
	_mqx_int								result = MQX_OK;
	CMUX_VCOMM							*vcommCtx;
	CMUX_CONTEXT						*cmuxCtx;
	
	io_dev_ptr     = fd_ptr->DEV_PTR;
	vcommDev = (void *)io_dev_ptr->DRIVER_INIT_PTR;
	
	(*io_dev_ptr->IO_IOCTL)(fd_ptr, IO_IOCTL_FLUSH_OUTPUT, NULL);
	
	vcommCtx = vcommDev->DEV_INIT_DATA_PTR;
	cmuxCtx = (CMUX_CONTEXT	*)vcommCtx->cmuxCtx;
	
#ifdef CMUX_DEBUG
	debugLock();
#endif
	cmuxSendData(cmuxCtx, vcommCtx->linkPort, CMUX_FRAME_DISC | CMUX_CONTROL_PF, NULL, 0);
#ifdef CMUX_DEBUG
	debugUnlock();
#endif
	
	if(--vcommDev->COUNT == 0)
	{	if(vcommDev->DEV_DEINIT)
			result = (_mqx_int)(*vcommDev->DEV_DEINIT)(vcommDev->DEV_INIT_DATA_PTR, vcommDev->DEV_INFO_PTR);
		
		SENS_MEM_FREE(vcommDev->bufPtr);
		vcommDev->bufPtr = NULL;
	}
	return (_mqx_int)(result);
}
//------------------------------------------------------------------------------
_mqx_int vCommRead(FILE_DEVICE_STRUCT_PTR fd_ptr, char *data_ptr, _mqx_int num)
{	IO_DEVICE_STRUCT_PTR		io_dev_ptr;
	IO_VCOMM_DEVICE_STRUCT	*vcommDev;
	_mqx_int readNum=0;
	
	io_dev_ptr = fd_ptr->DEV_PTR;
	vcommDev = (void *)io_dev_ptr->DRIVER_INIT_PTR;
	
	if(vcommDev->dataLength == 0)
		return 0;
	while(num)
	{	*data_ptr = *vcommDev->getPos;
		vcommDev->getPos++;
		data_ptr++;
		if(vcommDev->getPos == vcommDev->endPos)
			vcommDev->getPos = vcommDev->startPos;
		vcommDev->dataLength--;
		readNum++;
		if(!vcommDev->dataLength)
			break;
		num--;
	}
	return readNum;
}
//------------------------------------------------------------------------------
_mqx_int vCommWrite(FILE_DEVICE_STRUCT_PTR fd_ptr, char *data_ptr, _mqx_int num)
{	IO_DEVICE_STRUCT_PTR		io_dev_ptr;
	IO_VCOMM_DEVICE_STRUCT	*vcommDev;
	CMUX_VCOMM	*vcommCtx;
	CMUX_CONTEXT	*cmuxCtx;
	_mqx_int len;
	
	io_dev_ptr = fd_ptr->DEV_PTR;
	vcommDev = (void *)io_dev_ptr->DRIVER_INIT_PTR;
	vcommCtx = vcommDev->DEV_INIT_DATA_PTR;
	cmuxCtx = (CMUX_CONTEXT	*)vcommCtx->cmuxCtx;
#ifdef CMUX_DEBUG
	debugLock();
#endif
	len = cmuxSendData(cmuxCtx, vcommCtx->linkPort, CMUX_FRAME_UIH, data_ptr, num);
#ifdef CMUX_DEBUG
	debugUnlock();
#endif
	return len;
}
//------------------------------------------------------------------------------
_mqx_int vCommIoctl(FILE_DEVICE_STRUCT_PTR fd_ptr, _mqx_uint cmd, void *param_ptr)
{	IO_DEVICE_STRUCT_PTR		io_dev_ptr;
	IO_VCOMM_DEVICE_STRUCT	*vcommDev;
	_mqx_uint		result = MQX_OK;
	//_mqx_uint_ptr	uparam_ptr = (_mqx_uint_ptr)param_ptr;

   io_dev_ptr	= fd_ptr->DEV_PTR;
   vcommDev = (void *)io_dev_ptr->DRIVER_INIT_PTR;
   
	switch(cmd)
	{	case IO_IOCTL_CHAR_AVAIL:
					if(vcommDev->dataLength)
						*((bool *)param_ptr) = 1;
					else
						*((bool *)param_ptr) = 0;
					break;
		case IO_IOCTL_FLUSH_OUTPUT:
					break;
		case IO_IOCTL_SET_INIT_DATA_PTR:
					vcommDev->DEV_INIT_DATA_PTR = param_ptr;
					break;
		case IO_IOCTL_PUT_DATA:
					{	VCOMM_PUT_DATA_STRUCT *vcommPutData;
						int length;
						vcommPutData = (VCOMM_PUT_DATA_STRUCT *)param_ptr;
						
						for(length=0;length<vcommPutData->length;length++)
						{	if((vcommDev->recvPos == vcommDev->getPos) && vcommDev->dataLength)
							{	vcommDev->dropInputCount++;
								vcommPutData->buf++;
							}
							else
							{	*vcommDev->recvPos = *vcommPutData->buf;
								vcommPutData->buf++;
								vcommDev->recvPos++;
								if(vcommDev->recvPos >= vcommDev->endPos)
									vcommDev->recvPos = vcommDev->startPos;
								vcommDev->dataLength++;
							}
						}
					}
		default:
					break;
	}
	return (_mqx_int)result;
}

_mqx_uint vcommInstall(	char *identifier,
      									_mqx_uint (_CODE_PTR_ init)(void *, char *),
      									_mqx_uint (_CODE_PTR_ enable_ints)(void *),
      									_mqx_uint (_CODE_PTR_ deinit)(void *, void *),
      									void    	(_CODE_PTR_  putc)(void *, char),
      									_mqx_uint (_CODE_PTR_ ioctl)(void *, _mqx_uint, void *),
      									void			*init_data_ptr,
      									_mqx_uint	queue_size)
{	IO_VCOMM_DEVICE_STRUCT *vcommDev;
	uint32_t		result;

	vcommDev = _mem_alloc_system_zero(sizeof(IO_VCOMM_DEVICE_STRUCT));
	if(vcommDev == NULL) 
	{	return -1;
	} /* Endif */
	_mem_set_type(vcommDev, MEM_TYPE_VCOMM_DEVICE_STRUCT);    

	vcommDev->DEV_INIT          = init;
	vcommDev->DEV_ENABLE_INTS   = enable_ints;	//NULL
	vcommDev->DEV_DEINIT        = deinit;	//NULL
	vcommDev->DEV_PUTC          = putc;
	vcommDev->DEV_IOCTL         = ioctl;
	vcommDev->DEV_INIT_DATA_PTR = init_data_ptr;	//CMUX CTX
	vcommDev->QUEUE_SIZE        = queue_size;
   
	result = _io_dev_install(identifier, vCommOpen, vCommClose, vCommRead, vCommWrite, vCommIoctl, (void *)vcommDev); 
	return result;
}
#endif

#if defined SENSMINIS2 || defined SENSMINIA4_NEO || defined PLATFORM_XILINX
//------------------------------------------------------------------------------
int vCommWrite(UART_CTX *uartCtx, uint8_t *buffer, uint32_t length)
{	CMUX_VCOMM		*vcommCtx;
	CMUX_CONTEXT	*cmuxCtx;
	int len;

	vcommCtx = (CMUX_VCOMM *)uartCtx->vcommCtx;
	cmuxCtx = (CMUX_CONTEXT	*)vcommCtx->cmuxCtx;

#ifdef CMUX_DEBUG
	debugLock();
#endif
	len = cmuxSendData(cmuxCtx, vcommCtx->linkPort, CMUX_FRAME_UIH, (char *)buffer, length);
#ifdef CMUX_DEBUG
	debugUnlock();
#endif
	return len;
}
#endif

//------------------------------------------------------------------------------
const uint8_t cmuxCrctable[256] =
	{	0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75,
		0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
		0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69,
		0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
		0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D,
		0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
		0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51,
		0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
		0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05,
		0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
		0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19,
		0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
		0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D,
		0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
		0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21,
		0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
		0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95,
		0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
		0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89,
		0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
		0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD,
		0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
		0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1,
		0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
		0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5,
		0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
		0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9,
		0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
		0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD,
		0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
		0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1,
		0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF
	};
//------------------------------------------------------------------------------
uint8_t	cmuxFrameCheck(const uint8_t *input, int length)
{	uint8_t fcs = 0xFF;
	int i;
	for (i = 0; i < length; i++)
	{	fcs = cmuxCrctable[fcs ^ input[i]];
	}
	return (0xFF - fcs);
}
//------------------------------------------------------------------------------
static void cmuxFrameDestroy(CMUX_FRAME *frame)
{	if((frame->dataLength > 0) && frame->data)
	{	SENS_MEM_FREE(frame->data);
	}
	if(frame)
	{	SENS_MEM_FREE(frame);
	}
}
//------------------------------------------------------------------------------
static CMUX_FRAME *cmuxFrameParse(CMUX_BUFFER *buffer)
{	int end;
	int lengthNeeded = 5; /* channel, type, length, fcs, flag */
	uint8_t *data = NULL;
	uint8_t fcs = 0xFF;
	CMUX_FRAME *frame = NULL;

	/* Find start flag */
	while(!buffer->flagFound && CMUX_BUFFER_LENGTH(buffer) > 0)
	{	if (*buffer->readPoint == CMUX_HEAD_FLAG)
			buffer->flagFound = 1;
		INC_BUF_POINTER(buffer, buffer->readPoint);
	}
	if(!buffer->flagFound) /* no frame started */
		return NULL;

	/* skip empty frames (this causes troubles if we're using DLC 62) */
	while(CMUX_BUFFER_LENGTH(buffer) > 0 && (*buffer->readPoint == CMUX_HEAD_FLAG))
	{	INC_BUF_POINTER(buffer, buffer->readPoint);
	}

	if(CMUX_BUFFER_LENGTH(buffer) >= lengthNeeded)
	{	data = buffer->readPoint;
		frame = (CMUX_FRAME *)SENS_MEM_ZALLOC(sizeof(CMUX_FRAME));
		frame->data = NULL;

		frame->channel = ((*data & 252) >> 2);
		fcs = cmuxCrctable[fcs ^ *data];
		INC_BUF_POINTER(buffer, data);

		//frame->control = *data & (~(uint8_t)CMUX_CONTROL_PF);
		frame->control = *data;
		//frame->pf = *data & CMUX_CONTROL_PF;
		fcs = cmuxCrctable[fcs ^ *data];
		INC_BUF_POINTER(buffer, data);

		frame->dataLength = (*data & 254) >> 1;
		fcs = cmuxCrctable[fcs ^ *data];
		
		/* frame data length more than 127 bytes */
		if((*data & 1) == 0)
		{	INC_BUF_POINTER(buffer,data);
			frame->dataLength += (*data*128);
			fcs = cmuxCrctable[fcs^*data];
			lengthNeeded++;
			//LOG_D("len_need: %d, frame_data_len: %d.", lengthNeeded, frame->dataLength);
		}
		lengthNeeded += frame->dataLength;
		if(!(CMUX_BUFFER_LENGTH(buffer) >= lengthNeeded))
		{	cmuxFrameDestroy(frame);
			return NULL;
		}
		INC_BUF_POINTER(buffer, data);
		/* extract data */
		if(frame->dataLength > 0)
		{	frame->data = (unsigned char *)SENS_MEM_ZALLOC(frame->dataLength);
			if(frame->data != NULL)
			{	end = buffer->endPoint - data;
				if(frame->dataLength > end)
				{	memcpy(frame->data, data, end);
					memcpy(frame->data + end, buffer->data, frame->dataLength - end);
					data = buffer->data + (frame->dataLength - end);
				}
				else
				{	memcpy(frame->data, data, frame->dataLength);
					data += frame->dataLength;
					if(data == buffer->endPoint)
						data = buffer->data;
				}
				if(CMUX_FRAME_IS(CMUX_FRAME_UI, frame))
				{	for(end = 0; end < frame->dataLength; end++)
						fcs = cmuxCrctable[fcs ^ (frame->data[end])];
				}
			}
			else
			{	//LOG_E("Out of memory, when allocating space for frame data.");
				frame->dataLength = 0;
			}
		}
		/* check FCS */
		if(cmuxCrctable[fcs ^ (*data)] != 0xCF)
		{	//LOG_W("Dropping frame: FCS doesn't match.");
			cmuxFrameDestroy(frame);
			buffer->flagFound = 0;
			buffer->readPoint = data;
			return cmuxFrameParse(buffer);
		}
		else
		{	/* check end flag */
			INC_BUF_POINTER(buffer, data);
			if(*data != CMUX_HEAD_FLAG)
			{	//LOG_W("Dropping frame: End flag not found. Instead: %d.", *data);
				cmuxFrameDestroy(frame);
				buffer->flagFound = 0;
				buffer->readPoint = data;
				return cmuxFrameParse(buffer);
			}
			else
			{
			}
			INC_BUF_POINTER(buffer, data);
		}
		buffer->readPoint = data;
	}
	return frame;
}
//------------------------------------------------------------------------------
size_t cmuxBufferWrite(CMUX_BUFFER *buff, uint8_t *input, size_t count)
{	int c = buff->endPoint - buff->writePoint;
	
	count = MIN(count, CMUX_BUFFER_FREE(buff));
	if(count > c)
	{	memcpy(buff->writePoint, input, c);
		memcpy(buff->data, input + c, count - c);
		buff->writePoint = buff->data + (count - c);
	}
	else
	{	memcpy(buff->writePoint, input, count);
		buff->writePoint += count;
		if(buff->writePoint == buff->endPoint)
			buff->writePoint = buff->data;
	}
	return count;
}
//------------------------------------------------------------------------------
static int cmuxFramePush(CMUX_CONTEXT *cmux, int channel, CMUX_FRAME *frame)
{	UART_CTX *uartCtx;
	CMUX_VCOMM *vcommCtx;
	UART_RX_CTX *rxCtx;
	int idx=0;
	MOBILE_INSTANCE *wlInst;

	vcommCtx = &cmux->vComms[channel];
	uartCtx = vcommCtx->dev;
	rxCtx = uartCtx->rxCtx;
	uint8_t *buffer = frame->data;
	wlInst = (MOBILE_INSTANCE *)cmux->wlInst;
#ifdef CMUX_DEBUG_1
	//dbg_msg("push to channel:%d, payload length:%d\r\n", channel, frame->dataLength);
#endif
	
#if defined UART_RECV_INTER_LOCK
	
	#if 1
	if(uartCtx->id == UART_VCOMM_PPP)
	{	if(wlInst->startUsePpp)
		{	pppos_input_tcpip(wlInst->ppp, buffer, frame->dataLength);
		#if CMUX_TASK_PRIORITY > TPCIP_TASK_PRIORITY
			SENS_TIME_DELAY(2);
		#else
			portYIELD();
		#endif
			return 0;
		}
	}
	
	uint32_t head = rxCtx->head;
	uint32_t tail = rxCtx->tail;
	uint32_t length = frame->dataLength;
	
	uint32_t space = RB_SPACE(rxCtx, head, tail);
	
	if(length > space)
	{	rxCtx->dropInputCount += (length - space);
		length = space;
		rxCtx->overrun++;
		if(length == 0)	return -1;
	}
	
	uint32_t first = rxCtx->size - RB_MASK(rxCtx, head);
	if(first > length)	first = length;
	uint32_t second = length - first;
	
	memcpy(&rxCtx->buffer[RB_MASK(rxCtx, head)], buffer, first);
	if(second) memcpy(&rxCtx->buffer[0], buffer+first, second);
	head = RB_MASK(rxCtx, head + length);
	__DMB();	//data memory barrier
	rxCtx->head = head;
	
#if 0
	SENS_SEM_LOCK(uartCtx->sema);
	while(1)
	{	if(rxCtx->recvPos == rxCtx->getPos && rxCtx->length)
		{	SENS_SEM_UNLOCK(uartCtx->sema);
			SENS_TIME_DELAY(2);
			SENS_SEM_LOCK(uartCtx->sema);
		}
		else
		{	
			
			*rxCtx->recvPos = *buffer++;
			rxCtx->recvPos++;
			if(rxCtx->recvPos == rxCtx->endPos)
			{	rxCtx->recvPos = rxCtx->startPos;
			}
			rxCtx->length++;
		}
		idx++;
		if(idx >= frame->dataLength)
			break;
	}
	SENS_SEM_UNLOCK(uartCtx->sema);
#endif
	#else
	if(uartCtx->id != UART_VCOMM_PPP)
		SENS_SEM_LOCK(uartCtx->sema);
	while(1)
	{	if(rxCtx->recvPos == rxCtx->getPos && rxCtx->length)
		{	if(uartCtx->id != UART_VCOMM_PPP)
			{	SENS_SEM_UNLOCK(uartCtx->sema);
				SENS_TIME_DELAY(2);
				SENS_SEM_LOCK(uartCtx->sema);
			}
			else
			{	//dbgMsg("1.cmux->ppp %d\r\n", idx);
				xSemaphoreGive(uartCtx->sema);
				portYIELD();
				//SENS_TIME_DELAY(5);
				continue;
			}
		}
		else
		{	*rxCtx->recvPos = *buffer++;
			rxCtx->recvPos++;
			if(rxCtx->recvPos == rxCtx->endPos)
			{	rxCtx->recvPos = rxCtx->startPos;
			}
			rxCtx->length++;
		}
		idx++;
		if(idx >= frame->dataLength)
			break;
	}
	if(uartCtx->id == UART_VCOMM_PPP)
	{	//dbgMsg("cmux->ppp %d\r\n", idx);
		xSemaphoreGive(uartCtx->sema);
		portYIELD();
		//SENS_TIME_DELAY(5);
	}
	else
	{	SENS_SEM_UNLOCK(uartCtx->sema);
	}
	#endif
#else	//ndef UART_RECV_INTER_LOCK
	//uartFifoPut(0, uartCtx->id, buffer, frame->dataLength);
	SENS_SEM_LOCK(uartCtx->sema);
	//for(int idx=0;idx<frame->dataLength;idx++)
	while(1)
	{	if(rxCtx->recvPos == rxCtx->getPos && rxCtx->length)
		{	SENS_SEM_UNLOCK(uartCtx->sema);
			SENS_TIME_DELAY(10);
			//rxCtx->dropInputCount++;
			SENS_SEM_LOCK(uartCtx->sema);
			continue;
			
		}
		else
		{	*rxCtx->recvPos = *buffer++;
	#ifndef UART_USE_CRITICAL
			rxCtx->length = 1;
	#endif
			rxCtx->recvPos++;
			if(rxCtx->recvPos == rxCtx->endPos)
			{	rxCtx->recvPos = rxCtx->startPos;
			}
	#ifdef UART_USE_CRITICAL
			rxCtx->length++;
	#endif
		}
		idx++;
		if(idx >= frame->dataLength)
			break;
	}
	SENS_SEM_UNLOCK(uartCtx->sema);
	if(uartCtx->id == UART_VCOMM_PPP)
		SENS_TIME_DELAY(5);
#endif
	return 0;
}
//------------------------------------------------------------------------------
static int handleCommand(CMUX_CONTEXT *cmux, CMUX_FRAME *frame)
{	uint8_t	type;
#ifdef CMUX_DEBUG
  uint8_t signals; 
  int channel;
#endif
	int idx, typeLength, supported = 1, length;
	uint8_t *response;
	if(frame->dataLength > 0)
	{	type = frame->data[0];
		for(idx=0;(frame->dataLength > idx && (frame->data[idx] & CMUX_ADDRESS_EA) == 0);idx++);
		idx++;
		
		typeLength = idx;
		if((type & CMUX_ADDRESS_CR) == CMUX_ADDRESS_CR)
		{	while(frame->dataLength > idx)
			{	length = (length * 128) + ((frame->data[idx] & 254) >> 1);
				if((frame->data[idx] & CMUX_ADDRESS_EA) == CMUX_ADDRESS_EA)
					break;
				idx++;
			}
			idx++;
			switch((type & ~CMUX_ADDRESS_CR))
			{	case CMUX_C_CLD:
							cmux->state = CMUX_STATE_CLOSING;
							break;
				case CMUX_C_PSC:
							break;
				case CMUX_C_TEST:
							break;
				case CMUX_C_MSC:
							if((idx+1) < frame->dataLength)
							{	
#ifdef CMUX_DEBUG
                channel = ((frame->data[idx] & 252) >> 2);  
								idx++;
								signals = frame->data[idx];
								if((signals & CMUX_S_FC) == CMUX_S_FC)
									dbg_msg1("%s", "No frame allowed\r\n");
								else
									dbg_msg1("%s", "Frame allowed\r\n");
								if((signals & CMUX_S_RTC) == CMUX_S_RTC)
									dbg_msg1("%s", "Signal RTC\r\n");
								if((signals & CMUX_S_RTR) == CMUX_S_RTR)
									dbg_msg1("%s", "Signal RTR\r\n");
								if((signals & CMUX_S_IC) == CMUX_S_IC)
									dbg_msg1("%s", "Signal IC\r\n");
								if((signals & CMUX_S_DV) == CMUX_S_DV)
									dbg_msg1("%s", "Signal DV\r\n");
#endif
							}
							break;
				default:
#ifdef CMUX_DEBUG
							dbg_msg1("Unknown command %d from the control channel\r\n", type);
#endif
							response = SENS_MEM_ZALLOC(2+typeLength);
							if(response != NULL)
							{	idx = 0;
								response[idx++] = CMUX_C_NSC;
								typeLength &= 127;
								response[idx++] = CMUX_ADDRESS_EA | (typeLength << 1);
								while(typeLength--)
								{	response[idx] = frame->data[idx-2];
									idx++;
								}
								cmuxSendData(cmux, 0, CMUX_FRAME_UIH, (char *)response, idx);
								supported = 0;
								SENS_MEM_FREE(response);
							}
							break;
			}
			if(supported)
			{	frame->data[0] = frame->data[0] & ~CMUX_ADDRESS_CR;
				cmuxSendData(cmux, 0, CMUX_FRAME_UIH, (const char *)frame->data, frame->dataLength);
				if((type & ~CMUX_ADDRESS_CR) == CMUX_C_MSC)
				{	if(frame->control & CMUX_CONTROL_PF)
						cmux->uihPfBitReceived = 1;
					frame->data[0] = frame->data[0] | CMUX_ADDRESS_CR;
					cmuxSendData(cmux, 0, CMUX_FRAME_UIH, (const char *)frame->data, frame->dataLength);
				}
			}
		}
		else
		{	if(CMUX_COMMAND_IS(CMUX_C_NSC, type))
			{	
#if defined CMUX_DEBUG
				dbg_msg1("%s", RED"The mobile station didn't support the command sent\r\n"ORG_COLOR);
#else
				dbg_msg("%s", RED"The mobile station didn't support the command sent\r\n"ORG_COLOR);
#endif
			}
#ifdef CMUX_DEBUG
			else
				dbg_msg1("%s", RED"Command acknowledged by the mobile station\r\n"ORG_COLOR);
#endif
		}
	}
	return 0;
}
//------------------------------------------------------------------------------
static void cmuxRecvProcessdata(CMUX_CONTEXT *cmux, uint8_t *buf, size_t len)
{	size_t count = len;
	CMUX_FRAME *frame = NULL;
	//MOBILE_INSTANCE *wlInst = (MOBILE_INSTANCE *)cmux->wlInst;
	
#ifdef CMUX_DEBUG
	//memcpy(&cmuxRecvBuf[cmuxRecvLength++], buf, 1);
	/*memcpy(&cmuxRecvBuf[cmuxRecvLength], buf, len);
	cmuxRecvLength += len;
	if(cmuxRecvLength == MAX_UART_RECV_BUF_SIZE)
	{	cmuxRecvLength = 0;
		debugLock();
		dbg_msg1(BLUE"[CMUX] recv %d byte data:\r\n"ORG_COLOR, MAX_UART_RECV_BUF_SIZE);
		displayBufInHexNoLock(cmuxRecvBuf, MAX_UART_RECV_BUF_SIZE, __func__, __LINE__);
		debugUnlock();
	}*/
#endif

	cmuxBufferWrite(cmux->buffer, buf, count);

RE_PARSE:
	frame = cmuxFrameParse(cmux->buffer);
	if(frame != NULL)
	{	/* distribute different data */
#ifdef CMUX_DEBUG		
		debugLock();
		dbg_msg1(BLUE"[CMUX] channel %d process %d byte data:\r\n"ORG_COLOR, frame->channel, frame->dataLength);
		displayBufInHexNoLock(frame->data, frame->dataLength, __func__, __LINE__);
		debugUnlock();
#endif
		if((CMUX_FRAME_IS(CMUX_FRAME_UI, frame) || CMUX_FRAME_IS(CMUX_FRAME_UIH, frame)))
		{	//LOG_D("this is UI or UIH frame from channel(%d).", frame->channel);
			if(frame->control & CMUX_CONTROL_PF)
				cmux->uihPfBitReceived = 1;
			if(frame->channel > 0)
			{	/* receive data from logical channel, distribution them */
#ifdef CMUX_DEBUG
				debugLock();
				/*if(cmuxRecvLength)
				{	dbg_msg1(BLUE"[CMUX] channel %d recv last %d byte data:\r\n"ORG_COLOR, frame->channel, cmuxRecvLength);
					displayBufInHexNoLock(cmuxRecvBuf, cmuxRecvLength, __func__, __LINE__);
					cmuxRecvLength = 0;
				}*/
#endif
				cmuxFramePush(cmux, frame->channel, frame);			
#ifdef CMUX_DEBUG
				debugUnlock();
#endif
				//cmux_vcom_isr(cmux, frame->channel, frame->data_length);
			}
			else
			{	/* control channel command */
				//LOG_W("control channel command haven't support.");
#ifdef CMUX_DEBUG
				debugLock();
				/*if(cmuxRecvLength)
				{	dbg_msg1(BLUE"[CMUX] recv last %d byte data:\r\n"ORG_COLOR, cmuxRecvLength);
					displayBufInHexNoLock(cmuxRecvBuf, cmuxRecvLength, __func__, __LINE__);
					cmuxRecvLength = 0;
				}
				dbg_msg1("%s", "channel 0 control command\r\n");
				displayBufInHexNoLock(frame->data, frame->dataLength, __func__, __LINE__);*/
#endif
				handleCommand(cmux, frame);
#ifdef CMUX_DEBUG
				debugUnlock();
#endif
			}
		}
		else
		{	switch((frame->control & ~CMUX_CONTROL_PF))
			{	case CMUX_FRAME_UA:
							//LOG_D("This is UA frame for channel(%d).", frame->channel);
#ifdef CMUX_DEBUG
							debugLock();
							dbg_msg1(BLUE"[CMUX] UA FRAME channel :%d\r\n"ORG_COLOR, frame->channel);
							if(cmuxRecvLength)
							{	dbg_msg1(BLUE"[CMUX] UA FRAME channel :%d, recv last %d byte data:\r\n"ORG_COLOR, frame->channel, cmuxRecvLength);
								displayBufInHexNoLock(cmuxRecvBuf, cmuxRecvLength, __func__, __LINE__);
								cmuxRecvLength = 0;
							}
							debugUnlock();
#endif
							
							break;
				case CMUX_FRAME_DM:
							debugLock();
							dbg_msg1(BLUE"[CMUX] DM FRAME channel :%d\r\n"ORG_COLOR, frame->channel);
#ifdef CMUX_DEBUG
							if(cmuxRecvLength)
							{	dbg_msg1(BLUE"[CMUX] DM FRAME channel :%d, recv last %d byte data:\r\n"ORG_COLOR, frame->channel, cmuxRecvLength);
								displayBufInHexNoLock(cmuxRecvBuf, cmuxRecvLength, __func__, __LINE__);
								cmuxRecvLength = 0;
							}
#endif
							debugUnlock();
							break;
				case CMUX_FRAME_SABM:
							debugLock();
							dbg_msg1(BLUE"[CMUX] SABM FRAME recv channel : %d\r\n"ORG_COLOR, frame->channel);
#ifdef CMUX_DEBUG
							if(cmuxRecvLength)
							{	dbg_msg1(BLUE"[CMUX] SABM FRAME recv last %d byte data:\r\n"ORG_COLOR, cmuxRecvLength);
								displayBufInHexNoLock(cmuxRecvBuf, cmuxRecvLength, __func__, __LINE__);
								cmuxRecvLength = 0;
							}
#endif
							debugUnlock();
							break;
				case CMUX_FRAME_DISC:
							debugLock();
							dbg_msg1(BLUE"[CMUX] DISC FRAME channel :%d\r\n"ORG_COLOR, frame->channel);
#ifdef CMUX_DEBUG
							if(cmuxRecvLength)
							{	dbg_msg1(BLUE"[CMUX] DISC FRAME channel :%d, recv last %d byte data:\r\n"ORG_COLOR, frame->channel, cmuxRecvLength);
								displayBufInHexNoLock(cmuxRecvBuf, cmuxRecvLength, __func__, __LINE__);
								cmuxRecvLength = 0;
							}
#endif
							debugUnlock();
							break;
			}
		}
		cmuxFrameDestroy(frame);
#ifdef CMUX_RECV_USE_MASS_DATA
		if(cmux->buffer->readPoint != cmux->buffer->writePoint)
		{	goto RE_PARSE;
		}
#endif
	}
}
//------------------------------------------------------------------------------
static CMUX_BUFFER *cmuxBufferInit(void)
{	CMUX_BUFFER *buff = NULL;
	buff = SENS_MEM_ZALLOC(sizeof(CMUX_BUFFER));
	if(buff == NULL)
		return NULL;
	
	buff->readPoint = buff->data;
	buff->writePoint = buff->data;
	buff->endPoint = buff->data + MAX_CMUX_BUFFER_SIZE;
	return buff;
}
//------------------------------------------------------------------------------
/*static void vcomsCmuxFrameInit(CMUX_CONTEXT *cmux, int channel)
{	
	//LOG_D("init cmux data channel(%d) list.", channel);
}*/
//------------------------------------------------------------------------------
//int cmuxAttach(CMUX_CONTEXT *object, int linkPort, const char *aliasName, uint32_t queueSize)
int cmuxAttach(CMUX_CONTEXT *object, int linkPort, const char *aliasName, uint32_t queueSize)
{	//MOBILE_INSTANCE *wlInst = (MOBILE_INSTANCE *)object->wlInst;
	CMUX_VCOMM	*vcommCtx;
	//uint8_t	msc[5];
	if(linkPort >= object->vCommNum)
	{	//LOG_E("PORT[%02d] attach failed, please increase CMUX_PORT_NUMBER in the env.", linkPort);
		return -1;
	}

	//device = &object->vcoms[linkPort].device;
	vcommCtx = &object->vComms[linkPort];
	vcommCtx->linkPort = (uint8_t)linkPort;
	vcommCtx->cmuxCtx = (void *)object;
	//wlInst = (MOBILE_INSTANCE *)object->wlInst;
	//vcommInstall((char *)aliasName, NULL, NULL, NULL, NULL, NULL, &object->vComms[linkPort], queueSize);
#if defined PLATFORM_FSL
	vcommCtx->dev = fopen(aliasName, 0);
	ioctl(vcommCtx->dev, IO_IOCTL_SET_INIT_DATA_PTR, vcommCtx);
#else
	vcommCtx->dev = SENS_MEM_ZALLOC(sizeof(SENS_UART_CTX));
	SENS_UART_INIT(vcommCtx->dev, UART_VCOMM_CTRL+linkPort, 0, 0, queueSize, UT_VCOMM, 0);
	vcommCtx->dev->vcommCtx = (void *)vcommCtx;
#endif
	
#ifdef CMUX_DEBUG
	debugLock();
#endif
	cmuxSendData(object, vcommCtx->linkPort, CMUX_FRAME_SABM | CMUX_CONTROL_PF, NULL, 0);
#ifdef CMUX_DEBUG
	debugUnlock();
#endif	
	
#ifdef CMUX_DEBUG
	dbg_msg("vcomm %s at port %d, dev at %p, init done\r\n", aliasName, linkPort, object->vComms[linkPort].dev);
#endif
	//vcomsCmuxFrameInit(object, linkPort);
	/* interrupt mode or dma mode is meaningless, because we don't have buffer for vcom */
	return 0;
}
//------------------------------------------------------------------------------
int cmuxDetach(CMUX_CONTEXT *object, int linkPort)
{	CMUX_VCOMM *vcommCtx;
	
	vcommCtx = &object->vComms[linkPort];
	
#ifdef PLATFORM_FSL
	if(vcommCtx->dev)
		fclose(vcommCtx->dev);
#endif
	SENS_UART_DE_INIT(vcommCtx->dev);
	vcommCtx->dev = NULL;
	return 0;
}
//------------------------------------------------------------------------------
int cmuxStart(CMUX_CONTEXT *object)
{	int result = 0;

	/* attach cmux control channel into rt-thread device */
	//cmuxAttach(object, CMUX_CTRL_PORT, CMUX_CTRL_NAME, 256);
	cmuxAttach(object, CMUX_CTRL_PORT, CMUX_CTRL_NAME, 256);
	return result;
}
//------------------------------------------------------------------------------
int cmuxSendData(CMUX_CONTEXT *cmuxCtx, int port, uint8_t type, const char *data, int length)
{	/* flag, EA=1 C port, frame type, data_length 1-2 */
	uint8_t prefix[5] = {CMUX_HEAD_FLAG, CMUX_ADDRESS_EA | CMUX_ADDRESS_CR, 0, 0, 0};
	uint8_t postfix[2] = {0xFF, CMUX_HEAD_FLAG};
	int c, prefix_length = 4;
	//int temp;
	//int offset;

#ifdef CMUX_DEBUG_1
	if(cmuxDebug)
		dbg_msg("cmux send port:%d, payload length:%d\r\n", port, length);
#endif
	/* EA=1, Command, let's add address */
	prefix[1] = prefix[1] | ((CMUX_DHCL_MASK & port) << 2);
	/* cmux control field */
	prefix[2] = type;

	if((type == CMUX_FRAME_UIH || type == CMUX_FRAME_UI) && cmuxCtx->uihPfBitReceived && CMUX_COMMAND_IS(CMUX_C_MSC, data[0]))
	{	prefix[2] |= CMUX_CONTROL_PF;
		cmuxCtx->uihPfBitReceived = 0;
	}
	
	if(length > CMUX_DATA_MASK)
	{	prefix_length = 5;
		prefix[3] = ((CMUX_DATA_MASK & length) << 1);
		prefix[4] = (CMUX_HIGH_DATA_MASK & length) >> 7;
	}
	else
	{	prefix[3] = 1 | (length << 1);
	}
	/* CRC checksum */
	postfix[0] = cmuxFrameCheck(prefix + 1, prefix_length - 1);

	//dbg_msg(BLUE"cmux send port:%d, send from %p, payload length:%d\r\n"ORG_COLOR, port, cmuxCtx->dev, length);
#ifdef CMUX_DEBUG	
	//if(cmuxDebug)
	
	{	dbg_msg1("cmux send port:%d, send from %p, payload length:%d\r\n", port, cmuxCtx->dev, length);

		displayBufInHexNoLock(prefix, prefix_length, __func__, __LINE__);
		if(length)
			displayBufInHexNoLock((uint8_t *)data, length, __func__, __LINE__);
		displayBufInHexNoLock(postfix, 2, __func__, __LINE__);	
	}
#endif
	c = SENS_UART_WRITE(cmuxCtx->dev, (uint8_t *)prefix, prefix_length);
	if(c != prefix_length)
	{	//LOG_D("Couldn't write the whole prefix to the serial port for the virtual port %d. Wrote only %d  bytes.", port, c);
		return 0;
	}
	if(length > 0)
	{	/*offset = 0;
		while(length)
		{	if(length > 20)
				temp = 20;
			else
				temp = length;
			c = SENS_UART_WRITE(cmuxCtx->dev, (uint8_t *)&data[offset], temp);	
			if(temp != c)
			{	return 0;
			}
			length -= temp;	
			offset += temp;
		}*/
		/*if(port == 2)
		{	dbgMsg("send Mobile %d\r\n", length);
		}*/
		c = SENS_UART_WRITE(cmuxCtx->dev, (uint8_t *)data, length);
		if(length != c)
		{	//LOG_D("Couldn't write all data to the serial port from the virtual port %d. Wrote only %d bytes.", port, c);
			return 0;
		}
	}
	c = SENS_UART_WRITE(cmuxCtx->dev, (uint8_t *)postfix, 2);
	if (c != 2)
	{	//LOG_D("Couldn't write the whole postfix to the serial port for the virtual port %d. Wrote only %d bytes.", port, c);
		return 0;
	}
	return length;
}
//------------------------------------------------------------------------------
int cmuxInit(CMUX_CONTEXT *object, uint8_t vCommNum)
{	object->vCommNum = vCommNum;
	object->vComms = SENS_MEM_ZALLOC(vCommNum * sizeof(CMUX_VCOMM));
	if(object->vComms == NULL)
	{	return -1;
	}

	object->buffer = cmuxBufferInit();
	if(object->buffer == NULL)
	{	//LOG_E("cmux buffer malloc failed.");
		return -1;
	}
	return 0;
}
//------------------------------------------------------------------------------
#if defined OS_MQX
void cMuxTask(uint32_t temp)
#elif defined OS_FREERTOS
void cMuxTask(void *param)
#endif
{	MOBILE_INSTANCE *wlInst;
	NET_INSTANCE	*workingInst;
  
#if defined CMUX_RECV_USE_MASS_DATA
	uint8_t	*recvBuf;
	uint32_t recvLen;
	uint32_t readLen;
#else
	char OneByteN;
#endif
	
  //
  //uint8_t run=0;
  CMUX_CONTEXT *cMuxCtx;
  
#ifdef CMUX_DEBUG
	cmuxRecvLength = 0;
#endif
#ifdef CMUX_RECV_USE_MASS_DATA
	//recvBuf = SENS_MEM_ZALLOC(1536);
	recvBuf = SENS_MEM_ZALLOC(1024);
#endif
	//SENS_SEM_LOCK(taskAct.cMuxTask.lock);
#if defined UART_RECV_INTER_LOCK_IRQ
	//SENS_SEM_LOCK(taskAct.cMuxTask.lock);
	while(1)
	{	if(networkCtx == NULL)
			goto CMUX_TASK_WAIT;
		if(networkCtx->netInst == NULL)
			goto CMUX_TASK_WAIT;
		netInst = networkCtx->netInst;
		wlInst = netInst->wlInst;
		if(wlInst == NULL)
			goto CMUX_TASK_WAIT;
		cMuxCtx = (CMUX_CONTEXT *)wlInst->cMuxCtx;
		if(cMuxCtx != NULL)
			goto CMUX_TASK_RUN;
CMUX_TASK_WAIT:
		taskAct.cMuxTask.sts = TASK_STS_BLOCKED;
		SENS_TIME_DELAY(2);	
		continue;
CMUX_TASK_RUN:
		taskAct.cMuxTask.sts = TASK_STS_BLOCKED;
		xSemaphoreTake(cMuxCtx->dev->sema, portMAX_DELAY);
		taskAct.cMuxTask.sts = TASK_STS_RUNNING;
		taskAct.cMuxTask.runningStartTime = GetTimeTicks();
		while(SENS_UART_STATUS(cMuxCtx->dev))
		{	SENS_UART_READ(cMuxCtx->dev, (uint8_t *)&OneByteN, 1);
#ifdef CMUX_DEBUG
				//dbg_msg("CMUX recv %X\r\n", OneByteN);
#endif
			cmuxRecvProcessdata(cMuxCtx, (uint8_t *)&OneByteN, 1);
		}
	}
#else
	//SENS_SEM_UNLOCK(taskAct.cMuxTask.lock);
	while(1)
	{	//taskAct.cMuxTask.sts = TASK_STS_BLOCKED;
		SENS_SEM_LOCK(taskAct.cMuxTaskAct.lock);
		taskAct.cMuxTaskAct.sts = TASK_STS_RUNNING;
		taskAct.cMuxTaskAct.runningStartTime = GetTimeTicks();
		
		while(1)
		{	if(networkCtx == NULL)
				break;
			if(networkCtx->workingInst == NULL)
				break;
			workingInst = networkCtx->workingInst;
			wlInst = workingInst->wlInst;
			if(wlInst == NULL)
				break;
			cMuxCtx = (CMUX_CONTEXT *)wlInst->cMuxCtx;
			if(cMuxCtx == NULL)
				break;
			
			//if(!cMuxCtx->start)
			//	break;
#if defined CMUX_RECV_USE_MASS_DATA
			recvLen = SENS_UART_STATUS(cMuxCtx->dev);
			if(recvLen)
			{	/*if(recvLen > 32)
					recvLen = 32;*/
				if(recvLen > 1024)
					recvLen = 1024;
				readLen = SENS_UART_READ(cMuxCtx->dev, (uint8_t *)recvBuf, recvLen);
				if(readLen != recvLen)
				{	dbgMsg("CMUX Recv error, %d, %d\r\n", recvLen, readLen);
				}
				//SENS_UART_READ(cMuxCtx->dev, (uint8_t *)recvBuf, recvLen);
				cmuxRecvProcessdata(cMuxCtx, (uint8_t *)recvBuf, recvLen);
			}
#else
			if(SENS_UART_STATUS(cMuxCtx->dev))
			{	SENS_UART_READ(cMuxCtx->dev, (uint8_t *)&OneByteN, 1);
#ifdef CMUX_DEBUG
				//dbg_msg("CMUX recv %X\r\n", OneByteN);
#endif
				//recvLen = SENS_UART_READ(cMuxCtx->dev, recvBuf, 32);
				cmuxRecvProcessdata(cMuxCtx, (uint8_t *)&OneByteN, 1);
			}
#endif
			else
			{	//SENS_TIME_DELAY(1);
				break;
			}  
		}
		SENS_SEM_UNLOCK(taskAct.cMuxTaskAct.lock);
		SENS_TIME_DELAY(1);
	}
#endif	
}

#endif
