/* Copyright 2013 Broadcom Corporation
*
 * This program is the proprietary software of Broadcom Corporation and/or
* its licensors, and may only be used, duplicated, modified or distributed
* pursuant to the terms and conditions of a separate, written license
* agreement executed between you and Broadcom (an "Authorized License").
* Except as set forth in an Authorized License, Broadcom grants no license
* (express or implied), right to use, or waiver of any kind with respect to
* the Software, and Broadcom expressly reserves all rights in and to the
* Software and all intellectual property rights therein.  IF YOU HAVE NO
* AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
* WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
* THE SOFTWARE.
 *
* Except as expressly set forth in the Authorized License,
*
* 1. This program, including its structure, sequence and organization,
* constitutes the valuable trade secrets of Broadcom, and you shall use
* all reasonable efforts to protect the confidentiality thereof, and to
* use this information only in connection with your use of Broadcom
* integrated circuit products.
*
* 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
* "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
* REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
* OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
* DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
* NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
* ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
* CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
* OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
*
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
* BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
* SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
* IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
* IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
* ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
* OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
* NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*/

#ifndef _BCM85100_API_H_
#define _BCM85100_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HOSTDRIVER_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#include "defs85100.h"
#include "hostProtocolStructs.h"

//----------------------------------------------------
// DEFINITIONS
//----------------------------------------------------

// Boot
#define BCM85100_RESET_TIMEOUT                        1000000
#define BCM85100_BOOT_IS_ALIVE_TIMEOUT_MILLI          100
#define BCM85100_SPI_100KHZ							  100000
#define BCM85100_SPI_1MHZ							  1000000

#define BCM85100_BOOTFW_WAIT_TIME_MICRO				  1000000
#define BCM85100_STM_BOOTFW_FLASH_ERASE_WAIT_TIME_MICRO	3000000 //In STM Only Wait for Flash Erase 

#define BCM85100_FW_WAIT_TIME_MICRO					  1000000

// Auxielry
#define BCM85100_ADCS_NUM							  8
#define BCM85100_DACS_NUM							  16

// IQ samples
#define BCM85100_MAX_NUMBER_OF_PAIRS                8160
#define BCM85100_MAX_ADC_NUMBER_OF_PAIRS            1024
#define BCM85100_MAX_DAC_NUMBER_OF_PAIRS            3072
#define BCM85100_MAX_EQU_OUT_NUMBER_OF_PAIRS        2720
#define BCM85100_MAX_ED_NUMBER_OF_PAIRS             512
#define BCM85100_MAX_PAIRS_IN_CHUNK                 24
#define BCM85100_EQU_OUT_MAX_PAIRS_IN_CHUNK         16
#define BCM85100_TXTX_ED_DAC_MAX_PAIRS_IN_CHUNK     32
#define BCM85100_EQU_OUT_RECORD_MAX_TRIES           3

//----------------------------------------------------
// MACROS
//----------------------------------------------------
#define RETURNED_LENGTH_CHECK(expectedLength)                               \
	if ((retVal == BCM85100_SUCCESS_MSG_E) && (rxLen != (expectedLength)))  \
	{																		\
		retVal = BCM85100_INVALID_MSG_RX_E;                                 \
	}

#define BYTE_CAST(val)  (BYTE)val
#define BYTEP_CAST(val) (BYTE*)val
#define BOOL_CAST(val)  (BOOLEAN)val

//----------------------------------------------------
// ENUMS
//----------------------------------------------------

typedef enum
{
    //success
    BCM85100_SUCCESS_MSG_E,									//= 0

    //boot errors
    BCM85100_BOOT_DATA_TOO_BIG_E,							//= 1
    BCM85100_BOOT_DATA_TOO_SMALL_E,							//= 2
    BCM85100_BOOT_DATA_WRONG_INDEX_E,						//= 3
    BCM85100_BOOT_INVALID_CHECKSUM_E,						//= 4
    BCM85100_BOOT_INVALID_JUMP_ADDRESS_E,					//= 5
    BCM85100_BOOT_OPCODE_UNEXPECTED_E,						//= 6
    BCM85100_BOOT_FW_NOT_ALIVE_E,							//= 7
    BCM85100_FW_NOT_ALIVE_E,								//= 8
    BCM85100_BOOT_PLL_ERROR_E,								//= 9
    BCM85100_BOOT_DDR_ERROR_E,								//= 10

    //================================================

    //Communication Layer errors
    BCM85100_INVALID_MSG_RX_E,								//= 11
    BCM85100_INVALID_MSG_OPCODE_E,							//= 12
    BCM85100_ERROR_IN_COM_RECEIVE_E,						//= 13
    BCM85100_INVALID_MODEM_INDEX_E,							//= 14
    BCM85100_HOST_BUSY_E,									//= 15
    BCM85100_INVALID_PARAMS_E,								//= 16
    BCM85100_MEDIA_NOT_INITIALIZED_E,						//= 17
    BCM85100_INVALID_MEDIA_E,								//= 18
    BCM85100_INVALID_DEVICE_INDEX_E,						//= 19

    //config file errors
    BCM85100_ERROR_CFG_FILE_LEN_TOO_BIG_E,                  //= 20
    BCM85100_ERROR_CFG_FILE_LEN_TOO_SMALL_E,                //= 21
    BCM85100_ERROR_CFG_FILE_CRC_E,                          //= 22
    BCM85100_ERROR_CFG_FILE_UNKNOWN_RECORD_TYPE_E,          //= 23
    BCM85100_ERROR_CFG_FILE_VERSION_E,                      //= 24
    BCM85100_ERROR_CFG_FILE_ALREADY_CONFIGURED_E,           //= 25
    BCM85100_ERROR_CFG_FILE_TIMEOUT_E,                      //= 26

    //Peripherals error
    BCM85100_PERIPHERAL_NOT_READY_E,                        //= 27
    BCM85100_PERIPHERAL_TOO_MANY_BYTES_E,                   //= 28
    BCM85100_PERIPHERAL_TRANSACTION_LEN_CANNOT_BE_ZERO_E,   //= 29
    BCM85100_PERIPHERAL_I2C_ERROR_E,                        //= 30
    BCM85100_PERIPHERAL_SPI_ERROR_E,                        //= 31
    BCM85100_PERIPHERAL_TIMEOUT_E,                          //= 32
    BCM85100_PERIPHERAL_INVALID_SLAVE_ADDRESS_E,            //= 33

    //API errors
    BCM85100_ERROR_NOT_CONFIGURED_E,						//= 34
    BCM85100_IQ_SAMPLES_GET_ERROR_E,						//= 35
    BCM85100_LOOPBACK_INVALID_REQUEST_E,					//= 36
    BCM85100_ERROR_IN_SWITCH_E,								//= 37
    BCM85100_ERROR_ACM_ENGINE_ENABLED_E,					//= 38
    BCM85100_ACM_IMPROVE_REQUEST_EXPIRED_E,					//= 39
    BCM85100_TEST_BUS_BUSY_E,								//= 40
    BCM85100_HOST_MAILBOX_EMPTY_E,							//= 41
    BCM85100_HOST_MAILBOX_FULL_E,							//= 42
    BCM85100_CONFIGURATION_NOT_SUPPORTED_E,					//= 43

    //General API errors
    BCM85100_API_GENERAL_ERROR_E,							//= 44
    BCM85100_API_NOT_SUPPORTED_E,							//= 45
    BCM85100_API_INVALID_PARAMS_E,							//= 46
    BCM85100_API_MODEM_RECONFIG_ERROR_E,					//= 47

    //New errors
    BCM85100_ERROR_ACM_TRYING_DISABLE_CURRENT_PROFILE_E,	//= 48
    BCM85100_ERROR_ACM_UP_REQUEST_FROM_HOST_DISABLED_E,	    //= 49
    BCM85100_ERROR_CONFIGURATION_RERUN_NEEDED_E,			//= 50
    BCM85100_ERROR_CHIP_FLAVOR_RESTRICTED_E,                //= 51
    BCM85100_CAPTURE_ERROR_E,                               //= 52
    BCM85100_CAPTURE_ERROR_ACM_SWITCH_E,                    //= 53
    BCM85100_CAPTURE_LENGTH_UNAVILABLE_E,                   //= 54
    BCM85100_CAPTURE_ERROR_PREAMBLE_E,                      //= 55
    BCM85100_ERROR_HIGH_GAIN_VAL_E,                         //= 56
    BCM85100_ERROR_LOW_GAIN_VAL_E,                          //= 57

} BCM85100_ERROR_MSG_ENUM;

typedef enum
{
    BCM85100_COM_MEDIA_NULL_E,        // unconfigured media
    BCM85100_COM_MEDIA_UART_E,
    BCM85100_COM_MEDIA_SPI_E,
    BCM85100_COM_MEDIA_DPRAM_E,		  // Not Supported
    BCM85100_COM_MEDIA_MII_DEVICE_E,  // MII used for host-device communication
    BCM85100_COM_MEDIA_IP_CLIENT_E,   // Remote connection of MDP to message relay in host. Not for host-device communication
} BCM85100_COM_MEDIA_ENUM;

typedef enum
{
    BCM85100_DEVICE_RESET_ALL_E       = 0,  // reset MC-CPU and BCM85100
} BCM85100_DEVICE_RESET_TYPE_ENUM;

typedef enum
{
    BCM85100IFSBG        = 0,
    BCM85060HIFSBG       = 1,
    BCM85060LIFSBG       = 2,
    UNKNOWN              = 3,
} BCM85100_CHIP_FLAVORS_ENUM;

typedef enum
{
    BCM85100_ACQUIRE_IN_PROGRESS_E,
    BCM85100_ACQUIRE_LOCKED_E,
    BCM85100_ACQUIRE_UNLOCKED_E
} BCM85100_ACQUIRE_STATUS_ENUM;

typedef enum
{
    BCM85100_ACQUIRE_SUCCESS_E              = 0,
    BCM85100_ACQUIRE_ERR_AGC_E              = 1,    //(not in use)
    BCM85100_ACQUIRE_ERR_TIMING_E           = 2,  	//(not in use)
    BCM85100_ACQUIRE_ERR_FREQ_SWEEP_E       = 3,
    BCM85100_ACQUIRE_ERR_MSE_E              = 4,
    BCM85100_ACQUIRE_ERR_BIT_E              = 5,
    BCM85100_ACQUIRE_ERR_SERVICE_E          = 6,	//(not in use)
    BCM85100_ACQUIRE_ERR_BLIND_E            = 7,    //(not in use)
    BCM85100_ACQUIRE_ERR_TIMEOUT_E          = 10,
    BCM85100_ACQUIRE_ERR_STOPPED_E          = 11,	//modemStop called before Acquire ended
    BCM85100_ACQUIRE_ERR_FATAL_E            = 12,	//error in state-machine (came from UNKNOWN_STATE)
} BCM85100_ACQUIRE_ERR_ENUM;

typedef enum
{
    BCM85100_MODULATION_BPSK_E       =   0,
    BCM85100_MODULATION_QPSK_E		 =   1,
    BCM85100_MODULATION_16_POINTS_E  =   3,
    BCM85100_MODULATION_32_POINTS_E  =   4,
    BCM85100_MODULATION_64_QUAM_E    =   5,
    BCM85100_MODULATION_128_QUAM_E   =   6,
    BCM85100_MODULATION_256_QUAM_E   =   7,
    BCM85100_MODULATION_512_QUAM_E   =   8, // Not Supported
    BCM85100_MODULATION_1024_QUAM_E  =   9, // Not Supported
} BCM85100_MODULATION_TYPE_ENUM;

typedef enum
{
    BCM85100_BRC_INDEX_0_E			=	 0,
    BCM85100_BRC_INDEX_1_E			=	 1,
    BCM85100_BRC_INDEX_2_E			=	 2,
} BCM85100_BRC_INDEX_TYPE_ENUM;

typedef enum
{
    BCM85100_ACM_PROFILE_0_E			=	 0,
    BCM85100_ACM_PROFILE_1_E			=	 1,
    BCM85100_ACM_PROFILE_2_E			=	 2,
    BCM85100_ACM_PROFILE_3_E			=	 3,
    BCM85100_ACM_PROFILE_4_E			=	 4,
    BCM85100_ACM_PROFILE_5_E			=	 5,
    BCM85100_ACM_PROFILE_6_E			=	 6,
    BCM85100_ACM_PROFILE_7_E			=	 7,
    BCM85100_ACM_PROFILE_8_E			=	 8,
    BCM85100_ACM_PROFILE_9_E			=	 9,
    BCM85100_ACM_PROFILE_10_E			=	 10,
    BCM85100_ACM_PROFILE_11_E			=	 11,
    BCM85100_ACM_PROFILE_12_E			=	 12,
    BCM85100_ACM_PROFILE_13_E			=	 13,
    BCM85100_ACM_PROFILE_14_E			=	 14,
    BCM85100_ACM_PROFILE_15_E			=	 15,
} BCM85100_ACM_PROFILE_ENUM;

typedef enum
{
    BCM85100_ACM_DEGRADE_TABLE_E,
    BCM85100_ACM_IMPROVE_TABLE_E,
} BCM85100_ACM_TABLE_ENUM;

typedef enum
{
    BCM85100_CRITERIA_NA_E,
    BCM85100_CRITERIA_MSE_ONLY_E,
    BCM85100_CRITERIA_ANY_E, // Not Supported
    BCM85100_CRITERIA_ALL_E, // Not Supported
} BCM85100_ACM_CRITERIA_ENUM;

typedef enum
{
    BCM85100_SGMII_STATUS_LINK_UP_E,
    BCM85100_SGMII_STATUS_LINK_DOWN_E,
    BCM85100_SGMII_STATUS_NOT_SUPPORTED_E,
} BCM85100_SGMII_STATUS_ENUM;

typedef enum
{
    BCM85100_SGMII_STATUS_TYPE_FRAMES_E                  = 0,
    BCM85100_SGMII_STATUS_TYPE_BYTES_E                   = 1,
    BCM85100_SGMII_STATUS_TYPE_UNICAST_E                 = 2,
    BCM85100_SGMII_STATUS_TYPE_BROADCAST_E               = 3,
    BCM85100_SGMII_STATUS_TYPE_MULTICAST_E               = 4,
    BCM85100_SGMII_STATUS_TYPE_CRC_E                     = 5,
    BCM85100_SGMII_STATUS_TYPE_UNDERSIZE_E               = 6,
    BCM85100_SGMII_STATUS_TYPE_64B_E                     = 7,
    BCM85100_SGMII_STATUS_TYPE_65_127_E                  = 8,
    BCM85100_SGMII_STATUS_TYPE_128_255_E                 = 9,
    BCM85100_SGMII_STATUS_TYPE_256_511_E                 = 10,
    BCM85100_SGMII_STATUS_TYPE_512_1023_E                = 11,
    BCM85100_SGMII_STATUS_TYPE_1024_1518_E               = 12,
    BCM85100_SGMII_STATUS_TYPE_JAMBO_E                   = 13,
    BCM85100_SGMII_STATUS_TYPE_MAX                       = 14,
} BCM85100_SGMII_STATUS_TYPE_ENUM;

typedef enum
{
    BCM85100_PCS_PAUSE_NO_PAUSE_E                    = 0,
    BCM85100_PCS_PAUSE_ASYMMETRIC_PAUSE_E            = 1,
    BCM85100_PCS_PAUSE_SYMMETRIC_PAUSE_E             = 2,
    BCM85100_PCS_PAUSE_BOTH_PAUSE_E                  = 3,
} BCM85100_PCS_PAUSE_ADV_ENUM;

typedef enum
{
    BCM85100_SGMII_SET_PORT_ENABLE_E					= 0x1,
    BCM85100_SGMII_SET_PORT_MODE_E						= 0x2,
    BCM85100_SGMII_SET_FCS_PROCESSING_E					= 0x4,
    BCM85100_SGMII_SET_PORT_SPEED_E						= 0x8,
    BCM85100_SGMII_SET_PAUSE_BEHAVIOR_E					= 0x10,
    BCM85100_SGMII_SET_PAUSE_THRESHOLD_E				= 0x20,
    BCM85100_SGMII_SET_PAUSE_TIME_E						= 0x40,
    BCM85100_SGMII_SET_ENABLE_AUTO_NEGOTIATION_E		= 0x80,
    BCM85100_SGMII_SET_AUTO_NEGOTIATION_PAUSE_ADVER_E   = 0x100,
    BCM85100_SGMII_SET_TERMINAL_LOOPBACK_E				= 0x200,
    BCM85100_SGMII_SET_LINE_LOOPBACK_E					= 0x400,
} BCM85100_SGMII_SET_SELECT_ENUM;

typedef enum
{
    BCM85100_PCS_MODE_FIBER_E                            = 0,
    BCM85100_PCS_MODE_COPPER_MODE_E                      = 1,
    BCM85100_PCS_MODE_AUTO_FIBER_COPPER_MODE_E			 = 2,
} BCM85100_PCS_MODE_ENUM;

typedef enum
{
    BCM85100_SGMII_PAUSE_BEHAVIOR_RX_DIS_TX_DIS_E        = 0,
    BCM85100_SGMII_PAUSE_BEHAVIOR_RX_DIS_TX_EN_E         = 1,
    BCM85100_SGMII_PAUSE_BEHAVIOR_RX_EN_TX_DIS_E         = 2,
    BCM85100_SGMII_PAUSE_BEHAVIOR_RX_EN_TX_EN_E          = 3,
} BCM85100_SGMII_PAUSE_BEHAVIOR_ENUM;

typedef enum   // bit description of alarms (BCM85100_ALARMS_STRUCT)
{
    BCM85100_ALARM_MDM_BIT_E,
    BCM85100_ALARM_MODEM_LOCKED_E,
    BCM85100_ALARM_MODEM_LOL_E,
    BCM85100_ALARM_ACM_TX_SWITCH_E,
    BCM85100_ALARM_ACM_RX_SWITCH_E,
    BCM85100_ALARM_ACM_IMPRV_REQ_E,
    BCM85100_ALARM_TXTX_CALIB_SUCCESS_E,
    BCM85100_ALARM_TX_ENC_SNIP_E,
    BCM85100_ALARM_TX_GPI_SNIP_E,
    BCM85100_ALARM_TX_AFRM_E,
    BCM85100_ALARM_IMB_MBX_RX_E,
    BCM85100_ALARM_PM_ONE_SEC_BIT_E,
    BCM85100_ALARM_MC_WDOG_E,
    BCM85100_ALARM_CHIP_FLAVOR_RESTRICTED_E,
    BCM85100_ALARM_UNEXPECTED_EXT_ACM_EVENT_E,
    BCM85100_ALRAM_RTC_LOCK_E,
    BCM85100_ALARM_TXTX_CALIB_FAIL_E,
    BCM85100_ALARM_GFP_LOL_STATUS_E
} BCM85100_ALARM_BITS_ENUM;

typedef enum
{
    BCM85100_ACQUIRE_MODE_PREVIOUS_E,
    BCM85100_ACQUIRE_MODE_AUTO_E,
    BCM85100_ACQUIRE_MODE_MANUAL_E
} BCM85100_ACQUIRE_MODE_ENUM;

typedef enum
{
    BCM85100_SPECTRAL_INVERSION_MODE_PREVIOUS_E,
    BCM85100_SPECTRAL_INVERSION_MODE_NEVER_E,
    BCM85100_SPECTRAL_INVERSION_MODE_ALWAYS_E,
    BCM85100_SPECTRAL_INVERSION_MODE_SMART_AUTO_E
} BCM85100_SPECTRAL_INVERSION_MODE_ENUM;


typedef enum
{
    BCM85100_LOOPBACK_NONE_E,        // return to normal operation
    BCM85100_LOOPBACK_GPI_INTERFACE_E,
    BCM85100_LOOPBACK_MODEM_E
} BCM85100_LOOPBACK_TYPE_ENUM;

typedef enum
{
    BCM85100_TRANSMITTER_MODE_NORMAL_E,
    BCM85100_TRANSMITTER_MODE_ONETONE_E,
    BCM85100_TRANSMITTER_MODE_TWOTONE_E,
    BCM85100_TRANSMITTER_MODE_TONE_SWEEP_E,
    BCM85100_TRANSMITTER_MODE_WHITE_NOISE_E,
    BCM85100_TRANSMITTER_MODE_MUTE_E,
    BCM85100_TRANSMITTER_MODE_PRBS_E,
    BCM85100_TRANSMITTER_MODE_STUB_E,
} BCM85100_TRANSMITTER_MODE_ENUM;

typedef enum
{
    BCM85100_RTC_DISABLE_E,
    BCM85100_RTC_MODE_SLAVE_E,
    BCM85100_RTC_MODE_MASTER_E,
    BCM85100_RTC_MODE_AUTO_E,
} BCM85100_RTC_MODE_ENUM;

typedef enum
{
    BCM85100_ADC_OUT_E,
    BCM85100_DAC_OUT_E,
    BCM85100_ED_OUT_E,
    BCM85100_RX_RRC_OUT_E,
} BCM85100_IQ_SAMPLES_SEL_ENUM;

typedef enum
{
    BCM85100_TX_TX_DISABLED_E,
    BCM85100_TX_TX_FAILED_E,
    BCM85100_TX_TX_SUCCESS_E,
} BCM85100_TX_TX_TASK_STATUS_ENUM;

//----------------------------------------------------
// STRUCTS - use ONLY "PACKED" structs
//----------------------------------------------------

#ifdef WIN32
#pragma pack( push, com_structs, 1)
#endif

typedef __PACK_PREFIX__ struct
{
    UINT16              majorVersion;
    BYTE                minorVersion;
    BYTE                buildVersion;
    BYTE                cpuType;
    BYTE                chipVersion;				// BCM85100_CHIP_FLAVORS_ENUM: BCM85100IFSBG, BCM85060HIFSBG, BCM85060LIFSBG or UNKNOWN
    BYTE				reserved[2];
} __PACK__ BCM85100_MODEM_VERSION_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16              majorVersion;
    BYTE                minorVersion;
    BYTE                buildVersion;
} __PACK__ BCM85100_BOOT_LOADER_VERSION_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE                acquireStatus;              // BCM85100_ACQUIRE_STATUS_ENUM: locked, failed, in progress
    BYTE                lastAcquireError;           // BCM85100_ACQUIRE_ERR_ENUM
    INT16               absoluteMseTenths;          // In 0.1 dB steps 0 [tenth dB] … -600 [tenth dB]
    INT16               normalizedMseTenths;        // In 0.1 dB steps 0 [tenth dB] … -600 [tenth dB]
    INT16               radialMseTenths;            // In 0.1 dB steps 0 [tenth dB] … -600 [tenth dB]
    INT16				internalAgc;				// In 0.1 dB steps 48dB to -72dB [480 to -720 tenth dB]
    INT16               externalAgc;                // In 0.1 dB steps 48dB to -72dB [480 to -720 tenth dB]
    INT32               carrierOffset;			    // rx frequency correction ==> FGC 0 , In Hz, normal range [–15e6 to 15e6]
    UINT32              rxSymbolRate;               // In Baud, up to 2000e6
    UINT32              txSymbolRate;               // In Baud, up to 2000e6
    UINT32				ldpcDecoderStress;			// Not Supported
    BYTE                txAcmProfile;               // 0-15
    BYTE                rxAcmProfile;               // 0-15
    BOOLEAN             acmEngineRxSensorsEnabled;	// ACM engine Rx enable\disable
    BOOLEAN             acmEngineTxSwitchEnabled;	// ACM engine Tx enable\disable
    BYTE                debugIndications;           // BCM85100_INDICATIONS_ENUM
    UINT32              resPhNoiseVal;              // in dB^2 steps
    UINT32				RS;							// Not Supported
} __PACK__ BCM85100_MODEM_STATUS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE                acquireStatus;              // BCM85100_ACQUIRE_STATUS_ENUM: locked, failed, in progress
    BYTE                lastAcquireError;           // BCM85100_ACQUIRE_ERR_ENUM
    INT16               normalizedMseTenths;        // In 0.1 dB steps 0 [tenth dB] … -600 [tenth dB]
    INT16				internalAgc;				// In 0.1 dB steps 48dB to -72dB [480 to -720 tenth dB]
    INT16               externalAgc;                // In 0.1 dB steps 48dB to -72dB [480 to -720 tenth dB]
    UINT32				ldpcDecoderStress;			// Not Supported
    BYTE                txAcmProfile;               // 0-15
    BYTE                rxAcmProfile;               // 0-15
    BOOLEAN             acmEngineRxSensorsEnabled;	// ACM engine Rx enable\disable
    INT16               inPower;                    // Input power in tenth dBc from the FS
    INT16               inbandPower;                // Inband power in tenth dBc from the FS
    INT16               AuxAdc_1[16];               // Not Supported
    INT16               AuxAdc_2[16];               // Not Supported
} __PACK__ BCM85100_MODEM_FREQ_STATUS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16               acmRxProfilesHistory;
    UINT16               acmTxProfilesHistory;
} __PACK__ BCM85100_ACM_COUNTERS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT32               lolTimeMsec;                // cumulative, clear on read
    UINT32               numLolEvents;               // cumulative, clear on read
    UINT32               numAcquireTrials;           // cumulative, clear on read
    UINT32               lastLockTimeSymbols;        // in Usec
    UINT32               lastLockTimeBits;           // in Usec
    UINT32               lastLockTimeService;        // in Usec
    UINT32               lastAcquireTimeUsec;        // in Usec
    UINT32               lastTotalAcquireTimeUsec;   // in Usec
} __PACK__ BCM85100_MODEM_ACQUIRE_COUNTERS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    INT16               diffPhaseTenthsDeg;     // in 0.1 deg resolution
    INT16               diffGainTenthsDB;       // in 0.1 dB resolution
    INT16               dcLeakageITenthsPercent;
    INT16               dcLeakageQTenthsPercent;
    INT16               skewInPsec;
} __PACK__ BCM85100_IQ_IMBALANCE_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BCM85100_IQ_IMBALANCE_STRUCT    txTx;
    BCM85100_IQ_IMBALANCE_STRUCT    rxRx;
    BCM85100_IQ_IMBALANCE_STRUCT    rxTx;
} __PACK__ BCM85100_MODEM_IQ_IMBALANCE_STATUS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16	fffTaps[48];			// Not Supported
    UINT32  fffTaps_20Bit[80];      // real0, imag0, real1, imag1, …, real39, imag39.
} __PACK__ BCM85100_MODEM_EQU_STATUS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT32              regVal[17];
    UINT32              wordCnt; //good (fixed) blocks (sum of all the regVal above)
    UINT32              NofBadFecBlocks; //uncorrected FEC blocks
} __PACK__ BCM85100_MODEM_RS_HIST_GET_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT32    ethTx;
    UINT32    ethRx;
    UINT32	  dataTx;
    UINT32    dataRx;
} __PACK__ BCM85100_MODEM_FIFO_LEVELS_GET_STRUCT;

typedef __PACK_PREFIX__ struct
{
    INT16               g1Re;
    INT16               g1Im;
    INT16               g3Re;
    INT16               g3Im;
    INT32               g5Re;
    INT32               g5Im;
} __PACK__ BCM85100_MODEM_PREDISTORTION_STRUCT;

typedef __PACK_PREFIX__ struct
{
    INT16		        signalGain_dB;
    INT16               gainStep;
    BYTE                timeStepMs;
} __PACK__ BCM85100_MODEM_TX_SIGNAL_GAIN_STRUCT;

typedef __PACK_PREFIX__ struct
{
    INT16              mat_Filt1_Coef[16];
    INT16              mat_Filt2_Coef[16];
    INT16              mat_Filt3_Coef[16];
    INT16              mat_Filt4_Coef[16];
} __PACK__ BCM85100_MODEM_RX_MAT_FILTER_STRUCT;

typedef __PACK_PREFIX__ struct
{
    INT16              mat_Filt1_Coef[12];
    INT16              mat_Filt2_Coef[12];
    INT16              mat_Filt3_Coef[12];
    INT16              mat_Filt4_Coef[12];
} __PACK__ BCM85100_MODEM_TX_MAT_FILTER_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE                                 matrix_index;
    BCM85100_MODEM_TX_MAT_FILTER_STRUCT  pTPE_Mat;
} __PACK__ BCM85100_MODEM_TX_FIR_MSG_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT32              acmNumOfSymbols[16];
    UINT32              gpiPayloadSizeInBytes[16];
    BYTE                modulation[16];             // BCM85100_MODULATION_TYPE
    BYTE				Baudrate[16];				// BCM85100_BRC_INDEX_TYPE_ENUM
} __PACK__ BCM85100_ACM_CONFIGURATION_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE                idx;                 // port id
    BYTE                status;              // BCM85100_SGMII_STATUS_ENUM
    UINT64              rxCounters[BCM85100_SGMII_STATUS_TYPE_MAX]; // indexes are in the enum BCM85100_SGMII_STATUS_TYPE_ENUM
    UINT64              txCounters[BCM85100_SGMII_STATUS_TYPE_MAX]; // indexes are in the enum BCM85100_SGMII_STATUS_TYPE_ENUM
    BYTE                anPauseRes;          // autoNegotiation pause advertize BCM85100_PCS_PAUSE_ADV_ENUM
} __PACK__ BCM85100_SGMII_STATUS_GET_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE                idx;                        // index of BCM85100 port
    UINT32              cmdSelect;                  // BCM85100_SGMII_SET_SELECT_ENUM bit field for which commands to do
    BOOLEAN             portEnable;                 // enable or disable port
    BYTE                pcsMode;                    // BCM85100_PCS_MODE_ENUM copper or fiber
    BOOLEAN             fcsProceccingEnable;        // true or false for FCS processing
    BYTE                speed;                      // Not Supported
    BYTE                pauseBehavior;              // BCM85100_SGMII_PAUSE_BEHAVIOR_ENUM choose pause behavior
    UINT16              pauseThresholdHigh;         // pause threshold Low
    UINT16              pauseThresholdLow;          // pause threshold High
    UINT32              pauseTime;                  // the pause time
    BOOLEAN             autoNegotiation;            // autoNegotiation - TRUE or FALSE
    BYTE                autoNegotiationPauseAdv;    // BCM85100_PCS_PAUSE_ADV_ENUM autoNegotiation pause advertize
    BOOLEAN             terminalLoopBack;           // terminal loopback - TRUE or FALSE
    BOOLEAN             lineLoopBack;               // line loopback - TRUE or FALSE
} __PACK__ BCM85100_SGMII_CONFIGURE_STRUCT;

typedef __PACK_PREFIX__ struct                      // bit fields described in BCM85100_ALARM_BITS_ENUM
{
    UINT32              alarms;
} __PACK__ BCM85100_ALARMS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16              msgLen;
    BYTE                message[30];
} __PACK__ BCM85100_MAILBOX_MESSAGE_STRUCT;

typedef __PACK_PREFIX__ struct
{
    INT16               inPower;
    INT16               inbandPower;
} __PACK__ BCM85100_POWER_MANAG_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE                loopbackType;       // see BCM85100_LOOPBACK_TYPE_ENUM
    UINT32              durationMilli;      // 0 = not limited
    BOOLEAN             nearEnd;            // false = far-end
    BYTE                tributary;          // Not Supported
} __PACK__ BCM85100_LOOPBACK_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE                transmitterMode;    // see BCM85100_TRANSMITTER_MODE_ENUM
    INT32               toneFreqKhz;        // relevant for one-tone, two-tone and tone-sweep
    UINT16              sweepTimeMilli;     // relevant for tone-sweep only. Steps are in 1 msec units.
    INT32               toneSecFreqKhz;
    INT16               dcLeakageITenthsPercent; // Desired dcLeakageI, in tenths of percents
    INT16               dcLeakageQTenthsPercent; // Desired dcLeakageQ, in tenths of percents
} __PACK__ BCM85100_TRANSMITTER_MODE_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE                                 matrix_index;
    BCM85100_MODEM_RX_MAT_FILTER_STRUCT  pTPE_Mat;
} __PACK__ BCM85100_MODEM_RX_FIR_MSG_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE     isFinished;
    BYTE     configFileRetVal;
} __PACK__ BCM85100_CONFIG_FILE_STATUS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE     dacIndex;
    UINT16   dacValue;
} __PACK__ BCM85100_AUX_DAC_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16   *adcValue;
} __PACK__ BCM85100_AUX_ADC_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BOOLEAN enableAcmRx;
    BOOLEAN enableAcmTx;
} __PACK__ BCM85100_ACM_ENABLE_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BOOLEAN             rxEnabled;
    BOOLEAN             txEnabled;          //Note that this task cannot work if Tx/Tx Envelope loop is active
    BOOLEAN             rxDcEnabled;
    BOOLEAN             rxGainEnabled;
    BOOLEAN             rxPhaseEnabled;
    BOOLEAN             rxSkewEnabled;
    BOOLEAN             txDcEnabled;        //Enable DC - Update on Tx side and transfer on Rx side
    BOOLEAN             txGainEnabled;      //Enable Gain - Update on Tx side and transfer on Rx side
    BOOLEAN             txPhaseEnabled;     //Enable Phase - Update on Tx side and transfer on Rx side
    BOOLEAN             txSkewEnabled;      //Enable Skew - Update on Tx side and transfer on Rx side - Not in use at the moment
    INT16               txInitDcRe;
    INT16               txInitDcIm;         //%FS (tenths)	 -/+12.5 %	Initial TX DC IQ imbalance
    INT16               txInitGain;         //dB (tenths)	-/+3 dB	Initial TX Gain IQ Imbalance
    INT16               txInitPhase;        //Degree (tenths)	-/+14 deg	Initial TX Phase IQ Imbalance
    INT16               txInitSkew;         //psec	-/+200 ps   	Initial TX Skew IQ Imbalance side - Not in use at the moment
    UINT32              txTimeout;          //msec	1-1e5	TX Timeout
    INT32               rxFreqOffsetLowTh;  //Hz	0-10e3	Frequency Offset Threshold - hystersis
    INT32               rxFreqOffsetHighTh; //Hz	0-10e3	Frequency Offset Threshold - hystersis
    BOOLEAN             txEnvEnabled;       //enable the Tx Envelope task (so disable the txEnabled, see note above)
} __PACK__ BCM85100_RX_TX_API_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE    acqMode;
    BYTE    siMode;
} __PACK__ BCM85100_COM_ACQUIRE_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16      pwmValue;
    BOOLEAN     overwriteFlag;
    BOOLEAN     pwmReset;
} __PACK__ BCM85100_PWM_OVERWRITE_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BOOLEAN     ddsEn;
    BOOLEAN     ppsEn;
    BOOLEAN     clkTransEn;
} __PACK__ BCM85100_EXT_CLOCK_CFG_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16  numIqPairs;
    BYTE    testBusSel;
} __PACK__ BCM85100_COM_SAMPLES_REC_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE    acmProfile;
    BOOLEAN isLocal;
} __PACK__ BCM85100_COM_ACM_SWITCH_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE    dirDegradeImprove;      // BCM85100_ACM_TABLE_ENUM
    BYTE    profile;
    INT16   mseTenthsVal;
    BYTE    criteria;
    BYTE    nextProfile;
} __PACK__ BCM85100_ACM_THRESHOLD_STRUCT;

typedef __PACK_PREFIX__ struct
{
    INT16              pilotSpacing;
    INT16              pilotAmplitude;
    INT16              dataModulation;
    INT16              dataAmplitude;
    INT16              pilotFirst;
} __PACK__ BCM85100_SLICER_IN_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16      offset;
} __PACK__ BCM85100_COM_READ_REGISTER_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16      offset;
    UINT16      value;
} __PACK__ BCM85100_COM_WRITE_REGISTER_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT64 FrameLength;
    UINT64 FrameCount;
    UINT64 FrameErrCount;
    UINT64 ByteCount;
    UINT64 BerAcc;
    UINT64 BerMissed;
} __PACK__ BCM85100_BER_COUNTERS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BOOLEAN                 RtcTaskEn;
    BYTE                    RtcSide;
    BOOLEAN                 RtcLockFlag;
    INT16                   LinkLatency;
    INT16                   LastOffsetCorrection;
} __PACK__ BCM85100_RTC_STATUS_STRUCT;

typedef __PACK_PREFIX__ struct
{
    BYTE                taskStatus; // type of BCM85100_TX_TX_TASK_STATUS_ENUM
    UINT8               offsetDelay;
    UINT16              stbDelay;
    UINT16              edSat;
    UINT16              edRms;
    UINT32              txEnvLatencyTenth;
} __PACK__ BCM85100_ED_STATUS_STRUCT;

//----------------------------------------------------
// Functions
//----------------------------------------------------

#ifdef WIN32
#pragma pack( pop, com_structs )
#endif


//----------------------------------------------------
// Local function prototypes
//----------------------------------------------------
BOOLEAN                  bcm85100IsAlive									(BYTE deviceIndex, UINT32 timeInMs, UINT32 tries);
BCM85100_ERROR_MSG_ENUM  bcm85100FwSendAndJump								(BYTE deviceIndex, BYTE *pBuffer);

//----------------------------------------------------
// Init functions
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_Open							(void);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_Close						(void);

//----------------------------------------------------
// Communication Control Functions
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ComConfigure					(BYTE deviceIndex, BCM85100_COM_MEDIA_ENUM gpMedia, BCM85100_COM_MEDIA_ENUM mcMedia, UINT32 referenceClockInHz);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ComTimeoutSet				(BYTE deviceIndex, UINT32 timeoutInMilli);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ComMessageForward			(BYTE *pReqBuffer, INT32 reqLen, BYTE *pRespBuffer, INT32 *pRespLen);

//----------------------------------------------------
// Boot
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_FirmwareFromHostBoot			(BYTE deviceIndex, BYTE *pBuffer, UINT32 len, BCM85100_DEVICE_RESET_TYPE_ENUM resetType);

//----------------------------------------------------
// Configuration File Functions
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ConfigFileFromHostLoad		(BYTE deviceIndex, BYTE modemIndex, BYTE *pBuffer, UINT16 len);

//----------------------------------------------------
// Status and Performance Monitoring Functions (general)
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_VersionGet					(BYTE deviceIndex, BCM85100_MODEM_VERSION_STRUCT *pVer);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_BootLoaderVersionGet         (BYTE deviceIndex, BCM85100_BOOT_LOADER_VERSION_STRUCT *pVer);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_AlarmEnableGet				(BYTE deviceIndex, BYTE modemIndex, BCM85100_ALARMS_STRUCT *pAlarmsMask);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_AlarmEnableSet				(BYTE deviceIndex, BYTE modemIndex, BCM85100_ALARMS_STRUCT *pAlarmsMask);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_AlarmStatusGet				(BYTE deviceIndex, BYTE modemIndex, BCM85100_ALARMS_STRUCT *pAlarmsStatus);

//----------------------------------------------------
// Status and Performance Monitoring Functions (modem)
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemStatusGet				(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_STATUS_STRUCT *pMs);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_FrequentModemStatusGet       (BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_FREQ_STATUS_STRUCT *pMs);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RtcStatusGet                 (BYTE deviceIndex, BYTE modemIndex, BCM85100_RTC_STATUS_STRUCT *pStat);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_EdStatusGet                  (BYTE deviceIndex, BYTE modemIndex, BCM85100_ED_STATUS_STRUCT *pStat);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_BerCountersGet				(BYTE deviceIndex, BYTE modemIndex, BCM85100_BER_COUNTERS_STRUCT *berCounters);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_AcmCountersGet				(BYTE deviceIndex, BYTE modemIndex, BCM85100_ACM_COUNTERS_STRUCT *pMc);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcquireCountersGet		(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_ACQUIRE_COUNTERS_STRUCT *pAc);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemIqImbalanceStatusGet	(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_IQ_IMBALANCE_STATUS_STRUCT *pIs);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemEqualizerStatusGet		(BYTE deviceIndex, BYTE modemIndex, BOOLEAN xpicSlave, BCM85100_MODEM_EQU_STATUS_STRUCT *pEs);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemRsHistGet				(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_RS_HIST_GET_STRUCT *pRhs);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemFifoLevelsGet			(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_FIFO_LEVELS_GET_STRUCT *pFls);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RxPowerGet					(BYTE deviceIndex, BYTE modemIndex, BCM85100_POWER_MANAG_STRUCT *pPowerM);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_AcmConfigurationGet			(BYTE deviceIndex, BYTE modemIndex, BCM85100_ACM_CONFIGURATION_STRUCT *pCs);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmThresholdGet			(BYTE deviceIndex, BYTE modemIndex, BCM85100_ACM_TABLE_ENUM engineTable, BCM85100_ACM_PROFILE_ENUM profile, INT16 *mseTenthsVal, BYTE *ldpcMantissaTenths, BYTE *ldpcExponent, BCM85100_ACM_CRITERIA_ENUM *criteria, BCM85100_ACM_PROFILE_ENUM *nextProfile);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmProfileMaskGet		(BYTE deviceIndex, BYTE modemIndex, UINT16 *profilesMask);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemSamplesGet              (BYTE deviceIndex, BYTE modemIndex, UINT16 numIqPairs, INT16 *pBuffer, BCM85100_IQ_SAMPLES_SEL_ENUM inputSel);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_LoopbackGet					(BYTE deviceIndex, BYTE modemIndex, BCM85100_LOOPBACK_STRUCT *lb);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_TransmitterModeGet			(BYTE deviceIndex, BYTE modemIndex, BCM85100_TRANSMITTER_MODE_STRUCT *tx);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RxSlicerInGet                (BYTE deviceIndex, BYTE modemIndex, UINT16 NbrIQSamp, INT16 *pBuffer, BCM85100_SLICER_IN_STRUCT *pSlicerIn);
//----------------------------------------------------
//
// Status / Performance Monitoring Functions - networking
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_SgmiiStatusGet               (BYTE deviceIndex, BYTE modemIndex, BCM85100_SGMII_STATUS_GET_STRUCT *pPm);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_SgmiiConfigure				(BYTE deviceIndex, BYTE modemIndex, BCM85100_SGMII_CONFIGURE_STRUCT *config);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_GetRxPacketGfpCrcErrCnt      (BYTE deviceIndex, BYTE modemIndex, UINT16 *pPcktCnt);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_GetTxPacketGfpCrcErrCnt      (BYTE deviceIndex, BYTE modemIndex, UINT16 *pPcktCnt);

//----------------------------------------------------
// Operative Functions (general)
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_DeviceReset					(BYTE deviceIndex, BCM85100_DEVICE_RESET_TYPE_ENUM resetType);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_TxRxOperationStart			(BYTE deviceIndex, BYTE modemIndex);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_LoopbackSet					(BYTE deviceIndex, BYTE modemIndex, BCM85100_LOOPBACK_STRUCT *lb);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_TransmitterModeSet			(BYTE deviceIndex, BYTE modemIndex, BCM85100_TRANSMITTER_MODE_STRUCT *tx);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RxOfTxOverTheAirParamsSet    (BYTE deviceIndex, BYTE modemIndex, BCM85100_RX_TX_API_STRUCT *tx);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RxOfTxOverTheAirParamsGet	(BYTE deviceIndex, BYTE modemIndex, BCM85100_RX_TX_API_STRUCT *rx);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RtcModeSet                   (BYTE deviceIndex, BYTE modemIndex, BCM85100_RTC_MODE_ENUM mode);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_GeneralControlSet            (BYTE deviceIndex, BYTE modemIndex, UINT32 opcode, BYTE length, BYTE *data);

//----------------------------------------------------
// Operative Functions (Modem)
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcquireStart			(BYTE deviceIndex, BYTE modemIndex, BCM85100_ACQUIRE_MODE_ENUM  acqMode, BCM85100_SPECTRAL_INVERSION_MODE_ENUM siMode);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ExtPwmOverwrite              (BYTE deviceIndex, BYTE modemIndex, BCM85100_PWM_OVERWRITE_STRUCT pwmSt);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ExtClockConfig               (BYTE deviceIndex, BYTE modemIndex, BCM85100_EXT_CLOCK_CFG_STRUCT extClk);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_SetPrbsMode                  (BYTE deviceIndex, BYTE modemIndex);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_BerMonitorEnable             (BYTE deviceIndex, BYTE modemIndex, BOOLEAN enableMonitor);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_BerCountersClear             (BYTE deviceIndex, BYTE modemIndex);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemPredistortionCoefsSet	(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_PREDISTORTION_STRUCT *pPs);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemIqImbalanceTxTxSet		(BYTE deviceIndex, BYTE modemIndex, INT16 diffPhaseTenthsDeg, INT16 diffGainTenthsDB, INT16 dcLeakageITenthsPercent, INT16 dcLeakageQTenthsPercent);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RxFreqOffsetSet				(BYTE deviceIndex, BYTE modemIndex, INT32 offsetInHz);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemTxSignalGainSet			(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_TX_SIGNAL_GAIN_STRUCT *pRhs);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_TxFreqOffsetSet				(BYTE deviceIndex, BYTE modemIndex, INT32 offsetInHz);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemRxMatFilterStatusSet    (BYTE deviceIndex, BYTE modemIndex, BYTE matrixIndex, BCM85100_MODEM_RX_MAT_FILTER_STRUCT *pTPE_Mat);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemTxMatFilterStatusSet    (BYTE deviceIndex, BYTE modemIndex, BYTE matrixIndex, BCM85100_MODEM_TX_MAT_FILTER_STRUCT *pTPE_Mat);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RxSkewSet					(BYTE deviceIndex, BYTE modemIndex, INT16 skew);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_TxSkewSet					(BYTE deviceIndex, BYTE modemIndex, INT16 skew);

//----------------------------------------------------
// Operative Functions (ACM)
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmEngineEnable			(BYTE deviceIndex, BYTE modemIndex, BOOLEAN enableRxSensors, BOOLEAN enableTxSwitch);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmSwitch				(BYTE deviceIndex, BYTE modemIndex, BCM85100_ACM_PROFILE_ENUM acmProfile, BOOLEAN isLocal);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmProfileEnable		(BYTE deviceIndex, BYTE modemIndex, UINT16 profilesMask);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmThresholdSet			(BYTE deviceIndex, BYTE modemIndex, BCM85100_ACM_TABLE_ENUM engineTable, BCM85100_ACM_PROFILE_ENUM profile, INT16 mseTenthsVal, BYTE ldpcMantissaTenths, BYTE ldpcExponent, BCM85100_ACM_CRITERIA_ENUM criteria, BCM85100_ACM_PROFILE_ENUM nextProfile);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmImproveIfNeeded		(BYTE deviceIndex, BYTE modemIndex);

//----------------------------------------------------
// Operative Functions (networking)
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_MailboxMessageSend			(BYTE deviceIndex, BYTE modemIndex, BCM85100_MAILBOX_MESSAGE_STRUCT *pMbx);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_MailboxMessageReceive		(BYTE deviceIndex, BYTE modemIndex, BCM85100_MAILBOX_MESSAGE_STRUCT *pMbx);

//----------------------------------------------------
// Operative Functions (Auxiliary)
//----------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_AuxDacSet					(BYTE deviceIndex, BYTE modemIndex, BYTE dacIndex, UINT16  dacValue);

//----------------------------------------------------
// Direct Setup Functions
//---------------------------------------------------
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RegisterRead                 (BYTE deviceIndex, BYTE modemIndex, UINT16 offset, UINT16 *pVal);
DLLEXPORT BCM85100_ERROR_MSG_ENUM BCM85100_API_RegisterWrite                (BYTE deviceIndex, BYTE modemIndex, UINT16 offset, UINT16 val);

/////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
} //close extern "C"
#endif

#endif /* _BCM85100_API_H_ */

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

