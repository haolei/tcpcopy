#include "../communication/msg.h"
#include "../log/log.h"
#include "hash.h"
#include "router.h"
#include "delay.h"

static hash_table *table;

static inline uint64_t get_key(uint32_t ip,uint16_t port){
	uint64_t value=((uint64_t)ip<<16);
	value+=port;
	return value;
}

void router_init(){
	//we support 256k slots here
	table = hash_create(1024*256);
	table->deepDeleteFlag=0;
	strcpy(table->name,"client--src table");
	logInfo(LOG_NOTICE,"create table %s,size:%u",
			table->name,table->size);
}

void router_del(uint32_t ip,uint16_t port){
	hash_del(table,get_key(ip,port));
}

void router_add(uint32_t ip,uint16_t port,int fd){
	uint64_t key=get_key(ip,port);
	hash_add(table,key,(void *)(long)fd);
	delay_table_send(key,fd);
}

void router_update(struct iphdr *ip_header){
	if(ip_header->protocol != IPPROTO_TCP){
		return;
	}
	uint32_t size_ip = ip_header->ihl<<2;
	struct tcphdr *tcp_header = (struct tcphdr*)((char *)ip_header+size_ip);
	uint64_t key=get_key(ip_header->daddr,tcp_header->dest);
	void *fd = hash_find(table,key);
	struct receiver_msg_st msg;
	memcpy((void *) &(msg.ip_header),ip_header,sizeof(struct iphdr));
	memcpy((void *) &(msg.tcp_header),tcp_header,sizeof(struct tcphdr));
	if( NULL == fd ){
		delay_table_add(key,&msg);
		return ;
	}
	msg_receiver_send((int)(long)fd,&msg);

}

void router_destroy()
{
	if(table!=NULL)
	{
		logInfo(LOG_NOTICE,"destroy router table");
		hash_destory(table);
		free(table);
		table=NULL;
	}
}

