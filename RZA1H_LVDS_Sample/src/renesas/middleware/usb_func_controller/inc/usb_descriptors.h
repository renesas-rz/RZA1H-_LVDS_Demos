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
* File Name       : usbdescriptors.h
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      : HEW, Renesas SuperH Standard Tool chain v9.3
* H/W Platform    : RSK2+SH7269
* Description     : Descriptors required to enumerate a device as a
*                   Mass Storage Device Class.
* 
*                   NOTE: This will need to be modified for a particular
*                   product as it includes company/product specific data including
*                   string descriptors specifying
*                   Manufacturer, Product and Serial Number.
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

#ifndef FILENAME_USBDESCRIPTORS_H
#define FILENAME_USBDESCRIPTORS_H

/******************************************************************************
User Includes
******************************************************************************/
/* Following header file provides rte type definitions. */
#include "stdint.h"
#include "r_usbf_core.h"

/******************************************************************************
Macro Defines
******************************************************************************/
#define CONTROL_IN_PACKET_SIZE         (64)
#define HID_REPORT_DESCRIPTOR_SIZE     (34)
#define HID_DESCRIPTOR_SIZE            (9)
#define START_INDEX_OF_HID_WITHIN_CONFIG_DESC (18)

/* The Endpoint 1 and 2 are configured as Bulk Endpoint
   The Packet Size can range from 1 byte to 12 bytes */
#define ENDPOINT_1_2_PACKET_SIZE    (512)

/* The Endpoint 3 and 4 are configured as Bulk Endpoint
   The Packet Size can be 8,16,32,64,512 bytes */
#define ENDPOINT_3_4_PACKET_SIZE    (512)

/* The Endpoint 5 is configured as Bulk Endpoint
   The Packet Size can be 8,16,32,64,512 bytes */
#define ENDPOINT_5_PACKET_SIZE        (512)

/* The Endpoint 6,7,8,9 is configured as interrupt Endpoint
   The Packet Size can range from 1 byte tp 63 bytes */
#define ENDPOINT_6_7_PACKET_SIZE    (64)
#define ENDPOINT_8_9_PACKET_SIZE    (64)

/* As specified in the Report Descriptor */
#define OUTPUT_REPORT_SIZE       (17)
/* As specified in the Report Descriptor */
#define INPUT_REPORT_SIZE        (5)

#ifndef STRING_iMANUFACTURER
#define STRING_iMANUFACTURER     (1)
#endif

#ifndef STRING_iPRODUCT
#define STRING_iPRODUCT          (2)
#endif

#define STRING_iSERIAL           (3)

/******************************************************************************
Type Definitions
*******************************************************************************/

/* Device Descriptor */
extern const descriptor_t g_device_descriptor;

/* Configuration, Interface, HID and Endpoint Descriptor */
extern const descriptor_t g_configuration_descriptor;

/* String descriptors */
extern const descriptor_t g_string_desc_language_ids;
extern const descriptor_t g_string_desc_manufacturer;
extern const descriptor_t g_string_desc_product;
extern const descriptor_t g_string_desc_serial_num;
extern const descriptor_t g_desc_device_qualifier;
extern const descriptor_t g_desc_other_speed_config;

#endif /* FILENAME_USBDESCRIPTORS_H */
