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
 * Copyright (C) 2010 Renesas Electronics Corporation. All rights reserved.    */
/******************************************************************************
 * File Name    : drvStdIn.c
 * Version      : 1.00
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : Standard IN driver to "or" key presses from serial input or a
 *                USB keyboard.
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 MAB First Release
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

#include "iodefine_cfg.h"

#include "compiler_settings.h"
#include "control.h"

#include "r_task_priority.h"
#include "drvStdIn.h"
#include "trace.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Typedefs
 ******************************************************************************/

typedef struct _KBDIN
{
    /* The file descriptor of the input device to use */
    int      iStdIn;
    /* The ID of the task that manages the connection */
    os_task_t *p_taskid;
    /* Pointer to an event to show that a keyboard has been attached */
    event_t pAttached;

} KBDIN, *PKBDIN;

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

static int stdinOpen (st_stream_ptr_t pstream);
static void stdinClose (st_stream_ptr_t pstream);
static int stdinRead (st_stream_ptr_t pstream, uint8_t *pbyBuffer, uint32_t uiCount);
static int stdinControl (st_stream_ptr_t pstream, uint32_t ctlCode, void *pCtlStruct);

static void usb_stdin_task (void *parameters);

/******************************************************************************
 Constant Data
 ******************************************************************************/

/* Define the driver function table for this */
const st_r_driver_t gUsbStdInDriver =
{ "USB Host Standard IN Device Driver", stdinOpen, stdinClose, stdinRead, no_dev_io, stdinControl, no_dev_get_version };

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Function Name: stdinOpen
 Description:   Function to open the standard IN driver
 Arguments:     IN  st_stream_ptr_t - Pointer to the file stream
 Return value:  0 for success otherwise -1
 ******************************************************************************/
static int stdinOpen (st_stream_ptr_t pstream)
{
    PKBDIN pKbdIn = R_OS_AllocMem(sizeof(KBDIN), R_REGION_LARGE_CAPACITY_RAM);
    if (pKbdIn)
    {
        /* Set the driver extension */
        pstream->p_extension = pKbdIn;
        /* -1 is used to indicate that there is no keyboard connected */
        pKbdIn->iStdIn = -1;
        /* Create an event to signal the connection of a keyboard */
        R_OS_CreateEvent(&pKbdIn->pAttached);

        /* Create a task to manage the dynamic connection of a USB keyboard */
        pKbdIn->p_taskid = R_OS_CreateTask("USB keyboard stdin", usb_stdin_task, pKbdIn, R_OS_ABSTRACTION_PRV_SMALL_STACK_SIZE, TASK_USB_KEYBOARD_STD_PRI);

        if (NULL != pKbdIn->p_taskid)
        {
            return 0;
        }
        else
        {
            R_OS_FreeMem(pKbdIn);
        }
    }
    return -1;
}
/******************************************************************************
 End of function  stdinOpen
 ******************************************************************************/

/******************************************************************************
 Function Name: stdinClose
 Description:   Function to close the standard IN driver. All open devices
 connected to the stdin are closed.
 Arguments:     IN  st_stream_ptr_t - Pointer to the file stream
 Return value:  none
 ******************************************************************************/
static void stdinClose (st_stream_ptr_t pstream)
{
    PKBDIN pKbdIn = (PKBDIN) pstream->p_extension;

    close(pKbdIn->iStdIn);
    R_OS_DeleteEvent(&pKbdIn->pAttached);
    R_OS_DeleteTask(pKbdIn->p_taskid);
    R_OS_FreeMem(pKbdIn);
}
/******************************************************************************
 End of function  stdinClose
 ******************************************************************************/

/******************************************************************************
 Function Name: stdinRead
 Description:   Function to read from standard IN input devices
 Arguments:     IN  st_stream_ptr_t - Pointer to the file stream
 IN  pbyBuffer - Pointer to the destination memory
 IN  uiCount - The number of bytes to read
 Return value:  The number of bytes read or -1 on error
 ******************************************************************************/
static int stdinRead (st_stream_ptr_t pstream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    PKBDIN pKbdIn = (PKBDIN) pstream->p_extension;
    int_t iCount = 0;

    /* Always read just one key at a time */
    uiCount = (uiCount > 1) ? 1 : uiCount;

    /* While there is data to be read */
    while (uiCount)
    {
        /* Check to see if a keyboard is attached */
        if (pKbdIn->iStdIn == -1)
        {
            /* Wait here until one is available */
            R_OS_WaitForEvent(&pKbdIn->pAttached, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);
        }

        /* If there is a valid file descriptor */
        if (pKbdIn->iStdIn != -1)
        {
            int_t stCount = control(pKbdIn->iStdIn, CTL_GET_RX_BUFFER_COUNT, NULL);
            if (stCount > 0)
            {
                int_t iResult = read(pKbdIn->iStdIn, pbyBuffer + iCount, uiCount);
                if (iResult > 0)
                {
                    iCount += iResult;
                    uiCount = (uiCount - (long unsigned int)iResult);
                }
            }
            else
            {
                R_OS_TaskSleep(1UL);
            }
        }
    }
    return iCount;
}
/******************************************************************************
 End of function  stdinRead
 ******************************************************************************/

/******************************************************************************
 Function Name: stdinControl
 Description:   Function to handle custom controls for the standard IN device
 Arguments:     IN  st_stream_ptr_t - Pointer to the file stream
 IN  ctlCode - The custom control code
 IN  pCtlStruct - Pointer to the custorm control structure
 Return value:  0 for success -1 on error
 ******************************************************************************/
static int stdinControl (st_stream_ptr_t pstream, uint32_t ctlCode, void *pCtlStruct)
{
    PKBDIN pKbdIn = (PKBDIN) pstream->p_extension;

    /* Re-direct the control to the USB keyboard device driver */
    return control(pKbdIn->iStdIn, ctlCode, pCtlStruct);
}
/******************************************************************************
 End of function  stdinControl
 ******************************************************************************/

/*****************************************************************************
 Function Name: stdinTask
 Description:   Task to handle the dynamic connection of a USB keyboard
 Arguments:     IN  pKbdIn - Pointer to the device driver data
 Return value:  none
 *****************************************************************************/
static void usb_stdin_task (void *parameters)
{
    PKBDIN pKbdIn = (PKBDIN)parameters;

    while (true)
    {
        if (pKbdIn->iStdIn == -1)
        {
            int iStdIn = open(DEVICE_INDENTIFIER "HID Keyboard",
            O_RDWR,
            _IONBF);
            if (iStdIn != -1)
            {
                pKbdIn->iStdIn = iStdIn;

                TRACE(("stdinTask: USB Keyboard attached %d\r\n", pKbdIn->iStdIn));
                /* Set the attached event to start access to the keyboard */
                R_OS_SetEvent(&pKbdIn->pAttached);
            }
        }
        /* Check to see that the keyboard is still attached */
        if (pKbdIn->iStdIn != -1)
        {
            /* It does not matter what method is used here - this is just
             to see if the physical device is still connected */
            if (control(pKbdIn->iStdIn, CTL_GET_RX_BUFFER_COUNT,
            NULL) == -1)
            {
                /* Set the standard in file descriptor to -1 since the
                 control call failed indicating that the device has been
                 disconnected */
                pKbdIn->iStdIn = -1;
                TRACE(("stdinTask: USB Keyboard removed\r\n"));
                R_OS_TaskSleep(3500UL);
            }
        }
        R_OS_TaskSleep(250UL);
    }
}
/*****************************************************************************
 End of function  stdinTask
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
