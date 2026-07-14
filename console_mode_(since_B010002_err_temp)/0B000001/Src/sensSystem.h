#ifndef __SENS_SYSTEM_H__
#define __SENS_SYSTEM_H__

#include "sensminiCfg.h"

#define BOOT_ZONE2_FILE_NAME	"bootZone2.bin"

extern void prvSetupHardware( void ); 
extern void devTimeGet(void);
extern void checkPowerSaving(void);

extern volatile SENS_SYSTEM_STRUCT	*sensSys;
extern volatile SYS_CONFIG			*sysCfg;
extern volatile SYS_CTRL			*sysCtrl;
extern volatile SYS_TIMER			*sysTimer;

extern volatile uint8_t gRstWakeupTimerDone;

extern volatile TASK_ACTIVE taskAct;


extern void extendTaskTimer(TASK_INFO *actTask);
extern void resetServIdleXmitTimer(uint8_t setIdx);
//extern int chkServIdleXmitTimer(uint8_t chkIdx, long timeout);
extern void prepareDataToCloud(void);

extern void setPsmStatus(uint8_t setOrClr, uint32_t bmp);

extern CLOUD_SERVER_INFO *getServerInfoById(int idx);
extern CLOUD_SERVER_INFO *getServerInfoByServInst(CLOUD_SERVER_INSTANCE *servInst);

extern void setUploadImgFsmChg(IMAGE_UPLOAD_INSTANCE *imgUploadInst, enum IMG_UPLOAD_FSM next, uint32_t timeout);
extern void checkAndTriggerCamera(int currHistoryRecTimeSlot);
extern void pollWebImgCapture(void);
extern int isTimeToRecPq(void);
extern void recordDiHistory(void);
extern void powerSaving(void);
extern void rebootCheck(void);
extern void InternalDiProcess(void);
extern int isTimeToSendAlertImg(void);
extern void alarmProcess(void);
extern void checkPrevAlarm(void);

extern void watchDogClr(void);
extern void diWakeupProcess(void);
extern void clearDiReleaseFlag(int releaseFlag);
extern void cloudDataSendDoneProcess(CLOUD_SERVER_INSTANCE *serverInst);
extern void loadBootZone2ToSpi(void);

extern void checkPrevOtaStatus(void);
extern void startTrigOta(void);
extern int checkOta(void);
extern int checkPrevOta(void);
#endif