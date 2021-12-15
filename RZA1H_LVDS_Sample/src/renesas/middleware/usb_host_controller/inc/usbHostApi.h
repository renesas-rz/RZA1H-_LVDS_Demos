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
* Copyright (C) 2011 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************
* File Name    : usbHostApi.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : None
* H/W Platform : RSK+
* Description  : The API functions for the lower level host controller driver.
*******************************************************************************
* History      : DD.MM.YYYY Version Description
*              : 05.08.2010 1.00    First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

#ifndef USBHOSTAPI_H_INCLUDED
#define USBHOSTAPI_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "ddusbh.h"

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Function Name: usbhOpen
Description:   Function to open the USB host driver
Arguments:     IN  pUsbHc - Pointer to the host controller data
Return value:  true for success or false if not enough room in the host array
******************************************************************************/

extern  _Bool usbhOpen(PUSBHC pUsbHc);

/******************************************************************************
Function Name: usbhClose
Description:   Function to close the USB host driver
Arguments:     IN  pUsbHc - Pointer to the host controller data
Return value:  none
******************************************************************************/

extern  void usbhClose(PUSBHC pUsbHc);

/******************************************************************************
Function Name: enumRun
Description:   Function to run the enumerator, should be called at 1mS
               intervals.
Arguments:     none
Return value:  none
******************************************************************************/

extern  void enumRun(void);

/******************************************************************************
Function Name: usbhAddRootPort
Description:   Function to add a root port to the driver
Arguments:     IN  pUsbHc - Pointer to the host controller data
               IN  pRoot - Pointer to the port control functions for this port
Return value:  true if the port was added
******************************************************************************/

extern  _Bool usbhAddRootPort(PUSBHC pUsbHc, const PUSBPC pRoot);

/******************************************************************************
Function Name: usbhInterruptHandler
Description:   Function to handle the USB interrupt
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/

extern  void usbhInterruptHandler(PUSBHC pUsbHc);

#ifdef __cplusplus
}
#endif

#endif                              /* USBHOSTAPI_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
