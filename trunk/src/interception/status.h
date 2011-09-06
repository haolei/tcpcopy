/*
 * =====================================================================================
 *
 *       Filename:  status.h
 *       Compiler:  g++
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description:
 *
 *        Created:  2010-07-24 07:50:13
 * =====================================================================================
 */

#ifndef  _TCPCOPY_RECEIVER_STATUS_H__INC
#define  _TCPCOPY_RECEIVER_STATUS_H__INC

#ifdef __cplusplus
extern "C"
{
#endif

#include <netinet/ip.h>
#include <stdint.h>

	void status_init();
	void status_update(struct iphdr *ip_header);
	void status_add(uint32_t ,uint16_t,int);
	void status_del(uint32_t ,uint16_t);

#ifdef __cplusplus
}
#endif
#endif   /* ----- #ifndef _TCPCOPY_RECEIVER_STATUS_H__INC  ----- */

