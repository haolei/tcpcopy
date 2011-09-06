/*
 * =====================================================================================
 *
 *       Filename:  log.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/10/11 16:44:05
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangbin
 *        Company:  netease
 *
 * =====================================================================================
 */


#ifndef  _LOG_H_INC
#define  _LOG_H_INC

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef __cplusplus
extern "C" 
{
#endif

void logInfo(char *fmt, ...);

#ifdef __cplusplus
}
#endif


#endif  

