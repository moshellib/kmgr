#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
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

	if(fd == NULL || port < 0 || port > 65535) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Invalid listener arguments: port=%d output=%s", port, fd ? "provided" : "NULL");
		return -1;
	}
	*fd = -1;
    sd = socket(PF_INET, SOCK_STREAM, 0);
	if(sd < 0) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Failed to create listener socket: port=%d error=%s", port, strerror(errno));
		return -1;
	}
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Failed to bind listener: port=%d error=%s", port, strerror(errno));
		close(sd);
        return -1;
    }
    if(listen(sd, 10) != 0 )
    {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "Failed to listen: port=%d error=%s", port, strerror(errno));
		close(sd);
        return -2;
    }
	*fd = sd;
    return 0;
}

int socket_clean_me = -1;

void *servlet_entry(void *arg) {
	char buf[REQ_LINE_MAX_SIZE + 2 + REQ_BODY_MAX_SIZE + 2 + 1];
	char rsp_stline[RSP_STATUS_MAX_SIZE + 16];
	int bytes, tmp, rv;
	UINT32 body_length;
	servlet_arg_t servlet_arg;
	kmgr_response_t *response;

	memcpy(&servlet_arg, arg, sizeof(servlet_arg));
	free(arg);
	socket_clean_me = servlet_arg.client;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	while(1) {
		char received;
		body_length = bytes = 0;
		while((rv = read(servlet_arg.client, &received, 1)) == 1) {
			if(bytes >= REQ_LINE_MAX_SIZE + 2) {
				klog(KLOG_SUBJECT_KMGR, LL_WARNG, "servlet_entry: request line exceeds limit=%u", (unsigned int)REQ_LINE_MAX_SIZE + 2);
				goto servlet_done;
			}
			buf[bytes++] = received;
			if(received == '\n') break;
		}
		buf[bytes] = 0;
		if(rv != 1) {
			klog(KLOG_SUBJECT_KMGR, rv == 0 ? LL_DEBUG : LL_ERROR, "servlet_entry: client read ended; fd=%d rc=%d reason=%s", servlet_arg.client, rv, rv == 0 ? "peer disconnected" : strerror(errno));
			break;
		}
		if(bytes > 5 && memcmp(&buf[bytes - 5], "+++\r\n", 5) == 0) {
			UINT32 back;
			char *body_end, *body_start;
			unsigned long requested_body;
			buf[bytes - 5] = '\r';
			buf[bytes - 4] = '\n';
			bytes -= 3;
			for(back = 3; (int)back < bytes && buf[bytes - back] != ' '; back++);
			body_start = buf + bytes - back + 1;
			errno = 0;
			requested_body = strtoul(body_start, &body_end, 10);
			if(body_start[0] == '-' || body_end == body_start || body_end != buf + bytes - 2 || errno != 0 || requested_body > REQ_BODY_MAX_SIZE) {
				klog(KLOG_SUBJECT_KMGR, LL_WARNG, "servlet_entry: invalid request body length; maximum=%u", (unsigned int)REQ_BODY_MAX_SIZE);
				goto servlet_done;
			}
			body_length = requested_body;
			tmp = 0;
			while((rv = read(servlet_arg.client, &received, 1)) == 1) {
				buf[bytes + tmp++] = received;
				if((UINT32)tmp == body_length + 2) break;
			}
			if(rv != 1) {
				klog(KLOG_SUBJECT_KMGR, LL_ERROR, "servlet_entry: incomplete request body; received=%d expected=%u reason=%s", tmp, body_length + 2, rv == 0 ? "peer disconnected" : strerror(errno));
				break;
			}
			bytes += tmp;
		}
		buf[bytes] = 0;
		response = kmgr_issue_command(KMGR_CLIENT_SOCK, buf, bytes);
		if(response == NULL || response->status == NULL || (response->body_length && response->body == NULL)) {
			klog(KLOG_SUBJECT_KMGR, LL_ERROR, "servlet_entry: invalid command response");
			break;
		}
		tmp = snprintf(rsp_stline, sizeof(rsp_stline), "%s %d\r\n", response->status, response->body_length);
		if(tmp < 0 || (unsigned int)tmp >= sizeof(rsp_stline)) {
			klog(KLOG_SUBJECT_KMGR, LL_ERROR, "servlet_entry: response status exceeds buffer capacity");
			break;
		}
		rv = write(servlet_arg.client, rsp_stline, tmp);
		if(rv != tmp) {
			klog(KLOG_SUBJECT_KMGR, LL_ERROR, "servlet_entry: incomplete status write; written=%d expected=%d reason=%s", rv, tmp, rv < 0 ? strerror(errno) : "short write");
			break;
		}
		if(response->body_length) {
			rv = write(servlet_arg.client, response->body, response->body_length);
			if(rv != response->body_length) {
				klog(KLOG_SUBJECT_KMGR, LL_ERROR, "servlet_entry: incomplete body write; written=%d expected=%u reason=%s", rv, response->body_length, rv < 0 ? strerror(errno) : "short write");
				break;
			}
			if(write(servlet_arg.client, "\r\n", 2) != 2) {
				klog(KLOG_SUBJECT_KMGR, LL_ERROR, "servlet_entry: failed to write response trailer");
				break;
			}
		}
	}
servlet_done:
	servlet_valid = 0;
	close(servlet_arg.client);
	if(socket_clean_me == servlet_arg.client) socket_clean_me = -1;
	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "servlet_entry: connection closed; fd=%d", servlet_arg.client);
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
		servlet_arg_t *servlet_arg;
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
					servlet_arg = malloc(sizeof(*servlet_arg));
					if(servlet_arg == NULL) {
						klog(KLOG_SUBJECT_KMGR, LL_ERROR, "server_main: failed to allocate servlet argument");
						close(client);
						continue;
					}
					servlet_arg->client = client;
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
					int create_result = pthread_create(&servlet, &servlet_thread_attributes, servlet_entry, servlet_arg);
					pthread_attr_destroy(&servlet_thread_attributes);
					if(create_result != 0) {
						klog(KLOG_SUBJECT_KMGR, LL_ERROR, "server_main: failed to create servlet: %s", strerror(create_result));
						free(servlet_arg);
						close(client);
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
