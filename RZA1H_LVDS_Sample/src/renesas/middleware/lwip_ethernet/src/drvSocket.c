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
* File Name    : drvUDPSocket.c
* Version      : 1.0
* Description  : A driver to integrate lwIP into the POSIX style API framework
*                for functions close, read and write which conflict the
*                C run time library functions in lowsrc.c.
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
#include "drvSocket.h"
#include "trace.h"
#include "lwip\sockets.h"

/******************************************************************************
Function Macros
******************************************************************************/

#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif


/******************************************************************************
Function Prototypes
******************************************************************************/

static int drvOpen(st_stream_ptr_t pStream);
static void drvClose(st_stream_ptr_t pStream);
static int drvRead(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int drvWrite(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int drvControl(st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct);

/******************************************************************************
Constant Data
******************************************************************************/

/* Define the driver function table for this */
const st_r_driver_t gSocketDriver =
{
    "lwIP Socket Driver Wrapper",
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
    (void) pStream;        /* avoid unused parameter warning */

    TRACE(("drvOpen: lwIP Wrapper\r\n"));
    /* There is nothing to do here. When the lwip socket is opened the index
       is passed to this driver and set in the extension. This is just a
       wrapper for the read, write, and close functions. */
    return 0;
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
    int iSocket = (int) pStream->p_extension;
    TRACE(("drvClose: lwip_close(%d)\r\n", iSocket));
    lwip_close(iSocket);
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
    int iSocket = (int) pStream->p_extension;
    TRACE(("drvRead: lwip_read(%d)\r\n", iSocket));

    /* Read data from the socket */
    return  lwip_read(iSocket, pbyBuffer, (size_t)uiCount);
}
/******************************************************************************
End of function  drvRead
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
    int iSocket = (int) pStream->p_extension;
    TRACE(("drvWrite: lwip_write(%d)\r\n", iSocket));

    /* Write the data to the socket */
    return lwip_write(iSocket, pbyBuffer, (size_t)uiCount);
}
/******************************************************************************
End of function  drvWrite
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
    if ((ctlCode == CTL_SET_LWIP_SOCKET_INDEX) && (pCtlStruct))
    {
        pStream->p_extension = (void *)(*((int *) pCtlStruct));
        TRACE(("drvControl: lwIP set socket (%d)\r\n", pStream->p_extension));
        return 0;
    }

    if ((ctlCode == CTL_GET_LWIP_SOCKET_INDEX) && (pCtlStruct))
    {
        *((int *)pCtlStruct) = (int) pStream->p_extension;
        TRACE(("drvControl: lwIP get socket (%d)\r\n", pStream->p_extension));
        return 0;
    }

    if (CTL_STREAM_REQUIRES_AUTHENTICATION == ctlCode)
    {
        return 0;
    }

    return -1;
}
/******************************************************************************
End of function  drvControl
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
