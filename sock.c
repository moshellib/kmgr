#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>

#include <arpa/inet.h>
#include <pthread.h>

#include "gl_types.h"
#include "kdefs.h"
#include "kmgr.h"
#include "cmnstruct.h"
#include "cmnstruct_api.h"

typedef struct {
	int client;
} servlet_arg_t;

pthread_t servlet;
int servlet_valid = 0;

int open_listener(int port, int *fd)
{   int sd;
    struct sockaddr_in addr;

    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
    	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "can't bind port");
        return -1;
    }
    if(listen(sd, 10) != 0 )
    {
    	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "Can't configure listening port");
        return -2;
    }
    if(fd) *fd = sd;
    return 0;
}

int socket_clean_me = -1;

void *servlet_entry(void *arg) {
    char buf[REQ_LINE_MAX_SIZE + 2 + REQ_BODY_MAX_SIZE + 2 + 1], rsp_stline[RSP_STATUS_MAX_SIZE + 16];
    int bytes, tmp;
    UINT32 body_length;
    servlet_arg_t servlet_arg;
    kmgr_response_t *response;

	pthread_t me = pthread_self();
	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "servlet: me is 0x%llX", (unsigned long long)me);
    pthread_detach(me);

	memcpy(&servlet_arg, arg, sizeof(servlet_arg));
	socket_clean_me = servlet_arg.client;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	while(1) {
    	char c;
    	int rv;
        body_length = bytes = 0;
    	while((rv = read(servlet_arg.client, &c, 1)) == 1) {
    		buf[bytes++] = c;
    		if(c == '\n') break;
    	}
		buf[bytes] = 0;
    	if(rv != 1) break; // some connection problem
		if((bytes > 5) && (memcmp(&buf[bytes - 5], "+++\r\n", 5) == 0)) {
			UINT32 back;
			buf[bytes - 5] = '\r';
			buf[bytes - 4] = '\n';
			bytes -= 3;
			// body is only of flavor " %d+++\r\n"
			for(back = 3; ((int)back < bytes) && (buf[bytes - back] != ' '); back ++); // find offset to last token;
			body_length = atoi(buf + bytes - back + 1);
			tmp = 0;
	    	while((rv = read(servlet_arg.client, &c, 1)) == 1) {
	    		buf[bytes + tmp++] = c;
	    		if((UINT32)tmp == body_length + 2) break;
	    	}
	    	if(rv != 1) break; // some connection problem
	    	bytes += tmp;
		}
		buf[bytes] = 0;

		response = kmgr_issue_command(KMGR_CLIENT_SOCK, buf, bytes);

		tmp = snprintf(rsp_stline, sizeof(rsp_stline), "%s %d\r\n", response->status, response->body_length);
		if(write(servlet_arg.client, rsp_stline, tmp) != tmp) {
			break;
		}
		if(response->body_length) {
			if(write(servlet_arg.client, response->body, response->body_length) != response->body_length) {
				break;
			}
			if(write(servlet_arg.client, "\r\n", 2) != 2) {
				break;
			}
		}
	}
	servlet_valid = 0;
	if(socket_clean_me != -1) {
		close(socket_clean_me);
		socket_clean_me = -1;
	}
    pthread_exit(0);
    return NULL;
}

void *server_main(void *arg)
{
    int listener;
    char fcond[128];
    (void)arg;

#if 0
	pthread_t me = pthread_self();
	log(0, 0, "server: me is 0x%llX\n", (unsigned long long)me);
    pthread_detach(me);
#endif

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    listener = -1;

    while (1) {
    	struct sockaddr_in addr;
        int len = sizeof(addr);
        servlet_arg_t servlet_arg;
        int client;

        snprintf(fcond, sizeof(fcond), "%s/sock", env_kfs);
        if(ENV_TRUE(env_forcesock) || (access(fcond, F_OK) == 0)) {
        	if(listener < 0) {
        		UINT16 port = atoi(env_port);
      	    	if(open_listener(port, &listener) == 0) {		/* create server socket */
      	    		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "server is up");
      	    		int flags = fcntl(listener, F_GETFL, 0);
      	    		fcntl(listener, F_SETFL, flags | O_NONBLOCK);
      	    	}
        	}
        	if(listener >= 0) {
            	client = accept(listener, (struct sockaddr*)&addr, (socklen_t *)&len);		/* accept connection as usual */
        		if(client >= 0) {
        	        klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "server accepted connection: %s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        	        servlet_arg.client = client;
        	        if(servlet_valid) {
        	        	if(pthread_cancel(servlet) != 0) {
        	        		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "server: failed to cancel existing servlet");
        	        	} else {
        	        		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "server: canceled existing servlet");
        	        	}
        	        	servlet_valid = 0;
        	        }
        	    	pthread_attr_t servlet_thread_attributes;
        	    	pthread_attr_init(&servlet_thread_attributes);
        	    	pthread_attr_setdetachstate(&servlet_thread_attributes, PTHREAD_CREATE_DETACHED);
        	    	pthread_attr_setinheritsched(&servlet_thread_attributes, PTHREAD_EXPLICIT_SCHED);
        	        if(pthread_create(&servlet, &servlet_thread_attributes, &servlet_entry, (void *)&servlet_arg) != 0) {
        	        	close(client); // failed to create servelet, we close client socket
        	        } else {
            	        servlet_valid = 1;
        	        }
        		}
        	}
        } else {
        	// server is off, we need to make sure we are not listening and that servlet is not running
        	if(listener >= 0) {
        		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "closing listener");
        		close(listener);
        		listener = -1;
        	}
        	if(servlet_valid) {
            	if(pthread_cancel(servlet) != 0) {
            		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "server: failed to cancel existing servlet");
            	} else {
            		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "server: canceled existing servlet");
            	}
            	servlet_valid = 0;
            	if(socket_clean_me != -1) {
            		close(socket_clean_me);
            		socket_clean_me = -1;
            	}
        	}
        }
        sleep(1);
    }
    pthread_exit(0);
    return NULL;
}
