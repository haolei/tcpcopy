#include "hash.h"
#include "delay.h"
#include "../log/log.h"

static hash_table  *table;
static int mCount;
static int lCount;
static int fCount;
static int delayDel;
static int lDestroy;
static int count;

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
	logInfo(LOG_NOTICE,"create table %s,size:%u",table->name,table->size);
	mCount=0;
	fCount=0;
	lCount=0;
	delayDel=0;
	lDestroy=0;
	count=0;
}

static void delay_table_delete_obsolete(uint64_t key)
{
	linklist *l = get_linklist(table,key);
	time_t  nowtime = time(NULL);
	while(1){
		lnodeptr node = linklist_tail(l);
		if(! node ){
			break;
		}   
		hash_node *hnode = (hash_node *)node->data;
		if(hnode->access_time+table->timeout < nowtime){
			lnodeptr tail=linklist_pop_tail(l);
			hash_node *hnode = (hash_node *)tail->data;
			if(NULL!=hnode)
			{   
				if(hnode->data!=NULL)
				{
					linklist *msg_list=(linklist *)hnode->data;
					count+=linklist_destory(msg_list);
					lDestroy++;
				}
				hnode->data=NULL;
			}   
			tail->data=NULL;
			free(tail);
		}else{
			break;
		}   
	} 
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delay_table_add
 *  Description:  add msg to delay table
 * =====================================================================================
 */
void delay_table_add(uint64_t key,struct receiver_msg_st *msg){
	delay_table_delete_obsolete(key);	
	linklist *msg_list =(linklist *)hash_find(table,key);
	struct receiver_msg_st *cmsg = copy_message(msg);
	lnodeptr pnode = lnode_malloc((void *)cmsg);
	if(NULL == msg_list){
		lCount++;
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
	delay_table_delete_obsolete(key);	
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
	delay_table_delete_obsolete(key);	
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
		delayDel++;
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
					lDestroy++;
				}	
				hnode->data=NULL;
				node = linklist_get_next(list,node);
			}
		}

		logInfo(LOG_NOTICE,"destroy msg list items:%d,free:%d,total:%d",
				count,fCount,mCount);
		logInfo(LOG_NOTICE,"create linklist:%d,delay del:%d,destroy list:%d",
				lCount,delayDel,lDestroy);
		hash_destory(table);
		free(table);
		table=NULL;
	}
}

