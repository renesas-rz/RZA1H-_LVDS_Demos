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
 * @headerfile     r_usb_cdc.h
 * @brief          Human Interface Device (HID) USB Class.
 *                 Supports an IN and an OUT HID Report.
 *                 NOTE: This module does not have any knowledge of the
 *                       contents of the reports.
 * @version        2.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 17.09.2009 1.00 First Release
 *              : 30.06.2018 2.00
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef USB_HID_H
#define USB_HID_H

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_USB_HID USB HID
 * @brief USB function HID hardware interface functions
 * 
 * @anchor R_SW_PKG_93_USB_CDC_SUMMARY
 * @par Summary
 * 
 * This module contain the interface to the USB Human Interface Device
 * module, allowing the device to work with keyboards, mice etc. 
 * 
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/***********************************************************************************
User Includes
***********************************************************************************/
/*    Following header file provides definition common to Upper and Low Level USB 
    driver. */
#include "usb_common.h"
/*    Following header file provides USB descriptor information. */
#include "r_usb_default_descriptors.h"
#include "r_usbf_core.h"

/***********************************************************************************
Type Definitions
***********************************************************************************/

/***********************************************************************************
Function Prototypes
***********************************************************************************/

/**
 * @brief               Initialises this module.
 * 
 * @param[in]           _report_in - A report to send to the host when it asks
 *                        for it.
 * @param[in]           _cb - A Callback function to be called when a OUT
 *                        report is received.
 * 
 * @retval               ERROR_CODE: USB Error Code.
*/
usb_err_t R_USB_HidInit(volatile st_usb_object_t *_pchannel,
                      uint8_t (*_report_in)[],
                      CB_REPORT_OUT _cb);

/**
 * @brief               Send IN Report to host.
 *                      Uses Interrupt IN.
 * 
 * @param[in]           _report_in: The report to send to the host.
 * 
 * @retval              ERROR_CODE: USB Error Code.
*/
usb_err_t R_USB_HidReportIn(volatile st_usb_object_t *_pchannel,
                          uint8_t (*_report_in)[]);

/**
 * @brief               Get the USB cable connected state.
 * 
 * @retval              TRUE:  Connected
 * @retval              FALSE: Disconnected.
*/
BOOL R_USB_HidIsConnected(volatile st_usb_object_t *_pchannel);

#endif /*USB_HID_H*/
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
