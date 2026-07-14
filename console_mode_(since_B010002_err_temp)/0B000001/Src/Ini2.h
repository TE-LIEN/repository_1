#ifndef __INI2_H__
#define __INI2_H__

#include "sensminiCfg.h"


#define EOL '\n'

#define MAX_INI_LINE_LEN 400               // max line length
#define KEY_VAL_LEN 400             // max param length
#define INI_KEY_MAX_LEN					400

extern void mini_file_free(MiniFile *mini_file);
extern MiniFile *mini_file_update_value_for_key(MiniFile *mini_file, const char *section, const char *key, const char *newvalue);
extern MiniFile *mini_file_update_key_and_value(MiniFile *mini_file, const char *section, const char *oldkey, const char *oldvalue, const char *newkey, const char *newvalue);
extern char *mini_file_get_value(MiniFile *mini_file, const char *section, const char *key);
extern int mIniParserFile(const char *file_name, MiniFile *mIniFile);
extern MiniFile *mini_parse_file(const char *file_name);
extern int mIniSave(char *fname, MiniFile *mIniFile);
extern int mini_save_to_file(MiniFile *mini_file, MiniFile *mini_file_old, char *fname);
extern MiniFile *mini_file_delete_value_for_key(MiniFile *mini_file, const char *section, const char *key, int number);
#endif // __INI2_H__
