#include "Ini2.h"
#include "sensCommon.h"
#include "sdCardTask.h"
#include "sensSystem.h"

#if SPI_FILE_SYSTEM
#include "littleFs/littleFsPort.h"
#endif
//extern char *readOneLineByBuffer(char *buffer, int maxLen, char *startPos);
//extern sens_sys_struct *sensSys;
#define DISPLAY_MEM_ALLOC_FREE	0

#ifdef FS_FFS
#include "ff.h"
#endif

//------------------------------------------------------------------------------
#if 1
static char *_strdup_(const char *inStr)
{	char *outStr;
	if(inStr == NULL) 
		outStr = NULL;
	else
	{	outStr = (char *)SENS_MEM_ZALLOC((strlen(inStr) + 1));
#if DISPLAY_MEM_ALLOC_FREE
		dbg_msg("[MINI](%d) zAlloc, new ptr:%p\r\n", __LINE__, outStr);
#endif
		if (outStr != NULL) strcpy(outStr, inStr);
	}
	return outStr;
}
#endif

//------------------------------------------------------------------------------
static SectionData *mini_file_section_data_new(const char *key, const char *value)
{	SectionData *data;
	if (key == NULL || value == NULL) return NULL;

	data = (SectionData *)SENS_MEM_ZALLOC(sizeof(SectionData));
	if (data == NULL) return NULL;
#if DISPLAY_MEM_ALLOC_FREE
	dbg_msg("[MINI](%d) Alloc, new data ptr:%p\r\n", __LINE__, data);
#endif

	data->key = _strdup_ (key);
	data->value = _strdup_ (value);
	data->next = NULL;

	return data;
}

//------------------------------------------------------------------------------
static void mini_file_section_data_free(SectionData *data)
{	SectionData *p;
	if (data == NULL) return;

	while (data != NULL)
	{	p = data;
		data = p->next;
		p->next = NULL;
#if DISPLAY_MEM_ALLOC_FREE
		dbg_msg("[MINI](%d) free, key:%p, val:%p, ptr:%p, next:%p\r\n", __LINE__, p->key, p->value, p, data);
#endif
		SENS_MEM_FREE (p->key);
		SENS_MEM_FREE (p->value);
		SENS_MEM_FREE (p);
	}
}

//------------------------------------------------------------------------------
static Section *mini_file_section_new(const char *section_name)
{	Section *section;
	if (section_name == NULL) return NULL;

	section = (Section *)SENS_MEM_ZALLOC(sizeof(Section));
	if (section == NULL) return NULL;
		
#if DISPLAY_MEM_ALLOC_FREE
	dbg_msg("[MINI](%d) Alloc, section:%p\r\n", __LINE__, section);
#endif
	section->name = _strdup_ (section_name);
	section->data = NULL;
	section->next = NULL;

	return section;
}

//------------------------------------------------------------------------------
static void mini_file_section_free(Section *section)
{	Section *p;
	if (section == NULL) return;

	while (section != NULL)
	{	p = section;
		section = p->next;
		p->next = NULL;

		mini_file_section_data_free (p->data);
		p->data = NULL;
#if DISPLAY_MEM_ALLOC_FREE
		dbg_msg("[MINI](%d) free, name:%p, ptr:%p\r\n", __LINE__, p->name, p);
#endif
		SENS_MEM_FREE (p->name);
		SENS_MEM_FREE (p);
	}
}
//------------------------------------------------------------------------------
static Section *mini_file_find_section(const MiniFile *mini_file, const char *section)
{	Section *sec = NULL;
	if (mini_file == NULL || section == NULL) return NULL;

	for (sec = mini_file->section; sec != NULL; sec = sec->next)
		if(strcmp(sec->name, section) == 0)
			break;

	return sec;
}

//------------------------------------------------------------------------------
static SectionData *mini_file_find_key(const Section *section, const char *key)
{	SectionData *data = NULL;
	if (section == NULL || key == NULL) return NULL;

	for(data = section->data; data != NULL; data = data->next)
		if(strcmp (data->key, key) == 0)
			break;

	return data;
}
//------------------------------------------------------------------------------
MiniFile *mini_file_new(const char *file_name)
{	MiniFile *mini_file;

	mini_file = (MiniFile *)SENS_MEM_ZALLOC(sizeof(MiniFile));
	if (mini_file == NULL) return NULL;
#if DISPLAY_MEM_ALLOC_FREE
		dbg_msg("[MINI](%d) alloc, mini:%p\r\n", __LINE__, mini_file);
#endif

	mini_file->file_name = _strdup_(file_name);
	mini_file->section = NULL;

	return mini_file;
}

//------------------------------------------------------------------------------
void mini_file_free(MiniFile *mini_file)
{	if (mini_file == NULL) return;

	mini_file_section_free (mini_file->section);
	mini_file->section = NULL;

#if DISPLAY_MEM_ALLOC_FREE
	dbg_msg("[MINI](%d) free, filename:%p\r\n", __LINE__, mini_file->file_name);
#endif
	SENS_MEM_FREE(mini_file->file_name);
	mini_file->file_name = NULL;
#if DISPLAY_MEM_ALLOC_FREE
	dbg_msg("[MINI](%d) free, mini:%p\r\n", __LINE__, mini_file);
#endif
	SENS_MEM_FREE(mini_file);
}

//------------------------------------------------------------------------------
static MiniFile *mini_file_insert_section(MiniFile *mini_file, const char *section_name)
{	Section *section;
	if (mini_file == NULL) return NULL;

	section = mini_file_section_new(section_name);
	if (section == NULL) return NULL;

	// Insert at first position
	section->next = mini_file->section;
	mini_file->section = section;

	return mini_file;
}
//------------------------------------------------------------------------------
static MiniFile *mini_file_insert_key_and_value(MiniFile *mini_file, const char *key, const char *value)
{	SectionData *data;
	if (mini_file == NULL) return NULL;
	if (mini_file->section == NULL) return NULL;

	data = mini_file_section_data_new(key, value);
	if (data == NULL) return NULL;
#if 0 // add it to the head position
	data->next = mini_file->section->data;
	mini_file->section->data = data;
#else // add it to the tail position
	SectionData *p = mini_file->section->data;
	if(p == NULL) 
		mini_file->section->data = data;
	else
	{	while (p->next) p = p->next;
		p->next = data;
	}
#endif
	return mini_file;
}

//------------------------------------------------------------------------------
static MiniFile *mini_file_insert_key_and_value_in_section(MiniFile *mini_file, const char *section, const char *key, const char *value)
{	Section *sec;
	SectionData *data;

	sec = mini_file_find_section(mini_file, section);
	if (sec == NULL) return NULL;
  
	data = mini_file_section_data_new(key, value);
	if (data == NULL) return NULL;
#if 0 // add it to the head position
	data->next = sec->data;
	sec->data = data;
#else // add it to the tail position
	SectionData *p = sec->data;
	if (p == NULL) 
		sec->data = data;
	else
	{	while (p->next) p = p->next;
		p->next = data;
	}
#endif
	return mini_file;
}

//------------------------------------------------------------------------------
// insert key & value for the key found
MiniFile *mini_file_insert_value_for_key(MiniFile *mini_file, const char *section, const char *key, int number)
{	Section *sec;
	SectionData *data;
	SectionData *prev;

	int a = 0; 
  
	char key1[15];
	char key2[15];
	char key3[15];
	char value[15];
  
	SENS_SPRINTF(value, "%s", "0 0 0 0");
  
	if (key == NULL) return NULL;
  
	SENS_SPRINTF(key3, "%s_", key);

	sec = mini_file_find_section(mini_file, section);
	if (sec == NULL) return NULL;
  
	for(a = 0; a < number; a++)
	{	prev = sec->data;
		if(a == 0)
		{	SENS_SPRINTF(key1, "%s", key);
			SENS_SPRINTF(key2, "%s%d", key3, a+1);
		}
		else
		{	SENS_SPRINTF(key1, "%s%d", key3, a);
			SENS_SPRINTF(key2, "%s%d", key3, a+1);
		}
    
		data = mini_file_section_data_new(key2, value);
        
		while(prev != NULL)
		{	if(strcmp(prev->key, key1) != 0)
			{	prev = prev->next;
			}
			else
			{	data->next = prev->next;
				prev->next = data;
				prev = prev->next;
				break;
			}
		}
	}

	return mini_file;
}

//------------------------------------------------------------------------------
// delete key & value for the key found
MiniFile *mini_file_delete_value_for_key(MiniFile *mini_file, const char *section, const char *key, int number)
{	Section *sec;
	//SectionData *data;
	SectionData *prev;
	SectionData *curr;

	//int a; 
  (void)number;
	if (key == NULL) return NULL;

	sec = mini_file_find_section(mini_file, section);
	if (sec == NULL) return NULL;
 
	//for(a = 1; a < number + 1; a++)
	{	
#if 1
		curr = sec->data;
		prev = sec->data;
		while(curr != NULL)
		{	if(strcmp(curr->key, key))
			{	prev = curr;
				curr = curr->next;
			}
			else
			{	if(curr == sec->data)	//delete first
					sec->data = curr->next;
				else
					prev->next = curr->next;
				SENS_MEM_FREE(curr->key);
				SENS_MEM_FREE(curr->value);
				SENS_MEM_FREE(curr);
				break;
			}
		}
#else
		prev = sec->data;
		while(prev != NULL)
		{	if(strcmp(prev->key, key) != 0)
			{	prev = prev->next;
			}
			else
			{	data = prev->next;
				prev->next = data->next;
				prev = data;
			}
		}
#endif
	}
	return mini_file;
}
//------------------------------------------------------------------------------
// Update value for the key that is first found
MiniFile *mini_file_update_value_for_key(MiniFile *mini_file, const char *section, const char *key, const char *newvalue)
{	Section *sec;
	SectionData *data;

	if (key == NULL || newvalue == NULL) return NULL;

	sec = mini_file_find_section(mini_file, section);
	if(sec == NULL) return NULL;

	for(data = sec->data; data != NULL; data = data->next)
	{	if (!strcmp(data->key, key))
		{	
#if DISPLAY_MEM_ALLOC_FREE
			dbg_msg("[MINI](%d) free, value:%p\r\n", __LINE__, data->value);
#endif
			SENS_MEM_FREE(data->value);
			data->value = _strdup_(newvalue);
			break;
		}
		if(data->next == NULL)
			mini_file_insert_key_and_value_in_section(mini_file, section, key, newvalue);
	}
	return mini_file;
}

//------------------------------------------------------------------------------
// Update value for the first value 1 to 0
MiniFile *mini_file_update_value_first(MiniFile *mini_file, const char *section, const char *key)
{	Section *sec;
	SectionData *data;

	if (key == NULL) return NULL;

	sec = mini_file_find_section(mini_file, section);
	if (sec == NULL) return NULL;

	for (data = sec->data; data != NULL; data = data->next)
	{	if (!strcmp(data->key, key))
		{	data->value[0] = '0';
			break;
		}
	}
	return mini_file;
}

//------------------------------------------------------------------------------
MiniFile *mini_file_update_key_and_value(MiniFile *mini_file, const char *section, const char *oldkey, const char *oldvalue, const char *newkey, const char *newvalue)
{	Section *sec;
	SectionData *data;

	if(oldkey == NULL || oldvalue == NULL || newkey == NULL || newvalue == NULL) 
		return NULL;

	sec = mini_file_find_section(mini_file, section);
	if (sec == NULL) return NULL;

	for (data = sec->data; data != NULL; data = data->next)
	{	if ((!strcmp(data->key, oldkey)) && (!strcmp(data->value, oldvalue)))
		{	
#if DISPLAY_MEM_ALLOC_FREE
			dbg_msg("[MINI](%d) free, key:%p, value:%p\r\n", __LINE__, data->key, data->value);
#endif
			SENS_MEM_FREE(data->key);
			SENS_MEM_FREE(data->value);
			data->key = _strdup_(newkey);
			data->value = _strdup_(newvalue);
			break;
		}
	}
	return mini_file;
}

//------------------------------------------------------------------------------
unsigned int mini_file_get_number_of_sections(MiniFile *mini_file)
{	unsigned int num_sections = 0;
	Section *sec;
	if (mini_file == NULL) return 0;

	for (sec = mini_file->section; sec != NULL; sec = sec->next)
		num_sections++;

	return num_sections;
}

//------------------------------------------------------------------------------
unsigned int mini_file_get_number_of_keys(MiniFile *mini_file, const char *section)
{	unsigned int num_keys = 0;
	Section *sec;
	SectionData *data;
	if (mini_file == NULL) return 0;

	sec = mini_file_find_section(mini_file, section);
	if (sec == NULL) return 0;

	for (data = sec->data; data != NULL; data = data->next)
		num_keys++;

	return num_keys;
}

//------------------------------------------------------------------------------
void mini_file_print_keys_and_values(MiniFile *mini_file, const char *section)
{	Section *sec;
	SectionData *data;
	int i = 0;
	if (mini_file == NULL || section == NULL) return;

	sec = mini_file_find_section(mini_file, section);
	if (sec == NULL) return;

	dbg_msg("[%s] Number of keys = %ld\r\n", section, mini_file_get_number_of_keys(mini_file, section));

	for (data = sec->data; data != NULL; data = data->next)
	{	dbg_msg("%02d: (%s) = (%s)\r\n", ++i, data->key, data->value);
	}
}

//------------------------------------------------------------------------------
char *mini_file_get_value(MiniFile *mini_file, const char *section, const char *key)
{	Section *sec;
	SectionData *data;
	if (mini_file == NULL) return NULL;

	sec = mini_file_find_section(mini_file, section);
	if (sec == NULL) return NULL;

	data = mini_file_find_key(sec, key);
	if (data == NULL) return NULL;

	return data->value;
}

//------------------------------------------------------------------------------
static char *mini_lstrip(char *string)
{	char *p;
	if (string == NULL) return NULL;

	// Search the first non whitespace character from left to right
	for (p = string; (p != NULL) && isspace (*p); p++);
	return p;
}

//------------------------------------------------------------------------------
static char *mini_rstrip(char *string)
{	char *p;
	int pos;
	size_t len;
	if (string == NULL) return NULL;

	p = string;
	len = strlen (string);

	// Search the first non whitespace character from right to left
	for (pos = len - 1; (pos >= 0) && isspace (p[pos]); pos--);

	if ((pos >= 0) && !isspace (p[pos]))
		p[pos + 1] = '\0';

	return string;
}

//------------------------------------------------------------------------------
static char *mini_strip(char *string)
{	char *ret;
	if (string == NULL) return NULL;

	ret = mini_lstrip (string);
	ret = mini_rstrip (ret);

	return ret;
}

//------------------------------------------------------------------------------
static int mini_readline(SENS_FILE_PTR file, char *line)
{	// return 1 if success, else return 0
	return (SENS_FILE_GET(line, MAX_INI_LINE_LEN, file)!=NULL)?1:0;
}

//------------------------------------------------------------------------------
static int mini_parse_line(MiniFile *mini_file, char *line)
{	char *start, *end, *equal;	
	char *section, *key, *value;
	//char tmp[KEY_VAL_LEN], *p; // for stripping spaces from key and value
	char *tmp, *p; // for stripping spaces from key and value
	size_t section_len, key_len, value_len;
	MiniFile *mini_file_tmp;

	if(line == NULL) return -1;

	// Strip all whitespaces
	start = mini_strip (line);

	// Empty line
	if(strcmp (start, "") == 0) 
		return 0;

	tmp = SENS_MEM_ZALLOC(KEY_VAL_LEN);
#if DISPLAY_MEM_ALLOC_FREE
	dbg_msg("[MINI](%d) alloc, tmp:%p\r\n", __LINE__, tmp);
#endif
	// Non empty line
	switch(start[0]) 
	{	case '[':	// Section
					// At the end of the line must be an end of section (']')
					end = strchr (start, ']');
					if ((end == NULL) || (end[1] != '\0'))
					{	
#if DISPLAY_MEM_ALLOC_FREE
						dbg_msg("[MINI](%d) free, tmp:%p\r\n", __LINE__, tmp);
#endif
						SENS_MEM_FREE(tmp);
						return -1;
					}
					// Get length of the section
					section_len = strlen (start) - 2;
					// Empty section
					if(section_len == 0)
					{	
#if DISPLAY_MEM_ALLOC_FREE
						dbg_msg("[MINI](%d) free, tmp:%p\r\n", __LINE__, tmp);
#endif
						SENS_MEM_FREE(tmp);
						return -1;
					}
					// Get section string
					section = (char *) SENS_MEM_ZALLOC((section_len + 1) * sizeof (char));
#if DISPLAY_MEM_ALLOC_FREE
					dbg_msg("[MINI](%d) alloc, section:%p\r\n", __LINE__, section);
#endif
					if(section == NULL)
					{	
#if DISPLAY_MEM_ALLOC_FREE
						dbg_msg("[MINI](%d) free, tmp:%p\r\n", __LINE__, tmp);
#endif
						SENS_MEM_FREE(tmp);
						return -1;
					}
					strncpy (section, &start[1], section_len);
					section[section_len] = '\0';
					mini_file_tmp = mini_file_insert_section (mini_file, section);
					SENS_MEM_FREE(section);
					if(mini_file_tmp == NULL)
					{	
#if DISPLAY_MEM_ALLOC_FREE
						dbg_msg("[MINI](%d) free, tmp:%p\r\n", __LINE__, tmp);
#endif
						SENS_MEM_FREE(tmp);
						return -1;
					}
					break;
		case ';':	// Comment
					// Process comments here
					break;
		default:
					// Between key and value must be an equality symbol ('=')
					equal = strchr (start, '=');
					if((equal == NULL) || (start == equal) || (equal[1] == '\0'))
					{	
#if DISPLAY_MEM_ALLOC_FREE
						dbg_msg("[MINI](%d) free, tmp:%p\r\n", __LINE__, tmp);
#endif
						SENS_MEM_FREE(tmp);
						return -1;
					}
					// Get length of the key string
					for(key_len = 0; start[key_len] != '='; key_len++);
					// Get key string
					key = (char *) SENS_MEM_ZALLOC ((key_len + 1) * sizeof (char));
#if DISPLAY_MEM_ALLOC_FREE
					dbg_msg("[MINI](%d) alloc, key:%p\r\n", __LINE__, key);
#endif
					if(key == NULL)
					{	
#if DISPLAY_MEM_ALLOC_FREE
						dbg_msg("[MINI](%d) free, tmp:%p\r\n", __LINE__, tmp);
#endif
						SENS_MEM_FREE(tmp);
						return -1;
					}
#if 1
					memset(tmp, 0, KEY_VAL_LEN);
					strncpy(tmp, start, key_len);
					p = mini_strip(tmp);
					strcpy(key, p);
#else
					strncpy (key, start, key_len);
					key[key_len] = '\0';
#endif
					// Get length of the value string
					value_len = strlen (equal) - 1;

					// Get value string
					value = (char *) SENS_MEM_ZALLOC ((value_len + 1) * sizeof (char));
#if DISPLAY_MEM_ALLOC_FREE
					dbg_msg("[MINI](%d) alloc, value:%p\r\n", __LINE__, value);
#endif
					if (value == NULL)
					{	
#if DISPLAY_MEM_ALLOC_FREE
						dbg_msg("[MINI](%d) free, tmp:%p\r\n", __LINE__, tmp);
#endif
						SENS_MEM_FREE(tmp);
						return -1;
					}
#if 1
					memset(tmp, 0, KEY_VAL_LEN);
					strncpy (tmp, &equal[1], value_len);
					p = mini_strip(tmp);
					strcpy(value, p);
#else
					strncpy (value, &equal[1], value_len);
					value[value_len] = '\0';
#endif
					mini_file_tmp = mini_file_insert_key_and_value (mini_file, key, value);
					SENS_MEM_FREE(key);			//insert key will alloc new mem, so free it
					SENS_MEM_FREE(value);		//insert key will alloc new mem, so free it
					if(mini_file_tmp == NULL)
					{	
#if DISPLAY_MEM_ALLOC_FREE
						dbg_msg("[MINI](%d) free, tmp:%p\r\n", __LINE__, tmp);
#endif
						SENS_MEM_FREE(tmp);
						return -1;
					}
	}
#if DISPLAY_MEM_ALLOC_FREE
	dbg_msg("[MINI](%d) free, tmp:%p\r\n", __LINE__, tmp);
#endif
	SENS_MEM_FREE(tmp);
	return 0;
}
//------------------------------------------------------------------------------
int mIniParserFile(const char *file_name, MiniFile *mIniFile)
{	int ret;
	SENS_FILE_PTR file;
	char *line;
	int error;
	
	file = SENS_FILE_OPEN(FS_TF, (char *)file_name, "r", &error);
	if(error) 
	{	mini_file_free(mIniFile);
		SENS_FILE_CLOSE(file);
		//file = NULL;
		return -1;
	}
	line = SENS_MEM_ZALLOC(MAX_INI_LINE_LEN);
	ret = mini_readline(file, line);

#if defined OS_MQX || defined FS_MFS
	while(!feof(file) && (ret==1))
#elif defined FS_FFS
	while(!f_eof(file) && (ret==1))
#endif
	{	if(mini_parse_line(mIniFile, line) < 0) break;
		ret = mini_readline(file, line);
	}
	SENS_FILE_CLOSE(file);
	//file = NULL;
	SENS_MEM_FREE(line);
	return 0;
}
//------------------------------------------------------------------------------
static int iniParserFileFromSpi(char *filename, MiniFile *mini_file)
{	char *fileBuf;
	char *startPos, *endPos;
	char *crPos;
	char *line;
	int fileSize, readLength;
	int oneLineLength;
	
#if SPI_FILE_SYSTEM
	fileSize = SENS_SPI_FS_GET_FILE_SIZE(filename);
	if(fileSize)
	{	fileBuf = SENS_MEM_ZALLOC(fileSize);
		readLength = SENS_SPI_FS_READ_FILE(filename, (char *)fileBuf, fileSize, 0, NORMAL_READ_MODE);
		if(readLength != fileSize)
		{	SENS_MEM_FREE(fileBuf);
			return -1;
		}
	}
	else
	{	return -1;
	}
#endif
	
	startPos = fileBuf;
	endPos = fileBuf + fileSize;
	
	//line = SENS_MEM_ALLOC(MAX_INI_LINE_LEN);
	while(startPos < endPos)
	{	crPos = strstr((char *)startPos, "\r\n");
		if(crPos == NULL)
			break;
		oneLineLength = (crPos - startPos);
		*crPos = '\0';
		if(oneLineLength)
		{	//memset(line, 0, MAX_INI_LINE_LEN);
			line = SENS_MEM_ZALLOC(oneLineLength + 1);
			strcpy(line, startPos);
			//memcpy(line, startPos, oneLineLength);
			mini_parse_line(mini_file, line);
			SENS_MEM_FREE(line);
		}
		startPos = crPos + 2;
	}
	SENS_MEM_FREE(line);
	SENS_MEM_FREE(fileBuf);
	return 0;
}
//------------------------------------------------------------------------------
MiniFile *mini_parse_file(const char *file_name)
{	MiniFile *mini_file;
	char loadFilename[32];
	uint8_t changeToBakFile=0;
	Section *sensminiA4Section;
	SectionData *sectionKey;
	uint32_t bakFileLen;
	char *bakFileBuf;
  
  if(file_name == NULL) return NULL;
  	
	//if(!strcmp(INIFILE, file_name))
#if SPI_FILE_SYSTEM
	if(!sysCtrl->paramInSd)
	{	if(SENS_SPI_FS_FILE_EXIST(INIFILE) != SPI_FILE_IS_EXIST)
		{	return NULL;
		}
	}
	else
#endif
	{	if(chkFileExist(INI_BAK_FILE))
			changeToBakFile = 1;
	}	
	
RE_LOAD:
	memset(loadFilename, 0, 32);
	if(changeToBakFile)
		memcpy(loadFilename, INI_BAK_FILE, strlen(INI_BAK_FILE));
	else
		memcpy(loadFilename, file_name, strlen(file_name));
	
  mini_file = mini_file_new(loadFilename);
  if(mini_file == NULL) 
  {	mini_file_free(mini_file);
    return NULL;
  }

#if SPI_FILE_SYSTEM
	if(!sysCtrl->paramInSd)
	{	if(iniParserFileFromSpi((char *)loadFilename, mini_file))
		{	mini_file_free(mini_file);
			mini_file = NULL;
		}
		return mini_file;
	}
#endif
	if(iniParserFile((char *)loadFilename, mini_file))
	{	mini_file_free(mini_file);
		if(changeToBakFile)
		{	sdDeleteFileNew(INI_BAK_FILE);
			changeToBakFile = 0;
			goto RE_LOAD;
		}
		return NULL;
	}

	
	sensminiA4Section = mini_file_find_section(mini_file, SECTION);
	if(sensminiA4Section == NULL)
	{	sdDeleteFileNew(INI_BAK_FILE);
		mini_file_free(mini_file);
		if(changeToBakFile)
		{	changeToBakFile = 0;
			goto RE_LOAD;
		}
	}
	sectionKey = mini_file_find_key(sensminiA4Section, COMPLETE_KEY);
	if(changeToBakFile)	
	{	int readLen;
		if(sectionKey == NULL)
		{	sdDeleteFileNew(INI_BAK_FILE);
			mini_file_free(mini_file);
			changeToBakFile = 0;
			goto RE_LOAD;
		}
		sdDeleteFileNew(INIFILE);
		bakFileLen = getFileLength(INI_BAK_FILE);
		bakFileBuf = SENS_MEM_ALLOC(bakFileLen);
		readLen = sdReadFile(INI_BAK_FILE, bakFileBuf, bakFileLen, 0, NORMAL_READ_MODE);
		if(readLen == bakFileLen)
		{	sdWriteFile(INIFILE, bakFileBuf, bakFileLen, 0, OVER_WRITE_MODE);
			sdDeleteFileNew(INI_BAK_FILE);
		}
		else
		{	sysCtrl->isReadyToReboot = 1;
			dbg_msg("%s", "Read File error, reboot!!\r\n");
		}
		SENS_MEM_FREE(bakFileBuf);
		//renameFile(INI_BAK_FILE, INIFILE);
		dbg_msg("%s", "find new param file, replace now!!\r\n");
	}
	if(sectionKey)
		mini_file_delete_value_for_key(mini_file, SECTION, COMPLETE_KEY, 0);
  return mini_file;
}

int mIniSave(char *fname, MiniFile *mIniFile)
{	SENS_FILE_PTR file;
	Section *sec;
	SectionData *data;
	char *buf;
	int len, keycount;
	int realWriteLen;
	int error;
	Section *sensminiA4Section;
	SectionData *sectionKey;
	
	sensminiA4Section = mini_file_find_section(mIniFile, SECTION);
	if(sensminiA4Section == NULL)
		return FALSE;
	sectionKey = mini_file_find_key(sensminiA4Section, COMPLETE_KEY);
	if(sectionKey == NULL)
	{	mini_file_insert_key_and_value_in_section(mIniFile, SECTION, COMPLETE_KEY, "1");
	}
	
	file = SENS_FILE_OPEN(FS_TF, fname, "w", &error);
	if(error)
	{	//SENS_FILE_CLOSE(file);
		//file = NULL;
		return FALSE;
	}
	keycount = 0;
	buf = SENS_MEM_ZALLOC(INI_KEY_MAX_LEN);
    
	for(sec = mIniFile->section; sec != NULL; sec = sec->next)
	{	SENS_SPRINTF(buf, "[%s]\r\n", sec->name);
		len = strlen(buf);
		SENS_FILE_WRITE(file, (uint8_t *)buf, len, &realWriteLen);
		if(realWriteLen != len)
		{	SENS_FILE_CLOSE(file);
			file = NULL;
			SENS_MEM_FREE(buf);
			dbg_msg("%s written ini file into SD fail.\r\n", buf);
			return FALSE;
		}
		for(data = sec->data; data != NULL; data = data->next)
		{	keycount++;
			if(sec->next == NULL && data->next == NULL)
				SENS_SPRINTF(buf, "%s = %s\r\n\n", data->key, data->value);
			else
				SENS_SPRINTF(buf, "%s = %s\r\n", data->key, data->value);
			len = strlen(buf);
			SENS_FILE_WRITE(file, (uint8_t *)buf, len, &realWriteLen);
			if(realWriteLen != len)
			{	SENS_FILE_CLOSE(file);
				file = NULL;
				SENS_MEM_FREE(buf);
				dbg_msg("%s written into SD fail.\r\n", buf);
				return FALSE;
			}
		}
	} 
    
	SENS_FILE_CLOSE(file);
	SENS_MEM_FREE(buf);
	
	return TRUE;
}

//------------------------------------------------------------------------------
int mini_save_to_file(MiniFile *mini_file, MiniFile *mini_file_old, char *fname)
{	Section *sec;
	SectionData *data;
  int keycount;

	if(	mini_file == NULL || fname == NULL) 
	{	return 0;
	}

	// Count sections of the mini_file
	keycount = 0;
	for (sec = mini_file->section; sec != NULL; sec = sec->next)
	{	// Count keys of the section
		for (data = sec->data; data != NULL; data = data->next)
		{	if(keycount >= 0)
				keycount++;
			if(strlen(data->value) == 0)
				keycount = -1;
			if(strcmp(data->key, "IP_MASK") == 0)
			{	if(strcmp(data->value, "0.0.0.0") == 0)
				{	dbg_msg("%s", "Read IP mask error\r\n");
					keycount = -1;
				}
			}
		}
	}
//  dbg_msg("parameters(%d)\r\n", keycount);
  
	if(keycount > 0)
	{	if(mIniSaveToTf(fname, mini_file))
			return 0;
  }
  else
  {	dbg_msg("Write parameters fail.(%d)\r\n", keycount);
    //sensSys->IsReboot = 1;
    return 0;
  }
  return 1;
}
