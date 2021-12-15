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
* File Name       : usbdescriptors.c
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      : HEW, Renesas SuperH Standard Tool chain v9.3
* H/W Platform    : RSK2+SH7269
* Description     : Descriptors required to enumerate a device as a USB HID Class.
*                   This sets up a Report IN and a Report OUT.
* 
*                   NOTE: This will need to be modified for a particular
*                   product as it includes company/product specific data including
*                   string descriptors specifying
*                   Manufacturer, Product and Serial Number.
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

/******************************************************************************
User Includes
******************************************************************************/
/*    Following header file provides definition for Low level driver. */
#include "r_usb_hal.h"
/*    Following header file provides definition for usb_core.c. */
#include "r_usbf_core.h"
/*    Following header file provides USB descriptor information. */
#include "../inc/usb_descriptors.h"

/******************************************************************************
Macros Defines
******************************************************************************/
/*Vendor and Product ID*/
/*NOTE Please use your company Vendor ID when developing a new product.*/
#define VID (0x045B)
#define PID (0x2014)

/*    Descriptor sizes    */
#define DEVICE_DESCRIPTOR_SIZE                   (18)
#define CONFIG_DESCRIPTOR_SIZE                   (67)
#define STRING_MANUFACTURER_SIZE                 (16)
#define STRING_PRODUCT_SIZE                      (44)
#define STRING_SERIAL_NUM_SIZE                   (8)
#define DEVICE_QUALIFIER_DESCRIPTOR_SIZE         (10)
#define OTHER_SPEED_CONFIG_DESCRIPTOR_SIZE_TOTAL (67)

/******************************************************************************
Global Variables
******************************************************************************/

static const uint8_t device_descriptor_data[DEVICE_DESCRIPTOR_SIZE] =
{
    /*Size of this descriptor*/
    DEVICE_DESCRIPTOR_SIZE,

    /*Device Descriptor*/
    0x01,

    /*USB Version 2.0*/
    0x00,0x02,

    /*Class Code - CDC*/
    0x02,

    /*Subclass Code*/
    0x00,

    /*Protocol Code*/
    0x00,

    /*Max Packet Size for endpoint 0*/
    CONTROL_IN_PACKET_SIZE,

    /*Vendor ID LSB*/
    (uint8_t)(VID & 0xFF),

    /*Vendor ID MSB*/
    (uint8_t)((VID>>8)& 0xFF),

    /*Product ID LSB*/
    (uint8_t)(PID & 0xFF),

    /*Product ID MSB*/
    (uint8_t)((PID>>8)& 0xFF),

    /*Device Release Number*/
    0x00,0x01,

    /*Manufacturer String Descriptor*/
    STRING_iMANUFACTURER,

    /*Product String Descriptor*/
    STRING_iPRODUCT,

    /*Serial Number String Descriptor*/
    STRING_iSERIAL,

    /*Number of Configurations supported*/
    0x01
};

/*Configuration Descriptor*/
static const uint8_t configuration_descriptor_data[CONFIG_DESCRIPTOR_SIZE] =
{
    /*Size of this descriptor (Just the configuration part)*/
    0x09,

    /*Configuration Descriptor*/
    0x02,

    /*Combined length of all descriptors (little endian)*/
    CONFIG_DESCRIPTOR_SIZE,0x00,

    /*Number of interfaces*/
    0x02,

    /*This Interface Value*/
    0x01,

    /*No String Descriptor for this configuration*/
    0x00,

    /*bmAttributes - Self Powered(USB bus powered), No Remote Wakeup*/
    0x80,

    /*bmAttributes - Not USB Bus powered, No Remote Wakeup*/        
    /*0x80,*/        
    /*bMaxPower (2mA units) 100mA (A unit load is defined as 100mA)*/
    50,

/* Communication Class Interface Descriptor */
    /*Size of this descriptor*/
    0x09,

    /*INTERFACE Descriptor*/
    0x04,

    /*Index of Interface*/
    0x00,

    /*bAlternateSetting*/
    0x00,

    /*Number of Endpoints*/
    0x01,

    /*Class code = Communication*/
    0x02,

    /*Subclass = Abstract Control Model*/
    0x02,

    /*No Protocol*/
    0x00,

    /*No String Descriptor for this interface*/
    0x00,

/*Header Functional Descriptor*/
    /*bFunctionalLength*/
    0x05,

    /*bDescriptorType = CS_INTERFACE*/
    0x24,

    /*bDescriptor Subtype = Header*/
    0x00,

    /*bcdCDC 1.1*/
    0x10,0x01,

/* ACM Functional Descriptor */
    /*bFunctionalLength*/
    0x04,

    /*bDescriptorType = CS_INTERFACE*/
    0x24,

    /*bDescriptor Subtype = Abstract Control Management*/
    0x02,

    /*bmCapabilities GET_LINE_CODING etc supported*/
    0x02,

/* Union Functional Descriptor */
    /*bFunctionalLength*/
    0x05,

    /*bDescriptorType = CS_INTERFACE*/
    0x24,

    /*bDescriptor Subtype = Union*/
    0x06,

    /*bMasterInterface = Communication Class Interface*/
    0x00,

    /*bSlaveInterface = Data Class Interface*/
    0x01,

/* Call Management Functional Descriptor */
    /*bFunctionalLength*/
    0x05,

    /*bDescriptorType = CS_INTERFACE*/
    0x24,

    /*bDescriptor Subtype = Call Management*/
    0x01,

    /*bmCapabilities*/
    0x00,

    /*bDataInterface: Data Class Interface = 1*/
    0x01,
    
/* Interrupt Endpoint */
    /*Size of this descriptor*/
    0x07,

    /*ENDPOINT Descriptor*/
    0x05,

    /*bEndpointAddress - IN endpoint, endpoint number = 6*/
    0x86,

    /*Endpoint Type is Interrupt*/
    0x03,

    /*Max Packet Size*/
    (ENDPOINT_6_7_PACKET_SIZE & 0xFF), ((ENDPOINT_6_7_PACKET_SIZE >> 8)& 0xFF),

    /*Polling Interval in mS*/
    0x0A,

/* DATA Class Interface Descriptor */
    /*Size of this descriptor*/
    0x09,

    /*INTERFACE Descriptor*/
    0x04,

    /*Index of Interface*/
    0x01,

    /*bAlternateSetting*/
    0x00,

    /*Number of Endpoints*/
    0x02,

    /*Class code = Data Interface*/
    0x0A,

    /*Subclass = Abstract Control Model*/
    0x00,

    /*No Protocol*/
    0x00,

    /*No String Descriptor for this interface*/
    0x00,

/*Endpoint Bulk OUT */
    /*Size of this descriptor*/
    0x07,

    /*ENDPOINT Descriptor*/
    0x05,

    /*bEndpointAddress - OUT endpoint, endpoint number = 1*/
    0x01,

    /*Endpoint Type is BULK*/
    0x02,

    /*Max Packet Size*/
    (ENDPOINT_1_2_PACKET_SIZE & 0xFF), ((ENDPOINT_1_2_PACKET_SIZE >> 8)& 0xFF),

    /*Polling Interval in mS - IGNORED FOR BULK*/
    0x00,

/* Endpoint Bulk IN */
    /*Size of this descriptor*/
    0x07,

    /*ENDPOINT Descriptor*/
    0x05,

    /*bEndpointAddress - IN endpoint, endpoint number = 2*/
    0x82,

    /*Endpoint Type is BULK*/
    0x02,

    /*Max Packet Size*/
    (ENDPOINT_1_2_PACKET_SIZE & 0xFF), ((ENDPOINT_1_2_PACKET_SIZE >> 8)& 0xFF),

    /*Polling Interval in mS - IGNORED FOR BULK*/
    0x00
};

/*String Descriptors*/
    /*Note Language ID is in USB Core */

/*Manufacturer string*/
/* "Renesas" */

static const uint8_t string_desc_manufacturer_data[STRING_MANUFACTURER_SIZE] =
{
    /* Length of this descriptor*/
    STRING_MANUFACTURER_SIZE,

    /* Descriptor Type = STRING */
    0x03,

    /* Descriptor Text (unicode) */
    'R', 0x00, 'E', 0x00, 'N', 0x00, 'E', 0x00,
    'S', 0x00, 'A', 0x00, 'S', 0x00
};

/*Product string*/
/* "CDC USB Demonstration" */
static const uint8_t string_desc_product_data[STRING_PRODUCT_SIZE] =
{
    /* Length of this descriptor*/
    STRING_PRODUCT_SIZE,

    /* Descriptor Type = STRING */
    0x03,

    /* Descriptor Text (unicode) */
    'C', 0x00, 'D', 0x00, 'C', 0x00, ' ', 0x00,
    'U', 0x00, 'S', 0x00, 'B', 0x00, ' ', 0x00,
    'D', 0x00, 'e', 0x00, 'm', 0x00, 'o', 0x00,
    'n', 0x00, 's', 0x00, 't', 0x00, 'r', 0x00,
    'a', 0x00, 't', 0x00, 'i', 0x00, 'o', 0x00,
    'n', 0x00
};

/*Serial number string "1.1"*/
static const uint8_t string_desc_serial_num_data[STRING_SERIAL_NUM_SIZE] =
{
    /* Length of this descriptor*/
    STRING_SERIAL_NUM_SIZE,

    /* Descriptor Type = STRING */
    0x03,

    /* Descriptor Text (unicode) */
    '1', 0x00, '.', 0x00, '1', 0x00
};

/*    Device Qualifier Descriptor    */
static const uint8_t device_qualifier_desc_data[DEVICE_QUALIFIER_DESCRIPTOR_SIZE] =
{
    /*Size of this descriptor*/
    DEVICE_QUALIFIER_DESCRIPTOR_SIZE,

    /*Device Qualifier Type*/
    0x06,

    /*USB Version 2.0*/
    0x00,0x02,

    /*Class Code - None as HID is defined in the Interface Descriptor*/
    0x00,

    /*Subclass Code*/
    0x00,

    /*Protocol Code*/
    0x00,

    /*Max Packet Size for Other Speed*/
    CONTROL_IN_PACKET_SIZE,

    /*Number of Configurations supported*/
    0x01,

    /*Reserved*/
    0x00
};

/*
Other Speed Configuration Descriptor
For HID this includes Interface Descriptors, HID descriptor and endpoint descriptor.
Ensure START_INDEX_OF_HID_WITHIN_CONFIG_DESC and HID_DESCRIPTOR_SIZE
are defined according to this.  
*/
/*This includes Interfaces and Endpoints*/
static const uint8_t other_speed_config_desc_data[OTHER_SPEED_CONFIG_DESCRIPTOR_SIZE_TOTAL] =
{
    /*Size of this descriptor (Just the configuration part)*/
    0x09,

    /*Other_speed_Configuration Type*/
    0x07,

    /*Combined length of all descriptors (little endian)*/
    /*Total length of data returned*/
    OTHER_SPEED_CONFIG_DESCRIPTOR_SIZE_TOTAL,0x00,

    /*Number of interfaces*/
    0x02,

    /*This Interface Value*/
    0x01,

    /*No String Descriptor for this configuration*/
    0x00,

    /*bmAttributes - Self Powered(USB bus powered), No Remote Wakeup*/
    0x80,

    /*bmAttributes - Not USB Bus powered, No Remote Wakeup*/        
    /*0x80,*/        
    /*bMaxPower (2mA units) 100mA (A unit load is defined as 100mA)*/
    50,

/* Communication Class Interface Descriptor */
    /*Size of this descriptor*/
    0x09,

    /*INTERFACE Descriptor*/
    0x04,

    /*Index of Interface*/
    0x00,

    /*bAlternateSetting*/
    0x00,

    /*Number of Endpoints*/
    0x01,

    /*Class code = Communication*/
    0x02,

    /*Subclass = Abstract Control Model*/
    0x02,

    /*No Protocol*/
    0x00,

    /*No String Descriptor for this interface*/
    0x00,

/*Header Functional Descriptor*/
    /*bFunctionalLength*/
    0x05,

    /*bDescriptorType = CS_INTERFACE*/
    0x24,

    /*bDescriptor Subtype = Header*/
    0x00,

    /*bcdCDC 1.1*/
    0x10,0x01,

/* ACM Functional Descriptor */
    /*bFunctionalLength*/
    0x04,

    /*bDescriptorType = CS_INTERFACE*/
    0x24,

    /*bDescriptor Subtype = Abstract Control Management*/
    0x02,

    /*bmCapabilities GET_LINE_CODING etc supported*/
    0x02,

/* Union Functional Descriptor */
    /*bFunctionalLength*/
    0x05,

    /*bDescriptorType = CS_INTERFACE*/
    0x24,

    /*bDescriptor Subtype = Union*/
    0x06,

    /*bMasterInterface = Communication Class Interface*/
    0x00,

    /*bSlaveInterface = Data Class Interface*/
    0x01,

/* Call Management Functional Descriptor */
    /*bFunctionalLength*/
    0x05,

    /*bDescriptorType = CS_INTERFACE*/
    0x24,

    /*bDescriptor Subtype = Call Management*/
    0x01,

    /*bmCapabilities*/
    0x00,

    /*bDataInterface: Data Class Interface = 1*/
    0x01,
    
/* Interrupt Endpoint */
    /*Size of this descriptor*/
    0x07,

    /*ENDPOINT Descriptor*/
    0x05,

    /*bEndpointAddress - IN endpoint, endpoint number = 6*/
    0x86,

    /*Endpoint Type is Interrupt*/
    0x03,

    /*Max Packet Size*/
    (ENDPOINT_6_7_PACKET_SIZE & 0xFF), ((ENDPOINT_6_7_PACKET_SIZE >> 8)& 0xFF),

    /*Polling Interval in mS*/
    0x0A,

/* DATA Class Interface Descriptor */
    /*Size of this descriptor*/
    0x09,

    /*INTERFACE Descriptor*/
    0x04,

    /*Index of Interface*/
    0x01,

    /*bAlternateSetting*/
    0x00,

    /*Number of Endpoints*/
    0x02,

    /*Class code = Data Interface*/
    0x0A,

    /*Subclass = Abstract Control Model*/
    0x00,

    /*No Protocol*/
    0x00,

    /*No String Descriptor for this interface*/
    0x00,

/*Endpoint Bulk OUT */
    /*Size of this descriptor*/
    0x07,

    /*ENDPOINT Descriptor*/
    0x05,

    /*bEndpointAddress - OUT endpoint, endpoint number = 1*/
    0x01,

    /*Endpoint Type is BULK*/
    0x02,

    /*Max Packet Size*/
    0x40, 0x00,

    /*Polling Interval in mS - IGNORED FOR BULK*/
    0x00,

/* Endpoint Bulk IN */
    /*Size of this descriptor*/
    0x07,

    /*ENDPOINT Descriptor*/
    0x05,

    /*bEndpointAddress - IN endpoint, endpoint number = 2*/
    0x82,

    /*Endpoint Type is BULK*/
    0x02,

    /*Max Packet Size*/
    0x40,0x00,

    /*Polling Interval in mS - IGNORED FOR BULK*/
    0x00
    };

const descriptor_t g_device_descriptor =
{
    DEVICE_DESCRIPTOR_SIZE,
    (uint8_t *)device_descriptor_data
};

const descriptor_t g_configuration_descriptor =
{
    CONFIG_DESCRIPTOR_SIZE,
    (uint8_t *)configuration_descriptor_data
};

const descriptor_t  g_string_desc_manufacturer =
{
    STRING_MANUFACTURER_SIZE,
    (uint8_t *)string_desc_manufacturer_data
};

const descriptor_t g_string_desc_product =
{
    STRING_PRODUCT_SIZE,
    (uint8_t *)string_desc_product_data
};

const descriptor_t g_string_desc_serial_num =
{
    STRING_SERIAL_NUM_SIZE,
    (uint8_t *)string_desc_serial_num_data
};

const descriptor_t g_desc_device_qualifier =
{
    DEVICE_QUALIFIER_DESCRIPTOR_SIZE,
    (uint8_t *)device_qualifier_desc_data
};

const descriptor_t g_desc_other_speed_config =
{
    OTHER_SPEED_CONFIG_DESCRIPTOR_SIZE_TOTAL,
    (uint8_t *)other_speed_config_desc_data
};
