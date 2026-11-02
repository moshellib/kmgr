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

#  ifndef HOST_PROTOCOL_STRUCTS_H

#define HOST_PROTOCOL_STRUCTS_H

#include "defs85100.h"
#include "utils.h"

#define BCM85100_BOOT_FW_SIZE_BYTES                   (16 * 1024)
#define BCM85100_HOST_PROTOCOL_MAX_MSG_LEN            256
#define BCM85100_HOST_PROTOCOL_HEADER_LEN             8
#define BCM85100_HOST_PROTOCOL_MAX_PAYLOAD_LEN        (BCM85100_HOST_PROTOCOL_MAX_MSG_LEN - BCM85100_HOST_PROTOCOL_HEADER_LEN - BCM85100_CHECKSUM_LEN)
#define BCM85100_HOST_PROTOCOL_PAYLOAD_OFFSET         (BCM85100_HOST_PROTOCOL_HEADER_LEN)

//max size of the record buffer within the config file
#define BCM85100_MAX_CONFIG_FILE_LEN                   (28 * 1024) // config file max size = 28KB

///////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////
#define BCM85100_COM_REGISTER_RANGE_HEADER_SIZE       5
#define BCM85100_COM_MULTIPLE_REGISTERS_HEADER_SIZE   2
#define BCM85100_COM_DWORD_FIFO_HEADER_SIZE           6
#define BCM85100_COM_GENERAL_CONTROL_HEADER_SIZE      5

// host protocol header structure
#define BCM85100_ROUTING_INFO_PERIF_OFFSET			(0x0)
#define BCM85100_ROUTING_INFO_PORT_OFFSET			(0x1)
#define BCM85100_SPARE1_INFO_OFFSET                 (0x2)
#define BCM85100_SPARE2_INFO_OFFSET                 (0x3)
#define BCM85100_OPCODE_OFFSET                      (0x4)
#define BCM85100_FLAGS_OFFSET                       (BCM85100_OPCODE_OFFSET + 0x1) // 0x0F = seqnum, 0x10 = MC=0, 0x20 = int=0x20/ext=0, 0xc0 = modem index
#define BCM85100_ACK_CODE_OFFSET                    (BCM85100_OPCODE_OFFSET + 0x2)
#define BCM85100_MSG_LEN_OFFSET                     (BCM85100_OPCODE_OFFSET + 0x3)
#define BCM85100_MSG_HEADER_LEN                     (BCM85100_OPCODE_OFFSET + 0x4)

// host protocol "flags" field structure
#define BCM85100_FLAGS_SEQ_NUM_BIT                  (0x0)
#define BCM85100_FLAGS_SEQ_NUM_MASK                 (0xF)
#define BCM85100_FLAGS_CPU_DEST_BIT                 (0x4)
#define BCM85100_FLAGS_CPU_DEST_MASK                (0x1)
#define BCM85100_FLAGS_INTERNAL_BIT                 (0x5)
#define BCM85100_FLAGS_INTERNAL_MASK                (0x1)
#define BCM85100_FLAGS_MODEM_INDEX_BIT              (0x6)
#define BCM85100_FLAGS_MODEM_INDEX_MASK             (0x3)

// IP header structure
#define BCM85100_IP_PROTOCOL_HEADER_LEN             (4)
#define BCM85100_IP_PROTOCOL_SEQNUM_OFFSET          (0)
#define BCM85100_IP_PROTOCOL_DEVICE_ID_OFFSET       (1)
#define BCM85100_IP_PROTOCOL_GW_ERROR_OFFSET        (2)
#define BCM85100_IP_PROTOCOL_MODE_OFFSET            (3)


///////////////////////////////////////////////////////////////////////////////
// ENUMS
///////////////////////////////////////////////////////////////////////////////
typedef enum
{
    BCM85100_COM_CONFIG_FILE_START_E,
    BCM85100_COM_CONFIG_FILE_MIDDLE_E,
    BCM85100_COM_CONFIG_FILE_END_E
} BCM85100_COM_CFG_FILE_POSITION_ENUM;

///////////////////////////////////////////////////////////////////////////////
// STRUCTS - use ONLY "PACKED" structs
///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
    #pragma pack( push, com_structs, 1)
#endif

typedef __PACK_PREFIX__ struct
{
    BYTE    position; // start/middle/end
    BYTE    pBuffer[BCM85100_HOST_PROTOCOL_MAX_PAYLOAD_LEN-1];
} __PACK__ BCM85100_COM_CFG_FILE_CHUNK_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT32  address;
    UINT16  index;
    UINT16  size;
} __PACK__ BCM85100_BOOT_DATA_RECORD_HEADER_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT32  version;
    UINT32  jumpAddress;
    UINT32  totalLength;
    UINT32  totalBytesInImage;
    UINT32  ImageCheckSum;
} __PACK__ BCM85100_BOOT_HEADER_MSG_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT32      checksum;
    UINT16      totalLength;
    UINT16      versionMajor;   //620 comp (was reserved)
    BYTE        versionMinor;
    BYTE        versionBuild;
    UINT32      creationTime;
    BYTE        versionPackage;
    BYTE        reserved[4];    //window ready
} __PACK__ BCM85100_MODEM_CONFIG_FILE_HEADER_STRUCT;

typedef __PACK_PREFIX__ struct
{
    UINT16 opcode;
    UINT16 length;
    BYTE   pDataBuffer [BCM85100_HOST_PROTOCOL_MAX_PAYLOAD_LEN - BCM85100_COM_GENERAL_CONTROL_HEADER_SIZE]; // substruct the size of the previous params
} __PACK__ BCM85100_COM_GENERAL_CONTROL_STRUCT;

    #ifdef WIN32
        #pragma pack( pop, com_structs )
    #endif

#endif

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////



