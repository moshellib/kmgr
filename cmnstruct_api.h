#ifndef _CMNSTRUCT_API_H
#define _CMNSTRUCT_API_H

#include "gl_types.h"
#include "cmnstruct.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int logLevel_set(int client_id, cmnstrct_log_level_in_t *cmnstrct_log_level_in);
extern int softCfg_set(int client_id, cmnstrct_rf_soft_config_in_t *cmnstrct_rf_soft_config_in);
extern int hardCfg_set(int client_id, cmnstrct_rf_hard_config_in_t *cmnstrct_rf_hard_config_in);
extern int modulationTbl_set(int client_id, cmnstrct_modulation_tbl_in_t *cmnstrct_modulation_tbl_in);
extern int rf_loopback_set(int client_id, cmnstrct_loopback_set_in_t *cmnstrct_loopback_set_in);
extern int rf_state_set(int client_id, cmnstrct_rf_state_set_in_t *cmnstrct_rf_state_set_in);
extern int rf_reset_stats(int client_id, cmnstrct_reset_stats_in_t *cmnstrct_rf_reset_stats_in);
extern int rf_reset_ber_stats(int client_id, cmnstrct_reset_ber_stats_in_t *cmnstrct_rf_reset_ber_stats_in);
extern int rf_debug_set(int client_id, cmnstrct_rf_debug_set_in_t *cmnstrct_rf_debug_set_in);
extern int rf_reset(int client_id, cmnstrct_rf_reset_in_t *cmnstrct_rf_reset_in);
extern int system_info_set(int client_id, cmnstrct_system_info_set_in_t *cmnstrct_system_info_set_in);

extern int rf_system_info_get(int client_id, cmnstrct_system_info_get_out_t *system_info_out);
extern int rf_debug_stat_get(int client_id, cmnstrct_rf_debug_stat_get_out_t *rfDebug_stat_out);
extern int rf_stat_get(int client_id, cmnstrct_rf_stat_get_out_t *rf_stat_out);

#ifdef __cplusplus
}
#endif

#endif /* _CMNSTRUCT_API_H */
