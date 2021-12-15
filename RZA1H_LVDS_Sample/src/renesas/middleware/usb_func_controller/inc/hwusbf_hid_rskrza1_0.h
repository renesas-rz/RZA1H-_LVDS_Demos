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
 * @headerfile     hwusbf_cdc_rskrza1_0.h
 * @brief          USB Port 0 HID driver hardware interface
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 05.08.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef HWUSB1FHID_H_INCLUDED
#define HWUSB1FHID_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_USB_HID 
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
 * @var g_usbf0_hid_driver 
 * Table Includes:<BR>
 * "USB Func HID Port 0 Device Driver" - Driver Name <BR>
 * 
 * usbf_hid_open - Opens the CDC Driver <BR>
 * 
 * usbf_hid_close - Closes the CDC Driver <BR>
 * 
 * usbf_hid_read - Reads from the CDC Driver <BR>
 * 
 * usbf_hid_write - Writes to the CDC Driver <BR>
 * 
 * usbf_hid_control - <BR>
 * CTL_USBF_IS_CONNECTED: Returns Connection status of CDC <BR>
 * CTL_USBF_GET_CONFIGURATION: Gets the current HID Configuration <BR>
 * CTL_USBF_SET_CONFIGURATION: Sets the HID Configuration <BR> 
 * CTL_USBF_SEND_HID_REPORTIN: Sends Report In <BR> 
 * CTL_USBF_START: Starts the CDC Driver <BR>
 * CTL_USBF_STOP: Stops the CDC Driver <BR>
 * 
 * no_dev_get_version - GetVersion not supported
 */
extern const st_r_driver_t g_usbf0_hid_driver;

#endif /* HWUSB1FHID_H_INCLUDED*/
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
