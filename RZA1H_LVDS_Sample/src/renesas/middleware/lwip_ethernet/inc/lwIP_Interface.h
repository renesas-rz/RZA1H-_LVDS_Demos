/*******************************************************************************
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
 * and to discontinue the availability of this software. By using this
 * software, you agree to the additional terms and conditions found by
 * accessing the following link:
 * http://www.renesas.com/disclaimer
*******************************************************************************
* Copyright (C) 2018 Renesas Electronics Corporation. All rights reserved.
 *****************************************************************************/
/******************************************************************************
 * @headerfile     lwIP_Interface.h
 * @brief          Interface functions for lwIP
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 04.04.2011 1.00    First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef LWIP_INTERFACE_H
#define LWIP_INTERFACE_H

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_LWIP_IF lwIP Interface
 * @brief lwIP Module Interface 
 *
 * @anchor R_SW_PKG_93_ETHERNET_DRV_SUMMARY
 * @par Summary
 *
 * This middleware module contains the interface for the lwIP module
 * allowing the user to start, stop, configure etc. the lwIP. 
 * 
 * @anchor R_SW_PKG_93_ETHERNET_DRV_INSTANCES
 * @par Known Implementations:
 * This driver is used in the RZA1H Software Package.
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "r_event.h"
#include <stdio.h>
#include "nonVolatileData.h"

#ifndef _NET_MAX_RX_
#define _NET_MAX_RX_    20U
#endif

#ifndef _NET_MAX_TX_
#define _NET_MAX_TX_    20U
#endif

#define ETHERNET_SIMULTANEOUS_READS         _NET_MAX_RX_
#define ETHERNET_SIMULTANEOUS_WRITES        _NET_MAX_TX_


/******************************************************************************
Functions Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Function to start the Internet Protocol Suite
 * @param[in]     pLinkStatus:  Optional pointer to a file stream to receive
 *                              link status information
 * @param[in]     pszInterface: The symbolic link name of the interface
 * @param[in]     pMediaAccessControl: Pointer to the MAC address
 * @param[in]     pIpConfig: Pointer to the configuration
 *
 * @retval        0: For success
 * @retval       -1: On error
*/
extern  int32_t ipStart(FILE    *pLinkStatus,
                        char_t  *pszInterface,
                        uint8_t *pMediaAccessControl,
                        PNVDHCP pIpConfig);

/**
 * @brief         Function to stop lwIP
 * 
 * @return        None
*/
extern  void ipStop(void);

/**
 * @brief         Function to re-configure the interface
 * 
 * @param[in]     pszInterface: The symbolic link name of the interface
 * @param[in]     pIpConfig:    Pointer to the new configuration
 * 
 * @retval        0: For success
 * @retval       -1: On error
*/
extern  int32_t ipReconfigure(char_t *pszInterface, PNVDHCP pIpConfig);

/**
 * @brief         Function to get the current configuration
 * 
 * @param[in]     pszInterface: The symbolic link name of the interface
 * @param[out]    pIpConfig:    Pointer to the destination configuration
 * 
 * @retval        0: For success
 * @retval       -1: On error
*/
extern  int32_t ipGetConfiguration(char_t *pszInterface, PNVDHCP pIpConfig);

/**
 * @brief         Function to add an event to the link monitor list
 *
 * @param[in]     pEvent: Pointer to the event to add to the link monitor
 *                        list
 * 
 * @retval        True: if the event was added to the lists
*/
extern  _Bool ipAddLinkMonitorEvent(PEVENT pEvent);

/**
 * @brief         Function to remove an event from the link monitor list
 *
 * @param[in]     pEventL: Pointer to the event to remove from the link
 *                         monitor list
 * @return        None.
*/
extern  void ipRemoveLinkMonitorEvent(PEVENT pEvent);

/**
 * @brief         Function to return true if there is an IP data link available
 * 
 * @retval        True: if there is a link
*/
extern  _Bool ipLink(void);

/**
 * @brief         Function to display ipLinkStatus
 * 
 * @param[in]     pLinkStatus: Pointer to a file stream to print the link
 *                             status to
 * @return        None.
*/
extern  void ipLinkStatusDisplay(FILE *pLinkStatus);

/**
 * @brief         Function to set the network adapter into promiscuous mode
 * 
 * @param[in]     pszInterface:  Pointer to the interface to set
 * @param[in]     bfPromiscuous: true for promiscuous mode
 * 
 * @retval        0: For success
 * @retval       -1: On error
*/
extern int32_t ipSetPromiscuousMode(char_t *pszInterface, _Bool bfPromiscuous);

/**
 * @brief         Function to get the adapter promiscuous mode
 *
 * @param[in]     pszInterface: Pointer to the interface to get
 * 
 * @retval        True: for promiscuous
 * @retval        False: for everything else
*/
extern  _Bool ipGetPromiscuousMode(char_t *pszInterface);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_INTERFACE_H */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
