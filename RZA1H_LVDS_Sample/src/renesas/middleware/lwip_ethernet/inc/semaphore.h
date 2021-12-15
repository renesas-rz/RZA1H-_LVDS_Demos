/******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only 
* intended for use with Renesas products. No other uses are authorized. This 
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE 
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS 
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE 
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer
*******************************************************************************
* Copyright (C) 2012 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************
* File Name    : semaphore.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : FreeRTOS
* H/W Platform : RSK+
* Description  : semaphore functions required for lwIP
*******************************************************************************
* History      : DD.MM.YYYY Ver. Description
*              : 01.08.2009 1.00 First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

#ifndef SEMAPHORE_H_INCLUDED
#define SEMAPHORE_H_INCLUDED

/******************************************************************************
Macro definitions
******************************************************************************/

/******************************************************************************
Typedef definitions
******************************************************************************/

/* The data structure is encapsulated */
typedef struct _SEMT * PSEMT;

/*****************************************************************************
Public Functions
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
Function Name: semCreate
Description:   Function to create a semaphore object
Arguments:     none
Return value:  Pointer to the semaphore object or NULL on error
*****************************************************************************/

extern  PSEMT semCreate(void);

/*****************************************************************************
Function Name: semDestroy
Description:   Function to destroy a semaphore object
Arguments:     IN  pSem - Pointer to the semaphore object
Return value:  none
*****************************************************************************/

extern  void semDestroy(PSEMT pSem);

/*****************************************************************************
Function Name: semWait
Description:   Function to get a semephore (wait in lwIP terms)
Arguments:     IN  pSem - Pointer to the semaphore
               IN  uiTimeOut - The time out in mS
Return value:  The number of mS waited before the semephore was obtained
*****************************************************************************/

extern  uint32_t semWait(PSEMT pSem, uint32_t uiTimeOut);

/*****************************************************************************
Function Name: semSet
Description:   Function to release a mutex event (signal in lwIP terms)
Arguments:     IN  pSem - Pointer to the semaphore
Return value:  none
*****************************************************************************/

extern  void semSet(PSEMT pSem);

#ifdef __cplusplus
}
#endif

#endif /* SEMAPHORE_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
