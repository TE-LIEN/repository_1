/******************************************************************************
 * @file     SDGlue.c
 * @version  V1.00
 * $Revision: 4 $
 * $Date: 16/07/01 1:14p $
 * @brief    SD glue functions for FATFS
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
//#include <stdio.h>
//#include <stdlib.h>
#include "sensminiCfg.h"
#include <string.h>
#include "diskio.h"     /* FatFs lower layer API */
//#include "ff.h"     /* FatFs lower layer API */

#include "sensSystem.h"
#include "sensCommon.h"
#include "FS/fsCtrl.h"

//static FATFS  _FatfsVolSd0;
//static FATFS  _FatfsVolSd1;

//static TCHAR  _Path[3];

int SDH_Open_Disk(SDH_T *sdh, uint32_t u32CardDetSrc)
{	SDH_Open(sdh, u32CardDetSrc);
	if(SDH_Probe(sdh))
	{	dbgMsg("%s", "SD initial fail!!\r\n");
		return -1;
	}

	//_Path[1] = ':';
	//_Path[2] = 0;
	if((sdh == SDH0) 
#ifdef SENSMINIS2
		|| (sdh == SDH0_NS)
#endif
	)
	{	//_Path[0] = '0';
		mountFs(FS_TF, 0);
		//f_mount(&_FatfsVolSd0, _Path, 1);
		sensSys->sdInst.sdPresence = 1;
	}
	/*
	else
	{	_Path[0] = '1';
		f_mount(&_FatfsVolSd1, _Path, 1);
	}*/
	
	return 0;
}

void SDH_Close_Disk(SDH_T *sdh)
{	if((sdh == SDH0) 
#ifdef SENSMINIS2
		|| (sdh == SDH0_NS)
#endif
	)
	{	memset(&SD0, 0, sizeof(SDH_INFO_T));
		//f_mount(NULL, _Path, 1);
		//memset(&_FatfsVolSd0, 0, sizeof(FATFS));
		sensSys->sdInst.sdPresence = 0;
	}
	/*else
	{	printf("SD1 doesn't supported!!\n");
		//memset(&SD1, 0, sizeof(SDH_INFO_T));
		//f_mount(NULL, _Path, 1);
		//memset(&_FatfsVolSd1, 0, sizeof(FATFS));
	}*/
}

/*** (C) COPYRIGHT 2020 Nuvoton Technology Corp. ***/
