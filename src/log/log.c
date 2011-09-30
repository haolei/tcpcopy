#include "log.h"

static FILE* file=NULL;
int output_level;

static char* err_levels[] = { 
	"unknown",
	"emerg",
	"alert",
	"crit",
	"error",
	"warn",
	"notice",
	"info",
	"debug"
};

void initLogInfo()
{
	output_level=LOG_DEBUG;
	file=fopen("error.log","a+");
}

void logInfo(int level,const char *fmt, ...)
{
	va_list args;
	if(output_level >= level)
	{
		if (file) {
			time_t t;
			t=time(0);
			fprintf(file,"[%s] ",err_levels[level]);
			char* timeStr=asctime(localtime(&t));
			size_t len=strlen(timeStr);
			timeStr[len-1]=':';
			fprintf(file,"%s",timeStr);
			va_start(args, fmt);
			(void)vfprintf(file, fmt, args);
			fprintf( file, "\n" );
			va_end(args);
		}
	}
}

void endLogInfo()
{
	if(file)
	{
		fclose(file);
	}
}

