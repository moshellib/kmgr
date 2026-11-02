#ifndef _CMNSTRUCT_H
#define _CMNSTRUCT_H

enum BrcmModulationName {
	BRCM_MODULATION_QAM64 = 0,
	BRCM_MODULATION_QAM32 = 1,
	BRCM_MODULATION_QAM16 = 2,
	BRCM_MODULATION_QPSK3 = 3,
	BRCM_MODULATION_QPSK2 = 4,
	BRCM_MODULATION_QPSK1 = 5,
	BRCM_MODULATION_BPSK2 = 6,
	BRCM_MODULATION_BPSK1 = 7,
	BRCM_MODULATION_MAX   = 8 //always last
};

enum BrcmModulationMask {
	BRCM_MASK_NONE  = 0,
	BRCM_MASK_QAM64 = 1,
	BRCM_MASK_QAM32 = 2,
	BRCM_MASK_QAM16 = 4,
	BRCM_MASK_QPSK3 = 8,
	BRCM_MASK_QPSK2 = 16,
	BRCM_MASK_QPSK1 = 32,
	BRCM_MASK_BPSK2 = 64,
	BRCM_MASK_BPSK1 = 128,
};

#define BRCM_BW_MAX 4

enum BrcmBandWidth {
	BRCM_BW_250  = 0,
	BRCM_BW_500  = 1,
	BRCM_BW_750  = 2,
	BRCM_BW_1250 = 3,
	BRCM_BAND_WIDTH_MAX  = BRCM_BW_MAX //always last
};

typedef enum {
	CMNSTRUCT_RF_DEBUG_SET  		         = 0,
	CMNSTRUCT_RF_DEBUG_STAT_GET	      	     = 1,
	CMNSTRUCT_MODULATION_TBL_SET      	     = 2,
	CMNSTRUCT_ARQ_COMMON_SET	      	     = 3,
	CMNSTRUCT_RF_RESET		         	     = 4,
	CMNSTRUCT_RF_SOFT_CONFIG_SET      	     = 5,
	CMNSTRUCT_RF_HARD_CONFIG_SET      	     = 6,
	CMNSTRUCT_RF_STAT_GET		        	 = 7,
	CMNSTRUCT_LOOPBACK_SET                   = 8,
	CMNSTRUCT_RF_STATE			             = 9,
	CMNSTRUCT_RESET_STATS                    = 10,
	CMNSTRUCT_RESET_BER_STATS                = 11,
	CMNSTRUCT_ACM_COUNTERS_GET               = 12,
	CMNSTRUCT_MODEM_ACQUIRE_COUNTERS_GET     = 13,
	CMNSTRUCT_SYSTEM_INFO_GET                = 14,
	CMNSTRUCT_SYSTEM_INFO_SET                = 15,
	CMNSTRUCT_SET_LOG_LEVEL                  = 16,
} cmns_t;


typedef enum {
	CMN_ROLE_MASTR_E = 0,
	CMN_ROLE_SLAVE_E = 1,
	CMN_ROLE_AUTO_E  = 2,
} cmn_role_t;

typedef enum
{
	CMN_TX_MUTE_DISABLE_E  = 0,
	CMN_TX_MUTE_ENABLE_E   = 1,
} cmn_tx_mute_t;

typedef enum
{
	CMN_MODE_ADAPTIVE_E = 0,
	CMN_MODE_STATIC_E   = 1,
	CMN_MODE_ALIGN_E 	= 2,
} cmn_mode_t;

typedef enum
{
	CMN_RF_LOOPBACK_DISABLED_E  		= 1,
	CMN_RF_LOOPBACK_INTERNAL_MAC_SWAP_E = 2,
	CMN_RF_LOOPBACK_INTERNAL_E 			= 3,
} cmn_loopback_mode_t;

typedef enum
{
	CMN_RF_LOOPBACK_TX_E = 1,
	CMN_RF_LOOPBACK_RX_E = 2,
} cmn_loopback_dir_t;

typedef enum
{
	CMN_RF_OPERATIONAL_UP_E = 0,
	CMN_RF_OPERATIONAL_DOWN_E = 1,
} cmn_rf_operational_t;

typedef enum
{
	CMN_RF_SYNC_E			  =0,
	CMN_RF_SEARCH_COUNTDOWN_E =1,
	CMN_RF_FOUND_COUNTDOWN_E  =2,
	CMN_RF_NORMAL_E			  =3,
} cmn_rf_sync_state_t;


typedef enum
{
    CMN_GLOBAL_IDENTITY_2200F_E  =0,
    CMN_GLOBAL_IDENTITY_2200FX_E =1,
    CMN_GLOBAL_IDENTITY_2500F_E  =2,
    CMN_GLOBAL_IDENTITY_2500FX_E =3,
    CMN_GLOBAL_IDENTITY_5500FX_E =4,
} cmn_global_identity_mode_t;

typedef enum
{
    CMN_AGENT_FALSE_E = 0,
    CMN_AGENT_TRUE_E  = 1,
} cmn_agent_t;

typedef enum
{
	CMN_RF_BER_TEST_DISABLE_E  = 0,
	CMN_RF_BER_TEST_ENABLE_E = 1,
} cmn_ber_test_t ;

typedef enum
{
	CMN_CALIB_ENABLE_E  = 1,
	CMN_CALIB_DISABLE_E = 2,
} cmn_calib_t ;

typedef enum
{
	CMN_ARQ_COMMON_STATE_DOWN_E = 0,
	CMN_ARQ_COMMON_STATE_UP_E 	= 1,
} cmn_arq_common_state_t;

typedef enum
{
	CMN_FALSE_E = 0 , // FALSE must be equal to 0
	CMN_TRUE_E  = 1
} cmn_bool_t;

// CMNSTRUCT_SET_LOG_LEVEL
typedef struct {
    UINT8  log_level;
} cmnstrct_log_level_in_t;

// CMNSTRUCT_RF_STATE
typedef struct {
	UINT8  atpc;						  // cmn_agent_t
	UINT8  rssi_ptx_correction;			  // cmn_agent_t
	UINT8  disable_probe_alignment;		  // cmn_agent_t
	INT32  desired_tx_power;
	INT32  atpc_min_tx_power;
	INT32  atpc_target_rssi;
	INT32  atpc_min_mse;
} cmnstrct_rf_state_set_in_t;


// CMNSTRUCT_LOOPBACK_SET
typedef struct {
	UINT8  rf_loopback_mode;    	      // cmn_loopback_mode_t
	UINT8  rf_loopback_dir; 		  	  // cmn_loopback_dir_t
	UINT8  rf_loopback_modulation; 		  // BrcmModulationName
} cmnstrct_loopback_set_in_t;

// CMNSTRUCT_RESET_STATS
typedef struct {
	UINT8  rf_reset_hist_stats;     // reset min & max mse, rssi, modulation id
} cmnstrct_reset_stats_in_t;

// CMNSTRUCT_RESET_BER_STATS
typedef struct {
    UINT8  rf_reset_ber_stats;
} cmnstrct_reset_ber_stats_in_t;

// CMNSTRUCT_RF_DEBUG_SET
typedef struct {
	UINT8  calib;					// cmn_calib_t
}cmnstrct_rf_debug_set_in_t;

// CMNSTRUCT_RF_SOFT_CONFIG_SET
typedef struct {
	UINT8  ber_test;				// cmn_ber_test_t - You can change it only by configuring the modem with different bin file
	UINT8  tx_mute;       		 	// cmn_tx_mute_t
}cmnstrct_rf_soft_config_in_t;

// CMNSTRUCT_RF_HARD_CONFIG_SET
typedef struct {
	UINT8  tx_mode; 	 		// cmn_mode_t
	UINT8  tx_modulation; 		// BrcmModulationName
	UINT8  channel_width;	 	// cmn_channel_witdth_t - You can change it only by configuring the modem with different bin file
	UINT8  role;          		// cmn_role_t
	UINT32 tx_frequency;
} cmnstrct_rf_hard_config_in_t;

// CMNSTRUCT_MODULATION_TBL_SET
typedef struct
{
	//Indexes are arranged by BrcmModulationName !
	UINT8 active_bitmap[BRCM_BW_MAX];
	INT16 mse_low[BRCM_BW_MAX][8];		//multiplied by 10
	INT16 mse_high[BRCM_BW_MAX][8];		//multiplied by 10
	UINT8 backoff[BRCM_BW_MAX][8];
} cmnstrct_modulation_tbl_in_t;


// CMNSTRUCT_ARQ_COMMON_SET
typedef struct {
	UINT8 arq_admin;					// cmn_arq_common_state_t -2do relevant only for modulation table ?
}cmnstrct_arq_common_set_in_t;


// CMNSTRUCT_RF_RESET
typedef struct {
	UINT8 reset_rf;
}cmnstrct_rf_reset_in_t;


// CMNSTRUCT_SYSTEM_INFO_SET
typedef struct {
    UINT8  product_type;            // cmn_global_identity_mode_t
} cmnstrct_system_info_set_in_t;


// CMNSTRUCT_RF_DEBUG_STAT_GET
typedef struct {
	UINT8  rf_tx_state;			    // cmn_rf_sync_state_t
	UINT8  rf_rx_state;			    // cmn_rf_sync_state_t
	UINT8  rx_modulation;           // BrcmModulationName
	UINT8  max_modulation_id;
	UINT8  max_modulation_rate;
	UINT8  min_modulation_id;
	UINT8  min_modulation_rate;
	UINT8  calib_finished;          // cmn_agent_t
	UINT16 agc_line;
	INT32  mse;						//multiplied by 10
	INT32  rssi;                    //multiplied by 10
	UINT32 backoff;
	UINT64 in_ber_total;            //final value. clear is made in k60
	UINT64 in_ber_error;            //final value. clear is made in k60
	UINT64 in_octets;
	UINT64 out_octets;
	UINT64 in_pkts;
	UINT64 out_pkts;
	UINT64 in_errored_pkts;
	UINT32 phase_noise;			//new
	UINT32 frequency_offset;	//new
	INT32  min_mse;                  //multiplied by 10
	INT32  max_mse;                  //multiplied by 10
	INT32  link_length;
} cmnstrct_rf_debug_stat_get_out_t;


//CMNSTRUCT_SYSTEM_INFO_GET
typedef struct {
	UINT32 system_voltage_dc;
	UINT32 system_voltage_poe;
} cmnstrct_system_info_get_out_t;

// CMNSTRUCT_RF_STAT_GET
typedef struct {
    UINT8  tpc;
    UINT8  sub_band[2];               // [0] is 70G , [1] is 80G.
	INT32  temperature[2];			  // [0] is 70G , [1] is 80G.
	INT32  actual_tx_power;           //multiplied by 10
	INT32  min_rssi;                  //multiplied by 10
	INT32  max_rssi;                  //multiplied by 10
	INT32  rx_iq_imbalance_gain;
	INT32  rx_iq_imbalance_phase;
	INT32  tx_iq_imbalance_gain;      // new
	INT32  tx_iq_imbalance_phase;     // new
	INT32  remote_mse;				  //multiplied by 10
	INT32  remote_rssi;		          //multiplied by 10
    INT32  lo_correction_I;           //multiplied by 10
    INT32  lo_correction_Q;           //multiplied by 10
	UINT32 vtune[2];                  // [0] is 70G , [1] is 80G. // multiplied by 1000
} cmnstrct_rf_stat_get_out_t;

#endif // _CMNSTRUCT_H
