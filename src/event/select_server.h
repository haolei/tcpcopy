/*
 * =====================================================================================
 *
 *       Filename:  select_server.h
 *       Compiler:  gcc
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description:  
 *
 *        Created:  2010-07-12 22:11:24
 * =====================================================================================
 */

#ifndef  _TCPCOPY_SELECT_SERVER_H__INC
#define  _TCPCOPY_SELECT_SERVER_H__INC

#ifdef __cplusplus
extern "C"
{
#endif

	typedef void (*select_server_func)(int fd);

	void select_sever_set_callback(select_server_func func);
	void select_sever_add(int);
	void select_sever_del(int);

	void select_server_run();

#ifdef __cplusplus
}
#endif

#endif   /* ----- #ifndef _TCPCOPY_SELECT_SERVER_H__INC  ----- */



