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


#include "string.h"
#include "stdio.h"

#include "hostCommLayer.h"
#include "hostDriver.h"
#include "endian.h"
#include "diagnostics.h"

///////////////////////////////////////////////////////////////////////////////
// COM Layer DEFINES
///////////////////////////////////////////////////////////////////////////////
#undef HOST_PROTOCOL_TX_LOG 
//#define HOST_PROTOCOL_TX_LOG

#ifdef HOST_PROTOCOL_TX_LOG
    FILE *pfHostMsg = NULL;

    #define BCM85100_HOST_MSG_FILE_NAME  "hostProtocol.log"
#endif

#define BCM85100_DEFAULT_TIMEOUT_MILLI                1000

#define BCM85100_SPI_RX_MSG_SMALLEST_SIZE             9
#define BCM85100_SPI_RX_MSG_ERR_TH_SIZE               9
#define BCM85100_SPI_RX_MSG_LARGEST_SIZE              350
#define BCM85100_SPI_ALIGN_LEN                        16
#define BCM85100_SPI_ALIGN_MASK                       0xFFF0
#define BCM85100_SPI_IDLE_FLAGS_NUMBER                8

///////////////////////////////////////////////////////////////////////////////
// Enums
///////////////////////////////////////////////////////////////////////////////
typedef enum
{
    BCM85100_RXH_IDLE_E,
    BCM85100_RXH_FLAG_FOUND_E,
    BCM85100_RXH_IN_MSG_E,
    BCM85100_RXH_ESC_CODE_DETECTED_E,
    BCM85100_RXH_IGNORE_E,
} BCM85100_RXH_STATE_MACHINE_ENUM;

///////////////////////////////////////////////////////////////////////////////
// Structs
///////////////////////////////////////////////////////////////////////////////
typedef struct
{
    BYTE                              pTxMsg[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN + 4];       // 4 spare
    BYTE                              pRxMsg[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN + 4];       // 4 spare

    BYTE                              pStuffedBuff[2 * BCM85100_HOST_PROTOCOL_MAX_MSG_LEN + 2];     // worst case factor 2x + start / end flags
    UINT16                            stuffedBuffLen;

    BYTE                              commandSeqNum;	   //sequence number given to this message on entry, needed to match response to request
    BOOLEAN                           enableForwarding;    // prevent forwarding while booting

    // configuration
    UINT32                            rxTimeoutMilli;			// Timeout for receive operation
    BCM85100_COM_MEDIA_ENUM           mcMedia;				// media type - per interface
    UINT32                            systemClockInHz;		// for calculating boot latencies and wait times

    // who am i
    BYTE                              deviceIndex;

    // when IP, who is the host's device
    BYTE                              remoteDeviceIndex;

    // tx params
    UINT32                            txTotalMsgSize;

    // this message's media type
    BCM85100_COM_MEDIA_ENUM           msgMedia;

    // rx params
    UINT32                            rxTotalMsgSize;
    UINT32                            rxReceivedSize;
    BYTE                              rxOpcode;
    BYTE                              rxSequenceNumber;
    BYTE                              rxTaskContext;
    BCM85100_ERROR_MSG_ENUM           rxAckCode;
    BCM85100_RXH_STATE_MACHINE_ENUM   rxState;

} BCM85100_DATABASE_STRUCT;


///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////
BCM85100_DATABASE_STRUCT gBcm85100Database[BCM85100_DEVICES_NUM];

///////////////////////////////////////////////////////////////////////////////
// Local Function Prototypes
///////////////////////////////////////////////////////////////////////////////
void                    bcm85100MessageBuild(BCM85100_DATABASE_STRUCT *pDbRecord, BYTE opcode, BYTE modemIndex, BYTE *pSentParamsBuff, UINT32 sentParamsLen);

BCM85100_ERROR_MSG_ENUM bcm85100MessageSend(BCM85100_DATABASE_STRUCT *pDbRecord, BYTE bootFwMode);
BCM85100_ERROR_MSG_ENUM bcm85100MessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord);

void                    bcm85100MessageStuff(BCM85100_DATABASE_STRUCT *pDbRecord);
void                    bcm85100UartMessageSend(BCM85100_DATABASE_STRUCT *pDbRecord);
BCM85100_ERROR_MSG_ENUM bcm85100UartMessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord);

BCM85100_ERROR_MSG_ENUM bcm85100SpiMessageSend(BCM85100_DATABASE_STRUCT *pDbRecord, BYTE bootFwMode);
BCM85100_ERROR_MSG_ENUM bcm85100SpiMessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord);
UINT32					bcm85100SpiEstimatedRxMsgSizeGet(BYTE deviceIndex);

void					bcm85100MiiDeviceMessageSend(BCM85100_DATABASE_STRUCT *pDbRecord);
BCM85100_ERROR_MSG_ENUM bcm85100MiiDeviceMessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord);

void					bcm85100IpClientMessageSend(BCM85100_DATABASE_STRUCT *pDbRecord);
BCM85100_ERROR_MSG_ENUM bcm85100IpClientMessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord);

BCM85100_ERROR_MSG_ENUM bcm85100RxMessageParse(BCM85100_DATABASE_STRUCT *pDbRecord);
BCM85100_ERROR_MSG_ENUM bcm85100RxStateMachine(BCM85100_DATABASE_STRUCT *pDbRecord, BYTE val, BOOLEAN *isComplete);
BOOLEAN					bcm85100RxMessageValidityCheck(BCM85100_DATABASE_STRUCT *pDbRecord);



//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
// Communication Control Functions
//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100HostCommOpen(void)
{
    UINT32  ix;

    #ifdef HOST_PROTOCOL_TX_LOG
    fopen_s(&pfHostMsg, BCM85100_HOST_MSG_FILE_NAME, "w");
    #endif

    // initializations per device
    for (ix = 0; ix < BCM85100_DEVICES_NUM; ix++)
    {
        // general initializations
        gBcm85100Database[ix].enableForwarding = FALSE;
        gBcm85100Database[ix].deviceIndex = (BYTE)ix;
        gBcm85100Database[ix].rxTimeoutMilli = BCM85100_DEFAULT_TIMEOUT_MILLI;
        gBcm85100Database[ix].remoteDeviceIndex = 0;         // assume only one device, unless told otherwise later
        gBcm85100Database[ix].mcMedia = BCM85100_COM_MEDIA_NULL_E;
        gBcm85100Database[ix].commandSeqNum = 1;
        gBcm85100Database[ix].rxState = BCM85100_RXH_IDLE_E;
        gBcm85100Database[ix].rxTotalMsgSize = 0;
        gBcm85100Database[ix].rxReceivedSize = 0;
    }
    return (BCM85100_SUCCESS_MSG_E);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100HostCommClose(void)
{
    return (BCM85100_SUCCESS_MSG_E);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ComConfigure(BYTE deviceIndex, BCM85100_COM_MEDIA_ENUM gpMedia, BCM85100_COM_MEDIA_ENUM mcMedia, UINT32 referenceClockInHz)
{
    BOOLEAN isBigEndian;

    bcm85100HostApiMutexGet(deviceIndex);


    if (deviceIndex >= BCM85100_DEVICES_NUM)
    {
        bcm85100HostApiMutexRelease(deviceIndex);
        return BCM85100_LOG_STATUS(BCM85100_INVALID_DEVICE_INDEX_E);
    }

    if ((gpMedia != BCM85100_COM_MEDIA_NULL_E) || (mcMedia > BCM85100_COM_MEDIA_IP_CLIENT_E))
    {
        bcm85100HostApiMutexRelease(deviceIndex);
        return BCM85100_LOG_STATUS(BCM85100_INVALID_MEDIA_E);
    }

    // configure endianity for future req/resp swapping
    isBigEndian = bcm85100IsBigEndian();
    bcm85100EndianConversionConfigure(isBigEndian);

    // initialize device in database
    gBcm85100Database[deviceIndex].mcMedia = mcMedia;
    gBcm85100Database[deviceIndex].systemClockInHz = referenceClockInHz;
    gBcm85100Database[deviceIndex].enableForwarding = ((mcMedia == BCM85100_COM_MEDIA_NULL_E) ? FALSE : TRUE);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (BCM85100_SUCCESS_MSG_E);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ComTimeoutSet(BYTE deviceIndex, UINT32 timeoutInMilli)
{
    if(deviceIndex >= BCM85100_DEVICES_NUM)
        return BCM85100_LOG_STATUS(BCM85100_INVALID_DEVICE_INDEX_E);
    bcm85100HostApiMutexGet(deviceIndex);
    gBcm85100Database[deviceIndex].rxTimeoutMilli = timeoutInMilli;
    bcm85100HostApiMutexRelease(deviceIndex);
    return (BCM85100_SUCCESS_MSG_E);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ComMessageForward(BYTE *pReqBuffer, INT32 reqLen, BYTE *pRespBuffer, INT32 *pRespLen)
{
    BYTE localResponse[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN + BCM85100_IP_PROTOCOL_HEADER_LEN + 100];
    UINT32 localLen;
    BCM85100_ERROR_MSG_ENUM retVal;
    BYTE deviceIndex;

    do
    {
        // sanity checks
        if (reqLen < BCM85100_IP_PROTOCOL_HEADER_LEN + BCM85100_HOST_PROTOCOL_HEADER_LEN + BCM85100_CHECKSUM_LEN)
        {
            retVal = BCM85100_INVALID_MSG_RX_E;
            localLen = 0;
            break;
        }
        // use stuff from the IP header
        deviceIndex = pReqBuffer[BCM85100_IP_PROTOCOL_DEVICE_ID_OFFSET];    // take from buffer
        if (deviceIndex >= BCM85100_DEVICES_NUM)
        {
            retVal = BCM85100_INVALID_DEVICE_INDEX_E;
            localLen = 0;
            break;
        }

        // actually forward to the device
        retVal = bcm85100MessageForward(deviceIndex,
                                        pReqBuffer + BCM85100_IP_PROTOCOL_HEADER_LEN, reqLen - BCM85100_IP_PROTOCOL_HEADER_LEN,
                                        localResponse + BCM85100_IP_PROTOCOL_HEADER_LEN, &localLen);

        // if we got an error, disregard payload (send only IP header)
        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            localLen = 0;
            break;
        }
    }
    while (0);

    // create proper response header
    localResponse[BCM85100_IP_PROTOCOL_DEVICE_ID_OFFSET] = pReqBuffer[BCM85100_IP_PROTOCOL_DEVICE_ID_OFFSET];
    localResponse[BCM85100_IP_PROTOCOL_SEQNUM_OFFSET   ] = pReqBuffer[BCM85100_IP_PROTOCOL_SEQNUM_OFFSET   ];
    localResponse[BCM85100_IP_PROTOCOL_GW_ERROR_OFFSET ] = retVal;
    localResponse[BCM85100_IP_PROTOCOL_MODE_OFFSET     ] = 0;

    // copy response to user's buffer and return
    memcpy(pRespBuffer, localResponse, localLen + BCM85100_IP_PROTOCOL_HEADER_LEN);
    *pRespLen = localLen + BCM85100_IP_PROTOCOL_HEADER_LEN;
    return BCM85100_LOG_STATUS(retVal);
}

BCM85100_ERROR_MSG_ENUM bcm85100HostBootFwCommBuildSendAndReceive(BYTE                     deviceIndex,           // to which device to send
        BYTE                    *pTxPayloadBuff,        // payload to send (not including header or checksum)
        UINT32                   txPayloadLen,          // payload length
        BCM85100_COM_OPCODE_ENUM opcode,                // opcode
        BYTE                     modemIndex,            // to which modem
        BYTE                    *pRxPayloadBuff,        // Where to store the received payload (length assumed to be suffient)
        UINT32                  *pActualRxLen,          // Where to indicate actual received payload length
        BCM85100_ERROR_MSG_ENUM *pStatus)               // Where to place the status
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    BCM85100_DATABASE_STRUCT *pDbRecord = &gBcm85100Database[deviceIndex];
    UINT32 rxLen;
    if (pDbRecord->mcMedia == BCM85100_COM_MEDIA_NULL_E)
    {
        return BCM85100_LOG_STATUS(BCM85100_MEDIA_NOT_INITIALIZED_E);
    }
    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "CL: Send msg with opcode = %u\n", opcode);
        fflush(pfHostMsg);
    }
    #endif
    pDbRecord->rxState = BCM85100_RXH_IDLE_E;
    pDbRecord->rxTotalMsgSize = 0;
    pDbRecord->rxReceivedSize = 0;
    *pActualRxLen = 0;
    bcm85100MessageBuild(pDbRecord, (BYTE)opcode, modemIndex, pTxPayloadBuff, txPayloadLen);
    retVal = bcm85100MessageSend(pDbRecord, BCM85100_SPI_BOOTFW); // BootFw Operation
    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        return BCM85100_LOG_STATUS(retVal);
    }
    if (opcode  ==  BCM85100_COM_BOOT_HEADER_MSG_OPCODE_E) //W.A for ST only , let bootwf time erase flash if necessary
    {
        bcm85100HostSleep(BCM85100_STM_BOOTFW_FLASH_ERASE_WAIT_TIME_MICRO);

    }
    retVal = bcm85100MessageReceive(pDbRecord);
    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        return BCM85100_LOG_STATUS(retVal);
    }
    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "CL: Got response to msg with opcode = %u\n", opcode);
        fflush(pfHostMsg);
    }
    #endif
    bcm85100ResponseParamSwap((BCM85100_COM_OPCODE_ENUM)pDbRecord->rxOpcode,
                              pDbRecord->pRxMsg + BCM85100_HOST_PROTOCOL_HEADER_LEN,
                              pDbRecord->pRxMsg[BCM85100_MSG_LEN_OFFSET]);
    pDbRecord->commandSeqNum++;
    retVal = pDbRecord->rxAckCode;
    rxLen = pDbRecord->rxTotalMsgSize - (BCM85100_HOST_PROTOCOL_HEADER_LEN + BCM85100_CHECKSUM_LEN);
    if (pRxPayloadBuff)
    {
        memcpy(pRxPayloadBuff, pDbRecord->pRxMsg + BCM85100_HOST_PROTOCOL_HEADER_LEN, rxLen);
    }
    *pActualRxLen = rxLen;
    *pStatus = retVal;
    return BCM85100_LOG_STATUS(retVal);
}
///////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostCommBuildSendAndReceive
///////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100HostCommBuildSendAndReceive(BYTE                     deviceIndex,           // to which device to send
        BYTE                    *pTxPayloadBuff,        // payload to send (not including header or checksum)
        UINT32                   txPayloadLen,          // payload length
        BCM85100_COM_OPCODE_ENUM opcode,                // opcode
        BYTE                     modemIndex,            // to which modem
        BYTE                    *pRxPayloadBuff,        // Where to store the received payload (length assumed to be suffient)
        UINT32                  *pActualRxLen,          // Where to indicate actual received payload length
        BCM85100_ERROR_MSG_ENUM *pStatus)               // Where to place the status
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    BCM85100_DATABASE_STRUCT *pDbRecord = &gBcm85100Database[deviceIndex];
    UINT32 rxLen;

    // if media not initialized
    if (pDbRecord->mcMedia == BCM85100_COM_MEDIA_NULL_E)
    {
        return BCM85100_LOG_STATUS(BCM85100_MEDIA_NOT_INITIALIZED_E);
    }

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "CL: Send msg with opcode = %u\n", opcode);
        fflush(pfHostMsg);
    }
    #endif

    // clear rx message size for the response
    pDbRecord->rxState = BCM85100_RXH_IDLE_E;
    pDbRecord->rxTotalMsgSize = 0;
    pDbRecord->rxReceivedSize = 0;
    *pActualRxLen = 0;

    bcm85100MessageBuild(pDbRecord, (BYTE)opcode, modemIndex, pTxPayloadBuff, txPayloadLen);

    retVal = bcm85100MessageSend(pDbRecord, BCM85100_SPI_NO_BOOTFW); // Normal Operation
    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        return BCM85100_LOG_STATUS(retVal);
    }



    retVal = bcm85100MessageReceive(pDbRecord);
    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        return BCM85100_LOG_STATUS(retVal);
    }

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "CL: Got response to msg with opcode = %u\n", opcode);
        fflush(pfHostMsg);
    }
    #endif

    bcm85100ResponseParamSwap((BCM85100_COM_OPCODE_ENUM)pDbRecord->rxOpcode,
                              pDbRecord->pRxMsg + BCM85100_HOST_PROTOCOL_HEADER_LEN,
                              pDbRecord->pRxMsg[BCM85100_MSG_LEN_OFFSET]);

    pDbRecord->commandSeqNum++;
    retVal = pDbRecord->rxAckCode;

    rxLen = pDbRecord->rxTotalMsgSize - (BCM85100_HOST_PROTOCOL_HEADER_LEN + BCM85100_CHECKSUM_LEN);
    if (pRxPayloadBuff)
    {
        memcpy(pRxPayloadBuff, pDbRecord->pRxMsg + BCM85100_HOST_PROTOCOL_HEADER_LEN, rxLen);
    }

    *pActualRxLen = rxLen;
    *pStatus = retVal;

    return BCM85100_LOG_STATUS(retVal);
}


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100HostCommBuildSend
///////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100HostCommBuildSend(BYTE                      deviceIndex,
        BYTE                     *pTxPayloadBuff,
        UINT32                    txPayloadLen,
        BCM85100_COM_OPCODE_ENUM  opcode,
        BYTE                      modemIndex ,
        BYTE                      bootFwMode)
{
//    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    BCM85100_DATABASE_STRUCT *pDbRecord = &gBcm85100Database[deviceIndex];

    // if media is NULL, cannot do anything, media not initialized
    if (pDbRecord->mcMedia == BCM85100_COM_MEDIA_NULL_E)
    {
        return BCM85100_LOG_STATUS(BCM85100_MEDIA_NOT_INITIALIZED_E);
    }

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "CL: Send msg with opcode = %d\n", opcode);
        fflush(pfHostMsg);
    }
    #endif

    bcm85100MessageBuild(pDbRecord, (BYTE)opcode, modemIndex, pTxPayloadBuff, txPayloadLen);

//    retVal = bcm85100MessageSend(pDbRecord, bootFwMode);
    bcm85100MessageSend(pDbRecord, bootFwMode); //Siklu change

    pDbRecord->commandSeqNum++;

    return (BCM85100_SUCCESS_MSG_E);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100MessageForward(BYTE deviceIndex, BYTE *pReqBuffer, UINT32 reqLen, BYTE *pRespBuffer, UINT32 *pRespLen)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    BCM85100_DATABASE_STRUCT *pDest = &gBcm85100Database[deviceIndex];

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "CL: Want to forward msg\n");
        fflush(pfHostMsg);
    }
    #endif

    // check if this device has ever been configured
    if (pDest->mcMedia == BCM85100_COM_MEDIA_NULL_E)
    {
        return BCM85100_LOG_STATUS(BCM85100_MEDIA_NOT_INITIALIZED_E);
    }

    // check if forwarding is disabled prior to taking mutex
    if (pDest->enableForwarding == FALSE)
    {
        return BCM85100_LOG_STATUS(BCM85100_HOST_BUSY_E);
    }

    // boot message? ignore it with error
    if (reqLen > BCM85100_HOST_PROTOCOL_MAX_MSG_LEN)
    {
        return BCM85100_LOG_STATUS(BCM85100_INVALID_MSG_RX_E);
    }

    // these boot cases are not expected to return an answer, and cannot be forwarded
    if (pReqBuffer[BCM85100_OPCODE_OFFSET] == BCM85100_COM_JUMP_TO_FW_MSG_OPCODE_E)
    {
        return BCM85100_LOG_STATUS(BCM85100_INVALID_MSG_OPCODE_E);
    }

    bcm85100HostApiMutexGet(deviceIndex);

    do
    {
        // copy req to TX dest
        memcpy(pDest->pTxMsg, pReqBuffer, reqLen);
        pDest->txTotalMsgSize = reqLen;
        pDest->commandSeqNum = (pReqBuffer[BCM85100_FLAGS_OFFSET] >> BCM85100_FLAGS_SEQ_NUM_BIT) & BCM85100_FLAGS_SEQ_NUM_MASK;    // for matching with resp

        // send towards MC if message is destined to MC, and connection to MC exists
        pDest->msgMedia = pDest->mcMedia;

        // clear rx message size for the dest response
        pDest->rxState = BCM85100_RXH_IDLE_E;
        pDest->rxTotalMsgSize = 0;
        pDest->rxReceivedSize = 0;

        #ifdef HOST_PROTOCOL_TX_LOG
        if (pfHostMsg != NULL)
        {
            fprintf(pfHostMsg, "CL: Forward msg\n");
            fflush(pfHostMsg);
        }
        #endif

        // send to dest
        retVal = bcm85100MessageSend(pDest, BCM85100_SPI_NO_BOOTFW);
        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            break;
        }

        // receive from dest
        retVal = bcm85100MessageReceive(pDest);
        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            break;
        }

        #ifdef HOST_PROTOCOL_TX_LOG
        if (pfHostMsg != NULL)
        {
            fprintf(pfHostMsg, "CL: Got response to forward msg \n");
            fflush(pfHostMsg);
        }
        #endif

        // copy response to Rx dest
        memcpy(pRespBuffer, pDest->pRxMsg, pDest->rxTotalMsgSize);
        *pRespLen = pDest->rxTotalMsgSize;

    }
    while (0);

    bcm85100HostApiMutexRelease(deviceIndex);

    return BCM85100_LOG_STATUS(retVal);
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
void bcm85100HostCommEnableMessageForwarding(BYTE                   deviceIndex,
        BOOLEAN                enable)
{
    BCM85100_DATABASE_STRUCT *pDbRecord = &gBcm85100Database[deviceIndex];
    pDbRecord->enableForwarding = enable;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100RawDataSend
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100HostCommRawDataSend(BYTE deviceIndex, BYTE *pBuff, UINT32 buffLen)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 chunkLen, sleepTimeMicro;
    BCM85100_DATABASE_STRUCT *pDbRecord = &gBcm85100Database[deviceIndex];

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fflush(pfHostMsg);
        fseek(pfHostMsg, 0, SEEK_SET);
        fprintf(pfHostMsg, "CL: Raw data send\n");
        fflush(pfHostMsg);
    }
    #endif

    switch (pDbRecord->mcMedia)
    {
    case BCM85100_COM_MEDIA_UART_E:
        bcm85100HostUartDataSend(pDbRecord->deviceIndex, pBuff, buffLen);
        break;

    case BCM85100_COM_MEDIA_SPI_E:
        // it takes 330 system clocks to the FW to handle 1 byte.
        sleepTimeMicro = (330 * 256) / (pDbRecord->systemClockInHz / 1000000);

        bcm85100HostSpiModeSet(deviceIndex, 0);
        bcm85100HostSpiMaxRateSet(deviceIndex, BCM85100_SPI_BOOT_RATE); // Set Boot rate to 2MHZ



        while (buffLen > 0)
        {
            chunkLen = MIN(buffLen, 256);
            bcm85100HostSpiDataSend(pDbRecord->deviceIndex, pBuff, chunkLen);
            buffLen -= chunkLen;
            pBuff += chunkLen;
            bcm85100HostSleep(sleepTimeMicro);
        }

        break;

    case BCM85100_COM_MEDIA_MII_DEVICE_E:
        // Silently ignore - bootFw not injected using raw data, but rather using TFTP
        break;

    case BCM85100_COM_MEDIA_IP_CLIENT_E:
        // Silently ignore - bootFw not passed from IP client to host
        break;

    default:
        retVal = BCM85100_MEDIA_NOT_INITIALIZED_E;
        break;
    }

    return BCM85100_LOG_STATUS(retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
UINT32 bcm85100HostCommModifyRxTimeout(BYTE                   deviceIndex,
                                       UINT32                 newRxTimeout)
{
    UINT32 temp;
    BCM85100_DATABASE_STRUCT *pDbRecord = &gBcm85100Database[deviceIndex];

    temp = pDbRecord->rxTimeoutMilli;
    pDbRecord->rxTimeoutMilli = newRxTimeout;
    return (temp);
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
void bcm85100HostCommRxFlush(BYTE deviceIndex)
{
    // flush pending rx data
    if (gBcm85100Database[deviceIndex].mcMedia == BCM85100_COM_MEDIA_UART_E)
    {
        bcm85100HostUartReceiverFlush(deviceIndex);
    }
}


///////////////////////////////////////////////////////////////////////////////
// Build request message
///////////////////////////////////////////////////////////////////////////////
void bcm85100MessageBuild(BCM85100_DATABASE_STRUCT   *pDbRecord,
                          BYTE                        opcode,
                          BYTE                        modemIndex,
                          BYTE                       *pSentParamsBuff,
                          UINT32                      sentParamsLen)
{
    UINT32 checksum;

    // copy message payload
    memcpy(pDbRecord->pTxMsg + BCM85100_HOST_PROTOCOL_HEADER_LEN, pSentParamsBuff, sentParamsLen);
    pDbRecord->txTotalMsgSize = sentParamsLen + BCM85100_HOST_PROTOCOL_HEADER_LEN + BCM85100_CHECKSUM_LEN;

    // handle endian conversion if necessary
    bcm85100RequestParamSwap((BCM85100_COM_OPCODE_ENUM)opcode, pDbRecord->pTxMsg + BCM85100_HOST_PROTOCOL_HEADER_LEN, pDbRecord->txTotalMsgSize - BCM85100_HOST_PROTOCOL_HEADER_LEN);

    // build header
    pDbRecord->pTxMsg[BCM85100_OPCODE_OFFSET] = opcode;
    pDbRecord->pTxMsg[BCM85100_FLAGS_OFFSET] = ((pDbRecord->commandSeqNum & BCM85100_FLAGS_SEQ_NUM_MASK) << BCM85100_FLAGS_SEQ_NUM_BIT)
            | ((modemIndex << BCM85100_FLAGS_MODEM_INDEX_BIT) & (BCM85100_FLAGS_MODEM_INDEX_MASK << BCM85100_FLAGS_MODEM_INDEX_BIT))
            | ((BYTE)MC_E << BCM85100_FLAGS_CPU_DEST_BIT);
    pDbRecord->pTxMsg[BCM85100_ACK_CODE_OFFSET] = 0;
    pDbRecord->pTxMsg[BCM85100_MSG_LEN_OFFSET] = (BYTE)(pDbRecord->txTotalMsgSize - BCM85100_HOST_PROTOCOL_HEADER_LEN - BCM85100_CHECKSUM_LEN);

    // insert checksum
    checksum = bcm85100ChecksumCalc(0, pDbRecord->pTxMsg + BCM85100_OPCODE_OFFSET, pDbRecord->txTotalMsgSize - BCM85100_CHECKSUM_LEN - BCM85100_OPCODE_OFFSET, TRUE);
    memcpy((void*)(pDbRecord->pTxMsg + pDbRecord->txTotalMsgSize - BCM85100_CHECKSUM_LEN), (void*)&checksum, BCM85100_CHECKSUM_LEN);

    // keep info
    pDbRecord->msgMedia = pDbRecord->mcMedia;
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100MessageSend
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100MessageSend(BCM85100_DATABASE_STRUCT *pDbRecord , BYTE bootFwMode)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "TX: dev = %d want to send message\n", pDbRecord->deviceIndex);
        fflush(pfHostMsg);
    }
    #endif

    switch (pDbRecord->msgMedia)
    {
    case BCM85100_COM_MEDIA_UART_E:
        bcm85100UartMessageSend(pDbRecord);
        break;

    case BCM85100_COM_MEDIA_SPI_E:
        retVal=bcm85100SpiMessageSend(pDbRecord, bootFwMode);
        break;

    case BCM85100_COM_MEDIA_MII_DEVICE_E:
        bcm85100MiiDeviceMessageSend(pDbRecord);
        break;

    case BCM85100_COM_MEDIA_IP_CLIENT_E:
        bcm85100IpClientMessageSend(pDbRecord);
        break;

    default:
        // nothing to do when not initialized
        retVal = BCM85100_MEDIA_NOT_INITIALIZED_E;
        break;
    }

    return BCM85100_LOG_STATUS(retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100MessageStuff
/////////////////////////////////////////////////////////////////////////////////
void bcm85100MessageStuff(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    UINT16  ix;
    UINT32  tmpLen = 0;
    UINT32  buffLen = pDbRecord->txTotalMsgSize;
    BYTE    *pBuff = pDbRecord->pTxMsg;
    BYTE    *pStuffedBuff = pDbRecord->pStuffedBuff;

    // initial flag
    pStuffedBuff[tmpLen] = 0x7E;
    tmpLen++;

    // go over the whole input buffer
    for (ix = 0; ix < buffLen; ix++)
    {
        if (pBuff[ix] == 0x7E)
        {
            pStuffedBuff[tmpLen] = 0x7D;
            pStuffedBuff[tmpLen + 1] = 0x5E;
            tmpLen += 2;
        }
        else if (pBuff[ix] == 0x7D)
        {
            pStuffedBuff[tmpLen] = 0x7D;
            pStuffedBuff[tmpLen + 1] = 0x5D;
            tmpLen += 2;
        }
        else
        {
            pStuffedBuff[tmpLen] = pBuff[ix];
            tmpLen++;
        }
    }
    // ending flag
    pStuffedBuff[tmpLen] = 0x7E;
    tmpLen++;

    // copy resulting length
    pDbRecord->stuffedBuffLen = (UINT16)tmpLen;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100UartMessageSend
/////////////////////////////////////////////////////////////////////////////////
void bcm85100UartMessageSend(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    #ifdef HOST_PROTOCOL_TX_LOG
    UINT32 i;
    #endif

    // stuff the message locally - must happen before releasing the mutex
    bcm85100MessageStuff(pDbRecord);

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {

        fprintf(pfHostMsg, "TX: UART (time = %ld, dev = %d): ", bcm85100HostTickGet(), pDbRecord->deviceIndex);
        for (i = 0; i < pDbRecord->stuffedBuffLen; i++)
        {
            fprintf(pfHostMsg, "%02X ", pDbRecord->pStuffedBuff[i]);
        }
        fprintf(pfHostMsg, "\n");
        fflush(pfHostMsg);
    }
    #endif

    bcm85100HostUartReceiverFlush(pDbRecord->deviceIndex);

    // and then send it
    bcm85100HostUartDataSend(pDbRecord->deviceIndex, pDbRecord->pStuffedBuff, pDbRecord->stuffedBuffLen);
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100SpiMessageSend
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100SpiMessageSend(BCM85100_DATABASE_STRUCT *pDbRecord, BYTE bootFwMode)
{
    UINT16 dataLen, stuffedBuffLen;
    UINT8 *pDataLen = (UINT8 *)&dataLen;
    UINT32 i;
    BOOLEAN bigEndian = bcm85100IsBigEndian();
    UINT32 startTime = bcm85100HostTickGet();
    UINT32 flagsSent = 0;

    while(1)
    {
        // 1 sec ?
        if( (bcm85100HostTimeGet(startTime) >= BCM85100_DEFAULT_TIMEOUT_MILLI) || (flagsSent >= BCM85100_SPI_RX_MSG_LARGEST_SIZE) )  
        {
            return BCM85100_LOG_STATUS(BCM85100_ERROR_IN_COM_RECEIVE_E);
        }

        //wait for DATA READY low for transmit
        if(bcm85100HostSpiIsDataReady(0) == FALSE)
        {
            break;
        }
        else         
        {
            //Send 8 flags - use static local buffer, add error log - TODO boris
            pDbRecord->stuffedBuffLen = BCM85100_SPI_IDLE_FLAGS_NUMBER;
            for (i = 0; i < pDbRecord->stuffedBuffLen; i++)
            {
                pDbRecord->pStuffedBuff[i] = 0x7E;
            }

            // send 8 flags
            bcm85100HostSpiDataSend(pDbRecord->deviceIndex, pDbRecord->pStuffedBuff, pDbRecord->stuffedBuffLen);
            flagsSent += BCM85100_SPI_IDLE_FLAGS_NUMBER;
        }
    }

    // stuff the message locally - must happen before releasing the mutex
    bcm85100MessageStuff(pDbRecord);
    if (bootFwMode == BCM85100_SPI_NO_BOOTFW)
    {
        dataLen = pDbRecord->stuffedBuffLen;
        if (dataLen > 256)
        {
            dataLen = dataLen;
        }
        dataLen = (dataLen + BCM85100_SPI_ALIGN_LEN - 1) & BCM85100_SPI_ALIGN_MASK;
        for (i = pDbRecord->stuffedBuffLen; i < dataLen; i++)
        {
            pDbRecord->pStuffedBuff[i] = 0x7E;
        }
        if (pDataLen[0] == 0x7E)  // special character - change format;
        {
            pDataLen[0] = 0;
            pDataLen[1] |= 0x80;
        }

        if (bigEndian)
        {
            stuffedBuffLen = bcm85100EndianSwapBytes(pDataLen);
        }
        else
        {
            stuffedBuffLen = dataLen;
        }

        memcpy(&pDbRecord->pStuffedBuff[BCM85100_SPARE1_INFO_OFFSET + 1], &stuffedBuffLen, sizeof(stuffedBuffLen));
        pDbRecord->stuffedBuffLen = dataLen;
    }
    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "TX: SPI (time = %ld, dev = %d): ", bcm85100HostTickGet(), pDbRecord->deviceIndex);
        for (i = 0; i < pDbRecord->stuffedBuffLen; i++)
        {
            fprintf(pfHostMsg, "%02X ", pDbRecord->pStuffedBuff[i]);
        }
        fprintf(pfHostMsg, "\n");
        fflush(pfHostMsg);
    }
    #endif

    // and then send it
    bcm85100HostSpiDataSend(pDbRecord->deviceIndex, pDbRecord->pStuffedBuff, pDbRecord->stuffedBuffLen);
    return BCM85100_SUCCESS_MSG_E;
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100IpClientMessageSend
/////////////////////////////////////////////////////////////////////////////////
void bcm85100IpClientMessageSend(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    UINT32 localLen;

    #ifdef HOST_PROTOCOL_TX_LOG
    UINT32 ix;
    #endif

    // make local copy of the message, and add its header - must happen before releasing the mutex
    BYTE localMsg[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN + BCM85100_IP_PROTOCOL_HEADER_LEN];
    localMsg[BCM85100_IP_PROTOCOL_SEQNUM_OFFSET   ] = pDbRecord->commandSeqNum;               // this applies only on the "IP" leg
    localMsg[BCM85100_IP_PROTOCOL_DEVICE_ID_OFFSET] = pDbRecord->remoteDeviceIndex;           // this is the host's index
    localMsg[BCM85100_IP_PROTOCOL_GW_ERROR_OFFSET ] = BCM85100_SUCCESS_MSG_E;
    localMsg[BCM85100_IP_PROTOCOL_MODE_OFFSET     ] = 0;
    memcpy(localMsg + BCM85100_IP_PROTOCOL_HEADER_LEN, pDbRecord->pTxMsg, pDbRecord->txTotalMsgSize);

    localLen = pDbRecord->txTotalMsgSize + BCM85100_IP_PROTOCOL_HEADER_LEN;

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg,"TX: IPC (dev = %d, localLen = %lu): ", pDbRecord->deviceIndex, localLen);
        for (ix=0; ix<localLen; ix++)
        {
            fprintf(pfHostMsg, "%02X ", localMsg[ix]);
        }
        fprintf(pfHostMsg, "\n");
        fflush(pfHostMsg);
    }
    #endif

    // and then send it
    bcm85100HostMIIDataWrite(pDbRecord->deviceIndex, localMsg, localLen);
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100MiiDeviceMessageSend
/////////////////////////////////////////////////////////////////////////////////
void bcm85100MiiDeviceMessageSend(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    #ifdef HOST_PROTOCOL_TX_LOG
    UINT32 ix;
    if (pfHostMsg != NULL)

    {
        fprintf(pfHostMsg, "TX: MII (time = %ld, dev = %d): ", bcm85100HostTickGet() , pDbRecord->deviceIndex);
        for (ix = 0; ix < pDbRecord->txTotalMsgSize; ix++)
        {
            fprintf(pfHostMsg, "%02X ", pDbRecord->pTxMsg[ix]);
        }
        fprintf(pfHostMsg, "\n");
        fflush(pfHostMsg);
    }
    #endif

    // and then send it
    bcm85100HostMIIDataWrite(pDbRecord->deviceIndex, pDbRecord->pTxMsg, pDbRecord->txTotalMsgSize);
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: receiveMessage                                                   //
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100MessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;

    switch (pDbRecord->msgMedia)
    {
    case BCM85100_COM_MEDIA_UART_E:
        retVal = bcm85100UartMessageReceive(pDbRecord);
        break;

    case BCM85100_COM_MEDIA_SPI_E:
        retVal = bcm85100SpiMessageReceive(pDbRecord);
        break;

    case BCM85100_COM_MEDIA_MII_DEVICE_E:
        retVal = bcm85100MiiDeviceMessageReceive(pDbRecord);
        break;

    case BCM85100_COM_MEDIA_IP_CLIENT_E:
        retVal = bcm85100IpClientMessageReceive(pDbRecord);
        break;

    default:
        // nothing to do when not initialized
        retVal = BCM85100_MEDIA_NOT_INITIALIZED_E;
        break;
    }

    return BCM85100_LOG_STATUS(retVal);
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100UartMessageReceive                                         //
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100UartMessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    BCM85100_ERROR_MSG_ENUM      retVal = BCM85100_SUCCESS_MSG_E;
    BYTE                val;
    BOOLEAN             isComplete = FALSE;
    BOOLEAN             timeout = FALSE;
    UINT32              startTime;
    UINT32              bytesRead;
    UINT32              timeRemainingInMilli = pDbRecord->rxTimeoutMilli;

    startTime = bcm85100HostTickGet();
    while (!isComplete)
    {
        // support for media change - not expected to occur in typical system
        if (pDbRecord->mcMedia != BCM85100_COM_MEDIA_UART_E)
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "RX: Media changed away from UART?\n");
                fflush(pfHostMsg);
            }
            #endif

            return BCM85100_LOG_STATUS(BCM85100_INVALID_MEDIA_E);
        }

        // what's the time?
        timeRemainingInMilli = pDbRecord->rxTimeoutMilli - (bcm85100HostTickGet() - startTime);
        timeout = (bcm85100HostTickGet() - startTime) > pDbRecord->rxTimeoutMilli;
        if (timeout)
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "UART timeout\n");
                fflush(pfHostMsg);
            }
            #endif
            return BCM85100_LOG_STATUS(BCM85100_ERROR_IN_COM_RECEIVE_E);
        }
        // get and process 1 byte at a time
        bcm85100HostUartDataReceive(pDbRecord->deviceIndex, &val, 1, (UINT16)timeRemainingInMilli, &bytesRead);
        if (bytesRead != 1)
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "ERR2\n");
            }
            #endif
            return BCM85100_LOG_STATUS(BCM85100_ERROR_IN_COM_RECEIVE_E); //here it falls and make MDP print Communication error : No response (1)
            //because the FW don't answer at all, cause it didn't succeed to analyze corectly the command (cause of OR)
        }

        retVal = bcm85100RxStateMachine(pDbRecord, val, &isComplete);
        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "ERR3\n");
            }
            #endif
            return BCM85100_LOG_STATUS(retVal);
        }
        #ifdef HOST_PROTOCOL_TX_LOG
        if (pfHostMsg != NULL)
        {
            fprintf(pfHostMsg, "%02X ", val);
        }
        #endif

    }
    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "\n");
    }
    #endif


    return (BCM85100_SUCCESS_MSG_E);
}


/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100SpiMessageReceive                                          //
/////////////////////////////////////////////////////////////////////////////////

BCM85100_ERROR_MSG_ENUM bcm85100SpiMessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    BCM85100_ERROR_MSG_ENUM   retVal = BCM85100_SUCCESS_MSG_E;
    BYTE                    txBuf[BCM85100_SPI_RX_MSG_LARGEST_SIZE], rxBuf[BCM85100_SPI_RX_MSG_LARGEST_SIZE];
    BOOLEAN                 isComplete = FALSE;
    BOOLEAN                 timeout = FALSE;
    UINT32                  startTime;
    UINT32                  ix;
    UINT32                  estimatedMsgSize;

    timeout = bcm85100HostSpiDataReadyWait(0, pDbRecord->rxTimeoutMilli);   //wait for fW answer (SpiReady = True )
    if (timeout)
    {
        #ifdef HOST_PROTOCOL_TX_LOG
        if (pfHostMsg != NULL)
        {
            fprintf(pfHostMsg, "SPI-Rx ERR (time = %ld, dev = %d): SPI RDY not detected\n", bcm85100HostTickGet(), pDbRecord->deviceIndex);
        }
        #endif
        return BCM85100_LOG_STATUS(BCM85100_ERROR_IN_COM_RECEIVE_E);
    }

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "SPI-Rx: ");
        fflush(pfHostMsg);

    }
    #endif

    memset(txBuf, 0x7E, sizeof(txBuf));
    startTime = bcm85100HostTickGet();

    while (!isComplete)
    {
        // what's the time?
        timeout = (bcm85100HostTickGet() - startTime) > pDbRecord->rxTimeoutMilli;
        if (timeout)
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "ERR1\n");
                fflush(pfHostMsg);
            }
            #endif
            return BCM85100_LOG_STATUS(BCM85100_ERROR_IN_COM_RECEIVE_E);
        }

        if (pDbRecord->rxState ==  BCM85100_RXH_IN_MSG_E)
        {
            estimatedMsgSize =  bcm85100SpiEstimatedRxMsgSizeGet(pDbRecord->deviceIndex);
        }
        else
        {
            estimatedMsgSize =  BCM85100_SPI_RX_MSG_SMALLEST_SIZE;
        }
        #ifdef HOST_PROTOCOL_TX_LOG
        if (pfHostMsg != NULL)
        {
            fprintf(pfHostMsg, "\nEsstimated=%lu :", estimatedMsgSize);
            fflush(pfHostMsg);
        }
        #endif

        if ((estimatedMsgSize == 0) || (pDbRecord->rxReceivedSize + estimatedMsgSize >= BCM85100_SPI_RX_MSG_LARGEST_SIZE))
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "ERR4\n");
                fflush(pfHostMsg);
            }
            #endif
            return BCM85100_LOG_STATUS(BCM85100_INVALID_MSG_RX_E);
        }
        pDbRecord->rxReceivedSize += estimatedMsgSize;
        bcm85100HostSpiDataReceive(pDbRecord->deviceIndex, txBuf, rxBuf, estimatedMsgSize);

        for (ix = 0; ix < estimatedMsgSize && (!isComplete); ix++)
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "%02X ", rxBuf[ix]);
            }
            #endif
            retVal = bcm85100RxStateMachine(pDbRecord, rxBuf[ix], &isComplete);
            if (retVal != BCM85100_SUCCESS_MSG_E)
            {
                #ifdef HOST_PROTOCOL_TX_LOG
                if (pfHostMsg != NULL)
                {

                    fprintf(pfHostMsg, "ERR2\n");
                    fflush(pfHostMsg);
                }
                #endif

                bcm85100HostSleep(50);  //wait a little bit to enable FW Rx reconfigure

                return BCM85100_LOG_STATUS(retVal);
            }

            if (isComplete)
            {
                break;
            }
        }

        if ((pDbRecord->rxReceivedSize >= BCM85100_SPI_RX_MSG_ERR_TH_SIZE) && (pDbRecord->rxTotalMsgSize < BCM85100_SPI_RX_MSG_ERR_TH_SIZE / 2))
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "ERR6\n");
            }
            #endif
            return BCM85100_LOG_STATUS(BCM85100_INVALID_MSG_RX_E);
        }
    }

    #ifdef HOST_PROTOCOL_TX_LOG
    if (pfHostMsg != NULL)
    {
        fprintf(pfHostMsg, "\n");
    }
    #endif

    return (BCM85100_SUCCESS_MSG_E);
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100IpClientMessageReceive
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100IpClientMessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    BYTE localMsg[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN + BCM85100_IP_PROTOCOL_HEADER_LEN];
    UINT32 localLen;
    BOOLEAN timeout = FALSE;
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32  timeRemainingInMilli;
    UINT32  startTime;

    #ifdef HOST_PROTOCOL_TX_LOG
    UINT32 ix;
    #endif

    startTime = bcm85100HostTickGet();  // note that on some platforms, this may not be accurate

    while (1)
    {
        // support for media change - not expected to occur in typical system
        if (pDbRecord->mcMedia != BCM85100_COM_MEDIA_IP_CLIENT_E)
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "RX: Media changed away from IPC?\n");
                fflush(pfHostMsg);
            }
            #endif
            return BCM85100_LOG_STATUS(BCM85100_INVALID_MEDIA_E);
        }

        // wait for packet, up to current timeout for it (can be any arbitrary)
        timeRemainingInMilli = pDbRecord->rxTimeoutMilli - (bcm85100HostTickGet() - startTime);// what's the time?
        timeout = bcm85100HostMIIDataRead(pDbRecord->deviceIndex, localMsg, &localLen, timeRemainingInMilli);
        if (timeout)
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "RX: IPC timeout\n");
                fflush(pfHostMsg);
            }
            #endif
            return BCM85100_LOG_STATUS(BCM85100_ERROR_IN_COM_RECEIVE_E);
        }

        #ifdef HOST_PROTOCOL_TX_LOG
        if (pfHostMsg != NULL)
        {
            fprintf(pfHostMsg,"RX: IPC (dev=%d, localLen=%lu): ", pDbRecord->deviceIndex, localLen);
            for (ix=0; ix<localLen; ix++)
            {
                fprintf(pfHostMsg, "%02X ", localMsg[ix]);
            }
            fprintf(pfHostMsg, "\n");
            fflush(pfHostMsg);
        }
        #endif

        // disregard anything which does not look like "our" response
        if (localLen < BCM85100_IP_PROTOCOL_HEADER_LEN)
        {
            #ifdef HOST_PROTOCOL_TX_LOG
            if (pfHostMsg != NULL)
            {
                fprintf(pfHostMsg, "RX: IPC disregarding too small packet %lu\n", localLen);
                fflush(pfHostMsg);
            }
            #endif

            continue;
        }

        // Did the GW report an error (from GW or from device?)
        retVal = localMsg[BCM85100_IP_PROTOCOL_GW_ERROR_OFFSET];
        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            return BCM85100_LOG_STATUS(retVal);
        }

        // skip IP header, and copy internals to local database
        memcpy(pDbRecord->pRxMsg, localMsg + BCM85100_IP_PROTOCOL_HEADER_LEN, localLen - BCM85100_IP_PROTOCOL_HEADER_LEN);

        // update message size
        pDbRecord->rxTotalMsgSize = localLen - BCM85100_IP_PROTOCOL_HEADER_LEN;

        // override fields with content of IP header prior to handling
        // this must be done after valid message checksum (with fields that remote host and FW modified)

        pDbRecord->pRxMsg[BCM85100_FLAGS_OFFSET] &= (~BCM85100_FLAGS_SEQ_NUM_MASK << BCM85100_FLAGS_SEQ_NUM_BIT);     // clear device seqnum
        pDbRecord->pRxMsg[BCM85100_FLAGS_OFFSET] |= (localMsg[BCM85100_IP_PROTOCOL_SEQNUM_OFFSET] & BCM85100_FLAGS_SEQ_NUM_MASK) << BCM85100_FLAGS_SEQ_NUM_BIT; // use IP header instead

        // check response - should be done only once, since IP header indicates that this is the correct response
        return (bcm85100RxMessageParse(pDbRecord));
    }
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100MiiDeviceMessageReceive
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100MiiDeviceMessageReceive(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    BOOLEAN timeout;
    UINT32  timeRemainingInMilli;
    UINT32  startTime;

    startTime = bcm85100HostTickGet();  // note that on some platforms, this may not be accurate

    // support for media change - not expected to occur in typical system
    if (pDbRecord->mcMedia != BCM85100_COM_MEDIA_MII_DEVICE_E)
    {
        #ifdef HOST_PROTOCOL_TX_LOG
        if (pfHostMsg != NULL)
        {
            fprintf(pfHostMsg, "RX: Media changed away from MII?\n");
            fflush(pfHostMsg);
        }
        #endif

        return BCM85100_LOG_STATUS(BCM85100_INVALID_MEDIA_E);
    }

    // wait for packet, up to current timeout for it (can be any arbitrary)
    timeRemainingInMilli = pDbRecord->rxTimeoutMilli - (bcm85100HostTickGet() - startTime);// what's the time?
    timeout = bcm85100HostMIIDataRead(pDbRecord->deviceIndex, pDbRecord->pRxMsg, &pDbRecord->rxTotalMsgSize, timeRemainingInMilli);
    if (timeout)
    {
        #ifdef HOST_PROTOCOL_TX_LOG
        if (pfHostMsg != NULL)
        {
            fprintf(pfHostMsg, "RX: MII timeout\n");
        }
        #endif

        return BCM85100_LOG_STATUS(BCM85100_ERROR_IN_COM_RECEIVE_E);
    }

    // check response
    return (bcm85100RxMessageParse(pDbRecord));
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100SpiEstimatedRxMsgSizeGet
/////////////////////////////////////////////////////////////////////////////////
UINT32 bcm85100SpiEstimatedRxMsgSizeGet(BYTE deviceIndex)
{
    BCM85100_DATABASE_STRUCT *pDbRecord = &gBcm85100Database[deviceIndex];
    UINT16 guessedLen = 0;
    UINT16 remainLen = 0;
    BOOLEAN bigEndian = bcm85100IsBigEndian();

    // until we are inside the message, we cannot really know how much data to pull
    if (pDbRecord->rxState == BCM85100_RXH_IDLE_E ||                 // SM was never run
            pDbRecord->rxState == BCM85100_RXH_FLAG_FOUND_E ||       // flags in between messages
            pDbRecord->rxTotalMsgSize < BCM85100_OPCODE_OFFSET)      // inside message, but not enough of it received
    {
        return (BCM85100_SPI_RX_MSG_SMALLEST_SIZE);
    }

    // we have enough of the message to speculate about its length
    //guessedLen = pDbRecord->pRxMsg[BCM85100_MSG_LEN_OFFSET] + BCM85100_HOST_PROTOCOL_HEADER_LEN + BCM85100_CHECKSUM_LEN;
    memcpy(BYTEP_CAST(&guessedLen), &pDbRecord->pRxMsg[BCM85100_SPARE1_INFO_OFFSET], sizeof(UINT16));

    if (bigEndian)
    {
        guessedLen = bcm85100EndianSwapBytes((BYTE *)&guessedLen);
    }

    if (guessedLen & 0x8000)
    {
        remainLen = (guessedLen & 0x7FFF) + 0x7E;
    }
    else
    {
        remainLen = guessedLen;
    }

    remainLen -= pDbRecord->rxReceivedSize;
    remainLen  = MAX(remainLen, 0);
    remainLen  = MIN(remainLen, BCM85100_SPI_RX_MSG_LARGEST_SIZE);
    return (remainLen);
}

/////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: bcm85100RxStateMachine
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100RxStateMachine(BCM85100_DATABASE_STRUCT   *pDbRecord,
        BYTE                  val,
        BOOLEAN               *isComplete)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    BYTE *pBuff = pDbRecord->pRxMsg;

    *isComplete = FALSE;

    switch (pDbRecord->rxState)
    {
    case BCM85100_RXH_IDLE_E:
        if (val == 0x7E)
        {
            pDbRecord->rxState = BCM85100_RXH_FLAG_FOUND_E;
        }
        break;

    case BCM85100_RXH_FLAG_FOUND_E:
        if (val == 0x7D)
        {
            pDbRecord->rxState = BCM85100_RXH_ESC_CODE_DETECTED_E;
        }
        else if (val != 0x7E)
        {
            pBuff[pDbRecord->rxTotalMsgSize] = val;
            pDbRecord->rxTotalMsgSize++;
            pDbRecord->rxState = BCM85100_RXH_IN_MSG_E;
        }
        break;

    case BCM85100_RXH_IN_MSG_E:
        if (val == 0x7D)
        {
            pDbRecord->rxState = BCM85100_RXH_ESC_CODE_DETECTED_E;
        }
        else if (val == 0x7E)
        {
            retVal = bcm85100RxMessageParse(pDbRecord);
            if (retVal == BCM85100_SUCCESS_MSG_E)
            {
                // good message - break out
                pDbRecord->rxState = BCM85100_RXH_IDLE_E;
                *isComplete = TRUE;
                return BCM85100_LOG_STATUS(retVal);
            }
            else
            {
                // bad message / not matching request - restart
                pDbRecord->rxState = BCM85100_RXH_FLAG_FOUND_E;
                pDbRecord->rxTotalMsgSize = 0;
                pDbRecord->rxReceivedSize = 0;
            }
        }
        else
        {
            pBuff[pDbRecord->rxTotalMsgSize] = val;
            pDbRecord->rxTotalMsgSize++;
        }
        break;

    case BCM85100_RXH_ESC_CODE_DETECTED_E:
        if (val == 0x5E)
        {
            pBuff[pDbRecord->rxTotalMsgSize] = 0x7E;
            pDbRecord->rxTotalMsgSize++;
            pDbRecord->rxState = BCM85100_RXH_IN_MSG_E;
        }
        else if (val == 0x5D)
        {
            pBuff[pDbRecord->rxTotalMsgSize] = 0x7D;
            pDbRecord->rxTotalMsgSize++;
            pDbRecord->rxState = BCM85100_RXH_IN_MSG_E;
        }
        else
        {
            pDbRecord->rxTotalMsgSize = 0;
            pDbRecord->rxReceivedSize = 0;
            pDbRecord->rxState = BCM85100_RXH_IGNORE_E;
            retVal = BCM85100_ERROR_IN_COM_RECEIVE_E;  // FLAG/other while escaped - must be an error
        }
        break;

    case BCM85100_RXH_IGNORE_E:
        // not supposed to happen, but we'll allow it for now
        if (val == 0x7E)
        {
            pDbRecord->rxState = BCM85100_RXH_IDLE_E;
        }
        break;
    }


    // if message is too long, ignore it
    if (pDbRecord->rxTotalMsgSize > BCM85100_HOST_PROTOCOL_MAX_MSG_LEN)
    {
        pDbRecord->rxTotalMsgSize = 0;
        pDbRecord->rxReceivedSize = 0;
        pDbRecord->rxState = BCM85100_RXH_IGNORE_E;
        retVal = BCM85100_ERROR_IN_COM_RECEIVE_E;
    }

    return BCM85100_LOG_STATUS(retVal);
}

///////////////////////////////////////////////////////////////////////////////
// Checks if there is an error in the msg buffer.
// return FALSE if error, else TRUE.
///////////////////////////////////////////////////////////////////////////////
BOOLEAN bcm85100RxMessageValidityCheck(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    UINT32 checksum;

    // check if at least header len and checksum len exist
    if (pDbRecord->rxTotalMsgSize < (BCM85100_HOST_PROTOCOL_HEADER_LEN + BCM85100_CHECKSUM_LEN))
    {
        return (FALSE);
    }

    // check match between length in header and recieved length
    if ((UINT32)pDbRecord->rxTotalMsgSize !=
            (UINT32)(pDbRecord->pRxMsg[BCM85100_MSG_LEN_OFFSET] + BCM85100_HOST_PROTOCOL_HEADER_LEN + BCM85100_CHECKSUM_LEN))
    {
        return (FALSE);
    }



    // compare the message checksum to the calculated checksum
    memcpy((void*)&checksum, pDbRecord->pRxMsg + pDbRecord->rxTotalMsgSize - BCM85100_CHECKSUM_LEN, BCM85100_CHECKSUM_LEN);
    if (checksum != bcm85100ChecksumCalc(0, pDbRecord->pRxMsg + BCM85100_OPCODE_OFFSET, pDbRecord->rxTotalMsgSize - BCM85100_CHECKSUM_LEN - BCM85100_OPCODE_OFFSET, TRUE))
    {
        return (FALSE);
    }

    // split fields - save rx parameters in the host protocol database
    pDbRecord->rxOpcode = (BCM85100_COM_OPCODE_ENUM)pDbRecord->pRxMsg[BCM85100_OPCODE_OFFSET];
    pDbRecord->rxSequenceNumber = (pDbRecord->pRxMsg[BCM85100_FLAGS_OFFSET] >> BCM85100_FLAGS_SEQ_NUM_BIT) & BCM85100_FLAGS_SEQ_NUM_MASK;
    pDbRecord->rxTaskContext = (BYTE)((pDbRecord->pRxMsg[BCM85100_FLAGS_OFFSET] >> BCM85100_FLAGS_MODEM_INDEX_BIT) & BCM85100_FLAGS_MODEM_INDEX_MASK);
    pDbRecord->rxAckCode = (BCM85100_ERROR_MSG_ENUM)(pDbRecord->pRxMsg[BCM85100_ACK_CODE_OFFSET]);

    // check vs. request
    if (pDbRecord->rxSequenceNumber != (pDbRecord->commandSeqNum & BCM85100_FLAGS_SEQ_NUM_MASK))
    {
        return (FALSE);
    }

    // passed all filters - bingo, correct
    return (TRUE);
}

///////////////////////////////////////////////////////////////////////////////
// Parses host protocol message
///////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM  bcm85100RxMessageParse(BCM85100_DATABASE_STRUCT *pDbRecord)
{
    if (bcm85100RxMessageValidityCheck(pDbRecord))
    {
        return (BCM85100_SUCCESS_MSG_E);
    }

    // will be silently discarded
    return BCM85100_LOG_STATUS(BCM85100_INVALID_MSG_RX_E);
}


UINT32 bcm85100HostTimeGet(UINT32 startTime)
{
    UINT32 endTime = bcm85100HostTickGet();
    UINT32 resTime;

    if(endTime >= startTime) 
    {
        resTime = endTime - startTime;
    }
    else
    {
        resTime = 0xFFFFFFFF-startTime + endTime;	//define constand for max UINT32 number - TODO boris 
    }

    return resTime;
}




