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


#ifndef HOST_COMM_LAYER_H
#define HOST_COMM_LAYER_H

#include "defs85100.h"
#include "bcm85100_api.h"
#include "hostProtocolOpcodes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BCM85100_SPI_BOOTFW                           1  //  bootfw mode don't need tom pad 
#define BCM85100_SPI_NO_BOOTFW                        2
#define BCM85100_SPI_MAX_RATE                         4000000 //2MHZ
#define BCM85100_SPI_BOOT_RATE                        2000000 //2MHZ

///////////////////////////////////////////////////////////////////////////////
// Function Prototypes
///////////////////////////////////////////////////////////////////////////////
// open the comm layer - opens all threads, events, semaphores needed, etc.
BCM85100_ERROR_MSG_ENUM bcm85100HostCommOpen(void);

// close whatever's been openned
BCM85100_ERROR_MSG_ENUM bcm85100HostCommClose(void);

// complete "build, send and recieve" including building message from payload, endian swapping, etc.
// used mainly for local APIs
BCM85100_ERROR_MSG_ENUM bcm85100HostCommBuildSendAndReceive(BYTE                       deviceIndex,             // to which device to send
        BYTE                       *pTxPayloadBuff,        // payload to send (not including header or checksum)
        UINT32                     txPayloadLen,           // payload length
        BCM85100_COM_OPCODE_ENUM   opcode,                 // opcode (per MC)
        BYTE                       modemIndex,             // to which modem (N/A to BCM85100)
        BYTE                       *pRxPayloadBuff,        // Where to store the received payload (length assumed to be suffient)
        UINT32                     *pActualRxLen,          // Where to indicate actual received payload length
        BCM85100_ERROR_MSG_ENUM    *pStatus);               // Where to place the status

BCM85100_ERROR_MSG_ENUM bcm85100HostBootFwCommBuildSendAndReceive(BYTE                       deviceIndex,             // to which device to send
        BYTE                       *pTxPayloadBuff,        // payload to send (not including header or checksum)
        UINT32                     txPayloadLen,           // payload length
        BCM85100_COM_OPCODE_ENUM   opcode,                 // opcode (per MC)
        BYTE                       modemIndex,             // to which modem (N/A to BCM85100)
        BYTE                       *pRxPayloadBuff,        // Where to store the received payload (length assumed to be suffient)
        UINT32                     *pActualRxLen,          // Where to indicate actual received payload length
        BCM85100_ERROR_MSG_ENUM    *pStatus);               // Where to place the status
// Partial "build, send and recieve" including building message from payload, endian swapping, etc.
// response is never expected, not only to the user, but also to the hostComm layer
// used only in boot / upgrade scenarios
BCM85100_ERROR_MSG_ENUM bcm85100HostCommBuildSend(BYTE                       deviceIndex,
        BYTE                       *pTxPayloadBuff,
        UINT32                     txPayloadLen,
        BCM85100_COM_OPCODE_ENUM   opcode,
        BYTE                       modemIndex,
        BYTE                      bootFwMode);


void bcm85100HostCommEnableMessageForwarding(BYTE                       deviceIndex,
        BOOLEAN                    enable);

// Raw data access. Must run in exclusive mode. Used in boot scenario only
BCM85100_ERROR_MSG_ENUM bcm85100HostCommRawDataSend(BYTE                       deviceIndex,
        BYTE                       *pBuff,
        UINT32                     buffLen);

// Modify / override user's specified API timeout.
// Used in boot / upgrade for quick "is alive" polling
UINT32 bcm85100HostCommModifyRxTimeout(BYTE                       deviceIndex,
                                       UINT32                     newRxTimeout);

// Flush recevier and reset its state
// Used in boot scenarios
void bcm85100HostCommRxFlush(BYTE                       deviceIndex);


// IP forwarding of remote client into comm layer
BCM85100_ERROR_MSG_ENUM bcm85100MessageForward(BYTE                       deviceIndex,
        BYTE                       *pReqBuffer,
        UINT32                     reqLen,
        BYTE                       *pRespBuffer,
        UINT32                     *pRespLen);

UINT32 	bcm85100HostTimeGet(UINT32 startTime);


#ifdef __cplusplus
} //close extern "C"
#endif


#endif /* HOST_COMM_LAYER_H */


