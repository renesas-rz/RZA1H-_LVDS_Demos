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
 * @brief          USB CDC Class (Abstract Class Model).
 *                 Provides a virtual COM port
 * @version        2.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 12.11.2009 1.00 First Release
 *              : 30.06.2018 2.00 
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef USB_CDC_H
#define USB_CDC_H

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_USB_CDC USB CDC
 * @brief USB function CDC hardware interface functions
 * 
 * @anchor R_SW_PKG_93_USB_CDC_SUMMARY
 * @par Summary
 * 
 * This module contain the interface to the USB CDC commands
 * allowing the device to act as a virtual com port, for a device to connect 
 * to and write/receive data. 
 * 
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
User Includes
******************************************************************************/
/*    Following header file provides definition common to Upper and Low Level USB 
    driver. */
#include "usb_common.h"
#include "r_usbf_core.h"

/******************************************************************************
Function Prototypes
******************************************************************************/

/**
 * @brief              Initialise this module.
 *                     This must be called once before using any of the
 *                     other API functions.
 *                     Initialise the USB Core layer.
 * 
 * @param[in]          _pchannel: current peripheral
 *                                As code is reentrant functions should not
 *                                reference local variables
 * 
 * @retval             USB_ERR_OK: If Successful 
*/
usb_err_t R_USB_CdcInit(volatile st_usb_object_t *_pchannel);

/**
 * @brief               Get the USB cable connected state.
 * 
 * @retval              TRUE:  Connected
 * @retval              FALSE: Disconnected.
*/
BOOL R_USB_CdcIsConnected(volatile st_usb_object_t *_pchannel);

/**
 * @brief              Write 1 character (BULK IN).
 *                     Note: As this is a blocking function it must not be
 *                     called from an interrupt with higher priority than
 *                     the USB interrupts.
 *
 * @param[in]          _pchannel: current peripheral
 * @param[in]          _char: Byte to write.
 * 
 * @retval             USB_ER_CODE: Error code.
*/
usb_err_t R_USB_CdcPutChar(volatile st_usb_object_t *_pchannel,uint8_t _char);

/**
 * @brief              Read 1 character (BULK OUT).
 *                     Note: As this is a blocking function it must not be
 *                     called from an interrupt with higher priority than
 *                     the USB interrupts.
 * 
 * @param[out]         _pChar: Pointer to Byte to read into.
 * 
 * @retval             USB_ER_CODE: Error code.
*/
usb_err_t R_USB_CdcGetChar(volatile st_usb_object_t *_pchannel, uint8_t* _pChar);

/**
 * @brief              Perform a blocking write (BULK IN)
 *                     Note: As this is a blocking function it must not be
 *                     called from an interrupt with higher priority than
 *                     the USB interrupts.
 *
 * @param[in]          _pchannel: current peripheral
 * @param[in]          _num_bytes: Number of bytes to write.
 * @param[in]          _pbuffer:   Data Buffer.
 * 
 * @retval             USB_ER_CODE: Error code.
*/
usb_err_t R_USB_CdcWrite(volatile st_usb_object_t *_pchannel, uint32_t _num_bytes, uint8_t* _pbuffer);

/**
 * @brief               Start an asynchronous write. (BULK IN)
 * 
 * @param[in]          _num_bytes: Number of bytes to write.
 * @param[in]          _pbuffer:   Data Buffer.
 * @param[in]          _cb:        Callback when done.
 * 
 * @retval             USB_ER_CODE: Error code.
*/
usb_err_t R_USB_CdcWriteAsync(volatile st_usb_object_t *_pchannel, uint32_t _num_bytes, uint8_t* _pbuffer, CB_DONE _cb);

/**
 * @brief              Perform a blocking read. (BULK OUT)
 *                     Note: As this is a blocking function it must not be
 *                     called from an interrupt with higher priority than
 *                     the USB interrupts.
 *
 * @param[in]          _pbufferSize:    Buffer Size
 * @param[out]         _pbuffer:        Buffer to read data into.
 * @param[out]         _pNumBytesRead:  Number of bytes read.
 *  
 * @retval             USB_ER_CODE: Error code.
*/
usb_err_t R_USB_CdcRead(volatile st_usb_object_t *_pchannel, uint32_t _pbufferSize, uint8_t* _pbuffer, uint32_t* _pNumBytesRead);

/**
 * @brief              Start an asynchronous read. (BULK OUT)
 * 
 * @param[in]          _pbufferSize: Buffer Size
 * @param[out]         _pbuffer:     Buffer to read data into.
 * @param[in]          _cb:          Callback when done.
 *
 * @retval             USB_ER_CODE: Error code.
*/
usb_err_t R_USB_CdcReadAsync(volatile st_usb_object_t *_pchannel, uint32_t _pbufferSize, uint8_t* _pbuffer, CB_DONE_OUT _cb);

/**
 * @brief              Cancel waiting on any blocking functions. 
 * 
 * @retval             USB_ER_CODE: Error code.
*/
usb_err_t R_USB_CdcCancel(volatile st_usb_object_t * _pchannel);

#endif /*USB_CDC_H*/
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
