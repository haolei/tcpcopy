/*
 * =====================================================================================
 *
 *       Filename:  send.h
 *       Compiler:  g++
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description:  
 *
 *        Created:  2009-09-09 19:44:31
 * =====================================================================================
 */

#ifndef  _TCP_REDIRECT_SEND_H__INC
#define  _TCP_REDIRECT_SEND_H__INC



int send_init();
int send_close();
uint32_t send_ip_packet(bool isOutput,uint64_t fake_ip_addr,
		unsigned char *data,uint32_t ack_seq,uint32_t* sendSeq);


#endif   /* ----- #ifndef _TCP_REDIRECT_SEND_H__INC  ----- */


