#ifndef __RS485_H__
#define __RS485_H__

#include "sensminiCfg.h"


extern void setRs485Dir(int channel, uint8_t direction);
extern int rs485SendData(int channel, uint8_t *buf, uint32_t length);
extern void rs485Init(int channel);
#endif
