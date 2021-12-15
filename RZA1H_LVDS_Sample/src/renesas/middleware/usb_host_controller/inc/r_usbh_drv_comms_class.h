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
 * @headerfile     r_usbh_drv_comms_class.h
 * @brief          USB Comms Class
 * @version        1.10
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 05.08.2010 1.00    First Release
 *              : 27.01.2016 1.10    Updated for GSCE compliance
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef DRVCOMMSCLASS_H_INCLUDED
#define DRVCOMMSCLASS_H_INCLUDED


/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_USB_COMMS USB COMMS Class
 * @brief USB Comms Class API and configuration
 * 
 * @anchor R_SW_PKG_93_USB_HOST_API_SUMMARY
 * @par Summary
 * 
 * This module contains all configuration, data structures and API related
 * to the USB Comms class.
 * 
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Macro definitions
******************************************************************************/


/******************************************************************************
Typedef definitions
******************************************************************************/
/* The data interfaces */
typedef struct
{
    uint8_t by_interface;
    PUSBEI  p_cdc_out_endpoint;
    PUSBEI  p_cdc_in_endpoint;
} cdc_data_interfaces_t;

typedef struct
{
    uint8_t by_interface;
    PUSBEI  p_cdc_int_in_endpoint;
} cdc_int_interface_t;

typedef struct _CDCCONNECTION
{
    /** Pointer to the device information */
    PUSBDI  pdevice;
    cdc_data_interfaces_t data;
    cdc_int_interface_t ctl;
} cdc_connection_t;

typedef cdc_connection_t* pcdc_connection_t;

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Function to select the desired interface and start the
 * 
 * @param[in]       pCDCInfo:       Pointer to the interface information
 * @param[out]      ppdeviceDriver: Pointer to the destination driver
 * 
 * @retval        true: if the driver will work with the device
*/
extern _Bool R_USBH_CDCLoadDriver (pcdc_connection_t p_cdc_info, st_r_driver_t** ppdeviceDriver);

/**
 * @brief         Function to check the config descriptor and get hold of the
 *                data class interface descriptor
 * 
 * @param[in]     pdevice:   Pointer to the device information
 * @param[in]     pbyConfig: Pointer to the configuration descriptor
 * 
 * @retval        True:      If the data class interface was found and the
 *                           data endpoints found
*/
extern _Bool R_USBH_LoadCommsDeviceClass (PUSBDI pdevice, uint8_t *pby_config);

#ifdef __cplusplus
}
#endif

#endif /* COMMSCLASS_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
