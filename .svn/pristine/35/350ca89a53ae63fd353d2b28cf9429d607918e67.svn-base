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

#include <string.h>
#include "utils.h"
#include "hostProtocolStructs.h"
#include "bcm85100_api.h"
#include "hostCommLayer.h"
#include "hostDriver.h"

//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
// Init functions
//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------

BCM85100_ERROR_MSG_ENUM BCM85100_API_Open(void)
{
    if (bcm85100HostOpen() == TRUE)
    {
        return (BCM85100_MEDIA_NOT_INITIALIZED_E);
    }
    return (bcm85100HostCommOpen());
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_Close(void)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    BOOLEAN hostRetVal;
    retVal = bcm85100HostCommClose();
    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        return (retVal);
    }
    hostRetVal = bcm85100HostClose();
    if (hostRetVal == TRUE)
    {
        return (BCM85100_MEDIA_NOT_INITIALIZED_E);
    }
    return (BCM85100_SUCCESS_MSG_E);
}


/////////////////////////// /////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BOOLEAN bcm85100IsAlive(BYTE deviceIndex, UINT32 timeInMs, UINT32 tries)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    BCM85100_MODEM_VERSION_STRUCT ver;
    UINT32 savedTimeOutMilli;

    // save the current timeout val, and set the new timeout val
    savedTimeOutMilli = bcm85100HostCommModifyRxTimeout(deviceIndex, timeInMs);

    // Sleep 100 ms anyway
    bcm85100HostSleep((UINT32)timeInMs * 1000);

    while (tries)
    {
        retVal = BCM85100_API_VersionGet(deviceIndex, &ver);

        if (retVal == BCM85100_INVALID_MSG_OPCODE_E || retVal == BCM85100_SUCCESS_MSG_E)
        {
            // restore timeout
            (void)bcm85100HostCommModifyRxTimeout(deviceIndex, savedTimeOutMilli);
            return (TRUE);
        }

        tries--;
    }

    // restore timeout
    (void)bcm85100HostCommModifyRxTimeout(deviceIndex, savedTimeOutMilli);
    return (FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM bcm85100FwSendAndJump(BYTE deviceIndex, BYTE *pBuffer)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    BCM85100_BOOT_HEADER_MSG_STRUCT *pHeader;
    BCM85100_BOOT_DATA_RECORD_HEADER_STRUCT *pRecordHeader;
    BCM85100_BOOT_DATA_RECORD_HEADER_STRUCT nullRecordHeader = {0, 0, 0};
    INT32 totalLen;
    UINT32 offset = sizeof(UINT32); // skip the burn number
    UINT32 size;
    BOOLEAN bigEndian = bcm85100IsBigEndian();
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    // header message
    pHeader = (BCM85100_BOOT_HEADER_MSG_STRUCT *)&pBuffer[offset];
    retVal = bcm85100HostBootFwCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(pHeader),
             sizeof(BCM85100_BOOT_HEADER_MSG_STRUCT),
             BCM85100_COM_BOOT_HEADER_MSG_OPCODE_E,
             0,
             NULL,
             &rxLen,
             &rxStatus);
    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        return (retVal);
    }

    // skip the SW header and the boot fw
    offset += sizeof(BCM85100_BOOT_HEADER_MSG_STRUCT) + BCM85100_BOOT_FW_SIZE_BYTES;

    // data messages
    if (bigEndian)
    {
        totalLen = bcm85100EndianConversion(BYTEP_CAST(&pHeader->totalLength), sizeof(UINT32)) - BCM85100_BOOT_FW_SIZE_BYTES;
    }
    else
    {
        totalLen = pHeader->totalLength - BCM85100_BOOT_FW_SIZE_BYTES;
    }
    while (totalLen > 0)
    {
        // if address bytes are all 0 ==> zero padding reached ==> no more data records to send.
        if (!(memcmp(&pBuffer[offset],BYTEP_CAST(&nullRecordHeader), 4))) // Siklu change - removed boolean
        {
            offset += totalLen;
            break;
        }
        pRecordHeader = (BCM85100_BOOT_DATA_RECORD_HEADER_STRUCT*)&pBuffer[offset];

        if (bigEndian)
        {
            size = bcm85100EndianConversion(BYTEP_CAST(&pRecordHeader->size), sizeof(UINT16));
        }
        else
        {
            size = (UINT32)pRecordHeader->size;
        }

        retVal = bcm85100HostBootFwCommBuildSendAndReceive(deviceIndex,
                 BYTEP_CAST(&pBuffer[offset]),
                 sizeof(BCM85100_BOOT_DATA_RECORD_HEADER_STRUCT) + size,
                 BCM85100_COM_BOOT_DATA_MSG_OPCODE_E,
                 0,
                 NULL,
                 &rxLen,
                 &rxStatus);
        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            return (retVal);
        }

        if (totalLen < 0)
        {
            return (BCM85100_BOOT_DATA_TOO_BIG_E);
        }

        offset += sizeof(BCM85100_BOOT_DATA_RECORD_HEADER_STRUCT) + size;
        totalLen -= sizeof(BCM85100_BOOT_DATA_RECORD_HEADER_STRUCT) + size;
    }

    // checksum message
    retVal = bcm85100HostBootFwCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&pBuffer[offset]),
             sizeof(UINT32),
             BCM85100_COM_BOOT_CHECKSUM_MSG_OPCODE_E,
             0,
             NULL,
             &rxLen,
             &rxStatus);
    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        return (retVal);
    }

    // jump message
    retVal = bcm85100HostCommBuildSend(deviceIndex,
                                       NULL,
                                       0,
                                       BCM85100_COM_JUMP_TO_FW_MSG_OPCODE_E,
                                       0,
                                       BCM85100_SPI_BOOTFW);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
// pBuffer - pointer to the FW file that includes:
//           1. reserved field for burn number
//           2. header
//           3. boot fw code padded with zeroes
//           3. fw data records * N
//           4. checksum
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_FirmwareFromHostBoot(BYTE deviceIndex, BYTE *pBuffer, UINT32 len, BCM85100_DEVICE_RESET_TYPE_ENUM resetType)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 receivedChecksum;
//    BYTE spiClockMode;
    (void) resetType; //Siklu change

    // sanity check
    if (len < BCM85100_BOOT_FW_SIZE_BYTES)
    {
        return (BCM85100_BOOT_DATA_TOO_SMALL_E);
    }


    bcm85100HostApiMutexGet(deviceIndex);

    // compare the received checksum to the calculated checksum (not calculated on
    // burn number and checksum fields.
    memcpy(BYTEP_CAST(&receivedChecksum), &pBuffer[len - BCM85100_CHECKSUM_LEN], BCM85100_CHECKSUM_LEN);

    // subtract 16k boot Firmware file
    len -= BCM85100_BOOT_FW_SIZE_BYTES;

    if (bcm85100ChecksumCalc(0, &pBuffer[4] + BCM85100_BOOT_FW_SIZE_BYTES, len - 4 - BCM85100_CHECKSUM_LEN, TRUE) != receivedChecksum) // exclude burn number and checksum fields.
    {
        bcm85100HostApiMutexRelease(deviceIndex);
        return (BCM85100_BOOT_INVALID_CHECKSUM_E);
    }

    do
    {

        //disables message forwarding
        bcm85100HostCommEnableMessageForwarding(deviceIndex, FALSE);

        bcm85100HostDeviceReset(deviceIndex);

        // inject boot fw - burn number and SW header are skipped.
        retVal = bcm85100HostCommRawDataSend(deviceIndex, &pBuffer[4 + sizeof(BCM85100_BOOT_HEADER_MSG_STRUCT)], (UINT32)BCM85100_BOOT_FW_SIZE_BYTES);
        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            break;
        }

        // flush pending rx data
        bcm85100HostCommRxFlush(deviceIndex);

        bcm85100HostSleep(BCM85100_BOOTFW_WAIT_TIME_MICRO); //wait  for  bootfw sw to come up

        // send FW and jump
        if (bcm85100IsAlive(deviceIndex, (UINT32)BCM85100_BOOT_IS_ALIVE_TIMEOUT_MILLI, 10))
        {
            retVal = bcm85100FwSendAndJump(deviceIndex, pBuffer);
            if (retVal != BCM85100_SUCCESS_MSG_E)
            {
                break;
            }
        }
        bcm85100HostSleep(BCM85100_FW_WAIT_TIME_MICRO); //wait  for  fw sw to come up

		// spi settings - Siklu change
		bcm85100HostSpiModeSet(deviceIndex, 3); //Set clocking  mode to 3
		bcm85100HostSpiMaxRateSet(deviceIndex, 100000000 / 32); //Set SPi Master to 3MHz
		
        // wait up to 3 seconds for fw
        bcm85100HostSleep(BCM85100_BOOTFW_WAIT_TIME_MICRO);
        if (!bcm85100IsAlive(deviceIndex, (UINT32)BCM85100_BOOT_IS_ALIVE_TIMEOUT_MILLI, 10))
        {
            retVal = BCM85100_FW_NOT_ALIVE_E;
        }

    }
    while (0);

    //enables message forwarding once again
    if (retVal ==  BCM85100_SUCCESS_MSG_E)
    {
        bcm85100HostCommEnableMessageForwarding(deviceIndex, TRUE);
    }

    // end critical section
    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}


//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
// Configuration File Functions
//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////
// Config file APIs
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ConfigFileFromHostLoad(BYTE       deviceIndex,
        BYTE       modemIndex,
        BYTE       *pBuffer,
        UINT16     len) // in bytes
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    UINT16 bytesToSet;
    BCM85100_COM_CFG_FILE_CHUNK_STRUCT   cfgFileChunk;
    UINT16 bytesSetSoFar = 0;

    BYTE configStatus[2];
    BYTE sleepCycles = 20;
    BCM85100_ERROR_MSG_ENUM configFileRetVal;
    BYTE isFinished;
    bcm85100HostApiMutexGet(deviceIndex);
    cfgFileChunk.position = BYTE_CAST(BCM85100_COM_CONFIG_FILE_START_E);

    while ((len > 0) && (retVal == BCM85100_SUCCESS_MSG_E))
    {
        bytesToSet = (UINT16)MIN((BCM85100_HOST_PROTOCOL_MAX_PAYLOAD_LEN - 1), len);
        memcpy((BYTE*)cfgFileChunk.pBuffer, &pBuffer[bytesSetSoFar], bytesToSet);

        retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
                 BYTEP_CAST(&cfgFileChunk),
                 (UINT16)(bytesToSet + sizeof(BYTE)), // 1 byte for position
                 BCM85100_COM_LOAD_CONFIG_FILE_OPCODE_E,
                 modemIndex,
                 NULL,
                 &rxLen,
                 &rxStatus);

        len -= bytesToSet;
        bytesSetSoFar += bytesToSet;

        if (len <= (BCM85100_HOST_PROTOCOL_MAX_PAYLOAD_LEN - 1))
        {
            cfgFileChunk.position = BYTE_CAST(BCM85100_COM_CONFIG_FILE_END_E);
        }
        else
        {
            cfgFileChunk.position = BYTE_CAST(BCM85100_COM_CONFIG_FILE_MIDDLE_E);
        }
    }

    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        bcm85100HostApiMutexRelease(deviceIndex);
        return (retVal);
    }

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_RUN_CONFIG_FILE_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        bcm85100HostApiMutexRelease(deviceIndex);
        return (retVal);
    }

    while (sleepCycles > 0)
    {
        retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
                 NULL,
                 0,
                 BCM85100_COM_CONFIG_FILE_GET_DONE_OPCODE_E,
                 modemIndex,
                 configStatus,
                 &rxLen,
                 &rxStatus);

        RETURNED_LENGTH_CHECK(sizeof(BCM85100_CONFIG_FILE_STATUS_STRUCT));

        // check config file value only when config file Task finished working
        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            bcm85100HostApiMutexRelease(deviceIndex);
            return (retVal);
        }

        isFinished = ((BCM85100_CONFIG_FILE_STATUS_STRUCT*)&configStatus[0])->isFinished;
        configFileRetVal = (BCM85100_ERROR_MSG_ENUM)(((BCM85100_CONFIG_FILE_STATUS_STRUCT*)&configStatus[0])->configFileRetVal);
        if (isFinished == TRUE)
        {
            bcm85100HostApiMutexRelease(deviceIndex);
            return (configFileRetVal);
        }
        //sleep 100msec, then try again
        bcm85100HostSleep(100000);
        sleepCycles--;
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (BCM85100_ERROR_CFG_FILE_TIMEOUT_E);
}


//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
// Status and Performance Monitoring Functions (general)
//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_VersionGet(BYTE deviceIndex, BCM85100_MODEM_VERSION_STRUCT *pVer)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_VERSION_GET_OPCODE_E,
             0,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_MODEM_VERSION_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pVer), rxBuffer, sizeof(BCM85100_MODEM_VERSION_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_BootLoaderVersionGet(BYTE deviceIndex, BCM85100_BOOT_LOADER_VERSION_STRUCT *pVer)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_BOOT_LOADER_VERSION_GET_OPCODE_E,
             0,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_BOOT_LOADER_VERSION_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pVer), rxBuffer, sizeof(BCM85100_BOOT_LOADER_VERSION_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_AlarmEnableGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_ALARMS_STRUCT *pAlarmsMask)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_ALARM_ENABLE_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_ALARMS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pAlarmsMask), rxBuffer, sizeof(BCM85100_ALARMS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);

}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_AlarmEnableSet(BYTE deviceIndex, BYTE modemIndex, BCM85100_ALARMS_STRUCT *pAlarmsMask)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(pAlarmsMask),
             sizeof(BCM85100_ALARMS_STRUCT),
             BCM85100_COM_ALARM_ENABLE_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);

}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_AlarmStatusGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_ALARMS_STRUCT *pAlarmsStatus)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_ALARM_STATUS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_ALARMS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pAlarmsStatus), rxBuffer, sizeof(BCM85100_ALARMS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}


//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
// Status and Performance Monitoring Functions (modem)
//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------


/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemStatusGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_STATUS_STRUCT *pMs)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_STATUS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);


    RETURNED_LENGTH_CHECK(sizeof(BCM85100_MODEM_STATUS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pMs), rxBuffer, sizeof(BCM85100_MODEM_STATUS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_FrequentModemStatusGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_FREQ_STATUS_STRUCT *pMs)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_FREQ_STATUS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);


    RETURNED_LENGTH_CHECK(sizeof(BCM85100_MODEM_FREQ_STATUS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pMs), rxBuffer, sizeof(BCM85100_MODEM_FREQ_STATUS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RtcStatusGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_RTC_STATUS_STRUCT *pStat)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_RTC_STATUS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_RTC_STATUS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pStat), rxBuffer, sizeof(BCM85100_RTC_STATUS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_EdStatusGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_ED_STATUS_STRUCT *pStat)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_ED_STATUS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);


    RETURNED_LENGTH_CHECK(sizeof(BCM85100_ED_STATUS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pStat), rxBuffer, sizeof(BCM85100_ED_STATUS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_BerCountersGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_BER_COUNTERS_STRUCT *pBer)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_BER_COUNTERS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);


    RETURNED_LENGTH_CHECK(sizeof(BCM85100_BER_COUNTERS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pBer), rxBuffer, sizeof(BCM85100_BER_COUNTERS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
// Modem RS_HIST opcode
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemRsHistGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_RS_HIST_GET_STRUCT *pRhs)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_RS_HIST_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_MODEM_RS_HIST_GET_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pRhs), rxBuffer, sizeof(BCM85100_MODEM_RS_HIST_GET_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
// Modem Fifo_Levels_Get opcode
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemFifoLevelsGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_FIFO_LEVELS_GET_STRUCT *pFls)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_FIFO_LEVELS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_MODEM_FIFO_LEVELS_GET_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pFls), rxBuffer, sizeof(BCM85100_MODEM_FIFO_LEVELS_GET_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}


/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemRxMatFilterStatusSet(BYTE deviceIndex, BYTE modemIndex, BYTE matrixIndex, BCM85100_MODEM_RX_MAT_FILTER_STRUCT *matrix)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BCM85100_MODEM_RX_FIR_MSG_STRUCT data;

    data.matrix_index = matrixIndex;
    memcpy(&data.pTPE_Mat, matrix, sizeof(BCM85100_MODEM_RX_MAT_FILTER_STRUCT));
    bcm85100HostApiMutexGet(deviceIndex);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&data),
             sizeof(BCM85100_MODEM_RX_FIR_MSG_STRUCT),
             BCM85100_COM_MODEM_RX_FIR_COEFF_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);

    return (retVal);

}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemTxMatFilterStatusSet(BYTE deviceIndex, BYTE modemIndex, BYTE matrixIndex, BCM85100_MODEM_TX_MAT_FILTER_STRUCT *matrix)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BCM85100_MODEM_TX_FIR_MSG_STRUCT data;

    data.matrix_index = matrixIndex;
    memcpy(&data.pTPE_Mat, matrix, sizeof(BCM85100_MODEM_TX_MAT_FILTER_STRUCT));

    bcm85100HostApiMutexGet(deviceIndex);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&data),
             sizeof(BCM85100_MODEM_TX_FIR_MSG_STRUCT),
             BCM85100_COM_MODEM_TX_FIR_COEFF_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);

    return (retVal);
}


/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_AcmCountersGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_ACM_COUNTERS_STRUCT *pMc)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_ACM_COUNTERS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_ACM_COUNTERS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pMc), rxBuffer, sizeof(BCM85100_ACM_COUNTERS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);

}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcquireCountersGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_ACQUIRE_COUNTERS_STRUCT *pAc)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_ACQUIRE_COUNTERS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_MODEM_ACQUIRE_COUNTERS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pAc), rxBuffer, sizeof(BCM85100_MODEM_ACQUIRE_COUNTERS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemIqImbalanceStatusGet(BYTE deviceIndex,
        BYTE modemIndex,
        BCM85100_MODEM_IQ_IMBALANCE_STATUS_STRUCT *pIs)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_IQ_IMBALANCE_STATUS_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_MODEM_IQ_IMBALANCE_STATUS_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pIs), rxBuffer, sizeof(BCM85100_MODEM_IQ_IMBALANCE_STATUS_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemIqImbalanceTxTxSet(BYTE deviceIndex,
        BYTE modemIndex,
        INT16 diffPhaseTenthsDeg,     // in 0.1 deg resolution
        INT16 diffGainTenthsDB,        // in 0.1 dB resolution
        INT16 dcLeakageITenthsPercent, // in 0.1 percent resolution
        INT16 dcLeakageQTenthsPercent) // in 0.1 percent resolution
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BCM85100_IQ_IMBALANCE_STRUCT iqImbTxTx;

    bcm85100HostApiMutexGet(deviceIndex);
    iqImbTxTx.diffPhaseTenthsDeg = diffPhaseTenthsDeg;
    iqImbTxTx.diffGainTenthsDB = diffGainTenthsDB;
    iqImbTxTx.dcLeakageITenthsPercent = dcLeakageITenthsPercent;
    iqImbTxTx.dcLeakageQTenthsPercent = dcLeakageQTenthsPercent;

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&iqImbTxTx),
             sizeof(BCM85100_IQ_IMBALANCE_STRUCT),
             BCM85100_COM_MODEM_IQ_IMBALANCE_TXTX_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RxSkewSet(BYTE deviceIndex, BYTE modemIndex, INT16 skew)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&skew),
             sizeof(INT16),
             BCM85100_COM_MODEM_RX_SKEW_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_TxSkewSet(BYTE deviceIndex, BYTE modemIndex, INT16 skew)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&skew),
             sizeof(INT16),
             BCM85100_COM_MODEM_TX_SKEW_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemEqualizerStatusGet(BYTE                                   deviceIndex,
        BYTE                                   modemIndex,
        BOOLEAN                                xpicSlave,
        BCM85100_MODEM_EQU_STATUS_STRUCT       *pEs)
{
    BCM85100_ERROR_MSG_ENUM   retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];
    BYTE ix;
    BCM85100_COM_OPCODE_ENUM opcode = 0; //Siklu change
    (void) xpicSlave; // Siklu change

    // split API to 2 iterations
    for (ix = 0; ix < 2; ix++)
    {
        // get array from build send and recive is up to 256 BYTE per transaction
        // therefore must split to two transactions.
		if(ix == 0)
		{
	            opcode = BCM85100_COM_MODEM_EQUALIZER_STATUS_GET0_OPCODE_E;
		}
		else
		{
	            opcode = BCM85100_COM_MODEM_EQUALIZER_STATUS_GET1_OPCODE_E;
		}
        
		bcm85100HostApiMutexGet(deviceIndex);
        retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
                 NULL,
                 0,
                 opcode,
                 modemIndex,
                 rxBuffer,
                 &rxLen,
                 &rxStatus);

        RETURNED_LENGTH_CHECK(40 * sizeof(UINT32));

        if (retVal == BCM85100_SUCCESS_MSG_E)
        {
            switch (ix)
            {
            case 0:
                memcpy(BYTEP_CAST(&pEs->fffTaps_20Bit[0]), rxBuffer, 40 * sizeof(UINT32));
                break;

            case 1:
                memcpy(BYTEP_CAST(&pEs->fffTaps_20Bit[40]), rxBuffer, 40 * sizeof(UINT32));
                break;
            }
        }
        else
        {
            break;
        }

    }

    for (ix = 0; ix < 48; ix++)
    {
        pEs->fffTaps[ix] = 0;
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}



/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RxPowerGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_POWER_MANAG_STRUCT *pPowerM)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_RX_POWER_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);


    RETURNED_LENGTH_CHECK(sizeof(BCM85100_POWER_MANAG_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pPowerM), rxBuffer, sizeof(BCM85100_POWER_MANAG_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}


/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_AcmConfigurationGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_ACM_CONFIGURATION_STRUCT *pCs)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_ACM_CONFIGURATION_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_ACM_CONFIGURATION_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pCs), rxBuffer, sizeof(BCM85100_ACM_CONFIGURATION_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmProfileMaskGet(BYTE   deviceIndex,
        BYTE   modemIndex,
        UINT16 *profilesMask)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_ACM_PROFILE_MASK_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(UINT16));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(profilesMask), rxBuffer, sizeof(UINT16));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
// Status / Performance Monitoring Functions - networking
//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_SgmiiStatusGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_SGMII_STATUS_GET_STRUCT *pData)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_NET_ETH_PM_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_SGMII_STATUS_GET_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        // get ethernet PM
        memcpy(BYTEP_CAST(pData), rxBuffer, sizeof(BCM85100_SGMII_STATUS_GET_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_GetRxPacketGfpCrcErrCnt(BYTE deviceIndex,
        BYTE modemIndex,
        UINT16 *pPcktCnt)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_RX_PCKT_GFP_CRC_ERR_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(UINT16));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pPcktCnt), rxBuffer, sizeof(UINT16));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_GetTxPacketGfpCrcErrCnt(BYTE deviceIndex,
        BYTE modemIndex,
        UINT16 *pPcktCnt)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_TX_PCKT_GFP_CRC_ERR_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(UINT16));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pPcktCnt), rxBuffer, sizeof(UINT16));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_SgmiiConfigure(BYTE deviceIndex, BYTE modemIndex, BCM85100_SGMII_CONFIGURE_STRUCT *config)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(config),
             sizeof(BCM85100_SGMII_CONFIGURE_STRUCT),
             BCM85100_COM_NET_ETH_PM_CONFIG_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
// Operative Functions (general)
//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_DeviceReset(BYTE deviceIndex, BCM85100_DEVICE_RESET_TYPE_ENUM resetType)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;

    if (resetType != BCM85100_DEVICE_RESET_ALL_E)
    {
        return BCM85100_API_INVALID_PARAMS_E;
    }

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSend(deviceIndex,
                                       NULL,
                                       0,
                                       BCM85100_COM_MC_RESET_OPCODE_E,
                                       0,
                                       BCM85100_SPI_NO_BOOTFW);
    // Give it to boot
    bcm85100HostSleep(BCM85100_RESET_TIMEOUT);

    //Hw hook for host to implement
    bcm85100HostDeviceReset(deviceIndex);

    bcm85100HostApiMutexRelease(deviceIndex);

    return (retVal);
}



//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
// Operative Functions (Modem)
//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcquireStart(BYTE                                deviceIndex,
        BYTE                                modemIndex,
        BCM85100_ACQUIRE_MODE_ENUM            acqMode,
        BCM85100_SPECTRAL_INVERSION_MODE_ENUM siMode)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    BCM85100_COM_ACQUIRE_STRUCT  acquireParams;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    acquireParams.acqMode =  BYTE_CAST(acqMode);
    acquireParams.siMode  =  BYTE_CAST(siMode);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&acquireParams),
             sizeof(BCM85100_COM_ACQUIRE_STRUCT),
             BCM85100_COM_ACQUIRE_START_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ExtPwmOverwrite(BYTE                           deviceIndex,
        BYTE                           modemIndex,
        BCM85100_PWM_OVERWRITE_STRUCT  pwmSt)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&pwmSt),
             sizeof(BCM85100_PWM_OVERWRITE_STRUCT),
             BCM85100_COM_PWM_OVERWRITE_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ExtClockConfig(BYTE                           deviceIndex,
        BYTE                           modemIndex,
        BCM85100_EXT_CLOCK_CFG_STRUCT  extClk)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&extClk),
             sizeof(BCM85100_EXT_CLOCK_CFG_STRUCT),
             BCM85100_COM_EXT_CLOCK_CFG_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_SetPrbsMode(BYTE deviceIndex, BYTE modemIndex)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_PRBS_MODE_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_BerCountersClear(BYTE     deviceIndex,
        BYTE     modemIndex)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_BER_COUNTERS_CLEAR_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_BerMonitorEnable(BYTE deviceIndex, BYTE modemIndex, BOOLEAN enableMonitor)
{
    BCM85100_ERROR_MSG_ENUM  retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM  rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&enableMonitor),
             sizeof(BYTE),
             BCM85100_COM_BER_MONITOR_ENABLE_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);

    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemPredistortionCoefsSet(BYTE                                deviceIndex,
        BYTE                                modemIndex,
        BCM85100_MODEM_PREDISTORTION_STRUCT *pPs)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(pPs),
             sizeof(BCM85100_MODEM_PREDISTORTION_STRUCT),
             BCM85100_COM_MODEM_PREDISTORTION_COEFS_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemTxSignalGainSet(BYTE deviceIndex, BYTE modemIndex, BCM85100_MODEM_TX_SIGNAL_GAIN_STRUCT *pRhs)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(pRhs),
             sizeof(BCM85100_MODEM_TX_SIGNAL_GAIN_STRUCT),
             BCM85100_COM_MODEM_SIGNAL_GAIN_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
// set the rx frequency offset
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RxFreqOffsetSet(BYTE  deviceIndex,
        BYTE  modemIndex,
        INT32 offsetInHz)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&offsetInHz),
             sizeof(INT32),
             BCM85100_COM_MODEM_RX_FREQ_OFFSET_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
// set the tx frequency offset
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_TxFreqOffsetSet(BYTE  deviceIndex,
        BYTE  modemIndex,
        INT32 offsetInHz)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&offsetInHz),
             sizeof(INT32),
             BCM85100_COM_MODEM_TX_FREQ_OFFSET_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------
// Operative Functions (ACM)
//----------------------------------------------------
//----------------------------------------------------
//----------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmEngineEnable(BYTE deviceIndex, BYTE modemIndex, BOOLEAN enableRxSensors, BOOLEAN enableTxSwitch)
{
    BCM85100_ERROR_MSG_ENUM  retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM  rxStatus;
    BCM85100_ACM_ENABLE_STRUCT  acmEnable;

    bcm85100HostApiMutexGet(deviceIndex);
    acmEnable.enableAcmRx = enableRxSensors;
    acmEnable.enableAcmTx = enableTxSwitch;
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&acmEnable),
             sizeof(BCM85100_ACM_ENABLE_STRUCT),
             BCM85100_COM_ACM_ENGINE_ENABLE_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmProfileEnable(BYTE   deviceIndex,
        BYTE   modemIndex,
        UINT16 profilesMask)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&profilesMask),
             sizeof(UINT16),
             BCM85100_COM_ACM_PROFILE_ENABLE_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmSwitch(BYTE                           deviceIndex,
        BYTE                           modemIndex,
        BCM85100_ACM_PROFILE_ENUM      acmProfile,
        BOOLEAN                        isLocal)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    BCM85100_COM_ACM_SWITCH_STRUCT       acmSwitchParam;

    bcm85100HostApiMutexGet(deviceIndex);
    acmSwitchParam.acmProfile = BYTE_CAST(acmProfile);
    acmSwitchParam.isLocal    = isLocal;

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&acmSwitchParam),
             sizeof(BCM85100_COM_ACM_SWITCH_STRUCT),
             BCM85100_COM_ACM_SWITCH_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmImproveIfNeeded(BYTE   deviceIndex,
        BYTE   modemIndex)

{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal =  bcm85100HostCommBuildSendAndReceive(deviceIndex,
              0,
              0,
              BCM85100_COM_ACM_IMPROVE_OPCODE_E,
              modemIndex,
              NULL,
              &rxLen,
              &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
// set acm Engine Threshold value
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmThresholdSet(BYTE                       deviceIndex,
        BYTE                       modemIndex,
        BCM85100_ACM_TABLE_ENUM    engineTable,
        BCM85100_ACM_PROFILE_ENUM  profile,
        INT16                      mseTenthsVal,
        BYTE                       ldpcMantissaTenths,
        BYTE                       ldpcExponent,
        BCM85100_ACM_CRITERIA_ENUM criteria,
        BCM85100_ACM_PROFILE_ENUM  nextProfile)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    (void) ldpcMantissaTenths; // Siklu change
    (void) ldpcExponent;

    BCM85100_ACM_THRESHOLD_STRUCT acmThresholdSetStruct;

    bcm85100HostApiMutexGet(deviceIndex);

    acmThresholdSetStruct.dirDegradeImprove  = BYTE_CAST(engineTable);
    acmThresholdSetStruct.profile            = BYTE_CAST(profile);
    acmThresholdSetStruct.mseTenthsVal       = mseTenthsVal;
    acmThresholdSetStruct.criteria           = BYTE_CAST(criteria);
    acmThresholdSetStruct.nextProfile        = BYTE_CAST(nextProfile);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&acmThresholdSetStruct),
             sizeof(BCM85100_ACM_THRESHOLD_STRUCT),
             BCM85100_COM_ACM_THRESHOLD_SET_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);

}

/////////////////////////////////////////////////////////////////////////////////
// Get ACM Engine Threshold value
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemAcmThresholdGet(BYTE                        deviceIndex,
        BYTE                        modemIndex,
        BCM85100_ACM_TABLE_ENUM     engineTable,
        BCM85100_ACM_PROFILE_ENUM   profile,
        INT16                      *mseTenthsVal,
        BYTE                       *ldpcMantissaTenths,
        BYTE                       *ldpcExponent,
        BCM85100_ACM_CRITERIA_ENUM *criteria,
        BCM85100_ACM_PROFILE_ENUM  *nextProfile)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    BCM85100_ACM_THRESHOLD_STRUCT acmThresholdGetStruct;

    bcm85100HostApiMutexGet(deviceIndex);
    acmThresholdGetStruct.dirDegradeImprove = BYTE_CAST(engineTable);
    acmThresholdGetStruct.profile           = BYTE_CAST(profile);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&acmThresholdGetStruct),
             sizeof(BCM85100_ACM_THRESHOLD_STRUCT),
             BCM85100_COM_ACM_THRESHOLD_GET_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_ACM_THRESHOLD_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(&acmThresholdGetStruct), rxBuffer, sizeof(BCM85100_ACM_THRESHOLD_STRUCT));
        *mseTenthsVal       = acmThresholdGetStruct.mseTenthsVal;
        *ldpcMantissaTenths = 0;
        *ldpcExponent       = 0;
        *criteria           = (BCM85100_ACM_CRITERIA_ENUM)acmThresholdGetStruct.criteria;
        *nextProfile        = (BCM85100_ACM_PROFILE_ENUM)acmThresholdGetStruct.nextProfile;
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_MailboxMessageSend(BYTE deviceIndex, BYTE modemIndex, BCM85100_MAILBOX_MESSAGE_STRUCT *pMbx)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(pMbx),
             (UINT32)(pMbx->msgLen + sizeof(pMbx->msgLen)),
             BCM85100_COM_MAILBOX_MESSAGE_SEND_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_MailboxMessageReceive(BYTE deviceIndex, BYTE modemIndex, BCM85100_MAILBOX_MESSAGE_STRUCT *pMbx)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MAILBOX_MESSAGE_RECEIVE_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_MAILBOX_MESSAGE_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pMbx), rxBuffer, sizeof(BCM85100_MAILBOX_MESSAGE_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_TxRxOperationStart(BYTE     deviceIndex,
        BYTE     modemIndex)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_TX_RX_START_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_AuxDacSet(BYTE deviceIndex, BYTE modemIndex, BYTE dacIndex, UINT16 dacValue)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BCM85100_AUX_DAC_STRUCT auxDacStruct;

    bcm85100HostApiMutexGet(deviceIndex);

    auxDacStruct.dacIndex = dacIndex;
    auxDacStruct.dacValue = dacValue;

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&auxDacStruct),
             sizeof(BCM85100_AUX_DAC_STRUCT),
             BCM85100_AUX_DAC_SET_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);

    return (retVal);
}


/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_TransmitterModeSet(BYTE                           deviceIndex,
        BYTE                           modemIndex,
        BCM85100_TRANSMITTER_MODE_STRUCT *tx)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(tx),
             sizeof(BCM85100_TRANSMITTER_MODE_STRUCT),
             BCM85100_COM_TRANSMITTER_MODE_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_TransmitterModeGet(BYTE                           deviceIndex,
        BYTE                           modemIndex,
        BCM85100_TRANSMITTER_MODE_STRUCT *tx)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_TRANSMITTER_MODE_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_TRANSMITTER_MODE_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(tx), rxBuffer, sizeof(BCM85100_TRANSMITTER_MODE_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_LoopbackSet(BYTE                   deviceIndex,
        BYTE                   modemIndex,
        BCM85100_LOOPBACK_STRUCT *lb)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(lb),
             sizeof(BCM85100_LOOPBACK_STRUCT),
             BCM85100_COM_LOOPBACK_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_LoopbackGet(BYTE                   deviceIndex,
        BYTE                   modemIndex,
        BCM85100_LOOPBACK_STRUCT *lb)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_LOOPBACK_GET_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_LOOPBACK_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(lb), rxBuffer, sizeof(BCM85100_LOOPBACK_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RxOfTxOverTheAirParamsSet(BYTE deviceIndex, BYTE modemIndex, BCM85100_RX_TX_API_STRUCT *tx)
{
    BCM85100_ERROR_MSG_ENUM  retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM   rxStatus;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(tx),
             sizeof(BCM85100_RX_TX_API_STRUCT),
             BCM85100_COM_RX_TX_SET_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);

    return (retVal);
}


/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RxOfTxOverTheAirParamsGet(BYTE deviceIndex, BYTE modemIndex, BCM85100_RX_TX_API_STRUCT *rx)
{
    BCM85100_ERROR_MSG_ENUM retVal = BCM85100_SUCCESS_MSG_E;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM   rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_RX_TX_GET_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_RX_TX_API_STRUCT));



    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(rx), rxBuffer, sizeof(BCM85100_RX_TX_API_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);

    return (retVal);
}


/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RtcModeSet(BYTE deviceIndex, BYTE modemIndex, BCM85100_RTC_MODE_ENUM mode)
{
    BCM85100_ERROR_MSG_ENUM  retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM   rxStatus;
    BYTE byteMode = (BYTE) mode;

    bcm85100HostApiMutexGet(deviceIndex);
    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             BYTEP_CAST(&byteMode),
             sizeof(BYTE),
             BCM85100_COM_RTC_MODE_SET_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);

    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_ModemSamplesGet(BYTE deviceIndex, BYTE modemIndex, UINT16 numIqPairs, INT16 *pBuffer, BCM85100_IQ_SAMPLES_SEL_ENUM inputSel)
{
    BCM85100_ERROR_MSG_ENUM             retVal = BCM85100_SUCCESS_MSG_E;
    BCM85100_COM_SAMPLES_REC_STRUCT     iqSamplesRecParams;
    UINT16                              iqSamplesGetParams;
    UINT16                              numIqPairsChuckSize = BCM85100_MAX_PAIRS_IN_CHUNK;
    UINT16                              bufferIndex = 0;
    UINT32                              rxLen;
    BCM85100_ERROR_MSG_ENUM             rxStatus;
    INT16                               rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    if (numIqPairs > BCM85100_MAX_NUMBER_OF_PAIRS)
    {
        return (BCM85100_IQ_SAMPLES_GET_ERROR_E);
    }
    else if ((inputSel == BCM85100_DAC_OUT_E) && (numIqPairs > BCM85100_MAX_DAC_NUMBER_OF_PAIRS))
    {
        return (BCM85100_IQ_SAMPLES_GET_ERROR_E);
    }
    else if ((inputSel == BCM85100_ED_OUT_E) && (numIqPairs > BCM85100_MAX_ED_NUMBER_OF_PAIRS))
    {
        return (BCM85100_IQ_SAMPLES_GET_ERROR_E);
    }

    bcm85100HostApiMutexGet(deviceIndex);

    iqSamplesRecParams.numIqPairs  = numIqPairs;
    iqSamplesRecParams.testBusSel  = BYTE_CAST(inputSel);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             (BYTE*)&iqSamplesRecParams,
             sizeof(BCM85100_COM_SAMPLES_REC_STRUCT),
             BCM85100_COM_MODEM_SAMPLES_REC_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    if (retVal != BCM85100_SUCCESS_MSG_E)
    {
        bcm85100HostApiMutexRelease(deviceIndex);
        return (retVal);
    }

    // start receiving data, assuming that by the time the msgs
    // reach destination, all required data has been recorded already,
    // otherwise, return with BCM85100_IQ_SAMPLES_GET_ERROR_E

    if ((inputSel == BCM85100_DAC_OUT_E) || (inputSel == BCM85100_ED_OUT_E))
    {
        numIqPairsChuckSize = BCM85100_TXTX_ED_DAC_MAX_PAIRS_IN_CHUNK;
    }

    while (numIqPairs > 0)
    {
        iqSamplesGetParams = MIN(numIqPairs , numIqPairsChuckSize);

        retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
                 (BYTE*)&iqSamplesGetParams,
                 sizeof(UINT16),
                 BCM85100_COM_MODEM_SAMPLES_GET_OPCODE_E,
                 modemIndex,
                 (BYTE*)rxBuffer,
                 &rxLen,
                 &rxStatus);

        RETURNED_LENGTH_CHECK(iqSamplesGetParams * 2 * sizeof(UINT16));

        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            bcm85100HostApiMutexRelease(deviceIndex);
            return (retVal);
        }

        memcpy(pBuffer + bufferIndex, rxBuffer, iqSamplesGetParams * 2 * sizeof(INT16));

        bufferIndex += (iqSamplesGetParams) * 2;
        if (numIqPairs < numIqPairsChuckSize)
        {
            numIqPairs = 0; // out of while loop
        }
        else
        {
            numIqPairs -= numIqPairsChuckSize;
        }
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_GeneralControlSet(BYTE            deviceIndex,
        BYTE            modemIndex,
        UINT32          opcode,
        BYTE            length,
        BYTE            *data)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    BCM85100_COM_GENERAL_CONTROL_STRUCT generalControl;

    bcm85100HostApiMutexGet(deviceIndex);
    generalControl.opcode = (UINT16)opcode;
    length = MIN(length, sizeof(generalControl.pDataBuffer));
    generalControl.length = (UINT16)length;
    memcpy((BYTE*)generalControl.pDataBuffer, data, length);

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             (BYTE *)&generalControl,
             (BYTE)(BCM85100_COM_GENERAL_CONTROL_HEADER_SIZE + length),
             BCM85100_COM_GENERAL_CONTROL_SET_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

/////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RxSlicerInGet(BYTE deviceIndex, BYTE modemIndex, UINT16 NbrIQSamp, INT16 *pBuffer, BCM85100_SLICER_IN_STRUCT *pSlicerIn)
{
    BCM85100_ERROR_MSG_ENUM             retVal = BCM85100_SUCCESS_MSG_E;
    UINT16                              iqSamplesGetParams;
    UINT16                              numIqPairsChuckSize = BCM85100_EQU_OUT_MAX_PAIRS_IN_CHUNK;
    UINT32                              rxLen;
    BCM85100_ERROR_MSG_ENUM             rxStatus;
    INT16                               rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];
    BYTE                                slicInfoBuff[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];
    BYTE                                nofCycles = 0;
    UINT16                              numIqPairs = NbrIQSamp;
    UINT16                              bufferIndex = 0;

    if (numIqPairs > BCM85100_MAX_EQU_OUT_NUMBER_OF_PAIRS)
    {
        return (BCM85100_IQ_SAMPLES_GET_ERROR_E);
    }

    bcm85100HostApiMutexGet(deviceIndex);

    while (nofCycles < BCM85100_EQU_OUT_RECORD_MAX_TRIES)
    {
        retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
                 (BYTE*)&numIqPairs,
                 sizeof(UINT16),
                 BCM85100_COM_MODEM_SLICER_REC_OPCODE_E,
                 modemIndex,
                 NULL,
                 &rxLen,
                 &rxStatus);

        if (retVal != BCM85100_SUCCESS_MSG_E)
        {
            bcm85100HostApiMutexRelease(deviceIndex);
            return (retVal);
        }

        // start receiving data, assuming that by the time the msgs
        // reach destination, all required data has been recorded already,
        // otherwise, return with BCM85100_IQ_SAMPLES_GET_ERROR_E

        while (numIqPairs > 0)
        {
            iqSamplesGetParams = MIN(numIqPairs , numIqPairsChuckSize);

            retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
                     (BYTE*)&iqSamplesGetParams,
                     sizeof(UINT16),
                     BCM85100_COM_MODEM_SLICER_SAMPLES_GET_OPCODE_E,
                     modemIndex,
                     (BYTE*)rxBuffer,
                     &rxLen,
                     &rxStatus);

            RETURNED_LENGTH_CHECK(iqSamplesGetParams * 2 * sizeof(UINT16));

            if (retVal == BCM85100_CAPTURE_ERROR_PREAMBLE_E)
            {
                bufferIndex = 0;
                numIqPairs = NbrIQSamp;
                nofCycles++;
                break;
            }

            if (retVal != BCM85100_SUCCESS_MSG_E)
            {
                bcm85100HostApiMutexRelease(deviceIndex);
                return (retVal);
            }

            memcpy(pBuffer + bufferIndex, rxBuffer, iqSamplesGetParams * 2 * sizeof(INT16));

            bufferIndex += (iqSamplesGetParams) * 2;
            if (numIqPairs < numIqPairsChuckSize)
            {
                numIqPairs = 0; // out of while loop
            }
            else
            {
                numIqPairs -= numIqPairsChuckSize;
            }
        }

        if (numIqPairs == 0)
        {
            break;
        }
    }

    if (nofCycles == BCM85100_EQU_OUT_RECORD_MAX_TRIES)
    {
        return BCM85100_CAPTURE_ERROR_PREAMBLE_E;
    }

    //Get Slicer Info

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             NULL,
             0,
             BCM85100_COM_MODEM_SLICER_INFO_GET_OPCODE_E,
             modemIndex,
             slicInfoBuff,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(BCM85100_SLICER_IN_STRUCT));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(BYTEP_CAST(pSlicerIn), slicInfoBuff, sizeof(BCM85100_SLICER_IN_STRUCT));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION: BCM85100_API_registerRead
//
//  PARAMETERS:
//              modemIndex - [in] modem index
//              offset     - [in] register offset from modem base address
//              pVal       - [out] register value
//
//  DESCRIPTION:
//
//  RETURNS:    BCM85100_ERROR_MSG_ENUM
///////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RegisterRead(BYTE                deviceIndex,
        BYTE                modemIndex,
        UINT16              offset,
        UINT16              *pVal)
{
    BCM85100_ERROR_MSG_ENUM               retVal;
    BCM85100_COM_READ_REGISTER_STRUCT     registerToRead;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;
    BYTE rxBuffer[BCM85100_HOST_PROTOCOL_MAX_MSG_LEN];

    bcm85100HostApiMutexGet(deviceIndex);

    registerToRead.offset    = offset;

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             (BYTE *)&registerToRead,
             sizeof(BCM85100_COM_READ_REGISTER_STRUCT),
             BCM85100_COM_REGISTER_READ_OPCODE_E,
             modemIndex,
             rxBuffer,
             &rxLen,
             &rxStatus);

    RETURNED_LENGTH_CHECK(sizeof(UINT16));

    if (retVal == BCM85100_SUCCESS_MSG_E)
    {
        memcpy(pVal, rxBuffer, sizeof(UINT16));
    }

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION: BCM85100_API_registerWrite
//
//  PARAMETERS:
//              modemIndex - [in] modem index
//              offset     - [in] register offset from modem base address
//              val        - [in] register value
//
//  DESCRIPTION:
//
//  RETURNS:    BCM85100_ERROR_MSG_ENUM
///////////////////////////////////////////////////////////////////////////////
BCM85100_ERROR_MSG_ENUM BCM85100_API_RegisterWrite(BYTE               deviceIndex,
        BYTE               modemIndex,
        UINT16             offset,
        UINT16             val)
{
    BCM85100_ERROR_MSG_ENUM retVal;
    UINT32 rxLen;
    BCM85100_ERROR_MSG_ENUM rxStatus;

    BCM85100_COM_WRITE_REGISTER_STRUCT registerToWrite;

    bcm85100HostApiMutexGet(deviceIndex);
    registerToWrite.offset      = offset;
    registerToWrite.value       = val;

    retVal = bcm85100HostCommBuildSendAndReceive(deviceIndex,
             (BYTE *)&registerToWrite,
             sizeof(BCM85100_COM_WRITE_REGISTER_STRUCT),
             BCM85100_COM_REGISTER_WRITE_OPCODE_E,
             modemIndex,
             NULL,
             &rxLen,
             &rxStatus);

    bcm85100HostApiMutexRelease(deviceIndex);
    return (retVal);
}


