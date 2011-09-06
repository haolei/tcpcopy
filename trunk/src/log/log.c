/*
 * =====================================================================================
 *
 *       Filename:  log.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/10/11 16:46:30
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbin
 *        Company:  netease
 *
 * =====================================================================================
 */

#include "log.h"

void logInfo(char *fmt, ...)
{
	FILE* file = fopen("/tmp/tmp.log", "a+");
	va_list args;
	if (file) {
		time_t t;
		t=time(0);
		char* timeStr=asctime(localtime(&t));
		size_t len=strlen(timeStr);
		timeStr[len-1]=':';
		fprintf(file,timeStr);
		va_start(args, fmt);
		(void)vfprintf(file, fmt, args);
		fprintf( file, "\n" );
		va_end(args);
		if (file != stderr)
		{   
			(void)fclose(file);
		}   
	}   
}

