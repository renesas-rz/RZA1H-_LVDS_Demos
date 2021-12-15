/******************************************************************************
* DISCLAIMER

* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized.


* This software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.

* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES
* REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY,
* INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE EXPRESSLY
* DISCLAIMED.

* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS

* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
* FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS
* AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

* Renesas reserves the right, without notice, to make changes to this
* software and to discontinue the availability of this software.  
* By using this software, you agree to the additional terms and
* conditions found by accessing the following link:
* http://www.renesas.com/disclaimer
******************************************************************************
* Copyright (C) 2010 Renesas Electronics Corporation. All rights reserved.
******************************************************************************
* File Name    : cc.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : FreeRTOS
* H/W Platform : RSK+
* Description  : Compiler specific include file
******************************************************************************
* History : DD.MM.YYYY Version Description
*         : 04.02.2010 1.00    First Release
******************************************************************************/

#ifndef CC_H_INCLUDED
#define CC_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "r_typedefs.h"
#include "trace.h"

/*****************************************************************************
Typedefs
******************************************************************************/

/* This must be from a misconception about which data types scale with the
   architecture of the CPU or from compilers with bugs in. The type definitions
   here should work on all compilers */
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned long u32_t;
/*
* Renesas Electronics Corporation.
* 29.03.2017 Forced type s8_t to signed char as by default char can
             be defined as signed or unsigned by the compiler
*/
typedef signed char s8_t;
typedef short s16_t;
typedef long s32_t;
/* This should be fastest - provided alignment is correct */
typedef unsigned long mem_ptr_t;

/* Print formatters */
#define U16_F                       "hu"
#define S16_F                       "d"
#define X16_F                       "hX"
#define U32_F                       "u"
#define S32_F                       "d"
#define X32_F                       "X"
#define SZT_F                       "uz"
/* Endian */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif
/* Align to 4 bytes to get best performance from SH RISC architecture */
/* # ## pragma pack(1) */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x)        x

#ifdef _TRACE_ON_
#define LWIP_PLATFORM_DIAG(x)       _Trace_ x
#define LWIP_PLATFORM_ASSERT(x)     _Trace_ (x)
#else
#define LWIP_PLATFORM_DIAG(x)
#define LWIP_PLATFORM_ASSERT(x)
#endif

#define LWIP_CHKSUM_ALGORITHM       3

#endif /* CC_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
