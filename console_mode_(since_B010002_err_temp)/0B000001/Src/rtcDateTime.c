#include "rtcDateTime.h"
#include "sensSystem.h"
#include "sensCommon.h"
#include "user_def.h"

#ifdef NUVOTON

const uint16_t timeDaysInMonth[2][13] =
{	{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};


#define CLK_FIRST_YEAR	(1970UL)
#define NUM_LEAP_YEAR_SINCE_FIRST_YEAR  (CLK_FIRST_YEAR/4 - CLK_FIRST_YEAR/100 + CLK_FIRST_YEAR/400)
#define MAXIMUM_SECONDS_IN_TIME  (4102444799UL)


volatile uint32_t rtcIncTick;

#if defined SENSMINIA4_PRO || defined SENSMINIA4_NEO
#define SENS_RTC_GET_ALARM_INT_FLAG(x)		RTC_GET_ALARM_INT_FLAG()
#define SENS_RTC_CLEAR_ALARM_INT_FLAG(x)	RTC_CLEAR_ALARM_INT_FLAG()
#define SENS_RTC_GET_TICK_INT_FLAG(x)		RTC_GET_TICK_INT_FLAG()
#define SENS_RTC_CLEAR_TICK_INT_FLAG(x)		RTC_CLEAR_TICK_INT_FLAG()
#elif defined SENSMINIS2
#define SENS_RTC_GET_ALARM_INT_FLAG(x)		RTC_GET_ALARM_INT_FLAG(x)
#define SENS_RTC_CLEAR_ALARM_INT_FLAG(x)	RTC_CLEAR_ALARM_INT_FLAG(x)
#define SENS_RTC_GET_TICK_INT_FLAG(x)		RTC_GET_TICK_INT_FLAG(x)
#define SENS_RTC_CLEAR_TICK_INT_FLAG(x)		RTC_CLEAR_TICK_INT_FLAG(x)

#endif

void RTC_TickHandle(void)
{	S_RTC_TIME_DATA_T *dateTime;
	SYS_TIMER *sysTimer;
	
	if(sensSys)
	{	sysTimer = (SYS_TIMER *)&sensSys->sysTimer;
		dateTime = (S_RTC_TIME_DATA_T *)&sensSys->dateTime;
		// Get the current time
		rtcIncTick = GetTimeTicks();
		RTC_GetDateAndTime(dateTime);
		if(sysTimer->currentTimeStamp)
			sysTimer->currentTimeStamp++;
	}
}


void RTC_IRQHandler(void)
{	//uint32_t sprReg;
	if(SENS_RTC_GET_ALARM_INT_FLAG(RTC) == 1)
	{	/* Clear RTC alarm interrupt flag */
		SENS_RTC_CLEAR_ALARM_INT_FLAG(RTC);
		SYS_UnlockReg();
		while(1)
		{	SYS->IPRST0 |= SYS_IPRST0_CHIPRST_Msk;	//reset chip
		}
	}
	
	if(SENS_RTC_GET_TICK_INT_FLAG(RTC) == 1)
	{	/* Clear RTC tick interrupt flag */
		SENS_RTC_CLEAR_TICK_INT_FLAG(RTC);
		RTC_TickHandle();
	}
}

void setRtcAlarm(S_RTC_TIME_DATA_T *dateTime)
{	dbgMsg("set Alarm : %04d/%02d/%02d %02d:%02d:%02d, time scale:%d\r\n", dateTime->u32Year, dateTime->u32Month, dateTime->u32Day, dateTime->u32Hour, dateTime->u32Minute, dateTime->u32Second, dateTime->u32TimeScale);
	dateTime->u32TimeScale = RTC_CLOCK_24;
	RTC_SetAlarmDateAndTime(dateTime);
	
	RTC_GetAlarmDateAndTime(dateTime);
	dbgMsg("real set Alarm : %04d/%02d/%02d %02d:%02d:%02d, time scale:%d\r\n", dateTime->u32Year, dateTime->u32Month, dateTime->u32Day, dateTime->u32Hour, dateTime->u32Minute, dateTime->u32Second, dateTime->u32TimeScale);
	
	SENS_RTC_CLEAR_ALARM_INT_FLAG(RTC);
	RTC_EnableInt(RTC_INTEN_ALMIEN_Msk);
}


void rtcInit(void)
{	uint32_t u32TimeOutCount = RTC_TIMEOUT;
	SYS_TIMER *sysTimer;
	S_RTC_TIME_DATA_T *dateTime;
	/*S_RTC_TIME_DATA_T *dateTime;
	dateTime = (S_RTC_TIME_DATA_T *)&sensSys->dateTime;
	
	dateTime->u32Year = 2022;
	dateTime->u32Month = 9;
	dateTime->u32Day = 28;
	dateTime->u32Hour = 13;
	dateTime->u32Minute = 30;
	dateTime->u32Second = 0;
	dateTime->u32DayOfWeek = RTC_WEDNESDAY;
	dateTime->u32TimeScale = RTC_CLOCK_24;
	
	RTC_Open(dateTime);*/
	//RTC_Open(NULL);
	
	// if(!(RTC->INIT & RTC_INIT_ACTIVE_Msk))
	{	RTC->INIT = RTC_INIT_KEY;
		if(RTC->INIT != RTC_INIT_ACTIVE_Msk)
		{	RTC->INIT = RTC_INIT_KEY;
			while(RTC->INIT != RTC_INIT_ACTIVE_Msk)
			{	if(--u32TimeOutCount == 0) 
					break;
			}
		}
	}
	RTC->CLKFMT |= RTC_CLKFMT_DCOMPEN_Msk;
	//RTC->CLKFMT &= ~RTC_CLKFMT_DCOMPEN_Msk;
	
	//RTC_32KCalibration(327680000);
	
	RTC_SetTickPeriod(RTC_TICK_1_SEC);
	RTC_EnableInt(RTC_INTEN_TICKIEN_Msk);
	//NVIC_SetPriority(RTC_IRQn, 6);	//yushun add for taskENTER_CRITICAL
#ifdef SENSMINIA4_NEO
	NVIC_SetPriority(RTC_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
#endif
	NVIC_EnableIRQ(RTC_IRQn);
	//sysTimer = (SYS_TIMER *)&sensSys->sysTimer;
	
	dateTime = (S_RTC_TIME_DATA_T *)&sensSys->dateTime;
	/* Get the current time */
	rtcIncTick = GetTimeTicks();
	RTC_GetDateAndTime(dateTime);
	//sysTimer->currentTimeStamp
}

void setTime(S_RTC_TIME_DATA_T *dateTime)
{	SYS_TIMER *sysTimer;
	
	sysTimer = (SYS_TIMER *)&sensSys->sysTimer;
	taskENTER_CRITICAL();
	dateTime->u32TimeScale = RTC_CLOCK_24;
	dateTime->u32AmPm = RTC_AM;
	if(dateTime->u32Hour >= 12)
	{	dateTime->u32AmPm = RTC_PM;
	}
	RTC_SetDateAndTime(dateTime);
	/* Get the current time */
	//must add delay...
	//RTC_32KCalibration(327680000);
	RTC_GetDateAndTime((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
	sysTimer->currentTimeStamp = dateToTimestamp((S_RTC_TIME_DATA_T *)&sensSys->dateTime);
	rtcIncTick = GetTimeTicks();
	taskEXIT_CRITICAL();
}

static uint32_t _is_leap_year( uint16_t year )
{	uint32_t result = 0;
	if( year % 4 == 0 )
	{	/* leap year, add day */
		result = 1;
		if( year % 100 == 0)
		{	/* is not leap year, substract day*/
			result = 0;
		}
		if( year % 400 == 0)
		{	/* finally is leap year, get back one more day */
			result = 1;
		}
	}
	return result;
}

uint32_t dateToTimestamp(S_RTC_TIME_DATA_T *date)
{
	uint32_t time, day, month, yday;
	uint32_t leap;
	
	while(1)
	{	leap = _is_leap_year(date->u32Year);
		
		if (date->u32Day > timeDaysInMonth[leap][date->u32Month])
		{	date->u32Day -= timeDaysInMonth[leap][date->u32Month];
			date->u32Month++;
		}
		else if (date->u32Day < 1U)
		{ /* The day of month starts from 1 */
			date->u32Day += (uint16_t) timeDaysInMonth[leap][date->u32Month];
			date->u32Month--;
		}
		else
		{	break;
		}/* Endif */

		if (date->u32Month == 13U)
		{	date->u32Month = 1;
			date->u32Year ++;
		}
		else if (date->u32Month == 0U)
		{	date->u32Month = 12;
			date->u32Year --;
		}
	}
	
	if (date->u32Year < (uint16_t) CLK_FIRST_YEAR)
	{ /* Year must be larger than or equal to CLK_FIRST_YEAR (1970)*/
		return 0;
	} /* Endif */

	/*
	 * Determine the number of days since Jan 1, 1970 at 00:00:00
	 */
	day = (date->u32Year - CLK_FIRST_YEAR) * 365;
	/* Add the leap days from 1970 to YEAR */
	day += date->u32Year / 4 - date->u32Year / 100 + date->u32Year / 400 - NUM_LEAP_YEAR_SINCE_FIRST_YEAR;

	/* Find out if we are in a leap year. */
	leap = (uint32_t) _is_leap_year(date->u32Year);
	if(leap)
		day --;

	/*
	 * Add the number of day since Jan 1, to the first day of month.
	 */
	month = date->u32Month;
	yday = 0;
	while (--month > 0)
	{	yday += timeDaysInMonth[leap][month];
	} /* End while */

	/*
	 * Add the number of days since the beginning of the month
	 */
	yday += (uint32_t)date->u32Day - 1 ;
	day  += yday;
	/*
	 * Add the number of seconds in the hours since midnight
	 */
	time = (uint32_t) date->u32Hour * 3600;

	/*
	 * Add the number of seconds in the minutes since the hour
	 */
	time += (uint32_t) date->u32Minute * 60;

	/*
	 * add the number of seconds since the beginning of the minute
	 */
	time += (uint32_t) date->u32Second;


	/* Check if overflow */
	if ( ((MAXIMUM_SECONDS_IN_TIME - time)/ 86400UL) < day )
	{	return 0;
	}
	/*
	 * assign the times
	 */
	time += day * 86400UL;
	
	return time;
}

void timestampToDateTime(uint32_t timestamp, S_RTC_TIME_DATA_T *dateTime)
{	uint32_t	time;
  uint32_t	day, year, tmp;
  int  leap;
	
	time = timestamp;
	day = time / 86400U;
	time -= day * 86400U;
	
	dateTime->u32Hour = time / 3600;
	time -= (dateTime->u32Hour * 3600);
	dateTime->u32Minute = time / 60;
	time -= (dateTime->u32Minute * 60);
	dateTime->u32Second = time;
	dateTime->u32TimeScale = RTC_CLOCK_24;
	
	dateTime->u32DayOfWeek = ((day + 4) % 7);	//1970/1/1 us Thursday
	day += 365;
	year = 4 * (day / 1461);
	day -= year * 1461 / 4;
	tmp = day / 365;
	if(tmp == 4)
		tmp = 3;
	day -= tmp * 365;
	dateTime->u32Year = year + tmp + CLK_FIRST_YEAR - 1;
	
	leap = _is_leap_year(dateTime->u32Year);
	dateTime->u32Month = 1;
	
	while(day >= timeDaysInMonth[leap][dateTime->u32Month])
	{	day -= timeDaysInMonth[leap][dateTime->u32Month];
		dateTime->u32Month++;
	}
	dateTime->u32Day = day;
	
	dateTime->u32Day++;
	dateTime->u32AmPm = RTC_AM;
	if(dateTime->u32Hour >= 12)
	{	dateTime->u32AmPm = RTC_PM;
	}
	dateTime->u32TimeScale = RTC_CLOCK_24;
	return;
}

void timestampToSetRtc(uint32_t timestamp)
{	S_RTC_TIME_DATA_T dateTime;
	timestampToDateTime(timestamp, &dateTime);
	dbgMsg1("[RTC] set new time %04d/%02d/%02d %02d:%02d:%02d, week of day:%d\r\n", dateTime.u32Year, 
																					dateTime.u32Month,
																					dateTime.u32Day,
																					dateTime.u32Hour,
																					dateTime.u32Minute,
																					dateTime.u32Second,
																					dateTime.u32DayOfWeek+1);
	memcpy((void *)&sensSys->dateTime, (void *)&dateTime, sizeof(S_RTC_TIME_DATA_T));
	setTime(&dateTime);
}

unsigned long get_fattime(void)	//yushun add for get correct time
{	//S_RTC_TIME_DATA_T date;
	//_rtc_get_time_mqxd(&date);

	return	((unsigned long)(sensSys->dateTime.u32Year - 1980U) << 25U)	/* Fixed to Jan. 1, 2010 */
		| ((unsigned long)sensSys->dateTime.u32Month << 21)
		| ((unsigned long)sensSys->dateTime.u32Day << 16)
		| ((unsigned long)sensSys->dateTime.u32Hour << 11)
		| ((unsigned long)sensSys->dateTime.u32Minute << 5)
		| ((unsigned long)sensSys->dateTime.u32Second >> 1);
}

void timezoneDiffMobile(S_RTC_TIME_DATA_T *dateTimeStruct, int32_t diff, int32_t dayLightSaving)
{	uint32_t timestamp;
	int timeDiff;

	timestamp = dateToTimestamp(dateTimeStruct);
	
	timeDiff = (diff *15)*60;
	dayLightSaving *= 3600;
	
	timestamp = timestamp - timeDiff - dayLightSaving;
	timestampToDateTime(timestamp, dateTimeStruct);
}

void sntpSetSysTime(uint32_t timestamp)
{	timestampToSetRtc(timestamp);
}

#endif