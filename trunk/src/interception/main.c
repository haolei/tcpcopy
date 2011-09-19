#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "interception.h"

static void releaseResources()
{
	interception_over();
	endLogInfo();
}

static void signal_handler(int sig)
{
	printf("set signal handler:%d\n",sig);
	if(SIGSEGV==sig)
	{    
		signal(SIGSEGV, SIG_DFL);
		kill(getpid(), sig);
	}else
	{    
		exit(EXIT_SUCCESS);
	} 
}

static void set_signal_handler(){
	atexit(releaseResources);
	signal(SIGINT,signal_handler);
	signal(SIGPIPE,signal_handler);
	signal(SIGHUP,signal_handler);
	signal(SIGTERM,signal_handler);
}

int main(){
	initLogInfo();
	set_signal_handler();
	interception_init();
	interception_run();
	return 0;
}


