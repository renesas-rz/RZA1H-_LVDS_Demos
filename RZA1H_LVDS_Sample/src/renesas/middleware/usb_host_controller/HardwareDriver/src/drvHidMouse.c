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
 * File Name    : drvHidMouse.c
 * Version      : 1.00
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : Device driver for HID class mouse.
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
#include "ddusbh.h"
#include "usbhDeviceApi.h"
#include "r_event.h"
#include "r_cbuffer.h"

#include "r_task_priority.h"
#include "trace.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/

#define HID_MSE_REPORT_SIZE         8
#define HID_MSE_CHAR_BUFFER_SIZE    (32 * sizeof(MDAT))

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Typedef definitions
 ******************************************************************************/

typedef enum
{
    MSE_IDLE = 0, MSE_WAIT_REPORT

} MSEST;

#pragma pack(1)

typedef struct _HIDMSE
{
    /* The buffer used to hold the characters */
    PCBUFF   pMseBuffer;
    /* The transfer request structures */
    USBTR    readRequest;
    USBTR    deviceRequest;
    _Bool    bfDeviceRequestTimeOut;
    uint16_t wLengthTransferred;
    /* Pointer to the device and endpoints */
    PUSBDI   pDevice;
    PUSBEI   pInEndpoint;
    /* The last error code */
    MSEERR   errorCode;
    /* The stream name of the device */
    int8_t   pszStreamName[DEVICE_MAX_STRING_SIZE];
    /* Force the alignment to four byte boundary for data transfer */
    uint32_t dwAlign;
    /* The report descriptor buffer */
    uint8_t  pbyReportDescriptor[64];
    /* Task for polling the mouse */
    os_task_t * uiTaskID;
    /* The state of the mouse */
    MSEST    mseState;
    /* The key map of the buttons */
    uint8_t  byKeyMap;
    /* An event to prevent this from using all the CPU time */
    event_t  pevNewData;

} HIDMSE, *PHIDMSE;

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

static int_t mseOpen (st_stream_ptr_t pStream);
static void mseClose (st_stream_ptr_t pStream);
static int_t mseControl (st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct);
static PUSBEI mseGetFirstEndpoint (PUSBDI pDevice, USBDIR transferDirection);
static _Bool mseSetInterval (PHIDMSE pHidMse, uint8_t byInterval);
static void mse_poll_task (PHIDMSE pHidMse);

/******************************************************************************
 Constant Data
 ******************************************************************************/

/* Define the driver function table for this device */
const st_r_driver_t gHidMouseDriver =
{
    "HID Mouse Device Driver",
    mseOpen,
    mseClose,
    no_dev_io,
    no_dev_io,
    mseControl,
    no_dev_get_version
};

/******************************************************************************
 Private functions
 ******************************************************************************/

/******************************************************************************
 Function Name: mseOpen
 Description:   Function to open the mouse driver
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  0 for success otherwise -1
 ******************************************************************************/
static int mseOpen (st_stream_ptr_t pStream)
{
    /* Get a pointer to the device information from the host driver for this device */
    PUSBDI pDevice = usbhGetDevice((int8_t *) pStream->p_stream_name);

    if (pDevice)
    {
        /* This driver supports HID mouses with IN and OUT endpoints although
         only the IN endpoint is used in this sample code */
        PUSBEI pIn = mseGetFirstEndpoint(pDevice, USBH_IN);

        /* If the device has the two endpoints */
        if (pIn)
        {
            /* Create a buffer for the mouse data */
            PCBUFF pMseBuffer = cbCreate(HID_MSE_CHAR_BUFFER_SIZE);
            if (pMseBuffer)
            {
                /* Allocate the memory for the driver */
                pStream->p_extension = R_OS_AllocMem(sizeof(HIDMSE), R_REGION_LARGE_CAPACITY_RAM);

                if (pStream->p_extension)
                {
                    PHIDMSE pHidMse = pStream->p_extension;

                    /* Initialise the driver data */
                    memset(pHidMse, 0, sizeof(HIDMSE));

                    /* Set the buffer pointer */
                    pHidMse->pMseBuffer = pMseBuffer;

                    /* Copy in the link name */
                    strncpy((char_t *) pHidMse->pszStreamName, pStream->p_stream_name, DEVICE_MAX_STRING_SIZE);

                    /* Set the device and endpoint information found */
                    pHidMse->pDevice = pDevice;
                    pHidMse->pInEndpoint = pIn;

                    /* Add the events to the transfer requests */
                    R_OS_CreateEvent(&pHidMse->readRequest.ioSignal);
                    R_OS_CreateEvent(&pHidMse->deviceRequest.ioSignal);

                    /* Start a task to poll the mouse */
                    pHidMse->uiTaskID = R_OS_CreateTask("HID Mouse", (os_task_code_t) mse_poll_task, pHidMse, R_OS_ABSTRACTION_PRV_SMALL_STACK_SIZE, TASK_HID_MOUSE_PRI);

                    R_OS_CreateEvent(&pHidMse->pevNewData);

                    /* Set the report interval to once every 8mS */
                    mseSetInterval(pHidMse, 2);
                    TRACE(("mseOpen: Opened device %s\r\n", pStream->pszStreamName));
                    return HID_MSE_OK;
                }
                else
                {
                    TRACE(("mseOpen: Failed to allocate memory\r\n"));
                }
            }
            else
            {
                TRACE(("mseOpen: Failed to create mouse buffer\r\n"));
            }

            return HID_MSE_MEMORY_ALLOCATION_ERROR;
        }
        else
        {
            TRACE(("mseOpen: Failed to find interrupt IN endpoint\r\n"));
            return HID_MSE_ENDPOINT_NOT_FOUND;
        }
    }
    else
    {
        TRACE(("mseOpen: Failed to find device %s\r\n", pStream->pszStreamName));
    }

    return HID_MSE_DEVICE_NOT_FOUND;
}
/******************************************************************************
 End of function  mseOpen
 ******************************************************************************/

/******************************************************************************
 Function Name: mseClose
 Description:   Function to close the bulk endpoint driver
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  none
 ******************************************************************************/
static void mseClose (st_stream_ptr_t pStream)
{
    PHIDMSE pHidMse = pStream->p_extension;
    TRACE(("mseClose:\r\n"));

    /* If there is a request in progress */
    if (usbhTransferInProgress(&pHidMse->readRequest))
    {
        /* Cancel it */
        usbhCancelTransfer(&pHidMse->readRequest);
    }

    /* Destroy the task */
    R_OS_DeleteTask(pHidMse->uiTaskID);

    R_OS_DeleteEvent(&pHidMse->readRequest.ioSignal);
    R_OS_DeleteEvent(&pHidMse->deviceRequest.ioSignal);

    R_OS_DeleteEvent(&pHidMse->pevNewData);

    /* Destroy the mouse data buffer */
    cbDestroy(pHidMse->pMseBuffer);

    /* Free the driver extension */
    R_OS_FreeMem(pHidMse);
}
/******************************************************************************
 End of function  mseClose
 ******************************************************************************/

/******************************************************************************
 Function Name: mseControl
 Description:   Function to handle custom control functions for the mouse
 Arguments:     IN  pStream - Pointer to the file stream
                IN  ctlCode - The custom control code
                IN  pCtlStruct - Pointer to the custom control structure
 Return value:  1 for success 0 for no data and -1 on error
 ******************************************************************************/
static int_t mseControl (st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct)
{
    PHIDMSE pHidMse = pStream->p_extension;
    UNUSED_PARAM(pCtlStruct);

    if (CTL_GET_MOUSE_DATA == ctlCode)
    {

        if (cbUsed(pHidMse->pMseBuffer))
        {
            uint8_t * pbyDest = (uint8_t *) pCtlStruct;
            cbGet(pHidMse->pMseBuffer, pbyDest++);
            cbGet(pHidMse->pMseBuffer, pbyDest++);
            cbGet(pHidMse->pMseBuffer, pbyDest++);
            cbGet(pHidMse->pMseBuffer, pbyDest);
            return 1;
        }
        /* Wait here if there is no new data */
        R_OS_WaitForEvent(&pHidMse->pevNewData, 200);

        return 0;
    }

    return -1;
}
/******************************************************************************
 End of function  mseControl
 ******************************************************************************/

/******************************************************************************
 Function Name: mseDeviceRequest
 Description:   Function to send a device request
 Arguments:     IN  pHidMse - Pointer to the driver extension
                IN  bRequest - The request
                IN  bmRequestType - The request type
                IN  wValue - The Value
                IN  wIndex - The Index
                IN  wLength - The length of the data
                IN/OUT pbyData - Pointer to the data
 Return value:  0 for success or error code (control.h)
 ******************************************************************************/
static int mseDeviceRequest (PHIDMSE pHidMse, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
        uint16_t wLength, uint8_t *pbyData)
{
    if (usbhDeviceRequest(&pHidMse->deviceRequest, pHidMse->pDevice, bmRequestType, bRequest, wValue, wIndex, wLength,
            pbyData))
    {
        pHidMse->errorCode = HID_MSE_REQUEST_ERROR;
        return HID_MSE_REQUEST_ERROR;
    }

    return HID_MSE_OK;
}
/******************************************************************************
 End of function  mseDeviceRequest
 ******************************************************************************/

/******************************************************************************
 Function Name: mseGetFirstEndpoint
 Description:   Function to get the first bulk endpoint with matching transfer direction
 Arguments:     IN  pDevice - Pointer to the device information
                IN  transferDirection - The endpoint transfer direction
 Return value:  Pointer to the endpoint information or NULL if not found
 ******************************************************************************/
static PUSBEI mseGetFirstEndpoint (PUSBDI pDevice, USBDIR transferDirection)
{
    PUSBEI pEndpoint;

    if (transferDirection != USBH_SETUP)
    {
        pEndpoint = pDevice->pEndpoint;

        while (pEndpoint)
        {
            if ((pEndpoint->transferDirection == transferDirection) && (pEndpoint->transferType == USBH_INTERRUPT))
            {
                return pEndpoint;
            }

            pEndpoint = pEndpoint->pNext;
        }
    }

    return NULL;
}
/******************************************************************************
 End of function  mseGetFirstEndpoint
 ******************************************************************************/

/******************************************************************************
 Function Name: mseStartTransfer
 Description:   Function to start an IN transfer
 Arguments:     IN  pHidMse - Pointer to the driver extension
 Return value:  true if the transfer was started
 ******************************************************************************/
static _Bool mseStartTransfer (PHIDMSE pHidMse)
{
    _Bool bfResult;

    bfResult = usbhStartTransfer(pHidMse->pDevice, &pHidMse->readRequest, pHidMse->pInEndpoint,
            pHidMse->pbyReportDescriptor, (size_t) pHidMse->pInEndpoint->wPacketSize,
            REQ_IDLE_TIME_OUT_INFINITE);

    return bfResult;
}
/******************************************************************************
 End of function  mseStartTransfer
 ******************************************************************************/

/******************************************************************************
 Function Name: mseSetInterval
 Description:   Function to set the report interval
 Arguments:     IN  pHidMse - Pointer to the driver extension
                IN  byInterval - The report interval in 4mS steps
 Return value:  true if the interval is set
 ******************************************************************************/
static _Bool mseSetInterval (PHIDMSE pHidMse, uint8_t byInterval)
{
    if (mseDeviceRequest(pHidMse, (uint8_t) (0x20 | USB_CLASS_REQUEST),
            USBHID_REQUEST_SET_IDLE, USB_SET_VALUE(byInterval, 0), 0, 0, NULL) == HID_MSE_OK)
    {
        return true;
    }

    return false;
}
/******************************************************************************
 End of function  mseSetInterval
 ******************************************************************************/

/******************************************************************************
 Function Name: mseParseReport
 Description:    Parse the report from the mouse
 Arguments:     IN  pHidMse - Pointer to the driver extension
                IN  pbyReport - Pointer to the report
 Return value:  none
 ******************************************************************************/
static void mseParseReport (PHIDMSE pHidMse, uint8_t *pbyReport)
{
    /* The report buffer must be 4 byte aligned for the USB transfer so this is OK */

    /* Check to see if any one of the first four bytes is non zero */
    if (*((uint32_t *) pbyReport))
    {
        pHidMse->byKeyMap = *pbyReport;

        /* The _MDAT structure matches that of the mouse data */
        cbPut(pHidMse->pMseBuffer, *pbyReport++);
        cbPut(pHidMse->pMseBuffer, *pbyReport++);
        cbPut(pHidMse->pMseBuffer, *pbyReport++);
        cbPut(pHidMse->pMseBuffer, *pbyReport);
        R_OS_SetEvent(&pHidMse->pevNewData);
    }
    else if (pHidMse->byKeyMap != *pbyReport)
    {
        /* Update the key map and update the buttons */
        pHidMse->byKeyMap = *pbyReport;
        cbPut(pHidMse->pMseBuffer, *pbyReport);
        cbPut(pHidMse->pMseBuffer, 0);
        cbPut(pHidMse->pMseBuffer, 0);
        cbPut(pHidMse->pMseBuffer, 0);
        R_OS_SetEvent(&pHidMse->pevNewData);
    }
}
/******************************************************************************
 End of function  mseParseReport
 ******************************************************************************/

/******************************************************************************
 Function Name: mse_poll_task
 Description:   Task to poll the mouse for data
 Arguments:     IN  pHidMse - Pointer to the driver extension
 Return value:  none
 ******************************************************************************/
static void mse_poll_task (PHIDMSE pHidMse)
{
    while (1)
    {
        switch (pHidMse->mseState)
        {
            case MSE_IDLE:
            {
                /* Start a transfer */
                if (mseStartTransfer(pHidMse))
                {
                    pHidMse->mseState = MSE_WAIT_REPORT;
                }
                break;
            }

            case MSE_WAIT_REPORT:
            {
                /* Check the state of the transfer */
                if (!usbhTransferInProgress(&pHidMse->readRequest))
                {
                    /* Check the request error code */
                    if (pHidMse->readRequest.errorCode)
                    {
                        /* Set the error code for return to read function */
                        pHidMse->errorCode = HID_MSE_REPORT_ERROR;
                    }
                    else
                    {
                        /* Parse the report descriptor */
                        mseParseReport(pHidMse, pHidMse->pbyReportDescriptor);
                    }

                    /* Start the next transfer */
                    if (!mseStartTransfer(pHidMse))
                    {
                        pHidMse->mseState = MSE_IDLE;
                    }
                }
                break;
            }
        }

        R_OS_TaskSleep(8UL);
    }
}
/******************************************************************************
 End of function  mse_poll_task
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
