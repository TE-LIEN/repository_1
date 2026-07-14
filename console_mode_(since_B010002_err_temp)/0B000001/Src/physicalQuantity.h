#ifndef __PHYSICAL_QUANTITY_H__
#define __PHYSICAL_QUANTITY_H__

#include "sensminiCfg.h"

typedef struct _KALMAN_STRUCT
{	float preData;
	float preDataKalman;
}KALMAN_STRUCT;

typedef struct _ALOG_STD
{	float 		*array;
	//float			*stdArray;
	//float			std;
	//float 		average;
	uint32_t 	arrayIndex;
	uint32_t	arraySize;
	uint32_t 	arrayOffset;
	//uint32_t	stdCnt;
}ALOG_STD;

typedef struct _ALOG_MOVING
{	float		*arrays;
	float 		average;
	uint32_t 	avgIndex;
	uint32_t 	maxAvgSize;
}ALOG_MOVING;

typedef struct _PQ_DATA_STRUCT
{	struct _PQ_DATA_STRUCT	*next;
	int8_t					pqIdx;	//put to pqVal array idx
	int8_t					devIdx;		//0 is on board, 1~ is ext
	int8_t					measureIdx;
	int8_t					formulaIdx;
	uint8_t					filterType;
	uint8_t					pqIsReady;
	volatile float			pqVal;
	KALMAN_STRUCT			*kalmanAlog;
	ALOG_STD				*stdAlog;
	ALOG_MOVING				*movingAlog;
	char					*serv1Alias;
#if SUPPORT_IOA_WEB_API
	char					*serv2Alias;
#endif
}PQ_DATA_STRUCT;

typedef struct _SENS_PQ_STRUCT
{	uint32_t		pqNumber;
	uint32_t		*pqGetMap;
	uint32_t		*pqGetMapTemp;
	uint32_t		bytesPerRec;
	PQ_DATA_STRUCT	*pqData;
	volatile float	*pqVal;
	volatile float	*onboardPq;
	uint32_t		startTime;
}SENS_PQ_STRUCT;


extern volatile SENS_PQ_STRUCT	*sensPq;

extern void initPq(void);
extern void putValToMapPqData(int16_t devIdx, int16_t measureIdx, float val);
extern void pqDataToCloudPq(void);
extern void addPqData(PQ_DATA_STRUCT *newPqData);
extern void checkAndSetOnboardPqIsRdy(void);
extern void addFormula(FORMULA_STRUCT *newFormula);
//extern void setOnboardPqToNan(void);
extern void alertCheck(void);

extern float kalmanFilter(KALMAN_STRUCT *kalman, float inData);
extern float stdFilter(ALOG_STD *stdAlog, float inData, float stdMultiple, int *sts);
extern PQ_CONFIG *getPqConfig(int pqIdx);

extern void addPqCfg(PQ_CONFIG *pqCfg);
extern int getTotalPq(void);

extern int getHwPqWithFilter(SENSOR_HW_PQ_STRUCT *hwPqInf);

#endif
