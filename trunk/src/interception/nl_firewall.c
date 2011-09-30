#include <limits.h>
#include <stdio.h>
#include <netinet/in.h>
#include "../log/log.h"
#include "nl.h"
#include "nl_firewall.h"

int nl_firewall_init(){
	int sock = nl_init(NETLINK_FIREWALL,FIREWALL_GROUP);
	nl_set_mode(sock,IPQ_COPY_PACKET,65536);
	return sock;
}

static char buffer[65536];

struct iphdr *nl_firewall_recv(int sock,unsigned long *packet_id){
	ssize_t len = nl_recv(sock,buffer,sizeof(buffer));
	ssize_t normalLen=sizeof(struct ipq_packet_msg)+NLMSG_LENGTH(0);
	if(len < normalLen)
	{
		logInfo(LOG_ERR,"nl recv error:%ld",len);
		logInfo(LOG_ERR,"privilage problems(use root) or other error");
		fprintf(stderr,"privilage problems(use root) or other error\n");
		exit(-1);
	}else
	{
		struct ipq_packet_msg *msg = nl_payload(buffer);
		*packet_id=msg->packet_id;
		return (struct iphdr *)msg->payload;
	}
}


