#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "interception.h"

static int excuted=0;
static void signal_handler(int sig)
{
	if(!excuted)
	{
		logInfo(LOG_INFO,"set signal handler:%d",sig);
		interception_over();
		endLogInfo();
		excuted=1;
	}

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
	atexit(interception_over);
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


