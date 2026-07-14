#ifndef __CMUX_H__
#define __CMUX_H__

#include "sensminiCfg.h"

#ifdef PPP_USE_CMUX

#define MAX_CMUX_BUFFER_SIZE		16384

typedef enum _CMUX_STATES
{	CMUX_STATE_OPENING,
	CMUX_STATE_INITILIZING,
	CMUX_STATE_MUXING,
	CMUX_STATE_CLOSING,
	CMUX_STATE_OFF,
	CMUX_STATES_COUNT // keep this the last
}CMUX_STATES;


typedef struct _CMUX_BUFFER
{	uint8_t	data[MAX_CMUX_BUFFER_SIZE];
	uint8_t	*readPoint;
	uint8_t	*writePoint;
	uint8_t	*endPoint;
	int			flagFound;
}CMUX_BUFFER;


typedef struct _CMUX_FRAME
{	uint8_t		channel;
	uint8_t		control;
	uint8_t		pf;
	uint32_t	dataLength;
	uint8_t		*data;
}CMUX_FRAME;


typedef struct _CMUX_VCOMM
{	UART_CTX				*dev;
	uint8_t					linkPort;
	void 						*cmuxCtx;
	uint8_t					msc;
}CMUX_VCOMM;


typedef struct _CMUX_CONTEXT
{	UART_CTX				*dev;
	void						*wlInst;
	CMUX_BUFFER			*buffer;
	//CMUX_FRAME			*frame;
	CMUX_VCOMM			*vComms;
	uint8_t					vCommNum;
	uint8_t					start;
	uint8_t					uihPfBitReceived;
	CMUX_STATES			state;
	
}CMUX_CONTEXT;

#define CMUX_CTRL_NAME	"cmux_ctrl:"
#define CMUX_CTRL_PORT	0
#define CMUX_PPP_NAME	"cmux_ppp:"
#define CMUX_PPP_PORT	2
#define CMUX_AT_NAME "cmux_at:"
#define CMUX_AT_PORT	1

#define CMUX_MAX_PORT	3

// bits: Poll/final, Command/Response, Extension
#define CMUX_CONTROL_PF 16
#define CMUX_ADDRESS_CR 2
#define CMUX_ADDRESS_EA 1
// the types of the frames
#define CMUX_FRAME_SABM 47
#define CMUX_FRAME_UA 99
#define CMUX_FRAME_DM 15
#define CMUX_FRAME_DISC 67
#define CMUX_FRAME_UIH 239
#define CMUX_FRAME_UI 3
// the types of the control channel commands
#define CMUX_C_CLD 193
#define CMUX_C_TEST 33
#define CMUX_C_MSC 225
#define CMUX_C_NSC 17
#define CMUX_C_PSC	65
// basic mode flag for frame start and end
#define CMUX_HEAD_FLAG (unsigned char)0xF9


#define CMUX_S_FC 2
#define CMUX_S_RTC 4
#define CMUX_S_RTR 8
#define CMUX_S_IC 64
#define CMUX_S_DV 128

#define CMUX_DHCL_MASK       63         /* DLCI number is port number, 63 is the mask of DLCI; C/R bit is 1 when we send data */
#define CMUX_DATA_MASK       127        /* when data length is out of 127( 0111 1111 ), we must use two bytes to describe data length in the cmux frame */
#define CMUX_HIGH_DATA_MASK  32640      /* 32640 (? 0111 1111 1000 0000 ?), the mask of high data bits */

#define CMUX_COMMAND_IS(command, type) ((type & ~CMUX_ADDRESS_CR) == command)
#define CMUX_PF_ISSET(frame) ((frame->control & CMUX_CONTROL_PF) == CMUX_CONTROL_PF)
#define CMUX_FRAME_IS(type, frame) ((frame->control & ~CMUX_CONTROL_PF) == type)

//#define MIN(a, b) ((a) <= (b) ? (a) : (b))

#define INC_BUF_POINTER(buf, p)		do          \
																	{	(p)++;		\
    																if((p) == (buf)->endPoint)	\
    																	(p) = (buf)->data;	\
    															}while(0)

/* Tells, how many chars are saved into the buffer */
#define CMUX_BUFFER_LENGTH(buff) (((buff)->readPoint > (buff)->writePoint) ? (MAX_CMUX_BUFFER_SIZE - ((buff)->readPoint - (buff)->writePoint)) : ((buff)->writePoint - (buff)->readPoint))

/* Tells, how much free space there is in the buffer */
#define CMUX_BUFFER_FREE(buff) (((buff)->readPoint > (buff)->writePoint) ? ((buff)->readPoint - (buff)->writePoint) : (MAX_CMUX_BUFFER_SIZE - ((buff)->writePoint - (buff)->readPoint)))

#define CMUX_RECV_READ_MAX 32

//#define CMUX_THREAD_PRIORITY 8

#define CMUX_RECIEVE_RESET 0
#define CMUX_RECIEVE_BEGIN 1
#define CMUX_RECIEVE_PROCESS 2
#define CMUX_RECIEVE_END 3

#define CMUX_EVENT_RX_NOTIFY 1 /* serial incoming a byte */
#define CMUX_EVENT_CHANNEL_OPEN 2
#define CMUX_EVENT_CHANNEL_CLOSE 4
#define CMUX_EVENT_CHANNEL_OPEN_REQ 8
#define CMUX_EVENT_CHANNEL_CLOSE_REQ 16
#define CMUX_EVENT_FUNCTION_EXIT 32

extern void cMuxTask(void *param);

extern int cmuxInit(CMUX_CONTEXT *object, uint8_t vCommNum);
extern int cmuxSendData(CMUX_CONTEXT *cmuxCtx, int port, uint8_t type, const char *data, int length);
extern int cmuxStart(CMUX_CONTEXT *object);
//extern int cmuxAttach(CMUX_CONTEXT *object, int linkPort, const char *aliasName, uint32_t queueSize);
extern int cmuxAttach(CMUX_CONTEXT *object, int linkPort, const char *aliasName, uint32_t queueSize);
extern int cmuxDetach(CMUX_CONTEXT *object, int linkPort);

extern int vCommWrite(UART_CTX *uartCtx, uint8_t *buffer, uint32_t length);

#endif	//#ifdef PPP_USE_CMUX

#endif
