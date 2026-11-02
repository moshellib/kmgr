#include <string.h>
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

#include "utils.h"

///////////////////////////////////////////////////////////////////////////////
// perform checksum calc on the buffer - returns the result.
///////////////////////////////////////////////////////////////////////////////
UINT32 bcm85100ChecksumCalc(
                 UINT32     currentChecksum,
                 BYTE      *pBuf,
                 UINT32     len,
                 BOOLEAN    lastCall)
{
    UINT32   ix, tmp, remainder;

    // check if endian conversion
    if (bcm85100IsBigEndian())
    {
        // calculate on whole number of DWORDs
        for (ix=0; ix < len / 4; ix++)
        {
            tmp = bcm85100EndianConversion((BYTE*)(pBuf+ix*sizeof(UINT32)), sizeof(UINT32));
            currentChecksum += tmp;
        }

        // calculate on whatever's left
        remainder = len % 4;
        if (remainder)
        {
            tmp = bcm85100EndianConversion(&pBuf[len-remainder], remainder);
            currentChecksum += tmp;
        }

        if (lastCall)
        {
            currentChecksum ^= 0xDEADBEEF;
        }

        currentChecksum = bcm85100EndianConversion((BYTE*)&currentChecksum, sizeof(UINT32));
    }
    else
    {
        // calculate on whole number of DWORDs
        for (ix=0; ix < len / 4; ix++)
        {
            memcpy((BYTE*)&tmp, (BYTE*)(pBuf+ix*sizeof(UINT32)), sizeof(UINT32));
            currentChecksum += tmp;
        }

        // calculate on whatever's left
        remainder = len % 4;
        if (remainder)
        {
            tmp = 0;
            memcpy((void*)&tmp, &pBuf[len-remainder], remainder);
            currentChecksum += tmp;
        }

        if (lastCall)
        {
            currentChecksum ^= 0xDEADBEEF;
        }
    }

    return(currentChecksum);
}

///////////////////////////////////////////////////////////////////////////////
// Checks if the CPU endianty is a big endian
///////////////////////////////////////////////////////////////////////////////
BOOLEAN bcm85100IsBigEndian()
{
    UINT32   tmp = 1;
    BYTE     *ptr = (BYTE*)&tmp;

    // check if big endian
    if (*ptr == 0)
    {
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
UINT32 bcm85100EndianConversion(BYTE *pBuf, UINT32 size)
{
    UINT32   tmp = 0;
    BYTE     *ptr = (BYTE*)&tmp;

    ptr[3] = pBuf[0];
    if (size > 1)     ptr[2] = pBuf[1];
    if (size > 2)     ptr[1] = pBuf[2];
    if (size > 3)     ptr[0] = pBuf[3];

    return tmp;
}


///////////////////////////////////////////////////////////////////////////////
// This Function swap the  bytes of a  word (for Big / Little Endian support )
///////////////////////////////////////////////////////////////////////////////
UINT16  bcm85100EndianSwapBytes (BYTE *pBuf)
{
	UINT16   tmp = 0;
	BYTE     *ptr = (BYTE*)&tmp;

	ptr[1] = pBuf[0];
	ptr[0]=  pBuf[1];

	return tmp;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
BYTE bcm85100BitCount(UINT32 num)
{
    BYTE count;

    for (count = 0; num != 0; num &= num - 1)
    {
        count++;
    }

    return count;
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
INT32 bcm85100atoi(const char* asc, UINT32 len)
{
    UINT32   sign = 0, i = 0;
    INT32 val = 0;

    for (; i < len; i++)
    {
       if(asc[i] >= '0' && asc[i] <= '9')
              val = val * 10 + asc[i] -'0';
       else if(asc[0] == '-' && i==0) 
          sign =1;
       else 
         break;
    }
    if(sign == 1)
        val = val * -1;
    return val;
}




/////////////////////////////////////////////////////////////////////////////////
//bcm85100EndianShortConversion

void  bcm85100EndianShortConversion (UINT16  *word ,UINT16 *dest )
{
  
 
  BYTE *word_ptr=(BYTE *)word,*convert_ptr=(BYTE *)dest;
  
  *(convert_ptr) = *(word_ptr +1 );
  *(convert_ptr+ 1) = *(word_ptr);
                                            
 

}

