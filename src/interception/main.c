#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <signal.h>

#include "interception.h"

static void signal_handler(int sig)
{
	logInfo(LOG_INFO,"set signal handler:%d",sig);
	interception_over();
}

static void set_signal_handler(){
	atexit(interception_over);
	signal(SIGINT,signal_handler);
	signal(SIGPIPE,signal_handler);
	signal(SIGHUP,signal_handler);
	signal(SIGTERM,signal_handler);
}

int main(){
	set_signal_handler();
	interception_init();
	interception_run();
	return 0;
}


