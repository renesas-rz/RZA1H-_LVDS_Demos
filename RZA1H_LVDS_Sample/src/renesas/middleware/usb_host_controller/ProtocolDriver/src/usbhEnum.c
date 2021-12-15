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
 * Copyright (C) 2016 Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************
 * File Name    : usbhEnum.c
 * Version      : 1.51
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : USB host enumerator
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 First Release
 *              : 10.11.2010 1.01 Added timeout to emum suspend
 *              : 14.12.2010 1.02 Added error control
 *              : 29.02.2011 1.03 Added _OS_ conditional code.
 *              : 17.03.2011 1.04 Reduced descriptor size as some MS device fail
 *              : 28.03.2012 1.05 Fixed Bug #94 Removal of "not responding" dev.
 *              : 28.05.2015 1.50 Updated for the GNU Device
 *              : 12.01.2016 1.51 Increased device request timeouts
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
#include <string.h>

#include "iodefine_cfg.h"
#include "compiler_settings.h"

#include "usbhEnum.h"
#include "usbhDeviceApi.h"
#include "usbhDriverInternal.h"
#include "usbhClass.h"
#include "endian.h"
#include "trace.h"
#include "FreeRTOS.h"
#include "task.h"
#include "r_devlink_wrapper_cfg.h"
/******************************************************************************
 Macro definitions
 ******************************************************************************/

/* Times are related to enumRun call frequency. This should be called at 1kHz
 so delay times will be in mS */
#define ENUM_PORT_POLL_DELAY_COUNT  100UL
#define ENUM_PORT_POWER_DELAY_COUNT 100UL
#define ENUM_PORT_ENABLE_DELAY_COUNT 50UL
#define ENUM_ROOT_PORT_RESET_COUNT  50UL
#define ENUM_HUB_PORT_RESET_COUNT   10UL
#define ENUM_DEV_REQUEST_COUNT_OUT  500UL
/* The number of attempts to enumerate a device */
#define ENUM_RETRY_COUNT            8
#define ENUM_DESCRIPTOR_BUFFER_SIZE 255

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Typedef definitions
 ******************************************************************************/

typedef enum _ENUMST
{
    ENUM_IDLE = 0,
    ENUM_PORT_POLL_DELAY,
    ENUM_PORT_STATE_CHANGE,
    ENUM_CHECK_PORT_STATUS,
    ENUM_DEVICE_RETRY_LOOP,
    ENUM_DEVICE_SET_RESET_PORT_1,
    ENUM_DEVICE_RESET_HOLD_1,
    ENUM_DEVICE_CLEAR_RESET_PORT_1,
    ENUM_DEVICE_CONFIRM_PORT_STATUS,
    ENUM_DEVICE_ENABLE_PORT_1,
    ENUM_DEVICE_SLEEP_1,
    ENUM_DEVICE_SPEED,
    ENUM_DEVICE_REQUEST_DESCRIPTOR_8,
    ENUM_DEVICE_SET_RESET_PORT_2,
    ENUM_DEVICE_RESET_HOLD_2,
    ENUM_DEVICE_CLEAR_RESET_PORT_2,
    ENUM_DEVICE_ENABLE_PORT_2,
    ENUM_DEVICE_SLEEP_2,
    ENUM_DEVICE_REQUEST_DESCRIPTOR_ALL,
    ENUM_DEVICE_SET_RESET_PORT_3,
    ENUM_DEVICE_RESET_HOLD_3,
    ENUM_DEVICE_CLEAR_RESET_PORT_3,
    ENUM_DEVICE_ENABLE_PORT_3,
    ENUM_DEVICE_SLEEP_3,
    EMUM_DEVICE_SET_ADDRESS,
    ENUM_DEVICE_REQUEST_CONFIGURATION,
    ENUM_DEVICE_SET_CONFIGURATION,
    ENUM_DEVICE_REQUEST_MANUFACTURER_STRING,
    ENUM_DEVICE_COPY_MANUFACTURER_STRING,
    ENUM_DEVICE_REQUEST_PRODUCT_STRING,
    ENUM_DEVICE_COPY_PRODUCT_STRING,
    ENUM_DEVICE_REQUEST_SERIAL_NUMBER_STRING,
    ENUM_DEVICE_COPY_SERIAL_NUMBER_STRING,
    ENUM_DEVICE_COMPLETE,
    ENUM_CONFIGURE_HUB,
    ENUM_HUB_STATE_CHANGE,
    ENUM_CHECK_HUB_STATUS,
    ENUM_NUM_FUNCTIONS

} ENUMST;

#ifdef _TRACE_ON_
static const char_t state_str[][40] =
{
        "ENUM_IDLE",
        "ENUM_PORT_POLL_DELAY",
        "ENUM_PORT_STATE_CHANGE",
        "ENUM_CHECK_PORT_STATUS",
        "ENUM_DEVICE_RETRY_LOOP",
        "ENUM_DEVICE_SET_RESET_PORT_1",
        "ENUM_DEVICE_RESET_HOLD_1",
        "ENUM_DEVICE_CLEAR_RESET_PORT_1",
        "ENUM_DEVICE_CONFIRM_PORT_STATUS",
        "ENUM_DEVICE_ENABLE_PORT_1",
        "ENUM_DEVICE_SLEEP_1",
        "ENUM_DEVICE_SPEED",
        "ENUM_DEVICE_REQUEST_DESCRIPTOR_8",
        "ENUM_DEVICE_SET_RESET_PORT_2",
        "ENUM_DEVICE_RESET_HOLD_2",
        "ENUM_DEVICE_CLEAR_RESET_PORT_2",
        "ENUM_DEVICE_ENABLE_PORT_2",
        "ENUM_DEVICE_SLEEP_2",
        "ENUM_DEVICE_REQUEST_DESCRIPTOR_ALL",
        "ENUM_DEVICE_SET_RESET_PORT_3",
        "ENUM_DEVICE_RESET_HOLD_3",
        "ENUM_DEVICE_CLEAR_RESET_PORT_3",
        "ENUM_DEVICE_ENABLE_PORT_3",
        "ENUM_DEVICE_SLEEP_3",
        "EMUM_DEVICE_SET_ADDRESS",
        "ENUM_DEVICE_REQUEST_CONFIGURATION",
        "ENUM_DEVICE_SET_CONFIGURATION",
        "ENUM_DEVICE_REQUEST_MANUFACTURER_STRING",
        "ENUM_DEVICE_COPY_MANUFACTURER_STRING",
        "ENUM_DEVICE_REQUEST_PRODUCT_STRING",
        "ENUM_DEVICE_COPY_PRODUCT_STRING",
        "ENUM_DEVICE_REQUEST_SERIAL_NUMBER_STRING",
        "ENUM_DEVICE_COPY_SERIAL_NUMBER_STRING",
        "ENUM_DEVICE_COMPLETE",
        "ENUM_CONFIGURE_HUB",
        "ENUM_HUB_STATE_CHANGE",
        "ENUM_CHECK_HUB_STATUS",
        "ENUM_NUM_FUNCTIONS"
};
#endif
typedef enum _DEVREQ
{
    DEVREQ_IDLE = 0, DEVREQ_SETUP, DEVREQ_DATA, DEVREQ_STATUS, DEVREQ_NUM_STATES

} DEVREQ;

/******************************************************************************
 Constant Data
 ******************************************************************************/

#ifdef _TRACE_ON_
const int8_t * const gpszRequestErrorString[] =
{
    "REQ_NO_ERROR",
    "REQ_CRC_ERROR",
    "REQ_BIT_STUFFING_ERROR",
    "REQ_DATA_PID_MISS_MATCH_ERROR",
    "REQ_STALL_ERROR",
    "REQ_NOT_RESPONDING_ERROR",
    "REQ_PID_CHECK_ERROR",
    "REQ_UNEXPECTED_PID_ERROR",
    "REQ_DATA_OVERRUN_ERROR",
    "REQ_DATA_UNDERRUN_ERROR",
    "REQ_USBH_BABBLE_ERROR",
    "REQ_USBH_XACT_ERROR",
    "REQ_BUFFER_OVERRUN_ERROR",
    "REQ_BUFFER_UNDERRUN_ERROR",
    "REQ_FIFO_WRITE_ERROR",
    "REQ_FIFO_READ_ERROR",
    "REQ_INVALID_PARAMETER",
    "REQ_SETUP_PHASE_TIME_OUT",
    "REQ_DATA_PHASE_TIME_OUT",
    "REQ_STATUS_PHASE_TIME_OUT",
    "REQ_ENDPOINT_NOT_FOUND",
    "REQ_DEVICE_NOT_FOUND",
    "REQ_INVALID_ADDRESS",
    "REQ_IDLE_TIME_OUT",
    "REQ_SIGNAL_CREATE_ERROR",
    "REQ_PENDING"
};
const int8_t * const pszDeviceSpeedString[] =
{
    "USBH_FULL",
    "USBH_SLOW",
    "USBH_HIGH"
};
#endif

/* Define the driver function table for this */
const st_r_driver_t g_usb_hub_driver =
{ "Generic Hub Driver", no_dev_open, no_dev_close, no_dev_io, no_dev_io, no_dev_control, no_dev_get_version};

extern  int_t no_dev_io(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);

/******************************************************************************
 Imported global variables and functions (from other files)
 ******************************************************************************/


//extern const DEVICE gNoDriver;

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/
//static void enumDeviceRequestStatusPhase (void);
static void enumProcessDeviceRequest (void);
static void enumProcessStateFunction (void);
static void enumPowerPort (PUSBPI pPort, _Bool bfState);
static void enumResetPort (PUSBPI pPort, _Bool bfState);
static void enumEnablePort (PUSBPI pPort, _Bool bfState);
#if 0
static void enumSuspendPort (PUSBPI pPort, _Bool bfState);
#endif
static void enumGetPortStatus (PUSBPI pPort);

void enumInit (void);
void enumUninit (void);
void enumSuspend (void);
void enumResume (void);
void enumDeviceRequestIdle (void);
void enumConfigureHub (void);
void enumDeviceConfirmPortStatus (void);
void enumDeviceSetResetPort_SetDescriptorLength (void);
void enumCheckHubStatus (void);
void enumHubStateChange (void);

static struct _USBENUM
{
    /* The current state of the enumerator */
    ENUMST   currentState;
    /* Pointer to the current port */
    PUSBPI   pPort;
    /* The variables used by the state machine */
    uint32_t dwDelayCount;
    int      iTryCount;
    int      iHcIndex;
    PUSBDI   pDevice;
    PDEVICE  pDeviceDriver;
    int8_t   chAddress;
    uint8_t  bManufacturer;
    uint8_t  bProduct;
    uint8_t  bSerialNumber;
    uint32_t dwPortStatus;
    uint32_t dwHubStatus;
    PUSBHI   pLastHub;
    /* The current state of the device request */
    DEVREQ   reqStatus;
    /* The variables used by the device request state machine */
    uint32_t dwReqCountOut;
    USBDR    deviceRequest;
    size_t   stLengthTransferred;
    size_t   stDeviceDescriptorLength;
    USBTS    transferSpeed;
    USBTR    setupRequest;
    USBTR    dataRequest;
    USBTR    statusRequest;
    uint8_t  pbyDescriptor[ENUM_DESCRIPTOR_BUFFER_SIZE];
    DEVINFO  Information;
    int      iSuspend;
    uint32_t uiDeviceID;

} gUsbEnum;

/******************************************************************************
 Exported global variables and functions (to be accessed by other files)
 ******************************************************************************/

/******************************************************************************
 Function Name: enumInit
 Description:   Function to initialise the enumerator
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumInit (void)
{
    memset(&gUsbEnum, 0, sizeof(gUsbEnum));

    /* Create the events for the transfer requests used by the device request */
    R_OS_CreateEvent(&gUsbEnum.setupRequest.ioSignal);
    R_OS_CreateEvent(&gUsbEnum.dataRequest.ioSignal);
    R_OS_CreateEvent(&gUsbEnum.statusRequest.ioSignal);
}
/******************************************************************************
 End of function  enumInit
 ******************************************************************************/

/*****************************************************************************
 Function Name: enumUninit
 Description:   Function to uninitialise the enumerator
 Arguments:     none
 Return value:  none
 *****************************************************************************/
void enumUninit (void)
{
    /* Destroy the events */
    R_OS_DeleteEvent(&gUsbEnum.setupRequest.ioSignal);
    R_OS_DeleteEvent(&gUsbEnum.dataRequest.ioSignal);
    R_OS_DeleteEvent(&gUsbEnum.statusRequest.ioSignal);
}
/*****************************************************************************
 End of function  enumUninit
 ******************************************************************************/

/******************************************************************************
 Function Name: enumSuspend
 Description:   Function to suspend the enumerator
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumSuspend (void)
{
    while (gUsbEnum.reqStatus)
    {
        /* Wait for pending Enumerator activity complete */
        vTaskDelay(1);

        /* Previously sysYield() was used here. However in corner cases
         * the calling task was at a priority equal to or higher than the
         * enumerator task, never allowing OS time for the enumerator to
         * complete - causing this loop to be infinite
         * Now just wait for 1 tick and take another look.
         */
    }
    gUsbEnum.dwReqCountOut = ENUM_DEV_REQUEST_COUNT_OUT;
    gUsbEnum.iSuspend++;
}
/******************************************************************************
 End of function  enumSuspend
 ******************************************************************************/

/******************************************************************************
 Function Name: enumResume
 Description:   Function to resume the enumerator
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumResume (void)
{
    gUsbEnum.iSuspend--;
}
/******************************************************************************
 End of function  enumResume
 ******************************************************************************/

/******************************************************************************
 Function Name: enumRun
 Description:   Function to run the enumerator, should be called at 1mS
 intervals. This is the key enumeration timing control.
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumRun (void)
{
    if (!gUsbEnum.iSuspend)
    {
        /* Check for the processing of a device request */
        if (gUsbEnum.reqStatus)
        {
            /* Process the device request state machine */
            enumProcessDeviceRequest();
        }
        else
        { /* Process the enumeration state machine */
            enumProcessStateFunction();
        }
    }
    else
    {
        /* There is a maximum time that the enumerator can be suspended. This
         is because when a device is removed from the root port the USB
         module stops generating SOF interrupts. The SOF interrupt is used
         to time-out the requests and they get stuck. This is to resume
         operation of the enumerator in that circumstance */
        if (gUsbEnum.dwReqCountOut)
        {
            gUsbEnum.dwReqCountOut--;
        }
        else
        {
            gUsbEnum.iSuspend = 0;
        }
    }
}
/******************************************************************************
 End of function  enumRun
 ******************************************************************************/

/******************************************************************************
 Private Functions
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceRequestIdle
 Description:   Function to handle the DEVREQ_IDLE state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumDeviceRequestIdle (void)
{
    TRACE(("enumDeviceRequestIdle: Invalid state\r\n"));
}
/******************************************************************************
 End of function  enumDeviceRequestIdle
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceRequest
 Description:   Function to send a standard device request to a device
 All device request transfers are queued at once to provide
 improved performance. These transfers can all complete in one
 USB frame.
 Arguments:     IN  pDevice - Pointer to the device to send the request to
 IN  bmRequestType - The request type
 IN  bRequest - The request
 IN  wValue - The request value
 IN  wIndex - The request index
 IN  wLength - The request length
 IN/OUT pbyData - Pointer to the memory to use for the data phase
 Return value:
 ******************************************************************************/
static void enumDeviceRequest (PUSBDI pDevice, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue,
        uint16_t wIndex, uint16_t wLength, uint8_t *pbyData)
{
    if (pDevice)
    {
        /* Pointer to the endpoints to make the device request */
        PUSBEI pStatus = NULL, pSetup = pDevice->pControlSetup;
        /* Put the data in to the SETUP packet */
        gUsbEnum.deviceRequest.bmRequestType = bmRequestType;
        gUsbEnum.deviceRequest.bRequest = bRequest;
        gUsbEnum.deviceRequest.wValue = wValue;
        gUsbEnum.deviceRequest.wIndex = wIndex;
        gUsbEnum.deviceRequest.wLength = wLength;
        /* Set the enumerator's device request state */
        gUsbEnum.reqStatus = DEVREQ_SETUP;
        /* Setup packets are always have a PID of data 0 */
        pSetup->dataPID = USBH_DATA0;
        /* Start the setup packet transfer */
        usbhStartTransfer(pDevice, &gUsbEnum.setupRequest, pSetup, (uint8_t *) &gUsbEnum.deviceRequest, 8, 400UL);
        /* If there is a data phase */
        if (wLength)
        {
            PUSBEI pData;
            /* Select the endpoint to use for this request */
            if (gUsbEnum.deviceRequest.bmRequestType & USB_DEVICE_TO_HOST_MASK)
            {
                pData = pDevice->pControlIn;
                /* The status phase is always in the opposite direction to the
                 data */
                pStatus = pDevice->pControlOut;
            }
            else
            {
                pData = pDevice->pControlOut;
                /* The status phase is always in the opposite direction to the
                 data */
                pStatus = pDevice->pControlIn;
            }
            /* Data phase always has a PID of data 1 */
            pData->dataPID = USBH_DATA1;
            /* Start the data phase */
            usbhStartTransfer(pDevice, &gUsbEnum.dataRequest, pData, pbyData, (size_t) gUsbEnum.deviceRequest.wLength,
                    400UL);
        }
        else
        {
            /* Without a data phase the status phase is IN (Device to host) */
            pStatus = pDevice->pControlIn;
            gUsbEnum.dataRequest.errorCode = USBH_NO_ERROR;
        }
        /* Status phase always has a PID of data 1 */
        pStatus->dataPID = USBH_DATA1;
        /* Start the status phase */
        usbhStartTransfer(pDevice, &gUsbEnum.statusRequest, pStatus,
        NULL, 0, 400UL);
        /* Set the time-out */
        gUsbEnum.dwReqCountOut = ENUM_DEV_REQUEST_COUNT_OUT;
    }
    else
    {
        TRACE(("enumDeviceRequest: Invalid device pointer\r\n"));
        gUsbEnum.reqStatus = DEVREQ_IDLE;
    }
}
/******************************************************************************
 End of function  enumDeviceRequest
 ******************************************************************************/

/******************************************************************************
 Function Name: enumHandleRequestError
 Description:   Function to handle a request error
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumHandleRequestError (void)
{
    /* Look at the requests and report the error */
    if (gUsbEnum.setupRequest.errorCode)
    {
        TRACE(("enumProcessDeviceRequest: Setup error %s\r\n",
                        gpszRequestErrorString[gUsbEnum.setupRequest.errorCode]));
    }
    /* Look at the requests and report the error */
    if (gUsbEnum.dataRequest.errorCode)
    {
        TRACE(("enumProcessDeviceRequest: Data error %s\r\n",
                        gpszRequestErrorString[gUsbEnum.dataRequest.errorCode]));
    }
    /* Look at the requests and report the error */
    if (gUsbEnum.statusRequest.errorCode)
    {
        TRACE(("enumProcessDeviceRequest: Status error %s\r\n",
                        gpszRequestErrorString[gUsbEnum.statusRequest.errorCode]));
    }
    /* Cancel any pending requests */
    usbhCancelTransfer(&gUsbEnum.setupRequest);
    usbhCancelTransfer(&gUsbEnum.dataRequest);
    usbhCancelTransfer(&gUsbEnum.statusRequest);
    /* Reset the request state */
    gUsbEnum.reqStatus = DEVREQ_IDLE;
    /* Check the current state and take appropriate action */
    if (gUsbEnum.currentState == ENUM_DEVICE_SET_RESET_PORT_1)
    {
        /* A port on a hub has failed a status check - probably because it has
         been disconnected. This means that it is worth going back to the
         root ports and checking them rather than checking other ports */
        gUsbEnum.currentState = ENUM_IDLE;
    }
    if (gUsbEnum.currentState == ENUM_PORT_STATE_CHANGE)
    {
        /* Check the next port for state change */
        gUsbEnum.currentState = ENUM_IDLE;
    }
    else if (gUsbEnum.currentState < ENUM_DEVICE_REQUEST_MANUFACTURER_STRING)
    {
        /* Go back to the beginning of the enumeration loop & try again */
        gUsbEnum.currentState = ENUM_DEVICE_RETRY_LOOP;
    }
    else
    {
        /* There has been an error with the transfer of a string descriptor or
         a hub control request which should not re-start the enumeration
         process. Continue with the next state */
        gUsbEnum.currentState++;
    }
    enumProcessStateFunction();
}
/******************************************************************************
 End of function  enumHandleRequestError
 ******************************************************************************/

/******************************************************************************
 Function Name: enumProcessDeviceRequest
 Description:   Function to handle the  state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumProcessDeviceRequest (void)
{
    /* If it is complete */
    if (R_OS_EventState(&gUsbEnum.statusRequest.ioSignal) == EV_SET)
    {
        /* And is without error */
        if ((gUsbEnum.setupRequest.errorCode == USBH_NO_ERROR) && (gUsbEnum.dataRequest.errorCode == USBH_NO_ERROR)
                && (gUsbEnum.statusRequest.errorCode == USBH_NO_ERROR))
        {
            /* Set the transfer length */
            gUsbEnum.stLengthTransferred = gUsbEnum.dataRequest.uiTransferLength;
            /* Show that the request has been completed */
            gUsbEnum.reqStatus = DEVREQ_IDLE;
            /* Go to the next state */
            gUsbEnum.currentState++;
            if (gUsbEnum.currentState >= ENUM_NUM_FUNCTIONS)
            {
                /* This is to prevent jumping off the end of the table only */
                gUsbEnum.currentState = ENUM_IDLE;
            }
            /* Call the next state function from the table */
            enumProcessStateFunction();
        }
        else
        {
            enumHandleRequestError();
        }
    }
    else
    {
        /* Check for time-out */
        if (!gUsbEnum.dwReqCountOut)
        {
            TRACE(("gUsbEnum.dwReqCountOut\r\n"));
            enumHandleRequestError();
        }
#ifndef _OS_
        /* This is a fix for a hardware driver that is processing requests
         without a scheduler - Don't start the count out until the request
         has been processed by the driver */
        else if (gUsbEnum.setupRequest.bfInProgress)
#else
        else
#endif
        {
            /* Decrement the count out */
            gUsbEnum.dwReqCountOut--;
        }
    }
}
/******************************************************************************
 End of function  enumProcessDeviceRequest
 ******************************************************************************/

/******************************************************************************
 Function Name: enumIdle
 Description:   Function to handle the ENUM_IDLE state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumIdle (void)
{
    gUsbEnum.dwDelayCount = ENUM_PORT_POLL_DELAY_COUNT;
    gUsbEnum.currentState = ENUM_PORT_POLL_DELAY;
}
/******************************************************************************
 End of function  enumIdle
 ******************************************************************************/

/******************************************************************************
 Function Name: enumPortPollDelay
 Description:   Function to handle ENUM_PORT_POLL_DELAY state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumPortPollDelay (void)
{
    if (gUsbEnum.dwDelayCount)
    {
        gUsbEnum.dwDelayCount--;
    }
    else
    {
        static int iHubPowerPoll = 0;
        /* Get a pointer to the root port */
        gUsbEnum.pPort = usbhGetRootPort(gUsbEnum.iHcIndex++);
        /* Poll hubs for power state changes once a second */
        if (iHubPowerPoll++ > 5)
        {
            iHubPowerPoll = 0;
            gUsbEnum.pLastHub = NULL;
        }
        gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
    }
}
/******************************************************************************
 End of function  enumPortPollDelay
 ******************************************************************************/

/******************************************************************************
 Function Name: enumPortStateChange
 Description:   Function to handle the ENUM_PORT_STATE_CHANGE state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumPortStateChange (void)
{

    if (gUsbEnum.pPort)
    {
        enumGetPortStatus(gUsbEnum.pPort);
    }
    else
    {
        gUsbEnum.pPort = usbhGetRootPort(gUsbEnum.iHcIndex++);
        if (gUsbEnum.pPort)
        {
            enumGetPortStatus(gUsbEnum.pPort);
        }
        else
        {
            gUsbEnum.iHcIndex = 0;
            gUsbEnum.currentState = ENUM_IDLE;
        }
    }

}
/******************************************************************************
 End of function  enumPortStateChange
 ******************************************************************************/

/******************************************************************************
 Function Name: enumUpdatePortStatus
 Description:   Function to update the current port status. This will already be
 done if it is a root port. If the port it on a hub the status
 needs to be retrieved from the buffer
 Arguments:     none
 Return value:  true if the status has been updated
 ******************************************************************************/
static _Bool enumUpdatePortStatus (void)
{
    /* If the port is on a hub retrieve the status from the buffer */
    if (gUsbEnum.pPort->pHub)
    {
        if (gUsbEnum.stLengthTransferred == sizeof(uint32_t))
        {
            SWAP_ENDIAN_LONG_AT(&gUsbEnum.pPort->dwPortStatus, &gUsbEnum.dwPortStatus);
        }
        else
        {
            return false;
        }
    }
    return true;
}
/******************************************************************************
 End of function  enumUpdatePortStatus
 ******************************************************************************/

/******************************************************************************
 Function Name: enumCheckPortStatus
 Description:   Function to handle the ENUM_CHECK_PORT_STATUS state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumCheckPortStatus (void)
{
    /* Check that the port status was updated */
    if (!enumUpdatePortStatus())
    {
        TRACE(("enumCheckPortStatus: Failed to get port status %d\r\n",
                        gUsbEnum.pPort->uiPortIndex));
        /* This represents an error caused by a hub. This should not stop other
         ports from operating normally so check the next port for state
         change */
        gUsbEnum.pPort = gUsbEnum.pPort->pNext;
        gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
    }
    /* Check that the port is powered */
    if ((gUsbEnum.pPort->dwPortStatus & USBH_HUB_PORT_POWER) == 0)
    {
        TRACE(("enumCheckPortStatus: Port needs power\r\n"));
        /* The port is not powered so apply power */
        enumPowerPort(gUsbEnum.pPort, true);
        /* Return to the idle state */
        gUsbEnum.currentState = ENUM_IDLE;
    }
    else
    {
        /* If there is a device on the port */
        if ((gUsbEnum.pPort->pDevice)
        /* And it has become disconnected */
        && ((gUsbEnum.pPort->dwPortStatus & USBH_HUB_PORT_CONNECT_STATUS) == 0))
        {
            /* Remove the device */
            TRACE(("enumCheckPortStatus: Device [%s] detatched\r\n",
                            gUsbEnum.pPort->pDevice->pszSymbolicLinkName));
            /* Cancel all outstanding transfer requests on this device */
            usbhCancelAllTransferReqests(gUsbEnum.pPort->pDevice);

            /* Remove the device from the list */
            /* WARNING: This function can call free() when the device
             is removed (via close()). This is OK because the malloc
             list is protected by interrupt masking. In other cases
             it would be necessary to close the stream from a task
             not an ISR!! */
            R_DEVLINK_DevRemove((char *) gUsbEnum.pPort->pDevice->pszSymbolicLinkName);

            /* Delete the device information */
            usbhDestroyDeviceInformation(gUsbEnum.pPort->pDevice);
            gUsbEnum.pPort->pDevice = NULL;
        }
        /* If there is no device on the port */
        else if ((!gUsbEnum.pPort->pDevice)
        /* And it has a device on it */
        && (gUsbEnum.pPort->dwPortStatus & USBH_HUB_PORT_CONNECT_STATUS))
        {
            /* Start enumerating */
            TRACE(("enumCheckPortStatus: New Device found\r\n"));
            gUsbEnum.iTryCount = ENUM_RETRY_COUNT;
            gUsbEnum.currentState = ENUM_DEVICE_RETRY_LOOP;
        }
        else
        {
            if ((gUsbEnum.pPort->pHub) && (gUsbEnum.pLastHub != gUsbEnum.pPort->pHub))
            {
                /* Only check each hub once */
                gUsbEnum.pLastHub = gUsbEnum.pPort->pHub;
                gUsbEnum.currentState = ENUM_HUB_STATE_CHANGE;
            }
            else
            {
                /* Check the next port for state change */
                gUsbEnum.pPort = gUsbEnum.pPort->pNext;
                gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
            }
        }
    }
}
/******************************************************************************
 End of function  enumCheckPortStatus
 ******************************************************************************/

/******************************************************************************
 Function Name: enumHubStateChange
 Description:   Function to handle the ENUM_HUB_STATE_CHANGE state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumHubStateChange (void)
{
    /* Send a request to get the status of the hub */
    enumDeviceRequest(gUsbEnum.pPort->pHub->pPort->pDevice,
    USB_GET_HUB_STATUS,
    USB_REQUEST_GET_STATUS, 0, (uint16_t) 0, (uint16_t) sizeof(uint32_t), (uint8_t *) &gUsbEnum.dwHubStatus);
}
/******************************************************************************
 End of function  enumHubStateChange
 ******************************************************************************/

/******************************************************************************
 Function Name: enumCheckHubStatus
 Description:   Function to handle the ENUM_CHECK_HUB_STATUS state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumCheckHubStatus (void)
{
    if (gUsbEnum.stLengthTransferred == sizeof(uint32_t))
    {
        uint32_t dwHubStatus = SWAP_ENDIAN_LONG(gUsbEnum.dwHubStatus);
        /* Do the people that write the USB secification make it
         inconsistent to confuse the readers? Why is it "LOCAL POWER" on a
         hub and "BUS POWERED" and "SELF POWERED" everywhere else? Where do
         the terms "SELF" and "LOCAL" come from? Either it is powered by the
         bus or it has an external power source. Say it like it is! */
        if (dwHubStatus & USB_C_HUB_LOCAL_POWER)
        {
            gUsbEnum.pPort->pHub->bfBusPowered = true;
        }
        else
        {
            gUsbEnum.pPort->pHub->bfBusPowered = false;
        }
        if (dwHubStatus & USB_C_HUB_OVER_CURRENT)
        {
            gUsbEnum.pPort->pHub->bfOverCurrent = true;
            TRACE(("enumCheckHubStatus: USB_C_HUB_OVER_CURRENT\r\n"));
        }
        else
        {
            gUsbEnum.pPort->pHub->bfOverCurrent = false;
        }
    }
    /* Check the next port for state change */
    gUsbEnum.pPort = gUsbEnum.pPort->pNext;
    gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
}
/******************************************************************************
 End of function  enumCheckHubStatus
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceRetryLoop
 Description:   Function to handle the ENUM_DEVICE_RETRY_LOOP state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceRetryLoop (void)
{
    /* If there is temporary device information */
    if (gUsbEnum.pDevice)
    {
        /* See if this is the temporary device on the port */
        if (gUsbEnum.pPort->pDevice == gUsbEnum.pDevice)
        {
            gUsbEnum.pPort->pDevice = NULL;
        }
        /* Destroy the device information */
        usbhDestroyDeviceInformation(gUsbEnum.pDevice);
        gUsbEnum.pDevice = NULL;
        TRACE(("DestroyDevice\r\n"));
    }
    /* Count re-try attempt */
    if (gUsbEnum.iTryCount)
    {
        /* Count the trys */
        gUsbEnum.iTryCount--;
        /* Start by reseting the port */
        gUsbEnum.currentState = ENUM_DEVICE_SET_RESET_PORT_1;
    }
    else
    {
        static uint32_t uiDeviceID = 0;
        /* Create a device for visibility */
        gUsbEnum.pDevice = usbhCreateEnumerationDeviceInformation(gUsbEnum.transferSpeed);
        /* Set the device port pointer */
        gUsbEnum.pDevice->pPort = gUsbEnum.pPort;
        /* Set the port device pointer */
        gUsbEnum.pPort->pDevice = gUsbEnum.pDevice;
        /* Temporary device is now finished with */
        gUsbEnum.pDevice = NULL;
        /* Fill out the device information */
        sprintf(gUsbEnum.Information.pszDeviceLinkName, "Device %d Not Responding", (int)uiDeviceID++);
        strcpy((char*)gUsbEnum.pPort->pDevice->pszSymbolicLinkName, gUsbEnum.Information.pszDeviceLinkName);
        strcpy(gUsbEnum.Information.pszManufacturer, "N/A");
        strcpy(gUsbEnum.Information.pszProduct, "N/A");
        strcpy(gUsbEnum.Information.pszSerialNumber, "N/A");
        gUsbEnum.Information.wVID = 0;
        gUsbEnum.Information.wPID = 0;
        gUsbEnum.Information.wbcdVersion = 0;
        gUsbEnum.Information.byDeviceClass = 0xFF;
        gUsbEnum.Information.byDeviceSubClass = 0xFF;
        gUsbEnum.Information.byProtocolCode = 0xFF;
        /* Link with a the device driver located or a NULL pointer if not
         supported */
        if (!R_DEVLINK_DevAdd(gUsbEnum.Information.pszDeviceLinkName, (st_r_driver_t *) get_no_driver(), &gUsbEnum.Information))
        {
            TRACE(("enumDeviceRetryLoop: Failed to add device\r\n"));
        } TRACE(("enumDeviceRetryLoop: Enumeration failed\r\n"));
        /* Check the next port */
        gUsbEnum.pPort = gUsbEnum.pPort->pNext;
        gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
    }
}
/******************************************************************************
 End of function  enumDeviceRetryLoop
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceSetResetPort
 Description:   Function to handle the ENUM_DEVICE_SET_RESET_PORT_X state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceSetResetPort (void)
{
    enumResetPort(gUsbEnum.pPort, true);
}
/******************************************************************************
 End of function  enumDeviceSetResetPort
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceSetResetPort_SetDescriptorLength
 Description:   Function to handle the ENUM_DEVICE_SET_RESET_PORT_3 state where
 the entire length of the device desctiptor is set
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumDeviceSetResetPort_SetDescriptorLength (void)

{
    gUsbEnum.stDeviceDescriptorLength = gUsbEnum.stLengthTransferred;
    enumDeviceSetResetPort();
}
/******************************************************************************
 End of function  enumDeviceSetResetPort_SetDescriptorLength
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceClearResetPort
 Description:   Function to handle the ENUM_DEVICE_CLEAR_RESET_PORT_X state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceClearResetPort (void)
{
    enumResetPort(gUsbEnum.pPort, false);
}
/******************************************************************************
 End of function  enumDeviceClearResetPort
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceConfirmPortStatus
 Description:   Function to handle the ENUM_DEVICE_CONFIRM_PORT_STATUS state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumDeviceConfirmPortStatus (void)
{
    enumGetPortStatus(gUsbEnum.pPort);
}
/******************************************************************************
 End of function  enumDeviceConfirmPortStatus
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceEnablePort
 Description:   Function to handle the ENUM_DEVICE_ENABLE_PORT_X state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceEnablePort (void)
{
    if (!enumUpdatePortStatus())
    {
        TRACE(("enumDeviceEnablePort: Failed to get port status\r\n"));
        /* This represents an error caused by a hub. This should not stop other
         ports from operating normally so check the next port for state
         change */
        gUsbEnum.pPort = gUsbEnum.pPort->pNext;
        gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
    }
    /* Make sure that the port has something attached before enabling the
     port */
    if (gUsbEnum.pPort->dwPortStatus & USBH_HUB_PORT_CONNECT_STATUS)
    {
        enumEnablePort(gUsbEnum.pPort, true);
        gUsbEnum.dwDelayCount = ENUM_PORT_ENABLE_DELAY_COUNT;
    }
    else
    {
        /* The device has been removed so check the next port */
        gUsbEnum.pPort = gUsbEnum.pPort->pNext;
        gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
    }
}
/******************************************************************************
 End of function  enumDeviceEnablePort
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceSleep
 Description:   Function to handle the ENUM_DEVICE_SLEEP_X state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceSleep (void)
{
    if (gUsbEnum.dwDelayCount)
    {
        gUsbEnum.dwDelayCount--;
    }
    else
    {
        gUsbEnum.currentState++;
    }
}
/******************************************************************************
 End of function  enumDeviceSleep
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceSpeed
 Description:   Function to handle the ENUM_DEVICE_SPEED state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceSpeed (void)
{
    enumGetPortStatus(gUsbEnum.pPort);
}
/******************************************************************************
 End of function  enumDeviceSpeed
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceRequestDescriptor8
 Description:   Function to handle the ENUM_DEVICE_REQUEST_DESCRIPTOR_8 state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceRequestDescriptor8 (void)
{
    if (!enumUpdatePortStatus())
    {
        TRACE(("enumDeviceRequestDescriptor8: Failed to get port status\r\n"));
        /* This represents an error caused by a hub. This should not stop other
         ports from operating normally so check the next port for state
         change */
        gUsbEnum.pPort = gUsbEnum.pPort->pNext;
        gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
    }
    /* Default is Full speed */
    gUsbEnum.transferSpeed = USBH_FULL;
    /* Check the device speed flag - only one should be set */
    if (gUsbEnum.pPort->dwPortStatus & USBH_HUB_HIGH_SPEED_DEVICE)
    {
        gUsbEnum.transferSpeed = USBH_HIGH;
    }
    if (gUsbEnum.pPort->dwPortStatus & USBH_HUB_LOW_SPEED_DEVICE)
    {
        gUsbEnum.transferSpeed = USBH_SLOW;
    } TRACE(("enumDeviceRequestDescriptor8: Port %d Try %d, Speed %s\r\n",
                    gUsbEnum.pPort->uiPortIndex,
                    ((ENUM_RETRY_COUNT) - gUsbEnum.iTryCount),
                    pszDeviceSpeedString[gUsbEnum.transferSpeed]));
    /* Create a device for enumeration */
    gUsbEnum.pDevice = usbhCreateEnumerationDeviceInformation(gUsbEnum.transferSpeed);
    if (gUsbEnum.pDevice)
    {
        /* Attach the device to the port */
        gUsbEnum.pPort->pDevice = gUsbEnum.pDevice;
        /* Attach the port to the device */
        gUsbEnum.pDevice->pPort = gUsbEnum.pPort;
        /* Get the first 8 bytes of the device descriptor */
        enumDeviceRequest(gUsbEnum.pDevice,
        USB_DEVICE_TO_HOST,
        USB_REQUEST_GET_DESCRIPTOR, USB_SET_VALUE(USB_DEVICE_DESCRIPTOR_TYPE, 0), (uint16_t) 0, (uint16_t) 8,
                gUsbEnum.pbyDescriptor);
    }
    else
    {
        TRACE(("enumDeviceRequestDescriptor8:"
                        " Failed to create device information\r\n"));
        gUsbEnum.currentState = ENUM_DEVICE_RETRY_LOOP;
    }
}
/******************************************************************************
 End of function  enumDeviceRequestDescriptor8
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceRequestDescriptorAll
 Description:   Function to handle the ENUM_DEVICE_REQUEST_DESCRIPTOR_ALL state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceRequestDescriptorAll (void)
{
    TRACE(("enumDeviceRequestDescriptorAll: Endpoint 0 packet size %d \r\n",
                    (uint16_t)((PUSBDD)gUsbEnum.pbyDescriptor)->bMaxPacketSize0));
    /* Set the control pipe packet size */
    usbhSetEp0PacketSize(gUsbEnum.pDevice, (uint16_t) ((PUSBDD) gUsbEnum.pbyDescriptor)->bMaxPacketSize0);
    /* Get the complete device descriptor */
    enumDeviceRequest(gUsbEnum.pDevice,
    USB_DEVICE_TO_HOST,
    USB_REQUEST_GET_DESCRIPTOR, USB_SET_VALUE(USB_DEVICE_DESCRIPTOR_TYPE, 0), (uint16_t) 0,
            (uint16_t) ((PUSBDD) gUsbEnum.pbyDescriptor)->bLength, gUsbEnum.pbyDescriptor);
}
/******************************************************************************
 End of function  enumDeviceRequestDescriptorAll
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceSetAddress
 Description:   Function to handle the EMUM_DEVICE_SET_ADDRESS state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceSetAddress (void)
{
    if (gUsbEnum.stDeviceDescriptorLength < sizeof(USBDD))
    {
        TRACE(("enumDeviceSetAddress: Device descriptor too short (%d)\r\n",
                        gUsbEnum.stLengthTransferred));
        gUsbEnum.currentState = ENUM_DEVICE_RETRY_LOOP;
    }
    else
    {
        /* Get an address for this device */
        int8_t chAddress = usbhGetDeviceAddress(gUsbEnum.pPort->pUsbHc);
#ifdef _TRACE_ON_
        TRACE(("Device Descriptor:\r\n"));
        dbgPrintBuffer(gUsbEnum.pbyDescriptor, gUsbEnum.stDeviceDescriptorLength);
        TRACE(("\r\n"));
#endif
        if (chAddress > 0)
        {
            /* Destroy device information used for getting the device
             descriptor */
            usbhDestroyDeviceInformation(gUsbEnum.pDevice);
            /* Create the device information based on the device
             descriptor */
            gUsbEnum.pDevice = usbhCreateDeviceInformation((PUSBDD) gUsbEnum.pbyDescriptor, gUsbEnum.transferSpeed);
            if (gUsbEnum.pDevice)
            {
                /* Attach the port to the device */
                gUsbEnum.pDevice->pPort = gUsbEnum.pPort;
                /* Keep the manufacuter string index for later*/
                gUsbEnum.bManufacturer = ((PUSBDD) gUsbEnum.pbyDescriptor)->bManufacturer;
                gUsbEnum.bProduct = ((PUSBDD) gUsbEnum.pbyDescriptor)->bProduct;
                gUsbEnum.bSerialNumber = ((PUSBDD) gUsbEnum.pbyDescriptor)->bSerialNumber;
                TRACE(("enumDeviceSetAddress: Setting address %d\r\n", chAddress));
                /* Set the device address */
                enumDeviceRequest(gUsbEnum.pDevice,
                USB_HOST_TO_DEVICE,
                USB_REQUEST_SET_ADDRESS, USB_SET_VALUE(0, chAddress), (uint16_t) 0, (uint16_t) 0, (uint8_t *) 0);
                /* Remember the address for the next request */
                gUsbEnum.chAddress = chAddress;
            }
            else
            {
                TRACE(("enumDeviceSetAddress: Failed to create device information\r\n"));
                gUsbEnum.currentState = ENUM_DEVICE_RETRY_LOOP;
            }
        }
        else
        {
            TRACE(("enumDeviceSetAddress: Out of device addresses\r\n"));
            gUsbEnum.currentState = ENUM_DEVICE_RETRY_LOOP;
        }
    }
}
/******************************************************************************
 End of function  enumDeviceSetAddress
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceRequestConfiguration
 Description:   Function to handle the ENUM_DEVICE_SET_CONFIGURATION state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceRequestConfiguration (void)
{
    TRACE(("enumDeviceRequestConfiguration:\r\n"));
    /* The device address changes after the status phase of the set address
     request */
    gUsbEnum.pDevice->byAddress = (uint8_t) gUsbEnum.chAddress;
    /* Get the configuration descriptor */
    enumDeviceRequest(gUsbEnum.pDevice,
    USB_DEVICE_TO_HOST,
    USB_REQUEST_GET_DESCRIPTOR, USB_SET_VALUE(USB_CONFIGURATION_DESCRIPTOR_TYPE, 0), (uint16_t) 0,
            (uint16_t) ENUM_DESCRIPTOR_BUFFER_SIZE, gUsbEnum.pbyDescriptor);
}
/******************************************************************************
 End of function  enumDeviceRequestConfiguration
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceSetConfiguration
 Description:   Function to handle the  state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceSetConfiguration (void)
{
    /* Check the size of the response */
    if (gUsbEnum.stLengthTransferred < sizeof(USBCG))
    {
        TRACE(("enumDeviceSetConfiguration: Configuration descriptor too short\r\n"));
        gUsbEnum.currentState = ENUM_DEVICE_RETRY_LOOP;
    }
    else
    {
#ifdef _TRACE_ON_
        Trace("Config Descriptor:\r\n");
        dbgPrintBuffer(gUsbEnum.pbyDescriptor, gUsbEnum.stLengthTransferred);
        Trace("\r\n");
#endif
        /* Configure the device information */
        if (usbhConfigureDeviceInformation(gUsbEnum.pDevice, (PUSBCG) gUsbEnum.pbyDescriptor, (uint8_t) 0, (uint8_t) 0))
        {
            _Bool bfConfigureOK = true;
            if (gUsbEnum.pDevice->byInterfaceClass == USB_DEVICE_CLASS_HUB)
            {
                PUSBPI pPort = gUsbEnum.pPort;
                int iTier = 0;
                /* Find the root port to which this hub is connected */
                while (pPort)
                {
                    if (pPort->pRoot)
                    {
                        break;
                    }
                    /* Check the port to which the hub is attached */
                    pPort = pPort->pHub->pPort;
                    iTier++;
                }

                /* Check the number of tiers of hubs supported */
                if (iTier < USBH_MAX_TIER)
                {
                    /* Set the hub driver - it has no function since it is
                     intrinsically supported */
                    gUsbEnum.pDeviceDriver = (PDEVICE) &g_usb_hub_driver;
                }
                else
                {
                    TRACE(("enumDeviceSetConfiguration: Only %d tier(s) of hub(s) supported\r\n",
                                    USBH_MAX_TIER));
                    gUsbEnum.pDeviceDriver = NULL;
                }
            }
            else
            {
                /* See if there is a class device driver available to drive
                 this device */
                if (!usbhLoadClassDriver(gUsbEnum.pDevice, gUsbEnum.pbyDescriptor, gUsbEnum.stLengthTransferred,
                        &gUsbEnum.pDeviceDriver))
                {
                    TRACE(("enumDeviceSetConfiguration: Device not supported\r\n"));
                    gUsbEnum.pDeviceDriver = NULL;
                }
            }
            /* Check to see if this device is on a port attached to a hub */
            if ((gUsbEnum.pPort->pHub)

            /* If the hub is bus powered?
             Note that hubs report that they are powered even if they are
             bus powered. So remove this line if a bus powered hub is to
             be supported. */
            && (gUsbEnum.pPort->pHub->bfBusPowered))
            {
                /* Calculate the current draw */
                int iCurrent = usbhCalculateCurrent(gUsbEnum.pPort->pHub);
                TRACE(("enumDeviceSetConfiguration: Current %dmA\r\n", iCurrent));
                if (iCurrent > 500)
                {
                    TRACE(("enumDeviceSetConfiguration: Insufficient current budget for device\r\n"));
                    bfConfigureOK = false;
                }
            }

            /* If there is sufficient current to power up the device then
             bfConfigureOK is set true */
            if (bfConfigureOK)
            {
                /* Set the configuration - at this point the device can
                 "power up" */
                TRACE(("enumDeviceSetConfiguration: %d\r\n", gUsbEnum.pDevice->byConfigurationValue));

                enumDeviceRequest(gUsbEnum.pDevice,
                USB_HOST_TO_DEVICE,
                USB_REQUEST_SET_CONFIGURATION, USB_SET_VALUE(0, gUsbEnum.pDevice->byConfigurationValue), (uint16_t) 0,
                        (uint16_t) 0, (uint8_t *) NULL);

                /* The device has been configured */
                gUsbEnum.pDevice->bfConfigured = true;

            }
            else
            {
                /* Don't set the configuration but continue to get the
                 strings */
                gUsbEnum.currentState = ENUM_DEVICE_REQUEST_MANUFACTURER_STRING;
            }
        }
        else
        {
            TRACE(("enumDeviceSetConfiguration: Failed to configure device information\r\n"));
            gUsbEnum.currentState = ENUM_DEVICE_RETRY_LOOP;
        }
    }
}
/******************************************************************************
 End of function  enumDeviceSetConfiguration
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceRequestManufacturerString
 Description:   Function to handle the ENUM_DEVICE_REQUEST_MANUFACTURER_STRING
 state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceRequestManufacturerString (void)
{
    /* Check for manufacturer string */
    if (gUsbEnum.bManufacturer)
    {
        /* Request the manufacturer string */
        TRACE(("enumDeviceRequestManufacturerString:\r\n"));
        enumDeviceRequest(gUsbEnum.pDevice,
        USB_DEVICE_TO_HOST,
        USB_REQUEST_GET_DESCRIPTOR, USB_SET_VALUE(USB_STRING_DESCRIPTOR_TYPE, gUsbEnum.bManufacturer), (uint16_t) 0,
                (uint16_t) ENUM_DESCRIPTOR_BUFFER_SIZE, gUsbEnum.pbyDescriptor);
    }
    else
    {
        /* Request the product string */
        gUsbEnum.currentState = ENUM_DEVICE_REQUEST_PRODUCT_STRING;
    }
}
/******************************************************************************
 End of function  enumDeviceRequestManufacturerString
 ******************************************************************************/

/******************************************************************************
 Function Name: enumCopyStringDescriptor
 Description:   Function to copy a string descriptor
 Arguments:     OUT pszString - Pointer to the destination string
 IN  stLength - The length of the destination string
 Return value:  none
 ******************************************************************************/
static void enumCopyStringDescriptor (int8_t *pszString, size_t stLength)
{
    /* Zero the string */
    memset(pszString, 0, stLength);
    if (gUsbEnum.stLengthTransferred)
    {
        size_t stDescriptorLength = (gUsbEnum.stLengthTransferred - 2) / 2;
        int8_t *l_pszString = pszString;
        int8_t *pwString = (int8_t*) &gUsbEnum.pbyDescriptor[2];
        /* Set the length */
        if (stDescriptorLength < stLength)
        {
            stLength = stDescriptorLength;
        }
        else
        {
            stLength--;
        }
        /* Copy the string to its destination */
        while (stLength--)
        {
            *l_pszString++ = *pwString++;
            pwString++;
        }
    }
}
/******************************************************************************
 End of function  enumCopyStringDescriptor
 ******************************************************************************/

/******************************************************************************
 Function Name: enumCopyManufacturerString
 Description:   Function to handle the ENUM_DEVICE_COPY_MANUFACTURER_STRING
 state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumCopyManufacturerString (void)
{
    /* Copy the manufacturer string to the device information structure */
    enumCopyStringDescriptor(gUsbEnum.pDevice->pszManufacturer,
    USBH_MAX_STRING_LENGTH);
    /* Now try to get the product string */
    gUsbEnum.currentState = ENUM_DEVICE_REQUEST_PRODUCT_STRING;
}
/******************************************************************************
 End of function  enumCopyManufacturerString
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceRequestProductString
 Description:   Function to handle the ENUM_DEVICE_REQUEST_PRODUCT_STRING state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceRequestProductString (void)
{
    /* The device has been configured */
    gUsbEnum.pDevice->bfConfigured = true;
    /* Check for manufacturer string */
    if (gUsbEnum.bProduct)
    {
        /* Request the product string */
        TRACE(("enumDeviceRequestProductString:\r\n"));
        enumDeviceRequest(gUsbEnum.pDevice,
        USB_DEVICE_TO_HOST,
        USB_REQUEST_GET_DESCRIPTOR, USB_SET_VALUE(USB_STRING_DESCRIPTOR_TYPE, gUsbEnum.bProduct), (uint16_t) 0,
                (uint16_t) ENUM_DESCRIPTOR_BUFFER_SIZE, gUsbEnum.pbyDescriptor);
    }
    else
    {
        /* Request the serial number string */
        gUsbEnum.currentState = ENUM_DEVICE_REQUEST_SERIAL_NUMBER_STRING;
    }
}
/******************************************************************************
 End of function  enumDeviceRequestProductString
 ******************************************************************************/

/******************************************************************************
 Function Name: enumCopyProductString
 Description:   Function to handle the ENUM_DEVICE_COPY_PRODUCT_STRING state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumCopyProductString (void)
{
    /* Copy the product string to the device information structure */
    enumCopyStringDescriptor(gUsbEnum.pDevice->pszProduct,
    USBH_MAX_STRING_LENGTH);
    /* Now try to get the product string */
    gUsbEnum.currentState = ENUM_DEVICE_REQUEST_SERIAL_NUMBER_STRING;
}
/******************************************************************************
 End of function  enumCopyProductString
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceRequestSerialNumberString
 Description:   Function to handle the ENUM_DEVICE_REQUEST_SERIAL_NUMBER_STRING
 state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceRequestSerialNumberString (void)
{
    /* The device has been configured */
    gUsbEnum.pDevice->bfConfigured = true;
    /* Check for manufacturer string */
    if (gUsbEnum.bSerialNumber)
    {
        /* Request the serial number string */
        TRACE(("enumDeviceRequestSerialNumberString:\r\n"));
        enumDeviceRequest(gUsbEnum.pDevice,
        USB_DEVICE_TO_HOST,
        USB_REQUEST_GET_DESCRIPTOR, USB_SET_VALUE(USB_STRING_DESCRIPTOR_TYPE, gUsbEnum.bSerialNumber), (uint16_t) 0,
                (uint16_t) ENUM_DESCRIPTOR_BUFFER_SIZE, gUsbEnum.pbyDescriptor);
    }
    else
    {
        /* Complete the enumeration process */
        gUsbEnum.currentState = ENUM_DEVICE_COMPLETE;
    }
}
/******************************************************************************
 End of function  enumDeviceRequestSerialNumberString
 ******************************************************************************/

/******************************************************************************
 Function Name: enumCopySerialNumberString
 Description:   Function to handle the ENUM_DEVICE_COPY_SERIAL_NUMBER_STRING
 state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumCopySerialNumberString (void)
{
    /* Copy the serial number string to the device information structure */
    enumCopyStringDescriptor(gUsbEnum.pDevice->pszSerialNumber,
    USBH_MAX_STRING_LENGTH);
    /* Complete the enumeration process */
    gUsbEnum.currentState = ENUM_DEVICE_COMPLETE;
}
/******************************************************************************
 End of function  enumCopySerialNumberString
 ******************************************************************************/

/******************************************************************************
 Function Name: enumDeviceComplete
 Description:   Function to handle the ENUM_DEVICE_COMPLETE state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumDeviceComplete (void)
{
    PUSBDI pDevice = gUsbEnum.pDevice;
    static uint32_t uiDeviceID = 0;
    /* Fill out the device information */
    strcpy(gUsbEnum.Information.pszDeviceLinkName, (const char *) pDevice->pszSymbolicLinkName);
    strcpy(gUsbEnum.Information.pszManufacturer, (const char *) pDevice->pszManufacturer);
    strcpy(gUsbEnum.Information.pszProduct, (const char *) pDevice->pszProduct);
    strcpy(gUsbEnum.Information.pszSerialNumber, (const char *) pDevice->pszSerialNumber);
    gUsbEnum.Information.wVID = pDevice->wVID;
    gUsbEnum.Information.wPID = pDevice->wPID;
    gUsbEnum.Information.wbcdVersion = pDevice->wDeviceVersion;
    gUsbEnum.Information.byDeviceClass = pDevice->byInterfaceClass;
    gUsbEnum.Information.byDeviceSubClass = pDevice->byInterfaceSubClass;
    gUsbEnum.Information.byProtocolCode = pDevice->byInterfaceProtocol;

    /* Create a link name for this device */
    sprintf((char *) pDevice->pszSymbolicLinkName, "USB.VID0x%.4X.PID0x%.4X.I%lu", pDevice->wVID, pDevice->wPID, uiDeviceID++);

    /* link with a the device driver located or a NULL pointer if not
     supported */
    if (!R_DEVLINK_DevAdd((char *)pDevice->pszSymbolicLinkName, (st_r_driver_t *)gUsbEnum.pDeviceDriver, &gUsbEnum.Information))
    {
        TRACE(("enumDeviceComplete: Failed to add device\r\n"));
    }
#ifdef _TRACE_ON_
//    devList(stdout);
#endif
    TRACE(("enumDeviceComplete: Device [%s]\r\n", pDevice->pszSymbolicLinkName));
    if ((st_r_driver_t *)gUsbEnum.pDeviceDriver != &g_usb_hub_driver)
    {
        /* Temporary device is now finished with */
        gUsbEnum.pDevice = NULL;
        /* Set the device port pointer */
        pDevice->pPort = gUsbEnum.pPort;
        /* Set the port device pointer */
        gUsbEnum.pPort->pDevice = pDevice;
        /* Check next port */
        gUsbEnum.pPort = gUsbEnum.pPort->pNext;
        gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
    }
    else
    {
        /* The device attached is a hub, get the hub descriptor so it can be
         configured */
        TRACE(("enumDeviceComplete: Get hub descriptor\r\n"));
        enumDeviceRequest(gUsbEnum.pPort->pDevice,
        USB_GET_HUB_DESCRIPTOR,
        USB_REQUEST_GET_DESCRIPTOR, USB_SET_VALUE(USB_HUB_DESCRIPTOR_TYPE, 0), (uint16_t) 0,
                (uint16_t) ENUM_DESCRIPTOR_BUFFER_SIZE, gUsbEnum.pbyDescriptor);
    }
}
/******************************************************************************
 End of function  enumDeviceComplete
 ******************************************************************************/

/******************************************************************************
 Function Name: enumConfigureHub
 Description:   Function to handle the ENUM_CONFIGURE_HUB state
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void enumConfigureHub (void)
{
    PUSBDI pDevice = gUsbEnum.pDevice;
    /* Set the device port pointer */
    pDevice->pPort = gUsbEnum.pPort;
    /* Set the port device pointer */
    gUsbEnum.pPort->pDevice = pDevice;
    /* Check the size of the response */
    if (gUsbEnum.stLengthTransferred >= (sizeof(USBRH) - 62))
    {
        if (usbhCreateHubInformation(pDevice, (PUSBRH) gUsbEnum.pbyDescriptor))
        {
            TRACE(("enumConfigureHub: OK\r\n"));
        }
        else
        {
            TRACE(("enumConfigureHub: Failed\r\n"));
        }
    }
    /* Temporary device is now finished with */
    gUsbEnum.pDevice = NULL;
    /* Check next port */
    gUsbEnum.pPort = gUsbEnum.pPort->pNext;
    gUsbEnum.currentState = ENUM_PORT_STATE_CHANGE;
}
/******************************************************************************
 End of function  enumConfigureHub
 ******************************************************************************/

static void (* const gpStateFunction[ENUM_NUM_FUNCTIONS]) (void) =
{
    /* ENUM_IDLE */
    enumIdle,
    /* ENUM_PORT_POLL_DELAY */
    enumPortPollDelay,
    /* ENUM_PORT_STATE_CHANGE */
    enumPortStateChange,
    /* ENUM_CHECK_PORT_STATUS */
    enumCheckPortStatus,
    /* ENUM_DEVICE_RETRY_LOOP */
    enumDeviceRetryLoop,
    /* ENUM_DEVICE_SET_RESET_PORT_1 */
    enumDeviceSetResetPort,
    /* ENUM_DEVICE_RESET_HOLD_1 */
    enumDeviceSleep,
    /* ENUM_DEVICE_CLEAR_RESET_PORT_1 */
    enumDeviceClearResetPort,
    /* ENUM_DEVICE_CONFIRM_PORT_STATUS */
    enumDeviceConfirmPortStatus,
    /* ENUM_DEVICE_ENABLE_PORT_1 */
    enumDeviceEnablePort,
    /* ENUM_DEVICE_SLEEP_1 */
    enumDeviceSleep,
    /* ENUM_DEVICE_SPEED */
    enumDeviceSpeed,
    /* ENUM_DEVICE_REQUEST_DESCRIPTOR_8 */
    enumDeviceRequestDescriptor8,
    /* ENUM_DEVICE_SET_RESET_PORT_2 */
    enumDeviceSetResetPort,
    /* ENUM_DEVICE_RESET_HOLD_2 */
    enumDeviceSleep,
    /* ENUM_DEVICE_CLEAR_RESET_PORT_2 */
    enumDeviceClearResetPort,
    /* ENUM_DEVICE_ENABLE_PORT_2 */
    enumDeviceEnablePort,
    /* ENUM_DEVICE_SLEEP_2 */
    enumDeviceSleep,
    /* ENUM_DEVICE_REQUEST_DESCRIPTOR_ALL */
    enumDeviceRequestDescriptorAll,
    /* ENUM_DEVICE_SET_RESET_PORT_3 */
    enumDeviceSetResetPort_SetDescriptorLength,
    /* ENUM_DEVICE_RESET_HOLD_3 */
    enumDeviceSleep,
    /* ENUM_DEVICE_CLEAR_RESET_PORT_3 */
    enumDeviceClearResetPort,
    /* ENUM_DEVICE_ENABLE_PORT_3 */
    enumDeviceEnablePort,
    /* ENUM_DEVICE_SLEEP_3 */
    enumDeviceSleep,
    /* EMUM_DEVICE_SET_ADDRESS */
    enumDeviceSetAddress,
    /* ENUM_DEVICE_REQUEST_CONFIGURATION */
    enumDeviceRequestConfiguration,
    /* ENUM_DEVICE_SET_CONFIGURATION */
    enumDeviceSetConfiguration,
    /* ENUM_DEVICE_REQUEST_MANUFACTURER_STRING */
    enumDeviceRequestManufacturerString,
    /* ENUM_DEVICE_COPY_MANUFACTURER_STRING */
    enumCopyManufacturerString,
    /* ENUM_DEVICE_REQUEST_PRODUCT_STRING */
    enumDeviceRequestProductString,
    /* ENUM_DEVICE_COPY_PRODUCT_STRING */
    enumCopyProductString,
    /* ENUM_DEVICE_REQUEST_SERIAL_NUMBER_STRING */
    enumDeviceRequestSerialNumberString,
    /* ENUM_DEVICE_COPY_SERIAL_NUMBER_STRING */
    enumCopySerialNumberString,
    /* ENUM_DEVICE_COMPLETE */
    enumDeviceComplete,
    /* ENUM_CONFIGURE_HUB */
    enumConfigureHub,
    /* ENUM_HUB_STATE_CHANGE */
    enumHubStateChange,
    /* ENUM_CHECK_HUB_STATUS */
    enumCheckHubStatus
};

/******************************************************************************
 Function Name: enumProcessStateFunction
 Description:   Function to call the appropriate state funcion
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumProcessStateFunction (void)
{
    static uint8_t old_state = 0;

    if(old_state != gUsbEnum.currentState)
    {
        old_state = gUsbEnum.currentState;
//        printf("Process State [%s]\r\n", state_str[gUsbEnum.currentState]);
    }

    /* Call the next state function */
    gpStateFunction[gUsbEnum.currentState]();
}
/******************************************************************************
 End of function  enumProcessStateFunction
 ******************************************************************************/

/******************************************************************************
 Port control functions
 ******************************************************************************/

/******************************************************************************
 Function Name: enumSetCompleteRequest
 Description:   Function to make it look like a dtandard device request has been
 completed
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static void enumSetCompleteRequest (void)
{
    gUsbEnum.setupRequest.errorCode = USBH_NO_ERROR;
    gUsbEnum.dataRequest.errorCode = USBH_NO_ERROR;
    gUsbEnum.statusRequest.errorCode = USBH_NO_ERROR;

    R_OS_SetEvent(&gUsbEnum.statusRequest.ioSignal);

    gUsbEnum.reqStatus = DEVREQ_STATUS;
}
/******************************************************************************
 End of function  enumSetCompleteRequest
 ******************************************************************************/

/******************************************************************************
 Function Name: enumPowerPort
 Description:   Function to apply power to the port
 Arguments:     IN  pPort - Pointer to the port to reset
 IN  bfState - true to apply power to the port
 Return value:
 ******************************************************************************/
static void enumPowerPort (PUSBPI pPort, _Bool bfState)
{
    /* Check to see if this is a root port */
    if (pPort->pRoot)
    {
        pPort->pRoot->Power(bfState);
        /* Function was handled by control function on a root
         port so set the state of the device request and error */
        enumSetCompleteRequest();
        if (bfState)
        {
            /* Allow the power to settle before continuing */
            gUsbEnum.dwDelayCount = ENUM_PORT_POWER_DELAY_COUNT;
        }
        else
        {
            gUsbEnum.dwDelayCount = 0UL;
        }
    }
    else if (pPort->pHub)
    {
        TRACE(("enumPowerPort: %u %d\r\n", pPort->uiPortIndex, bfState));
        /* Check the power state */
        if (bfState)
        {
            enumDeviceRequest(pPort->pHub->pPort->pDevice,
            USB_SET_PORT_FEATURE,
            USB_REQUEST_SET_FEATURE, USB_SET_VALUE(0, USB_HUB_PORT_POWER), (uint16_t) pPort->uiPortIndex, (uint16_t) 0,
            NULL);
            /* Allow the power to settle before continuing */
            gUsbEnum.dwDelayCount = (uint32_t) pPort->pHub->wPowerOn2PowerGood_mS;
        }
        else
        {
            enumDeviceRequest(pPort->pHub->pPort->pDevice,
            USB_SET_PORT_FEATURE,
            USB_REQUEST_CLEAR_FEATURE, USB_SET_VALUE(0, USB_HUB_PORT_POWER), (uint16_t) pPort->uiPortIndex,
                    (uint16_t) 0,
                    NULL);
            gUsbEnum.dwDelayCount = 0UL;
        }
    }
}
/******************************************************************************
 End of function  enumPowerPort
 ******************************************************************************/

/******************************************************************************
 Function Name: enumResetPort
 Description:   Function to reset a port
 Arguments:     IN  pPort - Pointer to the port to reset
 IN  bfState - true to reset the port
 Return value:  none
 ******************************************************************************/
static void enumResetPort (PUSBPI pPort, _Bool bfState)
{
    /* Check to see if this is a root port */
    if (pPort->pRoot)
    {
        pPort->pRoot->Reset(bfState);

        /* Function was handled by control function on a root
         port so set the state of the device request and error */
        enumSetCompleteRequest();

        /* Hold the port in reset to allow speed detection and propagation
         through a hub */
        if (bfState)
        {
            gUsbEnum.dwDelayCount = ENUM_ROOT_PORT_RESET_COUNT;
        }
        else
        {
            gUsbEnum.dwDelayCount = 0UL;
        }
    }
    else if (pPort->pHub)
    {
        /* Check the reset state */
        if (bfState)
        {
            TRACE(("enumResetPort: %u %d\r\n", pPort->uiPortIndex, bfState));
            enumDeviceRequest(pPort->pHub->pPort->pDevice,
            USB_SET_PORT_FEATURE,
            USB_REQUEST_SET_FEATURE, USB_SET_VALUE(0, USB_PORT_RESET), (uint16_t) pPort->uiPortIndex, (uint16_t) 0,
            NULL);
            gUsbEnum.dwDelayCount = ENUM_HUB_PORT_RESET_COUNT;
        }
        else
        {
            /* The hub clears the reset automatically at the end of the
             procedure. Get the port status so the ENUM_DEVICE_ENABLE_PORT_X
             state function can check that the device is still attached */
            enumDeviceRequest(pPort->pHub->pPort->pDevice,
            USB_GET_PORT_STATUS,
            USB_REQUEST_GET_STATUS, 0, (uint16_t) pPort->uiPortIndex, (uint16_t) sizeof(uint32_t),
                    (uint8_t *) &gUsbEnum.dwPortStatus);
        }
    }
}
/******************************************************************************
 End of function  enumResetPort
 ******************************************************************************/

/******************************************************************************
 Function Name: enumEnablePort
 Description:   Function to enable / disable a port
 Arguments:     IN  pPort - Pointer to the port to enable / disable
 IN  bfState - true to enable
 Return value:  none
 ******************************************************************************/
static void enumEnablePort (PUSBPI pPort, _Bool bfState)
{
    /* Check to see if this is a root port */
    if (pPort->pRoot)
    {
        pPort->pRoot->Enable(bfState);
    }
    enumSetCompleteRequest();
}
/******************************************************************************
 End of function  enumEnablePort
 ******************************************************************************/
#if 0
/******************************************************************************
 Function Name: enumSuspendPort - not used and therefore not tested
 Description:   Function to issue suspend and resume signaling on a port
 Arguments:     IN  pPort - Pointer to the port to suspend
 IN  bfState - true to suspend false to resume
 Return value:  none
 ******************************************************************************/
static void enumSuspendPort (PUSBPI pPort, _Bool bfState)
{
    /* Check to see if this is a root port */
    if (pPort->pRoot)
    {
        pPort->pRoot->Suspend(bfState);
        /* Function was handled by control function on a root
         port so set the state of the device request and error */
        enumSetCompleteRequest();
    }
    else
    {
        TRACE(("enumSuspendPort: %u\r\n", pPort->uiPortIndex));
        /* Check the suspend state */
        if (bfState)
        {
            enumDeviceRequest(pPort->pHub->pPort->pDevice,
            USB_SET_PORT_FEATURE,
            USB_REQUEST_SET_FEATURE, USB_SET_VALUE(0, USB_PORT_SUSPEND), (uint16_t) pPort->uiPortIndex, (uint16_t) 0,
            NULL);
            gUsbEnum.dwDelayCount = ENUM_HUB_PORT_RESET_COUNT;
        }
        else
        {
            /* Get the status of the port */
            enumDeviceRequest(pPort->pHub->pPort->pDevice,
            USB_GET_PORT_STATUS,
            USB_REQUEST_GET_STATUS, 0, (uint16_t) pPort->uiPortIndex, (uint16_t) sizeof(uint32_t),
                    (uint8_t *) &gUsbEnum.dwPortStatus);
        }
    }
}
/******************************************************************************
 End of function  enumSuspendPort
 ******************************************************************************/
#endif
/******************************************************************************
 Function Name: enumGetPortStatus
 Description:   Function to find out if a device is attached to a port
 Arguments:     IN  pPort - Pointer to the port to check
 Return value:  returns the device transfer speed
 ******************************************************************************/
static void enumGetPortStatus (PUSBPI pPort)
{
    /* Check to see if this is a root port */
    if (pPort->pRoot)
    {
        /* Call the function to get the status of the port */
        pPort->dwPortStatus = pPort->pRoot->GetStatus();
        /* Function was handled by control function on a root
         port so set the state of the device request and error */
        enumSetCompleteRequest();
    }
    else if (pPort->pHub)
    {
        /* Send a device request to the hub to get the status of the port */
        enumDeviceRequest(pPort->pHub->pPort->pDevice,
        USB_GET_PORT_STATUS,
        USB_REQUEST_GET_STATUS, 0, (uint16_t) pPort->uiPortIndex, (uint16_t) sizeof(uint32_t),
                (uint8_t *) &gUsbEnum.dwPortStatus);
    }
}
/******************************************************************************
 End of function  enumGetPortStatus
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
