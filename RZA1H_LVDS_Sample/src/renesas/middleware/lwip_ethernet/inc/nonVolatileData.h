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
* File Name    : nonVolatileData.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : None
* H/W Platform : RSK+
* Description  : Functions to store and recall data from non-volatile memory
*******************************************************************************
* History      : DD.MM.YYYY Version Description
*              : 05.08.2010 1.00    First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

#ifndef NONVOLATILEDATA_H_INCLUDED
#define NONVOLATILEDATA_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "r_typedefs.h"

/*****************************************************************************
 Typedef definitions
 ******************************************************************************/

/* Enumerate the types of data stored in the non volatile memory */
typedef enum _NVDT
{
    NVDT_RESEVED = 0,
    NVDT_DHCP_SETTINGS_V1,
    NVDT_TEST_IP_ADDR_AND_PORT_V1,
    NVDT_LOGINS_AND_PASSWORDS_V1,

    /* TODO: Add other non volatile settings here.
       The laws of compatibility:
       The enumerated types should be linked to the data structures by the
       programmer. Never remove old types and always add to the end of the
       list even if it could do with tidying. Always append the version to
       the end of the type label.
     */

} NVDT,
*PNVDT;

/*****************************************************************************
Typedefs
******************************************************************************/

/* Define the data structure for the NVDT_DHCP_SETTINGS_V1 data type */
typedef struct _NVDHCP
{
    uint8_t         pbyIpAddress[4];
    uint8_t         pbyAddressMask[4];
    uint8_t         pbyGatewayAddress[4];
    uint8_t         byEnableDHCP;

} NVDHCP,
*PNVDHCP;

/* Define the data structure for the NVDT_TEST_IP_ADDR_AND_PORT_V1 data type */
typedef struct _NVIP
{
    uint8_t         pbyIpAddress[4];
    uint16_t        usPortNumber;

} NVIP,
*PNVIP;

/* Define the data structure for the NVDT_LOGINS_AND_PASSWORDS_V1 data type */
typedef struct _NVUSERS
{
    /* A string of the user name and password delimited by the chars : & |
       "admin:password|daemon:time|root:superuser" */
    int8_t          *pszList;
} NVUSERS;

/* TODO: Add other non volatile structures here
   The laws of compatibility:
   If you need to change a structure don't edit it! Take a copy of the
   structure, change the tag and the type, then edit. Add a new enumerated type
   associated with the modified data structure.
   Remind yourself of the reasons with a comment and append the version to
   the end of the type label.
*/

/******************************************************************************
 Constant Data
 ******************************************************************************/

static const NVDHCP gDefaultIpConfigV1 =
{
     {
          /* IP address */
          192, 168, 1, 10,
     },

	 {
          /* Subnet mask */
          255, 255, 255, 0,
     },

	 {
          /* Gateway */
          192, 168, 1, 1,
     },

	 /* DHCP Enabled (1) / Disabled (0) */
     1
};

/* TODO: Add other default types here */

/*****************************************************************************
Public Functions
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
Function Name: nvLoad
Description:   Function to load settings from non volatile memory
Arguments:     IN  nvDataType - The data type to load
               OUT pvDest - Pointer to the destination
               IN  stLength - The length of the destination memory
Return value:  0 for success -1 on error
*****************************************************************************/

extern  int nvLoad(NVDT nvDataType, void *pvDest, size_t stLength);

/*****************************************************************************
Function Name: nvStore
Description:   Function to store settings to non volatile memory
Arguments:     IN  nvDataType - The data type to load
               IN  pvSrc - Pointer to the data structure to store
               IN  stLength - The length of the destination memory
Return value:  0 for success -1 on error
*****************************************************************************/

extern  int nvStore(NVDT nvDataType, const void *pvSrc, size_t stLength);

#ifdef __cplusplus
}
#endif

#endif /* NONVOLATILEDATA_H_INCLUDED */

/******************************************************************************
 End  Of File
 ******************************************************************************/
