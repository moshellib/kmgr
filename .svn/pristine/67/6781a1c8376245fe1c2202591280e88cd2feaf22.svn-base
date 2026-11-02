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

#ifndef DEFS_100_H
#define DEFS_100_H

//--------------------------------------------------------------------------------------------
//define PACK usage - compiler wont add padding to defined structs (important for MSG structs)
//--------------------------------------------------------------------------------------------

#ifdef IAR
    #define __PACK_PREFIX__   __packed
    #define __PACK__   
#else
    #define __PACK_PREFIX__
    #define __PACK__          __attribute__((packed))
#endif


//--------------------------------------------------------------------------------------------

#ifdef WIN32
    #ifndef TRUE
        #define TRUE    1
    #endif
    #ifndef FALSE
        #define FALSE   0
    #endif

    typedef unsigned char       BYTE;
    typedef unsigned char       BOOLEAN;
    typedef signed char         INT8;
    typedef unsigned char       UINT8;
    typedef unsigned short      UINT16;
    typedef signed short        INT16;
    typedef unsigned int        UINT32;
    typedef signed int          INT32;
    typedef __int64             INT64;
    typedef unsigned __int64    UINT64;

#else
    #ifndef TRUE
        #define TRUE    1
    #endif
    #ifndef FALSE
        #define FALSE   0
    #endif
    #ifndef NULL
        #define NULL    0
    #endif
    typedef unsigned char       BYTE;
    typedef unsigned char       BOOLEAN;
#ifndef _GL_TYPES_H
    typedef signed char         INT8;
    typedef unsigned char       UINT8;
    typedef unsigned short      UINT16;
    typedef signed short        INT16;
    typedef unsigned int        UINT32;
    typedef signed int          INT32;
    typedef unsigned long long  UINT64;
    typedef long long           INT64;
#endif // _GL_TYPES_H

#endif


typedef enum {
    FAIL_E      = 0,
    SUCCESS_E   = 1
} RETURN_ENUM;

//the following should be consistent with the constant in frmOnlineTool.frm
typedef enum {
    MC_E
} CPU_TYPE_ENUM;


#ifndef BCM85100_DEVICES_NUM
    #define BCM85100_DEVICES_NUM      (1) //only support one device at this release
#endif

#ifndef MAX
    #define MAX(A,B) ((A)>(B) ? (A):(B))
#endif

#ifndef MIN
    #define MIN(A,B) ((A)<(B) ? (A):(B))
#endif

#ifndef ABS
    #define  ABS(A)  (((A) < 0) ? -(A):(A))
#endif

#ifndef SIGN
    #define SIGN(A)     ((A<0) ? (-1) : (1))
#endif

#define MASK_BIT_0          0x1
#define MASK_BIT_1          0x2
#define MASK_BIT_2          0x4
#define MASK_BIT_3          0x8
#define MASK_BIT_4         0x10
#define MASK_BIT_5         0x20
#define MASK_BIT_6         0x40
#define MASK_BIT_7         0x80
#define MASK_BIT_8        0x100
#define MASK_BIT_9        0x200
#define MASK_BIT_10       0x400
#define MASK_BIT_11       0x800
#define MASK_BIT_12      0x1000
#define MASK_BIT_13      0x2000
#define MASK_BIT_14      0x4000
#define MASK_BIT_15      0x8000
#define MASK_BIT_16     0x10000
#define MASK_BIT_17     0x20000
#define MASK_BIT_18     0x40000
#define MASK_BIT_19     0x80000
#define MASK_BIT_20    0x100000
#define MASK_BIT_21    0x200000
#define MASK_BIT_22    0x400000
#define MASK_BIT_23    0x800000
#define MASK_BIT_24   0x1000000
#define MASK_BIT_25   0x2000000
#define MASK_BIT_26   0x4000000
#define MASK_BIT_27   0x8000000
#define MASK_BIT_28  0x10000000
#define MASK_BIT_29  0x20000000
#define MASK_BIT_30  0x40000000
#define MASK_BIT_31  0x80000000

//SIGNEXT MACROS (for drivers)
#define GETMASK16(n)        (((UINT16)0x1<<(n-1)) + (((UINT16)0x1<<(n-1))-1))   //return mask at the size of input (2^n-1), 1<input<16
#define GETMASK32(n)        (((UINT32)0x1<<(n-1)) + (((UINT32)0x1<<(n-1))-1))   //return mask at the size of input (2^n-1), 1<input<32
#define GETMASK64(n)        (((UINT64)0x1<<(n-1)) + (((UINT64)0x1<<(n-1))-1))   //1<input<64
#define SIGNEXT16(x,k)      ((INT16)((x & GETMASK32(k)) << (16-k)) >> (16-k))   //x=input, k=input size (highest bit)
#define SIGNEXT32(x,k)      ((INT32)((x & GETMASK32(k)) << (32-k)) >> (32-k))
#define SIGNEXT64(x,k)      ((INT64)((x & GETMASK64(k)) << (64-k)) >> (64-k))

#endif //DEFS_H

