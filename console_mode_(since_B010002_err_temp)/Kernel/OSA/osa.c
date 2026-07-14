#include "sensminiCfg.h"
#include "string.h"
#include <stdarg.h>


void *OSA_MEM_CALLOC(size_t nmemb, size_t xWantedSize)
{	void *bufPtr;

	bufPtr = SENS_MEM_ALLOC(nmemb * xWantedSize);
	if(bufPtr != NULL)
		memset(bufPtr, 0, (uint32_t)(nmemb * xWantedSize));
	return bufPtr;
}

void *MEMORY_ALLOC_ZERO(size_t xWantedSize)
{	void *bufPtr;
	
	bufPtr = SENS_MEM_ALLOC(xWantedSize);
	if(bufPtr != NULL)
		memset(bufPtr, 0, xWantedSize);
	return bufPtr;
}

void OSA_MEM_FREE(void *ptr)
{	if(ptr == NULL)
		return;
	vPortFree(ptr);
}

uint32_t OSA_MSG_Q_INIT(QueueHandle_t *msgQ, uint32_t queueCount, uint32_t queueSize)
{	*msgQ = xQueueCreate(queueCount, queueSize);
	if(*msgQ == NULL)
	{	return -1;
	}
	return 0;
}

uint32_t OSA_MSG_Q_RECV(void *msgQ, uint32_t *msg, uint32_t flag, uint32_t timeout, void *flag2)
{	BaseType_t ret;
	
	if(timeout == 0)
		ret = xQueueReceive(msgQ, msg, portMAX_DELAY);
	else
	{	timeout /= 200;
		if(!timeout)
			timeout = 1;
		timeout *= 1000;
		ret = xQueueReceive(msgQ, msg, timeout/portTICK_PERIOD_MS);
	}
	if(ret == 1)
		return 0;
	else
		return 1;
}

extern const SENS_TASK_TEMPLATE  taskTemplateList[];

uint32_t OSA_TASK_CREATE(uint32_t taskId)
{	uint32_t idx;
	BaseType_t status;
	
	//for(idx=0;idx<(STRUCT_ARRAY_SIZE(MQX_template_list));idx++)
	for(idx=0;idx<TASK_MAX;idx++)
	{	if(taskId == taskTemplateList[idx].TASK_TEMPLATE_INDEX)
		{	status = xTaskCreate( taskTemplateList[idx].TASK_ADDRESS, taskTemplateList[idx].TASK_NAME, taskTemplateList[idx].TASK_STACKSIZE, NULL, taskTemplateList[idx].TASK_PRIORITY, NULL );
			if(status != pdPASS)
			{	return -1;
			}
			break;	
		}
	}
	return idx;
}

uint32_t OSA_EVENT_CREATE(SENS_EVENT_STRUCT *event, uint32_t flag)
{	*event = xEventGroupCreate();
	if(*event == NULL)
		return -1;
	return 0;
}

uint32_t OSA_EVENT_SET(SENS_EVENT_STRUCT_PTR event, uint32_t setBit)
{	EventBits_t uxBits;
	uxBits = xEventGroupSetBits(event, setBit);
	/*if((uxBits & setBit) != setBit)
		return -1;
	else*/
		return uxBits;
}

uint32_t OSA_EVENT_WAIT(SENS_EVENT_STRUCT_PTR event, uint32_t waitBits, uint32_t waitAll, uint32_t waitTimeout)
{	waitTimeout = waitTimeout / 200;
	waitTimeout *= 1000;
	return xEventGroupWaitBits(event, waitBits, pdTRUE, waitAll, waitTimeout/portTICK_PERIOD_MS);
}


int freertos_semaInit(SENS_SEM_STRUCT *sema, int count)
//SENS_SEM_STRUCT freertos_semaInit(void)
{	*sema = xSemaphoreCreateMutex();	//cannot use in interrupt
	//*sema = xSemaphoreCreateBinary();
	if(*sema == NULL)
		return -1;
	return 0;
}

BaseType_t freertos_semaLock(SemaphoreHandle_t sema)
{	BaseType_t err;
	err = xSemaphoreTake(sema, portMAX_DELAY);
	return err;
}
BaseType_t freertos_semaUnlock(SemaphoreHandle_t sema)
{	BaseType_t err;
	err = xSemaphoreGive(sema);
	return err;
}


#define INCLUDE_FLOATING_POINT_IO	0
#define IO_EOF	-1
#define SCAN_ERROR    (-1)
#define SCAN_LLONG    (0)
#define SCAN_WLONG    (1)
#define SCAN_BLONG    (2)
#define SCAN_MLONG    (3)

#ifndef HUGE_VAL
#define HUGE_VAL         99.e999
#endif

char _io_scanline_is_octal_digit(char c)
{	if ( (c >= '0') && (c <= '7') )
	{	return 1;
	}
	else
	{	return 0;
	}
}

char _io_scanline_is_hex_digit(char c)
{	if ( ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')))
	{	return 1;
	}
	else
	{	return 0;
	}
}

int32_t _io_scanline_base_convert(register unsigned char c, uint32_t base)
{	if(  c >= 'a' && c <= 'z'  )
	{	/* upper case c */
		c -= 32;
	} /* Endif */

	if(  c >= 'A' && c <= 'Z'  )
	{	/* reduce hex digit */
		c -= 55;
	}
	else if (  (c >= '0') && (c <= '9')  )
	{	/* reduce decimal digit */
		c -= 0x30;
	}
	else
	{	return SCAN_ERROR;
	} /* Endif */
	if ( (uint32_t)c  > (base-1) )
	{	return SCAN_ERROR;
	}
	else
	{	return c;
	} /* Endif */
}

char _io_scanline_format_ignore_white_space(register char **s_ptr, register uint32_t *count)
{	register char c;

	*count = 0;
	c = **s_ptr;
	while (  (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\v') || (c == '\f') )
	{	c = *(++*s_ptr);
		(*count)++;
	} /* Endwhile */
	return (c);
}

char _io_scanline_ignore_white_space(register char  **s_ptr, register uint32_t *count, register uint32_t width)
{	register char c;

	c = **s_ptr;
	*count = 0;
	while (  (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\v') || (c == '\f') )
	{	c = *(++*s_ptr);
		(*count)++;
		if ( width )
		{	if ( --width == 0 )
			{	return(c);
			}
		}
	}
	return (c);
}

int _io_scanline(char  *line_ptr, char  *format, va_list args_ptr)
{	char				suppress_field;
	register int32_t	c;
	register uint32_t	n;
	char      			*sptr = NULL;
	int32_t			sign;
	uint32_t    		val;
	int32_t			width;
	int32_t			numtype;  /* used to indicate bit size of argument */
	register int32_t	number_of_chars;
	uint32_t			temp;
	uint32_t			base;
	void      			*tmp_ptr;
#if INCLUDE_FLOATING_POINT_IO
	double    			dnum;
#endif

	if( *line_ptr == '\0' )
	{	return IO_EOF;
	}

	n = 0;
	number_of_chars = 0;
	while ((c = *format++) != 0)
	{	width = 0;
		/*
		 * skip white space in format string, and any in input line
		 */
		if (  (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\v') || (c == '\f') )
		{	if ( ! _io_scanline_format_ignore_white_space((char  **)&format, &temp ) )
			{	/*
             	 * End of format string encountered, scanning is finished.
             	 */
            	return (int32_t)n;
			} /* Endif */

			if ( ! _io_scanline_ignore_white_space((char  **)&line_ptr , &temp, 0 ) )
			{	/*
			 	 * End of line string encountered, no more input to scan.
			 	 */
				return (int32_t)n;
			} /* Endif */
			number_of_chars += temp;
			continue;
		} /* Endif */

		if ( c != '%' )
		{	/*
		 	 * if format character is not %, then it must match text
		 	 * in input line
		 	 */
			if ( c != _io_scanline_ignore_white_space((char  **)&line_ptr , &temp, 0) )
			{	/*
			 	 * Text not matched, stop scanning..
			 	 */
				return (int32_t)n;
			}
			else
			{	++line_ptr;
			} /* Endif */
			number_of_chars += temp;
		}
		else
		{	/*
		 	 * process % format conversion
		 	 */
			suppress_field = 0;
			width = 0;
			numtype = SCAN_MLONG;
			sign = 1;
			val = 0;

			/*
			 * Skip whitespace. Check for " %  ", return if found, otherwise
			 * get next character.
			 */
			if ( ! _io_scanline_format_ignore_white_space( (char  **)&format, &temp ) )
			{	return (int32_t)n;
			} /* Endif */
			c = *format;

			/*
			 * Check for assignment suppression. if "*" given,
			 * suppress assignment
			 */
			if ( c == '*' )
			{	++suppress_field;
				c = *(++format);
			} /* Endif */

			/*
			 * Skip whitespace. Check for " %  *  ", return if found, otherwise
			 * get next character.
			 */
			if ( ! _io_scanline_format_ignore_white_space( (char  **)&format,&temp))
			{	return (int32_t)n;
			} /* Endif */
			c = *format;

			/*
			 * Now check for a width field
			 */
			width = 0;
			while (  ('0' <= c) && (c <= '9')  )
			{	width = width * 10 + (int32_t)(c - '0');
            	c = *(++format);
			} /* Endwhile */

         /*
          * Skip whitespace. Check for " %  *  23 ", return if found,
          * otherwise get next character.
          */
         if ( ! _io_scanline_format_ignore_white_space(
            (char  **)&format,   &temp ) ) {
            return (int32_t)n;
         } /* Endif */
         c = *format++;

         /*
          * Check to see if c is lower case, if so convert to upper case
          */
         if (  (c >= 'a') && (c <= 'z')  ) {
            c -= 32;
         } /* Endif */

         /*
          * Now check to see if c is a type specifier.
          */
         if (c  == 'H') {
            numtype = SCAN_WLONG;
            if ( ! _io_scanline_format_ignore_white_space(
               (char  **)&format, &temp ) )
            {
               return (int32_t)n;
            } /* Endif */
            c = *format++;
         } else if (c == 'L') {
            numtype = SCAN_LLONG;
            if ( ! _io_scanline_format_ignore_white_space(
               (char  **)&format, &temp ) )
            {
               return (int32_t)n;
            } /* Endif */
            c = *format++;
         } else if (c == 'Z') {
            numtype = SCAN_BLONG;
            if ( ! _io_scanline_format_ignore_white_space(
               (char  **)&format, &temp ) )
            {
               return (int32_t)n;
            } /* Endif */
            c = *format++;
         } else if (c == 'M') {
            numtype = SCAN_MLONG;
            if ( ! _io_scanline_format_ignore_white_space(
               (char  **)&format, &temp ) )
            {
               return (int32_t)n;
            } /* Endif */
            c = *format++;
         } /* Endif */

         /*
          * Check to see if c is lower case, if so convert to upper case
          */
         if (  (c >= 'a') && (c <= 'z')  ) {
            c -= 32;
         } /* Endif */

         /*
          * Now check to see if c is a valid format specifier.
          */
         switch ( c ) {

            case 'I':
               c = _io_scanline_ignore_white_space( (char  **)&line_ptr,
                  &temp, 0 );
               if ( *line_ptr == '0' ) {
                  ++number_of_chars;
                  ++line_ptr;
                  if ( width ) {
                     if ( --width <= 0 ) {
                        goto print_val;
                     } /* Endif */
                  } /* Endif */
                  if ( (*line_ptr == 'x') || (*line_ptr == 'X') ) {
                     base = 16;
                     ++line_ptr;
                     ++number_of_chars;
                     if ( width ) {
                        if ( --width <= 0 ) {
                           goto print_val;
                        } /* Endif */
                     } /* Endif */
                  } else if ( (*line_ptr == 'b') || (*line_ptr == 'B') ) {
                     base = 2;
                     ++line_ptr;
                     ++number_of_chars;
                     if ( width ) {
                        if ( --width <= 0 ) {
                           goto print_val;
                        } /* Endif */
                     } /* Endif */
                  } else {
                     base = 8;
                     if ( ! _io_scanline_is_octal_digit(*line_ptr) ) {
                        goto print_val;
                     } /* Endif */
                  } /* Endif */
               } else {
                  goto decimal;
               } /* Endif */
               goto doval;

            case 'P':
            case 'X':
               base = 16;
               c = _io_scanline_ignore_white_space( (char  **)&line_ptr,
                  &temp, 0 );
               if ( *line_ptr == '0' ) {
                  ++line_ptr;
                  ++number_of_chars;
                  if ( width ) {
                     if ( --width <= 0 ) {
                        goto print_val;
                     } /* Endif */
                  } /* Endif */
                  if ( (*line_ptr == 'x') || (*line_ptr == 'X') ) {
                     ++line_ptr;
                     ++number_of_chars;
                     if ( width ) {
                        if ( --width <= 0 ) {
                           goto print_val;
                        } /* Endif */
                     } /* Endif */
                  } else if ( ! _io_scanline_is_hex_digit(*line_ptr) ) {
                     goto print_val;
                  } /* Endif */
               } /* Endif */
               goto doval;

            case 'O':
               base = 8;
               c = _io_scanline_ignore_white_space( (char  **)&line_ptr,
                  &temp, 0 );
               if ( *line_ptr == '0' ) {
                  ++number_of_chars;
                  ++line_ptr;
                  if ( width ) {
                     if ( --width <= 0 ) {
                        goto print_val;
                     } /* Endif */
                  } /* Endif */
                  if ( ! _io_scanline_is_octal_digit(*line_ptr) ) {
                     goto print_val;
                  }
               } /* Endif */
               goto doval;

            case 'B':
               base = 2;
               c = _io_scanline_ignore_white_space( (char  **)&line_ptr,
                  &temp, 0 );
               if ( *line_ptr == '0' ) {
                  ++line_ptr;
                  ++number_of_chars;
                  if ( width ) {
                     if ( --width <= 0 ) {
                        goto print_val;
                     } /* Endif */
                  } /* Endif */
                  if ( (*line_ptr == 'b') || (*line_ptr == 'B') ) {
                     ++line_ptr;
                     ++number_of_chars;
                     if ( width ) {
                        if ( --width <= 0 ) {
                           goto print_val;
                        } /* Endif */
                     } /* Endif */
                  } else if ( ! _io_scanline_is_hex_digit(*line_ptr) ) {
                     goto print_val;
                  } /* Endif */
               } /* Endif */
               goto doval;

            case 'D':
               decimal:
               base = 10;
               temp = 0;
               if (  _io_scanline_ignore_white_space( (char  **)&line_ptr,
                  &temp, 0 ) == '-'  )
               {
                  number_of_chars += temp;
                  sign = -1;
                  ++line_ptr;
                  ++number_of_chars;
                  if ( width ) {
                     width -= (int32_t)temp;
                     if ( width <= 0 ) {
                        goto print_val;
                     } /* Endif */
                  } /* Endif */
               } else {
                  number_of_chars += temp;
               } /* Endif */

            case 'U':
               base = 10;
               c = _io_scanline_ignore_white_space( (char  **)&line_ptr,
                  &temp, 0 );
doval:
               val = 0;
               /* remove spaces if any but don't */
               /* parse passed end of line       */
               c = *line_ptr;
               number_of_chars += temp;
               if ( width ) {
                  width -= temp;
                  if ( width <= 0 ) {
                     break;
                  } /* Endif */
               } /* Endif */
               if (  _io_scanline_base_convert( (unsigned char)c, base ) == SCAN_ERROR  ) {
                  return (int32_t)n;
               } /* Endif */

               while ( (( c = _io_scanline_base_convert( *line_ptr, base ))
               != SCAN_ERROR) ) {
                  ++line_ptr;
                  ++number_of_chars;
                  val = val * base + (uint32_t)((unsigned char)c & 0x7F);
                  if ( width ) {
                     if ( --width <= 0 ) {
                        break;
                     } /* Endif */
                  } /* Endif */
               } /* Endwhile */

print_val:
               if (  ! suppress_field  ) {
                  /* assign value using appropriate pointer */
                  val *= sign;
                  tmp_ptr = (void *)va_arg(args_ptr, void *);
                  switch ( numtype ) {
                     case SCAN_LLONG:
                        *((uint32_t *)tmp_ptr) = val;
                        break;
                     case SCAN_WLONG:
                        *((uint16_t *)tmp_ptr) = (uint16_t)val;
                        break;
                     case SCAN_BLONG:
                        *((unsigned char *)tmp_ptr) = (unsigned char)val;
                        break;
                     case SCAN_MLONG:
                        *((uint32_t *)tmp_ptr) = (uint32_t)val;
                        break;
                     default:
                        break;
                  } /* End Switch */
                  ++n;
               } /* Endif */
               break;

            case 'S':
               temp = 0;
               _io_scanline_ignore_white_space( (char  **)&line_ptr,
                  &temp, 0 );
               number_of_chars += temp;
               if ( ! suppress_field ) {
                  sptr = (char *)va_arg(args_ptr, void *);
               } /* Endif */
               if ( width ) {
                  width -= (int32_t)temp;
                  if ( width <= 0 ) {
                     goto string_done;
                  } /* Endif */
               } /* Endif */

               while (( c = *line_ptr ) != 0) {
                  ++line_ptr;
                  ++number_of_chars;
                  if ( c == *format ) {
                     ++format;
                     break;
                  } /* Endif */

                  if ( ! suppress_field ) {
                     *sptr++ = (char)c;
                  } /* Endif */
                  if ( width ) {
                     if ( --width <= 0 ) {
                        break;
                     } /* Endif */
                  } /* Endif */

               } /* Endwhile */

string_done:
               if ( ! suppress_field ) {
                  ++n;
                  *sptr = '\0';
               } /* Endif */
               break;

            case 'C':
               if ( width == 0 ) {
                  width = 1;
               } /* Endif */

               if ( ! suppress_field ) {
                  sptr = (char *)va_arg(args_ptr, void *);
               } /* Endif */
               while ( width-- > 0 ) {
                  if ( ! suppress_field ) {
                     *sptr++ = (unsigned char)*line_ptr;
                  } /* Endif */

                  ++line_ptr;
                  ++number_of_chars;
               } /* Endwhile */

               if ( ! suppress_field ) {
                  ++n;
               } /* Endif */
               break;

            case 'N':
               if ( ! suppress_field ) {
                  tmp_ptr = va_arg(args_ptr, void *);
                  *(int32_t *)(tmp_ptr) = (int32_t)number_of_chars;
               } /* Endif */
               break;

#if INCLUDE_FLOATING_POINT_IO
         case 'G':
         case 'F':
         case 'E':
            dnum = strtod((char *)line_ptr, (char  **)&tmp_ptr);

            if ((dnum == HUGE_VAL) || (dnum == -HUGE_VAL))
            {
               return (int32_t)n;
            }

            line_ptr = tmp_ptr;
            tmp_ptr = (void *)va_arg(args_ptr, void *);
            if (SCAN_LLONG == numtype)
            {
               *((double *)tmp_ptr) = dnum;
            } else {
               *((float *)tmp_ptr) = (float)dnum;
            }
            ++n;
         break;
#endif

            default:
               return (int32_t)n;

         } /* End Switch */

      } /* Endif */
#if 0
      if ( ! *line_ptr || ! *format ) {
         return (int32_t)n;
      } /* Endif */
#endif

      /* if end of input string, return */

   } /* Endwhile */
   return (int32_t)n;

}

int __io_sscanf(char  *str_ptr, const char  *format_ptr, ...)
{	va_list ap;
	int result;

	va_start(ap, format_ptr);
	result = _io_scanline(str_ptr, (char *)format_ptr, ap);
	va_end(ap);
	return result;
}

#define _MQX_IO_DIVISION_ADJUST_64 1000000000000000000LL
#define PRINT_OCTAL_64   (22L)
#define PRINT_DECIMAL_64 (20L)
#define PRINT_HEX_64     (16L)

#define _MQX_IO_DIVISION_ADJUST 1000000000L
#define PRINT_OCTAL   (11L)
#define PRINT_DECIMAL (10L)
#define PRINT_HEX     (8L)

#define PRINT_ADDRESS (8L)

#define MAX_INT_32    (0x7FFFFFFFL)
#define _CODE_PTR_ *
typedef int32_t (_CODE_PTR_ IO_PUTCHAR_FPTR)( int32_t, void *);

static const char upper_hex_string[] = "0123456789ABCDEF";
static const char lower_hex_string[] = "0123456789abcdef";

void _io_doprint_prt64(int64_t num, unsigned char *str_ptr, int32_t type, uint8_t use_caps)
{ /* Body */
   register int32_t i;
   char     temp[32];

   temp[0] = '\0';
   if (type == PRINT_OCTAL_64) {
      for (i = 1; i <= type; i++) {
         temp[i] = (char)((num & 0x7) + '0');
         num >>= 3;
      } /* Endfor */
      temp[PRINT_OCTAL_64] &= '1';
   } else if (type == PRINT_DECIMAL_64) {
      for (i = 1; i <= type; i++) {
         temp[i] = (char)(num % 10 + '0');
         num /= 10;
      } /* Endfor */
   } else {
      for (i = 1; i <= type; i++) {
         temp[i] = use_caps ? upper_hex_string[num & 0xF] : lower_hex_string[num & 0xF];
         num >>= 4;
      } /* Endfor */
   } /* Endif */
   for (i = type; temp[i] == '0'; i--) {
      ;
   } /* Endfor */
   if (i == 0) {
      i++;
   } /* Endif */
   while (i >= 0) {
      *str_ptr++ = temp[i--];
   } /* Endwhile */

} /* Endbody */

void _io_doprint_prt(int32_t num, unsigned char *str_ptr, int32_t type, uint8_t use_caps)
{ /* Body */
   register int32_t i;
   char     temp[16];

   temp[0] = '\0';
   if (type == PRINT_OCTAL) {
      for (i = 1; i <= type; i++) {
         temp[i] = (char)((num & 0x7) + '0');
         num >>= 3;
      } /* Endfor */
      temp[PRINT_OCTAL] &= '3';
   } else if (type == PRINT_DECIMAL) {
      for (i = 1; i <= type; i++) {
         temp[i] = (char)(num % 10 + '0');
         num /= 10;
      } /* Endfor */
   } else {
      for (i = 1; i <= type; i++) {
         temp[i] = use_caps ? upper_hex_string[num & 0xF] : lower_hex_string[num & 0xF];
         num >>= 4;
      } /* Endfor */
   } /* Endif */
   for (i = type; temp[i] == '0'; i--) {
      ;
   } /* Endfor */
   if (i == 0) {
      i++;
   } /* Endif */
   while (i >= 0) {
      *str_ptr++ = temp[i--];
   } /* Endwhile */

}

static int32_t _io_mputc(void *farg, IO_PUTCHAR_FPTR func_ptr, int32_t max_count, char c, int32_t count)
{	int32_t   cnt = 0;

    if (count < 0)
    {	count = 0;
    }

    for (;(cnt < count); cnt++)
    {
        if (max_count > (cnt + 1))
        {
            if (c != func_ptr((int32_t) c, farg))
            {
                break;
            }
        }
    }

    return cnt;
}

static int32_t _io_putstr(void *farg, IO_PUTCHAR_FPTR func_ptr, int32_t max_count, char *str)
{	int32_t res = 0;
    char compare = TRUE;

    for ( ; *str && compare; res++, str++)
    {	if (max_count > (res + 1))
        {	compare = ((int32_t) *str == func_ptr((int32_t) *str, farg));
        }
        else
        {	compare = TRUE;
        }
    }

    if (*str)
    {	res--;
    }
    return res;
}

int32_t _io_doprint(void *farg, IO_PUTCHAR_FPTR func_ptr, int32_t max_count, register char *fmt_ptr, va_list args_ptr)
{ /* Body */
   unsigned char    string[32];   /* The string str points to this output   */
   char     c;
   int32_t   i;
   int64_t   i64;
   int32_t f;            /* the format character (comes after %)   */
   unsigned char      *str_ptr;  /* running pointer in string              */
   /*    from number conversion              */
   int32_t length;       /* Length of string "str"                 */
   char     fill;         /* Fill character ( ' ' or '0' )          */
   int32_t leftjust;     /* 0 = right-justified, else left-just.   */
   int32_t prec,width;   /* Field specifications % width . precision */
   int32_t leading;      /* No. of leading/trailing fill chars.    */
   unsigned char    sign;         /* Set to '-' for negative decimals       */
   int      digit1;       /* Offset to add to first numeric digit.  */
   char     use_caps;     /* Hex output to use capital letters.     */
   char     use_signs;    /* Always output a sign for %d            */
   char     use_space;    /* If positive and not sign, print space for %d */
   char     use_prep;     /* Prepend 0 for octal, 0x , 0X for hex   */
   char     use_int_prec; /* Precision usage with d, i, o, u, x, X  */
   char     zero_value;   /* indicates 0 output value */
   char     zero_char;    /* indicates 0x00 char  */
   char     prepend;      /* What prepend type 3 octal 2 0x, 1 0X   */
   int32_t arg_length;   /* optional argument length               */
   char     done;
   char     is_integer;
   int32_t number_of_characters;  /* # chars printed out */
#if INCLUDE_FLOATING_POINT_IO
   char     prec_set;
   double   fnumber;
   char     fstring[324]; /* Floating point output string           */
#endif
   uint32_t *temp_ptr;
   int32_t    func_result = 0;

   enum types{ HALF, HALFHALF, LONG, LONGLONG, LONGDOUBLE };

   number_of_characters = 0;
   /* if max_count == -1 then maximal positive number is stored in max_count */
   /* it is not gnu standard because max count shoud be unsigned */
   if (max_count == -1)
   {	max_count = MAX_INT_32;
   }
   for ( ;; ) {
      /* Echo characters until '%' or end of fmt_ptr string */
      while ((c = *fmt_ptr++ ) != '%')
      {
         if (c == '\0')
         {
            if (func_result == IO_EOF)
            {
               return(IO_EOF);
            } else
            {
               return number_of_characters;
            } /* Endif */
         } /* Endif */
         number_of_characters++;
         if (max_count > number_of_characters)
         {
            func_result = (*func_ptr)((int32_t)c, farg);
         }
      } /* Endwhile */

      /* Echo "...%%..." as '%' */
      if (  *fmt_ptr == '%'  )
      {
         number_of_characters++;
         if (max_count > number_of_characters)
         {
             func_result = (*func_ptr)((int32_t)*fmt_ptr++, farg);
         }
            continue;
      } /* Endif */

      /* Set default flag */
      fill         = ' ';
      sign         = '\0';
      arg_length   = sizeof(int32_t);
      leftjust     = 0;
      use_caps     = 0;
      use_space    = 0;
      use_prep     = 0;
      use_signs    = 0;
      use_int_prec = 0;
      prepend      = 0;
      zero_value   = 0;
      zero_char    = 0;
#if INCLUDE_FLOATING_POINT_IO
      prec_set   = 0;
#endif

      /* Collect Valid Flags */
      done = FALSE;
      while ( *fmt_ptr && ! done ) {
         switch ( *fmt_ptr ) {

            case '-':
               /* Check for "%-..." EQ Left-justified output */
               leftjust = 1;
               fmt_ptr++;
               break;

            case '0':
               fill = '0';
               fmt_ptr++;
               break;

            case '+':
               use_signs = 1;
               fmt_ptr++;
               break;

            case ' ':
               use_space = 1;
               fmt_ptr++;
               break;

            case '#':
               use_prep = 1;
               fmt_ptr++;
               break;

            default:
               done = TRUE;
            break;
         } /* Endswitch */
      } /* Endwhile */

      /* Allow for minimum field width specifier for %d,u,x,o,c,s */
      /* Also allow %* for variable width ( %0* as well)           */
      width = 0;
      if (  *fmt_ptr == '*'  ) {
         width = va_arg(args_ptr, uint32_t);
         ++fmt_ptr;
      } else {
         while (  ('0' <= *fmt_ptr) && (*fmt_ptr <= '9')  ) {
            width = width * 10 + *fmt_ptr++ - '0';
         } /* Endwhile */
         /* Allow for maximum string width for %s */
      } /* Endif */

      prec = -1;
      if (  *fmt_ptr == '.'  ) {
         prec = 0;
         if (  *(++fmt_ptr) == '*'  ) {
            prec = va_arg(args_ptr, uint32_t);
#if INCLUDE_FLOATING_POINT_IO
            prec_set = 1;
#endif
            ++fmt_ptr;
         } else {
            while (  ('0' <= *fmt_ptr) && (*fmt_ptr <= '9')  ) {
               prec = prec * 10 + *fmt_ptr++ - '0';
#if INCLUDE_FLOATING_POINT_IO
               prec_set = 1;
#endif
            } /* Endwhile */
         } /* Endif */
      } /* Endif */

      str_ptr = string;
      if (  (f = *fmt_ptr++) == '\0'  ) {
         number_of_characters++;
         if (max_count > number_of_characters)
         {
             func_result = (*func_ptr)((int32_t)'%', farg);
         }
         if (func_result == IO_EOF) {
            return(IO_EOF);
         } else {
            return number_of_characters;
         } /* Endif */
      } /* Endif */

        switch (f)
        {
        case 'h' :
            arg_length = HALF;
            f = *fmt_ptr++;
            if (f == 'h')
            {
                arg_length = HALFHALF;
                f = *fmt_ptr++;
            }
            break;
        case 'l' :
            arg_length = LONG;
            f = *fmt_ptr++;
            if (f == 'l')
            {
                arg_length = LONGLONG;
                f = *fmt_ptr++;
            }
            break;
        case 'L' :
            arg_length = LONGDOUBLE;
            f = *fmt_ptr++;
            break;
        default:
            break;
        }

      use_caps = FALSE;
      is_integer = FALSE;
      length = 0;
      switch (  f  ) {
         case 'c':
         case 'C':
            string[0] = (char)va_arg(args_ptr, int32_t);
            if (string[0] == 0) {
               length += 1;
               zero_char = 1;
            } /* Endif */
            string[1] = '\0';
            prec = 0;
            fill = ' ';
            use_prep = 0;
            break;

         case 's':
         case 'S':
            str_ptr = (unsigned char *)va_arg(args_ptr, void *);
            fill = ' ';
            use_prep = 0;
            break;

         case 'i':
         case 'I':
         case 'd':
         case 'D':
            is_integer = TRUE;
         case 'u':
         case 'U':
            if (f == 'u' || f == 'U') {
            }
            if (prec > 0) {
                use_int_prec = 1;
            }
            if (is_integer) {
               if (arg_length == LONGLONG) {
                  i64 = va_arg(args_ptr, int64_t);
                  zero_value = i64 == 0L;
                  if (i64 < 0) {
                     sign = '-';
                     i64 = -i64;
                  } else if ( use_signs ) {  /* Always a sign */
                     sign = '+';
                  } else if ( use_space ) {  /* use spaces */
                     sign = ' ';
                  } /* Endif */
               } else {
                  if (arg_length == HALF) {
                     i = (int16_t) va_arg(args_ptr,  signed int);
                  } else if (arg_length == HALFHALF) {
                     i = (int8_t) va_arg(args_ptr,  signed int);
                  } else if (arg_length == LONG) {
                     i = va_arg(args_ptr, int32_t);
                  } else {
                     i = va_arg(args_ptr, int32_t);
                  }
                  zero_value = i == 0;
                  if (i < 0) {
                     sign = '-';
                     i = -i;
                  } else if ( use_signs ) {  /* Always a sign */
                     sign = '+';
                  } else if ( use_space ) {  /* use spaces */
                     sign = ' ';
                  } /* Endif */
               }
            } else {
               if (arg_length == HALF) {
                  i = (uint16_t) va_arg(args_ptr, unsigned int);
                  zero_value = i == 0;
               } else if (arg_length == HALFHALF) {
                  i = (uint8_t) va_arg(args_ptr, unsigned int);
                  zero_value = i == 0;
               } else if (arg_length == LONG) {
                  i = va_arg(args_ptr, uint32_t);
                  zero_value = i == 0;
               } else if (arg_length == LONGLONG) {
                  i64 = va_arg(args_ptr, uint64_t);
                  zero_value = i64 == 0L;
               } else {
                  i = va_arg(args_ptr, uint32_t);
                  zero_value = i == 0;
               }/* Endif */
            } /* Endif */
            digit1 = '\0';
            /* "negative" longs in unsigned format       */
            /* can't be computed with long division      */
            /* convert *args_ptr to "positive", digit1   */
            /* = how much to add back afterwards         */
            if (arg_length == LONGLONG) {
               while (i64 < 0) {
                  i64 -= _MQX_IO_DIVISION_ADJUST_64;
                  ++digit1;
               } /* Endwhile */
               _io_doprint_prt64(i64, str_ptr + 1, PRINT_DECIMAL_64, (uint8_t)use_caps);
               str_ptr[1] += digit1;
               if (str_ptr[1] > '9') {
                  str_ptr[1] -= 10;
                  str_ptr[0] = '1';
               } else {
                  str_ptr += 1;
               }
            } else {
               while (i < 0) {
                  i -= _MQX_IO_DIVISION_ADJUST;
                  ++digit1;
               } /* Endwhile */
               _io_doprint_prt(i, str_ptr, PRINT_DECIMAL, (uint8_t)use_caps);
               str_ptr[0] += digit1;
            }
            use_prep = 0;
            break;

         case 'o':
         case 'O':
            if (prec > 0) {
               use_int_prec = 1;
            }
            if (arg_length == LONGLONG) {
               i64 = va_arg(args_ptr, uint64_t);
               zero_value = i64 == 0L;
               _io_doprint_prt64(i64, str_ptr, PRINT_OCTAL_64, (uint8_t)use_caps);
            } else {
               if (arg_length == HALF) {
                  i = (uint16_t) va_arg(args_ptr, unsigned int);
               } else if (arg_length == HALFHALF) {
                  i = (uint8_t) va_arg(args_ptr, unsigned int);
               } else if (arg_length == LONG) {
                  i = va_arg(args_ptr, uint32_t);
               } else {
                  i = va_arg(args_ptr, uint32_t);
               }/* Endif */
               zero_value = i == 0;
               _io_doprint_prt(i, str_ptr, PRINT_OCTAL, (uint8_t)use_caps);
            }
            prepend = 3;
            break;

         case 'P':
            use_caps = TRUE;
         case 'p':
            leading = 0;
            temp_ptr = (uint32_t *) va_arg(args_ptr, void *);
            _io_doprint_prt((uint32_t)temp_ptr, str_ptr, PRINT_ADDRESS,
               (uint8_t)use_caps);
         break;

         case 'X':
            use_caps = TRUE;
            prepend  = 1;
         case 'x':
            if (prec > 0) {
                use_int_prec = 1;
            }
            prepend++;
            if (arg_length == LONGLONG) {
               i64 = va_arg(args_ptr, uint64_t);
               zero_value = i64 == 0L;
               _io_doprint_prt64(i64, str_ptr, PRINT_HEX_64, (uint8_t)use_caps);
            } else {
               if (arg_length == HALF) {
                  i = (uint16_t) va_arg(args_ptr, unsigned int);
               } else if (arg_length == HALFHALF) {
                  i = (uint8_t) va_arg(args_ptr, unsigned int);
               } else if (arg_length == LONG) {
                  i = va_arg(args_ptr, uint32_t);
               } else {
                  i = va_arg(args_ptr, uint32_t);
               }/* Endif */
               zero_value = i == 0;
               _io_doprint_prt(i, str_ptr, PRINT_HEX, (uint8_t)use_caps);
            }
            break;

         case 'n':   /* Defaults to size of number on stack */
         case 'N':   /* Defaults to size of number on stack */
            str_ptr = (unsigned char *)va_arg(args_ptr, void *);
            *(int32_t *)(str_ptr) = (int32_t)number_of_characters;
            continue;

#if INCLUDE_FLOATING_POINT_IO

         case 'g':
         case 'G':

             if (! prec_set)
             {
                prec = 6;
             }
             if (arg_length == LONGDOUBLE) {
                fnumber = va_arg(args_ptr, long double);
             } else {
                fnumber = va_arg(args_ptr, double);
             }/* Endif */
             i = _io_dtog(fstring, fnumber, &fill, use_signs, use_space, use_prep,
                (prec - 1), (char)f);
             str_ptr = (unsigned char *)fstring;
             if (*str_ptr == '+') {
                sign = '+';
                str_ptr++;
             } else if (*str_ptr == '-') {
                sign = '-';
                str_ptr++;
             }
             use_prep = 0;
             prec = 0;
          break;

         case 'f':
         case 'F':
            if (! prec_set) {
               prec = 6;
            } /* Endif */
            if (prec == 0) {
               prec = 1;
            }
            if (arg_length == LONGDOUBLE) {
               fnumber = va_arg(args_ptr, long double);
            } else {
               fnumber = va_arg(args_ptr, double);
            }/* Endif */
            i = _io_dtof(fstring, fnumber, &fill, use_signs, use_space, use_prep,
               prec, (char)f);
            str_ptr = (unsigned char *)fstring;
            if (*str_ptr == '+') {
               sign = '+';
               str_ptr++;
            } else if (*str_ptr == '-') {
               sign = '-';
               str_ptr++;
            }
            use_prep = 0;
            prec = 0;
         break;

         case 'e':
         case 'E':
            if (! prec_set) {
               prec = 6;
            }
            if (arg_length == LONGDOUBLE) {
               fnumber = va_arg(args_ptr, long double);
            } else {
               fnumber = va_arg(args_ptr, double);
            }/* Endif */
            i = _io_dtoe(fstring, fnumber, &fill, use_signs, use_space, use_prep,
               prec, (char)f);
            str_ptr = (unsigned char *)fstring;
            if (*str_ptr == '+') {
               sign = '+';
               str_ptr++;
            } else if (*str_ptr == '-') {
               sign = '-';
               str_ptr++;
            }
            use_prep = 0;
            prec = 0;
         break;
#endif

         default:
            number_of_characters++;
            if (max_count > number_of_characters)
            {
                func_result = (*func_ptr)((int32_t)f, farg);
            }
            if (func_result == IO_EOF) {
               return(IO_EOF);
            } else {
               return number_of_characters;
            } /* Endif */
      } /* End Switch */

      for (; str_ptr[length] != '\0'; length++ )
      {
         ;
      } /* Endfor */

      if ( (width < 0) )
      {
         width = 0;
      } /* Endif */
      if ( (prec < 0) )
      {
         prec = 0;
      }
      else
      {
         if (('s' == f) || ('S' == f))
         {
            if (length > prec)
            {
               length = prec;
            }
         }
      }
      leading = 0;

      if ( (prec != 0) || (width != 0) ) {
         /*
          * prec = precision
          *    For a string, the maximum number of characters to be printed
          *    from the string.
          *    for f,e,E the number of digits after the decimal point
          *    for g,G the number of significant digits
          *    for integer, the minimum number of digits to be printed
          *       leading 0's will precede
          */
         if ( !use_int_prec && (prec != 0) ) {
            if ( length > prec ) {
               if ( str_ptr == string ) {
                  /* this was a numeric input */
                  /* thus truncate to the left */
                  str_ptr += (length - prec);
               } /* Endif */
               length = prec;
            } /* Endif */
         } /* Endif */

         /* width = minimum field width
          *    argument will be printed in a field at least this wide,
          *    and wider if necessary.  If argument has fewer characters
          *    than the field width, is will be padded on left or right
          *    to make up the width.  Normally pad ' ' unless 0 specified.
          */
         if (use_int_prec) {
            if (width > prec) {
               leading = (prec > length) ? width - prec : width - length;
               if ( !zero_value && use_prep && (prepend == 3) && (prec > length)) {
                  leading++; /* correction for octal because one prepend 0 is pad as well as prepend character */
               }
            } else {
               leading = 0;
            }
         } else if ( width != 0 ) {
            leading = width - length;
         } /* Endif */

         if ( sign ) {
            --leading;
         } /* Endif */
         if ( use_prep && !zero_value ) {
            --leading;
            if ( prepend < 3 ) { /* HEX */
               --leading;
            } /* Endif */
         } /* Endif */

      } /* Endif */

      if (leading < 0) {
          leading = 0;
      }

      if ( leftjust == 0 ) { /* main flow for right justification */
         if (use_int_prec) {
            /* print out leading spaces first */
            number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, ' ', leading);

            /* print out sign if there's any */
            if ( ((sign == '+') || (sign == '-') || (sign == ' ')) ) { /* Leading '-', '+', ' ' */
               number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, sign, 1);
            }

            /* handle leading '0', '0x' or '0X' */
            if ( use_prep && !zero_value ) {
               switch (prepend) {
               case 1:
                  number_of_characters += _io_putstr(farg, func_ptr, max_count - number_of_characters, "0x");
                  break;
               case 2:
                  number_of_characters += _io_putstr(farg, func_ptr, max_count - number_of_characters, "0X");
                  break;
               default:
                  if (!((prec - length) > 0)) {
                     number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, '0', 1);
                  }
               }
            } /* Endif */

            /* print out leading 0's given by precision */
            number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, '0', prec - length);
         } else { /* right justification without precision for integer types */
            if (fill == '0') { /* flow for leading zeroes */
               /* print out sign if there's any */
               if ((sign == '+') || (sign == '-') || (sign == ' ')) { /* Leading '-', '+', ' ' */
                  number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, sign, 1);
               }

               /* handle leading '0', '0x' or '0X' */
               if ( use_prep && !zero_value ) {
                  switch (prepend) {
                  case 1:
                     number_of_characters += _io_putstr(farg, func_ptr, max_count - number_of_characters, "0x");
                     break;
                  case 2:
                     number_of_characters += _io_putstr(farg, func_ptr, max_count - number_of_characters, "0X");
                     break;
                  default:
                     number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, '0', 1);
                  }
               } /* Endif */

               /* print out leading 0's */
               number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, fill, leading);
            } else { // flow for leading spaces
               /* print out leading spaces */
               number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, fill, leading);

               /* print out sign if there's any */
               if ((sign == '+') || (sign == '-') || (sign == ' ')) { /* Leading '-', '+', ' ' */
                  number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, sign, 1);
               }

               /* handle leading '0', '0x' or '0X' */
               if ( use_prep && !zero_value ) {
                  switch (prepend) {
                  case 1:
                     number_of_characters += _io_putstr(farg, func_ptr, max_count - number_of_characters, "0x");
                     break;
                  case 2:
                     number_of_characters += _io_putstr(farg, func_ptr, max_count - number_of_characters, "0X");
                     break;
                  default:
                      number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, '0', 1);
                  }
               } /* Endif */
            }
         }
      } else { /* left justification has different flow */
         /* print out sign if there's any */
         if ( ((sign == '+') || (sign == '-') || (sign == ' ')) ) { /* Leading '-', '+', ' ' */
            number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, sign, 1);
         }

         /* handle leading '0', '0x' or '0X' */
         if ( use_prep && !zero_value ) {
            switch (prepend) {
            case 1:
               number_of_characters += _io_putstr(farg, func_ptr, max_count - number_of_characters, "0x");
               break;
            case 2:
               number_of_characters += _io_putstr(farg, func_ptr, max_count - number_of_characters, "0X");
               break;
            default:
               if (!use_int_prec || ((prec - length) < 0)) {
                   number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, '0', 1);
               }
            }
         } /* Endif */

         if (use_int_prec) {
            /* print out leading 0's given by precision */
            number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, '0', prec - length);
         }
      }

      /* print out actual characters */
      length++;
      if (length > max_count - number_of_characters)
      {
      	 length = max_count - number_of_characters;
      }

      if(zero_char) {
         number_of_characters += _io_mputc(farg, func_ptr, length, *str_ptr, 1);
      } else {
         number_of_characters += _io_putstr(farg, func_ptr, length, (char *) str_ptr);
      }

      if (leftjust != 0) { /* Print trailing pad characters */
         /* print out leading 0's */
         number_of_characters += _io_mputc(farg, func_ptr, max_count - number_of_characters, ' ', leading);
      } /* Endif */

   } /* Endfor */
}

int32_t _io_sputc(int32_t c, void *input_string_ptr)
{ /* Body */
   char **string_ptr = (char  **)((void *)input_string_ptr);

   *(*string_ptr)++ = (char)c;
   return c;
}

int32_t _io_sprintf(char *str_ptr, const char *fmt_ptr, ...)
{	int32_t result;
	va_list ap;

	va_start(ap, fmt_ptr);
	result = _io_doprint(((void *)&str_ptr), _io_sputc, -1, (char *)fmt_ptr, ap);
	*str_ptr = '\0';
	va_end(ap);
	return result;
}