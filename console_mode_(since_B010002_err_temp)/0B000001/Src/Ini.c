#include "Ini.h"

//#include <stdio.h>
//#include <ctype.h>
//#include <string.h>

//#include "General.h"


//#define MAX_LINE 200
#define MAX_LINE 300
#define MAX_SECTION 50
//#define MAX_NAME  50
#define MAX_NAME 300

#if !SPI_FILE_SYSTEM
//------------------------------------------------------------------------------
static char* rstrip(char* s)
{	char* p = s + strlen(s);
	while (p > s && isspace(*--p))
		*p = '\0';
	return s;
}

//------------------------------------------------------------------------------
// Return pointer to first non-whitespace char in given string.
static char* lskip(const char* s)
{	while (*s && isspace(*s))
		s++;
	return (char*)s;
}

//------------------------------------------------------------------------------
// Return pointer to first char c or ';' comment in given string, or pointer to
// null at end of string if neither found. ';' must be prefixed by a whitespace
// character to register as a comment.
static char* find_char_or_comment(const char* s, char c)
{	int was_whitespace = 0;
	while (*s && *s != c && !(was_whitespace && *s == ';')) 
	{	was_whitespace = isspace(*s);
		s++;
	}
	return (char*)s;
}

//------------------------------------------------------------------------------
// Version of strncpy that ensures dest (size bytes) is null-terminated.
static char* strncpy0(char* dest, const char* src, size_t size)
{	strncpy(dest, src, size);
	dest[size - 1] = '\0';
	return dest;
}

char *readOneLineByBuffer(char *buffer, int maxLen, char *startPos)
{	int idx;
	char *endPos = startPos;
  
	for(idx = 0; idx < maxLen; idx++)
	{	if((startPos[idx] == 0x0D) && (startPos[idx+1] == 0x0A))
		{	endPos += 2;
			buffer[idx] = '\0';
			break;
		}
		else if(startPos[idx] == 0x0A)
		{	endPos++;
			buffer[idx] = '\0';
			break;
		}
		else if(startPos[idx] == 0x00)
		{	buffer[idx] = '\0';
			return NULL;
		}
		buffer[idx] = startPos[idx];
		endPos++;
	}
	return endPos;
}

//------------------------------------------------------------------------------
int ini_parse_file(SENS_FILE_PTR file, int (*handler)(void*, const char*, const char*, const char*), void* user)
{	// Uses a fair bit of stack (use heap instead if you need to)
	char line[MAX_LINE];
	char section[MAX_SECTION] = "";
	char prev_name[MAX_NAME] = "";

	char* start;
	char* end;
	char* name;
	char* value;
	int lineno = 0;
	int error = 0;

	// Scan through file line by line
	while(SENS_FILE_GET(line, sizeof(line), file) != NULL)
	{	lineno++;
		start = lskip(rstrip(line));

		if(*start == ';' || *start == '#') 
		{	// Allow '#' comments at start of line
		}
#if INI_ALLOW_MULTILINE
		else if (*prev_name && *start && start > line) 
		{	// Non-black line with leading whitespace, treat as continuation
			// of previous name's value
			if (!handler(user, section, prev_name, start) && !error)
				error = lineno;
		}
#endif
		else if(*start == '[') 
		{	// A "[section]" line
			end = find_char_or_comment(start + 1, ']');
			if(*end == ']') 
			{	*end = '\0';
				strncpy0(section, start + 1, sizeof(section));
				*prev_name = '\0';
			}
			else if(!error) 
			{	// No ']' found on section line
				error = lineno;
			}
		}
		else if (*start && *start != ';') 
		{	// Not a comment, must be a name[=:]value pair
			end = find_char_or_comment(start, '=');
			if (*end != '=') 
			{	end = find_char_or_comment(start, ':');
			}
			
			if (*end == '=' || *end == ':') 
			{	*end = '\0';
				name = rstrip(start);
				value = lskip(end + 1);
				end = find_char_or_comment(value, '\0');
				if(*end == ';')
					*end = '\0';
				rstrip(value);
				
				// Valid name[=:]value pair found, call handler
				strncpy0(prev_name, name, sizeof(prev_name));
				if(!handler(user, section, name, value) && !error)
					error = lineno;
			}
			else if (!error) 
			{	// No '=' or ':' found on name[=:]value line
				error = lineno;
			}
		}
	}
	return error;
}

//------------------------------------------------------------------------------
int ini_parse(const char* filename, int (*handler)(void*, const char*, const char*, const char*), void* user)
{	SENS_FILE_PTR file;
	int error;
	
	file = SENS_FILE_OPEN(FS_TF, (char *)filename, "r", &error);
    
	if(error) 
	{	SENS_FILE_CLOSE(file);
		return -1;
	}
  
	error = ini_parse_file(file, handler, user);
	SENS_FILE_CLOSE(file);
	return error;
}
#endif