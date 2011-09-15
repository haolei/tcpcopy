#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include "select_server.h"

static int              	max_fd;
static fd_set           	read_set;
static select_server_func 	callback_func;
static int              	valid_fds[1024];
static int              	fd_nums;

void select_sever_set_callback(select_server_func func){
	callback_func = func;
}

void select_sever_add(int fd){
	FD_SET(fd,&read_set);
	if(fd > max_fd){
		max_fd = fd;
	}
	valid_fds[fd_nums] = fd;
	fd_nums++;
}

void select_sever_del(int fd){
	FD_CLR(fd,&read_set);
	max_fd = 0;
	int i = 0;
	for(i=0;i<fd_nums;i++){
		if(valid_fds[i] == fd){
			int j=i;
			while(j<fd_nums-1){
				valid_fds[j] = valid_fds[j+1];
				if(valid_fds[j] > max_fd){
					max_fd = valid_fds[j];
				}
			}
			fd_nums--;
			break;
		}
		if(valid_fds[i] > max_fd){
			max_fd = valid_fds[i];
		}
	}
}

void select_server_run(){
	while(1){
		fd_set r_set = read_set;
		int ret = select(max_fd+1,&r_set,NULL,NULL,NULL);
		if(ret == -1){
			continue;
		}else if(ret == 0){
			continue;
		}else{
			int i = 0;
			for(i=0;i<fd_nums;i++){
				if(FD_ISSET(valid_fds[i],&r_set)){
					callback_func(valid_fds[i]);
				}
			}
		}
	}
}

