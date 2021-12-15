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
 * @headerfile     r_usbh_comms_class.h
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
#ifndef USBHCDCLASS_H_INCLUDED
#define USBHCDCLASS_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_USB_COMMS
 * @{
 *****************************************************************************/ 
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "r_usbh_drv_comms_class.h"

/******************************************************************************
Macro definitions
******************************************************************************/

/* CDC Interface Class Codes */
#define USB_CDC_CL_COMMUNICATION_INTERFACE_CLASS      (0x02)

/* CDC Interface Subclass Codes */
#define USB_CDC_SC_SUBCLASS_RESERVED                  (0x00)
#define USB_CDC_SC_DIRECT_LINE_CONTROL_MODEL          (0x01)
#define USB_CDC_SC_ABSTRACT_CONTROL_MODEL             (0x02)
#define USB_CDC_SC_TELEPHONE_CONTROL_MODEL            (0x03)
#define USB_CDC_SC_MILTI_CHANNEL_CONTROL_MODEL        (0x04)
#define USB_CDC_SC_CAPI_CONTROL_MODEL                 (0x05)
#define USB_CDC_SC_ETHERNET_NETWORKING_CONTROL_MODEL  (0x06)
#define USB_CDC_SC_ATM_NETWORKING_CONTROL_MODEL       (0x07)
#define USB_CDC_SC_RESERVED_FUTURE_USE                (<08h-7Fh>)
#define USB_CDC_SC_RESERVED_VENDOR_SPECIFIC           (<80h-FEh>)

/* CDC Interface Protocol Codes */
#define USB_CDC_CPR_USB_SPECIFICATION               (0x00)
#define USB_CDC_CPR_USB_V25TER                      (0x01)
#define USB_CDC_CPR_RESERVED_VENDOR_SPECIFIC        (0xFF)

/* CDC Data Interface Class Codes */
#define USB_CDC_DI_DATA_INTERFACE_CLASS            (0x0A)

/* CDC Data Interface Sub Class Codes */
#define USB_CDC_DI_DATA_INTERFACE_SUB_CLASS        (0x00)

/* CDC Data Interface Protocol Codes */
#define USB_CDC_DPR_USB_SPECIFICATION         (0x00)
#define USB_CDC_DPR_ISDN430                   (0x30)
#define USB_CDC_DPR_HDLC                      (0x31)
#define USB_CDC_DPR_TRANSPARENT               (0x32)
#define USB_CDC_DPR_Q921_MAMAGEMENT           (0x50)
#define USB_CDC_DPR_Q921_DATA                 (0x51)
#define USB_CDC_DPR_Q921_TEI_MULTIPLEXOR      (0x52)
#define USB_CDC_DPR_V42BIS                    (0x90)
#define USB_CDC_DPR_ISDN_EURO                 (0x91)
#define USB_CDC_DPR_ISDN_V24                  (0x92)
#define USB_CDC_DPR_CAPI                      (0x93)
#define USB_CDC_DPR_HOST_DRIVER               (0xFD)
#define USB_CDC_DPR_CDC_SPECIFICATION         (0xFE)
#define USB_CDC_DPR_RESERVED_VENDOR_SPECIFIC  (0xFF)

/* CDC Functional class specific descriptors */
#define USB_CDC_INTERFACE         (0x24)
#define USB_CDC_ENDPOINT          (0x25)

/* CDC Class Requests IDs from Table 46: Class-Specific Request Codes */
#define USB_CDC_SEND_ENCAP_COMMAND                           (0x00)
#define USB_CDC_GET_ENCAP_RESPONSE                           (0x01)
#define USB_CDC_SET_COMM_FEATURE                             (0x02)
#define USB_CDC_GET_COMM_FEATURE                             (0x03)
#define USB_CDC_CLEAR_COMM_FEATURE                           (0x04)
#define USB_CDC_SET_AUX_LINE_STATE                           (0x10)
#define USB_CDC_SET_HOOK_STATE                               (0x11)
#define USB_CDC_PULSE_SETUP                                  (0x12)
#define USB_CDC_SEND_PULSE                                   (0x13)
#define USB_CDC_SET_PULSE_TIME                               (0x14)
#define USB_CDC_RING_AUX_JACK                                (0x15)
#define USB_CDC_SET_LINE_CODING                              (0x20)
#define USB_CDC_GET_LINE_CODING                              (0x21)
#define USB_CDC_SET_CONTROL_LINE_STATE                       (0x22)
#define USB_CDC_SEND_BREAK                                   (0x23)
#define USB_CDC_SET_RINGER_PARMS                             (0x30)
#define USB_CDC_GET_RINGER_PARMS                             (0x31)
#define USB_CDC_SET_OPERATION_PARMS                          (0x32)
#define USB_CDC_GET_OPERATION_PARMS                          (0x33)
#define USB_CDC_SET_LINE_PARMS                               (0x34)
#define USB_CDC_GET_LINE_PARMS                               (0x35)
#define USB_CDC_DIAL_DIGITS                                  (0x36)
#define USB_CDC_SET_UNIT_PARAMETER                           (0x37)
#define USB_CDC_GET_UNIT_PARAMETER                           (0x38)
#define USB_CDC_CLEAR_UNIT_PARAMETER                         (0x39)
#define USB_CDC_GET_PROFILE                                  (0x3A)
#define USB_CDC_SET_ETHERNET_MULTICAST_FILTERS               (0x40)
#define USB_CDC_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER (0x41)
#define USB_CDC_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER (0x42)
#define USB_CDC_SET_ETHERNET_PACKET_FILTER                   (0x43)
#define USB_CDC_GET_ETHERNET_STATISTIC                       (0x44)
#define USB_CDC_SET_ATM_DATA_FORMAT                          (0x50)
#define USB_CDC_GET_ATM_DEVICE_STATISTICS                    (0x51)
#define USB_CDC_SET_ATM_DEFAULT_VC                           (0x52)
#define USB_CDC_GET_ATM_VC_STATISTICS                        (0x53)

/* From Table 51: Control Signal Bitmap Values for SetControlLineState */
#define USB_CDC_RTS                 (BIT_1)
#define USB_CDC_DTR                 (BIT_0)

/* Table 25: bDescriptor SubType in Functional Descriptors */
#define USB_CDC_FD_HEADER                                           (0x00) /* marks the beginning of the concatenated set of functional descriptors for the interface. */
#define USB_CDC_FD_CALL_MANAGEMENT                                  (0x01)
#define USB_CDC_FD_ABSTRACT_CONTROL_MANAGEMENT                      (0x02)
#define USB_CDC_FD_DIRECT_LINE_MANAGEMENT_FD                        (0x03)
#define USB_CDC_FD_TELEPHONE_RINGER                                 (0x04)
#define USB_CDC_FD_TELEPHONE_CALL_AND_LINE_STATE_REPORT_CAPABILITY  (0x05)
#define USB_CDC_FD_UNION                                            (0x06)
#define USB_CDC_FD_COUNTRY_SELECTION                                (0x07)
#define USB_CDC_FD_TELEPHONE_OPERATIONAL_MODES                      (0x08)
#define USB_CDC_FD_USB_TERMINAL                                     (0x09)
#define USB_CDC_FD_NETWORK_CHANNEL_TERMINAL                         (0x0A)
#define USB_CDC_FD_PROTOCOL_UNIT                                    (0x0B)
#define USB_CDC_FD_EXTENSION_UNIT                                   (0x0C)
#define USB_CDC_FD_MULTI_CHANNEL_MANAGEMENT                         (0x0D)
#define USB_CDC_FD_CAPI_CONTROL_MANAGEMENT                          (0x0E)
#define USB_CDC_FD_ETHERNET_NETWORKING                              (0x0F)
#define USB_CDC_FD_ATM_NETWORKING                                   (0x10)


/******************************************************************************
Typedef definitions
******************************************************************************/

/* NOTE: Only a subset of the Comms Class descriptors are defined here */

#pragma pack (1)

/** The CDC Interface Header Descriptor (Table 26) */
typedef struct
{
    uint8_t  blength;
    uint8_t  bdescriptor_type;
    uint8_t  bdescriptor_subtype;
    uint16_t bcd_cdc;
} usbcdchd_t;
typedef usbcdchd_t* pusbcdchd_t;

/** The CDC Call Management Functional Descriptor (Table 27) */
typedef struct
{
    /** Size of this functional descriptor, in bytes.*/
    uint8_t  blength;
    uint8_t  bdescriptor_type;
    uint8_t  bdescriptor_subtype;

    /**  The capabilities that this configuration supports:0
        D7..D2: RESERVED (Reset to zero)
        D1: 0 - Device sends/receives call management information only over
                the Communication Class interface.
            1 - Device can send/receive call management information over a
                Data Class interface.
        D0: 0 - Device does not handle call management itself.
            1 - Device handles call management itself.
        The previous bits, in combination, identify which call management
        scenario is used. If bit D0 is reset to 0, then the value of bit D1 is
        ignored. In this case, bit D1 is reset to zero for future
        compatibility. */
    uint8_t  bmcapabilities;

    /** Indicates that multiplexed commands are handled via data interface 01h
       (same value as used in the UNION Functional Descriptor) */
    uint8_t  bdatainterface;
} usbcdccmgmt_t;
typedef usbcdccmgmt_t* pusbcdccmgmt_t;

/** The CDC Call Management Functional Descriptor (Table 27) */
typedef struct
{
    /** Size of this functional descriptor, in bytes.*/
    uint8_t  blength;
    uint8_t  bdescriptor_type;
    uint8_t  bdescriptor_subtype;

    /**  The capabilities that this configuration supports. (A bit value of zero
        means that the request is not supported.)
        D7..D4: RESERVED (Reset to zero)
        D3: 1 - Device supports the notification Network_Connection.
        D2: 1 - Device supports the request Send_Break
        D1: 1 - Device supports the request combination of Set_Line_Coding,
                Set_Control_Line_State, Get_Line_Coding, and the notification
                Serial_State.
        D0: 1 - Device supports the request combination of Set_Comm_Feature,
                Clear_Comm_Feature, and Get_Comm_Feature.
        The previous bits, in combination, identify which requests /
        notifications are supported by a Communication Class interface with
        the SubClass code of Abstract Control Model. */
    uint8_t  bmcapabilities;
} usbcdcacmgmt_t;
typedef usbcdcacmgmt_t* pusbcdcacmgmt_t;

#pragma pack()

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Function to check the configuration descriptor
 * 
 * @param[in]     pdevice:             Pointer to the device information
 * @param[in]     pby_config:          Pointer to the configuration descriptor
 * @param[in]     bf_create_interface: Flag set true to create interface
 *                                     information
 * @param[out]    pp_device_driver:    Pointer to the destination driver
 * @param[out]    p_cdc_info:          Pointer to the CDC information
 * 
 * @retval        true: if the sample driver will work with the device
*/
_Bool R_USBH_LoadCommsClass(   PUSBDI          pdevice,
                               uint8_t         *pby_config,
                               _Bool           bf_create_interface,
                               PDEVICE         *pp_device_driver,
                               pcdc_connection_t  p_cdc_info);

#ifdef __cplusplus
}
#endif

#endif /* USBHCDCLASS_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
