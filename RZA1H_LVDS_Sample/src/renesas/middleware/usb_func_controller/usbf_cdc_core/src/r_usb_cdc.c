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
* File Name      : usb_cdc.c
* Version        : 1.00
* Device         : RZA1(H)
* Tool Chain     : HEW, Renesas SuperH Standard Tool chain v9.3
* H/W Platform   : RSK2+SH7269
* Description    : USB CDC Class (Abstract Class Model).
*                  Provides a virtual COM port.
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

/******************************************************************************
System Includes
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <assert.h>

/******************************************************************************
User Includes
******************************************************************************/

#include "compiler_settings.h"

/*    Following header file provides definition common to Upper and Low Level USB 
    driver. */
#include "usb_common.h"
/*    Following header file provides definition for Low level driver. */
#include "r_usb_hal.h"
/*    Following header file provides definition for usb_core.c. */
#include "r_usbf_core.h"
/*    Following header file provides USB descriptor information. */
#include "usb_descriptors.h"
/*    Following header file provides definition for USB CDC application. */
#include "r_usb_cdc.h"

/******************************************************************************
Macros Defines
******************************************************************************/
/* CDC Class Requests IDs*/
#define SET_LINE_CODING         (0x20)
#define GET_LINE_CODING         (0x21)
#define SET_CONTROL_LINE_STATE  (0x22)

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


/******************************************************************************
Global Variables
******************************************************************************/

/* End point configuration array CDC */
static volatile uint16_t  end_ptbl_1[] =
{
    PIPE1 |                                                /* Pipe Window Select Register (0x64)  */
    C_FIFO_USE,                                            /* This macro specifies the register to
                                                              be used to read from the FIFO buffer
                                                              memory and write data to the FIFO
                                                              buffer memory */
    ((((BULK | DBLBOFF) | CNTMDON) | DIR_P_OUT) | EP1),    /* Pipe Configuration Register (0x68)  */
    BUF_SIZE(BULK_OUT_PACKET_SIZE) | 16,                   /* Pipe Buffer setting Register (0x6A) */
    BULK_OUT_PACKET_SIZE,                                  /* Pipe Maxpacket Size Register (0x6C) */
    0,                                                     /* Pipe Cycle Control Register (0x6E)  */

    PIPE2 |                                                /* Pipe Window Select Register (0x64)  */
    C_FIFO_USE,                                            /* This macro specifies the register to
                                                              be used to read from the FIFO buffer
                                                              memory and write data to the FIFO
                                                              buffer memory */
    ((((BULK | DBLBON) | CNTMDON) | DIR_P_IN) | EP2),      /* Pipe Configuration Register (0x68)  */
    BUF_SIZE(BULK_IN_PACKET_SIZE) | 36,                    /* Pipe Buffer setting Register (0x6A) */
    BULK_IN_PACKET_SIZE,                                   /* Pipe Maxpacket Size Register (0x6C) */
    0,                                                     /* Pipe Cycle Control Register (0x6E)  */

    PIPE3 |                                                /* Pipe Window Select Register (0x64)  */
    D0_FIFO_USE,                                           /* This macro specifies the register to
                                                              be used to read from the FIFO buffer
                                                              memory and write data to the FIFO
                                                              buffer memory */
    ((((BULK | DBLBON) | CNTMDOFF) | DIR_P_IN) | EP3),     /* Pipe Configuration Register (0x68)  */
    BUF_SIZE(ENDPOINT_3_4_PACKET_SIZE) | 57,               /* Pipe Buffer setting Register (0x6A) */
    BULK_IN_PACKET_SIZE,                                   /* Pipe Maxpacket Size Register (0x6C) */
    0,                                                     /* Pipe Cycle Control Register (0x6E)  */

    PIPE4 |                                                /* Pipe Window Select Register (0x64)  */
    D0_FIFO_USE,                                           /* This macro specifies the register to
                                                              be used to read from the FIFO buffer
                                                              memory and write data to the FIFO
                                                              buffer memory */
    ((((BULK | DBLBON) | CNTMDOFF) | DIR_P_OUT) | EP4),    /* Pipe Configuration Register (0x68)  */
    BUF_SIZE(ENDPOINT_3_4_PACKET_SIZE) | 77,               /* Pipe Buffer setting Register (0x6A) */
    BULK_OUT_PACKET_SIZE,                                  /* Pipe Maxpacket Size Register (0x6C) */
    0,                                                     /* Pipe Cycle Control Register (0x6E)  */

    PIPE5 |                                                /* Pipe Window Select Register (0x64)  */
    D0_FIFO_USE,                                           /* This macro specifies the register to
                                                              be used to read from the FIFO buffer
                                                              memory and write data to the FIFO
                                                              buffer memory */
    ((((BULK | DBLBOFF) | CNTMDOFF) | DIR_P_IN) | EP5),    /* Pipe Configuration Register (0x68)  */
    BUF_SIZE(ENDPOINT_5_PACKET_SIZE) | 98,                 /* Pipe Buffer setting Register (0x6A) */
    BULK_IN_PACKET_SIZE,                                   /* Pipe Maxpacket Size Register (0x6C) */
    0,                                                     /* Pipe Cycle Control Register (0x6E)  */

    PIPE6,                                                 /* Pipe Window Select Register (0x64)  */
    ((INT | DIR_P_IN) | EP6),                              /* Pipe Configuration Register (0x68)  */
    BUF_SIZE(ENDPOINT_6_7_PACKET_SIZE) | 4,                /* Pipe Buffer setting Register (0x6A) */
    INTERRUPT_IN_PACKET_SIZE,                              /* Pipe Maxpacket Size Register (0x6C) */
    0,                                                     /* Pipe Cycle Control Register (0x6E)  */

    PIPE7,                                                 /* Pipe Window Select Register (0x64)  */
    ((INT | DIR_P_OUT) | EP7),                             /* Pipe Configuration Register (0x68)  */
    BUF_SIZE(ENDPOINT_6_7_PACKET_SIZE) | 5,                /* Pipe Buffer setting Register (0x6A) */
    INTERRUPT_IN_PACKET_SIZE,                              /* Pipe Maxpacket Size Register (0x6C) */
    0,                                                     /* Pipe Cycle Control Register (0x6E)  */

    PIPE8,                                                 /* Pipe Window Select Register (0x64)  */
    ((INT | DIR_P_IN) | EP9),                              /* Pipe Configuration Register (0x68)  */
    BUF_SIZE(ENDPOINT_8_9_PACKET_SIZE) | 6,                /* Pipe Buffer setting Register (0x6A) */
    INTERRUPT_IN_PACKET_SIZE,                              /* Pipe Maxpacket Size Register (0x6C) */
    0,                                                     /* Pipe Cycle Control Register (0x6E) */

    PIPE9,                                                 /* Pipe Window Select Register (0x64) */
    ((INT | DIR_P_IN) | EP9),                              /* Pipe Configuration Register (0x68) */
    BUF_SIZE(ENDPOINT_8_9_PACKET_SIZE) | 7,                /* Pipe Buffer setting Register (0x6A) */
    INTERRUPT_IN_PACKET_SIZE,                              /* Pipe Maxpacket Size Register (0x6C) */
    0,                                                     /* Pipe Cycle Control Register (0x6E) */
};

/* Configuration 2 */
static volatile uint16_t  end_ptbl_2[] = { 0 };

/* Configuration 3 */
static volatile uint16_t  end_ptbl_3[] = { 0 };

/* Configuration 4 */
static volatile uint16_t  end_ptbl_4[] = { 0 };

/* Configuration 5 */
static volatile uint16_t  end_ptbl_5[] = { 0 };

/******************************************************************************
Function Prototypes
******************************************************************************/
static uint16_t initialise_data(volatile st_usb_object_t *_pchannel);
static usb_err_t process_class_setup_packet(volatile st_usb_object_t *_pchannel,
                                         setup_packet_t* _pSetupPacket,
                                         uint16_t* _pNumBytes,
                                         uint8_t** _ppBuffer);

/*Callbacks required by USB CORE*/
static usb_err_t cb_unhandled_setup_packet(volatile st_usb_object_t *_pchannel,
                                        setup_packet_t* _pSetupPacket,
                                        uint16_t* _pNumBytes,
                                        uint8_t** _ppBuffer);

static void cb_done_control_out(volatile st_usb_object_t *_pchannel, usb_err_t _err, uint32_t _num_bytes);
static void cb_done_bulk_out(volatile st_usb_object_t *_pchannel,usb_err_t _err, uint32_t _num_bytes);
static void cb_done_bulk_in(volatile st_usb_object_t *_pchannel, usb_err_t _err);

static void cb_error(volatile st_usb_object_t * _pchannel, usb_err_t _err);

//static void set_connected_status(volatile st_usb_object_t *_pchannel, BOOL _status );

static void release_flags(volatile st_usb_object_t *_pchannel, usb_err_t _err);
static void cb_cable(volatile st_usb_object_t *_pchannel, BOOL _bConnected);

/******************************************************************************
User Program Code
******************************************************************************/
                    
/*****************************************************************************
* Function Name   :   R_USB_CdcInit
* Description     :   Initialise this module.
*                     This must be called once before using any of the
*                     other API functions.
*                     Initialise the USB Core layer.
* Argument        :   _pchannel: current peripheral
*                                As code is reentrant functions should not
*                                reference local variables
* Return value    :   - USB_ERR_OK
*****************************************************************************/
usb_err_t R_USB_CdcInit(volatile st_usb_object_t *_pchannel)
{
    _pchannel->err =  USB_ERR_OK;
    
    /* Initialise the data */
    if(initialise_data(_pchannel))
    {
        /*Initialise the USB core*/
        _pchannel->err = R_USBF_CoreInit(_pchannel,
                        g_string_desc_manufacturer.puc_data,
                       g_string_desc_manufacturer.length,
                       g_string_desc_product.puc_data,
                       g_string_desc_product.length,
                       g_string_desc_serial_num.puc_data,
                       g_string_desc_serial_num.length,
                       g_device_descriptor.puc_data,
                       g_device_descriptor.length,
                       g_desc_device_qualifier.puc_data,
                       g_desc_device_qualifier.length,
                       g_configuration_descriptor.puc_data,
                       g_configuration_descriptor.length,
                       g_desc_other_speed_config.puc_data,
                       g_desc_other_speed_config.length,
                       (CB_SETUP_PACKET)cb_unhandled_setup_packet,
                       (CB_DONE_OUT)cb_done_control_out,
                       (CB_CABLE)cb_cable,
                       (CB_ERROR)cb_error);
    }

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_CdcInit
******************************************************************************/

/*****************************************************************************
* Function Name   :    R_USB_CdcIsConnected
* Description     :    Get the USB cable connected state.
* Argument        :    -
* Return value    :    TRUE = Connected, FALSE = Disconnected.
*****************************************************************************/
BOOL R_USB_CdcIsConnected(volatile st_usb_object_t *_pchannel)
{
    return (BOOL)_pchannel->connected;
}
/******************************************************************************
End of function R_USB_CdcIsConnected
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_CdcGetChar
* Description     :   Read 1 character (BULK OUT).
*                     Note: As this is a blocking function it must not be
*                     called from an interrupt with higher priority than
*                     the USB interrupts.
* Argument        :   _pChar: Pointer to Byte to read into.
* Return value    :   Error code.
******************************************************************************/
usb_err_t R_USB_CdcGetChar(volatile st_usb_object_t *_pchannel, uint8_t* _pChar)
{
    _pchannel->err = USB_ERR_OK;
    uint32_t dummy;
    
    _pchannel->err = R_USB_CdcRead(_pchannel, 1, _pChar, &dummy);
    
    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_CdcGetChar
******************************************************************************/   

/*****************************************************************************
* Function Name   :   R_USB_CdcPutChar
* Description     :   Write 1 character (BULK IN).
*                     Note: As this is a blocking function it must not be
*                     called from an interrupt with higher priority than
*                     the USB interrupts.
* Argument        :   _pchannel: current peripheral
*                     _char: Byte to write.
* Return value    :    Error code.
*****************************************************************************/
usb_err_t R_USB_CdcPutChar(volatile st_usb_object_t *_pchannel, uint8_t _char)
{
    return R_USB_CdcWrite(_pchannel, 1, &_char);
}
/******************************************************************************
End of function R_USB_CdcPutChar
******************************************************************************/   

/******************************************************************************
* Function Name   :   R_USB_CdcWrite
* Description     :   Perform a blocking write (BULK IN)
*                     Note: As this is a blocking function it must not be
*                     called from an interrupt with higher priority than
*                     the USB interrupts.
* Argument        :   _pchannel: current peripheral
*                     _num_bytes: Number of bytes to write.
*                     _pbuffer:   Data Buffer.
* Return value    :   Error code.
******************************************************************************/
usb_err_t R_USB_CdcWrite(volatile st_usb_object_t *_pchannel, uint32_t _num_bytes, uint8_t* _pbuffer)
{
    _pchannel->err = USB_ERR_OK;
    
    /*This can not complete until connected*/
    if(TRUE == _pchannel->connected)
    {
        /*Check not already busy*/
        if(FALSE == _pchannel->bulk_in.m_busy)
        {
            /*Set Flag to wait on*/
            _pchannel->bulk_in.m_busy = TRUE;
            _pchannel->bulk_in.m_cb_done = NULL; /*No call back this is a blocking function */
            _pchannel->err = R_USB_HalBulkIn(_pchannel, _num_bytes, _pbuffer, (CB_DONE_BULK_IN)cb_done_bulk_in);
            if(USB_ERR_OK == _pchannel->err)
            {
                while(TRUE == _pchannel->bulk_in.m_busy)
                {
                    /*Wait for flag that will be cleared when the operation has completed*/
                }
                _pchannel->err = _pchannel->bulk_in.m_err;
            }
            else
            {
                _pchannel->bulk_in.m_busy = FALSE;
            }
        }
        else
        {
            _pchannel->err = USB_ERR_BUSY;
        }
    }
    else
    {
        _pchannel->err = USB_ERR_NOT_CONNECTED;
    }
    
    return _pchannel->err;
}
/*****************************************************************************
End of function R_USB_CdcWrite
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_CdcWriteAsync
* Description     :   Start an asynchronous write. (BULK IN)
* Argument        :   _num_bytes:     Number of bytes to write.
*                     _pbuffer:    Data Buffer.
*                     _cb:         Callback when done.
* Return value    :   Error Code.
*****************************************************************************/
usb_err_t R_USB_CdcWriteAsync(volatile st_usb_object_t *_pchannel, uint32_t _num_bytes, uint8_t* _pbuffer, CB_DONE _cb)
{
    _pchannel->err = USB_ERR_OK;
    
    /*This can not complete until connected*/
    if(TRUE == _pchannel->connected)
    {
        /*Check not already busy*/
        if(FALSE == _pchannel->bulk_in.m_busy)
        {
            /*Set Flag to wait on*/
            _pchannel->bulk_in.m_busy = TRUE;
            _pchannel->bulk_in.m_cb_done = _cb;
            _pchannel->err = R_USB_HalBulkIn(_pchannel, _num_bytes, _pbuffer, (CB_DONE_BULK_IN)cb_done_bulk_in);
            if(USB_ERR_OK != _pchannel->err)
            {
                _pchannel->bulk_in.m_busy = FALSE;
            }
        }
        else
        {
            _pchannel->err = USB_ERR_BUSY;
        }
    }
    else
    {
        _pchannel->err = USB_ERR_NOT_CONNECTED;
    }
    
    return _pchannel->err;
}
/*****************************************************************************
End of function R_USB_CdcWriteAsync
******************************************************************************/   

/******************************************************************************
* Function Name   :   R_USB_CdcRead
* Description     :   Perform a blocking read. (BULK OUT)
*                     Note: As this is a blocking function it must not be
*                     called from an interrupt with higher priority than
*                     the USB interrupts.
* Argument        :   _pbufferSize:     Buffer Size
*                     _pbuffer:        Buffer to read data into.
*                     _pNumBytesRead: (OUT)Number of bytes read. 
* Return value    :   Error Code.
*****************************************************************************/
usb_err_t R_USB_CdcRead(volatile st_usb_object_t *_pchannel, uint32_t _pbufferSize, uint8_t* _pbuffer, uint32_t* _pNumBytesRead)
{
    _pchannel->err = USB_ERR_OK;
    
    /*This can not complete until connected*/
    if(TRUE == _pchannel->connected)
    {
        /*Check not already busy*/
        if(FALSE == _pchannel->bulk_out.m_busy)
        {
            /*Set Flag to wait on*/
            _pchannel->bulk_out.m_busy = TRUE;

            /*No call back this is a blocking function */
            _pchannel->bulk_out.m_cb_done = NULL;
            _pchannel->err = R_USB_HalBulkOut(_pchannel,
                                  _pbufferSize,
                                  _pbuffer,
                                  (CB_DONE_OUT)cb_done_bulk_out);
            if(USB_ERR_OK == _pchannel->err)
            {
                while(TRUE == _pchannel->bulk_out.m_busy)
                {
                    /*Wait for flag that will be cleared when the operation has completed*/
                    R_OS_TaskSleep(10);
                }
        
                *_pNumBytesRead = _pchannel->bulk_out.m_bytes_read;
                _pchannel->err = _pchannel->bulk_out.m_err;
            }
            else
            {
                _pchannel->bulk_out.m_busy = FALSE;
            }
        }
        else
        {
            _pchannel->err = USB_ERR_BUSY;
        }
    }
    else
    {
        _pchannel->err = USB_ERR_NOT_CONNECTED;
    }
    
    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_CdcRead
******************************************************************************/   

/*****************************************************************************
* Function Name   :   R_USB_CdcReadAsync
* Description     :   Start an asynchronous read. (BULK OUT)
* Argument        :   _pbufferSize:     Buffer Size
*                     _pbuffer:        Buffer to read data into.
*                     _cb: Callback when done.
* Return value    :   Error Code.
*****************************************************************************/
usb_err_t R_USB_CdcReadAsync(volatile st_usb_object_t *_pchannel, uint32_t _pbufferSize, uint8_t* _pbuffer, CB_DONE_OUT _cb)
{
    _pchannel->err = USB_ERR_OK;
    
    /*This can not complete until connected*/
    if(TRUE == _pchannel->connected)
    {
        /*Check not already busy*/
        if(FALSE == _pchannel->bulk_out.m_busy)
        {
            /*Set Flag to wait on*/
            _pchannel->bulk_out.m_busy = TRUE;
            _pchannel->bulk_out.m_cb_done = _cb;
            _pchannel->err = R_USB_HalBulkOut(_pchannel,
                                  _pbufferSize,
                                  _pbuffer,
                                 (CB_DONE_OUT)cb_done_bulk_out);
                                 
            if(USB_ERR_OK != _pchannel->err)
            {
                _pchannel->bulk_out.m_busy = FALSE;
            }
        }
        else
        {
            _pchannel->err = USB_ERR_BUSY;
        }
    }
    else
    {
        _pchannel->err = USB_ERR_NOT_CONNECTED;
    }
    
    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_CdcReadAsync
******************************************************************************/   

/******************************************************************************
* Function Name   :    R_USB_CdcCancel
* Description     :    Cancel waiting on any blocking functions.
* Return value    :    Error code.
******************************************************************************/
usb_err_t R_USB_CdcCancel(volatile st_usb_object_t * _pchannel)
{
    _pchannel->err = USB_ERR_OK;
    
    release_flags(_pchannel, USB_ERR_CANCEL);
    
    /*Reset HAL*/
    _pchannel->err = R_USB_HalReset(_pchannel);
    
    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_CdcCancel
******************************************************************************/   

/*****************************************************************************
* Function Name   :   cb_cable
* Description     :   Callback when the USB cable is connected or disconnected.
* Argument        :   _bConnected: TRUE = Connected, FALSE = Disconnected.
* Return value    :   -
*****************************************************************************/
static void cb_cable(volatile st_usb_object_t *_pchannel, BOOL _bConnected)
{    
    if(TRUE == _bConnected)
    {
        DEBUG_MSG_HIGH( ("USBCDC: Cable Connected\r\n"));
        
        /* Initialise data - as this is like re-starting */
        initialise_data(_pchannel);
        
        _pchannel->connected = TRUE;
    }
    else
    {
        DEBUG_MSG_HIGH( ("USBCDC: Cable Disconnected\r\n"));
        _pchannel->connected = FALSE;
        
        /*In case we are waiting on any flags set them here*/
        release_flags(_pchannel, USB_ERR_NOT_CONNECTED);
    }
} 
/******************************************************************************
End of function cb_cable
******************************************************************************/   

/******************************************************************************
* Function Name   :   cb_error
* Description     :   Callback saying that an error has occurred in a
*                     lower layer.
* Argument        :   _err - error code.
* Return value    :   -
*****************************************************************************/
static void cb_error(volatile st_usb_object_t *_pchannel, usb_err_t _err)
{
    DEBUG_MSG_HIGH(("USBCDC: ***cb_error***\r\n"));
    
    /*If the terminal sends data to us when we're not expecting any
    then would get this and it's not an error.
    However, could be that data is arriving too quick for us to read
    and echo out.*/
    if(USB_ERR_BULK_OUT_NO_BUFFER == _err)
    {
        DEBUG_MSG_HIGH(("USBCDC: No Bulk Out Buffer.\r\n"));
    }
    else
    {
        /*Try resetting HAL*/
        R_USB_HalReset(_pchannel);
        
        /* configure and Reset Endpoints */
        R_USB_HalResetEp(_pchannel, 1);
        
        /*Release flags in case we are waiting on any*/
        release_flags(_pchannel, _err);
    }
}
/******************************************************************************
End of function cb_error
******************************************************************************/   

/*****************************************************************************
* Function Name   :   cb_unhandled_setup_packet
* Description     :   Called from the USB Core when it can't deal with
*                     a setup packet.
*                     Provides a buffer if there is a data stage.
*                     Expect the core to deal with Standard requests so
*                     this should be Class specific.
*                     This is a function of type CB_SETUP_PACKET.
* Argument        :   _pSetupPacket - Setup packet.
*                     _pNumBytes - (OUT)Buffer size.
*                     _ppBuffer - (OUT)Buffer.    
* Return value    :    Error code
*****************************************************************************/
static usb_err_t cb_unhandled_setup_packet(volatile st_usb_object_t *_pchannel,
                                        setup_packet_t* _pSetupPacket,
                                        uint16_t* _pNumBytes,
                                        uint8_t** _ppBuffer)
{
    _pchannel->err = USB_ERR_OK;
    
    switch(_pSetupPacket->bm_request.bit_val.d65)
    {
        case REQUEST_CLASS:
        {
            _pchannel->err = process_class_setup_packet(_pchannel, _pSetupPacket, _pNumBytes, _ppBuffer);
            break;
        }
        case REQUEST_VENDOR:
        case REQUEST_STANDARD:
        default:
        {
            DEBUG_MSG_HIGH(("USBCDC: Unsupported Request type\r\n"));
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }
    
    return _pchannel->err;
}
/******************************************************************************
End of function cb_unhandled_setup_packet
******************************************************************************/   

/******************************************************************************
* Function Name   :   cb_done_bulk_out
* Description     :   Callback when a USBHAL_Bulk_OUT request has completed.
*                     Clear busy flag.
*                     Call registered callback.
* Argument        :   _err - Error code.
*                     _num_bytes - The number of bytes received from the host.
* Return value    :   -
*****************************************************************************/
static void cb_done_bulk_out(volatile st_usb_object_t *_pchannel, usb_err_t _err, uint32_t NumBytes)
{
    _pchannel->bulk_out.m_bytes_read = NumBytes;
    _pchannel->bulk_out.m_err = _err;
    _pchannel->bulk_out.m_busy = FALSE;
    
    /*If a call back was registered then call it.*/
    if(NULL != _pchannel->bulk_out.m_cb_done)
    {
        _pchannel->bulk_out.m_cb_done(_pchannel, _err, NumBytes);
    }
}
/******************************************************************************
End of function cb_done_bulk_out
******************************************************************************/   

/*****************************************************************************
* Function Name   :   cb_done_bulk_in
* Description     :   A Bulk IN request has completed.
*                     Clear busy flag.
*                     Call registered callback.
* Argument        :   err - error code.
* Return value    :   -
*****************************************************************************/
static void cb_done_bulk_in(volatile st_usb_object_t *_pchannel, usb_err_t _err)
{            
    _pchannel->bulk_in.m_busy = FALSE;

    /*If a call back was registered then call it.*/
    if(NULL != _pchannel->bulk_in.m_cb_done)
    {
        _pchannel->bulk_in.m_cb_done(_err);
    }
}
/******************************************************************************
End of function cb_done_bulk_in
******************************************************************************/   

/*****************************************************************************
* Function Name   :   cb_done_control_out
* Description     :   A Control Out has completed in response to a
*                     setup packet handled in cb_unhandled_setup_packet.
* Argument        :   _err - Error code.
*                     _num_bytes - The number of bytes received from the host.
* Return value    :   -
*****************************************************************************/
static void cb_done_control_out(volatile st_usb_object_t *_pchannel,
                             usb_err_t _err,
                             uint32_t _num_bytes)
{
    (void) _num_bytes;
    assert(USB_ERR_OK == _err);

    /*Assume this is SET_LINE_CODING data as it is the
    only control out this deals with.*/
    
    /*Construct dw_dte_rate to avoid Endian issues.
    NOTE: Buffer always arrives in little Endian*/
    _pchannel->set_control_line_data.dw_dte_rate =
            ((uint32_t)((_pchannel->control_line_state_pbuffer[0]) << 0) &  0x000000FF);
    _pchannel->set_control_line_data.dw_dte_rate |=
            ((uint32_t)((_pchannel->control_line_state_pbuffer[1]) << 8) &  0x0000FF00);
    _pchannel->set_control_line_data.dw_dte_rate |=
            ((uint32_t)((_pchannel->control_line_state_pbuffer[2]) << 16) & 0x00FF00FF);
    _pchannel->set_control_line_data.dw_dte_rate |=
            ((uint32_t)((_pchannel->control_line_state_pbuffer[3]) << 24) & 0xFF000000);
    
    
    _pchannel->set_control_line_data.b_char_format = _pchannel->control_line_state_pbuffer[4];
    _pchannel->set_control_line_data.b_parity_type = _pchannel->control_line_state_pbuffer[5];
    _pchannel->set_control_line_data.b_data_bits = _pchannel->control_line_state_pbuffer[6];
    
    
    DEBUG_MSG_MID(("USBCDC: SET_LINE_CODING data received.\r\n"));
    DEBUG_MSG_MID(("USBCDC: dw_dte_rate = %lu\r\n", _pchannel->set_control_line_data.dw_dte_rate));
    DEBUG_MSG_MID(("USBCDC: b_char_format = %d\r\n", _pchannel->set_control_line_data.b_char_format));
    DEBUG_MSG_MID(("USBCDC: b_parity_type = %d\r\n", _pchannel->set_control_line_data.b_parity_type));
    DEBUG_MSG_MID(("USBCDC: b_data_bits = %d\r\n", _pchannel->set_control_line_data.b_data_bits));
    
    /*Don't need to do anything with this data*/
}
/******************************************************************************
End of function cb_done_control_out
******************************************************************************/   

/*****************************************************************************
* Function Name   :   process_class_setup_packet
* Description     :   Processes a CDC class setup packet.
*                     Provides a buffer if there is a data stage. 
* Argument        :   _pSetupPacket - Setup packet.
*                     _pNumBytes - (OUT)Buffer size.
*                     _ppBuffer - (OUT)Buffer.
* Return value    :   Error code
*****************************************************************************/
static usb_err_t process_class_setup_packet(volatile st_usb_object_t *_pchannel,
                                         setup_packet_t* _pSetupPacket,
                                         uint16_t* _pNumBytes,
                                         uint8_t** _ppBuffer)
{
    _pchannel->err = USB_ERR_OK;
    
    switch(_pSetupPacket->b_request)
    {
        case GET_LINE_CODING:
        {
            DEBUG_MSG_HIGH(("USBCDC: GET_LINE_CODING\r\n"));
                
            /*Data IN response */
            *_pNumBytes = LINE_CODING_DATA_SIZE;
            *_ppBuffer = (uint8_t*)_pchannel->line_coding;
            break;
        }

        /*(Required for hyper-terminal)*/
        case SET_LINE_CODING: 
        {
            DEBUG_MSG_HIGH(("USBCDC: SET_LINE_CODING\r\n"));

            /*No action required for this request.*/
            /*Data OUT*/
            *_pNumBytes = SET_CONTROL_LINE_STATE_DATA_SIZE;
            *_ppBuffer = (uint8_t *)_pchannel->control_line_state_pbuffer;
            break;
        }
        case SET_CONTROL_LINE_STATE:
        {
            DEBUG_MSG_HIGH(("USBCDC: SET_CONTROL_LINE_STATE\r\n"));

            /*No action required for this request.*/
            /*No data response */
            *_pNumBytes = 0;        
            break;
        }
        default:
        {
            DEBUG_MSG_HIGH(("USBCDC: Unsupported Class request %d\r\n",
                         _pSetupPacket->bRequest));
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }

    return _pchannel->err;
}
/******************************************************************************
End of function process_class_setup_packet
******************************************************************************/   

/******************************************************************************
* Function Name   :   release_flags
* Description     :   Set Error codes and release and clear busy flags.
*                     To be used when an error has been detected and need
*                     to ensure this module isn't waiting on a callback
*                     that will not come.
* Argument        :   err - Error Code
* Return value    :   -
*****************************************************************************/
static void release_flags(volatile st_usb_object_t *_pchannel, usb_err_t _err)
{
    _pchannel->bulk_in.m_err   = _err;
    _pchannel->bulk_out.m_err  = _err;
    _pchannel->bulk_in.m_busy  = FALSE;
    _pchannel->bulk_out.m_busy = FALSE;
}
/******************************************************************************
End of function release_flags
******************************************************************************/   

/*****************************************************************************
* Function Name   :   initialise_data
* Description     :   Initialise this modules data.
*                     Put into a function so that it can be done each time
*                     the USB cable is connected.
* Argument        :   -
* Return value    :   -
*****************************************************************************/
static uint16_t initialise_data(volatile st_usb_object_t *_pchannel)
{    
    /*Bulk Out*/
    /*Busy Flag*/
    _pchannel->bulk_out.m_busy = FALSE;

    /*Number of bytes read*/
    _pchannel->bulk_out.m_bytes_read = 0;

    /*Error Status*/
    _pchannel->bulk_out.m_err = USB_ERR_OK;

    /*Callback done*/
    _pchannel->bulk_out.m_cb_done = NULL;
    
    /*Bulk In*/
    /*Busy Flag*/
    _pchannel->bulk_in.m_busy = FALSE;

    /*Error Status*/
    _pchannel->bulk_in.m_err = USB_ERR_OK;

    /*Callback done*/
    _pchannel->bulk_in.m_cb_done = NULL;

    _pchannel->pend_pnt[0] = end_ptbl_1;
    _pchannel->pend_pnt[1] = end_ptbl_2;
    _pchannel->pend_pnt[2] = end_ptbl_3;
    _pchannel->pend_pnt[3] = end_ptbl_4;
    _pchannel->pend_pnt[4] = end_ptbl_5;

    _pchannel->control_line_state_pbuffer[0] = 0;
    _pchannel->control_line_state_pbuffer[1] = 0;
    _pchannel->control_line_state_pbuffer[2] = 0;
    _pchannel->control_line_state_pbuffer[3] = 0;
    _pchannel->control_line_state_pbuffer[4] = 0;
    _pchannel->control_line_state_pbuffer[5] = 0;
    _pchannel->control_line_state_pbuffer[6] = 0;

    /*GET_LINE_CODING data response*/
    /*Note: These values are not relevant for a virtual COM port.*/
    _pchannel->line_coding[0] = 0x00;
    _pchannel->line_coding[1] = 0xc2;
    _pchannel->line_coding[2] = 0x01;
    _pchannel->line_coding[3] = 0x00;
    _pchannel->line_coding[4] = 0x00;
    _pchannel->line_coding[5] = 0x00;
    _pchannel->line_coding[6] = 0x08;

    return (1);
}
/******************************************************************************
End of function initialise_data
******************************************************************************/   

///*****************************************************************************
//* Function Name   :    set_connected_status
//* Description     :    Set the USB cable connected state.
//* Argument        :    -
//* Return value    :    TRUE = Connected, FALSE = Disconnected.
//*****************************************************************************/
//static void set_connected_status(volatile st_usb_object_t *_pchannel, BOOL _status )
//{
//   _pchannel->connected = _status;
//}
///******************************************************************************
//End of function set_connected_status
//******************************************************************************/
//
