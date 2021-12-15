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
* File Name    : usbMSBulkOnly.h
* Version      : 1.00
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : None
* H/W Platform : RSK+
* Description  : USB Mass Storage Class Bulk Only Transport Protocol. See
*                usbmassbulk_10.pdf for details.
*******************************************************************************
* History : DD.MM.YYYY Version Description
*         : 05.08.2010 1.00    First Release
*         : 14.12.2010 1.01    Replaced UMSERR return with int
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

#ifndef USBMSBULKONLY_H_INCLUDED
#define USBMSBULKONLY_H_INCLUDED

/******************************************************************************
Typedef definitions
******************************************************************************/

/* The transfer direction is from the host's perspective */
typedef enum _UMSDIR
{
    USB_MS_DIRECTION_OUT = 0,
    USB_MS_DIRECTION_IN
} UMSDIR;

typedef enum _UMSERR
{
    USB_MS_OK,
    USB_MS_ERROR,
    USB_MS_PHASE_ERROR,
    USB_MS_ENDPOINT_STALLED,
    USB_MS_TRANSFER_TIME_OUT,
    USB_MS_DEVICE_REMOVED,
    USB_MS_DRIVER_ERROR
} UMSERR;

/* The structure of a USB MS Command */
typedef struct _MSCMD
{
    /* This is a command name for clarity when debugging only */
    int8_t      *pszCommandName;
    /* The SCSI Command Block itself */
    uint8_t     pbyCB[16];
    /* The length of the SCSI Command */
    uint8_t     byCBLength;
    /* The direction of the data transfer */
    UMSDIR      transferDirection;
    /* The length of data to be transferred with the command */
    size_t      stTransferLength;
    /* A time-out in mS */
    uint32_t    dwTimeOut;

} MSCMD,
*PMSCMD;

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Function Name: usbMsClassReset
Description:   Function to issue the USB Mass Storage Class Reset
Arguments:     IN  iMsDev - The mass storage device file descriptor
Return value:  0 for success or error code
******************************************************************************/

extern  UMSERR usbMsClassReset(int iMsDev);

/******************************************************************************
Function Name: usbMsClassGetMaxLun
Description:   Function to issue the USB Mass Storage Class Get Max Lun request
Arguments:     IN  iMsDev - The mass storage device file descriptor
               OUT pbyMaxLun - Pointer to the destination max lun varaible
Return value:  0 for success or error code
******************************************************************************/

extern  UMSERR usbMsClassGetMaxLun(int iMsDev, uint8_t *pbyMaxLun);

/******************************************************************************
Function Name: usbMsCommand
Description:   Function to transport a USB MS Bulk Only command as per sect. 5
Arguments:     IN  iMsDev - The mass storage device file descriptor
               IN  iLun - The logical uint number 
               IN  pMsCmd - Pointer to the command to send
               OUT pvData - Pointer to the source or destination data
               OUT pstLenTrans - Pointer to the length of data transferred
Return value:  0 for success or error code
******************************************************************************/

extern  UMSERR usbMsCommand(int    iMsDev,
                         int    iLun,
                         PMSCMD pMsCmd,
                         void   *pvData,
                         size_t *pstLenTrans);

#ifdef __cplusplus
}
#endif

#endif /* USBMSBULKONLY_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
