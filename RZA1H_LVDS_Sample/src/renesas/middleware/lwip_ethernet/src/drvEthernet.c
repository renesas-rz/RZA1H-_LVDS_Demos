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
* File Name    : drvEthernet.c
* Version      : 1.01
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : FreeRTOS
* H/W Platform : RSK+
* Description  : Ethernet device driver using the proposed Ethernet Periheral
*                Driver Library API (R_Ether_... functions)
*******************************************************************************
* History      : DD.MM.YYYY Version Description
*              : DD.MM.YYYY 1.00 First Release
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

#include "compiler_settings.h"

#include "r_ether.h"
#include "r_phy.h"
#include "trace.h"
#include "lwip_interface.h"
#include "r_task_priority.h"
#include "r_cache_l1_rz_api.h"
#include "drvEthernet.h"

/******************************************************************************
Macro definitions
******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

#define ET_CHANNEL                      (0)

/******************************************************************************
Typedef definitions
******************************************************************************/
/* Enumerate the Ethernet events */
typedef enum _ETEV
{
    ET_RX_ISR = 0,
    ET_LINK_STATUS_CHANGE,
    ET_NUM_EVENTS
} ETEV;

#pragma pack(1)
/* Define the structure of data used by the driver */
typedef struct _ETDRV
{
    /* The events */
    PEVENT      ppEventList[ET_NUM_EVENTS];
    uint8_t     pbyMacAddress[6];
    _Bool       bfPromiscuous;
    os_task_t   *uiLinkMonitorTaskID;
    uint32_t    uiLinkStatus;
    uint32_t    uiRxIsrCount;
} ETDRV,
*PETDRV;
#pragma pack()

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/

static int etOpen(st_stream_ptr_t pStream);
static void etClose(st_stream_ptr_t pStream);
static int etRead(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int etWrite(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int etControl(st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct);
static void etTaskLinkMonitor(PETDRV pEtDrv);
static void etRxIsrCallBack(PETDRV pEtDrv);

/* Define the driver function table for this device */
const st_r_driver_t gEtherCDriver =
{
    "EtherC Device Driver",
    etOpen,
    etClose,
    etRead,
    etWrite,
    etControl,
    no_dev_get_version
};

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/**********************************************************************************
 Function Name: etMalloc
 Description:   Function to allocate memory aligned to a byte boundary
 Parameters:    IN  stLength - The length of memory to allocate
                IN  iAlign - The byte boundary to align to
                OUT ppvBase - Pointer to the base memory pointer
 Return value:  Pointer to the aligned memory
 **********************************************************************************/
void *etMalloc(size_t stLength, uint32_t iAlign, void **ppvBase)
{
    /* Allocate the memory with enough length to align to the boundary */
    void *pvResult = R_OS_AllocMem(stLength + (size_t) iAlign, R_REGION_UNCACHED_RAM);

    if (pvResult)
    {
        void     *pvPhysical;
        size_t    stPhysicalLength;

        /* Cast to mirror address */
        uint32_t stAddress = (size_t) pvResult;

        /* Return the pointer to the base of the memory */
        if (ppvBase)
        {
            *ppvBase = (void *)pvResult;
        }

        /* Align to the 32byte boundary */
        stAddress += (iAlign - 1U);
        stAddress &= ~(iAlign - 1U);
        pvPhysical = (void *) stAddress;
        stPhysicalLength = stLength;

        R_CACHE_L1_CleanLine((uint32_t) pvPhysical, stPhysicalLength);

        TRACE(("etMalloc: Allocated 0x%p Length %lu\r\n", pvPhysical, stPhysicalLength));

        /* Check to see if there is a page boundary in the allocated range */
        if (stPhysicalLength < stLength)
        {
            /* We have now got a problem since the area that R_OS_AllocMem has given us
             * has a page boundary in it. TODO: Deal with this at a later date */
            R_OS_FreeMem(pvResult);
            pvResult = NULL;
        }
        else
        {
            /* Return the address of the physical memory */
            pvResult = pvPhysical;
        }
    }
    else if (ppvBase)
    {
        *ppvBase = NULL;
    }

    return pvResult;
}
/**********************************************************************************
 End of function  etMalloc
 ***********************************************************************************/

/**********************************************************************************
 Function Name: etFree
 Description:   Function to free the memory allocated
 Parameters:    IN  pvBase
 Return value:  none
 **********************************************************************************/
void etFree(void *pvBase)
{
    R_OS_FreeMem(pvBase);
}
/**********************************************************************************
 End of function  etFree
 ***********************************************************************************/

/*****************************************************************************
 * Function Name: etOpen
 * Description  : Function to open the Ethernet controller
 * Arguments    : IN  pStream - Pointer to the file stream
 * Return Value : 0 for success otherwise -1
 ******************************************************************************/
static int etOpen(st_stream_ptr_t pStream)
{
    PETDRV pEtDrv = (PETDRV) R_OS_AllocMem(sizeof(ETDRV), R_REGION_LARGE_CAPACITY_RAM);

    if (pEtDrv != NULL)
    {
        uint32_t events_not_created;

        memset(pEtDrv, 0, sizeof(ETDRV));

        /* Create a number of events */
        events_not_created = eventCreate(pEtDrv->ppEventList, ET_NUM_EVENTS);

        if (events_not_created > 0)
        {
            /* Failed to create one or more event, so free the ones allocated
               and return an error */
            eventDestroy(pEtDrv->ppEventList, ET_NUM_EVENTS - events_not_created);
            R_OS_FreeMem(pEtDrv);
            return -1;
        }

        /* Set the extension */
        pStream->p_extension = pEtDrv;

        /* Open the driver - only succeeds if there is a link available */
        if (0 == R_Ether_Open(ET_CHANNEL, pEtDrv->pbyMacAddress))
        {
            /* Set the receive call-back */
            lan_set_rx_call_back((void (*)(void *)) etRxIsrCallBack, (void *) pEtDrv);

            /* Create a task to monitor the state of the link */
            pEtDrv->uiLinkMonitorTaskID = R_OS_CreateTask("EtherC Link", (os_task_code_t) etTaskLinkMonitor, pEtDrv,
                    R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_ETHERC_LINK_MON_PRI);

            if (NULL != pEtDrv->uiLinkMonitorTaskID)
            {
                return 0;
            }
        }

        etClose(pStream);
    }

    return -1;
}
/**********************************************************************************
 End of function  etOpen
 ***********************************************************************************/

/*****************************************************************************
End of function  etOpen
******************************************************************************/

/*****************************************************************************
 * Function Name: etClose
 * Description  : Function to close the ethernet controller
 * Arguments    : IN  pStream - Pointer to the file stream
 * Return Value : none
 ******************************************************************************/
static void etClose(st_stream_ptr_t pStream)
{
    PETDRV  pEtDrv = (PETDRV)pStream->p_extension;

    /* Destroy the link monitor task */
    if (NULL != pEtDrv->uiLinkMonitorTaskID)
    {
        R_OS_DeleteTask(pEtDrv->uiLinkMonitorTaskID);
    }

    /* Close the lower level driver */
    R_Ether_Close(ET_CHANNEL);

    /* Set the event to make a task waiting on a read transfer return */
    eventSet(pEtDrv->ppEventList[ET_RX_ISR]);

    /* Remove the call-back */
    lan_set_rx_call_back(NULL, NULL);

    /* Free the events */
    eventDestroy(pEtDrv->ppEventList, (uint32_t) (ET_NUM_EVENTS));

    /* Free the memory */
    R_OS_FreeMem(pEtDrv);
}
/*****************************************************************************
 End of function  etClose
 ******************************************************************************/


/*****************************************************************************
 * Function Name: etRead
 * Description  : Function to read from the Ethernet controller
 * Arguments    : IN  pStream - Pointer to the file stream
 *                IN  pbyBuffer - Pointer to the destination memory
 *                IN  uiCount - The number of bytes to read
 * Return Value : The number of bytes read -1 on error
 ******************************************************************************/
static int etRead(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    PETDRV pEtDrv = (PETDRV) pStream->p_extension;
    int32_t iResult = -1;

    /* Check to make sure that the buffer size is not too small */
    if (uiCount < SIZE_OF_BUFFER)
    {
        return -1;
    }

    /* Block until some data is available */
    while (iResult < 0)
    {
        /* Try to read from the lower level driver */
        iResult = R_Ether_Read(ET_CHANNEL, pbyBuffer);

        /* Wait on the ISR event if there is no data available */
        if (R_ETHER_NODATA == iResult)
        {
            eventWait(&pEtDrv->ppEventList[ET_RX_ISR], 1, true);
        }
    }

    return (int) iResult;
}
/*****************************************************************************
 End of function  etRead
 ******************************************************************************/


/*****************************************************************************
 * Function Name: etWrite
 * Description  : Function to write to the Ethernet controller
 * Arguments    : IN  pStream - Pointer to the file stream
 *                IN  pbyBuffer - Pointer to the source memory
 *                IN  uiCount - The number of bytes to write
 * Return Value : The number of bytes written -1 on error
 ******************************************************************************/
static int etWrite(st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    (void) pStream;        /* avoid unused parameter warning */

    return R_Ether_Write(ET_CHANNEL, pbyBuffer, uiCount);
}
/*****************************************************************************
 End of function  etWrite
 ******************************************************************************/


/*****************************************************************************
 * Function Name: etControl
 * Description  : Ethernet Control Functions
 * Arguments    : IN  pStream - Pointer to the file stream
 *                IN  ctlCode - The custom control code
 *                IN  pCtlStruct - Pointer to the custom control structure
 * Return Value : 0 for success -1 on error
 ******************************************************************************/
static int etControl(st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct)
{
    /* Get the extension */

    PETDRV  pEtDrv = (PETDRV)pStream->p_extension;
    switch (ctlCode)
    {
        case CTL_GET_LINK_STATE:
        {
            if (pCtlStruct)
            {
                if (pEtDrv->uiLinkStatus > NO_LINK)
                {
                    *((_Bool *) pCtlStruct) = true;
                }
                else
                {
                    *((_Bool *) pCtlStruct) = false;
                }

                return 0;
            }

            break;
        }

        case CTL_GET_LINK_CHANGE_EVENT:
        {
            if (pCtlStruct)
            {
                /* For clarity the void* passed to this function is
                   the address of the destination event pointer,
                   therefore it is a pointer to an event pointer */
                PPEVENT ppEvent = (PPEVENT) pCtlStruct;
                *ppEvent = pEtDrv->ppEventList[ET_LINK_STATUS_CHANGE];
                return 0;
            }
            break;
        }

        case CTL_SET_MEDIA_ACCESS_CONTROL_ADDRESS:
        {
            if (pCtlStruct)
            {
                memcpy(pEtDrv->pbyMacAddress, pCtlStruct, sizeof(pEtDrv->pbyMacAddress));
                lan_mac_set(pEtDrv->pbyMacAddress);
                pEtDrv->bfPromiscuous = false;
                lan_promiscuous_mode(pEtDrv->bfPromiscuous);
                return 0;
            }
            break;
        }

        case CTL_SET_PROMISCUOUS_MODE:
        {
            if (pCtlStruct)
            {
                pEtDrv->bfPromiscuous = *((_Bool *) pCtlStruct);
                lan_promiscuous_mode(pEtDrv->bfPromiscuous);
                return 0;
            }
            break;
        }

        case CTL_GET_PROMISCUOUS_MODE:
        {
            if (pCtlStruct)
            {
                *((_Bool *) pCtlStruct) = pEtDrv->bfPromiscuous;
                return 0;
            }
            break;
        }

        default:
        {
            TRACE(("etControl: Unknown control code\r\n"));
            break;
        }
    }

    return -1;
}
/*****************************************************************************
 End of function  etControl
 ******************************************************************************/


/******************************************************************************
 Private functions
 ******************************************************************************/


/******************************************************************************
 * Function Name: etTaskLinkMonitor
 * Description  : Test task monitor the state of the Ethernet link
 * Arguments    : IN  pEtDrv - Pointer to the Ethernet driver
 * Return Value : none
 ******************************************************************************/
static void etTaskLinkMonitor(PETDRV pEtDrv)
{
    R_OS_TaskUsesFloatingPoint();

    while (1)
    {
        /* Check the status of the link */
        int32_t linkStatus;

        /* Poll the link 1 times a second */
        R_OS_TaskSleep(1000UL);

        /* Check the status of the link */
        linkStatus = lan_link_check();

        /* Compare with the previous status for change */
        if ((uint32_t) linkStatus != pEtDrv->uiLinkStatus)
        {
            /* Save the link status for the next compare */
            pEtDrv->uiLinkStatus = (uint32_t) linkStatus;

            /* Set the link status change event */
            eventSet(pEtDrv->ppEventList[ET_LINK_STATUS_CHANGE]);
        }
    }
}
/******************************************************************************
 End of function  etTaskLinkMonitor
 ******************************************************************************/


/******************************************************************************
 * Function Name: etRxIsrCallBack
 * Description  : Function to handle the RX ISR call-back
 * Arguments    : IN  pEtDrv - Pointer to the Ethernet driver
 * Return Value : none
 ******************************************************************************/
static void etRxIsrCallBack(PETDRV pEtDrv)
{
    pEtDrv->uiRxIsrCount++;
    eventSet(pEtDrv->ppEventList[ET_RX_ISR]);
}
/******************************************************************************
 End of function  etRxIsrCallBack
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
