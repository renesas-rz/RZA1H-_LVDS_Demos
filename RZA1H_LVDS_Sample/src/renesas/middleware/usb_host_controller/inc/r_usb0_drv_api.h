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
 * @headerfile     r_usb0_drv_api.h
 * @brief          USB Port 0 Host driver hardware interface
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 05.08.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef R_USB0_DRV_AP_H_INCLUDED
#define R_USB0_DRV_AP_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_USB_HOST_API USB HOST API
 * @brief USB Host driver hardware interface functions
 * 
 * @anchor R_SW_PKG_93_USB_HOST_API_SUMMARY
 * @par Summary
 * 
 * This module contais the interface for the USB Host driver hardware 
 * interface, with a POSIX style API: open, close, read, write, control. 
 * 
 * Additionally, the module denotes the API used to access the Host Hardware
 * Peripheral functionality. 
 * 
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "r_devlink_wrapper.h"

/******************************************************************************
Constant Data
******************************************************************************/

/** 
 * @var g_usb0_host_driver 
 * Table Includes:<BR>
 * "USB Host Port 0 Device Driver" - Driver Name <BR>
 * 
 * usb_host_open - Opens the Host Driver <BR>
 * 
 * usb_host_close - Closes the Host Driver <BR>
 * 
 * usb_host_no_io - Reading not supported <BR>
 * 
 * usb_host_no_io - Writing not supported <BR>
 * 
 * usb_host_control - <BR>
 * Not Used <BR>
 * 
 * no_dev_get_version - GetVersion not supported
 */
extern const st_r_driver_t g_usb0_host_driver;

#endif /* R_USB0_DRV_AP_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
