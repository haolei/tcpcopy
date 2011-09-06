/*
 * =====================================================================================
 *
 *       Filename:  hash.h
 *       Compiler:  gcc
 *
 *         Author:  wangbo@corp.netease.com
 *
 *      CopyRight:  Copyright (c) netease
 *
 *    Description: 
 *
 *        Created:  2010-07-12 19:04:23
 * =====================================================================================
 */

#ifndef  _TCPCOPY_HASH_H__INC
#define  _TCPCOPY_HASH_H__INC

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/file.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "linklist.h"

#define DEFAULT_TIMEOUT   1200

typedef struct hash_node_st{
	uint64_t key;
	time_t   access_time;
	void     *data;
}hash_node;

typedef struct hash_table_st{
	uint32_t size;
	int      timeout;
	struct linklist **lists;
}hash_table;

hash_table * hash_create(size_t size);
void hash_set_timeout(hash_table *,int);
void hash_destory(hash_table *);
void hash_add(hash_table *,uint64_t ,void *);
void *hash_find(hash_table *,uint64_t);
void hash_del(hash_table *,uint64_t);

#ifdef __cplusplus
}
#endif

#endif   /* ----- #ifndef _TCPCOPY_CLIENT_HASH_H__INC  ----- */




