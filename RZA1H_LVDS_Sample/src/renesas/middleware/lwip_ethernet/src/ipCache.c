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
* File Name    : ipCache.c
* Version      : 1.00
* Description  : IPV4 address cache functions
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

#include <stdlib.h>
#include <string.h>
#include "ipCache.h"
#include "compiler_settings.h"

/******************************************************************************
Typedefs
******************************************************************************/

typedef struct _IPLST
{
    /* The Count for Least Recently Used replacement */
    uint32_t    uiLRU;

    /* The IP address, 0 for invalid */
    IPADR       ipAddress;
} IPLST,
*PIPLST;

typedef struct _IPCACHE
{
    /* The total number of entries */
    uint32_t    uiCacheSize;

    /* The array of IP entries */
    IPLST       ipList;
} IPCACHE;

/******************************************************************************
Private Function Prototypes
******************************************************************************/

static PIPLST ipFind(PIPCACHE pIpCache, PIPADR pIP);
static PIPLST ipGetEntry(PIPCACHE pIpCache);

/******************************************************************************
Public Functions
******************************************************************************/

/******************************************************************************
* Function Name: icCreate
* Description  : Function to create an IP address cache
* Arguments    : IN  uiCacheSize - The number of entries to keep in the cache
* Return Value : Pointer to the cache or NULL on error
******************************************************************************/
PIPCACHE icCreate(uint32_t uiCacheSize)
{
    size_t      stSize = (uiCacheSize * sizeof(IPLST)) + sizeof(IPCACHE);
    PIPCACHE    pIpCache = R_OS_AllocMem(stSize, R_REGION_LARGE_CAPACITY_RAM);

    if (pIpCache)
    {
        memset(pIpCache, 0U, stSize);
        pIpCache->uiCacheSize = uiCacheSize;
    }

    return pIpCache;
}
/******************************************************************************
End of function icCreate
******************************************************************************/

/******************************************************************************
* Function Name: icDestroy
* Description  : Function to destroy an IP address cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
* Return Value : none
******************************************************************************/
void icDestroy(PIPCACHE pIpCache)
{
    R_OS_FreeMem(pIpCache);
}
/******************************************************************************
End of function icDestroy
******************************************************************************/

/******************************************************************************
* Function Name: icAdd
* Description  : Function to add an address to the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
*                IN  pIP - Pointer to the IP address to add
* Return Value : none
******************************************************************************/
void icAdd(PIPCACHE pIpCache, PIPADR pIP)
{
    if (pIpCache)
    {
        /* Look to see if it is in the list */
        PIPLST  pIpEntry = ipFind(pIpCache, pIP);

        if (!pIpEntry)
        {
            /* Get a new entry */
            pIpEntry = ipGetEntry(pIpCache);

            /* Add the IP address to the cache entry */
            pIpEntry->ipAddress = *pIP;
            pIpEntry->uiLRU = 0U;
        }
    }
}
/******************************************************************************
End of function icAdd
******************************************************************************/

/******************************************************************************
* Function Name: icRemove
* Description  : Function to remove an address from the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
*                IN  pIP - Pointer to the IP address to remove
* Return Value : none
******************************************************************************/
void icRemove(PIPCACHE pIpCache, PIPADR pIP)
{
    if (pIpCache)
    {
        PIPLST  pIpEntry = &pIpCache->ipList;
        PIPLST  pIpEnd = pIpEntry + pIpCache->uiCacheSize;
        IPADR   ipAddress = *pIP;

        while (pIpEntry < pIpEnd)
        {
            if (pIpEntry->ipAddress == ipAddress)
            {
                pIpEntry->ipAddress = 0U;
                return;
            }

            pIpEntry++;
        }
    }
}
/******************************************************************************
End of function icRemove
******************************************************************************/

/******************************************************************************
* Function Name: icPurge
* Description  : Function to purge the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
* Return Value : none
******************************************************************************/
void icPurge(PIPCACHE pIpCache)
{
    if (pIpCache)
    {
        PIPLST  pIpEntry = &pIpCache->ipList;
        PIPLST  pIpEnd = pIpEntry + pIpCache->uiCacheSize;

        while (pIpEntry < pIpEnd)
        {
            pIpEntry->ipAddress = 0U;
            pIpEntry++;
        }
    }
}
/******************************************************************************
End of function icPurge
******************************************************************************/

/******************************************************************************
* Function Name: icSearch
* Description  : Function to search the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
*                IN  pIP - Pointer to the IP address to search
* Return Value : true if the entry is found
******************************************************************************/
_Bool icSearch(PIPCACHE pIpCache, PIPADR pIP)
{
    if (pIpCache)
    {
        PIPLST  pIpEntry = &pIpCache->ipList;
        PIPLST  pIpEnd = pIpEntry + pIpCache->uiCacheSize;
        IPADR   ipAddress = *pIP;

        while (pIpEntry < pIpEnd)
        {
            if (pIpEntry->ipAddress == ipAddress)
            {
                return true;
            }
            pIpEntry++;
        }
    }

    return false;
}
/******************************************************************************
End of function icSearch
******************************************************************************/

/******************************************************************************
Private Functions
******************************************************************************/

/******************************************************************************
* Function Name: ipFind
* Description  : Function to find an entry in the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
*                IN  pIP - Pointer to the IP address to search
* Return Value : Pointer to the entry or NULL if not found
******************************************************************************/
static PIPLST ipFind(PIPCACHE pIpCache, PIPADR pIP)
{
    PIPLST  pIpEntry = &pIpCache->ipList;
    PIPLST  pIpEnd = pIpEntry + pIpCache->uiCacheSize;
    IPADR   ipAddress = *pIP;

    while (pIpEntry < pIpEnd)
    {
        if (pIpEntry->ipAddress == ipAddress)
        {
            pIpEntry->uiLRU = 0U;
            return pIpEntry;
        }

        pIpEntry->uiLRU++;
        pIpEntry++;
    }

    return NULL;
}
/******************************************************************************
End of function ipFind
******************************************************************************/

/******************************************************************************
* Function Name: ipGetEntry
* Description  : Function to find an entry in the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
* Return Value : Pointer to the entry
******************************************************************************/
static PIPLST ipGetEntry(PIPCACHE pIpCache)
{
    PIPLST      pIpEntry = &pIpCache->ipList;
    PIPLST      pResult = pIpEntry;
    PIPLST      pIpEnd = pIpEntry + pIpCache->uiCacheSize;
    uint32_t    uiUsedCount = 0U;

    while (pIpEntry < pIpEnd)
    {
        /* Use the first invalid entry found */
        if (!pIpEntry->ipAddress)
        {
            pResult = pIpEntry;
            break;
        }

        /* Select from the least recently used */
        if (pIpEntry->uiLRU > uiUsedCount)
        {
            pIpEntry->uiLRU = uiUsedCount;
            pResult = pIpEntry;
        }

        pIpEntry++;
    }

    return pResult;
}
/******************************************************************************
End of function ipGetEntry
******************************************************************************/

/******************************************************************************
End of File
******************************************************************************/
