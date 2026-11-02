#ifndef _KMGR_H
#define _KMGR_H

#include "gl_types.h"
#include "cmnutil.h"

extern char env_kfs[];
extern char env_port[];

enum kmgr_clients {
	KMGR_CLIENT_MAIN   = 0,
	KMGR_CLIENT_SOCK   = 1,
	KMGR_CLIENT_OTHER  = 2,
	KMGR_CLIENT_LAST   = 3, // keep last
};

enum brcm_bin_file_types {
	BRCM_ALIGNMENT_BIN_FILE   = 0,
	BRCM_BW_250_BIN_FILE      = 1,
	BRCM_BW_500_BIN_FILE      = 2,
	BRCM_BW_750_BIN_FILE      = 3,
	BRCM_BW_1250_BIN_FILE     = 4,
	BRCM_BW_250_BER_BIN_FILE  = 5,
	BRCM_BW_500_BER_BIN_FILE  = 6,
	BRCM_BW_750_BER_BIN_FILE  = 7,
	BRCM_BW_1250_BER_BIN_FILE = 8,
};

#ifdef __cplusplus
extern "C" {
#endif

extern void load_brcm_bin_file(int binFileType);
extern int switch_k60s_to_relay_mode(void);
extern int lcmd_f_brcmcfg(int argc, char **argv, UINT32 body_length, char *body, kmgr_response_t *response, char *usage[3]);

#ifdef __cplusplus
}
#endif

extern kmgr_response_t *kmgr_issue_command(int client, char *buff, int length);

extern char env_sromfile0[];
extern char env_sromfile1[];

#endif /* _KMGR_H */
