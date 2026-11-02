#ifndef _CMNUTIL_H
#define _CMNUTIL_H

#include "gl_types.h"

#define RSP_STATUS_MAX_SIZE     256
#define RSP_BODY_MAX_SIZE     16384 // we want large body, mainly for help and lua validator...
#define REQ_LINE_MAX_SIZE       256
#define REQ_BODY_MAX_SIZE     16384 // we want large body, mainly for lua validator...

typedef struct {
	UINT16 status_length;
	UINT16 body_length;
	char *status;
	char *body;
	char buff[RSP_STATUS_MAX_SIZE + 2 + RSP_BODY_MAX_SIZE + 2 + 1];
} kmgr_response_t;

typedef enum {
	DUMP_RESPONSE_BODY_STYLE_NONE,
	DUMP_RESPONSE_BODY_STYLE_BODY,
	DUMP_RESPONSE_BODY_STYLE_STRING,
	DUMP_RESPONSE_BODY_STYLE_HEX,
} dump_response_body_style_t;

extern int make_args(char *buf, UINT32 length, UINT32 max_argc, UINT32 *argc, char**argv);
extern void dump_response(char *prefix, dump_response_body_style_t style, kmgr_response_t *response);


#endif /* _CMNUTIL_H */
