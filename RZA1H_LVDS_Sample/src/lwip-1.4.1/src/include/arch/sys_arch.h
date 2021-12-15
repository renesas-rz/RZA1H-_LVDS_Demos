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
* File Name    : sys_arch.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : FreeRTOS
* H/W Platform : RSK+
* Description  : System architecture interface for lwIP
******************************************************************************
* History : DD.MM.YYYY Version Description
*         : 04.02.2010 1.00    First Release
*         : DD.MM.YYYY ?.??    Updated for lwIP V1.4.1
******************************************************************************/

#ifndef SYS_ARCH_H_INCLUDED
#define SYS_ARCH_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "compiler_settings.h"
#include "semaphore.h"

/******************************************************************************
Macro definitions
******************************************************************************/

#define SYS_SEM_NULL                NULL
#define SYS_MBOX_NULL               NULL
/* Do not change this without changing the memory management functions in 
   sys_arc.c */
#define LWIP_PRIMARY_MEMORY         0
#define LWIP_COMPAT_MUTEX           1

/*****************************************************************************
Typedefs
******************************************************************************/

typedef struct _SEMT *sys_sem_t;
typedef struct _MBOX *sys_mbox_t;
typedef uint32_t sys_thread_t;
typedef int sys_prot_t;

/* Our memory allocation and free functions */
extern void *lwip_malloc(size_t stLength);
extern void *lwip_realloc(void *pvReAlloc, size_t stLength);
extern void lwip_free(void *pvFree);
/* Override the lwIP ones */
#ifdef mem_malloc
#undef mem_malloc
#endif
#define mem_malloc lwip_malloc
#ifdef mem_realloc
#undef mem_realloc
#endif
#define mem_realloc lwip_realloc
#ifdef mem_free
#undef mem_free
#endif
#define mem_free lwip_free

#endif /* SYS_ARCH_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
