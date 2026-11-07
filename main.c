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

static int start_thread(pthread_t *thread, void *(*entry)(void *), const char *name)
{
	pthread_attr_t attributes;
	int result = pthread_attr_init(&attributes);
	if(result != 0) {
		klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "Failed to initialize %s thread attributes: %s", name, strerror(result));
		return -1;
	}
	result = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	if(result == 0) result = pthread_attr_setinheritsched(&attributes, PTHREAD_EXPLICIT_SCHED);
	if(result == 0) result = pthread_create(thread, &attributes, entry, NULL);
	pthread_attr_destroy(&attributes);
	if(result != 0) {
		klog(KLOG_SUBJECT_KMGR, LL_CRTCL, "Failed to start %s thread: %s", name, strerror(result));
		return -1;
	}
	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "Started %s thread", name);
	return 0;
}

int main(int argc,char** argv) {

	if((argc > 2) || ((argc == 2) && (argv[1][0] == '-'))) {
		printf("Please start with %s [envfile]\n", argv[0]);
		if(argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) return EXIT_SUCCESS;
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Invalid startup arguments: argc=%d", argc);
		return EXIT_FAILURE;
	}
	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "starting...");
	kmgr_init(argc > 1 ? argv[1] : NULL);

    signal(SIGPIPE, SIG_IGN); // we do not want to crash on broken pipe... this is needed for server writing to a client that is not reading

	if(start_thread(&kio_thread, kio_thread_main, "serial I/O") < 0 ||
	   start_thread(&kmgr_thread, kmgr_thread_main, "manager") < 0 ||
	   start_thread(&server_thread, server_main, "server") < 0)
		return EXIT_FAILURE;

	// we keep main context free for modemd stuff
	while(1) {
		sleep(1);
	}

	return 0;

}




