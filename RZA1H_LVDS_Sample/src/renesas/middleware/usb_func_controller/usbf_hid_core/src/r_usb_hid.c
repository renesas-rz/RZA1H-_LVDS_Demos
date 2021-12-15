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
* File Name       : R_USBHID.c
* Version         : 1.00
* Device          : RZA1H ( )
* Tool Chain      :
* H/W Platform    : RSK+RZA1H
* Description     : Human Interface Device (HID) USB Class.
*                   Supports an IN and an OUT HID Report.
* 
*                   NOTE: This module does not have any knowledge of the
*                   contents of the reports.
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

/***********************************************************************************
System Includes
***********************************************************************************/
#include <assert.h>

/***********************************************************************************
User Includes
***********************************************************************************/
/*    Following header file provides definition common to Upper and Low Level USB 
    driver. */
#include "usb_common.h"
/* USB Firmware Header File */
#include "r_usb_hal.h"
/*    Following header file provides definition for usb_core.c. */
#include "r_usbf_core.h"
/*    Following header file provides USB descriptor information. */
#include "../inc/r_usb_default_descriptors.h"
/* Following header file provides HID definitions. */
#include "r_usb_hid.h"

#include "trace.h"

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
    #undef TRACE
    #define TRACE(x)
#endif

/***********************************************************************************
Macros Defines
***********************************************************************************/
/* HID Descriptor Types */
#define HID_DESCRIPTOR            (0x21)
#define HID_REPORT_DESCRIPTOR     (0x22)

/* HID Class Request Codes */
#define GET_REPORT     (0x01)
#define SET_REPORT     (0x09)
#define SET_IDLE       (0x0A)

/***********************************************************************************
Local Types
***********************************************************************************/

/* The Endpoint 1 and 2 are configured as Bulk Endpoint
   The Packet Size can range from 1 byte to 512 bytes */
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

/* End point configuration array HID */
static volatile uint16_t  end_ptbl_1[] =
{
    PIPE1 |                                     /* Pipe Window Select Register (0x64) */
    D0_FIFO_USE,                                /* This macro specifies the register to
                                                   be used to read from the FIFO buffer
                                                   memory and write data to the FIFO
                                                   buffer memory */
    (BULK |
    ((DBLBOFF | CNTMDON) |
    (DIR_P_OUT | EP1))),                        /* Pipe Configuration Register (0x68) */

    (BUF_SIZE(BULK_OUT_PACKET_SIZE) | 16),      /* Pipe Buffer setting Register (0x6A) */

    BULK_OUT_PACKET_SIZE,                       /* Pipe Maxpacket Size Register (0x6C) */

    0,                                          /* Pipe Cycle Control Register (0x6E) */

    (PIPE2 |                                    /* Pipe Window Select Register (0x64) */
    D0_FIFO_USE),                               /* This macro specifies the register to
                                                   be used to read from the FIFO buffer
                                                   memory and write data to the FIFO
                                                   buffer memory */

    (BULK |
    ((DBLBON | CNTMDON) |
    (DIR_P_IN | EP2))),                         /* Pipe Configuration Register (0x68) */

    (BUF_SIZE(ENDPOINT_1_2_PACKET_SIZE) | 20),  /* Pipe Buffer setting Register (0x6A) */

    BULK_IN_PACKET_SIZE,                        /* Pipe Maxpacket Size Register (0x6C) */

    0,                                          /* Pipe Cycle Control Register (0x6E) */

    (PIPE3 |                                    /* Pipe Window Select Register (0x64) */
    D0_FIFO_USE),                               /* This macro specifies the register to
                                                   be used to read from the FIFO buffer
                                                   memory and write data to the FIFO
                                                   buffer memory */

    (BULK |
    ((DBLBON | CNTMDOFF) |
    (DIR_P_IN | EP3))),                         /* Pipe Configuration Register (0x68) */

    (BUF_SIZE(ENDPOINT_3_4_PACKET_SIZE) | 24),  /* Pipe Buffer setting Register (0x6A) */

    BULK_IN_PACKET_SIZE,                        /* Pipe Maxpacket Size Register (0x6C) */

    0,                                          /* Pipe Cycle Control Register (0x6E) */

    (PIPE4 |                                    /* Pipe Window Select Register (0x64) */
    D0_FIFO_USE),                               /* This macro specifies the register to
                                                   be used to read from the FIFO buffer
                                                   memory and write data to the FIFO
                                                   buffer memory */
    (BULK |
    ((DBLBON | CNTMDOFF) |
    (DIR_P_OUT | EP4))),                        /* Pipe Configuration Register (0x68) */

    (BUF_SIZE(ENDPOINT_3_4_PACKET_SIZE) | 28),  /* Pipe Buffer setting Register (0x6A) */

    BULK_OUT_PACKET_SIZE,                       /* Pipe Maxpacket Size Register (0x6C) */

    0,                                          /* Pipe Cycle Control Register (0x6E) */

    (PIPE5 |                                    /* Pipe Window Select Register (0x64) */
    D0_FIFO_USE),                               /* This macro specifies the register to
                                                   be used to read from the FIFO buffer
                                                   memory and write data to the FIFO
                                                   buffer memory */
    (BULK |
    ((DBLBOFF | CNTMDOFF) |
    (DIR_P_IN | EP5))),                         /* Pipe Configuration Register (0x68) */

    (BUF_SIZE(ENDPOINT_5_PACKET_SIZE) | 32),    /* Pipe Buffer setting Register (0x6A) */

    BULK_IN_PACKET_SIZE,                        /* Pipe Maxpacket Size Register (0x6C) */

    0,                                          /* Pipe Cycle Control Register (0x6E) */

    PIPE6,                                      /* Pipe Window Select Register (0x64) */
    (INT |
    (DIR_P_IN | EP6)),                          /* Pipe Configuration Register (0x68) */

    (BUF_SIZE(ENDPOINT_6_7_PACKET_SIZE) | 4),   /* Pipe Buffer setting Register (0x6A) */

    INTERRUPT_IN_PACKET_SIZE,                   /* Pipe Maxpacket Size Register (0x6C) */

    0,                                          /* Pipe Cycle Control Register (0x6E) */

    PIPE7,                                      /* Pipe Window Select Register (0x64) */

    (INT | (DIR_P_OUT | EP7)),                  /* Pipe Configuration Register (0x68) */

    (BUF_SIZE(ENDPOINT_6_7_PACKET_SIZE) | 5),   /* Pipe Buffer setting Register (0x6A) */

    INTERRUPT_IN_PACKET_SIZE,                   /* Pipe Maxpacket Size Register (0x6C) */
    0,                                          /* Pipe Cycle Control Register (0x6E) */

    PIPE8,                                      /* Pipe Window Select Register (0x64) */
    (INT | (DIR_P_IN | EP9)),                   /* Pipe Configuration Register (0x68) */

    (BUF_SIZE(ENDPOINT_8_9_PACKET_SIZE) | 6),   /* Pipe Buffer setting Register (0x6A) */

    INTERRUPT_IN_PACKET_SIZE,                   /* Pipe Maxpacket Size Register (0x6C) */
    0,                                          /* Pipe Cycle Control Register (0x6E) */

    PIPE9,                                      /* Pipe Window Select Register (0x64) */

    (INT | (DIR_P_IN | EP9)),                   /* Pipe Configuration Register (0x68) */

    BUF_SIZE(ENDPOINT_8_9_PACKET_SIZE) | 7,     /* Pipe Buffer setting Register (0x6A) */

    INTERRUPT_IN_PACKET_SIZE,                   /* Pipe Maxpacket Size Register (0x6C) */

    0,                                          /* Pipe Cycle Control Register (0x6E) */
};

/* Configuration 2 */
static volatile uint16_t  end_ptbl_2[] = { 0 };

/* Configuration 3 */
static volatile uint16_t  end_ptbl_3[] = { 0 };

/* Configuration 4 */
static volatile uint16_t  end_ptbl_4[] = { 0 };

/* Configuration 5 */
static volatile uint16_t  end_ptbl_5[] = { 0 };

/***********************************************************************************
Global Variables
***********************************************************************************/

/*Callback when a Report OUT is received */
static CB_REPORT_OUT m_cbreport_out;

/***********************************************************************************
Function Prototypes
***********************************************************************************/
static usb_err_t process_standard_setup_packet(volatile  st_usb_object_t *_pchannel,
                                            setup_packet_t* _pSetupPacket,
                                            uint16_t* _pNumBytes,
                                             uint8_t** _ppBuffer);
                                            
static usb_err_t process_get_descriptor(volatile  st_usb_object_t *_pchannel,
                                      setup_packet_t* _pSetupPacket,
                                        uint16_t* _pNumBytes,
                                         const uint8_t** _ppBuffer);
                                            
static usb_err_t process_class_setup_packet(volatile  st_usb_object_t *_pchannel,
                                        setup_packet_t* _pSetupPacket,
                                        uint16_t* _pNumBytes,
                                        uint8_t** _ppBuffer);

/*Callbacks required by USB_CORE*/
static usb_err_t cbunhandled_setup_packet(volatile  st_usb_object_t *_pchannel,
                                        setup_packet_t* _pSetupPacket,
                                        uint16_t* _pNumBytes,
                                        uint8_t** _ppBuffer);
                                        
static void cbdone_control_out(volatile  st_usb_object_t *_pchannel, usb_err_t _err, uint32_t _NumBytes);

static void cdcable(volatile  st_usb_object_t *_pchannel, BOOL _bConnected);

static void cberror(volatile st_usb_object_t *_pchannel, usb_err_t _err);

static void cbdone_interrupt_in(usb_err_t _err);

static void copy_input_report(volatile st_usb_object_t *_pchannel, uint8_t (*_report_in)[]);

static int16_t initialise_data(volatile st_usb_object_t *_pchannel);

/*******************************************************************************
User Program Code
********************************************************************************/


/*****************************************************************************
* Function Name   :   initialise_data
* Description     :   Initialise this modules data.
*                     Put into a function so that it can be done each time
*                     the USB cable is connected.
* Argument        :   -
* Return value    :   -
*****************************************************************************/
static int16_t initialise_data(volatile st_usb_object_t *_pchannel)
{
    /* Initialise Endpoints Descriptors */
    _pchannel->pend_pnt[0] = end_ptbl_1;
    _pchannel->pend_pnt[1] = end_ptbl_2;
    _pchannel->pend_pnt[2] = end_ptbl_3;
    _pchannel->pend_pnt[3] = end_ptbl_4;
    _pchannel->pend_pnt[4] = end_ptbl_5;

    return 1;
}
/******************************************************************************
End of function initialise_data
******************************************************************************/

/******************************************************************************
* Function Name    :    cbunhandled_setup_packet
* Description    :    Called from USB core when it can't deal with a setup
*                      packet.
*                     This is a function of type CB_SETUP_PACKET.
*                 
* Argument        :    _pSetupPacket - Setup packet.
*                     _pNumBytes - (OUT)Buffer size.
*                     _ppBuffer - (OUT)Buffer.
*                     
* Return value    :    Error code 
****************************************************************************/
static usb_err_t cbunhandled_setup_packet(volatile  st_usb_object_t *_pchannel,
                                        setup_packet_t* _pSetupPacket,
                                        uint16_t* _pNumBytes,
                                        uint8_t** _ppBuffer)
{
    _pchannel->err = USB_ERR_OK;
    
    switch(_pSetupPacket->bm_request.bit_val.d65)
    {
        case REQUEST_STANDARD:
        {
            /* Standard Type */
            _pchannel->err = process_standard_setup_packet(_pchannel,
                                             _pSetupPacket,
                                             _pNumBytes, _ppBuffer);
            break;
        }
        case REQUEST_CLASS:
        {
            /*Class Type */
            _pchannel->err = process_class_setup_packet(_pchannel, _pSetupPacket, _pNumBytes, _ppBuffer);
            break;
        }
        case REQUEST_VENDOR:
        {
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
            break;
        }
        default:
        {
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }
    
    return _pchannel->err;
}
/**********************************************************************************
End of function cbunhandled_setup_packet
***********************************************************************************/   

/***********************************************************************************
* Function Name    :    process_standard_setup_packet
* Description    :    Process a Standard Setup Packet that the lower layers couldn't.
* Argument        :    __pSetupPacket: Setup Packet
*                     _pNumBytes: (OUT)If this  can handle this then
*                                  this will be set with the size of the data.
*                                  (If there is no data stage then this will be set to zero)
*                     _ppBuffer: (OUT)If this  can handle this then
*                                  this will be set to point to the data(IN) or a buffer(OUT).
*                                  (If there is a data stage for this packet).
* Return value    :    Error Code
****************************************************************************/
static usb_err_t process_standard_setup_packet(volatile  st_usb_object_t *_pchannel,
                                            setup_packet_t* _pSetupPacket,
                                            uint16_t* _pNumBytes,
                                             uint8_t** _ppBuffer)
{
    _pchannel->err = USB_ERR_OK;
        
    switch(_pSetupPacket->b_request)
    {
        case GET_DESCRIPTOR:
        {
            _pchannel->err = process_get_descriptor(_pchannel,
                                       _pSetupPacket, _pNumBytes,
                                       (const uint8_t**)_ppBuffer);
            break;
            
        }
        default:
        {
            DEBUG_MSG_HIGH( ("USBHID: Unsupported Standard Setup request %d\r\n",
                         _pSetupPacket->b_request));
            TRACE( ("USBHID: Unsupported Standard Setup request %d\r\n", _pSetupPacket->b_request));
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }

    return _pchannel->err;
}
/**********************************************************************************
End of function process_standard_setup_packet
***********************************************************************************/   

/******************************************************************************
* Function Name    :    process_get_descriptor
* Description    :    Process a Get Descriptor request the the lower layers couldn't.
* Argument        :    _pSetupPacket: Setup Packet
*                     _pNumBytes: (OUT)If this  can handle this then
*                                  this will be set with the size of the data.
*                                  (If there is no data stage then this will be set to zero)
*                     _ppBuffer: (OUT)If this  can handle this then
*                                  this will be set to point to the data(IN)(Descriptor).
*                                  (If there is a data stage for this packet).
* Return value    :    Error Code 
****************************************************************************/
static usb_err_t process_get_descriptor(volatile  st_usb_object_t *_pchannel,
                                      setup_packet_t* _pSetupPacket,
                                      uint16_t* _pNumBytes,
                                      const uint8_t** _ppBuffer)
{
    _pchannel->err = USB_ERR_OK;
    
    /*USB Core handles normal Get Descriptor requests,
    so only need to support HID specific here.*/
    uint8_t descriptor_type = (uint8_t)((_pSetupPacket->w_value >> 8) & 0x00FF);
    switch(descriptor_type)
    {
        /* Hid Descriptor */
        case HID_DESCRIPTOR:
        {
            DEBUG_MSG_HIGH( ("USBHID: GET_DESCIPTOR - HID_DESCRIPTOR\r\n"));
            TRACE( ("USBHID: GET_DESCIPTOR - HID_DESCRIPTOR\r\n"));

            /* The HID Descriptor is stored within the configuration descriptor */
            /*Data IN response */
            *_pNumBytes = HID_DESCRIPTOR_SIZE;
            *_ppBuffer = &_pchannel->descriptors.config.puc_data[START_INDEX_OF_HID_WITHIN_CONFIG_DESC];
            break;
        }

        /* HID Report Descriptor */
        case HID_REPORT_DESCRIPTOR:
        {

            DEBUG_MSG_HIGH( ("USBHID: GET_DESCIPTOR - HID_REPORT_DESCRIPTOR\r\n"));
            TRACE( ("USBHID: GET_DESCIPTOR - HID_REPORT_DESCRIPTOR\r\n"));

            if(0 != _pchannel->descriptors.hid_report_in.length)
            {
                /*Data IN response */
                *_pNumBytes = _pchannel->descriptors.hid_report_in.length;
                *_ppBuffer = _pchannel->descriptors.hid_report_in.puc_data;
            }
            else
            {
                /* Report not configured return USB_ERR_UNKNOWN_REQUEST */
                _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
            }

            break;
        }
        default:
        {
            DEBUG_MSG_HIGH( ("USBHID: Unsupported GetDescriptor type %d\r\n",
                         descriptor_type));
            TRACE( ("USBHID: Unsupported GetDescriptor type %d\r\n", descriptor_type));
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }
    
    return _pchannel->err;
}
/**********************************************************************************
End of function process_get_descriptor
***********************************************************************************/   

/***********************************************************************************
* Function Name    :    process_class_setup_packet
* Description    :    Process HID Class specific requests.
* Argument        :    _pSetupPacket: Setup Packet
*                     _pNumBytes: (OUT)If this  can handle this then
*                                  this will be set with the size of the data.
*                                  (If there is no data stage then this will be set to zero)
*                     _ppBuffer: (OUT)If this  can handle this then
*                                  this will be set to point to the data(IN) or a buffer(OUT).
*                                  (If there is a data stage for this packet).
* Return value    :    Error Code 
****************************************************************************/
static usb_err_t process_class_setup_packet(volatile  st_usb_object_t *_pchannel,
                                         setup_packet_t* _pSetupPacket,
                                         uint16_t* _pNumBytes,
                                         uint8_t** _ppBuffer)
{
    _pchannel->err = USB_ERR_OK;

    switch(_pSetupPacket->b_request)
    {
        case SET_REPORT:
        {
            DEBUG_MSG_HIGH( ("USBHID: SET_REPORT\r\n"));

            if(0 != _pchannel->descriptors.report_out.length)
            {
                /*HOST is going to send the output report*/
                /*Data OUT*/
                *_pNumBytes = _pchannel->descriptors.report_out.length;
                *_ppBuffer = _pchannel->descriptors.report_out.puc_data;
            }
            else
            {
                /* Report not configured return USB_ERR_UNKNOWN_REQUEST */
                *_pNumBytes = 0;
                *_ppBuffer = 0;
                _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
            }
            break;
        }
        case GET_REPORT:
        {
            DEBUG_MSG_HIGH( ("USBHID: GET_REPORT\r\n"));

            if(0 != _pchannel->descriptors.report_in.length)
            {
                /*HOST has requested the input report*/
                /*Data IN*/
                *_pNumBytes = _pchannel->descriptors.report_in.length;
                *_ppBuffer = _pchannel->descriptors.report_in.puc_data;
            }
            else
            {
                /* Report not configured return USB_ERR_UNKNOWN_REQUEST */
                *_pNumBytes = 0;
                *_ppBuffer = 0;
                _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
            }
            break;
        }
        case SET_IDLE:
        {
            /* dummy code */
            *_pNumBytes = 0;
            *_ppBuffer = 0;
            break;
        }
        default:
        {
            DEBUG_MSG_HIGH( ("USBHID: Unsupported Class Setup request %d\r\n",
                         _pSetupPacket->b_request));
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }
    
    return _pchannel->err;
}
/******************************************************************************
End of function process_class_setup_packet
***********************************************************************************/

/***********************************************************************************
* Function Name    :    cbdone_control_out
* Description    :    A Control Out has completed in response to a
*                     setup packet handled in cbunhandled_setup_packet.
* Argument        :    _err: Error Code
*                     _NumBytes: Number of bytes received.
* Return value    :    - 
****************************************************************************/
static void cbdone_control_out(volatile  st_usb_object_t *_pchannel, usb_err_t _err, uint32_t _NumBytes)
{
    (void) _err;
    (void) _NumBytes;
    /*Call registered callback*/
    m_cbreport_out((uint8_t(*)[])_pchannel->descriptors.report_out.puc_data);
}

/******************************************************************************
End of function cbdone_control_out
***********************************************************************************/   

/***********************************************************************************
* Function Name    :    cbdone_interrupt_in
* Description    :    A Control IN has completed in response to a
*                     setup packet handled in cbunhandled_setup_packet.
* Argument        :    _err: Error Code
* Return value    :    - 
****************************************************************************/
static void cbdone_interrupt_in(usb_err_t _err)
{
    assert(USB_ERR_OK == _err);

    /*Nothing to do*/
} 
/**********************************************************************************
End of function cbdone_interrupt_in
***********************************************************************************/   

/******************************************************************************
* Function Name    :    cdcable
* Description    :    Callback function called when the USB cable is
*                     Connected/Disconnected.
*                     Sets connected flag.
*                 
* Argument        :    _bConnected: TRUE = Connected, FALSE = Disconnected.
* Return value    :    - 
****************************************************************************/
static void cdcable(volatile  st_usb_object_t *_pchannel, BOOL _bConnected)
{    
    if(TRUE == _bConnected)
    {
        DEBUG_MSG_HIGH( ("USBHID: Cable Connected\r\n"));
        TRACE( ("USBHID: Cable Connected\r\n"));
        
        _pchannel->connected = TRUE;
    }
    else
    {
        DEBUG_MSG_HIGH( ("USBHID: Cable Disconnected\r\n"));
        TRACE( ("USBHID: Cable Disconnected\r\n"));

        _pchannel->connected = FALSE;
    }
}
/**********************************************************************************
End of function cdcable
***********************************************************************************/   

/***********************************************************************************
* Function Name    :    cberror
* Description    :    One of the lower layers has reported an error.
*                     Not expecting this but try resetting HAL.
* Argument        :    -
* Return value    :    - 
****************************************************************************/
static void cberror(volatile st_usb_object_t *_pchannel, usb_err_t _err)
{
    (void) _err;

    DEBUG_MSG_HIGH( ("USBHID: ***cberror***\r\n"));
    assert(0);
    
    /*Reset HAL*/
    R_USB_HalReset(_pchannel);
    
    /* configure and Reset Endpoints */
    R_USB_HalResetEp(_pchannel, 1);
}
/**********************************************************************************
End of function cberror
***********************************************************************************/   

/******************************************************************************
* Function Name    :    copy_input_report
* Description    :    Copy supplied input report to input report buffer.
* Argument        :    _report_in - Input report
* Return value    :    - 
****************************************************************************/
static void copy_input_report(volatile st_usb_object_t *_pchannel, uint8_t (*_report_in)[])
{
    uint16_t index;
        
    /*Copy INPUT Report - so can send it whenever a GET_REPORT
    request is received.*/
    for(index = 0; index < _pchannel->descriptors.report_in.length; index++)
    {
        _pchannel->descriptors.report_in.puc_data[index] = (*_report_in)[index];
    }
}
/**********************************************************************************
End of function copy_input_report
***********************************************************************************/   


/******************************************************************************
* Function Name   :    R_USB_HidInit
* Description     :    Initialises this module.
* Argument        :    _report_in - A report to send to the host when it asks
                        for it.
                       _cb - A Callback function to be called when a OUT
                        report is received.
* Return value    :    Error Code.
****************************************************************************/
usb_err_t R_USB_HidInit(volatile st_usb_object_t *_pchannel,
                      uint8_t (*_report_in)[],
                      CB_REPORT_OUT _cb)
{
    _pchannel->err = USB_ERR_OK;

    /* Initialise the data */
    if(initialise_data(_pchannel))
    {
        /* Initialise the USB core */
        _pchannel->err = R_USBF_CoreInit(_pchannel,
                       g_usbf_str_desc_manufacturer.puc_data,
                       g_usbf_str_desc_manufacturer.length,
                       g_usbf_str_desc_product.puc_data,
                       g_usbf_str_desc_product.length,
                       g_usbf_str_desc_serial_num.puc_data,
                       g_usbf_str_desc_serial_num.length,
                       g_usbf_DeviceDescriptor.puc_data,
                       g_usbf_DeviceDescriptor.length,
                       g_usbf_device_qualifier_desc.puc_data,
                       g_usbf_device_qualifier_desc.length,
                       g_usbf_ConfigurationDescriptor.puc_data,
                       g_usbf_ConfigurationDescriptor.length,
                       g_usbf_other_speed_config_desc.puc_data,
                       g_usbf_other_speed_config_desc.length,
                       (CB_SETUP_PACKET)cbunhandled_setup_packet,
                       (CB_DONE_OUT)cbdone_control_out,
                       (CB_CABLE)cdcable,
                       (CB_ERROR)cberror);
    }

    /*Store call back*/
    m_cbreport_out = _cb;

    /*Initialise the IN Report*/
    copy_input_report(_pchannel, _report_in);

    return _pchannel->err;
}
/***********************************************************************************
End of function R_USB_HidInit
***********************************************************************************/

/******************************************************************************
* Function Name   :    R_USB_HidReportIn
* Description     :    Send IN Report to host.
*                      Uses Interrupt IN.
* Argument        :    _report_in: The report to send to the host.
* Return value    :    Error code
****************************************************************************/
usb_err_t R_USB_HidReportIn(volatile st_usb_object_t *_pchannel, uint8_t (*_report_in)[])
{
    _pchannel->err = USB_ERR_OK;

    copy_input_report(_pchannel, _report_in);

    /*Send Report using INTERRUPT IN*/
    R_USB_HalInterruptIn(_pchannel,
                        _pchannel->descriptors.report_in.length,
                        _pchannel->descriptors.report_in.puc_data,
                        cbdone_interrupt_in);

    return _pchannel->err;
}
/***********************************************************************************
End of function R_USB_HidReportIn
***********************************************************************************/


/*****************************************************************************
* Function Name   :    R_USB_HidIsConnected
* Description     :    Get the USB cable connected state.
* Argument        :    -
* Return value    :    TRUE = Connected, FALSE = Disconnected.
*****************************************************************************/
BOOL R_USB_HidIsConnected(volatile st_usb_object_t *_pchannel)
{
    (void) _pchannel;
    return true;
//    return (BOOL)_pchannel->connected;
}
/******************************************************************************
End of function R_USB_HidIsConnected
******************************************************************************/
