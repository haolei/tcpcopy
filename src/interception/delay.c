#include "hash.h"
#include "delay.h"
#include "../log/log.h"

static hash_table  *table;
static int mCount;
static int fCount;
static struct receiver_msg_st * copy_message(struct receiver_msg_st *msg){
	struct receiver_msg_st *cmsg = NULL;
	cmsg=(struct receiver_msg_st *)malloc(sizeof(struct receiver_msg_st));
	if(NULL == cmsg){
		perror("malloc");
		exit(1);
	}
	memcpy(cmsg,msg,sizeof(struct receiver_msg_st));
	return cmsg;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delay_table_init
 *  Description:  init delay table
 * =====================================================================================
 */
void delay_table_init(){
	//we support 64k slots here
	table = hash_create(1024*64);
	hash_set_timeout(table,30);
	strcpy(table->name,"delay-table");
	table->deepDeleteFlag=1;
	logInfo(LOG_NOTICE,"create table %s,size:%u",table->name,table->size);
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delay_table_add
 *  Description:  add msg to delay table
 * =====================================================================================
 */
void delay_table_add(uint64_t key,struct receiver_msg_st *msg){
	linklist *msg_list =(linklist *)hash_find(table,key);
	struct receiver_msg_st *cmsg = copy_message(msg);
	lnodeptr pnode = lnode_malloc((void *)cmsg);
	if(NULL == msg_list){
		msg_list = linklist_create();
		hash_add(table,key,msg_list);
	}
	mCount++;
	linklist_append(msg_list,pnode);
	return;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delay_table_send
 *  Description:  send delayed message according key
 * =====================================================================================
 */
void delay_table_send(uint64_t key,int fd){
	linklist *msg_list =(linklist *)hash_find(table,key);
	if(NULL == msg_list){
		return;	
	}
	while(! linklist_is_empty(msg_list)){
		lnodeptr first = linklist_pop_first(msg_list);
		struct receiver_msg_st *msg = (first->data);
		msg_receiver_send(fd,msg);
		if(msg!=NULL)
		{
			free(msg);
		}
		fCount++;
		lnode_free(first);
	}
}

void delay_table_del(uint64_t key){
	linklist *msg_list =(linklist *)hash_find(table,key);
	if(NULL == msg_list){
		return;	
	}
	while(! linklist_is_empty(msg_list)){
		lnodeptr first = linklist_pop_first(msg_list);
		struct receiver_msg_st *msg = (first->data);
		if(msg!=NULL)
		{
			free(msg);
		}
		fCount++;
		lnode_free(first);
	}
	hash_del(table,key);
	free(msg_list);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delay_table_destroy
 *  Description:  destroy delay table
 * =====================================================================================
 */
void delay_table_destroy()
{
	if(table!=NULL)
	{
		logInfo(LOG_NOTICE,"destroy delayed table");
		uint32_t i=0;
		int count=0;
		for(;i<table->size;i++)
		{
			linklist* list=table->lists[i];
			lnodeptr node = linklist_first(list);
			while(node){
				hash_node *hnode = (hash_node *)node->data;
				if(hnode->data!=NULL)
				{
					linklist *msg_list=(linklist *)hnode->data;
					count+=linklist_destory(msg_list);
				}	
				hnode->data=NULL;
				node = linklist_get_next(list,node);
			}
		}

		logInfo(LOG_NOTICE,"destroy msg list items:%d,free:%d,total:%d",
				count,fCount,mCount);
		hash_destory(table);
		free(table);
		table=NULL;
	}
}

