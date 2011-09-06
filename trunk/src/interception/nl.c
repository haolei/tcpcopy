/*
 * =====================================================================================
 *
 *       Filename:  nl.c
 *       Compiler:  gcc
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description:  
 *
 *        Created:  2010-07-21 19:02:07
 * =====================================================================================
 */

#include "nl.h"

static int sock_init(int protocol){
	int sock = socket(AF_NETLINK,SOCK_RAW,protocol);
	if(sock == -1){
		perror("socket:");
		exit(errno);
	}
	return sock;
}

static void sock_bind(int sock,int groups){
	struct sockaddr_nl addr;
	memset(&addr,0,sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	addr.nl_groups = groups;
	if(bind(sock,(struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror("bind:");
		exit(errno);
	}
}

int nl_init(int protocol,int groups){
	int sock = sock_init(protocol);
	sock_bind(sock,groups);
	int rcvbuf = 1024*1024;
	setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&rcvbuf,sizeof(rcvbuf));
	return sock;
}

void nl_set_mode(int sock,uint8_t mode,size_t range){
	struct {
		struct nlmsghdr head;
		ipq_peer_msg_t  body;
	}req;
	memset(&req, 0, sizeof(req));
	req.head.nlmsg_len = NLMSG_LENGTH(sizeof(req));
	req.head.nlmsg_flags = NLM_F_REQUEST;
	req.head.nlmsg_type = IPQM_MODE;
	req.head.nlmsg_pid = getpid();
	req.body.msg.mode.value = mode;
	req.body.msg.mode.range = range;
	struct sockaddr_nl addr;
	memset(&addr,0,sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = 0;
	addr.nl_groups = 0;
	if(sendto(sock, &req, req.head.nlmsg_len,0,(struct sockaddr *)&addr,sizeof(addr)) < 0){
		perror("cannot set mode:");
		exit(errno);
	}
}

ssize_t nl_recv(int sock,void *buffer,size_t length){
	ssize_t recvlen = recv(sock,buffer,length,0);
	if(recvlen <0 ){
		return -1;
	}
	if((size_t)recvlen < sizeof(struct nlmsghdr)){
		printf("msg wrong\n");
		return -1;
	}
	return recvlen;
}

void *nl_payload(void *buf){
	 return NLMSG_DATA((struct nlmsghdr *)(buf));
}


