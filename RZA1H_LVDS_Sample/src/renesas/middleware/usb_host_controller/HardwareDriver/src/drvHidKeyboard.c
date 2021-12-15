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
 * Copyright (C) 2015 Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************
 * File Name    : drvHidKeyboard.c
 * Version      : 1.00
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : Device driver for HID class keyboards. This driver uses the
 *                keyboard in its boot mode for simplicity.
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 03.06.2015 1.00 First Release
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
#include <fcntl.h>
#include <errno.h>

#include "iodefine_cfg.h"

#include "compiler_settings.h"
#include "control.h"
#include "ddusbh.h"
#include "usbhDeviceApi.h"
#include "r_event.h"
#include "r_cbuffer.h"

#include "r_task_priority.h"
#include "drvStdIn.h"
#include "trace.h"

#include "r_led_drv_api.h"

/******************************************************************************
 Macro Definitions
 ******************************************************************************/

#define HID_KBD_CHAR_BUFFER_SIZE    64
#define HID_REPORT_PACKET_SIZE      8
#define HID_DESCRIPTOR_TIME_OUT     16

/* The initial delay count for auto repeat */
#define HID_KBD_AUTO_REPEAT_INIT    50

/* The repeat delay count for auto repeat */
#define HID_KBD_AUTO_REPEAT         10

/* The modifier bit map */
#define HID_KEY_LEFT_CTRL           BIT_0
#define HID_KEY_LEFT_SHIFT          BIT_1
#define HID_KEY_LEFT_ALT            BIT_2
#define HID_KEY_LEFT_GUI            BIT_3
#define HID_KEY_RIGHT_CTRL          BIT_4
#define HID_KEY_RIGHT_SHIFT         BIT_5
#define HID_KEY_RIGHT_ALT           BIT_6
#define HID_KEY_RIGHT_GUI           BIT_7

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
    KBD_IDLE = 0, KBD_WAIT_REPORT
} KBDST;

typedef enum
{
    KBD_AUTO_REPEAT_INITIAL_DELAY = 0, KBD_AUTO_REPEAT_DELAY
} KBDRPT;

#pragma pack(1)

typedef struct
{
    /* The country code */
    uint8_t byCountryCode;

    /* String for identification */
    int8_t *pszCountryName;

    /* Two maps one for SHIFT and one normal */
    struct
    {
        const int8_t *pChar;
        size_t       stNumEntries;
    } Standard, Shifted;
} KEYMAP, *PKEYMAP;

typedef struct _HIDKBD
{
    /* The buffer used to hold the characters */
    PCBUFF   pKeyBuffer;

    /* The transfer request structures */
    USBTR    readRequest;
    USBTR    deviceRequest;
    _Bool   bfDeviceRequestTimeOut;
    uint16_t wLengthTransferred;
    PEVENT   ppEventList[1];

    /* Pointer to the device and endpoints */
    PUSBDI   pDevice;
    PUSBEI   pInEndpoint;

    /* The last error code */
    KBDERR   errorCode;

    /* Force the alignment to four byte boundary for data transfer */
    uint32_t dwAlign;

    /* The report descriptor buffer */
    uint8_t  pbyReportDescriptor[64];

    /* Pointer to the keyboard mapping */
    PKEYMAP  pKeyMap;

    /* Task for polling the keyboard */
    os_task_t *uiTaskID;

    /* Pointer to an event to signal new key event */
    event_t   keyEvent;

    /* The state of the keyboard */
    KBDST    kbdState;

    /* The current key that is pressed - unlike a music keyboard! */
    uint8_t  byKeyDown;

    /* The current key modifier */
    uint8_t  byModifier;

    /* A bit map of the keys that are pressed */
    uint8_t  pbyKeyMap[32];

    /* A flag to track the state of the caps lock key */
    bool_t    bfCapsLock;

    /* A flag to track the state of the num lock key */
    bool_t    bfNumLock;

    /* The auto repeat key state */
    KBDRPT   rptState;

    /* The auto repeat counter */
    int      iRepeatCount;

} HIDKBD, *PHIDKBD;
#pragma pack()

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Function Prototypes
 ******************************************************************************/
static int_t kbdOpen (st_stream_ptr_t pStream);
static void kbdClose (st_stream_ptr_t pStream);
static int_t kbdRead (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int_t kbdControl (st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct);

static PUSBEI kbdGetFirstEndpoint (PUSBDI pDevice, USBDIR transferDirection);
static bool_t kbdSetInterval (PHIDKBD pHidKbd, uint8_t byInterval);
static os_task_code_t kbd_poll_task (void *param);

/******************************************************************************
 Constant Data
 ******************************************************************************/

/* Define the driver function table for this device */
const st_r_driver_t gHidKeyboardDriver =
{
    "HID Keyboard Device Driver",
    kbdOpen,
    kbdClose,
    kbdRead,
    no_dev_io,
    kbdControl,
    no_dev_get_version
};

/* The first 4 entries are return error codes that are not printed 
 for more information see section 10 Keyboard/Keypad Page (0x07)
 in Hut1_12.pdf */
static const int8_t gpKeyMapUS[] =
{ "\0\0\0\0abcdefghijklmnopqrstuvwxyz1234567890\r\b\b\t -=[]\\#;?',./" };
static const int8_t gpKeyMapShiftUS[] =
{ "\0\0\0\0ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()\r\b\b\t _+{}|~:?~<>?" };
static const int8_t gpKeyMapUK[] =
{ "\0\0\0\0abcdefghijklmnopqrstuvwxyz1234567890\r\b\b\t -=[]\\#;'',./" };

static const int8_t gpKeyMapShiftUK[] =
{

/* GNU Compiler modification - below the '\xA3' string entry is used to create the
 * GB Pound symbol. If the ASCII character £ is used the compiler inserts 0xC2, 0xA3
 * which skews the SIZEOF and indexing;*/
/* expected string:
   "\0\0\0\0ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"£$%^&*()\r\b\b\t _+{}|~:@~<>?"
   Modified string: */
"\0\0\0\0ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"\xA3$%^&*()\r\b\b\t _+{}|~:@~<>?"

};

/* Define the key mapping for the supported countries */
KEYMAP gpCountryKeyMapping[] =
{
    /* 0 - Not supported - See section 6.2.1 of HID specification HID1_11.pdf */
    /* 1..31 - Arabic..Taiwan - Outside of sample code support requirement */
    /* 32 - UK */
    {
        32,
        (int8_t *) "UK",
        {
            gpKeyMapUK,
            (sizeof(gpKeyMapUK) - 1)
        },
        {
            gpKeyMapShiftUK,
            (sizeof(gpKeyMapShiftUK) - 1)
        },
    },

    /* 33 - US */
    {
        33,
        (int8_t *) "US",
        {
            gpKeyMapUS,
            sizeof(gpKeyMapUS)
        },
        {
            gpKeyMapShiftUS,
            sizeof(gpKeyMapShiftUS)
        },
    }

    /* 34..34 - Yuogoslavia..Turkish-F - Outside of sample code support */
    /* 36..255 - Reserved */
};

/******************************************************************************
 Private Functions
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdOpen
 Description:   Function to open the keyboard driver
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  0 for success otherwise Keyboard Error code (control.h)
 ******************************************************************************/
static int_t kbdOpen (st_stream_ptr_t pStream)
{
    /* Get a pointer to the device information from the host driver for this device */
    PUSBDI pDevice = usbhGetDevice((int8_t *) pStream->p_stream_name);

    if (pDevice)
    {
        /* This driver supports HID keyboards with IN and OUT endpoints
         although only the IN endpoint is used in this sample code */
        PUSBEI pIn = kbdGetFirstEndpoint(pDevice, USBH_IN);

        /* If the device has the an IN endpoint */
        if (pIn)
        {
            /* Create a buffer for the key presses */
            PCBUFF pKeyBuffer = cbCreate(HID_KBD_CHAR_BUFFER_SIZE);

            if (pKeyBuffer)
            {
                /* Allocate the memory for the driver */
                pStream->p_extension = R_OS_AllocMem(sizeof(HIDKBD),R_REGION_LARGE_CAPACITY_RAM);

                if (pStream->p_extension)
                {
                    PHIDKBD pHidKbd = pStream->p_extension;

                    /* Initialise the driver data */
                    memset(pHidKbd, 0, sizeof(HIDKBD));

                    /* Set the buffer pointer */
                    pHidKbd->pKeyBuffer = pKeyBuffer;

                    /* Set the device and endpoint information found */
                    pHidKbd->pDevice = pDevice;
                    pHidKbd->pInEndpoint = pIn;
                    /* Set a default key map */
#ifdef _US_KEYBOARD_
                    pHidKbd->pKeyMap = &gpCountryKeyMapping[1];
#else
                    pHidKbd->pKeyMap = &gpCountryKeyMapping[0];
#endif
                    /* Create the signals for the transfer requests */
                    R_OS_CreateEvent(&pHidKbd->readRequest.ioSignal);

                    /* Put the event pointer in the event list */
                    pHidKbd->ppEventList[0] = pHidKbd->readRequest.ioSignal;

                    R_OS_CreateEvent(&pHidKbd->deviceRequest.ioSignal);
                    R_OS_CreateEvent(&pHidKbd->keyEvent);

                    /* Start a task to poll the keyboard */
                    pHidKbd->uiTaskID = R_OS_CreateTask("HID Keyboard", (os_task_code_t) kbd_poll_task, pHidKbd,
                            R_OS_ABSTRACTION_PRV_SMALL_STACK_SIZE, TASK_HID_KEYBOARD_PRI);

                    /* Set the report interval in 4mS steps*/
                    kbdSetInterval(pHidKbd, 255);
                    TRACE(("kbdOpen: Opened device %s\r\n", pStream->p_stream_name));
                    return HID_KBD_OK;
                }
                else
                {
                    TRACE(("kbdOpen: Failed to allocate memory\r\n"));
                }
            }
            else
            {
                TRACE(("kbdOpen: Failed to create keyboard buffer\r\n"));
            }

            return HID_KBD_MEMORY_ALLOCATION_ERROR;
        }
        else
        {
            TRACE(("kbdOpen: Failed to find interrupt IN endpoint\r\n"));
            return HID_KBD_ENDPOINT_NOT_FOUND;
        }
    }
    else
    {
        TRACE(("kbdOpen: Failed to find device %s\r\n", pStream->p_stream_name));
    }

    return HID_KBD_DEVICE_NOT_FOUND;
}
/******************************************************************************
 End of function  kbdOpen
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdClose
 Description:   Function to close the bulk endpoint driver
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  none
 ******************************************************************************/
static void kbdClose (st_stream_ptr_t pStream)
{
    PHIDKBD pHidKbd = pStream->p_extension;
    TRACE(("kbdClose: Closed device %s\r\n", pStream->p_stream_name));

    /* If there is a request in progress */
    if (usbhTransferInProgress(&pHidKbd->readRequest))
    {
        /* Cancel it */
        usbhCancelTransfer(&pHidKbd->readRequest);
    }

    /* Stop the polling task */
    R_OS_DeleteTask(pHidKbd->uiTaskID);

    R_OS_DeleteEvent(&pHidKbd->readRequest.ioSignal);
    R_OS_DeleteEvent(&pHidKbd->deviceRequest.ioSignal);

    R_OS_DeleteEvent(&pHidKbd->keyEvent);

    /* Destroy the key buffer */
    cbDestroy( pHidKbd->pKeyBuffer);

    /* Free the driver extension */
    R_OS_FreeMem(pHidKbd);
}
/******************************************************************************
 End of function  kbdClose
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdRead
 Description:   Function to read from the bulk IN endpoint
 Arguments:     IN  pStream - Pointer to the file stream
                IN  pbyBuffer - Pointer to the destination memory
                IN  uiCount - The number of bytes to read
 Return value:  The number of bytes read or -1 on error
 ******************************************************************************/
static int_t kbdRead (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    PHIDKBD pHidKbd = pStream->p_extension;
    uint8_t *pbyEnd = pbyBuffer + uiCount;

    /* For the length of data */
    while (pbyBuffer < pbyEnd)
    {
        /* Get the data from the buffer */
        if (cbGet(pHidKbd->pKeyBuffer, pbyBuffer))
        {
            pbyBuffer++;
        }
        else
        {
            /* Wait here for a key-press */
            R_OS_WaitForEvent(&pHidKbd->keyEvent, portMAX_DELAY);
            R_OS_ResetEvent(&pHidKbd->keyEvent);
        }
    }

    /* Check to see if there has been an error */
    if (pHidKbd->errorCode)
    {
        /* Return an error code */
        pHidKbd->errorCode = HID_KBD_OK;

        /* NOTE: Actual error code from driver has been lost */
        return EBADF;
    }

    /* return the number of bytes read */
    return (int) uiCount;
}
/******************************************************************************
 End of function  kbdRead
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdControl
 Description:   Function to handle custom control functions for the kyboard
 Arguments:     IN  pStream - Pointer to the file stream
                IN  ctlCode - The custom control code
                IN  pCtlStruct - Pointer to the custom control structure
 Return value:  0 for success -1 on error
 ******************************************************************************/
static int_t kbdControl (st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct)
{
    PHIDKBD pHidKbd = pStream->p_extension;
    UNUSED_PARAM(pCtlStruct);

    if (CTL_GET_RX_BUFFER_COUNT == ctlCode)
    {
        return (int) cbUsed(pHidKbd->pKeyBuffer);
    }
    else if (CTL_GET_STDIN_EVENT == ctlCode)
    {
        PPEVENT ppEvent = (PPEVENT) pCtlStruct;
        *ppEvent = &pHidKbd->keyEvent;
        return 0;
    }
    else if (CTL_STREAM_TCP == ctlCode)
    {
        return 0;
    }
    else if (CTL_HID_KEYBOARD == ctlCode)
    {
        return 0;
    }
    return -1;
}
/******************************************************************************
 End of function  kbdControl
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdDeviceRequest
 Description:   Function to send a device request
 Arguments:     IN  pHidKbd - Pointer to the driver extension
                IN  bRequest - The request
                IN  bmRequestType - The request type
                IN  wValue - The Value
                IN  wIndex - The Index
                IN  wLength - The length of the data
                IN/OUT pbyData - Pointer to the data
 Return value:  0 for success or error code
 ******************************************************************************/
static int kbdDeviceRequest (PHIDKBD pHidKbd, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
        uint16_t wLength, uint8_t *pbyData)
{
    if (usbhDeviceRequest(&pHidKbd->deviceRequest, pHidKbd->pDevice, bmRequestType, bRequest, wValue, wIndex, wLength,
            pbyData))
    {
        pHidKbd->errorCode = HID_KBD_REQUEST_ERROR;
        return HID_KBD_REQUEST_ERROR;
    }

    return HID_KBD_OK;
}
/******************************************************************************
 End of function  kbdDeviceRequest
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdGetFirstEndpoint
 Description:   Function to get the first interrupt endpoint with matching transfer direction
 Arguments:     IN  pDevice - Pointer to the device information
                IN  transferDirection - The endpoint transfer direction
 Return value:  Pointer to the endpoint information or NULL if not found
 ******************************************************************************/
static PUSBEI kbdGetFirstEndpoint (PUSBDI pDevice, USBDIR transferDirection)
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
 End of function  kbdGetFirstEndpoint
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdStartTransfer
 Description:   Function to start an IN transfer
 Arguments:     IN  pHidKbd - Pointer to the driver extension
 Return value:  true if the transfer was started
 ******************************************************************************/
static bool_t kbdStartTransfer (PHIDKBD pHidKbd)
{
    bool_t bfResult;

    bfResult = usbhStartTransfer(pHidKbd->pDevice, &pHidKbd->readRequest, pHidKbd->pInEndpoint,
            pHidKbd->pbyReportDescriptor, (size_t) pHidKbd->pInEndpoint->wPacketSize,
            REQ_IDLE_TIME_OUT_INFINITE);

    return bfResult;
}
/******************************************************************************
 End of function  kbdStartTransfer
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdSetInterval
 Description:   Function to set the report interval
 Arguments:     IN  pHidKbd - Pointer to the driver extension
                IN  byInterval - The report interval in 4mS steps
 Return value:  true if the interval is set
 ******************************************************************************/
static bool_t kbdSetInterval (PHIDKBD pHidKbd, uint8_t byInterval)
{
    if (kbdDeviceRequest(pHidKbd, (uint8_t) (0x20 | USB_CLASS_REQUEST ),
            USBHID_REQUEST_SET_IDLE, USB_SET_VALUE(byInterval, 0), 0, 0, NULL) == HID_KBD_OK)
    {
        return true;
    }

    return false;
}
/******************************************************************************
 End of function  kbdSetInterval
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdUpdateKeyMap
 Description:   Function to pars the report and update the key map
 Arguments:     IN  pHidKbd - Pointer to the driver extension
                IN  pbyReport - Pointer to the report
 Return value:  The most recent key that has been pressed
 ******************************************************************************/
static uint8_t kbdUpdateKeyMap (PHIDKBD pHidKbd, uint8_t *pbyReport)
{
    static const int pbyNoDiodes[] =
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
    /* First compare to the "The matrix will not allow me to figure out which
       key is pressed because the product is so low cost the design engineer
     forgot to put in the diodes" message */
    if (memcmp(&pbyReport[2], pbyNoDiodes, sizeof(pbyNoDiodes)) == 0)
    {
        return 0;
    }

    /* The keys are presented as codes in a 6 byte format where the most
       recent key can appear wherever the device puts it. This demonstrates
       the level of diligence put into the report data format by the HID
       working group. Well done chaps! However, the mess needs to be
       cleaned up */
    uint8_t pbyCurrent[32];
    int iIndex = HID_REPORT_PACKET_SIZE;
    uint8_t byResult = 0;

    /* Initialise the map - keys that are pressed or down are set to 1 */
    memset(pbyCurrent, 0, 32);

    /* Expand into a bit map */
    while (iIndex-- > 1)
    {
        if (pbyReport[iIndex])
        {
            uint8_t byNew;

            /* Calculate the byte in the bit map */
            int iByte = (pbyReport[iIndex] >> 3);

            /* Set the bit in the map from the code in the report data */
            pbyCurrent[iByte] |= (uint8_t) (1 << (pbyReport[iIndex] & 0x7));

            /* Spot new keys with an exclusive or with the last map and
             then mask to ignore key ups */
            byNew = (uint8_t) ((pbyCurrent[iByte] ^ pHidKbd->pbyKeyMap[iByte]) & pbyCurrent[iByte]);

            /* If there is a bit set in the byte then this indicates a new
             key press has been detected */
            if (byNew)
            {
                uint8_t byBit = BIT_0;
                int iBit = 0;

                /* Go through the bits in the byte */
                while (iBit < 8)
                {
                    /* Check to see which bit is set */
                    if (byBit & byNew)
                    {
                        /* Calculate the key code for the return */
                        byResult = (uint8_t) (iBit + (iByte << 3));
                    }

                    iBit++;
                    byBit = (uint8_t) (byBit << 1);
                }
            }
        }
    }

    /* Take a copy of the current map - so new keys can be spotted next time */
    memcpy(pHidKbd->pbyKeyMap, pbyCurrent, 32);
    return byResult;
}
/******************************************************************************
 End of function  kbdUpdateKeyMap
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdKeyIsDown
 Description:   Function to return true if the key is still reported as being
 pressed
 Arguments:     IN  pHidKbd - Pointer to the driver extension
 IN  pbyReport - Pointer to the report
 Return value:  true if the key is down or pressed
 ******************************************************************************/
static bool_t kbdKeyIsDown (PHIDKBD pHidKbd, uint8_t byKeyCode)
{
    int iByte = byKeyCode >> 3;
    uint8_t byBit = (uint8_t) (1 << (byKeyCode & 0x7));

    if (pHidKbd->pbyKeyMap[iByte] & byBit)
    {
        return true;
    }

    return false;
}
/******************************************************************************
 End of function  kbdKeyIsDown
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdGetKeyCode
 Description:   Function to translate the key code to an ASCII character
 Arguments:     IN  pHidKbd - Pointer to the driver extension
 Return value:  The ASCII code for the current key or zero for none
 ******************************************************************************/
static uint8_t kbdGetKeyCode (PHIDKBD pHidKbd)
{
    /* Check for the ctrl and alt modifier keys */
    if (pHidKbd->byModifier & (HID_KEY_LEFT_CTRL | HID_KEY_LEFT_ALT | HID_KEY_RIGHT_CTRL | HID_KEY_RIGHT_ALT))
    {
        /* TODO: return a control or alt character code if required */
        return 0;
    }

    /* Get the state of the caps lock key */
    bool_t bfShift = pHidKbd->bfCapsLock;

    /* Update with the state of the shift keys */
    if (pHidKbd->byModifier & (HID_KEY_RIGHT_SHIFT | HID_KEY_LEFT_SHIFT))
    {
        bfShift ^= true;
    }

    if (bfShift)
    {
        if (pHidKbd->byKeyDown < pHidKbd->pKeyMap->Shifted.stNumEntries)
        {
            return (uint8_t) pHidKbd->pKeyMap->Shifted.pChar[pHidKbd->byKeyDown];
        }
    }
    else
    {
        if (pHidKbd->byKeyDown < pHidKbd->pKeyMap->Standard.stNumEntries)
        {
            return (uint8_t) pHidKbd->pKeyMap->Standard.pChar[pHidKbd->byKeyDown];
        }
    }

    return 0;
}
/******************************************************************************
 End of function  kbdGetKeyCode
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdPutString
 Description:   Function to put a string in the key buffer
 Arguments:     IN  pKeyBuffer - Pointer to the key buffer
                IN  pszString - Pointer to the string
 Return value:  none
 ******************************************************************************/
static void kbdPutString (PCBUFF pKeyBuffer, char_t *pszString)
{
    while (*pszString)
    {
        int8_t ch = *pszString++;
        cbPut(pKeyBuffer, (uint8_t) ch);
    }
}
/******************************************************************************
 End of function  kbdPutString
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdPutKey
 Description:   Function to put the current key down into the output buffer
 Arguments:     IN  pHidKbd - Pointer to the driver extension
 Return value:  None
 ******************************************************************************/
static void kbdPutKey (PHIDKBD pHidKbd)
{
    PCBUFF pKeyBuffer = pHidKbd->pKeyBuffer;

    if (cbUsed(pKeyBuffer) > (HID_KBD_CHAR_BUFFER_SIZE - 4))
    {
        return;
    }

    /* From Hut1_12.pdf section 10 table 12 (page 53)
     TODO: change to desired function or mapping */
    switch (pHidKbd->byKeyDown)
    {
        case 0x00: /* No event */
        break;
        case 0x01: /* Roll over*/
            TRACE(("kbdPutKey: Rollover\r\n"));
        break;
        case 0x02: /* Fail */
            TRACE(("kbdPutKey: Key Fail\r\n"));
        break;
        case 0x03: /* Error */
            TRACE(("kbdPutKey: Key Error\r\n"));
        break;
        case 0x28: /* Enter */
            cbPut(pKeyBuffer, (uint8_t) '\r');
        break;
        case 0x29: /* ESC */
            kbdPutString(pKeyBuffer, "ESC");
        break;
        case 0x39: /* Caps lock */
            pHidKbd->bfCapsLock ^= true;
        break;
        case 0x3A: /* F1 */
            kbdPutString(pKeyBuffer, "\x1Bop\r");
        break;
        case 0x3B: /* F2 */
            kbdPutString(pKeyBuffer, "\x1Boq\r");
        break;
        case 0x3C: /* F3 */
            kbdPutString(pKeyBuffer, "\x1Bor\r");
        break;
        case 0x3D: /* F4 */
            kbdPutString(pKeyBuffer, "\x1Bos\r");
        break;
        case 0x3E: /* F5 */
            kbdPutString(pKeyBuffer, "\x1Bot\r");
        break;
        case 0x3F: /* F6 */
            kbdPutString(pKeyBuffer, "\x1Bou\r");
        break;
        case 0x40: /* F7 */
            kbdPutString(pKeyBuffer, "\x1Bov\r");
        break;
        case 0x41: /* F8 */
            kbdPutString(pKeyBuffer, "\x1Bow\r");
        break;
        case 0x42: /* F9 */
            kbdPutString(pKeyBuffer, "\x1Box\r");
        break;
        case 0x43: /* F10 */
            kbdPutString(pKeyBuffer, "\x1Boy\r");
        break;
        case 0x44: /* F11 */
            kbdPutString(pKeyBuffer, "\x1Boz\r");
        break;
        case 0x45: /* F12 */
            kbdPutString(pKeyBuffer, "\x1Boo\r");
        break;
        case 0x46: /* PrintScreen */
        break;
        case 0x48: /* Pause */
        break;
        case 0x49: /* Insert */
        break;
        case 0x4A: /* Home */
        break;
        case 0x4B: /* PageUp */
        break;
        case 0x4C: /* Delete */
            cbPut(pKeyBuffer, (uint8_t) '\b');
        break;
        case 0x4E: /* PageDown */
        break;
        case 0x4F: /* RightArrow1 */
        break;
        case 0x50: /* LeftArrow1 */
        break;
        case 0x51: /* DownArrow1 */
        break;
        case 0x52: /* UpArrow1 */
        break;
        case 0x53: /* Num Lock */
            pHidKbd->bfNumLock ^= true;
        break;
        case 0x54: /*  Keypad / */
            cbPut(pKeyBuffer, (uint8_t) '/');
        break;
        case 0x55: /* Keypad * */
            cbPut(pKeyBuffer, (uint8_t) '*');
        break;
        case 0x56: /* Keypad - */
            cbPut(pKeyBuffer, (uint8_t) '-');
        break;
        case 0x57: /* Keypad + */
            cbPut(pKeyBuffer, (uint8_t) '+');
        break;
        case 0x58: /* Keypad enter */
            cbPut(pKeyBuffer, (uint8_t) '\r');
        break;
        case 0x59: /* Keypad 1 and End */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '1');
            }
        break;
        case 0x5A: /* Keypad 2 and Down Arrow */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '2');
            }
        break;
        case 0x5B: /* Keypad 3 and PageDn */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '3');
            }
        break;
        case 0x5C: /* Keypad 4 and Left Arrow */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '4');
            }
        break;
        case 0x5D: /* Keypad 5 */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '5');
            }
        break;
        case 0x5E: /* Keypad 6 and Right Arrow */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '6');
            }
        break;
        case 0x5F: /* Keypad 7 and Home */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '7');
            }
        break;
        case 0x60: /* Keypad 8 and Up Arrow */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '8');
            }
        break;
        case 0x61: /* Keypad 9 and PageUp */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '9');
            }
        break;
        case 0x62: /* Keypad 0 and Insert */
            if (pHidKbd->bfNumLock)
            {
                cbPut(pKeyBuffer, (uint8_t) '0');
            }
        break;
        case 0x64: /* UK \ and | */
        {
            /* Get the state of the caps lock key */
            bool_t bfShift = pHidKbd->bfCapsLock;
            /* Update with the state of the shift keys */
            if (pHidKbd->byModifier & (HID_KEY_RIGHT_SHIFT | HID_KEY_LEFT_SHIFT))
            {
                bfShift ^= true;
            }

            if (bfShift)
            {
                cbPut(pKeyBuffer, (uint8_t) '|');
            }
            else
            {
                cbPut(pKeyBuffer, (uint8_t) '\\');
            }
            break;
        }

        default: /* Every other key that does not require a special function */
        {
            uint8_t byKey = kbdGetKeyCode(pHidKbd);
            if (byKey)
            {
                cbPut(pKeyBuffer, byKey);
            }
            break;
        }
    }

    /* Set the event to wake any task that is waiting for a key press */
    R_OS_SetEvent(&pHidKbd->keyEvent);
}
/******************************************************************************
 End of function  kbdPutKey
 ******************************************************************************/

/******************************************************************************
 Function Name: kbdParseReport
 Description:   Parse the report
 Arguments:     IN  pHidKbd - Pointer to the driver extension
                IN  pbyReport - Pointer to the report
 Return value:  none
 ******************************************************************************/
static void kbdParseReport (PHIDKBD pHidKbd, uint8_t *pbyReport)
{
    /* Update the key map */
    uint8_t byCur = kbdUpdateKeyMap(pHidKbd, pbyReport);

    /* Update the modifier */
    if (pHidKbd->byModifier ^ *pbyReport)
    {
        pHidKbd->byModifier = *pbyReport;

        /* This is to stop the auto-repeat function */
        pHidKbd->byKeyDown = 0;
    }

    /* Check for a new key press */
    if (byCur)
    {
        /* Track the current key for the auto repeat function */
        pHidKbd->byKeyDown = byCur;

        /* Put the key into the buffer */
        kbdPutKey(pHidKbd);

        /* Set the auto-repeat state */
        pHidKbd->rptState = KBD_AUTO_REPEAT_INITIAL_DELAY;
        pHidKbd->iRepeatCount = 0;
    }
    else if (pHidKbd->byKeyDown)
    {
        /* Check to see if the key is still down */
        if (kbdKeyIsDown(pHidKbd, pHidKbd->byKeyDown))
        {
            switch (pHidKbd->rptState)
            {
                case KBD_AUTO_REPEAT_INITIAL_DELAY :
                {
                    pHidKbd->iRepeatCount++;
                    if (pHidKbd->iRepeatCount > HID_KBD_AUTO_REPEAT_INIT)
                    {
                        /* Repeat the key press */
                        kbdPutKey(pHidKbd);

                        /* Move to the faster repeat state */
                        pHidKbd->iRepeatCount = 0;
                        pHidKbd->rptState = KBD_AUTO_REPEAT_DELAY;
                    }
                    break;
                }

                default :
                {
                    pHidKbd->iRepeatCount++;
                    if (pHidKbd->iRepeatCount > HID_KBD_AUTO_REPEAT)
                    {
                        pHidKbd->iRepeatCount = 0;
                        /* Repeat the key press */
                        kbdPutKey(pHidKbd);
                    }
                    break;
                }
            }
        }
        else
        {
            pHidKbd->byKeyDown = 0;
        }
    }
}
/******************************************************************************
 End of function  kbdParseReport
 ******************************************************************************/

/******************************************************************************
 Function Name: kbd_poll_task
 Description:   Task to poll the keyboard for data
 Arguments:     IN  pHidKbd - Pointer to the driver extension
 Return value:  none
 ******************************************************************************/
static os_task_code_t kbd_poll_task (void *param)
{
    PHIDKBD pHidKbd = (PHIDKBD) param;

    while (1)
    {
        switch (pHidKbd->kbdState)
        {
            case KBD_IDLE:
            {
                /* Start a transfer */
                if (kbdStartTransfer(pHidKbd))
                {
                    pHidKbd->kbdState = KBD_WAIT_REPORT;
                }

                break;
            }

            case KBD_WAIT_REPORT:
            {
                /* Wait for the device to return a report descriptor or the timer to time-out */
                bool_t result = R_OS_WaitForEvent(&pHidKbd->readRequest.ioSignal, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);

                if (!result)
                {
                    usbhCancelTransfer(&pHidKbd->readRequest);
                }
                else
                {
                    /* Check the request error code */
                    if (pHidKbd->readRequest.errorCode)
                    {
                        /* Set the error code for return to read function */
                        pHidKbd->errorCode = HID_KBD_REPORT_ERROR;
                    }
                    else
                    {
                        /* Parse the report descriptor */
                        kbdParseReport(pHidKbd, pHidKbd->pbyReportDescriptor);
                    }
                }

                /* Start the next transfer */
                if (!kbdStartTransfer(pHidKbd))
                {
                    pHidKbd->kbdState = KBD_IDLE;
                }

                break;
            }
        }
        R_OS_Yield();
    }

    return NULL;
}
/******************************************************************************
 End of function kbd_poll_task
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
