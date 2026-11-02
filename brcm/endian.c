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

#define ENDIAN_C

#include "string.h"
#include "endian.h"
#include "utils.h"


// macros for swap request and response functions.
// used for switching UINT64,UINT32,UINT16 and BYTES var's bytes order.
// the macros also increment the buffer pointer by the number of bytes of the var type
#define BCM85100_SWAP_QWORDS(N)              bcm85100SwapQwords(pMsgBuffer, N);  pMsgBuffer += N*sizeof(UINT64);
#define BCM85100_SWAP_DWORDS(N)              bcm85100SwapDwords(pMsgBuffer, N);  pMsgBuffer += N*sizeof(UINT32);
#define BCM85100_SWAP_WORDS(N)               bcm85100SwapWords(pMsgBuffer, N);   pMsgBuffer += N*sizeof(UINT16);
#define BCM85100_ALL_DWORDS_FROM_NOW_ON()    bcm85100AllDwordsFromNowOn(pMsgBuffer, pBufferAddressEnd);
#define BCM85100_SKIP_BYTES(N)               pMsgBuffer += N;
#define BCM85100_ALL_WORDS_FROM_NOW_ON()     bcm85100AllWordsFromNowOn(pMsgBuffer, pBufferAddressEnd);


BOOLEAN gEndianConversion = FALSE;

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
static void bcm85100SwapBytes(BYTE *ptr, int offset1, int offset2)
{
    BYTE temp;
    temp = ptr[offset1];
    ptr[offset1] = ptr[offset2];
    ptr[offset2] = temp;
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
static void bcm85100SwapWordBytes(BYTE *ptr)
{
    bcm85100SwapBytes(ptr, 0, 1);
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
static void bcm85100SwapQwordBytes(BYTE *ptr)
{
    bcm85100SwapBytes(ptr, 0, 7);
    bcm85100SwapBytes(ptr, 1, 6);
    bcm85100SwapBytes(ptr, 2, 5);
    bcm85100SwapBytes(ptr, 3, 4);
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
static void bcm85100SwapDwordBytes(BYTE *ptr)
{
    bcm85100SwapBytes(ptr, 0, 3);
    bcm85100SwapBytes(ptr, 1, 2);
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
static void bcm85100SwapWords(BYTE* ptr, int numWords)
{
    int ix;

    for (ix = numWords; ix > 0; ix--)
    {
        bcm85100SwapWordBytes(ptr);
        ptr += sizeof(UINT16);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
static void bcm85100SwapDwords(BYTE* ptr, int numDwords)
{
    int ix;

    for (ix = numDwords; ix > 0; ix--)
    {
        bcm85100SwapDwordBytes(ptr);
        ptr += sizeof(UINT32);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
static void bcm85100SwapQwords(BYTE* ptr, int numQwords)
{
    int ix;

    for (ix = numQwords; ix > 0; ix--)
    {
        bcm85100SwapQwordBytes(ptr);
        ptr += sizeof(UINT64);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
static void bcm85100AllWordsFromNowOn(BYTE *ptr, BYTE *endPtr)
{
    while (ptr < endPtr)
    {
        bcm85100SwapWordBytes(ptr);
        ptr += sizeof(UINT16);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
static void bcm85100AllDwordsFromNowOn(BYTE *ptr, BYTE *endPtr)
{
    while (ptr < endPtr)
    {
        bcm85100SwapDwordBytes(ptr);
        ptr += sizeof(UINT32);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Configure endian conversion
///////////////////////////////////////////////////////////////////////////////
void bcm85100EndianConversionConfigure(BOOLEAN enabled)
{
    gEndianConversion = enabled;
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
void bcm85100RequestParamSwap(BCM85100_COM_OPCODE_ENUM opcode, BYTE *pMsgBuffer, UINT32 msgBufferLen)
{
    BYTE *pBufferAddressEnd = pMsgBuffer + msgBufferLen;

    if (gEndianConversion == FALSE)
    {
        return;
    }

    switch (opcode)
    {
    case BCM85100_COM_JUMP_TO_FW_MSG_OPCODE_E:
    case BCM85100_COM_VERSION_GET_OPCODE_E:
    case BCM85100_COM_BOOT_LOADER_VERSION_GET_OPCODE_E:
    case BCM85100_COM_LOAD_CONFIG_FILE_OPCODE_E:
    case BCM85100_COM_RUN_CONFIG_FILE_OPCODE_E:
    case BCM85100_COM_CONFIG_FILE_GET_DONE_OPCODE_E:
    case BCM85100_COM_ALARM_ENABLE_GET_OPCODE_E:
    case BCM85100_COM_ALARM_STATUS_GET_OPCODE_E:
    case BCM85100_COM_MC_RESET_OPCODE_E:
    case BCM85100_COM_ACQUIRE_START_OPCODE_E:
    case BCM85100_COM_ACM_ENGINE_ENABLE_OPCODE_E:
    case BCM85100_COM_BER_MONITOR_ENABLE_OPCODE_E:
    case BCM85100_COM_BER_COUNTERS_CLEAR_OPCODE_E:
    case BCM85100_COM_ACM_SWITCH_OPCODE_E:
    case BCM85100_COM_MAILBOX_MESSAGE_RECEIVE_OPCODE_E:
    case BCM85100_COM_MODEM_STATUS_GET_OPCODE_E:
    case BCM85100_COM_MODEM_ED_STATUS_GET_OPCODE_E:
    case BCM85100_COM_RTC_STATUS_GET_OPCODE_E:
    case BCM85100_COM_MODEM_FREQ_STATUS_GET_OPCODE_E:
    case BCM85100_COM_BER_COUNTERS_GET_OPCODE_E:
    case BCM85100_COM_MODEM_RX_POWER_GET_OPCODE_E:
    case BCM85100_COM_MODEM_ACQUIRE_COUNTERS_GET_OPCODE_E:
    case BCM85100_COM_MODEM_IQ_IMBALANCE_STATUS_GET_OPCODE_E:
    case BCM85100_COM_MODEM_EQUALIZER_STATUS_GET0_OPCODE_E:
    case BCM85100_COM_MODEM_EQUALIZER_STATUS_GET1_OPCODE_E:
    case BCM85100_COM_MODEM_RS_HIST_GET_OPCODE_E:
    case BCM85100_COM_MODEM_FIFO_LEVELS_GET_OPCODE_E:

    case BCM85100_COM_ACM_COUNTERS_GET_OPCODE_E:
    case BCM85100_COM_MODEM_LOOPBACK_GET_OPCODE_E:
    case BCM85100_COM_TRANSMITTER_MODE_GET_OPCODE_E:
    case BCM85100_COM_NET_ETH_PM_GET_OPCODE_E:
    case BCM85100_COM_MODEM_RX_PCKT_GFP_CRC_ERR_GET_OPCODE_E:
    case BCM85100_COM_MODEM_TX_PCKT_GFP_CRC_ERR_GET_OPCODE_E:
    case BCM85100_COM_BOOT_HEADER_MSG_OPCODE_E:
    case BCM85100_COM_BOOT_DATA_MSG_OPCODE_E:
    case BCM85100_COM_BOOT_CHECKSUM_MSG_OPCODE_E:
    case BCM85100_COM_ACM_CONFIGURATION_GET_OPCODE_E:
    case BCM85100_COM_TX_RX_START_OPCODE_E:
    case BCM85100_COM_ACM_IMPROVE_OPCODE_E:
    case BCM85100_COM_ACM_PROFILE_MASK_GET_OPCODE_E:
    case BCM85100_COM_RTC_MODE_SET_E:
    case BCM85100_COM_MODEM_SLICER_INFO_GET_OPCODE_E:
    case BCM85100_COM_EXT_CLOCK_CFG_OPCODE_E:
    case BCM85100_COM_PRBS_MODE_SET_OPCODE_E:
        // no swapping
        break;

    case BCM85100_COM_PWM_OVERWRITE_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_NET_ETH_PM_CONFIG_OPCODE_E:
        BCM85100_SKIP_BYTES(1);
        BCM85100_SWAP_DWORDS(1);
        BCM85100_SKIP_BYTES(5);
        BCM85100_SWAP_WORDS(2);
        BCM85100_SWAP_DWORDS(1);
        break;

    case BCM85100_COM_ALARM_ENABLE_SET_OPCODE_E:
        BCM85100_SWAP_DWORDS(1);
        break;

    case BCM85100_COM_LOOPBACK_SET_OPCODE_E:
        BCM85100_SKIP_BYTES(1);
        BCM85100_SWAP_DWORDS(1);
        break;

    case BCM85100_COM_TRANSMITTER_MODE_SET_OPCODE_E:
        BCM85100_SKIP_BYTES(1);
        BCM85100_SWAP_DWORDS(1);
        BCM85100_SWAP_WORDS(1);
        BCM85100_SWAP_DWORDS(1);
        BCM85100_SWAP_WORDS(2);
        break;

    case BCM85100_COM_ACM_PROFILE_ENABLE_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MAILBOX_MESSAGE_SEND_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_RX_TX_SET_E:
        BCM85100_SKIP_BYTES(10);
        BCM85100_SWAP_WORDS(5);
        BCM85100_SWAP_DWORDS(3);
        break;

    case BCM85100_COM_RX_TX_GET_E:
        break;

    case BCM85100_COM_ACM_THRESHOLD_SET_E:
        BCM85100_SKIP_BYTES(2);
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_ACM_THRESHOLD_GET_E:
        BCM85100_SKIP_BYTES(2);
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_RX_FREQ_OFFSET_SET_OPCODE_E:
    case BCM85100_COM_MODEM_TX_FREQ_OFFSET_SET_OPCODE_E:
        BCM85100_SWAP_DWORDS(1);
        break;

    case BCM85100_COM_MODEM_PREDISTORTION_COEFS_SET_OPCODE_E:
        BCM85100_SWAP_WORDS(4);
        BCM85100_SWAP_DWORDS(2);
        break;

    case BCM85100_COM_MODEM_SIGNAL_GAIN_SET_OPCODE_E:
        BCM85100_SWAP_WORDS(2);
        break;

    case BCM85100_COM_MODEM_IQ_IMBALANCE_TXTX_SET_OPCODE_E:
        BCM85100_ALL_WORDS_FROM_NOW_ON();
        break;

    case BCM85100_COM_MODEM_RX_SKEW_SET_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_TX_SKEW_SET_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_RX_FIR_COEFF_SET_OPCODE_E:
        BCM85100_SKIP_BYTES(1);
        BCM85100_SWAP_WORDS(64);
        break;

    case BCM85100_COM_MODEM_TX_FIR_COEFF_SET_OPCODE_E:
        BCM85100_SKIP_BYTES(1);
        BCM85100_SWAP_WORDS(48);
        break;

    case BCM85100_AUX_DAC_SET_E:
        BCM85100_SKIP_BYTES(1);
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_SAMPLES_REC_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_SLICER_REC_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_SAMPLES_GET_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_SLICER_SAMPLES_GET_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_GENERAL_CONTROL_SET_OPCODE_E:
        BCM85100_SWAP_WORDS(2);
        break;

    case BCM85100_COM_REGISTER_READ_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_REGISTER_WRITE_OPCODE_E:
        BCM85100_SWAP_WORDS(2);
        break;

    default:
        break;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
void bcm85100ResponseParamSwap(BCM85100_COM_OPCODE_ENUM opcode, BYTE *pMsgBuffer, UINT32 msgBufferLen)
{
    BYTE *pBufferAddressEnd = pMsgBuffer + msgBufferLen;

    if (gEndianConversion == FALSE)
    {
        return;
    }

    switch (opcode)
    {
    case BCM85100_COM_JUMP_TO_FW_MSG_OPCODE_E:
    case BCM85100_COM_BOOT_HEADER_MSG_OPCODE_E:
    case BCM85100_COM_BOOT_DATA_MSG_OPCODE_E:
    case BCM85100_COM_BOOT_CHECKSUM_MSG_OPCODE_E:
    case BCM85100_COM_LOAD_CONFIG_FILE_OPCODE_E:
    case BCM85100_COM_RUN_CONFIG_FILE_OPCODE_E:
    case BCM85100_COM_CONFIG_FILE_GET_DONE_OPCODE_E:
    case BCM85100_COM_ALARM_ENABLE_SET_OPCODE_E:
    case BCM85100_COM_MC_RESET_OPCODE_E:
    case BCM85100_COM_LOOPBACK_SET_OPCODE_E:
    case BCM85100_COM_TRANSMITTER_MODE_SET_OPCODE_E:
    case BCM85100_COM_ACQUIRE_START_OPCODE_E:
    case BCM85100_COM_PWM_OVERWRITE_OPCODE_E:
    case BCM85100_COM_ACM_ENGINE_ENABLE_OPCODE_E:
    case BCM85100_COM_BER_COUNTERS_CLEAR_OPCODE_E:
    case BCM85100_COM_BER_MONITOR_ENABLE_OPCODE_E:
    case BCM85100_COM_ACM_THRESHOLD_SET_E:
    case BCM85100_COM_ACM_PROFILE_ENABLE_OPCODE_E:
    case BCM85100_COM_ACM_SWITCH_OPCODE_E:
    case BCM85100_COM_MAILBOX_MESSAGE_SEND_OPCODE_E:
    case BCM85100_COM_MODEM_IQ_IMBALANCE_TXTX_SET_OPCODE_E:
    case BCM85100_COM_MODEM_RX_SKEW_SET_OPCODE_E:
    case BCM85100_COM_MODEM_TX_SKEW_SET_OPCODE_E:
    case BCM85100_COM_MODEM_RX_FREQ_OFFSET_SET_OPCODE_E:
    case BCM85100_COM_MODEM_TX_FREQ_OFFSET_SET_OPCODE_E:
    case BCM85100_COM_MODEM_PREDISTORTION_COEFS_SET_OPCODE_E:
    case BCM85100_COM_MODEM_SIGNAL_GAIN_SET_OPCODE_E:
    case BCM85100_COM_TX_RX_START_OPCODE_E:
    case BCM85100_COM_ACM_IMPROVE_OPCODE_E:
    case BCM85100_COM_MODEM_RX_FIR_COEFF_SET_OPCODE_E:
    case BCM85100_COM_MODEM_TX_FIR_COEFF_SET_OPCODE_E:
    case BCM85100_COM_NET_ETH_PM_CONFIG_OPCODE_E:
    case BCM85100_COM_RX_TX_SET_E:
    case BCM85100_COM_RTC_MODE_SET_E:
    case BCM85100_COM_MODEM_SAMPLES_REC_OPCODE_E:
    case BCM85100_COM_MODEM_SLICER_REC_OPCODE_E:
    case BCM85100_COM_GENERAL_CONTROL_SET_OPCODE_E:
    case BCM85100_COM_REGISTER_WRITE_OPCODE_E:
    case BCM85100_COM_EXT_CLOCK_CFG_OPCODE_E:
    case BCM85100_COM_PRBS_MODE_SET_OPCODE_E:
        // no swapping
        break;

    case BCM85100_COM_VERSION_GET_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_BOOT_LOADER_VERSION_GET_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_ACM_PROFILE_MASK_GET_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_ACM_CONFIGURATION_GET_OPCODE_E:
        BCM85100_SWAP_DWORDS(32);
        break;

    case BCM85100_COM_ALARM_ENABLE_GET_OPCODE_E:
        BCM85100_SWAP_DWORDS(1);
        break;

    case BCM85100_COM_ALARM_STATUS_GET_OPCODE_E:
        BCM85100_SWAP_DWORDS(1);
        break;

    case BCM85100_COM_MODEM_STATUS_GET_OPCODE_E:
        BCM85100_SKIP_BYTES(2);
        BCM85100_SWAP_WORDS(5);
        BCM85100_SWAP_DWORDS(4);
        BCM85100_SKIP_BYTES(5);
        BCM85100_SWAP_DWORDS(2);
        break;

    case BCM85100_COM_RTC_STATUS_GET_OPCODE_E:
        BCM85100_SKIP_BYTES(3);
        BCM85100_SWAP_WORDS(2);
        break;

    case BCM85100_COM_MODEM_FREQ_STATUS_GET_OPCODE_E:
        BCM85100_SKIP_BYTES(2);
        BCM85100_SWAP_WORDS(3);
        BCM85100_SWAP_DWORDS(1);
        BCM85100_SKIP_BYTES(3);
        BCM85100_SWAP_WORDS(34);
        break;

    case BCM85100_COM_MODEM_ED_STATUS_GET_OPCODE_E:
        BCM85100_SKIP_BYTES(2);
        BCM85100_SWAP_WORDS(3);
        BCM85100_SWAP_DWORDS(1);
        break;

    case BCM85100_COM_BER_COUNTERS_GET_OPCODE_E:
        BCM85100_SWAP_QWORDS(6);
        break;

    case BCM85100_COM_MODEM_RX_POWER_GET_OPCODE_E:
        BCM85100_SWAP_WORDS(2);
        break;

    case BCM85100_COM_MODEM_RS_HIST_GET_OPCODE_E:
        BCM85100_SWAP_DWORDS(19);
        break;

    case BCM85100_COM_MODEM_FIFO_LEVELS_GET_OPCODE_E:
        BCM85100_SWAP_DWORDS(4);
        break;

    case BCM85100_COM_MAILBOX_MESSAGE_RECEIVE_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_ACQUIRE_COUNTERS_GET_OPCODE_E:
        BCM85100_ALL_DWORDS_FROM_NOW_ON();
        break;

    case BCM85100_COM_MODEM_IQ_IMBALANCE_STATUS_GET_OPCODE_E:
        BCM85100_ALL_WORDS_FROM_NOW_ON();
        break;

    case BCM85100_COM_MODEM_EQUALIZER_STATUS_GET0_OPCODE_E:
        BCM85100_SWAP_DWORDS(40);
        break;

    case BCM85100_COM_MODEM_EQUALIZER_STATUS_GET1_OPCODE_E:
        BCM85100_SWAP_DWORDS(40);
        break;

    case BCM85100_COM_ACM_COUNTERS_GET_OPCODE_E:
        BCM85100_SWAP_WORDS(2);
        break;

    case BCM85100_COM_MODEM_LOOPBACK_GET_OPCODE_E:
        BCM85100_SKIP_BYTES(1);
        BCM85100_SWAP_DWORDS(1);
        break;

    case BCM85100_COM_TRANSMITTER_MODE_GET_OPCODE_E:
        BCM85100_SKIP_BYTES(1);
        BCM85100_SWAP_DWORDS(1);
        BCM85100_SWAP_WORDS(1);
        BCM85100_SWAP_DWORDS(1);
        BCM85100_SWAP_WORDS(2);
        break;

    case BCM85100_COM_NET_ETH_PM_GET_OPCODE_E:
        BCM85100_SKIP_BYTES(2);
        BCM85100_SWAP_QWORDS(28); //BCM85100_SGMII_STATUS_TYPE_MAX * 2
        break;

    case BCM85100_COM_MODEM_RX_PCKT_GFP_CRC_ERR_GET_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_TX_PCKT_GFP_CRC_ERR_GET_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_RX_TX_GET_E:
        BCM85100_SKIP_BYTES(10);
        BCM85100_SWAP_WORDS(5);
        BCM85100_SWAP_DWORDS(3);
        break;

    case BCM85100_COM_ACM_THRESHOLD_GET_E:
        BCM85100_SKIP_BYTES(2);
        BCM85100_SWAP_WORDS(1);
        break;

    case BCM85100_COM_MODEM_SAMPLES_GET_OPCODE_E:
        BCM85100_ALL_WORDS_FROM_NOW_ON();
        break;

    case BCM85100_COM_MODEM_SLICER_SAMPLES_GET_OPCODE_E:
        BCM85100_ALL_WORDS_FROM_NOW_ON();
        break;

    case BCM85100_COM_MODEM_SLICER_INFO_GET_OPCODE_E:
        BCM85100_ALL_WORDS_FROM_NOW_ON();
        break;

    case BCM85100_COM_REGISTER_READ_OPCODE_E:
        BCM85100_SWAP_WORDS(1);
        break;

    default:
        break;

    }
}

