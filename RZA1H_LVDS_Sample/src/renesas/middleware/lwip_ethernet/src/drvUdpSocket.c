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
* File Name    : drvUdpSocket.c
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : None
* H/W Platform : RSK+
* Description  : A driver to make a lwip UDP socket look like a file stream.
                 This is so the console code (which uses the file streams)
                 can be used to make a console over UDP. This socket always
                 sends the data back to the sender.
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
#include "r_task_priority.h"

#include "drvUdpSocket.h"
#include "trace.h"
#include "lwIP_Interface.h"
#include "lwip\sockets.h"
#include "lwip\netdb.h"

/******************************************************************************
Defines
******************************************************************************/

#define UDP_ECHO_PORT_DEFAULT       5000

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

typedef enum _LNKSIG
{
    UDPE_LINK_STATUS_CHANGE = 0,
    UDPE_LINK_STATE,
    UDPE_NUM_LINK_EVENTS
} LNKSIG;

/******************************************************************************
Typedefs
******************************************************************************/

/* Similar to Microsoft Windows Extended data types */
typedef struct sockaddr_in SOCKADDR_IN, *PSOCKADDR_IN;
typedef struct sockaddr SOCKADDR, *PSOCKADDR;

#pragma pack(1)
typedef struct _UDPS
{
    /* The file descriptor of the UDP socket */
    int         iSocket;
    /* The port number to use */
    uint16_t    usPortNumber;
    /* The destination - so we know when to connect */
    SOCKADDR_IN destIP;
    /* Flag to show that the destination address has been set */
    _Bool       bfDestSet;
    /* The address of the source message */
    SOCKADDR_IN srcIP;
    /* The ID of the link monitor task */
    os_task_t   *uiLinkMonTaskID;
    /* The link status change events */
    PEVENT      ppEventList[UDPE_NUM_LINK_EVENTS];
} UDPE,
*PUDPE;
#pragma pack()

/******************************************************************************
Function Prototypes
******************************************************************************/

static int drvOpen(st_stream_ptr_t pStream);
static void drvClose(st_stream_ptr_t pStream);
static int drvRead(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int drvWrite(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int drvControl(st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct);
static void drvWaitIoReady(PUDPE pUdp);
static void drvLinkMonTask(PUDPE pUdp);

/******************************************************************************
Constant Data
******************************************************************************/

/* Define the driver function table for this */
const st_r_driver_t gUdpSocketDriver =
{
    "UDP/IP Socket Echo Driver",
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
Parameters:    IN  pStream - Pointer to the file stream
Return value:  0 for success otherwise -1
******************************************************************************/
static int drvOpen(st_stream_ptr_t pStream)
{
    /* Allocate the memory for the driver */
    PUDPE pUdp = R_OS_AllocMem(sizeof(UDPE), R_REGION_LARGE_CAPACITY_RAM);

    if (pUdp)
    {
        /* LNKSIG  linkSig; */
        uint32_t events_not_created;
        memset(pUdp, 0, sizeof(UDPE));

        /* Create the events */
        events_not_created = eventCreate(pUdp->ppEventList, UDPE_NUM_LINK_EVENTS);

        if (events_not_created == 0)
        {
            /* Register the link status change event */
            if (ipAddLinkMonitorEvent(pUdp->ppEventList[UDPE_LINK_STATUS_CHANGE]))
            {
                /* Set the default port number */
                pUdp->usPortNumber = UDP_ECHO_PORT_DEFAULT;
                pUdp->iSocket = -1;

                /* Create the task to monitor the link status */
                pUdp->uiLinkMonTaskID = R_OS_CreateTask("UDP/IP Link Mon", (os_task_code_t) drvLinkMonTask, pUdp,
                        R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_UDP_IP_LINK_MON_PRI);

                if (NULL != pUdp->uiLinkMonTaskID)
                {
                    pStream->p_extension = pUdp;
                    return 0;
                }
            }
        }

        /* Destroy any events that were created */
        eventDestroy(pUdp->ppEventList, UDPE_NUM_LINK_EVENTS - events_not_created);

        /* Free the memory */
        R_OS_FreeMem(pUdp);
    }

    return -1;
}
/******************************************************************************
End of function  drvOpen
******************************************************************************/

/******************************************************************************
Function Name: drvClose
Description:   Function to close the driver
Parameters:    IN  pStream - Pointer to the file stream
Return value:  none
******************************************************************************/
static void drvClose(st_stream_ptr_t pStream)
{
    PUDPE pUdp = (PUDPE) pStream->p_extension;
    TRACE(("drvUdpSocket: drvClose:\r\n"));

    R_OS_DeleteTask(pUdp->uiLinkMonTaskID);
    ipRemoveLinkMonitorEvent(pUdp->ppEventList[UDPE_LINK_STATUS_CHANGE]);
    eventDestroy(pUdp->ppEventList, UDPE_NUM_LINK_EVENTS);

    if (pUdp->iSocket >= 0)
    {
        lwip_close(pUdp->iSocket);
    }

    R_OS_FreeMem(pUdp);
}
/******************************************************************************
End of function  drvClose
******************************************************************************/

/******************************************************************************
Function Name: drvRead
Description:   Function to read from the device
Parameters:    IN  pStream - Pointer to the file stream
               IN  pbyBuffer - Pointer to the destination memory
               IN  uiCount - The number of bytes to read
Return value:  The number of bytes read or -1 on error
******************************************************************************/
static int drvRead(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    PUDPE pUdp = (PUDPE) pStream->p_extension;
    socklen_t srcIpLength = sizeof(SOCKADDR_IN);
    int iReceived;

    /* If there is no link then wait here for a valid one to become available */
    drvWaitIoReady(pUdp);
    iReceived = lwip_recvfrom(pUdp->iSocket, pbyBuffer, (size_t) uiCount, 0, (PSOCKADDR) &pUdp->srcIP, &srcIpLength);

    /* If the link goes down then bomb out */
    if (!ipLink())
    {
        return 0;
    }

    /* Check for a connection to a new client */
    if ((!pUdp->bfDestSet) || (memcmp(&pUdp->srcIP, &pUdp->destIP, sizeof(SOCKADDR_IN))))
    {
        pUdp->destIP = pUdp->srcIP;
        lwip_connect(pUdp->iSocket, (PSOCKADDR) &pUdp->destIP, sizeof(SOCKADDR_IN));

        /* Set the flag to allow outgoing messages */
        pUdp->bfDestSet = true;
    }

    return iReceived;
}
/******************************************************************************
End of function  drvRead
******************************************************************************/

/******************************************************************************
Function Name: drvWrite
Description:   Function to write to the device
Parameters:    IN  pStream - Pointer to the file stream
               IN  pbyBuffer - Pointer to the source memory
               IN  uiCount - The number of bytes to write
Return value:  The length of data written -1 on error
******************************************************************************/
static int drvWrite(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    PUDPE pUdp = (PUDPE) pStream->p_extension;

    /* Because this is UDP - which is designed as a "fire and forget" message -
       any errors that happen here will not be passed to the upper level */
    int iResult;
    int32_t bytes_written = -1;

    /* Check that there is an open socket and a destination */
    if ((pUdp->iSocket >= 0) && (pUdp->bfDestSet))
    {
        bytes_written = 0;

        while (uiCount > 0)
        {
            /* Write the data - the maximum size of UDP transfer is 65507 
               but that it too big! Split into smaller writes as required.
               In this case ones that will fit into the MTU */
            size_t stLength = ((size_t) uiCount > 1460UL) ? 1460UL : (size_t) uiCount;

            iResult = lwip_write(pUdp->iSocket, pbyBuffer, stLength);
            if (iResult > 0)
            {
                uiCount -= (uint32_t) iResult;
                bytes_written += iResult;
            }
            else
            {
                bytes_written = -1;
                break;
            }
        }
    }

    return bytes_written;
}
/******************************************************************************
End of function  drvWrite
******************************************************************************/

/******************************************************************************
Function Name: drvControl
Description:   Function to handle custom controls for the device
Parameters:    IN  pStream - Pointer to the file stream
               IN  ctlCode - The custom control code
               IN  pCtlStruct - Pointer to the custom control structure
Return value:  0 for success -1 on error
******************************************************************************/
static int drvControl(st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct)
{
    if ((ctlCode == CTL_SET_PORT_NUMBER) && (pCtlStruct))
    {
        uint16_t * pusPortNumber = (uint16_t *) pCtlStruct;
        PUDPE pUdp = (PUDPE) pStream->p_extension;
        pUdp->usPortNumber = *pusPortNumber;

        if (pUdp->iSocket >= 0)
        {
            if (lwip_bind(pUdp->iSocket, (PSOCKADDR) &pUdp->srcIP, sizeof(SOCKADDR_IN)))
            {
                TRACE(("drvControl: CTL_SET_PORT_NUMBER Bind Failed\r\n"));
                return -1;
            }
        }

        return 0;
    }

    if (CTL_STREAM_UDP_TCP == ctlCode)
    {
        return 0;
    }

    return -1;
}
/******************************************************************************
End of function  drvControl
******************************************************************************/

/*****************************************************************************
Function Name: drvWaitIoReady
Description:   Function to wait until it is ready for IO
Parameters:    IN  pUdp - Pointer to the driver data
Return value:  none
*****************************************************************************/
static void drvWaitIoReady(PUDPE pUdp)
{
    while (pUdp->iSocket < 0)
    {
        /* Wait for the link monitor task to signal a good link */
        eventWait(&pUdp->ppEventList[UDPE_LINK_STATE], 1, true);
    }
}
/*****************************************************************************
End of function  drvWaitIoReady
******************************************************************************/

/*****************************************************************************
Function Name: drvLinkMonTask
Description:   Function to monitor the link status and manage the socket
Parameters:    IN  pUdp - Pointer to the driver data
Return value:  none
*****************************************************************************/
static void drvLinkMonTask(PUDPE pUdp)
{
    R_OS_TaskUsesFloatingPoint();

    while (true)
    {
        /* Check for an IP link */
        if (ipLink())
        {
            /* Check that the socket is closed */
            if (pUdp->iSocket < 0)
            {
                /* Open a UDP socket */
                int iSocket = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                if (iSocket >= 0)
                {
                    /* Set the input for our default port number and any IP address */
                    pUdp->srcIP.sin_family = AF_INET;
                    pUdp->srcIP.sin_addr.s_addr = INADDR_ANY;
                    pUdp->srcIP.sin_port = htons(pUdp->usPortNumber);
                    if (!lwip_bind(iSocket,
                                   (PSOCKADDR)&pUdp->srcIP,
                                   sizeof(SOCKADDR_IN)))
                    {
                        /* Set the Socket */
                        pUdp->iSocket = iSocket;

                        /* Signal any IO that the link has changed */
                        eventSet(pUdp->ppEventList[UDPE_LINK_STATE]);
                    }
                }
            }
        }
        else
        {
            /* If the socket is open */
            if (pUdp->iSocket >= 0)
            {
                int iSocket = pUdp->iSocket;
                pUdp->iSocket = -1;
                pUdp->bfDestSet = false;

                /* Close it */
                lwip_close(iSocket);

                /* Make sure that we connect again */
                memset(&pUdp->destIP, 0, sizeof(SOCKADDR_IN));
            }
        }

        /* Wait here for a link status change signal */
        eventWait(pUdp->ppEventList, 1, true);
    }
}
/*****************************************************************************
End of function  drvLinkMonTask
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
