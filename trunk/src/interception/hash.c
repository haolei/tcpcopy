#include "hash.h"
#include "linklist.h"
#include "../log/log.h"

static hash_node *hash_node_malloc(uint64_t key,void *data){
	hash_node * newnode = (hash_node *)malloc(sizeof(hash_node));
	if(! newnode){
		perror("cannot malloc memory!");
		exit(errno);
	}
	newnode->key = key;
	newnode->data = data;
	newnode->access_time = time(NULL);
	return newnode;
}
static inline size_t   get_slot(uint64_t key,size_t size){
	return key%size;
}

hash_table *hash_create(size_t size){
	hash_table * htable  = (hash_table *)malloc(sizeof(hash_table));
	if(! htable){
		perror("cannot malloc memory!");
		exit(errno);
	}
	htable->size = size;
	htable->lists = (struct linklist **) malloc(sizeof(linklist *) *size);
	if(! htable->lists){
		perror("cannot malloc memory!");
		exit(errno);
	}
	size_t i =0;
	for(i=0;i<size;i++){
		htable->lists[i] = linklist_create();
	}
	htable->timeout = DEFAULT_TIMEOUT;
	return htable;
}

static void delete_timeout(hash_table *table,linklist *l){
	time_t  nowtime = time(NULL);
	int count=0;
	int deepDeleteFlag=table->deepDeleteFlag;
	while(1){
		lnodeptr node = linklist_tail(l);
		if(! node ){
			break;
		}
		hash_node *hnode = (hash_node *)node->data;
		if(hnode->access_time+table->timeout < nowtime){
			lnodeptr tail=linklist_pop_tail(l);
			if(deepDeleteFlag&&NULL!=tail->data)
			{
				free(tail->data);
			}
			free(tail);
			count++;
		}else{
			break;
		}
	}
	logInfo(LOG_NOTICE,"table %s,delete timeout:%d",table->name,count);
}
static inline linklist * get_linklist(hash_table *table,uint64_t key){
	size_t slot = get_slot(key,table->size);
	linklist *l = table->lists[slot];
	return l;
}

static time_t lastUpdate;
static void checkDeleteTimeout(hash_table *table,linklist *l)
{
	if(0==lastUpdate)
	{
		lastUpdate=time(0);
	}
	time_t now=time(0);
	int diff=now-lastUpdate;
	if(diff>10)
	{
		delete_timeout(table,l);
		lastUpdate=now;
	}
}

static lnodeptr  hash_find_node(hash_table *table,uint64_t key){
	linklist *l = get_linklist(table,key);
	checkDeleteTimeout(table,l);
	lnodeptr node = linklist_first(l);
	while(node){
		hash_node *hnode = (hash_node *)node->data;
		if(hnode->key == key){
			hnode->access_time = time(NULL);
			linklist_remove(node);
			linklist_push(l,node);
			return node;
		}
		node = linklist_get_next(l,node);
	}
	return NULL;
}

void * hash_find(hash_table *table,uint64_t key){
	lnodeptr node = hash_find_node(table,key);
	if(node != NULL){
		hash_node *hnode = (hash_node *) node->data;
		return hnode->data;
	}
	return NULL;
}

void hash_add(hash_table *table,uint64_t key,void *data){
	lnodeptr node = hash_find_node(table,key);
	if(node != NULL){
		hash_node *hnode = (hash_node *) node->data;
		hnode->data = data;
		return;
	}
	hash_node *newnode = hash_node_malloc(key,data);
	lnodeptr  pnode = lnode_malloc(newnode);
	linklist *l = get_linklist(table,key);
	linklist_push(l,pnode);
	return;
}


void hash_del(hash_table *table,uint64_t key){
	lnodeptr node = hash_find_node(table,key);
	int deepDeleteFlag=table->deepDeleteFlag;
	if(node != NULL){
		linklist_remove(node);
		if(deepDeleteFlag&&node->data!=NULL)
		{
			free(node->data);
		}
		lnode_free(node);
	}
	return;
}

void hash_set_timeout(hash_table *table,int t){
	table->timeout = t;
}

void hash_destory(hash_table *table)
{
	uint32_t index=0;
	linklist* l=NULL;
	int deepDeleteFlag=table->deepDeleteFlag;
	for(;index<table->size;index++)
	{
		l=table->lists[index];
		if(l!=NULL)
		{
			linklist_destory(l,deepDeleteFlag);
		}
	}
	free(table->lists);
}

