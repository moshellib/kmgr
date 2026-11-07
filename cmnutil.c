// this file holds utility functions common to all application under the kmodemd umbrella

#include <stdio.h>
#include "gl_types.h"
#include "cmnutil.h"
#include "kdefs.h"

int make_args(char *buf, UINT32 length, UINT32 max_argc, UINT32 *argc, char**argv) {
	UINT32 i;
	UINT32 n_tokens = 0;
	UINT32 token = 0;

	if(buf == NULL || argc == NULL || argv == NULL || length == (UINT32)-1) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "make_args: invalid buffers or length=%u", length);
		return -1;
	}
	*argc = 0;
	length += 1; // we need the terminating 0 to parse args
	for(i=0; i<length; i++) {
	    if((buf[i] != ' ') && (buf[i] != '\t')) break;
	}
	for(; i<length; i++) {
		switch(buf[i]) {
		case 0:
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			if(token == 1) { // end of a token
				buf[i] = 0;
				++n_tokens;
				token = 0;
			}
			break;
		default:
			if(token == 0) { // start of token
				if(n_tokens >= max_argc) {
					klog(KLOG_SUBJECT_KMGR, LL_WARNG, "make_args: too many arguments; maximum=%u", max_argc);
					return -1;
				}
				argv[n_tokens] = &buf[i];
				token = 1;
			}
			break;
		}
	}
	*argc = n_tokens;
	return 0;
}

void dump_response(char *prefix, dump_response_body_style_t style, kmgr_response_t *response) {
	if(response == NULL)
	    return;

	klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "%s%s (%d)\n", prefix, response->status, response->body_length);
	if(response->body_length) {
		switch(style) {
		case DUMP_RESPONSE_BODY_STYLE_BODY:
		    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "body...\n");
			break;
		case DUMP_RESPONSE_BODY_STYLE_STRING:
		    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "%s\n", response->body);
			break;
		case DUMP_RESPONSE_BODY_STYLE_HEX:
		    klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "2do - implement body hex dump\n");
			break;
		case DUMP_RESPONSE_BODY_STYLE_NONE:
		default:
			break;
		}
	}
}





