#ifndef _LOG_MODULE_H

#define _LOG_MODULE_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

enum log_level
{
	LL_ERROR=3,
	LL_VERBOSE,
	LL_DEBUG,
	LL_HIGHEST
};

extern short g_log_level;

#define DEBUG_LEVEL LL_DEBUG
/*macros to save log to file*/
#define flogd(file,arg...) _log_this(LL_DEBUG,file,##arg)
#define flogv(file,arg...) _log_this(LL_VERBOSE,file,##arg)
#define floge(file,arg...) _log_this(LL_ERROR,file,##arg)
/*macros to save log to stdout. redirect applications stdout to a file in append mode when this macro is used.*/
#define logd(arg...) _log_this(LL_DEBUG,NULL,##arg)
#define logv(arg...) _log_this(LL_VERBOSE,NULL,##arg)
#define loge(arg...) _log_this(LL_ERROR,NULL,##arg)

int _log_this(int level,char *file,const char *format, ...);
char *get_time_str(void);
void set_log_level(short level);
short get_log_level();

#endif /* end of include guard: _LOG_MODULE_H */
