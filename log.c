// logging, all logs generate k entities (either in k60 or kmgr) use this to generate logs

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "gl_types.h"
#include "kdefs.h"

#define USE_SIKLU_LOG 0
#if USE_SIKLU_LOG
#include "sk_log.h"
#else
#define LOG(...)
#endif

log_cb_t log_cb = NULL;

struct {
	char *name;
	UINT8 index;
	UINT8 active;
	UINT8 display_level;
	UINT8 store_level;
} subject_info[KLOG_SUBJECT_LAST] = {
	{ "kmgr", KLOG_SUBJECT_KMGR, 1, LL_DEBUG, LL_DEBUG },
	{ "shrt", KLOG_SUBJECT_SHRT, 1, LL_DEBUG, LL_DEBUG },
	{ "bckg", KLOG_SUBJECT_BCKG, 1, LL_DEBUG, LL_DEBUG },
	{ "mgmt", KLOG_SUBJECT_MGMT, 1, LL_DEBUG, LL_DEBUG },
	{ "vshr", KLOG_SUBJECT_VSHR, 1, LL_DEBUG, LL_DEBUG },
};

struct {
	char *name;
	UINT8 index;
} level_info[] = {
	{ "emerg", LL_EMERG },
	{ "alert", LL_ALERT },
	{ "crtcl", LL_CRTCL },
	{ "error", LL_ERROR },
	{ "warng", LL_WARNG },
	{ "ntice", LL_NTICE },
	{ "infor", LL_INFOR },
	{ "debug", LL_DEBUG },
};

char *klog_subj_to_name(int subj) {
	if((subj < 0) || (subj >= (int)(sizeof(subject_info)/sizeof(subject_info[0])))) {
		return NULL;
	}
	return subject_info[subj].name;
}


int klog_name_to_subj(char *name) {
	int i;
	for(i=0; i<(int)(sizeof(subject_info)/sizeof(subject_info[0])); i++) {
		if(strcmp(subject_info[i].name, name) == 0) return i;
	}
	return -1;
}


char *klog_level_to_name(int level) {
	int i;
	for(i=0; i<(int)(sizeof(level_info)/sizeof(level_info[0])); i++) {
		if(level == level_info[i].index) return level_info[i].name;
	}
	return NULL;
}


int klog_name_to_level(char *name) {
	int i;
	for(i=0; i<(int)(sizeof(level_info)/sizeof(level_info[0])); i++) {
		if(strcmp(level_info[i].name, name) == 0) return i;
	}
	return -1;
}

int klog_get_filter_level(int subj, int type) {
	if((subj >= 0) && (subj < (int)(sizeof(subject_info)/sizeof(subject_info[0])))) {
		return type == KLOG_TYPE_STORE ? subject_info[subj].store_level : subject_info[subj].display_level;
	}
	return -1;
}

void klog_set_filter_level(int subj, int type, int level) {
	if((subj >= 0) && (subj < (int)(sizeof(subject_info)/sizeof(subject_info[0]))) && (level >= 0) && (level<(int)(sizeof(level_info)/sizeof(level_info[0])))) {
		if(type == KLOG_TYPE_STORE) {
			subject_info[subj].store_level = level;
		} else {
			subject_info[subj].display_level = level;
		}
	}
}

void klog(int subject, int level, char *fmt, ...) {
	char buf[256];
	va_list args;
	va_start(args, fmt);
	if(subject_info[subject].active) {
		vsnprintf(buf, sizeof(buf), fmt, args);
		char *level_name = klog_level_to_name(level);
		if(level_name == NULL) level_name = "?????";
		if(level <= subject_info[subject].display_level) {
			if(log_cb) {
				log_cb(subject_info[subject].name, level, level_name, buf);
			} else {
				printf("%.4s %.5s %s\n", subject_info[subject].name, level_name, buf);
			}
		}
		if(level <= subject_info[subject].store_level) {
			char buf2[sizeof(buf) + 16];
			snprintf(buf2, sizeof(buf2), "%s: %s", subject_info[subject].name, buf)
			LOG(level, buf2);
		}
	}
	va_end(args);
}

void klog_set_cb(log_cb_t host_log_cb) {
	log_cb = host_log_cb;
}
