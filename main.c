#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "gl_types.h"
#include "kdefs.h"
#include "kmgr.h"

pthread_t kio_thread;
pthread_t kmgr_thread;
pthread_t server_thread;

int main(int argc,char** argv) {

	if((argc > 2) || ((argc == 2) && (argv[1][0] == '-'))) {
		printf("Please start with %s [envfile]\n", argv[0]);
		exit(0);
	}
	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "starting...");
	kmgr_init(argc > 1 ? argv[1] : NULL);

    signal(SIGPIPE, SIG_IGN); // we do not want to crash on broken pipe... this is needed for server writing to a client that is not reading

	pthread_attr_t io_thread_attributes;
	pthread_attr_init(&io_thread_attributes);
	pthread_attr_setdetachstate(&io_thread_attributes, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&io_thread_attributes, PTHREAD_EXPLICIT_SCHED);
	pthread_create(&kio_thread, &io_thread_attributes, &kio_thread_main, NULL);

	pthread_attr_t mgr_thread_attributes;
	pthread_attr_init(&mgr_thread_attributes);
	pthread_attr_setdetachstate(&mgr_thread_attributes, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&mgr_thread_attributes, PTHREAD_EXPLICIT_SCHED);
	pthread_create(&kmgr_thread, &mgr_thread_attributes, &kmgr_thread_main, NULL);

	pthread_attr_t server_thread_attributes;
	pthread_attr_init(&server_thread_attributes);
	pthread_attr_setdetachstate(&server_thread_attributes, PTHREAD_CREATE_DETACHED);
	pthread_attr_setinheritsched(&server_thread_attributes, PTHREAD_EXPLICIT_SCHED);
	pthread_create(&server_thread, &server_thread_attributes, &server_main, NULL);

	// we keep main context free for modemd stuff
	while(1) {
		sleep(1);
	}

	return 0;

}




