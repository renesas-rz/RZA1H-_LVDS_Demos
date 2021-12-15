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
 * and to discontinue the availability of this software. By using this software
 * you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 ******************************************************************************
 * Copyright (C) 2012 Renesas Electronics Corporation. All rights reserved.
 ******************************************************************************
 * File Name    : usbMSBulkOnly.c
 * Version      : 1.00
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : FreeRTOS
 * H/W Platform : RSK+
 * Description  : USB Mass Storage Class Bulk Only Transport Protocol. See
 *                usbmassbulk_10.pdf for details.
 *                www.usb.org/developers/devclass_docs/usbmassbulk_10.pdf
 ******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 First Release
 *****************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/
#include <string.h>

#include "compiler_settings.h"
#include "r_devlink_wrapper.h"
#include "control.h"
#include "usbMSBulkOnly.h"
#include "endian.h"
#include "trace.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/

/* Define the CBW and CSW signatures */
#define USB_MS_CBW_SIGNATURE        (uint32_t)(('U') | (('S') << 8) | \
                                           (('B') << 16) | (('C') << 24))
#define USB_MS_CSW_SIGNATURE        (uint32_t)(('U') | (('S') << 8) | \
                                           (('B') << 16) | (('S') << 24))

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif
#define XTRACE(x)

/******************************************************************************
 Typedef definitions
 ******************************************************************************/

/* Define the structure of the USB MS Command Block Wrapper
 NOTE: The data in this structure is LITTLE ENDIAN */
/* This structure is packed, see section 5.1 */
#pragma pack (1)
typedef union _UCBW
{
    /* This is 31 bytes in size so it will always be a short packet for any
     of the packet sizes supported by USB */
    uint8_t                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     Byte[31];

    struct
    {
        /* = "USBC" */
        uint32_t dCBWSignature;
        /* CSW tag must be set to match this */
        uint32_t dCBWTag;
        /* Number of bytes for the host to transfer */
        uint32_t dCBWDataTransferLength;
        /* Bit 7 is direction (0 = Data from host to device) */
        uint8_t  bmCBWFlags;
        /* Top 4 bits must be zero */
        uint8_t  bCBWLUN;
        /* Length of command block (must be 1 to 16 inclusive) */
        uint8_t  bCBWCBLength;
        /* The actual command (SCSI-RBC in this case) */
        uint8_t  pbCBWCB[16];

    } Field;
} CBW, *PCBW;
#pragma pack()

/* Define the structure of the USB MS Command Status Wrapper
 NOTE: The data in this structure is LITTLE ENDIAN */
/* This structure is packed, see section 5.2 */
#pragma pack (1)
typedef union _CSW
{
    uint8_t                                                                                                                                                                                                                                                                                                                                                   Byte[13];
    struct
    {
        /* = "USBS" */
        uint32_t dCSWSignature;
        /* Must match Tag sent in CBW */
        uint32_t dCSWTag;
        /* Number of bytes NOT transferred out of dCBWDataTransferLength */
        uint32_t dCSWDataResidue;
        /* 0 = pass, 1 = Fail, 2 = Full Phase Error */
        uint8_t  bCSWStatus;
    } Field;

} CSW, *PCSW;
#pragma pack()

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

static UMSERR usbMsPutGet (int iMsDev, int iLun, PMSCMD pMsCmd, void *pvData, size_t *pstLenTrans);
static UMSERR usbMsGetErrorCode (int iDriverErrorCode);
static UMSERR usbMsSendCBW (int iMsDev, int iLun, PMSCMD pMsCmd, PCBW pCBW);
static UMSERR usbMsReceiveCSW (int iMsDev, PCSW pCSW);

/******************************************************************************
 Exported global variables and functions (to be accessed by other files)
 ******************************************************************************/

/******************************************************************************
 Function Name: usbMsClassReset
 Description:   Function to issue the USB Mass Storage Class Reset
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 Return value:  0 for success or error code
 ******************************************************************************/
UMSERR usbMsClassReset (int iMsDev)
{
    UMSERR iResult;

    /* Section 3.1 */
    control(iMsDev, CTL_USB_MS_WAIT_MUTEX, NULL);
    iResult = control(iMsDev, CTL_USB_MS_RESET, NULL);
    control(iMsDev, CTL_USB_MS_RELEASE_MUTEX, NULL);
    return iResult;
}
/******************************************************************************
 End of function  usbMsClassReset
 ******************************************************************************/

/******************************************************************************
 Function Name: usbMsClassGetMaxLun
 Description:   Function to issue the USB Mass Storage Class Get Max Lun request
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 OUT pbyMaxLun - Pointer to the destination max lun varaible
 Return value:  0 for success or error code
 ******************************************************************************/
UMSERR usbMsClassGetMaxLun (int iMsDev, uint8_t *pbyMaxLun)
{
    UMSERR iResult;
    /* Section 3.2 */
    control(iMsDev, CTL_USB_MS_WAIT_MUTEX, NULL);
    iResult = control(iMsDev, CTL_USB_MS_GET_MAX_LUN, pbyMaxLun);
    control(iMsDev, CTL_USB_MS_RELEASE_MUTEX, NULL);
    return iResult;
}
/******************************************************************************
 End of function  usbMsClassGetMaxLun
 ******************************************************************************/

#define  _MULTIPLE_MS_ACCESS_ (1)

/******************************************************************************
 Function Name: usbMsCommand
 Description:   Function to transport a USB MS Bulk Only command as per section 5
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 IN  pMsCmd - Pointer to the command to send
 OUT pvData - Pointer to the source or destination data
 OUT pstLenTrans - Pointer to the length of data transferred
 Return value:  0 for success or error code
 ******************************************************************************/
UMSERR usbMsCommand (int iMsDev, int iLun, PMSCMD pMsCmd, void *pvData, size_t *pstLenTrans)
{
    UMSERR iErrorCode = 0;

    /* The driver has a bug with multiple MS commands because the pipe can
     be re-assigned when the CSW is stuck in the FIFO */
#ifndef _MULTIPLE_MS_ACCESS_
    static     event_t ms_command_mutex;
    R_OS_EventWaitMutex(&ms_command_mutex, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);
#else
    /* Each logical unit is communicated with through the same ID so mutually
     exclusive access must be provided */
    control(iMsDev, CTL_USB_MS_WAIT_MUTEX, NULL);
#endif
    iErrorCode = usbMsPutGet(iMsDev, iLun, pMsCmd, pvData, pstLenTrans);
#ifndef _MULTIPLE_MS_ACCESS_
    R_OS_EventReleaseMutex(&ms_command_mutex);
#else
    control(iMsDev, CTL_USB_MS_RELEASE_MUTEX, NULL);
#endif
    return iErrorCode;
}
/******************************************************************************
 End of function  usbMsCommand
 ******************************************************************************/

/******************************************************************************
 Private Functions
 ******************************************************************************/

/******************************************************************************
 Function Name: usbMsPutGet
 Description:   Function to transport a USB MS Bulk Only command as per section 5
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 IN  pMsCmd - Pointer to the command to send
 OUT pvData - Pointer to the source or destination data
 OUT pstLenTrans - Pointer to the length of data transferred
 Return value:  0 for success or error code
 ******************************************************************************/
static UMSERR usbMsPutGet (int iMsDev, int iLun, PMSCMD pMsCmd, void *pvData, size_t *pstLenTrans)
{
    CBW cmdBlkWpr;
    CSW cmdStsWpr;
    UMSERR iErrorCode = USB_MS_OK;
    size_t stLengthTransferred = 0UL;

    /* Set the time-out for the command */
    control(iMsDev, CTL_SET_TIME_OUT, &pMsCmd->dwTimeOut);

    /* Write the CBW to the device */
    iErrorCode = usbMsSendCBW(iMsDev, iLun, pMsCmd, &cmdBlkWpr);
    if (iErrorCode)
    {
        TRACE(("usbMsPutGet: Failed to send CBW %d\r\n", iErrorCode));
        /* Section 6.6.1 */
        control(iMsDev, CTL_USB_MS_RESET, NULL);
        ;
        return usbMsGetErrorCode(iErrorCode);
    }
    else
    {
        XTRACE(("usbMsPutGet: CBW sent\r\n"));
    }
    /* Read or write the data - if there is any */
    if (pMsCmd->stTransferLength)
    {
        UMSERR iDataPhaseError = USB_MS_OK;
        if (pMsCmd->transferDirection == USB_MS_DIRECTION_IN)
        {
            stLengthTransferred = (size_t) read(iMsDev, pvData, (uint32_t) pMsCmd->stTransferLength);
            TRACE(("usbMsPutGet: Read %d\r\n", stLengthTransferred));
        }
        else
        {
            stLengthTransferred = (size_t) write(iMsDev, pvData, (uint32_t) pMsCmd->stTransferLength);
            XTRACE(("usbMsPutGet: Write %lu\r\n", stLengthTransferred));
        }
        if (pstLenTrans)
        {
            *pstLenTrans = stLengthTransferred;
        }

        /* Get the error code from the data transfer */
        control(iMsDev, CTL_GET_LAST_ERROR, &iDataPhaseError);
        if (stLengthTransferred == -1u)
        {
            /* If the command is not supported by the device it may stall
             the IN endpoint */
            if ((int)iDataPhaseError == (int)BULK_EP_STALL_ERROR)
            {
                /* Clear a stalled IN endpoint */
                control(iMsDev, CTL_USB_MS_CLEAR_BULK_IN_STALL, NULL);
                /* Get the status of the command */
                cmdStsWpr.Field.dCSWTag = cmdBlkWpr.Field.dCBWTag;
                iErrorCode = usbMsReceiveCSW(iMsDev, &cmdStsWpr);
                if (iErrorCode)
                {
                    TRACE(("usbMsPutGet: Failed to get CSW\r\n"));
                }
                return USB_MS_PHASE_ERROR;
            }
            else
            {
                control(iMsDev, CTL_USB_MS_RESET, NULL);
                ;
                return usbMsGetErrorCode(iDataPhaseError);
            }
        }
    } XTRACE(("usbMsPutGet: Try to receive CSW\r\n"));

    /* Get the status of the command */
    cmdStsWpr.Field.dCSWTag = cmdBlkWpr.Field.dCBWTag;
    iErrorCode = usbMsReceiveCSW(iMsDev, &cmdStsWpr);

    if (iErrorCode)
    {
        TRACE(("usbMsPutGet: Failed to get CSW\r\n"));
        return usbMsGetErrorCode(iErrorCode);
    }
    else
    {
        XTRACE(("usbMsPutGet: CSW Received\r\n"));
    }

    /* Check to see if there was an error */
    switch (cmdStsWpr.Field.bCSWStatus)
    {
        case 0 :
            return USB_MS_OK;

        case 1 :
            TRACE(("usbMsPutGet: Error in CSW\r\n"));
            return USB_MS_ERROR;

        default :
        case 2 :
            TRACE(("usbMsPutGet: Error in command\r\n"));
            control(iMsDev, CTL_USB_MS_RESET, NULL);
            return USB_MS_PHASE_ERROR;
    }
}
/******************************************************************************
 End of function  usbMsPutGet
 ******************************************************************************/

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Function Name: usbMsGetErrorCode
 Description:   Function to get the appropriate error code from the lower level
 driver error code
 Arguments:     IN  iDriverErrorCode - The driver error code
 Return value:  The USB MS Error Code
 ******************************************************************************/
static UMSERR usbMsGetErrorCode (int iDriverErrorCode)
{
    switch (iDriverErrorCode)
    {
        case BULK_EP_NO_ERROR :
            return USB_MS_OK;
        case BULK_EP_TIME_OUT :
            TRACE(("BULK_EP_TIME_OUT\r\n"));
            return USB_MS_TRANSFER_TIME_OUT;
        case BULK_EP_DEVICE_REMOVED :
            TRACE(("BULK_EP_DEVICE_REMOVED\r\n"));
            return USB_MS_DEVICE_REMOVED;
        case BULK_EP_STALL_ERROR :
            TRACE(("BULK_EP_STALL_ERROR\r\n"));
            return USB_MS_ENDPOINT_STALLED;
        default :
            TRACE(("Bulk driver error code %d\r\n", iDriverErrorCode));
            return USB_MS_DRIVER_ERROR;
    }
}
/******************************************************************************
 End of function  usbMsGetErrorCode
 ******************************************************************************/

/******************************************************************************
 Function Name: usbMsSendCBW
 Description:   Function to send the USB MS Command Block Wrapper
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical Unit Number
 IN  pMsCmd - Pointer to the command to send
 OUT pCBW - Pointer to the command block wrapper
 Return value:  0 for success or error code
 ******************************************************************************/
static UMSERR usbMsSendCBW (int iMsDev, int iLun, PMSCMD pMsCmd, PCBW pCBW)
{
    static uint32_t dwCBWTag = 1;
    size_t stLengthWritten;

    /* Initialise the command block wrapper */
    memset(pCBW, 0, sizeof(CBW));

    /* Fill out the command block wrapper structure */
    pCBW->Field.dCBWSignature = SWAP_ENDIAN_LONG(USB_MS_CBW_SIGNATURE);
    SWAP_ENDIAN_LONG_AT(&pCBW->Field.dCBWTag, &dwCBWTag);
    SWAP_ENDIAN_LONG_AT(&pCBW->Field.dCBWDataTransferLength, &pMsCmd->stTransferLength);

    if (pMsCmd->transferDirection == USB_MS_DIRECTION_IN)
    {
        pCBW->Field.bmCBWFlags = BIT_7;
    }
    else
    {
        pCBW->Field.bmCBWFlags = 0;
    }
    pCBW->Field.bCBWLUN = (uint8_t) (0x0F & iLun);
    pCBW->Field.bCBWCBLength = pMsCmd->byCBLength;
    memcpy(pCBW->Field.pbCBWCB, pMsCmd->pbyCB, (size_t) ((pMsCmd->byCBLength < 16) ? pMsCmd->byCBLength : 16));

    TRACE(("usbMsSendCBW: %s (%d)\r\n", pMsCmd->pszCommandName, pMsCmd->stTransferLength));

    /* Write it to the device */
    stLengthWritten = (size_t) write(iMsDev, (uint8_t *) pCBW, sizeof(CBW));

    /* Bump the tag for the next packet */
    dwCBWTag++;
    if (stLengthWritten == -1U)
    {
        /* Convert the error code */
        int iErrorCode;
        if (control(iMsDev, CTL_GET_LAST_ERROR, &iErrorCode) != -1)
        {
            return usbMsGetErrorCode(iErrorCode);
        }
        /* report a generic error */
        return USB_MS_DRIVER_ERROR;
    }
    return USB_MS_OK;
}
/******************************************************************************
 End of function  usbMsSendCBW
 ******************************************************************************/

/******************************************************************************
 Function Name: usbMsValidateCSW
 Description:   Function to validate a CSW
 Arguments:     IN  pdwTempPacket- Pointer to the packet containg the CSW
 IN  dwCSWTag - Pointer to the CSW Tag
 IN  stLength - The length of data returned
 Return value:  true if it is valid
 ******************************************************************************/
static _Bool usbMsValidateCSW (uint32_t *pdwTempPacket, uint32_t dwCSWTag, size_t stLength)
{
    if (stLength != sizeof(CSW))
    {
        TRACE(("usbMsValidateCSW: Size Mismatch\r\n"));
        return false;
    }
    if (*pdwTempPacket++ != SWAP_ENDIAN_LONG(USB_MS_CSW_SIGNATURE))
    {
        TRACE(("usbMsValidateCSW: Signature mismatch\r\n"));
        return false;
    }
    if (*pdwTempPacket != dwCSWTag)
    {
        TRACE(("usbMsValidateCSW: Tag mismatch\r\n"));
        return false;
    }
    return true;
}
/******************************************************************************
 End of function  usbMsValidateCSW
 ******************************************************************************/

/******************************************************************************
 Function Name: usbMsReceiveCSW
 Description:   Function to receive the CSW
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 OUT pCSW - pointer to the Command Status Wrapper
 Return value:  0 for success or error code
 ******************************************************************************/
static UMSERR usbMsReceiveCSW (int iMsDev, PCSW pCSW)
{
    CSW msCSW;
    size_t stLengthTransferred;
    int iStallCount = 0;
    PBLKERR ErrorCode;

    /* Loop so if the device returns more data than expected CSW is still
     received */
    do
    {
        ErrorCode = 0;

        /* Read one packet at a time until a CSW is found */
        stLengthTransferred = (size_t) read(iMsDev, (uint8_t *) &msCSW, sizeof(CSW));
        /* Get the error code */
        control(iMsDev, CTL_GET_LAST_ERROR, &ErrorCode);
        /* Figure 2 states STALL Bulk-In or Bulk Error - so we don't care about
         the reason for failure */
        if (ErrorCode)
        {
#ifdef _DEBUG_
            /* However, when debugging the comms we do care about the error */
            ERRSTR errStr;
            /* Set the error code */
            errStr.iErrorCode = ErrorCode;
            /* Get the string */
            if (!control(iMsDev, CTL_GET_ERROR_STRING, &errStr))
            {
                TRACE(("usbMsReceiveCSW: Stall count %d Error %s\r\n",
                                iStallCount, errStr.pszErrorString));
            }
            else
            {
                TRACE(("usbMsReceiveCSW: Failed with unknown error %d\r\n",
                                errStr.iErrorCode));
            }
#endif
            /* Section 5.3.3 Figure 2 */
            iStallCount++;
            if (iStallCount > 1)
            {
                /* Perform reset recovery */
                TRACE(("Second CSW failure, issuing class reset\r\n"));
                control(iMsDev, CTL_USB_MS_RESET, NULL);
                ;
                return USB_MS_PHASE_ERROR;
            }
            else
            {
                /* First error (stall or other) issue a clear stall */
                TRACE(("First CSW failure, clearing stall\r\n"));
                control(iMsDev, CTL_USB_MS_CLEAR_BULK_IN_STALL, NULL);
            }
        }
    } while (!usbMsValidateCSW((uint32_t *) &msCSW, pCSW->Field.dCSWTag, stLengthTransferred));

    /* Copy the data to the destination CSW */
    *pCSW = msCSW;

    return USB_MS_OK;
}
/******************************************************************************
 End of function  usbMsReceiveCSW
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
