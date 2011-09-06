/*
 * =====================================================================================
 *
 *       Filename:  nl_firewall.h
 *       Compiler:  gcc
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description:  
 *
 *        Created:  2010-07-21 19:46:28
 * =====================================================================================
 */


#ifndef  _TCPCOPY_NL_FIREWALL_H__INC
#define  _TCPCOPY_NL_FIREWALL_H__INC

#ifdef __cplusplus
extern "C"
{
#endif

#include "nl.h"
#include <linux/netfilter_ipv4/ip_queue.h>

#define FIREWALL_GROUP  0

int nl_firewall_init();
struct iphdr *nl_firewall_recv(int sock,int *packet_id);

#ifdef __cplusplus
}
#endif
#endif   /* ----- #ifndef _TCPCOPY_NL_FIREWALL_H__INC  ----- */
