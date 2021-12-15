/******************************************************************************
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
 * and to discontinue the availability of this software. By using this software,
 * you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *******************************************************************************
 * Copyright (C) 2012 Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************
 * File Name    : usbhClass.c
 * Version      : 1.02
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : USB Host class driver loader
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 First Release
 *              : 14.12.2010 1.01 Added FTDI Device
 *              : 19.01.2011 1.02 Added Microchip Device (CDClass) and CDC class
 *                                load function
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/
#include <string.h>

#include "compiler_settings.h"
#include "r_devlink_wrapper.h"
#include "usbhClass.h"
#include "usbhEnum.h"
#include "trace.h"
#include "usbhAudioClass.h"
#include "r_usbh_comms_class.h"
#include "dskManager.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/

#define MCP2200EV_VID               0x04D8
#define MCP2200EV_PID               0x00DF

#define XR21B1411_VID               0x04E2
#define XR21B1411_PID               0x1411

#define STREAM_IT_CDC_VID           0x045B
#define STREAM_IT_CDC_PID           0x8111

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

/* This is used to create the CDC data interface information */
extern _Bool usbhLoadCommsDeviceClass (PUSBDI pDevice, uint8_t *pbyConfig);
static _Bool usbhSelectDeviceDriver (PUSBDI pDevice, uint8_t *pbyConfig, PDEVICE *ppDeviceDriver);

/******************************************************************************
 Imported global variables and functions (from other files)
 ******************************************************************************/
/* The lowest level driver for MS Bulk Only class devices */
extern const st_r_driver_t gMsBulkDriver;

/* The driver for HID keyboards */
extern const st_r_driver_t gHidKeyboardDriver;

/* The driver for HID mice */
extern const st_r_driver_t gHidMouseDriver;

/* The driver for CDC ACM Devices */
extern const st_r_driver_t g_cdc_driver;

/* Add other device drivers here as required...*/

/******************************************************************************
 Exported global variables and functions (to be accessed by other files)
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhLoadClassDriver
 Description:   Function to pars the device and configuration descriptor to load
 an appropriate driver for the device
 Arguments:     IN  pDevice - Pointer to the device
 IN  pbyConfig - Pointer to the device configuration descriptor
 IN  stLength - The length of the configuration descriptor
 OUT ppDeviceDriver - Pointer to the destination driver pointer
 Return value:  true if there is a class driver available
 ******************************************************************************/
_Bool usbhLoadClassDriver (PUSBDI pDevice, uint8_t *pbyConfig, size_t stLength, PDEVICE *ppDeviceDriver)
{
#define SUPPORT_MCP2200EV
#ifndef SUPPORT_MCP2200EV
    TRACE(( "usbClass.c - a Microchip MCP2200 device was recognised, but not yet supported.\r\n"));

#else
    /* Support for the MCP2200 device is done by the VID & PID only */
    if ( (pDevice->wVID == MCP2200EV_VID) )
    {
        if ( (pDevice->wPID == MCP2200EV_PID) )
        {
            /* Select the CDC device driver */
            if (R_USBH_LoadCommsDeviceClass(pDevice, pbyConfig))
            {
                *ppDeviceDriver = (PDEVICE)&g_cdc_driver;
                return true;
            }
            else
            {
                return false;
            }
        }
    }
#endif //SUPPORT_MCP2200EV

#define SUPPORT_XR21B1411
#ifndef SUPPORT_XR21B1411
    TRACE(( "usbClass.c - a Exar XR21B1411 device was recognised, but not yet supported.\r\n"));

#else
    /* Support for the XR21B1411 device is done by the VID & PID only */

    /* NOTE: XR21B1411 defaults to hardware Flow control - must use full loopback connector 
       or full connected RSR232 cable*/
    if ( (pDevice->wVID == XR21B1411_VID) )
    {
        if ( (pDevice->wPID == XR21B1411_PID) )
        {
            /* Select the CDC device driver */
            if (R_USBH_LoadCommsDeviceClass(pDevice, pbyConfig))
            {
                *ppDeviceDriver = (PDEVICE)&g_cdc_driver;
                return true;
            }
            else
            {
                return false;
            }
        }
    }
#endif //SUPPORT_XR21B1411

    /* Support CDC ACM class by the class and subclass */
    if ( (pDevice->byInterfaceClass == USB_CDC_CL_COMMUNICATION_INTERFACE_CLASS) )
    {
        if ( (pDevice->byInterfaceSubClass == USB_CDC_SC_ABSTRACT_CONTROL_MODEL) )
        {
            /* Select the CDC device driver */
            if (R_USBH_LoadCommsDeviceClass(pDevice, pbyConfig))
            {
                *ppDeviceDriver = (PDEVICE)&g_cdc_driver;
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    /* Check to see if the Device class is supported at interface level */
    if ((pDevice->byInterfaceClass == 0x00) || (pDevice->byInterfaceClass == 0xFF))
    {
        uint8_t *pbyInterface = pbyConfig;
        uint8_t *pbyEnd = pbyConfig + stLength;
        while (pbyInterface < pbyEnd)
        {
            PUSBIF pInterface = (PUSBIF) pbyInterface;
            /* Check the descriptor type for that of a interface type */
            if (pInterface->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
            {
                /* If it is then check it to see if there is a driver which
                 can be loaded. NOTE: This code is very simple and takes the
                 first interface which is supported. This means that compound
                 devices (devices with more than one interface) are not
                 supported */
                if (usbhSelectDeviceDriver(pDevice, pbyConfig, ppDeviceDriver))
                {
                    return true;
                }
                else
                {
                    /* Check the next descriptor */
                    pbyInterface += sizeof(USBIF);
                }
            }
            else
            {
                /* First item of every descriptor should be its length */
                if (pInterface->bLength)
                {
                    /* Check the next descriptor */
                    pbyInterface += pInterface->bLength;
                }
                else
                {
                    return false;
                }
            }
        }
        /* No appropriate driver found */
        return false;
    }
    /* Check to see if the class is supported at device level */
    else
    {
        return usbhSelectDeviceDriver(pDevice, pbyConfig, ppDeviceDriver);
    }
}
/******************************************************************************
 End of function  usbhLoadClassDriver
 ******************************************************************************/

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhCheckMSClass
 Description:   Function to check the suitability of the mass storage class
 device
 Arguments:     IN  bySubClass
 IN  byProtocol
 OUT ppDeviceDriver - Pointer to the destination driver pointer
 Return value:  true if there is a class driver available
 ******************************************************************************/
static _Bool usbhCheckMSClass (uint8_t bySubClass, uint8_t byProtocol, PDEVICE *ppDeviceDriver)
{
    _Bool bfScsi = false;
    switch (bySubClass)
    {
        case 0x01 :
            TRACE(("Subclass is SCSI Reduced Block Commands\r\n"));
            bfScsi = true;
        break;
        case 0x06 :
            TRACE(("Subclass is SCSI\r\n"));
            bfScsi = true;
        break;
        case 0x02 :
            TRACE(("Subclass is SFF-8020i, MMC-2(ATAPI) CD/DVD\r\n"));
        break;
        case 0x03 :
            TRACE(("Subclass is QIC-157 (Tape Drive)\r\n"));
        break;
        case 0x04 :
            TRACE(("Subclass is UFI (FDD)\r\n"));
        break;
        case 0x05 :
            TRACE(("Subclass is SFF-8070i (FDD)\r\n"));
            bfScsi = true;
        break;
        default :
            TRACE(("Subclass is unknown\r\n"));
        break;
    }
    /* Check to see if the device uses the SCSI command set */
    if (bfScsi)
    {
        /* Check for the supported protocols */
        switch (byProtocol)
        {
            case 0x00 :
            case 0x01 :
                TRACE(("Control/Bulk/Interrupt protocol not supported\r\n"));
                return false;
            case 0x50 :
                TRACE(("Selecting Mass Storage Class Bulk Only Transport Driver\r\n"));
                *ppDeviceDriver = (PDEVICE) &gMsBulkDriver;

                /* Call the disk management function to handle the disk */
                dskNewPnPDrive();
                return true;
            default :
                TRACE(("Unknown mass storage protocol\r\n"));
            break;
        }
    }
    return false;
}
/******************************************************************************
 End of function  usbhCheckMSClass
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhCheckHIDClass
 Description:   Function to check the suitability of the HID class
 device
 Arguments:     IN  bySubClass
 IN  byProtocol
 OUT ppDeviceDriver - Pointer to the destination driver pointer
 Return value:  true if there is a class driver available
 ******************************************************************************/
static _Bool usbhCheckHIDClass (uint8_t bySubClass, uint8_t byProtocol, PDEVICE *ppDeviceDriver)
{
    /* Check for Boot Interface subclass */
    if (bySubClass == 1)
    {
        /* Check for a keyboard */
        if (byProtocol == 1)
        {
            /* Assign the keyboard driver to this device */
            *ppDeviceDriver = (PDEVICE) &gHidKeyboardDriver;
            return true;
        }
        /* Check for a mouse */
        if (byProtocol == 2)
        {
            /* Assign the mouse driver to this device */
            *ppDeviceDriver = (PDEVICE) &gHidMouseDriver;
            return true;
        }
    }
    return false;
}
/******************************************************************************
 End of function  usbhCheckHIDClass
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhSelectDeviceDriver
 Description:   Function to select the appropriate device driver
 Arguments:     IN  pDevice - Pointer to the device information
 IN  pbyConfig - Pointer to the device configuration descriptor
 OUT ppDeviceDriver - Pointer to the destination driver pointer
 Return value:  true if there is a class driver available
 ******************************************************************************/
static _Bool usbhSelectDeviceDriver (PUSBDI pDevice, uint8_t *pbyConfig, PDEVICE *ppDeviceDriver)
{
    UNUSED_PARAM(pbyConfig);
    switch (pDevice->byInterfaceClass)
    {
        case 0x01 :
        {
            //ASSERVE48 audioInfo;
            //TODO RC return usbhLoadAudioClass(pDevice, pbyConfig, true, ppDeviceDriver, &audioInfo);
            break;
        }
        case 0x02 :
        {
            //cdc_connection_t   cdcInfo;
            TRACE(("Device is CDC class\r\n"));
            //TODO RC            return R_USBH_LoadCommsClass(pDevice, pbyConfig, true, ppDeviceDriver, &cdcInfo);
            break;
        }
        case 0x03 :
            TRACE(("Device is HID class\r\n"));
            return usbhCheckHIDClass(pDevice->byInterfaceSubClass, pDevice->byInterfaceProtocol, ppDeviceDriver);
        case 0x05 :
            TRACE(("Device is Physical class\r\n"));
        break;
        case 0x06 :
            TRACE(("Device is Image class\r\n"));
        break;
        case 0x07 :
            TRACE(("Device is Printer class\r\n"));
        break;
        case 0x08 :
            TRACE(("Device is Mass Storage class\r\n"));
            return usbhCheckMSClass(pDevice->byInterfaceSubClass, pDevice->byInterfaceProtocol, ppDeviceDriver);
        case 0x0A :
            TRACE(("Device is CDC-Data class\r\n"));
        break;
        case 0x0B :
            TRACE(("Device is Chip/Smart Card class\r\n"));
        break;
        case 0x0D :
            TRACE(("Device is Content-Security class\r\n"));
        break;
        case 0x0E :
            TRACE(("Device is Video class\r\n"));
        break;
        case 0x0F :
            TRACE(("Device is Personal Healthcare class\r\n"));
        break;
        case 0xE0 :
            TRACE(("Device is Wireless Controller class\r\n"));
        break;
        case 0xEF :
            TRACE(("Device is Miscellaneous class\r\n"));
        break;
        case 0xFE :
            TRACE(("Device is Application Specific\r\n"));
        break;
        case 0xFF :
            TRACE(("Device is Vendor Specific\r\n"));
        break;
        default :
            TRACE(("Device is of unknown class\r\n"));
        break;
    }
    return false;
}
/******************************************************************************
 End of function  usbhSelectDeviceDriver
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
