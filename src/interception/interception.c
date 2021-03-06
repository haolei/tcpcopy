#include <limits.h>
#include "../communication/msg.h"
#include "../event/select_server.h"
#include "nl_firewall.h"
#include "interception.h"
#include "router.h"
#include "delay.h"

static  int firewall_sock;
static  int msg_listen_sock;
passed_ip_addr passed_ips;

static void set_sock_no_delay(int sock){
	int flag = 1;
	if(setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(char *)&flag,
				sizeof(flag)) == -1){
		perror("setsockopt:");
		logInfo(LOG_ERR,"setsockopt error:%s",strerror(errno));
		sync(); 
		exit(errno);
	}else
	{
		logInfo(LOG_NOTICE,"setsockopt ok");
	}
}

static void formatOutput(int level,struct iphdr *ip_header)
{
	struct in_addr srcaddr;
	struct in_addr destaddr;
	char sbuf[1024];
	char dbuf[1024];
	char *tmpbuf=NULL;
	size_t size_ip;
	struct tcphdr *tcp_header=NULL;
	uint32_t packSize;
	unsigned int seq;
	unsigned int ack_seq;

	if(global_out_level < level)
	{
		return;
	}
	srcaddr.s_addr=ip_header->saddr;
	destaddr.s_addr=ip_header->daddr;
	tmpbuf=inet_ntoa(srcaddr);
	memset(sbuf,0,1024);
	strcpy(sbuf,tmpbuf);
	memset(dbuf,0,1024);
	tmpbuf=inet_ntoa(destaddr);
	strcpy(dbuf,tmpbuf);
	size_ip = ip_header->ihl<<2;
	tcp_header= (struct tcphdr*)((char *)ip_header+size_ip);
	packSize=ntohs(ip_header->tot_len);
	seq=ntohl(tcp_header->seq);
	ack_seq=ntohl(tcp_header->ack_seq);
	{
		logInfo(level,"%s:%u-->%s:%u,length %u ,seq=%u,ack_seq=%u",sbuf,
				ntohs(tcp_header->source),dbuf,ntohs(tcp_header->dest),
				packSize,seq,ack_seq);
	}

}

static uint32_t seq =1;
static unsigned char drop_buffer[128];

static int drop_netlink_packet(unsigned long packet_id)
{
	struct nlmsghdr* nl_header=(struct nlmsghdr*)drop_buffer;
	struct ipq_verdict_msg *ver_data = NULL;
	struct sockaddr_nl addr;

	/*
	 * The IPQM_VERDICT message is used to release packets 
	 * from the kernel ip queue module.
	 */
	nl_header->nlmsg_type=IPQM_VERDICT;
	nl_header->nlmsg_len=NLMSG_LENGTH(sizeof(struct ipq_verdict_msg));
	nl_header->nlmsg_flags=(NLM_F_REQUEST);
	nl_header->nlmsg_pid=getpid();
	nl_header->nlmsg_seq=seq++;
	ver_data=(struct ipq_verdict_msg *)NLMSG_DATA(nl_header);
	ver_data->value=NF_DROP;
	ver_data->id=packet_id;
	memset(&addr,0,sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = 0;
	addr.nl_groups = 0;
	/*
	 * In an effort to keep packets properly ordered,
	 * the impelmentation of the protocol requires that
	 * the user space application send an IPQM_VERDICT message
	 * after every IPQM PACKET message is received.
	 *
	 */
	if(sendto(firewall_sock,(void *)nl_header,nl_header->nlmsg_len,0,
				(struct sockaddr *)&addr,sizeof(struct sockaddr_nl))<0)
	{
		perror("unable to send mode message");
		logInfo(LOG_ERR,"unable to send mode message:%s",
				strerror(errno));
		sync(); 
		exit(0);
	}
	return 1;
}

static unsigned char pass_buffer[128];
static int pass_netlink_packet(unsigned long packet_id)
{
	struct nlmsghdr* nl_header=(struct nlmsghdr*)pass_buffer;
	struct ipq_verdict_msg *ver_data = NULL;
	struct sockaddr_nl addr;

	/*
	 * The IPQM_VERDICT message is used to release packets 
	 * from the kernel ip queue module.
	 */
	nl_header->nlmsg_type=IPQM_VERDICT;
	nl_header->nlmsg_len=NLMSG_LENGTH(sizeof(struct ipq_verdict_msg));
	nl_header->nlmsg_flags=(NLM_F_REQUEST);
	nl_header->nlmsg_pid=getpid();
	nl_header->nlmsg_seq=seq++;
	ver_data=(struct ipq_verdict_msg *)NLMSG_DATA(nl_header);
	ver_data->value=NF_ACCEPT;
	ver_data->id=packet_id;
	memset(&addr,0,sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = 0;
	addr.nl_groups = 0;
	/*
	 * In an effort to keep packets properly ordered,
	 * the impelmentation of the protocol requires that
	 * the user space application send an IPQM_VERDICT message
	 * after every IPQM PACKET message is received.
	 *
	 */
	if(sendto(firewall_sock,(void *)nl_header,nl_header->nlmsg_len,0,
				(struct sockaddr *)&addr,sizeof(struct sockaddr_nl))<0)
	{
		perror("unable to send mode message");
		logInfo(LOG_ERR,"unable to send mode message:%s",
				strerror(errno));
		sync(); 
		exit(0);
	}
	return 1;
}


static void interception_process(int fd){
	int newfd;
	int i=0;
	int pass_through_flag=0;
	unsigned long packet_id;
	struct iphdr *ip_header=NULL;
	struct copyer_msg_st *c_msg=NULL;

	if(fd == msg_listen_sock){
		newfd = accept(msg_listen_sock,NULL,NULL);	
		set_sock_no_delay(newfd);
		if(newfd != -1){
			select_sever_add(newfd);
		}
	}else if(fd == firewall_sock){
		packet_id=0;
		ip_header = nl_firewall_recv(firewall_sock,&packet_id);
		if(ip_header!=NULL)
		{
			/*check if it is the valid user to pass through firewall*/
			for(i=0;i<passed_ips.num;i++)
			{
				if(passed_ips.ips[i]==ip_header->daddr)
				{
					pass_through_flag=1;
					break;
				}
			}
			if(pass_through_flag)
			{
				pass_netlink_packet(packet_id);  	
			}else
			{
				router_update(ip_header);
#if (DEBUG_TCPCOPY)
				formatOutput(LOG_DEBUG,ip_header);
#endif
				/* 
				 * drop the packet 
				 */
				drop_netlink_packet(packet_id);  	
			}
		}
	}else{
		c_msg = msg_receiver_recv(fd);
		if(c_msg){
			if(c_msg->type == CLIENT_ADD){
				router_add(c_msg->client_ip,c_msg->client_port,fd);
			}else if(c_msg->type == CLIENT_DEL){
				router_del(c_msg->client_ip,c_msg->client_port);
			}
		}else{
			close(fd);
			select_sever_del(fd);
		}
	}
}

/* 
 * ===  FUNCTION  ==============================================
 *         Name:  interception_init
 *  Description:  init for interception
 * =============================================================
 */
void interception_init(){
	delay_table_init();
	router_init();
	select_sever_set_callback(interception_process);
	msg_listen_sock = msg_receiver_init();
	logInfo(LOG_NOTICE,"msg listen socket:%d",msg_listen_sock);
	select_sever_add(msg_listen_sock);
	firewall_sock = nl_firewall_init();
	logInfo(LOG_NOTICE,"firewall socket:%d",firewall_sock);
	select_sever_add(firewall_sock);
}


/* 
 * ===  FUNCTION  ==============================================
 *         Name:  interception_run
 *  Description:  main procedure for running interception
 * =============================================================
 */
void interception_run(){
	select_server_run();
}


/* 
 * ===  FUNCTION  ==============================================
 *         Name:  interception_over
 *  Description:  interception over
 * =============================================================
 */
void interception_over(){
	if(firewall_sock!=-1){
		close(firewall_sock);
		firewall_sock=-1;
		logInfo(LOG_NOTICE,"firewall sock is closed");
	}

	if(msg_listen_sock!=-1){
		close(msg_listen_sock);
		msg_listen_sock=-1;
		logInfo(LOG_NOTICE,"msg listen sock is closed");
	}
	router_destroy();
	delay_table_destroy();
}

