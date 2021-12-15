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
* File Name    : lwipStart.c
* Version      : 1.00
* Description  : lwIP internet protocol suit configuration
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
#include "lwipStart.h"
#include "lwip/netifapi.h"

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
Typedef definitions
******************************************************************************/

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/

static const uint8_t gpbyNullMac[] =
{
    0U,0U,0U,0U,0U,0U
};

static const uint8_t gpbyBroadcastMac[] =
{
    0xFFU,0xFFU,0xFFU,0xFFU,0xFFU,0xFFU
};

/******************************************************************************
Public functions
******************************************************************************/

/******************************************************************************
* Function Name: ipStartOnChipController
* Description  : Function to start lwIP using the on-chip ethernet controller
* Arguments    : IN  pLinkStatus - Pointer to a file stream for link status
* Return Value : 0 for success or -1 on error
******************************************************************************/
int32_t ipStartOnChipController(FILE *pLinkStatus)
{
    FILE    *pMac;

    /* Open the file with the MAC address in it */
    pMac = fopen("\\\\.\\eeprom", "r");
    if (pMac)
    {
        uint8_t         pbyMAC[NETIF_MAX_HWADDR_LEN];
        NVDHCP          ipConfig;

        /* The MAC address is kept in the on-chip data FLASH.
           For compatibility with other network platforms it is offset by one
           byte from the start of the file. If this byte is 0xA5 then the
           MAC address has been set. The leading byte is not checked */
        fseek(pMac, 1L, SEEK_SET);

        /* Put the MAC address in the net interface data structure */
        fread(pbyMAC, 1UL, NETIF_MAX_HWADDR_LEN, pMac);
        TRACE(("ipStartOnChipController: MAC: "
               "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X\r\n",
               pbyMAC[0U], pbyMAC[1U], pbyMAC[2U], pbyMAC[3U],
               pbyMAC[4U], pbyMAC[5U]));
        fclose(pMac);

        /* Do a quick check for a valid MAC */
        if ((!memcmp(pbyMAC, gpbyNullMac,
                     sizeof(gpbyNullMac)))
        ||  (!memcmp(pbyMAC, gpbyBroadcastMac,
                     sizeof(gpbyBroadcastMac))))
        {
#ifndef _DEBUG_
            TRACE(("lwipStart: INVALID MAC ADDRESS\r\n"));
            return -1;
#else
            static const uint8_t pbyDefaultMac[NETIF_MAX_HWADDR_LEN]
                = {0x00, 0x00, 0x87, 0x37, 0xCA, 0xAF};
            /* Be nice and use a default MAC */
            memcpy(pbyMAC, pbyDefaultMac, NETIF_MAX_HWADDR_LEN);
#endif
        }
        /* Load the user's network settings */
        if (nvLoad(NVDT_DHCP_SETTINGS_V1, &ipConfig, sizeof(NVDHCP)))
        {
            /* If the load fails then there are no user defined settings,
               set the default settings - defined in nonVolatileData.h */
            ipConfig = gDefaultIpConfigV1;
        }
        /* Start lwip using this interface adapter */
        return ipStart(pLinkStatus,
                       "\\\\.\\ether0",
                       pbyMAC,
                       &ipConfig);
    }
    return -1;
}
/******************************************************************************
End of function ipStartOnChipController
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
