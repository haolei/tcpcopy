/*
 * =====================================================================================
 *
 *       Filename:  nl.h
 *       Compiler:  gcc
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description:  
 *
 *        Created:  2010-07-21 18:59:03
 * =====================================================================================
 */



#ifndef  _TCPCOPY_NL_H__INC
#define  _TCPCOPY_NL_H__INC

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <asm/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv4/ip_queue.h>


	int nl_init(int ,int);
	void nl_set_mode(int sock,uint8_t mode,size_t range);
	ssize_t nl_recv(int ,void *,size_t);
	void *nl_payload(void *);

#ifdef __cplusplus
}
#endif

#endif   /* ----- #ifndef _TCPCOPY_NLUTIL_H__INC  ----- */

