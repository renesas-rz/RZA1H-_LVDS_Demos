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
 * @headerfile     usbhConfig.h
 * @brief          Usb host stack configuration defines
 * @version        2.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 05.08.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef USBHCONFIG_H_INCLUDED
#define USBHCONFIG_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_USB_HOST_API
 * @defgroup R_SW_PKG_93_USB_HOST_CONFIG USB Host Config
 * @brief Usb host stack configuration defines
 * 
 * @anchor R_SW_PKG_93_USB_CDC_SUMMARY
 * @par Summary
 * 
 * This configuration module contains all the settings related to the setup
 * and configuration of the USB Host driver. 
 * 
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Macro definitions
******************************************************************************/

/** Define the USB Host controller hardware structure tag here */
#define USBH_HOST_STRUCT_TAG        st_usb20

/** Define the number of root ports 1 or 2 on each host controller */
#define USBH_NUM_ROOT_PORTS         2

/** Define high speed support 1 for SH / ARM on chip peripherals. */
#define USBH_HIGH_SPEED_SUPPORT     1

/** The PIPE0 access delay count for the high speed version */
#define USBH_PIPE_0_ACCESS_DELAY    2000U

/** Define the FIFO access size only 32 and 16 are valid here.
   Usually 32 for SH on chip peripherals.
   Currently 16 for RX on chip peripherals */
#define USBH_FIFO_BIT_WIDTH         32

/** The maximum number of host controllers supported */
#define USBH_MAX_CONTROLLERS        2

/** The number of tiers of hubs supported */
#define USBH_MAX_TIER               1

/** The maximum number of hubs that can be connected */
#define USBH_MAX_HUBS               2

/** The maximum number of ports */
#define USBH_MAX_PORTS              10

/** The maximum number of devices */
#define USBH_MAX_DEVICES            10

#if USBH_HIGH_SPEED_SUPPORT == 1
#define USBH_MAX_ADDRESS            10
#else
#define USBH_MAX_ADDRESS            5
#endif

/** The maximum number of endpoints */
#define USBH_MAX_ENDPOINTS          64


/** The source provided here only supports the High Speed USB peripheral,
 * the define remains for legacy reasons but throw an error if Full Speed
 * USB peripheral is selected.
 */
#ifndef USBH_HIGH_SPEED_SUPPORT
    #error "Support for the Full Speed USB Peripheral is not included."
#endif

#endif                              /* USBHCONFIG_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
