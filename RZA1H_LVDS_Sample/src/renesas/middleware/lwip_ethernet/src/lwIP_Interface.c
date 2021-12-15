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
* File Name    : lwIP_Interface.c
* Version      : 1.00
* Description  : Interface functions for lwIP
*******************************************************************************
* History      : DD.MM.YYYY Ver. Description
*              : 04.04.2011 1.00 First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "compiler_settings.h"

#include "r_event.h"

#include "lwIP_Interface.h"
#include "task.h"
#include "lwip/tcpip.h"
#include "lwip/netifapi.h"
#include "netif/etharp.h"
#include "trace.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"

/******************************************************************************
Macro definitions
******************************************************************************/

#define ETHERNET_INPUT_BUFFER_SIZE          1600U
#define ETHERNET_OUTPUT_BUFFER_SIZE         1600U
#define ETHERNET_MALLOC_TYPE                HEAP_SRAM
#define ETHERNET_MAX_ADAPTER_NAME_LENGTH    16U
#define ETHERNET_NETIF_MTU                  1500U

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
Typedef definitions
******************************************************************************/

typedef struct ip_addr *PIPADDR;

typedef struct _RTEIP *PRTEIP;

/* A structure to keep a list of events which are set when the link status
   changes */
typedef struct _LNKMON
{
    struct _LNKMON  *pNext;
    PEVENT          pEvent;
} LNKMON,
*PLNKMON;

#pragma pack(1)
typedef struct _RTEIP
{
    /* Pointer to the next interface */
    struct _RTEIP   *pNext;

    /* The data required by lwIP for a network interface */
    struct netif    ipNetIf;

    /* The file descriptor of the Ethernet driver */
    int             iEtherC;

    /* The symbolic link name of the Ethernet driver */
    char_t          pszInterface[ETHERNET_MAX_ADAPTER_NAME_LENGTH];

    /* The IP configuration of this interface (address, mask & gateway...) */
    NVDHCP          ipConfig;

    /* The File stream to print Ethernet link status changes to */
    FILE            *pFile;

    /* The link status flag */
    _Bool           bfLink;

    /* The ID of the link monitor task */
    os_task_t       *uiLinkMonitorTaskID;

    /* The ID of the lwIP "TCP/IP" task */
    os_task_t       *uilwIPTaskID;

    /* The ID of the input task */
    os_task_t       *uiInputTaskID;

    /* Buffer used to create the output packet */
    uint8_t         pbyOuputBuffer[ETHERNET_OUTPUT_BUFFER_SIZE];

    /* The mutex event used to protect the output buffer */
    PEVENT          pevOutputBufferMutex;
} RTEIP;
#pragma pack()

/******************************************************************************
Function Prototypes
******************************************************************************/

static void ipIfStatusDisplay(PRTEIP pEtherC, FILE *pOut);
static err_t ipInitialise(struct netif *pIpNetIf);
static void ipInputTask(PRTEIP pEtherC);
static void ipLinkMonitor(PRTEIP pEtherC);
static void ipLinkStatus(struct netif *pIpNetIf);
static void ipSetTcpIpTaskID(PRTEIP pEtherC);
static err_t ipOutput(struct netif *pIpNetIf, struct pbuf *pPacket);
static void ipAddNetIf(PRTEIP *ppEtherC, PRTEIP pEtherC);
static PRTEIP ipRemoveNetIf(PRTEIP *ppEtherC, PRTEIP pEtherC);
static PRTEIP ipFindNetIf(PRTEIP *ppEtherC, char_t *pszInterface);
static void ipAddLinkMonitor(PLNKMON *ppLinkMon, PLNKMON pMon);
static PLNKMON ipRemoveLinkMonitor(PLNKMON *ppLinkMon, PEVENT pEvent);
static void ipLinkChangeNotification(PLNKMON pLinkMon);

/******************************************************************************
External Variables
******************************************************************************/
#ifdef _ALPHA_NUMERIC_DISPLAY_
extern FILE    *gpLCD;
#endif

/******************************************************************************
Global Variables
******************************************************************************/

/* Mutex events to protect the link change list & the network interface list */
static PEVENT   gpevLinkChangeListLock = NULL;
static PEVENT   gpevNetListLock = NULL;

/* The linked list of Ethernet Controller interfaces */
static PRTEIP   gpEtherC = NULL;

/* The list of link change events */
static PLNKMON  gpLinkMon = NULL;

/******************************************************************************
Public Functions
******************************************************************************/

/******************************************************************************
* Function Name: ipStart
* Description  : Function to start the Internet Protocol Suite
* Arguments    : IN  pLinkStatus - Optional pointer to a file stream to receive
*                                  link status information
*                IN  pszInterface - The symbolic link name of the interface
*                IN  pMediaAccessControl - Pointer to the MAC address
*                IN  pIpConfig - Pointer to the configuration
* Return Value : 0 for success or -1 on error
******************************************************************************/
int32_t ipStart(FILE *pLinkStatus, char_t *pszInterface, uint8_t *pMediaAccessControl, PNVDHCP pIpConfig)
{
    PRTEIP pEtherC = (PRTEIP) R_OS_AllocMem(sizeof(RTEIP), R_REGION_LARGE_CAPACITY_RAM);

    if (pEtherC)
    {
        /* Initialise the data */
        memset(pEtherC, 0U, sizeof(RTEIP));
        pEtherC->pFile = pLinkStatus;
        pEtherC->uilwIPTaskID = 0;
        pEtherC->uiInputTaskID = 0;

        /* If this is the first call to this function then lwIP must be initialised */
        if (NULL == gpEtherC)
        {
            PEVENT  pEvent;
            if (eventCreate(&pEvent, 1U) == 0U)
            {
                /* Initialise lwIP */
                tcpip_init((void(*) (void *)) eventSet, pEvent);

                /* When the lwIP task has finished initialisation it
                   will call eventSet and wake this task up */
                eventWait(&pEvent, 1U, true);
                eventDestroy(&pEvent, 1U);

                /* Set a call back from the task created by the init
                   function to get its ID */
                tcpip_callback((void(*) (void *)) ipSetTcpIpTaskID, pEtherC);
            }
        }

        /* Add to the list of interfaces */
        eventWaitMutex(&gpevNetListLock, EV_WAIT_INFINITE);
        ipAddNetIf(&gpEtherC, pEtherC);
        eventReleaseMutex(&gpevNetListLock);

        /* Put the MAC address in the net interface data structure */
        memcpy(pEtherC->ipNetIf.hwaddr, pMediaAccessControl, NETIF_MAX_HWADDR_LEN);

        /* Set the length of the MAC address */
        pEtherC->ipNetIf.hwaddr_len = NETIF_MAX_HWADDR_LEN;

        /* Set the user's network settings */
        pEtherC->ipConfig = *pIpConfig;

        /* Check the DHCP setting */
        if (pEtherC->ipConfig.byEnableDHCP)
        {
            /* Reset the IP address, net mask and gateway to zero */
            memset(&pEtherC->ipConfig, 0, sizeof(NVDHCP));
            memset(pEtherC->ipConfig.pbyAddressMask, 0xFF, 3);
            pEtherC->ipConfig.byEnableDHCP = true;
        }

        /* Copy in the interface symbolic link name */
        strncpy((char_t *) pEtherC->pszInterface, (char_t *) pszInterface, ETHERNET_MAX_ADAPTER_NAME_LENGTH);

        /* Add the net interface */
        if (netif_add(&pEtherC->ipNetIf,
                      (PIPADDR) pEtherC->ipConfig.pbyIpAddress,
                      (PIPADDR) pEtherC->ipConfig.pbyAddressMask,
                      (PIPADDR) pEtherC->ipConfig.pbyGatewayAddress,
                      pEtherC,
                      ipInitialise,
                      ethernet_input))
        {
            /* Set the link status call-back */
            netif_set_status_callback(&pEtherC->ipNetIf, ipLinkStatus);

            /* Set it as default */
            netif_set_default(&pEtherC->ipNetIf);

            /* Update the link status if DHCP is not enabled */
            if (0 == pEtherC->ipConfig.byEnableDHCP)
            {
                ipLinkStatus(&pEtherC->ipNetIf);
            }

            return 0U;
        }
        else
        {
            TRACE(("ipStart: netif_add failed\r\n"));
        }
    }

    return -1;
}
/******************************************************************************
End of function  ipStart
******************************************************************************/

/******************************************************************************
* Function Name: ipStop
* Description  : Function to stop lwIP
* Arguments    : none
* Return Value : none
******************************************************************************/
void ipStop(void)
{
    eventWaitMutex(&gpevNetListLock, EV_WAIT_INFINITE);
    while (gpEtherC)
    {
        PRTEIP  pEtherC = gpEtherC;

        /* Close the linked events */
        if(NULL != pEtherC->pevOutputBufferMutex)
        {
              eventWaitMutex(&pEtherC->pevOutputBufferMutex, EV_WAIT_INFINITE);
           eventDestroy(&pEtherC->pevOutputBufferMutex, 1);
           pEtherC->pevOutputBufferMutex = NULL;
        }

        /* Stop all the tasks */
        R_OS_DeleteTask(pEtherC->uilwIPTaskID);
        R_OS_DeleteTask(pEtherC->uiInputTaskID);
        R_OS_DeleteTask(pEtherC->uiLinkMonitorTaskID);

        /* Close the interface */
        close(pEtherC->iEtherC);

        /* This function will set gpEtherC = NULL when there are no more
           interfaces on the list */
        if (ipRemoveNetIf(&gpEtherC, pEtherC))
        {
            R_OS_FreeMem(pEtherC);
        }
    }
    eventReleaseMutex(&gpevNetListLock);
    eventDestroy(&gpevNetListLock, 1);
    gpevNetListLock = NULL;
}
/******************************************************************************
End of function  ipStop
******************************************************************************/

/******************************************************************************
* Function Name: ipReconfigure
* Description  : Function to re-configure the interface
* Arguments    : IN  pszInterface - The symbolic link name of the interface
*                IN  pIpConfig - Pointer to the new configuration
* Return Value : 0 for success or -1 on error
******************************************************************************/
int32_t ipReconfigure(char_t *pszInterface, PNVDHCP pIpConfig)
{
    PRTEIP pEtherC = ipFindNetIf(&gpEtherC, pszInterface);
    if (pEtherC)
    {
        if (0U == pIpConfig->byEnableDHCP)
        {
            _Bool bfLinkUp = false;
            pEtherC->ipConfig = *pIpConfig;

            /* If DHCP was previously on */
            if (pEtherC->ipConfig.byEnableDHCP)
            {
                /* Stop it */
                tcpip_callback((void(*)(void*)) dhcp_release, &pEtherC->ipNetIf);
                tcpip_callback((void(*)(void*)) dhcp_stop,
                &pEtherC->ipNetIf);
            }

            /* Set the fixed address */
            memcpy(&pEtherC->ipNetIf.ip_addr.addr, pEtherC->ipConfig.pbyIpAddress, sizeof(struct ip_addr));
            memcpy(&pEtherC->ipNetIf.netmask.addr, pEtherC->ipConfig.pbyAddressMask, sizeof(struct ip_addr));
            memcpy(&pEtherC->ipNetIf.gw.addr, pEtherC->ipConfig.pbyGatewayAddress, sizeof(struct ip_addr));

            /* Check the state of the link */
            if ((control(pEtherC->iEtherC, CTL_GET_LINK_STATE, &bfLinkUp) == 0U) && (bfLinkUp))
            {
                /* Tell lwIP that the data link is up */
                tcpip_callback((void(*) (void *)) netif_set_up, &pEtherC->ipNetIf);
            }
        }
        else
        {
            if (0U == pEtherC->ipConfig.byEnableDHCP)
            {
                pEtherC->ipConfig = *pIpConfig;

                /* Protect against concurrent access,
                   specifically in the ARP modules */
                tcpip_callback((void (*) (void *)) netif_set_link_down, &pEtherC->ipNetIf);

                /* Also have to do this one as well. Is it not obvious
                   that if the physical link has been set down that
                   the data link is down too? */
                tcpip_callback((void (*) (void*)) netif_set_down, &pEtherC->ipNetIf);

                /* Tell lwIP to start DHCP */
                tcpip_callback((void (*) (void *)) dhcp_start, &pEtherC->ipNetIf);
            }
            else
            {
                pEtherC->ipConfig = *pIpConfig;
            }
        }

        return 0;
    }

    return -1;
}
/******************************************************************************
End of function  ipReconfigure
******************************************************************************/

/******************************************************************************
* Function Name: ipGetConfiguration
* Description  : Function to get the current configuration
* Arguments    : IN  pszInterface - The symbolic link name of the interface
*                OUT pIpConfig - Pointer to the destination configuration
* Return Value : 0 for success or -1 on error
******************************************************************************/
int32_t ipGetConfiguration(char_t *pszInterface, PNVDHCP pIpConfig)
{
    PRTEIP pEtherC = ipFindNetIf(&gpEtherC, pszInterface);
    if (pEtherC)
    {
        *pIpConfig = pEtherC->ipConfig;
        return 0;
    }

    return -1;
}
/******************************************************************************
End of function  ipGetConfiguration
******************************************************************************/

/******************************************************************************
* Function Name: ipAddLinkMonitorEvent
* Description  : Function to add an event to the link monitor list
* Arguments    : IN  pEvent - Pointer to the event to add to the link monitor
*                             list
* Return Value : true if the event was added to the listsd
******************************************************************************/
_Bool ipAddLinkMonitorEvent(PEVENT pEvent)
{
    PLNKMON pLinkMon =  (PLNKMON) R_OS_AllocMem(sizeof(PLNKMON), R_REGION_LARGE_CAPACITY_RAM);

    if (pLinkMon)
    {
        pLinkMon->pEvent = pEvent;
        eventWaitMutex(&gpevLinkChangeListLock, EV_WAIT_INFINITE);
        ipAddLinkMonitor(&gpLinkMon, pLinkMon);
        eventReleaseMutex(&gpevLinkChangeListLock);
        return true;
    }

    return false;
}
/******************************************************************************
End of function  ipAddLinkMonitorEvent
******************************************************************************/

/******************************************************************************
* Function Name: ipRemoveLinkMonitorEvent
* Description  : Function to remove an event from the link monitor list
* Arguments    : IN  pEvent - Pointer to the event to remove from the link
*                             monitor list
* Return Value : none
******************************************************************************/
void ipRemoveLinkMonitorEvent(PEVENT pEvent)
{
    PLNKMON pLinkMon;

    eventWaitMutex(&gpevLinkChangeListLock, EV_WAIT_INFINITE);
    pLinkMon = ipRemoveLinkMonitor(&gpLinkMon, pEvent);
    eventReleaseMutex(&gpevLinkChangeListLock);

    if (pLinkMon)
    {
        R_OS_FreeMem(pLinkMon);
    }
}
/******************************************************************************
End of function  ipRemoveLinkMonitorEvent
******************************************************************************/

/******************************************************************************
* Function Name: ipLink
* Description  : Function to return true if there is an IP data link available
* Arguments    : none
* Return Value : true if there is a link
******************************************************************************/
_Bool ipLink(void)
{
    _Bool bfLink = false;

    eventWaitMutex(&gpevNetListLock, EV_WAIT_INFINITE);
    {
        PRTEIP  pEtherC = gpEtherC;
        while (pEtherC)
        {
            if (pEtherC->bfLink)
            {
                bfLink = true;
                break;
            }

            pEtherC = pEtherC->pNext;
        }
    }

    eventReleaseMutex(&gpevNetListLock);

    return bfLink;
}
/******************************************************************************
End of function  ipLink
******************************************************************************/

/******************************************************************************
* Function Name: ipLinkStatusDisplay
* Description  : Function to display ipLinkStatus
* Arguments    : IN  pLinkStatus - Pointer to a file stream to print the link
*                                  status to
* Return Value : none
******************************************************************************/
void ipLinkStatusDisplay(FILE *pLinkStatus)
{
    eventWaitMutex(&gpevNetListLock, EV_WAIT_INFINITE);
    {
        PRTEIP  pEtherC = gpEtherC;

        while (pEtherC)
        {
            ipIfStatusDisplay(pEtherC, pLinkStatus);
            fflush(pLinkStatus);
            pEtherC = pEtherC->pNext;
        }
    }

    eventReleaseMutex(&gpevNetListLock);
}
/******************************************************************************
End of function ipLinkStatusDisplay
******************************************************************************/

/******************************************************************************
* Function Name: ipSetPromiscuousMode
* Description  : Function to set the network adapter into promiscuous mode
* Arguments    : IN  pszInterface - Pointer to the interface to set
*                IN  bfPromiscuous - true for promiscuous mode
* Return Value : 0 for success or -1 on error
******************************************************************************/
int32_t ipSetPromiscuousMode(char_t *pszInterface, _Bool bfPromiscuous)
{
    PRTEIP pEtherC = ipFindNetIf(&gpEtherC, pszInterface);

    if (pEtherC)
    {
        return (int32_t) control(pEtherC->iEtherC, CTL_SET_PROMISCUOUS_MODE, &bfPromiscuous);
    }

    return -1;
}
/******************************************************************************
End of function  ipSetPromiscuousMode
******************************************************************************/

/******************************************************************************
* Function Name: ipGetPromiscuousMode
* Description  : Function to get the adapter promiscuous mode
* Arguments    : IN  pszInterface - Pointer to the interface to get
* Return Value : true for promiscuous, false for everything else
******************************************************************************/
_Bool ipGetPromiscuousMode(char_t *pszInterface)
{
    _Bool   bfResult = false;
    PRTEIP pEtherC = ipFindNetIf(&gpEtherC, pszInterface);

    if (pEtherC)
    {
        control(pEtherC->iEtherC, CTL_GET_PROMISCUOUS_MODE, &bfResult);
    }

    return bfResult;
}
/******************************************************************************
End of function  ipGetPromiscuousMode
******************************************************************************/

/******************************************************************************
Private Functions
******************************************************************************/
//TODO: Remove #if(0)
#if 0
/**********************************************************************************
Function Name: ipPrintIPv6Address
Description:   Function to print an IPv6 address
Parameters:    OUT pszDest - Pointer to the destination buffer
               IN  pIPv6Address - Pointer to the address
Return value:  none
**********************************************************************************/
static void ipPrintIPv6Address(char          *pszDest,
                               ip6_addr_t    *pIPv6Address)
{
    /* TODO: Enhance to insert :: */
    sprintf(pszDest,
            "%X:%X:%X:%X:%X:%X:%X:%X",
            IP6_ADDR_BLOCK1(pIPv6Address),
            IP6_ADDR_BLOCK2(pIPv6Address),
            IP6_ADDR_BLOCK3(pIPv6Address),
            IP6_ADDR_BLOCK4(pIPv6Address),
            IP6_ADDR_BLOCK5(pIPv6Address),
            IP6_ADDR_BLOCK6(pIPv6Address),
            IP6_ADDR_BLOCK7(pIPv6Address),
            IP6_ADDR_BLOCK8(pIPv6Address));
    // TODO: Remove
    Trace("IPv6 %s\r\n", pszDest);
}
/**********************************************************************************
End of function  ipPrintIPv6Address
***********************************************************************************/
#endif
/******************************************************************************
* Function Name: ipIfStatusDisplay
* Description  : Function display Link status
* Arguments    : IN  pEtherC - Pointer to the ethernet controller to display
*                IN  pOut - Pointer to the output file stream
* Return Value : none
******************************************************************************/
static void ipIfStatusDisplay(PRTEIP pEtherC, FILE *pOut)
{
#if 0
    int32_t      iCount = 1;
    char         pszIPv6Address[48];
#endif
    struct netif *pIpNetIf = &pEtherC->ipNetIf;
    static const char * const pszEnabled[] = 
    {
        "Disabled",
        "Enabled"
    };
    fprintf(pOut,
            "Ethernet Controller \"%s\"\r\n",
            pEtherC->pszInterface);
    if (pIpNetIf->flags & NETIF_FLAG_UP)
    {
        fprintf(pOut, "Link Up\r\n");
        pEtherC->bfLink = true;
    }
    else
    {
        fprintf(pOut, "Link Down\r\n");
        pEtherC->bfLink = false;
    }
    fprintf(pOut, "MAC     = %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\r\n",
            pIpNetIf->hwaddr[0U], pIpNetIf->hwaddr[1U],
            pIpNetIf->hwaddr[2U], pIpNetIf->hwaddr[3U],
            pIpNetIf->hwaddr[4U], pIpNetIf->hwaddr[5U]);

    /* Inform the user of the current settings */
    fprintf(pOut, "DHCP    = %s\r\n"
           "Address = %*d.%*d.%*d.%*d\r\n"
           "Mask    = %*d.%*d.%*d.%*d\r\n"
           "Gateway = %*d.%*d.%*d.%*d\r\n",
           pszEnabled[(pEtherC->ipConfig.byEnableDHCP) ? 1U : 0U],
           3, pEtherC->ipConfig.pbyIpAddress[0U],
           3, pEtherC->ipConfig.pbyIpAddress[1U],
           3, pEtherC->ipConfig.pbyIpAddress[2U],
           3, pEtherC->ipConfig.pbyIpAddress[3U],
           3, pEtherC->ipConfig.pbyAddressMask[0U],
           3, pEtherC->ipConfig.pbyAddressMask[1U],
           3, pEtherC->ipConfig.pbyAddressMask[2U],
           3, pEtherC->ipConfig.pbyAddressMask[3U],
           3, pEtherC->ipConfig.pbyGatewayAddress[0U],
           3, pEtherC->ipConfig.pbyGatewayAddress[1U],
           3, pEtherC->ipConfig.pbyGatewayAddress[2U],
           3, pEtherC->ipConfig.pbyGatewayAddress[3U]);

    /* Of the list of IPv6 addresses index 0 is the link local.
       This unicast address is created from the MAC */
    //TODO: Remove #if(0)
#if 0
    ipPrintIPv6Address(pszIPv6Address, &pIpNetIf->ip6_addr[0]);
    fprintf(pOut, "IPv6 Link Local:\r\n%s\r\n", pszIPv6Address);
    while (iCount < LWIP_IPV6_NUM_ADDRESSES)
    {
        if (netif_ip6_addr_state(pIpNetIf, iCount))
        {
            //TODO:
            //ipPrintIPv6Address(pszIPv6Address, &pIpNetIf->ip6_addr[iCount]);
            fprintf(pOut, "IPv6 Address:\r\n%s\r\n", pszIPv6Address);
        }
        iCount++;
    }
#endif
    fflush(pOut);
}
/******************************************************************************
End of function  ipIfStatusDisplay
******************************************************************************/

/******************************************************************************
* Function Name: ipInitialise
* Description  : Callback function that initialises the interface
* Arguments    : IN  pIpNetIf - Pointer to the network interface data structure
*                               required by lwIP
* Return Value : ERR_OK if successful & ERR_IF if it went wrong
******************************************************************************/
static err_t ipInitialise(struct netif *pIpNetIf)
{
    PRTEIP pEtherC = (PRTEIP) pIpNetIf->state;

    /* Open the interface to the Ethernet driver */
    pEtherC->iEtherC = open((char_t *) pEtherC->pszInterface, O_RDWR, 0x1000 /* FNBIO */);

    if (pEtherC->iEtherC > 0)
    {
        /* Tell the hardware it's MAC address */
        control(pEtherC->iEtherC, CTL_SET_MEDIA_ACCESS_CONTROL_ADDRESS, pEtherC->ipNetIf.hwaddr);
        TRACE(("ipInitialise: %d\r\n", pEtherC->iEtherC));

        /* Set the interface name RE for Renesas Electronics */
        pIpNetIf->name[0U] = 'R';
        pIpNetIf->name[1U] = 'E';

        /* Set the host name */
        pIpNetIf->hostname = "WebEngine";

        /* Use the lwIP Ethernet output function to resolve addresses.
           This then send the data to the driver. Use the function
           supplied by lwIP for Ethernet */
        pIpNetIf->output = etharp_output;

        /* Specify the actual output function for V4 */
        pIpNetIf->linkoutput = ipOutput;

        /* And for V6 */
        //TODO:
        //pIpNetIf->output_ip6 = ethip6_output;

        /* Set the maximum transfer unit */
        pIpNetIf->mtu = ETHERNET_NETIF_MTU;

        /* Set the device capabilities */
        pIpNetIf->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

        /* Create an IPv6 address from the MAC */
        //TODO:
        //netif_create_ip6_linklocal_address(pIpNetIf, true);
        /* Enable IPV6 auto configuration */
        //TODO:
        //pIpNetIf->ip6_autoconfig_enabled = true;
        /* Create a task to monitor the link status */

        pEtherC->uiLinkMonitorTaskID  = R_OS_CreateTask("EtherC Link Mon", (os_task_code_t) ipLinkMonitor, pEtherC,
                R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_ETHERC_LINK_MON_PRI);
        pEtherC->uiInputTaskID  = R_OS_CreateTask("EtherC Input", (os_task_code_t) ipInputTask, pEtherC,
                R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_ETHERC_INPUT_PRI);

        return ERR_OK;
    }
    else
    {
        TRACE(("ipInitialise: Error opening driver %d\r\n", pEtherC->iEtherC));
        return ERR_IF;
    }
}
/******************************************************************************
End of function  ipInitialise
******************************************************************************/

/******************************************************************************
* Function Name: ipLinkMonitor
* Description  : Task to monitor the state of the hardware link and inform
*                lwIP of the changes
* Arguments    : IN  pEtherC - Pointer to the ethernet controller data
* Return Value : none
******************************************************************************/
static void ipLinkMonitor(PRTEIP pEtherC)
{
    PEVENT  pLinkChangeEvent = NULL;

    R_OS_TaskUsesFloatingPoint();

    if (control(pEtherC->iEtherC, CTL_GET_LINK_CHANGE_EVENT, &pLinkChangeEvent) == 0U)
    {
        TRACE(("ipLinkMonitor: Started\r\n"));
        while (true)
        {
            _Bool bfLinkUp = false;

            /* Wait for a signal from the device driver */
            eventWait(&pLinkChangeEvent, 1U, true);

            /* Check the state of the link now */
            if (control(pEtherC->iEtherC, CTL_GET_LINK_STATE, &bfLinkUp) == 0U)
            {
                /* Check the status of the link */
                if (bfLinkUp)
                {
                    TRACE(("ipLinkMonitor: Link Up\r\n"));

                    /* Protect against concurrent access, specifically in the ARP modules */
                    tcpip_callback((void(*)(void*)) netif_set_link_up, &pEtherC->ipNetIf);

                    /* Start DHCP if required */
                    if (pEtherC->ipConfig.byEnableDHCP)
                    {
                        tcpip_callback((void(*) (void *)) dhcp_start, &pEtherC->ipNetIf);
                    }
                    else
                    {
                        /* Tell lwIP that the data link is up */
                        tcpip_callback((void(*) (void *)) netif_set_up, &pEtherC->ipNetIf);
                    }
                }
                else
                {
                    /* Protect against concurrent access, specifically in the
                       ARP modules, by using a call-back */
                    TRACE(("ipLinkMonitor: Link Down\r\n"));

                    /* If the link was up & DHCP was enabled */
                    if ((pEtherC->bfLink) && (pEtherC->ipConfig.byEnableDHCP))
                    {
                        /* Release the DHCP lease */
                        tcpip_callback((void(*) (void *)) dhcp_release, &pEtherC->ipNetIf);
                    }

                    /* Tell lwIP that the link is down */
                    tcpip_callback((void(*) (void *)) netif_set_link_down,
                                   &pEtherC->ipNetIf);

                    /* Also have to do this one as well. Is it not obvious
                       that if the physical link has been set down that
                       the data link is down too? */
                    tcpip_callback((void(*) (void *)) netif_set_down, &pEtherC->ipNetIf);

                    /* Stop DHCP if enabled */
                    if (pEtherC->ipConfig.byEnableDHCP)
                    {
                        tcpip_callback((void(*) (void *)) dhcp_stop, &pEtherC->ipNetIf);
                    }
                }
            }
        }
    }
    else
    {
        TRACE(("ipLinkMonitor: Failed to get link change event\r\n"));
    }
}
/******************************************************************************
End of function  ipLinkMonitor
******************************************************************************/

/******************************************************************************
* Function Name: ipLinkStatus
* Description  : Function called by lwIP when interface is brought up/down
* Arguments    : IN  pIpNetIf - Pointer to the network interface
* Return Value : none
******************************************************************************/
static void ipLinkStatus(struct netif *pIpNetIf)
{
    PRTEIP  pEtherC = (PRTEIP) pIpNetIf->state;
    FILE    *pFile = pEtherC->pFile;

    /* Update our copy of the configuration */
    memcpy(pEtherC->ipConfig.pbyIpAddress, &pIpNetIf->ip_addr.addr, sizeof(struct ip_addr));
    memcpy(pEtherC->ipConfig.pbyAddressMask, &pIpNetIf->netmask.addr, sizeof(struct ip_addr));
    memcpy(pEtherC->ipConfig.pbyGatewayAddress, &pIpNetIf->gw.addr, sizeof(struct ip_addr));

    /* check the net interface status */
    if (pIpNetIf->flags & NETIF_FLAG_UP)
    {
        pEtherC->bfLink = true;
    }
    else
    {
        pEtherC->bfLink = false;
    }

    /* Update the status display if required */
    if (pFile)
    {
        ipIfStatusDisplay(pEtherC, pFile);
    }

    /* Notify any task waiting for link change notification */
    ipLinkChangeNotification(gpLinkMon);
}
/******************************************************************************
End of function  ipLinkStatus
******************************************************************************/

/******************************************************************************
* Function Name: ipSetTcpIpTaskID
* Description  : Function to get the ID of the task created by lwIP so it can
*                be killed when it is not required
* Arguments    : IN  pEtherC - Pointer to the data structure
* Return Value : none
******************************************************************************/
static void ipSetTcpIpTaskID(PRTEIP pEtherC)
{
    pEtherC->uilwIPTaskID = R_OS_GetCurrentTask();
}
/******************************************************************************
End of function  ipSetTcpIpTaskID
******************************************************************************/

/******************************************************************************
* Function Name: ipAddNetIf
* Description  : Function to add a network interface to the list
* Arguments    : IN  ppEtherC - Pointer to the network interface list pointer
*                IN  pEtherC - Pointer to the interface to add
* Return Value : none
******************************************************************************/
static void ipAddNetIf(PRTEIP *ppEtherC, PRTEIP pEtherC)
{
    /* Go to the end of the list */
    while (*ppEtherC)
    {
        ppEtherC = &(*ppEtherC)->pNext;
    }

    if (*ppEtherC)
    {
        /* Add to the end of the list */
        *ppEtherC = pEtherC;
    }
    else
    {
        /* Add to the start (*ppEtherC == NULL) */
        pEtherC->pNext = *ppEtherC;
        *ppEtherC = pEtherC;
    }
}
/******************************************************************************
End of function  ipAddNetIf
******************************************************************************/

/******************************************************************************
* Function Name: ipRemoveNetIf
* Description  : Function to remove a network interface
* Arguments    : IN  ppEtherC - Pointer to the network interface list pointer
*                IN  pEtherC - Pointer to the network interface to remove
* Return Value : Pointer to the network interface that was removed
******************************************************************************/
static PRTEIP ipRemoveNetIf(PRTEIP *ppEtherC, PRTEIP pEtherC)
{
    /* Search the list for the request */
    while ((NULL != *ppEtherC) && ((*ppEtherC) != pEtherC))
    {
        ppEtherC = &(*ppEtherC)->pNext;
    }

    /* If it has been found */
    if (*ppEtherC)
    {
        pEtherC = *ppEtherC;

        /* Delete from the list */
        *ppEtherC = pEtherC->pNext;
        return pEtherC;
    }

    return NULL;
}
/******************************************************************************
End of function  ipRemoveNetIf
******************************************************************************/

/******************************************************************************
* Function Name: ipFindNetIf
* Description  : Function to find a network interface
* Arguments    : IN  ppEtherC - Pointer to the network interface list pointer
*                IN  pszInterface - Pointer to the interface string use NULL
                                    to get the first interface
* Return Value : Pointer to the interface or NULL if not found
******************************************************************************/
static PRTEIP ipFindNetIf(PRTEIP *ppEtherC, char_t *pszInterface)
{
    /* If no interface is specified just return the first */
    if (pszInterface)
    {
        eventWaitMutex(&gpevNetListLock, EV_WAIT_INFINITE);

        /* Search the list for the request */
        while ((NULL != *ppEtherC) && (strcmp((*ppEtherC)->pszInterface, pszInterface)))
        {
            ppEtherC = &(*ppEtherC)->pNext;
        }

        eventReleaseMutex(&gpevNetListLock);
    }

    return *ppEtherC;
}
/******************************************************************************
End of function  ipFindNetIf
******************************************************************************/

/******************************************************************************
* Function Name: ipAddLinkMonitor
* Description  : Function to add a link montor to the list
* Arguments    : IN  ppLnkMon - Pointer to the link montor list to add the
*                               event to
* Return Value : none
******************************************************************************/
static void ipAddLinkMonitor(PLNKMON *ppLinkMon, PLNKMON pMon)
{
    /* Go to the end of the list */
    while (*ppLinkMon)
    {
        ppLinkMon = &(*ppLinkMon)->pNext;
    }

    if (*ppLinkMon)
    {
        /* Add to the end of the list */
        *ppLinkMon = pMon;
    }
    else
    {
        /* Add to the start (*ppLinkMon == NULL) */
        pMon->pNext = *ppLinkMon;
        *ppLinkMon = pMon;
    }
}
/******************************************************************************
End of function  ipAddLinkMonitor
******************************************************************************/

/******************************************************************************
* Function Name: ipRemoveLinkMonitor
* Description  : Function to remove the link monitor event
* Arguments    : IN  ppLnkMon - Pointer to the link montor list to remove the
                              event from
*                IN  pEvent - Pointer to the event to remove
* Return Value : Pointer to the link monitor object that was removed or NULL
******************************************************************************/
static PLNKMON ipRemoveLinkMonitor(PLNKMON *ppLinkMon, PEVENT pEvent)
{
    /* Search the list for the request */
    while ((NULL != *ppLinkMon) && ((*ppLinkMon)->pEvent != pEvent))
    {
        ppLinkMon = &(*ppLinkMon)->pNext;
    }

    /* If it has been found */
    if (*ppLinkMon)
    {
        PLNKMON pLinkMon = *ppLinkMon;

        /* Delete from the list */
        *ppLinkMon = pLinkMon->pNext;
        return pLinkMon;
    }

    return NULL;
}
/******************************************************************************
End of function  ipRemoveLinkMonitor
******************************************************************************/

/******************************************************************************
* Function Name: ipLinkChangeNotification
* Description  : Function to notify of the link change
* Arguments    : IN  ppLinkMon - Pointer to the notification list
* Return Value : none
******************************************************************************/
static void ipLinkChangeNotification(PLNKMON pLinkMon)
{
    while (pLinkMon)
    {
        eventSet(pLinkMon->pEvent);
        pLinkMon = pLinkMon->pNext;
    }
}
/******************************************************************************
End of function  ipLinkChangeNotification
*****************************************************************************/

/******************************************************************************
* Function Name: ipRead
* Description  : Function to allocate a packet buffer
* Arguments    : IN  stLength - The length of the buffer to allocate
* Return Value : Pointer to the allocated buffer
******************************************************************************/
static struct pbuf * ipAllocPacketBuffer(size_t stLength)
{
    struct pbuf *pPacket = NULL;

    while (!pPacket)
    {
        /* Allocate the pbuf to receive the data */
        pPacket = pbuf_alloc(PBUF_RAW, (u16_t) stLength, PBUF_RAM);

        if (pPacket)
        {
            break;
        }
        else
        {
            /* Failed to allocate a packet - try again */
            R_OS_Yield();
        }
    }

    return pPacket;
}
/******************************************************************************
End of function  ipAllocPacketBuffer
******************************************************************************/

/******************************************************************************
* Function Name: ipInputCallBack
* Description  : Function to put the received packet into lwIP via the call-back
*                mechanism. This is so the input timing is defined.
* Arguments    : IN  pPacket - Pointer to the input packet
* Return Value : none
******************************************************************************/
static void ipInputCallBack(struct pbuf *pPacket)
{
    PRTEIP pEtherC = (PRTEIP) pPacket->next;
    pPacket->next = NULL;
    pEtherC->ipNetIf.input(pPacket, &pEtherC->ipNetIf);
}
/******************************************************************************
End of function  ipInputCallBack
******************************************************************************/

/******************************************************************************
* Function Name: ipInputTask
* Description  : Task to read data from the ethernet driver and pass it to lwIP
* Arguments    : IN  pEtherC - Pointer to the ethernet controller data
* Return Value : none
******************************************************************************/
static void ipInputTask(PRTEIP pEtherC)
{
    R_OS_TaskUsesFloatingPoint();

    /* Enter the main function of the input task */
    while (true)
    {
        /* Allocate a buffer to hold the received data */
        struct pbuf *pPacket = ipAllocPacketBuffer(ETHERNET_INPUT_BUFFER_SIZE);

        /* Get a pointer to the payload */
        uint8_t     *pbyBuffer = pPacket->payload;
        int32_t     iResult;

        /* Padding is required from the start */
        pbyBuffer += ETH_PAD_SIZE;

        /* Read from the Ethernet controller */
        iResult = read(pEtherC->iEtherC, pbyBuffer, ETHERNET_INPUT_BUFFER_SIZE);

        /* Check the result */
        if (iResult > 0)
        {
            /* Put in the actual length of data delivered */
            pPacket->tot_len = (u16_t)(iResult + ETH_PAD_SIZE);

            /* Making segment length the same as the packet length tells lwIP
               that this is the end of the packet chain. This is slightly
               pointless since lwIP does not support packet queues */
            pPacket->len = (u16_t)(iResult + ETH_PAD_SIZE);
#ifdef _TRACE_RX_DATA_
            Trace("RX %d\r\n", iResult);
            dbgPrintBuffer(pbyBuffer, iResult);
#endif
            /* Put the packet into lwIP */
            pPacket->next = (struct pbuf *)pEtherC;
            tcpip_callback((void(*)(void*))ipInputCallBack, pPacket);
        }
        else
        {
            /* Error in the read, free the buffer and try again */
            pbuf_free(pPacket);
        }
    }
}
/******************************************************************************
End of function  ipInputTask
******************************************************************************/

/******************************************************************************
* Function Name: ipOutput
* Description  : This function is called by the ARP module when it wants
*                to send a packet on the interface. This function outputs
*                the pbuf as-is on the link medium.
* Arguments    : IN  pIpNetIf - Pointer to the network interface
*                IN  pPacket - Pointer to the lwIP pbuf structure
* Return Value : ERR_OK
******************************************************************************/
static err_t ipOutput(struct netif *pIpNetIf, struct pbuf *pPacket)
{
    PRTEIP      pEtherC = (PRTEIP) pIpNetIf->state;
    int         iEtherC = pEtherC->iEtherC;
    uint8_t     *pbyBuffer = pEtherC->pbyOuputBuffer;
    uint32_t    uiLength = 0;

    eventWaitMutex(&pEtherC->pevOutputBufferMutex, EV_WAIT_INFINITE);

    /* Check to see if the packet needs to be concatenated */
    if (pPacket->next)
    {
        struct pbuf *pGather = pPacket;
        uint8_t     *pbyGather = pbyBuffer;

        /* Gather the segments to make a frame */
        while (pGather)
        {
            /* Copy into the destination buffer */
            memcpy(pbyGather, pGather->payload, pGather->len);

            /* Add on the length */
            pbyGather += pGather->len;
            uiLength += pGather->len;

            /* Advance to the next one on the list */
            pGather = pGather->next;
        }

        /* Set the length */
        if (uiLength != pPacket->tot_len)
        {
            TRACE(("ipOutput: Error in gathered length %d (%d)", uiLength, pPacket->tot_len));
        }
    }
    else
    {
        pbyBuffer = (uint8_t *) pPacket->payload;

        /* Set the length of the data */
        uiLength = (uint32_t) pPacket->tot_len;
    }

    /* Padding is required from the start */
    pbyBuffer += ETH_PAD_SIZE;
    uiLength -= ETH_PAD_SIZE;

#ifdef _TRACE_TX_DATA_
    Trace("TX %d\r\n", uiLength);
    dbgPrintBuffer(pbyBuffer, uiLength);
#endif

    /* Write the data */
    write(iEtherC, pbyBuffer, uiLength);
    eventReleaseMutex(&pEtherC->pevOutputBufferMutex);
    return ERR_OK;
}
/******************************************************************************
End of function ipOutput
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
