/******************************************************************************
* DISCLAIMER                                                                      
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized.
* This software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES
* REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY,
* INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE EXPRESSLY
* DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
* FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS
* AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this
* software and to discontinue the availability of this software.  
* By using this software, you agree to the additional terms and
* conditions found by accessing the following link:
* http://www.renesas.com/disclaimer
******************************************************************************/
/* Copyright (C) 2010 Renesas Electronics Corporation. All rights reserved.*/

/******************************************************************************
* File Name       : usb_hal.h
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      : HEW, Renesas SuperH Standard Tool chain v9.3
* H/W Platform    : RZA1H
* Description     : Hardware Abstraction Layer (HAL)
* 
*                   Provides a hardware independent API to the USB peripheral
*                   on the SKSH7269.
*                   Supports:-
*                   Control IN, Control OUT, Bulk IN, Bulk OUT, Interrupt IN.
*
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

#ifndef USB_HAL_H_INCLUDED
#define USB_HAL_H_INCLUDED

/******************************************************************************
User Includes (Project Level Includes)
******************************************************************************/
/*    Following header file provides structure and prototype definition of USB 
    API's. */
#include "usb.h"

#include "r_usbf_core.h"

/******************************************************************************
Macro Defines
******************************************************************************/
#define USB_HID_INTERRUPT_PROIRITY (0x03)

/*HW defined packet sizes*/
#define CONTROL_IN_PACKET_SIZE      (64)
#define CONTROL_OUT_PACKET_SIZE     (64)

#define BULK_IN_PACKET_SIZE         (512)
#define BULK_OUT_PACKET_SIZE        (512)

#define INTERRUPT_IN_PACKET_SIZE    (64)

/******************************************************************************
Type Definitions
******************************************************************************/
/*State of USB device*/
typedef enum 
{
    STATE_POWERED,
    STATE_DEFAULT,
    STATE_ADDRESSED,
    STATE_CONFIGURED,
    STATE_SUSPENDED
} e_usbf_state_device_t;

/******************************************************************************
Function Prototypes
******************************************************************************/
usb_err_t R_USB_HalInit(volatile st_usb_object_t *_pchannel,
                    CB_SETUP _p_cb_setup,
                    CB_CABLE _p_cb_cable,
                    CB_ERROR _p_cb_error);

usb_err_t R_USB_HalClose(volatile st_usb_object_t *_pchannel);

/*Configuration*/
const st_usb_hal_config_t* R_USB_HalConfigGet(volatile st_usb_object_t *_pchannel);

usb_err_t R_USB_HalConfigSet(volatile st_usb_object_t *_pchannel,
                            st_usb_hal_config_t* _pConfig);
                     
/*Control*/                     
usb_err_t R_USB_HalControlAck(volatile st_usb_object_t *_pchannel);

usb_err_t R_USB_HalControlIn(volatile st_usb_object_t *_pchannel,
                            uint16_t _num_bytes,
                            const uint8_t* _pbuffer);

usb_err_t R_USB_HalControlOut(volatile st_usb_object_t *_pchannel,
                             uint16_t _num_bytes,
                             uint8_t* _pbuffer,
                             CB_DONE_OUT _CBDone);

/*Bulk*/
usb_err_t R_USB_HalBulkIn(volatile st_usb_object_t *_pchannel,
                         uint32_t _num_bytes,
                         const uint8_t* _pbuffer,
                         CB_DONE_BULK_IN _CBDone);

usb_err_t R_USB_HalBulkOut(volatile st_usb_object_t *_pchannel,
                          uint32_t _num_bytes,
                          uint8_t* _pbuffer,
                          CB_DONE_OUT _CBDone);
    
/*Interrupt*/                     
usb_err_t R_USB_HalInterruptIn(volatile st_usb_object_t *_pchannel,
                              uint32_t _num_bytes,
                              const uint8_t* _pbuffer,
                              CB_DONE _CBDone);

/*Cancel all pending operations and call callbacks*/
usb_err_t R_USB_HalCancel(volatile st_usb_object_t *_pchannel);

/*Reset module - callbacks not called*/
usb_err_t R_USB_HalReset(volatile st_usb_object_t *_pchannel);

/*Stall*/
void R_USB_HalControlStall(volatile st_usb_object_t *_pchannel);
void R_USB_HalBulkInStall(volatile st_usb_object_t *_pchannel);
void R_USB_HalBulkOutStall(volatile st_usb_object_t *_pchannel);
void R_USB_HalInterruptInStall(volatile st_usb_object_t *_pchannel);

/*get device state*/
usb_err_t R_USB_HalGetDeviceState(volatile st_usb_object_t *_pchannel, uint8_t* state);

/* Get End Point Stalled Status */
BOOL R_USB_HalIsEndpointStalled(volatile st_usb_object_t *_pchannel, uint8_t pipe);

/* interrupt handlier abstraction */
void R_USB_HalIsr(volatile st_usb_object_t *_pchannel);

void P_usb_detach(volatile st_usb_object_t *_pchannel);
#endif /* USB_HAL_H_INCLUDED */
