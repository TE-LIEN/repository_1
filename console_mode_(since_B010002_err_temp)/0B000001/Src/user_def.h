#include "stdint.h"

#define SUPPORT_USB_STILL_CAPTURE   1
#define STREAMING_ENABLE            0
#define SAVE_TO_SD                  1

#define SELECT_STILL_RES_WIDTH   (2592)		//2592	//1280	//960		//848		//640		//640		//352		//320		//2560	//2048	//1920
#define SELECT_STILL_RES_HEIGHT  (1944)  	//1944	//720		//540		//480		//480		//400		//288		//240		//1440	//1536	//1080

// #define IMAGE_MAX_SIZE       (SELECT_STILL_RES_WIDTH*SELECT_STILL_RES_HEIGHT*2)
//######define IMAGE_MAX_SIZE       (1 * 512 * 1024)	//1572864 //(1*1048576)

// #define IMAGE_BUFF_CNT       4
#define IMAGE_BUFF_CNT       2

#define IGNORE_IMG_FRAME_CNT	0

#define START_CAPTURE           0x01
#define END_CAPTURE             0x00

#define FORCE_RE_SETTING_RTC 0//1

// #define EXT_SRAM_SIZE				(2 * 1024 * 1024)

#define DEBUG_UVC               0      /* set to 0 to disable diagnostic messages */
#define ON				1
#define OFF				0
