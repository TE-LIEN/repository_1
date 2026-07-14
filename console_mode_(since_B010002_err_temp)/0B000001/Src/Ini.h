#ifndef __INI_H__
#define __INI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sensminiCfg.h"
//#include <stdio.h>

//#include <mqx.h>
//#include <fio.h>

#ifndef INI_ALLOW_MULTILINE
#define INI_ALLOW_MULTILINE 1
#endif

extern char *readOneLineByBuffer(char *buffer, int maxLen, char *startPos);
extern int ini_parse_file(SENS_FILE_PTR file, int (*handler)(void*, const char*, const char*, const char*), void* user);
extern int ini_parse(const char* filename, int (*handler)(void*, const char*, const char*, const char*), void* user);

#ifdef __cplusplus
}
#endif

#endif /* __INI_H__ */
