#ifndef _KDEFS_H
#define _KDEFS_H

#include "rfctl_cmn.h"
#include <errno.h>

extern char env_baud[];
extern char env_forcesock[];
extern char env_kfs[];
extern char env_port[];
extern char env_timeout[];
extern char env_tty[];

#define ENV_TRUE(var) (var[0] == '1')
#define ENV_FALSE(var) (var[0] != '1')

// additional sub-channels that do not pass over serial as is, but are used by kmgr

#define H2K_SUBC_RAW    N_H2K_SUBC
#define K2H_SUBC_RAW    N_K2H_SUBC
#define K2H_SUBC_EVNT   (N_K2H_SUBC + 1)

#define KIO_BUFF_SIZE 4096

#if FIX_ENDIAN
#define FE16(v) ((((v) >> 8) & 0x00FF) | (((v) << 8) & 0xFF00))
#define FE32(v) ((((v) >> 24) & 0x000000FF) | (((v) >> 8) & 0x0000FF00) | (((v) << 8) & 0x00FF0000)  | (((v) << 24) & 0xFF000000))
#else
#define FE16(v) (v)
#define FE32(v) (v)
#endif

enum kio_flags {
	KIO_FLAG_RAW = 1,
	KIO_FLAG_NO_READ = 2,
	KIO_FLAG_FLUSH_RX = 4,
};

enum {
	E_TX_STATE_DOWN = 0,
    E_TX_STATE_UP   = 1,
    E_CTRLCH_CHECK  = 2,
};

// log

enum klog_subjects {
	KLOG_SUBJECT_KMGR = 0,
	KLOG_SUBJECT_SHRT = 1,
	KLOG_SUBJECT_BCKG = 2,
	KLOG_SUBJECT_MGMT = 3,
	KLOG_SUBJECT_VSHR = 4,
	KLOG_SUBJECT_LAST = 5,
};

enum klog_types {
	KLOG_TYPE_STORE   = 0,
	KLOG_TYPE_DISPLAY = 1,
};

enum {
	LL_EMERG = 0,
	LL_ALERT = 1,
	LL_CRTCL = 2,
	LL_ERROR = 3,
	LL_WARNG = 4,
	LL_NTICE = 5,
	LL_INFOR = 6,
	LL_DEBUG = 7,
};


#ifdef __cplusplus
extern "C" {
#endif

extern void kmgr_init(char *env_fname);
extern void *kmgr_thread_main(void* arg);
extern void *kio_thread_main(void *arg);

extern UINT32 kio_control(UINT32 flag, UINT32 value);
extern UINT32 kio_read(UINT32 subc, void *buf, UINT32 length);
extern UINT32 kio_write(UINT32 subc, void *buf, UINT32 length, UINT32 commit, UINT32 pdu_flags);
extern UINT32 kio_commit(UINT32 subc);

// sock
extern void *server_main(void *arg);


extern void klog(int subject, int level, char *fmt, ...);

static inline int klog_return_error(int result, const char *function, int line)
{
	int saved_errno = errno;
	klog(KLOG_SUBJECT_KMGR, LL_ERROR, "%s: returning error rc=%d source_line=%d", function, result, line);
	errno = saved_errno;
	return result;
}

#define KLOG_RETURN_ERROR(result) klog_return_error((result), __func__, __LINE__)

extern char *klog_subj_to_name(int subj);
extern int klog_name_to_subj(char *name);
extern char *klog_level_to_name(int subj);
extern int klog_name_to_level(char *level);
extern int klog_get_filter_level(int subj, int type);
extern void klog_set_filter_level(int subj, int type, int level);
extern void klog_event_indexes(UINT32 *e_first, UINT32 *e_last, UINT32 *e_wrap);
extern UINT32 make_ipf_by_files(char *fname0, char *fname1, char *ipf, UINT32 max_size);

extern UINT32 make_ipf_by_buff(char *in, int inlen, char *ipf, UINT32 max_size);
extern int loader_entry(char *dev_name, char *speed, char *file, int force_burn);

typedef void (*log_cb_t)(char *subject_name, int level, char *level_name, char *content);
extern void klog_set_cb(log_cb_t host_log_cb);

typedef void (*modem_ready_cb_t)(void);
extern void modem_set_cb(modem_ready_cb_t modem_ready_cb);

typedef UINT32 (*inv70_tx_freq_cb_t)(void);
extern void inv70_tx_freq_cb(inv70_tx_freq_cb_t inv70_tx_freq_cb);

typedef void (*reset_k60s_cb_t)(void);
extern void reset_k60s_set_cb(reset_k60s_cb_t reset_k60s_cb);

typedef void (*rf_operational_cb_t)(unsigned char rf_state);
extern void rf_operational_set_cb(rf_operational_cb_t rf_operational_cb);

#ifdef __cplusplus
}
#endif


#endif /* _KDEFS_H */
