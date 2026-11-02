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


#ifndef _HOST_DRIVER_H_
#define _HOST_DRIVER_H_

#include "defs85100.h"
#include "bcm85100_api.h"


#ifdef __cplusplus
extern "C" {
#endif


#define SPI_CLOCK_MODE_0 0
#define SPI_CLOCK_MODE_3 1
/*------------------------------------------------------
    Init functions
------------------------------------------------------*/
BOOLEAN bcm85100HostOpen                    (void);
BOOLEAN bcm85100HostClose                   (void);

/*------------------------------------------------------
    General functions
------------------------------------------------------*/
void    bcm85100HostDeviceReset             (BYTE deviceIndex);
void    bcm85100HostSleep                   (UINT32 sleepTimeMicro);
UINT32  bcm85100HostTickGet                 (void);
void    bcm85100HostApiMutexGet             (BYTE deviceIndex);
void    bcm85100HostApiMutexRelease         (BYTE deviceIndex);

/*------------------------------------------------------
    UART functions
------------------------------------------------------*/
void    bcm85100HostUartDataSend           (BYTE deviceIndex, BYTE *pBuff, UINT32 buffLen);
void    bcm85100HostUartDataReceive        (BYTE deviceIndex, BYTE *pBuff, UINT32 buffLen, UINT16 timeoutMilli, UINT32 *pReadBytes);
void    bcm85100HostUartReceiverFlush      (BYTE deviceIndex);

/*------------------------------------------------------
    SPI functions
------------------------------------------------------*/
void    bcm85100HostSpiDataSend            (BYTE deviceIndex, BYTE *pTxBuff, UINT32 buffLen);
void    bcm85100HostSpiDataReceive         (BYTE deviceIndex, BYTE *pTxBuff, BYTE *pRxBuff, UINT32 buffLen);
void	bcm85100HostSpiModeSet			   (BYTE deviceIndex, INT32 mode);
void    bcm85100HostSpiMaxRateSet		   (BYTE deviceIndex, UINT32 maxRateHz);
BOOLEAN bcm85100HostSpiIsDataReady		   (BYTE deviceIndex);
BOOLEAN bcm85100HostSpiDataReadyWait	   (BYTE deviceIndex, UINT32 timeoutMilli);
//void    bcm85100SetSpiClockMode (BYTE clockMode0  ); Siklu change
//BYTE    bcm85100GetSpiClockMode (void);

/*------------------------------------------------------
    MII functions
------------------------------------------------------*/
void    bcm85100HostMIIDataWrite           (BYTE deviceIndex, BYTE *pBuff, UINT32 buffLen);
BOOLEAN bcm85100HostMIIDataRead            (BYTE deviceIndex, BYTE *pBuff, UINT32 *pBuffLen, UINT32 timeoutMilli);

#ifdef __cplusplus
}
#endif

#endif  /*_HOST_DRIVER_H_*/



