/******************************************************************************
* DISCLAIMER
* Please refer to http://www.renesas.com/disclaimer
******************************************************************************
  Copyright (C) 2011. Renesas Technology Corp., All Rights Reserved.
*******************************************************************************
* File Name    : ipCache.h
* Version      : 1.00
* Description  : IPV4 address cache functions
*******************************************************************************
* History      : DD.MM.YYYY Version Description
*              : 04.04.2011 1.00    First Release
******************************************************************************/

#ifndef IPCACHE_FILE_H
#define IPCACHE_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "r_typedefs.h"

/******************************************************************************
Typedef definitions
******************************************************************************/

typedef uint32_t IPADR, *PIPADR;
typedef struct _IPCACHE *PIPCACHE;

/******************************************************************************
Functions Prototypes
******************************************************************************/

/******************************************************************************
* Function Name: icCreate
* Description  : Function to create an IP address cache
* Arguments    : IN  uiCacheSize - The number of entries to keep in the cache
* Return Value : Pointer to the cache or NULL on error
******************************************************************************/

extern  PIPCACHE icCreate(uint32_t uiCacheSize);

/******************************************************************************
* Function Name: icDestroy
* Description  : Function to destroy an IP address cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
* Return Value : none
******************************************************************************/

extern  void icDestroy(PIPCACHE pIpCache);

/******************************************************************************
* Function Name: icAdd
* Description  : Function to add an address to the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
*                IN  pIP - Pointer to the IP address to add
* Return Value : none
******************************************************************************/

extern  void icAdd(PIPCACHE pIpCache, PIPADR pIP);

/******************************************************************************
* Function Name: icRemove
* Description  : Function to remove an address from the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
*                IN  pIP - Pointer to the IP address to remove
* Return Value : none
******************************************************************************/

extern  void icRemove(PIPCACHE pIpCache, PIPADR pIP);

/******************************************************************************
* Function Name: icPurge
* Description  : Function to purge the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
* Return Value : none
******************************************************************************/

extern  void icPurge(PIPCACHE pIpCache);

/******************************************************************************
* Function Name: icSearch
* Description  : Function to search the cache
* Arguments    : IN  pIpCache - Pointer to the IP address cache
*                IN  pIP - Pointer to the IP address to search
* Return Value : true if the entry is found
******************************************************************************/

extern _Bool icSearch(PIPCACHE pIpCache, PIPADR pIP);

#ifdef __cplusplus
}
#endif

#endif /* IPCACHE_FILE_H */

/******************************************************************************
End of file
******************************************************************************/
