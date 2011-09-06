/*
 * =====================================================================================
 *
 *       Filename:  delay.c
 *       Compiler:  gcc
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description:  
 *
 *        Created:  2010-07-25 16:21:25
 * =====================================================================================
 */

#include "hash.h"
#include "delay.h"

static hash_table  *table;

void delay_table_init(){
	table = hash_create(65536);
	hash_set_timeout(table,10);
}

static struct receiver_msg_st * copy_message(struct receiver_msg_st *msg){
	struct receiver_msg_st *cmsg = (struct receiver_msg_st *)malloc(sizeof(struct receiver_msg_st));
	if(cmsg == NULL){
		perror("malloc");
		exit(1);
	}
	memcpy(cmsg,msg,sizeof(struct receiver_msg_st));
	return cmsg;
}

void delay_table_add(uint64_t key,struct receiver_msg_st *msg){
	linklist *msg_list =(linklist *)hash_find(table,key);
	struct receiver_msg_st *cmsg = copy_message(msg);
	lnodeptr pnode = lnode_malloc((void *)cmsg);
	if(msg_list == NULL){
		msg_list = linklist_create();
		hash_add(table,key,msg_list);
	}
	linklist_append(msg_list,pnode);
	return;
}

void delay_table_send(uint64_t key,int fd){
	linklist *msg_list =(linklist *)hash_find(table,key);
	if(msg_list == NULL){
		return;	
	}
	while(! linklist_is_empty(msg_list)){
		lnodeptr first = linklist_pop_first(msg_list);
		struct receiver_msg_st *msg = (first->data);
		msg_receiver_send(fd,msg);
		free(msg);
		lnode_free(first);
	}
}


