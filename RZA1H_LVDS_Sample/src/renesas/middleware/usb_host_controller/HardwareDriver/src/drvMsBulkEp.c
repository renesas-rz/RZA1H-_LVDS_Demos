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
 * File Name    : drvMsBulkEp.c
 * Version      : 1.00
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : Device driver for Mass Storage Class devices BULK-ONLY
 *                transport. This is the lowest level of the driver stack
 *                supporting only the data transport to the USB device.
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 First Release
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iodefine_cfg.h"

#include "compiler_settings.h"
#include "control.h"

#include "usbhDeviceApi.h"
#include "trace.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/
#define BLK_EP_DEFAULT_TIME_OUT     8000UL

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Typedef definitions
 ******************************************************************************/

typedef struct _BULKEP
{
    /* The transfer request structures */
    USBTR    writeRequest;
    USBTR    readRequest;
    USBTR    deviceRequest;
    _Bool    bfDeviceRequestTimeOut;
    /* The last error code */
    BLKERR   lastError;
    /* Pointer to the device and endpoints */
    PUSBDI   pDevice;
    PUSBEI   pOutEndpoint;
    PUSBEI   pInEndpoint;
    /* The current time-out */
    uint32_t dwTimeOut_mS;

    /* The mutex event to make sure that devices with more than one logical
     uint are accessed sequentially */
    event_t evDeviceMutex;

    /* The stream name of the device */
    int8_t   pszStreamName[DEVICE_MAX_STRING_SIZE];

} BULKEP, *PBULKEP;

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

static int blkepOpen (st_stream_ptr_t pstream);
static void blkepClose (st_stream_ptr_t pstream);
static int blkepRead (st_stream_ptr_t pstream, uint8_t *pbyBuffer, uint32_t uiCount);
static int blkepWrite (st_stream_ptr_t pstream, uint8_t *pbyBuffer, uint32_t uiCount);
static int blkepControl (st_stream_ptr_t pstream, uint32_t ctlCode, void *pCtlStruct);
static PUSBEI blkepGetFirstEndpoint (PUSBDI pDevice, USBDIR transferDirection);
static BLKERR bulkepSetErrorCode (REQERR errorCode);

/******************************************************************************
 Constant Data
 ******************************************************************************/

/* Define the driver function table for this */
const st_r_driver_t gMsBulkDriver =
{ "MS BULK Only Device Driver", blkepOpen, blkepClose, blkepRead, blkepWrite, blkepControl, no_dev_get_version };

static char * gpszErrorString[] =
{ "BULK_EP_NO_ERROR", "BULK_EP_IO_PENDING", "BULK_EP_TIME_OUT", "BULK_EP_DATA_ERROR", "BULK_EP_REQUEST_ERROR",
        "BULK_EP_DEVICE_REMOVED", "BULK_EP_STALL_ERROR", "BULK_EP_OVERRUN_ERROR", "BULK_EP_INVALID_CONTROL_CODE",
        "BULK_EP_INVALID_PARAMETER" };

/******************************************************************************
 Function Name: blkepOpen
 Description:   Function to open the bulk endpoint driver
 Arguments:     IN  st_stream_ptr_t - Pointer to the file stream
 Return value:  0 for success otherwise -1
 ******************************************************************************/
static int blkepOpen (st_stream_ptr_t pstream)
{
    /* Get a pointer to the device information from the host driver for this
     device */
    PUSBDI pDevice = usbhGetDevice((int8_t *)pstream->p_stream_name);
    if (pDevice)
    {
        /* This driver supports USB devices with Bulk IN and OUT endpoints */
        PUSBEI pIn = blkepGetFirstEndpoint(pDevice, USBH_IN);
        PUSBEI pOut = blkepGetFirstEndpoint(pDevice, USBH_OUT);

        /* If the device has the two endpoints */
        if ((pOut) && (pIn))
        {
            /* Allocate the memory for the driver */
            pstream->p_extension = R_OS_AllocMem(sizeof(BULKEP), R_REGION_LARGE_CAPACITY_RAM);
            if (pstream->p_extension)
            {
                PBULKEP pBulkEp = pstream->p_extension;
                /* Initialise the driver data */
                memset(pBulkEp, 0, sizeof(BULKEP));
                /* Copy in the link name */
                memcpy(pBulkEp->pszStreamName, pstream->p_stream_name, DEVICE_MAX_STRING_SIZE);//TODO
                /* Set the device and endpoint information found */
                pBulkEp->pDevice = pDevice;
                pBulkEp->pOutEndpoint = pOut;
                pBulkEp->pInEndpoint = pIn;
                pBulkEp->dwTimeOut_mS = BLK_EP_DEFAULT_TIME_OUT;

                /* Create the signals for the transfer requests */
                R_OS_CreateEvent(&pBulkEp->writeRequest.ioSignal);
                R_OS_CreateEvent(&pBulkEp->readRequest.ioSignal);
                R_OS_CreateEvent(&pBulkEp->deviceRequest.ioSignal);

                /* Create the mutex for the CTL_USB_MS_WAIT_MUTEX control */
                pBulkEp->evDeviceMutex = R_OS_CreateMutex();

                TRACE(("blkepOpen: Opened device %s\r\n",
                                pstream->p_stream_name));
                return BULK_EP_NO_ERROR;
            }
            else
            {
                TRACE(("blkepOpen: Failed to allocate memory\r\n"));
            }
        }
        else
        {
            TRACE(("blkepOpen: Failed to find bulk endpoints\r\n"));
        }
    }
    else
    {
        TRACE(("blkepOpen: Failed to find device %s\r\n",
                        pstream->p_stream_name));
    }
    return -1;
}
/******************************************************************************
 End of function  blkepOpen
 ******************************************************************************/

/******************************************************************************
 Function Name: blkepClose
 Description:   Function to close the bulk endpoint driver
 Arguments:     IN  st_stream_ptr_t - Pointer to the file stream
 Return value:  none
 ******************************************************************************/
static void blkepClose (st_stream_ptr_t pstream)
{
    PBULKEP pBulkEp = pstream->p_extension;
    TRACE(("blkepClose: Closing device %s\r\n",
                    pstream->p_stream_name));


    /* Cancel any outstanding transfer requests */
    usbhCancelTransfer(&pBulkEp->writeRequest);
    usbhCancelTransfer(&pBulkEp->readRequest);
    usbhCancelTransfer(&pBulkEp->deviceRequest);

    /* Destroy the events */
    R_OS_DeleteEvent(&pBulkEp->writeRequest.ioSignal);
    R_OS_DeleteEvent(&pBulkEp->readRequest.ioSignal);
    R_OS_DeleteEvent(&pBulkEp->deviceRequest.ioSignal);

    R_OS_DeleteMutex(pBulkEp->evDeviceMutex);

    /* Free the memory */
    R_OS_FreeMem(pstream->p_extension);

}
/******************************************************************************
 End of function  blkepClose
 ******************************************************************************/

/******************************************************************************
 Function Name: blkepRead
 Description:   Function to read from the bulk IN endpoint
 Arguments:     IN  st_stream_ptr_t - Pointer to the file stream
 IN  pbyBuffer - Pointer to the destination memory
 IN  uiCount - The number of bytes to read
 Return value:  The number of bytes read or -1 on error
 ******************************************************************************/
static int blkepRead (st_stream_ptr_t pstream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    PBULKEP pBulkEp = pstream->p_extension;
    if (pBulkEp)
    {
        _Bool bfResult;
        pBulkEp->lastError = BULK_EP_NO_ERROR;
        pBulkEp->readRequest.errorCode = REQ_NO_ERROR;

        bfResult = usbhStartTransfer(pBulkEp->pDevice, &pBulkEp->readRequest, pBulkEp->pInEndpoint, pbyBuffer,
                (size_t) uiCount, pBulkEp->dwTimeOut_mS);

        /* If the transfer was started */
        if (bfResult)
        {
            /* Wait for the transfer to complete or time-out */
            R_OS_WaitForEvent(&pBulkEp->readRequest.ioSignal, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);

            /* Check the error code */
            if (pBulkEp->readRequest.errorCode)
            {
                TRACE(("blkepRead: Error %d\r\n", pBulkEp->readRequest.errorCode));
                /* Simplify the error code */
                pBulkEp->lastError = bulkepSetErrorCode(pBulkEp->readRequest.errorCode);
                return -1;
            }
            /* return the length transferred */
            return (int) pBulkEp->readRequest.uiTransferLength;
        }
        else
        {
            TRACE(("blkepRead: Failed to start transfer\r\n"));
            pBulkEp->lastError = BULK_EP_REQUEST_ERROR;
        }
    }
    return -1;
}
/******************************************************************************
 End of function  blkepRead
 ******************************************************************************/

/******************************************************************************
 Function Name: blkepWrite
 Description:   Function to write to the bulk OUT endpoint
 Arguments:     IN  st_stream_ptr_t - Pointer to the file stream
 IN  pbyBuffer - Pointer to the source memory
 IN  uiCount - The number of bytes to write
 Return value:  The length of data written -1 on error
 ******************************************************************************/
static int blkepWrite (st_stream_ptr_t pstream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    PBULKEP pBulkEp = pstream->p_extension;
    if (pBulkEp)
    {
        _Bool bfResult = true;
        pBulkEp->lastError = BULK_EP_NO_ERROR;
        bfResult = usbhStartTransfer(pBulkEp->pDevice, &pBulkEp->writeRequest, pBulkEp->pOutEndpoint, pbyBuffer,
                (size_t) uiCount, pBulkEp->dwTimeOut_mS);
        /* If the transfer was started */
        if (bfResult)
        {
            /* Wait for the transfer to complete or time-out */
            R_OS_WaitForEvent(&pBulkEp->writeRequest.ioSignal, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);

            /* Check the error code */
            if (pBulkEp->writeRequest.errorCode)
            {
                TRACE(("blkepWrite: %d\r\n", pBulkEp->writeRequest.errorCode));
                /* Simplify the error code */
                pBulkEp->lastError = bulkepSetErrorCode(pBulkEp->writeRequest.errorCode);
                return -1;
            }
            /* return the length transferred */
            return (int) pBulkEp->writeRequest.uiTransferLength;
        }
        else
        {
            TRACE(("blkepWrite: Failed to start transfer\r\n"));
            pBulkEp->lastError = BULK_EP_REQUEST_ERROR;
        }
    }
    return -1;
}
/******************************************************************************
 End of function  blkepWrite
 ******************************************************************************/

/******************************************************************************
 Function Name: bulkepSetErrorCode
 Description:
 Arguments:     IN  errorCode - The transfer request error code
 Return value:  The bulk driver simplified error code
 ******************************************************************************/
static BLKERR bulkepSetErrorCode (REQERR errorCode)
{
    switch (errorCode)
    {
        case REQ_NOT_RESPONDING_ERROR :
            TRACE(("bulkepSetErrorCode: Endpoint stalled\r\n"));
            return BULK_EP_STALL_ERROR;

        case REQ_DATA_OVERRUN_ERROR :
            TRACE(("bulkepSetErrorCode: Endpoint buffer overrun\r\n"));
            return BULK_EP_OVERRUN_ERROR;

        case REQ_IDLE_TIME_OUT :
            TRACE(("bulkepSetErrorCode: Transfer time-out\r\n"));
            return BULK_EP_TIME_OUT;

        default :
            TRACE(("bulkepSetErrorCode: "
                            "Not stalled or time-out but other error %d\r\n", errorCode));
            return BULK_EP_DATA_ERROR;
    }
}
/******************************************************************************
 End of function  bulkepSetErrorCode
 ******************************************************************************/

/******************************************************************************
 Function Name: blkepDeviceRequest
 Description:   Function to send a device request
 Arguments:     IN  pBulkEp - Pointer to the driver extension
 IN  bRequest - The request
 IN  bmRequestType - The request type
 IN  wValue - The Value
 IN  wIndex - The Index
 IN  wLength - The length of the data
 IN/OUT pbyData - Pointer to the data
 Return value:  0 for success or error code
 ******************************************************************************/
static int blkepDeviceRequest (PBULKEP pBulkEp, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue,
        uint16_t wIndex, uint16_t wLength, uint8_t *pbyData)
{
    if (usbhDeviceRequest(&pBulkEp->deviceRequest, pBulkEp->pDevice, bmRequestType, bRequest, wValue, wIndex, wLength,
            pbyData))
    {
        pBulkEp->lastError = BULK_EP_REQUEST_ERROR;
        return BULK_EP_REQUEST_ERROR;
    }
    return BULK_EP_NO_ERROR;
}
/******************************************************************************
 End of function  blkepDeviceRequest
 ******************************************************************************/

/******************************************************************************
 Function Name: blkepMsReset
 Description:   Function to send the USB MS Bulk Only Class Reset vendor request
 Arguments:     IN  pBulkEp - Pointer to the driver extension
 Return value:  0 for success or error code
 ******************************************************************************/
static int blkepMsReset (PBULKEP pBulkEp)
{
    int iErr;

    /* Mass storage reset, see section 3.1 */
    iErr = blkepDeviceRequest(pBulkEp, 0x21, /* Class, Interface, Host to device */
    0xFF, /* MS class reset request */
    0, /* Value field set to zero */
    /* Index field set to the interface number */
    (uint16_t) pBulkEp->pDevice->byInterfaceNumber, 0, /* Length field set to zero */
    /* No data */
    NULL);

    if (!iErr)
    {
        USBRQ usbRequest;
        usbRequest.Field.Direction = USB_HOST_TO_DEVICE;
        usbRequest.Field.Type = USB_STANDARD_REQUEST;
        usbRequest.Field.Recipient = USB_RECIPIENT_ENDPOINT;
        /* Clear feature HALT Bulk IN endpoint, see section 5.3.4 */
        iErr = blkepDeviceRequest(pBulkEp, usbRequest.bmRequestType,
        USB_REQUEST_CLEAR_FEATURE,
        USB_FEATURE_ENDPOINT_STALL,
                (uint16_t) ((uint16_t) (pBulkEp->pInEndpoint->byEndpointNumber | USB_DEVICE_TO_HOST )), 0,
                NULL);
        if (!iErr)
        {
            pBulkEp->pInEndpoint->dataPID = USBH_DATA0;
        }
        usbRequest.Field.Direction = USB_HOST_TO_DEVICE;
        usbRequest.Field.Type = USB_STANDARD_REQUEST;
        usbRequest.Field.Recipient = USB_RECIPIENT_ENDPOINT;

        /* Clear feature HALT Bulk OUT endpoint, see section 5.3.4 */
        iErr = blkepDeviceRequest(pBulkEp, usbRequest.bmRequestType,
        USB_REQUEST_CLEAR_FEATURE,
        USB_FEATURE_ENDPOINT_STALL, (uint16_t) ((uint16_t) pBulkEp->pOutEndpoint->byEndpointNumber), 0,
        NULL);
        if (!iErr)
        {
            pBulkEp->pOutEndpoint->dataPID = USBH_DATA0;
        }
    }
    return iErr;
}
/******************************************************************************
 End of function  blkepMsReset
 ******************************************************************************/

/******************************************************************************
 Function Name: blkEpClearInStall
 Description:   Function to clear an IN stall
 Arguments:     IN  pBulkEp - Pointer to the driver extension
 Return value:  0 for success or error code
 ******************************************************************************/
static int blkEpClearInStall (PBULKEP pBulkEp)
{

    int iErr;
    USBRQ usbRequest;
    usbRequest.Field.Direction = USB_HOST_TO_DEVICE;
    usbRequest.Field.Type = USB_STANDARD_REQUEST;
    usbRequest.Field.Recipient = USB_RECIPIENT_ENDPOINT;

    /* Clear feature HALT Bulk IN endpoint, see section 5.3.4 */
    iErr = blkepDeviceRequest(pBulkEp, usbRequest.bmRequestType,
    USB_REQUEST_CLEAR_FEATURE,
    USB_FEATURE_ENDPOINT_STALL, (uint16_t) ((uint16_t) (pBulkEp->pInEndpoint->byEndpointNumber | USB_DEVICE_TO_HOST )),
            0,
            NULL);
    return iErr;
}
/******************************************************************************
 End of function  blkEpClearInStall
 ******************************************************************************/

/******************************************************************************
 Function Name: blkepGetMaxLun
 Description:   Function to send the USB MS Bulk Only Class get max lun vendor
 request
 Arguments:     IN  pBulkEp - Pointer to the driver extension
 OUT pbyMaxLun - The maximum number of "disks"
 (Logical Unit Number)
 Return value:  0 for success or error code
 ******************************************************************************/
static int blkepGetMaxLun (PBULKEP pBulkEp, uint8_t *pbyMaxLun)
{
    int iErr;
    uint32_t dwResult;
    iErr = blkepDeviceRequest(pBulkEp, 0xA1, /* Class, Interface, Host to device */
    0xFE, /* MS class reset request */
    0, /* Value field set to zero */
    /* Index field set to the interface number*/
    (uint16_t) pBulkEp->pDevice->byInterfaceNumber, 1, /* Length field set to one */
    (uint8_t *) &dwResult);
    if ((!iErr) && (pbyMaxLun))
    {
#ifdef _BIG
        *pbyMaxLun = (uint8_t)(dwResult >> 24);
#else
        *pbyMaxLun = (uint8_t) (dwResult);
#endif
    }
    return iErr;
}
/******************************************************************************
 End of function  blkepGetMaxLun
 ******************************************************************************/

/******************************************************************************
 Function Name: blkepControl
 Description:   Function to handle custom controls for the bulk endpoints
 Arguments:     IN  st_stream_ptr_t - Pointer to the file stream
 IN  ctlCode - The custom control code
 IN  pCtlStruct - Pointer to the custom control structure
 Return value:  0 for success -1 on error
 ******************************************************************************/
static int blkepControl (st_stream_ptr_t pstream, uint32_t ctlCode, void *pCtlStruct)
{
    PBULKEP pBulkEp = pstream->p_extension;
    switch (ctlCode)
    {
        case CTL_GET_LAST_ERROR :
        {
            if (pCtlStruct)
            {
                *((PBLKERR) pCtlStruct) = pBulkEp->lastError;
                return BULK_EP_NO_ERROR;
            }
            break;
        }

        case CTL_GET_ERROR_STRING :
        {
            if (pCtlStruct)
            {
                PERRSTR pErrStr = (PERRSTR) pCtlStruct;
                if (pErrStr->iErrorCode < BULK_EP_NUM_ERRORS)
                {
                    pErrStr->pszErrorString = (int8_t *) gpszErrorString[pErrStr->iErrorCode];
                    return BULK_EP_NO_ERROR;
                }
            }
            break;
        }

        case CTL_SET_TIME_OUT :
            if (pCtlStruct)
            {
                pBulkEp->dwTimeOut_mS = *((uint32_t *) pCtlStruct);
            }
            else
            {
                pBulkEp->dwTimeOut_mS = BLK_EP_DEFAULT_TIME_OUT;
            }
            return BULK_EP_NO_ERROR;

        case CTL_USB_MS_RESET :
            return blkepMsReset(pBulkEp);

        case CTL_USB_MS_GET_MAX_LUN :
        {
            if (pCtlStruct)
            {
                return blkepGetMaxLun(pBulkEp, (uint8_t *) pCtlStruct);
            }
            break;
        }

        case CTL_USB_MS_CLEAR_BULK_IN_STALL :
            return blkEpClearInStall(pBulkEp);

        case CTL_USB_ATTACHED :
        {
            if (pCtlStruct)
            {
                if (!(strcmp((char *) pBulkEp->pszStreamName, pstream->p_stream_name))
                        && (usbhGetDevice((int8_t *) pstream->p_stream_name)))
                {
                    *((_Bool *) pCtlStruct) = true;
                }
                else
                {
                    *((_Bool *) pCtlStruct) = false;
                }
                return BULK_EP_NO_ERROR;
            }
            break;
        }

        case CTL_USB_MS_WAIT_MUTEX :
        {
            R_OS_EventWaitMutex(&pBulkEp->evDeviceMutex, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);
            break;
        }

        case CTL_USB_MS_RELEASE_MUTEX :
        {
            R_OS_EventReleaseMutex(&pBulkEp->evDeviceMutex);
            break;
        }

        default :
            return BULK_EP_INVALID_CONTROL_CODE;
    }

    return BULK_EP_NO_ERROR;
}
/******************************************************************************
 End of function  blkepControl
 ******************************************************************************/

/******************************************************************************
 Function Name: blkepGetFirstEndpoint
 Description:   Function to get the first bulk endpoint with matching transfer
 direction
 Arguments:     IN  pDevice - Pointer to the deivice information
 IN  transferDirection - The endpoint transfer direction
 Return value:  Pointer to the endpoint information or NULL if not found
 ******************************************************************************/
static PUSBEI blkepGetFirstEndpoint (PUSBDI pDevice, USBDIR transferDirection)
{
    PUSBEI pEndpoint;
    if (transferDirection != USBH_SETUP)
    {
        pEndpoint = pDevice->pEndpoint;
        while (pEndpoint)
        {
            if ((pEndpoint->transferDirection == transferDirection) && (pEndpoint->transferType == USBH_BULK))
            {
                return pEndpoint;
            }
            pEndpoint = pEndpoint->pNext;
        }
    }
    return NULL;
}
/******************************************************************************
 End of function  blkepGetFirstEndpoint
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
