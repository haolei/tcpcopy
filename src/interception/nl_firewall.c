#include "nl.h"
#include "nl_firewall.h"

int nl_firewall_init(){
	int sock = nl_init(NETLINK_FIREWALL,FIREWALL_GROUP);
	nl_set_mode(sock,IPQ_COPY_PACKET,65536);
	return sock;
}

static char buffer[65536];

struct iphdr *nl_firewall_recv(int sock,int *packet_id){
	nl_recv(sock,buffer,sizeof(buffer));
	struct ipq_packet_msg *msg = nl_payload(buffer);
	*packet_id=msg->packet_id;
	return (struct iphdr *)msg->payload;
}


