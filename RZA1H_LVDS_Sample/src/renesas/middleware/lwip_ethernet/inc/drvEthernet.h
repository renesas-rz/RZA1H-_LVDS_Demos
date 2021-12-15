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
 * @headerfile     drvEthernet.h
 * @brief          Ethernet device driver using the proposed Ethernet Peripheral
 *                 Driver Library API (R_Ether_... functions)
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 30.06.2018 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef DRVETHERNET_H_INCLUDED
#define DRVETHERNET_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_ETHER_API lwIP Ethernet
 * @brief Ethernet device driver Interface using the Ethernet Peripheral
 *
 * @anchor R_SW_PKG_93_ETHERNET_DRV_SUMMARY
 * @par Summary
 *
 * This middleware module contains the functionality for the ethernet
 * driver to free and allocate memory. 
 * 
 * @anchor R_SW_PKG_93_ETHERNET_DRV_INSTANCES
 * @par Known Implementations:
 * This driver is used in the RZA1H Software Package.
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "r_devlink_wrapper.h"

/*****************************************************************************
Public Functions
******************************************************************************/
/**
 * @brief         Function to allocate memory aligned to a byte boundary
 *
 * @param[in]     stLength: The length of memory to allocate
 * @param[in]     iAlign:   The byte boundary to align to
 * @param[out]    ppvBase:  Pointer to the base memory pointer
 *
 * @retval        ptr:      Pointer to the aligned memory
 */
void *etMalloc(size_t stLength, uint32_t iAlign, void **ppvBase);

/**
 * @brief           Function to free the memory allocated
 * 
 * @param[in]       pvBase: base memory pointer
 * 
 * @return          None. 
 */
void etFree(void *pvBase);

/******************************************************************************
Variable Externs
******************************************************************************/
/**driver function table for this device */
extern  const st_r_driver_t gEtherCDriver;

#endif /* DRVETHERNET_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
