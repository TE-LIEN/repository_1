#ifndef __RTC_DATE_TIME_H__
#define __RTC_DATE_TIME_H__


#include "sensminiCfg.h"


extern void rtcInit(void);
extern uint32_t dateToTimestamp(S_RTC_TIME_DATA_T *date);

extern void setTime(S_RTC_TIME_DATA_T *dateTime);
extern void timestampToSetRtc(uint32_t timestamp);
extern void RTC_TickHandle(void);
extern void timestampToDateTime(uint32_t timestamp, S_RTC_TIME_DATA_T *dateTime);

extern unsigned long get_fattime(void);
extern void setRtcAlarm(S_RTC_TIME_DATA_T *dateTime);
extern void timezoneDiffMobile(S_RTC_TIME_DATA_T *dateTimeStruct, int32_t diff, int32_t dayLightSaving);

#endif