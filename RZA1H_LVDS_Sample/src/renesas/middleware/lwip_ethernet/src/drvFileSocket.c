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
* Copyright (C) 2014 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************
* File Name    : drvFileSocket.h
* Version      : 1.0
* Description  : A driver to make a lwip socket look like a file stream.
                 This is so the console code (which uses the file streams)
                 can be used to make a console over TCP. The read part is
                 inefficient in its implementation but the "command shell"
                 is expecting key presses!
******************************************************************************
* History      : DD.MM.YYYY Ver. Description
*              : 04.02.2010 1.00 First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "compiler_settings.h"
#include "drvFileSocket.h"
#include "trace.h"
#include "lwIP_Interface.h"
#include "r_cbuffer.h"
#include "socket.h"

/******************************************************************************
Defines
******************************************************************************/

#define TCP_RX_SOFTWARE_FIFO_SIZE   1024

/* Define a session inactivity time out to disconnect clients in seconds */
#define TCP_SESSION_TIME_OUT        (15 * 60UL)

/******************************************************************************
Function Macros
******************************************************************************/

#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/*****************************************************************************
Enumerated Types
******************************************************************************/

typedef enum _IPFDSIG
{
    IPFD_SOCKET_STATUS_CHANGE = 0,
    IPFD_SOCKET_READY,
    IPFD_CLOSE_LINK,
    IPFD_READ_WAKE,
    IPFD_NUM_LINK_EVENTS
} IPFDSIG;

/******************************************************************************
Typedefs
******************************************************************************/

#pragma pack(1)
typedef struct _IPFD
{
    /* The file descriptor of the data socket */
    int         iSocket;
    /* The ID of the reader task */
    os_task_t   *uiInStreamTaskID;
    /* The ID of the link monitor task */
    os_task_t   *uiIdleTimeOutTaskID;
    /* The link status change events */
    PEVENT      ppEventList[IPFD_NUM_LINK_EVENTS];
    /* The circular buffer for input */
    PCBUFF      pcbBuffer;
    /* The Idle counter */
    uint32_t    uiIdleCounter;
    /* Flag to indicate time-out */
    _Bool       bfConnectionTerminated;
    /* Data to hold the idle timer call-back function */
    IDLECB      idleCallBack;
} IPFD,
*PIPFD;
#pragma pack()

/******************************************************************************
Function Prototypes
******************************************************************************/

static int drvOpen(st_stream_ptr_t pStream);
static void drvClose(st_stream_ptr_t pStream);
static int drvRead(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int drvReadFromBuffer(PCBUFF pcBuffer, uint8_t *pbyBuffer, uint32_t uiCount);
static int drvWrite(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int drvControl(st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct);
static void drvWaitIoReady(PIPFD pIPFD);
static void drvStreamInputTask(PIPFD pIPFD);
static void drvSessionTimerTask(PIPFD pIPFD);

/******************************************************************************
Constant Data
******************************************************************************/

/* Define the driver function table for this device */
const st_r_driver_t gFileSocketDriver =
{
    "File Socket Driver",
    drvOpen,
    drvClose,
    drvRead,
    drvWrite,
    drvControl,
    no_dev_get_version
};

/******************************************************************************
Private Functions
******************************************************************************/

/******************************************************************************
 Function Name: drvOpen
 Description:   Function to open the driver
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  0 for success otherwise -1
 ******************************************************************************/
static int drvOpen(st_stream_ptr_t pStream)
{
    /* Allocate the memory for the driver */
    PIPFD pIPFD = R_OS_AllocMem(sizeof(IPFD), R_REGION_LARGE_CAPACITY_RAM);

    if (pIPFD)
    {
        uint32_t events_not_created;

        memset(pIPFD, 0, sizeof(IPFD));
        pIPFD->pcbBuffer = cbCreate(TCP_RX_SOFTWARE_FIFO_SIZE);

        if (pIPFD->pcbBuffer)
        {
            /* Create the events */
            events_not_created = eventCreate(pIPFD->ppEventList, IPFD_NUM_LINK_EVENTS);

            if (events_not_created == 0)
            {
                /* Set the default port number */
                pIPFD->iSocket = -1;

                /* Create the task to monitor the link status */
                pIPFD->uiInStreamTaskID = R_OS_CreateTask("TCP File Reader", (os_task_code_t) drvStreamInputTask, pIPFD,
                        R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_UDP_IP_LINK_MON_PRI);

                if (pIPFD->uiInStreamTaskID != NULL)
                {
                    pStream->p_extension = pIPFD;
                    pIPFD->uiIdleTimeOutTaskID = R_OS_CreateTask("TCP Idle Timer", (os_task_code_t) drvSessionTimerTask, pIPFD,
                            R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_UDP_IP_LINK_MON_PRI);
                    TRACE(("drvOpen:\r\n"));
                    return 0;
                }
            }

            /* Destroy any events that were created */
            eventDestroy(pIPFD->ppEventList, IPFD_NUM_LINK_EVENTS - events_not_created);
        }

        /* Free the memory */
        R_OS_FreeMem(pIPFD);
    }

    return -1;
}
/******************************************************************************
 End of function  drvOpen
 ******************************************************************************/

/******************************************************************************
 Function Name: drvClose
 Description:   Function to close the driver
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  none
 ******************************************************************************/
static void drvClose(st_stream_ptr_t pStream)
{
    PIPFD pIPFD = (PIPFD) pStream->p_extension;
    TRACE(("drvFileSocket: drvClose:\r\n"));

    pStream->p_extension = NULL;

    R_OS_DeleteTask(pIPFD->uiInStreamTaskID);
    R_OS_DeleteTask(pIPFD->uiIdleTimeOutTaskID);

    eventDestroy(pIPFD->ppEventList, IPFD_NUM_LINK_EVENTS);
    if (pIPFD->iSocket >= 0)
    {
        lwip_close(pIPFD->iSocket);
    }

    cbDestroy(pIPFD->pcbBuffer);
    R_OS_FreeMem(pIPFD);
}
/******************************************************************************
 End of function  drvClose
 ******************************************************************************/

/******************************************************************************
 Function Name: drvRead
 Description:   Function to read from the device
 Arguments:     IN  pStream - Pointer to the file stream
                IN  pbyBuffer - Pointer to the destination memory
                IN  uiCount - The number of bytes to read
 Return value:  The number of bytes read or -1 on error
 ******************************************************************************/
static int drvRead(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    PIPFD pIPFD = (PIPFD) pStream->p_extension;
    PCBUFF pcbBuffer = pIPFD->pcbBuffer;

    /* Reset the wake event */
    eventReset(pIPFD->ppEventList[IPFD_READ_WAKE]);
    if (cbUsed(pcbBuffer))
    {
        return drvReadFromBuffer(pcbBuffer, pbyBuffer, uiCount);
    }

    /* Wait for some data to be read by the link monitor task */
    eventWait(&pIPFD->ppEventList[IPFD_READ_WAKE], 1, true);

    /* Check for session time-out */
    if (pIPFD->bfConnectionTerminated)
    {
        return -1;
    }

    /* Check that the link is still up */
    if (pIPFD->iSocket >= 0)
    {
        /* Deliver the data */
        return drvReadFromBuffer(pcbBuffer, pbyBuffer, uiCount);
    }

    return -1;
}
/******************************************************************************
 End of function drvRead
 ******************************************************************************/

/*****************************************************************************
 Function Name: drvReadFromBuffer
 Description:   Function to copy data from the circular buffer to the destination
 Arguments:     IN  pcbBuffer - Pointer to the circular buffer object
                IN  pbyBuffer - Pointer to the destination buffer
                IN  uiCount - The length of the destination buffer
 Return value:  The number of bytes delivered
 *****************************************************************************/
static int drvReadFromBuffer(PCBUFF pcbBuffer, uint8_t *pbyBuffer, uint32_t uiCount)
{
    uint8_t *pbyDest = pbyBuffer;

    while (uiCount--)
    {
        if (!cbGet(pcbBuffer, pbyDest))
        {
            break;
        }
        else
        {
            pbyDest++;
        }
    }

    return (int) (pbyDest - pbyBuffer);
}
/*****************************************************************************
 End of function drvReadFromBuffer
 ******************************************************************************/

/******************************************************************************
 Function Name: drvWrite
 Description:   Function to write to the device
 Arguments:     IN  pStream - Pointer to the file stream
                IN  pbyBuffer - Pointer to the source memory
                IN  uiCount - The number of bytes to write
 Return value:  The length of data written -1 on error
 ******************************************************************************/
static int drvWrite(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    PIPFD pIPFD = (PIPFD) pStream->p_extension;
    int iResult;

    /* If there is no link then wait here for a valid one to become available */
    drvWaitIoReady(pIPFD);

    /* Write the data */
    iResult = lwip_write(pIPFD->iSocket, pbyBuffer, (size_t) uiCount);

    /* Check for session time-out */
    if (pIPFD->bfConnectionTerminated)
    {
        return -1;
    }

    /* If the link is dropped then write returns -1 */
    if (iResult < 0)
    {
        /* Set the flag to make everything quit */
        pIPFD->bfConnectionTerminated = true;

        /* Signal the link monitor task to close the link and wait for another connection */
        eventSet(pIPFD->ppEventList[IPFD_CLOSE_LINK]);
    }
    else
    {
        /* Refresh the session timer */
        pIPFD->uiIdleCounter = TCP_SESSION_TIME_OUT;
    }

    return iResult;
}
/******************************************************************************
 End of function drvWrite
 ******************************************************************************/

/******************************************************************************
 Function Name: drvControl
 Description:   Function to handle custom controls for the device
 Arguments:     IN  pStream - Pointer to the file stream
                IN  ctlCode - The custom control code
                IN  pCtlStruct - Pointer to the custom control structure
 Return value:  0 for success -1 on error
 ******************************************************************************/
static int drvControl(st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct)
{
    PIPFD  pIPFD = (PIPFD)pStream->p_extension;

    if ((ctlCode == CTL_SET_LWIP_SOCKET_INDEX) && (pCtlStruct))
    {
        /* Control to set the lwIP socket */
        pIPFD->iSocket = (*((int *) pCtlStruct));
        eventSet(pIPFD->ppEventList[IPFD_SOCKET_STATUS_CHANGE]);
        TRACE(("FileSocket: lwIP set socket (%d) to File %d\r\n",
                pIPFD->iSocket, pStream->file_number));
        return 0;
    }

    if ((ctlCode == CTL_GET_LWIP_SOCKET_INDEX) && (pCtlStruct))
    {
        *((int *) pCtlStruct) = pIPFD->iSocket;
        TRACE(("FileSocket: lwIP get socket (%d)\r\n", pIPFD->iSocket));
        return 0; 
    }

    if (CTL_STREAM_REQUIRES_AUTHENTICATION == ctlCode)
    {
        #ifdef _DEBUG_
        /* Switch authentication (passwords) off when debugging */
        return -1;
        #else
        return 0;
        #endif
    }

    if ((CTL_STREAM_TCP_CONNECTED == ctlCode) && (pCtlStruct))
    {
        if (pIPFD->bfConnectionTerminated)
        {
            *((_Bool *) pCtlStruct) = false;
        }
        else
        {
            *((_Bool *) pCtlStruct) = true;
        }

        return 0;
    }

    if ((CTL_STREAM_SET_CONNECTION_IDLE_CALL_BACK == ctlCode) && (pCtlStruct))
    {
        pIPFD->idleCallBack = *((PIDLECB) pCtlStruct);
    }
    else if (CTL_STREAM_TCP == ctlCode)
    {
        return 0;
    }
    else if (CTL_STREAM_UDP_TCP == ctlCode)
    {
        return 0;
    }
    else if (CTL_GET_RX_BUFFER_COUNT == ctlCode)
    {
        return (int) cbUsed(pIPFD->pcbBuffer);
    }

    return -1;
}
/******************************************************************************
 End of function drvControl
 ******************************************************************************/

/*****************************************************************************
 Function Name: drvWaitIoReady
 Description:   Function to wait until it is ready for IO
 Arguments:     IN  pIPFD - Pointer to the driver data
 Return value:  none
 *****************************************************************************/
static void drvWaitIoReady(PIPFD pIPFD)
{
    while ((pIPFD->iSocket < 0) && (!pIPFD->bfConnectionTerminated))
    {
        /* Wait for the link monitor task to signal a good link */
        eventWait(&pIPFD->ppEventList[IPFD_SOCKET_READY], 1, true);
    }
}
/*****************************************************************************
 End of function drvWaitIoReady
 ******************************************************************************/

/*****************************************************************************
 Function Name: drvStreamInputTask
 Description:   Function to read data from the input stream
 Arguments:     IN  pIPFD - Pointer to the driver data
 Return value:  none
 *****************************************************************************/
static void drvStreamInputTask(PIPFD pIPFD)
{
    R_OS_TaskUsesFloatingPoint();

    while (true)
    {
        /* Wait here for the lwIP socket signal */
        eventWait(&pIPFD->ppEventList[IPFD_SOCKET_STATUS_CHANGE], 1, true);

        /* Signal a write waiting on a valid socket */
        eventSet(pIPFD->ppEventList[IPFD_SOCKET_READY]);

        /* Check for an IP link */
        while (ipLink())
        {
            uint8_t pbyBuffer[512];

            /* Read data from the socket */
            int iReceived = lwip_read(pIPFD->iSocket, pbyBuffer, sizeof(pbyBuffer));

            /* Check for a failed write */
            if (eventState(pIPFD->ppEventList[IPFD_CLOSE_LINK]) == EV_SET)
            {
                break;
            }

            /* If the link is dropped then write returns -1 */
            if (iReceived <= 0)
            {
                /* Set the flag to make everything quit */
                pIPFD->bfConnectionTerminated = true;

                /* Wake a pending read */
                eventSet(pIPFD->ppEventList[IPFD_READ_WAKE]);
                break;
            }
            else
            {
                /* Put the data in the circular buffer */
                uint8_t *pbyPut = pbyBuffer;
                while (iReceived--)
                {
                    /* This is designed for a keyboard interface so in the
                       interest in simplicity if lots of data is received
                       some of it will be lost */
                    cbPut(pIPFD->pcbBuffer, *pbyPut++);
                }

                /* Wake a pending read */
                eventSet(pIPFD->ppEventList[IPFD_READ_WAKE]);

                /* Refresh the session timer */
                pIPFD->uiIdleCounter = TCP_SESSION_TIME_OUT;
            }
        }
    }
}
/*****************************************************************************
 End of function drvStreamInputTask
 ******************************************************************************/

/*****************************************************************************
 Function Name: drvSessionTimerTask
 Description:   Function to refresh the session timer
 Arguments:     IN  pIPFD - Pointer to the driver data
 Return value:  none
 *****************************************************************************/
static void drvSessionTimerTask(PIPFD pIPFD)
{
    R_OS_TaskUsesFloatingPoint();

    while (true)
    {
        /* Wait 1 second */
        R_OS_TaskSleep(1000UL);
        pIPFD->uiIdleCounter--;

        if (!pIPFD->uiIdleCounter)
        {
            break;
        }
    }

    /* Set the terminated flag */
    pIPFD->bfConnectionTerminated = true;

    /* Wake a pending read */
    eventSet(pIPFD->ppEventList[IPFD_READ_WAKE]);

    /* Call the idle time-out call back function */
    if (pIPFD->idleCallBack.pCallBack)
    {
        pIPFD->idleCallBack.pCallBack(pIPFD->idleCallBack.uiTaskID,
                                      pIPFD->idleCallBack.pvParameter);
    }
}
/*****************************************************************************
 End of function drvSessionTimerTask
 ******************************************************************************/

/******************************************************************************
 End Of File
 ******************************************************************************/
