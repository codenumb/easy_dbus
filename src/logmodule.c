#include "logmodule.h"
/**********************************************************************************************/
short g_log_level=LL_HIGHEST;

void set_log_level(short level)
{
	g_log_level=level;
}

short get_log_level()
{
	return g_log_level;
}
/**
 * Function to get current time string in Y-M-d HH:MM:SS format.
 * @return time string.
 */
char *get_time_str(void) 
{
	static char time_str[30];
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
	return (time_str);
}

/**
 *Function to save the logs to file or stdout. if the requisted log level is higher than applications
 * log level, then the logs are discarded. this is usefull to make release build easly. 
 * Use provided macros to call this function instead calling directly.
 * @param level verbosity level, see log_level type.
 * @param file path to save the logs or pass NULL to save the log to stdout.
 * @param formatted log data.
 * @return -1 on error else number of chars written to the file.
 **/
int _log_this(int level,char *file,const char *format, ...)
{
	 if(level>g_log_level)
		 return 0;
	 va_list arg;
	 int ret=0;
	 FILE *fd=NULL;
	 if(file!=NULL)
	 {
	 	char path[75]={0};
		sprintf(path,"%s",file);
		fd=fopen(path,"a");
	  	if(fd==NULL)
		{
			printf("failed to open %s\n",path);
			return -1;
		}
	 }
	 else
	 	fd=stdout;
	 va_start (arg, format);
	 ret=fprintf (fd, "%s: ", get_time_str());
	 ret = vfprintf (fd, format, arg);
	 ret=fprintf (fd, "\n");
	 va_end (arg);
	 fflush(fd);
	 fclose(fd);
	 return ret;
}