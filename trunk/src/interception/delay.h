/*
 * =====================================================================================
 *
 *       Filename:  delay.h
 *       Compiler:  gcc
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description:
 *
 *        Created:  2010-07-25 16:16:06
 * =====================================================================================
 */


#ifndef  _TCPCOPY_RECEIVER_DELAY_H__INC
#define  _TCPCOPY_RECEIVER_DELAY_H__INC

#ifdef __cplusplus
extern "C"
{
#endif

#include "../communication/msg.h"

	void delay_table_init();
	void delay_table_add(uint64_t key,struct receiver_msg_st *);
	void delay_table_send(uint64_t key,int fd);

#ifdef __cplusplus
}
#endif

#endif   /* ----- #ifndef _TCPCOPY_RECEIVER_DELAY_H__INC  ----- */
