#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "interception.h"

static void signal_handler(int sig)
{
	printf("%d\n",sig);
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


