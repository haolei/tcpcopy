/*
 * =====================================================================================
 *
 *       Filename:  interception.h
 *       Compiler:  gcc
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description:  
 *
 *        Created:  2010-07-12 18:25:10
 * =====================================================================================
 */


#ifndef  _TCPCOPY_SERVER_H__INC
#define  _TCPCOPY_SERVER_H__INC

#ifdef __cplusplus
extern "C
{
#endif

#include <stdint.h>

void interception_init();
void interception_run();
void interception_over();
#ifdef __cplusplus
}
#endif

#endif   /* ----- #ifndef _TCPCOPY_SERVER_H__INC  ----- */
