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
 * @headerfile     usbhEnum.h
 * @brief          Functions provided by the host stack for enumeration
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 05.08.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef USBHENUM_H_INCLUDED
#define USBHENUM_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_USB_HOST_ENUM USB ENUM
 * @brief USB Host driver hardware interface functions
 * 
 * @anchor R_SW_PKG_93_USB_HOST_API_SUMMARY
 * @par Summary
 * 
 * This module contais the interface for the USB Host Enumeration. 
 * 
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "compiler_settings.h"
#include "r_devlink_wrapper.h"

#include "ddusbh.h"

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Function to create device information struct
 * 
 * @param[in]     pUsbDevDesc:   Pointer to the USB Device descriptor 
 * @param[in]     transferSpeed: The device transfer speed
 * 
 * @retval        ptr_device: Pointer to the device 
 * @retval        NULL: On error
*/
extern  PUSBDI usbhCreateDeviceInformation(PUSBDD  pUsbDevDesc,
                                           USBTS   transferSpeed);

/**
 * @brief         Function to create a device for enumeration
 * 
 * @param[in]     transferSpeed: The required device transfer speed
 * 
 * @retval        ptr_device: Pointer to the device 
 * @retval        NULL: On error
*/
extern  PUSBDI usbhCreateEnumerationDeviceInformation(USBTS transferSpeed);

/**
 * @brief         Function to set the control pipe packet size
 * 
 * @param[in]     pDevice:     Pointer to the device to set
 * @param[in]     wPacketSize: The packet size
 * 
 * @return        none
*/
extern  void usbhSetEp0PacketSize(PUSBDI pDevice, uint16_t wPacketSize);

/**
 * @brief         Function to create the interface information
 * 
 * @param[in]     pDevice:    Pointer to the device
 * @param[in]     pUsbIfDesc: Pointer to the interface descriptor
 * 
 * @retval        true: if information created
*/
extern  _Bool usbhCreateInterfaceInformation(PUSBDI pDevice,
                                             PUSBIF pUsbIfDesc);

/**
 * @brief         Function to set the device interface information
 * 
 * @param[in]     pDevice:            Pointer to the device information to configure
 * @param[in]     pUsbCfgDesc:        Pointer to the configuration descriptor
 * @param[in]     byInterfaceNumber:  The interface number to use
 * @param[in]     byAlternateSetting: The alternate interface setting 
 * 
 * @retval        true: if the device information was configured successfully
*/
extern  _Bool usbhConfigureDeviceInformation(PUSBDI  pDevice,
                                             PUSBCG  pUsbCfgDesc,
                                             uint8_t byInterfaceNumber,
                                             uint8_t byAlternateSetting);

/**
 * @brief         Function to destroy the device
 * 
 * @param[in]     pDevice: Pointer to the device information to destroy
 * 
 * @return        none
*/
extern  void usbhDestroyDeviceInformation(PUSBDI pDevice);

/**
 * @brief         Function to create the hub information structure
 * 
 * @param[in]     pDevice:  Pointer to the device information
 * @param[in]     pHubDesc: Pointer to the hub descriptor
 * 
 * @retval        ptr_hub:Pointer to the hub information
 * @retval        NULL: on failure
*/
extern  PUSBHI usbhCreateHubInformation(PUSBDI pDevice, PUSBRH pHubDesc);

/**
 * @brief         Function to calculate the current draw of the devices attached
 *                to a hub
 * 
 * @param[in]     pHub: Pointer to the hub
 * 
 * @retval        current: The current draw in mA
*/
extern  int usbhCalculateCurrent(PUSBHI pHub);

/**
 * @brief         Function to get the first available device address
 * 
 * @param[in]     pUsbHc: Pointer to the host controller
 * 
 * @retval        dev_adr: The first available device address 
 * @retval       -1: If none available 
*/
extern  int8_t usbhGetDeviceAddress(PUSBHC pUsbHc);

/**
 * @brief         Function to get a pointer to the root port
 * 
 * @param[in]     iIndex: The index of the Host Controller
 * 
 * @retval        ptr_root: Pointer to the root pointer
 * @retval        NULL: if no more root ports available
*/
extern  PUSBPI usbhGetRootPort(int iIndex);

#ifdef __cplusplus
}
#endif

#endif                              /* USBHENUM_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
