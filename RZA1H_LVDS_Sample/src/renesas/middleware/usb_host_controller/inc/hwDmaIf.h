/******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized.
* This software is owned by Renesas Electronics Corporation and is  protected
* under all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES
* REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY,
* INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR  A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE  EXPRESSLY
* DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE  LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
* FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS
* AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this
* software and to discontinue the availability of this software.
* By using this software, you agree to the additional terms and
* conditions found by accessing the following link:
* http://www.renesas.com/disclaimer
*******************************************************************************
* Copyright (C) 2010 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************
* File Name    : hwDmaIf.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : None
* H/W Platform : RSK+
* Description  : The hardware interface functions for the DMA. The functions
*                here could abstract from the DMA hardware providing a simple
*                interface for DMA usage. This is beyond the scope of this
*                sample code so it has been made as simple as possible.
*******************************************************************************
* History : DD.MM.YYYY Version Description
*         : 05.08.2010 1.00    First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

#ifndef HWDMAIF_H_INCLUDED
#define HWDMAIF_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include <string.h>

/******************************************************************************
Macro definitions
******************************************************************************/

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Function Name: usbOpenDmaDriver
 * Description  : Open the DMA driver for USBN out and in
 * Arguments    : none
 * Return Value : DRV_SUCCESS or DRV_ERROR
 *****************************************************************************/
extern int_t usbOpenDmaDriver(void);

/******************************************************************************
Function Name: dmaStartUsbOutCh0
Description:   Function to start DMA channel 0 for a USB OUT transfer
               This is where the DMAC writes to the designated pipe FIFO.
               In this implementation the assignment is DMA Channel 0 always
               uses the USB D0FIFO.
Arguments:     IN  pvSrc - Pointer to the 4 byte aligned source memory
               IN  st_length - The length of data to transfer
               IN  p_fifo - Pointer to the destination FIFO
               IN  pv_param - Pointer to pass to the completion routine
               IN  pf_complete - Pointer to the completion routine
Return value:  none
******************************************************************************/

extern  void dmaStartUsbOutCh0(void     *pvSrc,
                               size_t   st_length,
                               void     *p_fifo,
                               void     *pv_param,
                               void (*pf_complete)(void *pv_param));

/******************************************************************************
Function Name: dmaGetUsbOutCh0Count
Description:   Function to get the current DMA transfer count
Arguments:     none
Return value:  The value of the transfer count register
******************************************************************************/

extern  unsigned long dmaGetUsbOutCh0Count(void);

/******************************************************************************
Function Name: dmaStopUsbOutCh0
Description:   Function to stop a USB OUT DMA transfer on channel 0
Arguments:     none
Return value:  none
******************************************************************************/

extern  void dmaStopUsbOutCh0(void);

/******************************************************************************
Function Name: dmaStartUsbInCh1
Description:   Function to start DMA channel 1 for a USB IN transfer
               This is where the DMAC writes to the designated pipe FIFO.
               In this implementation the assignment is DMA Channel 1 always
               uses the USB D1FIFO.
Arguments:     IN  pv_dest - Pointer to the 4 byte aligned destination memory
               IN  st_length - The length of data to transfer
               IN  p_fifo - Pointer to the source FIFO
               IN  pv_param - Pointer to pass to the completion routine
               IN  pf_complete - Pointer to the completion routine
Return value:  none
******************************************************************************/

extern  void dmaStartUsbInCh1(void     *pv_dest,
                              size_t   st_length,
                              void     *p_fifo,
                              void     *pv_param,
                              void (*pf_complete)(void *pv_param));

/******************************************************************************
Function Name: dmaGetUsbInCh1Count
Description:   Function to get the USB DMA transfer on channel 1
Arguments:     none
Return value:  The value of the transfer count register
******************************************************************************/

extern  unsigned long dmaGetUsbInCh1Count(void);

/******************************************************************************
Function Name: dmaStopUsbInCh1
Description:   Function to stop a USB OUT DMA transfer on channel 1
Arguments:     none
Return value:  none
******************************************************************************/

extern  void dmaStopUsbInCh1(void);

#ifdef __cplusplus
}
#endif

#endif /* HWDMAIF_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
