#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gl_types.h"
#include "kdefs.h"
#include "kmgr.h"
#include "cmnstruct.h"
#include "cmnstruct_api.h"
#include "brcm/bcm85100_api.h"

#define OFS(a, b)  ((UINT16)(UINT64)&(((a*)NULL)->b))
#define SIZE(a, b) (sizeof(((a*)NULL)->b))
#define OFS_AND_SIZE(a, b) OFS(a, b), SIZE(a, b)
#define ELEMENTS(a, b) (sizeof(((a*)NULL)->b) / sizeof(((a*)NULL)->b[0]))

typedef struct {
	UINT32 ofs;
	UINT32 size;
	UINT32 n_element;
} struct_def_field_t;

typedef struct {
	UINT32 id;
	UINT32 size_in;
	UINT32 n_fileds_in;
	struct_def_field_t *fields_in;
	UINT32 size_out;
	UINT32 n_fileds_out;
	struct_def_field_t *fields_out;
} struct_def_t;


int action(int client_id, struct_def_t *def, void *in, void *out, UINT32 out_max_size) {
	char buf[REQ_LINE_MAX_SIZE + 2 + REQ_BODY_MAX_SIZE + 2 + 1];
	UINT32 i, j, k;
	UINT32 length;
	kmgr_response_t *response;

	length = snprintf(buf, sizeof(buf), "mgmts %d %d\r\n", def->id, def->size_in);

	if(length + def->size_in + 2 >= sizeof(buf)) {
		klog(KLOG_SUBJECT_KMGR, LL_DEBUG, "k60s mgmts body is too big...");
		return -1;
	}
	if(def->size_in) {
		memcpy(&buf[length], in, def->size_in);
#if FIX_ENDIAN
		for (i = 0 ; i < def->n_fileds_in ; i++) {
			UINT32 element_ofs = 0, element_size = def->fields_in[i].size / def->fields_in[i].n_element;
			for(j = 0; j < def->fields_in[i].n_element; j++) {
				UINT8 temp;
				for (k = 0 ; k < (element_size / 2) ; k++) {
					temp = buf[length + def->fields_in[i].ofs + element_ofs + k];
					buf[length + def->fields_in[i].ofs + element_ofs + k] = buf[length + def->fields_in[i].ofs + element_ofs - k -1 + element_size];
					buf[length + def->fields_in[i].ofs + element_ofs -k -1 + element_size] = temp;
				}
				element_ofs += element_size;
			}
		}
#endif
		length += def->size_in;
		length += snprintf(&buf[length], sizeof(buf) - length, "\r\n");
	}
	response = kmgr_issue_command(client_id, buf, length);
	if(strcmp(response->status, "OK") != 0) {
		klog(KLOG_SUBJECT_KMGR, LL_ERROR, "k60s mgmts command failed... id=%d", def->id);
		return -2;
	}
#if FIX_ENDIAN
	response->body_length = FE16(response->body_length);
#endif
	length = response->body_length <= out_max_size ? response->body_length : out_max_size;
	if(length) {
#if FIX_ENDIAN
		for (i = 0 ; i < def->n_fileds_out ; i++) {
			UINT32 element_ofs = 0, element_size = def->fields_out[i].size / def->fields_out[i].n_element;
			for(j = 0; j < def->fields_out[i].n_element; j++) {
				UINT8 temp;
				for (k = 0 ; k < (element_size / 2) ; k++) {
					temp = response->body[ def->fields_out[i].ofs + element_ofs + k];
					response->body[ def->fields_out[i].ofs + element_ofs + k] = response->body[ def->fields_out[i].ofs + element_ofs -k -1 + element_size];
					response->body[ def->fields_out[i].ofs + element_ofs -k -1 + element_size] = temp;
				}
				element_ofs += element_size;
			}
		}
#endif
		memcpy(out, response->body, length);
	}
	return 0;
}


// logLevel_set

struct_def_field_t logLevel_set_in_fields[] = {
    {OFS_AND_SIZE(cmnstrct_log_level_in_t, log_level), 1},
};

struct_def_t logLevel_set_def = {
    CMNSTRUCT_SET_LOG_LEVEL, sizeof(cmnstrct_log_level_in_t), sizeof(logLevel_set_in_fields)/sizeof(logLevel_set_in_fields[0]), &logLevel_set_in_fields[0], 0, 0, NULL,
};

int logLevel_set(int client_id, cmnstrct_log_level_in_t *cmnstrct_log_level_in) {
    return action(client_id, &logLevel_set_def, cmnstrct_log_level_in, NULL, 0);
}


// softCfg_set

struct_def_field_t softCfg_set_in_fields[] = {
	{OFS_AND_SIZE(cmnstrct_rf_soft_config_in_t, ber_test),      1},
	{OFS_AND_SIZE(cmnstrct_rf_soft_config_in_t, tx_mute),       1},
};

struct_def_t softCfg_set_def = {
	CMNSTRUCT_RF_SOFT_CONFIG_SET, sizeof(cmnstrct_rf_soft_config_in_t), sizeof(softCfg_set_in_fields)/sizeof(softCfg_set_in_fields[0]), &softCfg_set_in_fields[0], 0, 0, NULL,
};

int softCfg_set(int client_id, cmnstrct_rf_soft_config_in_t *cmnstrct_rf_soft_config_in) {
	return action(client_id, &softCfg_set_def, cmnstrct_rf_soft_config_in, NULL, 0);
}

// hardCfg_set

struct_def_field_t hardCfg_set_in_fields[] = {
	{OFS_AND_SIZE(cmnstrct_rf_hard_config_in_t, tx_mode),       1},
	{OFS_AND_SIZE(cmnstrct_rf_hard_config_in_t, tx_modulation), 1},
	{OFS_AND_SIZE(cmnstrct_rf_hard_config_in_t, channel_width), 1},
	{OFS_AND_SIZE(cmnstrct_rf_hard_config_in_t, role),          1},
	{OFS_AND_SIZE(cmnstrct_rf_hard_config_in_t, tx_frequency),  1},
};

struct_def_t hardCfg_set_def = {
	CMNSTRUCT_RF_HARD_CONFIG_SET, sizeof(cmnstrct_rf_hard_config_in_t), sizeof(hardCfg_set_in_fields)/sizeof(hardCfg_set_in_fields[0]), &hardCfg_set_in_fields[0], 0, 0, NULL,
};

int hardCfg_set(int client_id, cmnstrct_rf_hard_config_in_t *cmnstrct_rf_hard_config_in) {
	return action(client_id, &hardCfg_set_def, cmnstrct_rf_hard_config_in, NULL, 0);
}

// modulationTbl_create_set

struct_def_field_t modulationTbl_create_in_fields[] = {
	{OFS_AND_SIZE(cmnstrct_modulation_tbl_in_t, active_bitmap),ELEMENTS(cmnstrct_modulation_tbl_in_t, active_bitmap)},
	{OFS_AND_SIZE(cmnstrct_modulation_tbl_in_t, mse_low),  (sizeof(((cmnstrct_modulation_tbl_in_t*)0)->mse_low) / sizeof(((cmnstrct_modulation_tbl_in_t*)0)->mse_low[0][0]))  },
	{OFS_AND_SIZE(cmnstrct_modulation_tbl_in_t, mse_high), (sizeof(((cmnstrct_modulation_tbl_in_t*)0)->mse_high)/ sizeof(((cmnstrct_modulation_tbl_in_t*)0)->mse_high[0][0])) },
	{OFS_AND_SIZE(cmnstrct_modulation_tbl_in_t, backoff),  (sizeof(((cmnstrct_modulation_tbl_in_t*)0)->backoff) / sizeof(((cmnstrct_modulation_tbl_in_t*)0)->backoff[0][0]))  },
};

struct_def_t modulationTbl_create_set_def = {
	CMNSTRUCT_MODULATION_TBL_SET, sizeof(cmnstrct_modulation_tbl_in_t), sizeof(modulationTbl_create_in_fields)/sizeof(modulationTbl_create_in_fields[0]), &modulationTbl_create_in_fields[0], 0, 0, NULL,
};

int modulationTbl_set(int client_id, cmnstrct_modulation_tbl_in_t *cmnstrct_modulation_tbl_in) {
	return action(client_id, &modulationTbl_create_set_def, cmnstrct_modulation_tbl_in, NULL, 0);
}


// loopback_set

struct_def_field_t loopback_set_in_fields[] = {
	{OFS_AND_SIZE(cmnstrct_loopback_set_in_t, rf_loopback_mode),       1},
	{OFS_AND_SIZE(cmnstrct_loopback_set_in_t, rf_loopback_dir),        1},
	{OFS_AND_SIZE(cmnstrct_loopback_set_in_t, rf_loopback_modulation), 1},
};

struct_def_t loopback_set_def = {
		CMNSTRUCT_LOOPBACK_SET, sizeof(cmnstrct_loopback_set_in_t), sizeof(loopback_set_in_fields)/sizeof(loopback_set_in_fields[0]), &loopback_set_in_fields[0], 0, 0, NULL,
};

int rf_loopback_set(int client_id, cmnstrct_loopback_set_in_t *cmnstrct_loopback_set_in) {
	return action(client_id, &loopback_set_def, cmnstrct_loopback_set_in, NULL, 0);
}

// rf_state_set

struct_def_field_t rf_state_set_in_fields[] = {
	{OFS_AND_SIZE(cmnstrct_rf_state_set_in_t, atpc),                     1},
	{OFS_AND_SIZE(cmnstrct_rf_state_set_in_t, rssi_ptx_correction),      1},
	{OFS_AND_SIZE(cmnstrct_rf_state_set_in_t, disable_probe_alignment),  1},
	{OFS_AND_SIZE(cmnstrct_rf_state_set_in_t, desired_tx_power),         1},
	{OFS_AND_SIZE(cmnstrct_rf_state_set_in_t, atpc_min_tx_power),        1},
	{OFS_AND_SIZE(cmnstrct_rf_state_set_in_t, atpc_target_rssi),         1},
	{OFS_AND_SIZE(cmnstrct_rf_state_set_in_t, atpc_min_mse),             1},
};

struct_def_t rf_state_set_def = {
		CMNSTRUCT_RF_STATE, sizeof(cmnstrct_rf_state_set_in_t), sizeof(rf_state_set_in_fields)/sizeof(rf_state_set_in_fields[0]), &rf_state_set_in_fields[0], 0, 0, NULL,
};

int rf_state_set(int client_id, cmnstrct_rf_state_set_in_t *cmnstrct_rf_state_set_in) {
	return action(client_id, &rf_state_set_def, cmnstrct_rf_state_set_in, NULL, 0);
}

// rf_debug_set

struct_def_field_t rf_debug_set_in_fields[] = {
	{OFS_AND_SIZE(cmnstrct_rf_debug_set_in_t, calib), 1},
};

struct_def_t rf_debug_set_def = {
		CMNSTRUCT_RF_DEBUG_SET, sizeof(cmnstrct_rf_debug_set_in_t), sizeof(rf_debug_set_in_fields)/sizeof(rf_debug_set_in_fields[0]), &rf_debug_set_in_fields[0], 0, 0, NULL,
};

int rf_debug_set(int client_id, cmnstrct_rf_debug_set_in_t *cmnstrct_rf_debug_set_in) {
	return action(client_id, &rf_debug_set_def, cmnstrct_rf_debug_set_in, NULL, 0);
}

// rf_reset
struct_def_field_t rf_reset_in_fields[] = {
    {OFS_AND_SIZE(cmnstrct_rf_reset_in_t, reset_rf), 1},
};

struct_def_t rf_reset_def = {
        CMNSTRUCT_RF_RESET, sizeof(cmnstrct_rf_reset_in_t), sizeof(rf_reset_in_fields)/sizeof(rf_reset_in_fields[0]), &rf_reset_in_fields[0], 0, 0, NULL,
};

int rf_reset(int client_id, cmnstrct_rf_reset_in_t *cmnstrct_rf_reset_in) {
    return action(client_id, &rf_reset_def, cmnstrct_rf_reset_in, NULL, 0);
}

// system_info_set
struct_def_field_t system_info_set_in_fields[] = {
    {OFS_AND_SIZE(cmnstrct_system_info_set_in_t, product_type), 1},
};

struct_def_t system_info_set_def = {
        CMNSTRUCT_SYSTEM_INFO_SET, sizeof(cmnstrct_system_info_set_in_t), sizeof(system_info_set_in_fields)/sizeof(system_info_set_in_fields[0]), &system_info_set_in_fields[0], 0, 0, NULL,
};

int system_info_set(int client_id, cmnstrct_system_info_set_in_t *cmnstrct_system_info_set_in) {
    return action(client_id, &system_info_set_def, cmnstrct_system_info_set_in, NULL, 0);
}

// rf_reset_stats

struct_def_field_t rf_reset_stats_in_fields[] = {
	{OFS_AND_SIZE(cmnstrct_reset_stats_in_t, rf_reset_hist_stats),       1},
};

struct_def_t rf_reset_stats_def = {
		CMNSTRUCT_RESET_STATS, sizeof(cmnstrct_reset_stats_in_t), sizeof(rf_reset_stats_in_fields)/sizeof(rf_reset_stats_in_fields[0]), &rf_reset_stats_in_fields[0], 0, 0, NULL,
};

int rf_reset_stats(int client_id, cmnstrct_reset_stats_in_t *cmnstrct_rf_reset_stats_in) {
	return action(client_id, &rf_reset_stats_def, cmnstrct_rf_reset_stats_in, NULL, 0);
}

// rf_reset_ber_stats

struct_def_field_t rf_reset_ber_stats_in_fields[] = {
    {OFS_AND_SIZE(cmnstrct_reset_ber_stats_in_t, rf_reset_ber_stats),     1},
};

struct_def_t rf_reset_ber_stats_def = {
        CMNSTRUCT_RESET_BER_STATS, sizeof(cmnstrct_reset_ber_stats_in_t), sizeof(rf_reset_ber_stats_in_fields)/sizeof(rf_reset_ber_stats_in_fields[0]), &rf_reset_ber_stats_in_fields[0], 0, 0, NULL,
};

int rf_reset_ber_stats(int client_id, cmnstrct_reset_ber_stats_in_t *cmnstrct_rf_reset_ber_stats_in) {
    return action(client_id, &rf_reset_ber_stats_def, cmnstrct_rf_reset_ber_stats_in, NULL, 0);
}

// rf_debug_stat_get

static struct_def_field_t rfDebug_stat_out_fields[] = {
    {OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, rf_tx_state),         1},
    {OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, rf_rx_state),         1},
    {OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, rx_modulation),       1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, max_modulation_id),   1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, max_modulation_rate), 1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, min_modulation_id),   1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, min_modulation_rate), 1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, calib_finished),      1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, agc_line),            1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, mse),                 1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, rssi),                1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, backoff),             1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, in_ber_total),        1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, in_ber_error),        1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, in_octets),           1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, out_octets),          1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, in_pkts),             1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, out_pkts),            1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, in_errored_pkts),     1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, phase_noise),         1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, frequency_offset),    1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, min_mse),             1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, max_mse),             1},
	{OFS_AND_SIZE(cmnstrct_rf_debug_stat_get_out_t, link_length),         1},
};

struct_def_t rfDebug_stat_get_def = {
	CMNSTRUCT_RF_DEBUG_STAT_GET, 0, 0, NULL, sizeof(cmnstrct_rf_debug_stat_get_out_t), sizeof(rfDebug_stat_out_fields)/sizeof(rfDebug_stat_out_fields[0]), &rfDebug_stat_out_fields[0]
};

int rf_debug_stat_get(int client_id, cmnstrct_rf_debug_stat_get_out_t *rfDebug_stat_out) {
	return action(client_id, &rfDebug_stat_get_def, NULL, rfDebug_stat_out, sizeof(*rfDebug_stat_out));
}

// rf_system_info_get

static struct_def_field_t rf_system_info_get_out_fields[] = {
	{OFS_AND_SIZE(cmnstrct_system_info_get_out_t, system_voltage_dc),  1},
	{OFS_AND_SIZE(cmnstrct_system_info_get_out_t, system_voltage_poe), 1},
};

struct_def_t rf_system_info_get_def = {
		CMNSTRUCT_SYSTEM_INFO_GET, 0, 0, NULL, sizeof(cmnstrct_system_info_get_out_t), sizeof(rf_system_info_get_out_fields)/sizeof(rf_system_info_get_out_fields[0]), &rf_system_info_get_out_fields[0]
};

int rf_system_info_get(int client_id, cmnstrct_system_info_get_out_t *system_info_out) {
	return action(client_id, &rf_system_info_get_def, NULL, system_info_out, sizeof(*system_info_out));
}


// rf_stat_get

static struct_def_field_t rf_stat_out_fields[] = {
    {OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, tpc),                                                         1},
    {OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, sub_band),       ELEMENTS(cmnstrct_rf_stat_get_out_t, sub_band)},
    {OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, temperature), ELEMENTS(cmnstrct_rf_stat_get_out_t, temperature)},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, actual_tx_power),                                             1},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, min_rssi),                                                    1},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, max_rssi),                                                    1},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, rx_iq_imbalance_gain),                                        1},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, rx_iq_imbalance_phase),                                       1},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, tx_iq_imbalance_gain),                                        1},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, tx_iq_imbalance_phase),                                       1},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, remote_mse),                                                  1},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, remote_rssi),                                                 1},
    {OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, lo_correction_I),                                             1},
    {OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, lo_correction_Q),                                             1},
	{OFS_AND_SIZE(cmnstrct_rf_stat_get_out_t, vtune),             ELEMENTS(cmnstrct_rf_stat_get_out_t, vtune)},
};

struct_def_t rf_stat_get_def = {
	CMNSTRUCT_RF_STAT_GET, 0, 0, NULL, sizeof(cmnstrct_rf_stat_get_out_t), sizeof(rf_stat_out_fields)/sizeof(rf_stat_out_fields[0]), &rf_stat_out_fields[0]
};

int rf_stat_get(int client_id, cmnstrct_rf_stat_get_out_t *rf_stat_out) {
	return action(client_id, &rf_stat_get_def, NULL, rf_stat_out, sizeof(*rf_stat_out));
}


// acm_counters_get

static struct_def_field_t acm_counters_out_fields[] = {
	{OFS_AND_SIZE(BCM85100_ACM_COUNTERS_STRUCT, acmRxProfilesHistory), 1},
	{OFS_AND_SIZE(BCM85100_ACM_COUNTERS_STRUCT, acmTxProfilesHistory), 1},
};

struct_def_t acm_counters_get_def = {
	CMNSTRUCT_ACM_COUNTERS_GET, 0, 0, NULL, sizeof(BCM85100_ACM_COUNTERS_STRUCT), sizeof(acm_counters_out_fields)/sizeof(acm_counters_out_fields[0]), &acm_counters_out_fields[0]
};

int acm_counters_get(int client_id, BCM85100_ACM_COUNTERS_STRUCT *acm_counters_out) {
	return action(client_id, &acm_counters_get_def, NULL, acm_counters_out, sizeof(*acm_counters_out));
}


