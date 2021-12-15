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
* File Name       : R_USBF_Core.c
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      :
* H/W Platform    : RSK2+RZA!H
* Description     : The USB core sits above the HAL in the USB stack.
*                   Initialises the HAL.
*                   Handles standard setup packets.
* 
* NOTE            : This only supports 1 language ID and that is currently
*                   English. defined in static string_desc_language_ids
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

/******************************************************************************
System Includes (Project Level Includes)
******************************************************************************/
/* The following header file defines the assert() macro */
#include <assert.h>

/******************************************************************************
User Includes (Project Level Includes)
******************************************************************************/
/*    Following header file provides definition for Low level driver. */
#include "r_usb_hal.h"
/*    Following header file provides definition for usb_core.c. */
#include "usb_common.h"
/*    Following header file provides definition for usb_core.c. */
#include "r_usbf_core.h"

#include "r_lib_int.h"

/******************************************************************************
Type Definitions
******************************************************************************/

/*Language ID - Currently only supports English*/
#define STRING_LANGUAGE_IDS_SIZE (0x04)

/*English USA*/
#define STRING_LANGUAGE_ID (0x0409)

/******************************************************************************
Function Prototypes
******************************************************************************/
static void cb_setup(volatile void *__pchannel, const uint8_t(*_pSetupPacket)[USB_SETUP_PACKET_SIZE]);

static void populate_setup_packet(volatile st_usb_object_t *_pchannel,
                                const uint8_t(*_pSetupPacket)[USB_SETUP_PACKET_SIZE]);

static usb_err_t process_setup_packet(volatile st_usb_object_t *_pchannel,
                                    uint16_t* _pNumBytes,
                                    uint8_t** _ppBuffer);

static usb_err_t process_standard_setup_packet(volatile st_usb_object_t *_pchannel,
                                            uint16_t* _pNumBytes,
                                            uint8_t** _ppBuffer);

static usb_err_t process_get_descriptor(volatile st_usb_object_t *_pchannel,
                                      uint16_t* _pNumBytes,
                                      const uint8_t** _ppDescriptor);

static usb_err_t process_get_descriptor_string(volatile st_usb_object_t *_pchannel,
                                            uint16_t* _pNumBytes,
                                            const uint8_t** _ppDescriptor);

static usb_err_t process_get_configuration(volatile st_usb_object_t *_pchannel,
                                         uint16_t* _pNumBytes,
                                         const uint8_t** _ppConfigValue);

static usb_err_t process_get_status(volatile st_usb_object_t *_pchannel,
                                  uint16_t* _pNumBytes,
                                  const uint8_t** _ppStatus);

static usb_err_t process_set_feature(volatile st_usb_object_t *_pchannel);

static usb_err_t process_clear_feature(volatile st_usb_object_t *_pchannel);

/******************************************************************************
Global Variables
******************************************************************************/

static uint8_t string_desc_language_ids[STRING_LANGUAGE_IDS_SIZE] =
{
    /*Length of this*/
    STRING_LANGUAGE_IDS_SIZE,

    /*Descriptor Type = STRING*/
    0x03,
    (STRING_LANGUAGE_ID & 0xFF),
    ((STRING_LANGUAGE_ID >> 8) & 0xFF),
};

/******************************************************************************
User Program Code
******************************************************************************/

PEVENT          g_usb_devices_events[R_USB_SUPPORTED_CHANNELS];

/*******************************************************************************
* Function Name  : R_USBF_CoreInit
* Description    : Initialise the USB core.
*                  Descriptors are supplied so that this can handle standard
*                  GET_DESCRIPTOR requests.
*                  Initialises the HAL.
* Argument       : _Manufacturer:  Manufacturer String Descriptor
*                  _ManufacturerSize: Manufacturer String Descriptor Size
*                  _Product:  Product String Descriptor
*                  _ProductSize: Product String Descriptor Size
*                  _Serial:  Serial String Descriptor
*                  _SerialSize: Serial String Descriptor Size
*                  _DeviceDescriptor: Device descriptor
*                  _ConfigDescriptor: Configuration Descriptor
*                  _fpcb_setupPacket: Function that will be called if
*                                a setup packet can't be handled by this layer.
*                    (Usually a Vender or Class specific GetDescriptor request)
*                  _fpCBControlOut: Function that will be called when 
*                        a control data out occurs for a setup packet
*                        not handled by this layer.
*                  _fpCBCable: Function that will be called when USB cable is
*                        connected/disconnected
*                  _fpCBError: Function that will be called when an unhandled
*                  error occurs.
* Return value  : Error code.
******************************************************************************/
usb_err_t R_USBF_CoreInit(volatile st_usb_object_t *_pchannel,
                     uint8_t* _Manufacturer, uint16_t _ManufacturerSize,
                     uint8_t* _Product, uint16_t _ProductSize,
                     uint8_t* _Serial, uint16_t _SerialSize,
                     uint8_t* _DeviceDescriptor, uint16_t _DeviceDescriptorSize,
                     uint8_t* _DeviceQDescriptor, uint16_t _DeviceQDescriptorSize,
                     uint8_t* _ConfigDescriptor, uint16_t _ConfigDescriptorSize,
                     uint8_t* _OtherConfigDescriptor, uint16_t _OtherConfigDescriptorSize,
                     CB_SETUP_PACKET _fpcb_setupPacket,
                     CB_DONE_OUT _fpCBControlOut,
                     CB_CABLE _fpCBCable,
                     CB_ERROR _fpCBError)
{
    _pchannel->err = USB_ERR_OK;
    
    if(0 == _pchannel->descriptors_initialised)
    {
        /*Update passed in descriptors*/
        _pchannel->descriptors.device.puc_data = _DeviceDescriptor;
        _pchannel->descriptors.device.length = _DeviceDescriptorSize;

        _pchannel->descriptors.config.puc_data = _ConfigDescriptor;
        _pchannel->descriptors.config.length = _ConfigDescriptorSize;

        _pchannel->descriptors.string_manufacturer.puc_data = _Manufacturer;
        _pchannel->descriptors.string_manufacturer.length = _ManufacturerSize;

        _pchannel->descriptors.string_product.puc_data = _Product;
        _pchannel->descriptors.string_product.length = _ProductSize;

        _pchannel->descriptors.string_serial.puc_data = _Serial;
        _pchannel->descriptors.string_serial.length = _SerialSize;

        _pchannel->descriptors.dev_qualifier.puc_data = _DeviceQDescriptor;
        _pchannel->descriptors.dev_qualifier.length = _DeviceQDescriptorSize;

        _pchannel->descriptors.other_speed_config.puc_data = _OtherConfigDescriptor;
        _pchannel->descriptors.other_speed_config.length = _OtherConfigDescriptorSize;

        _pchannel->descriptors_initialised++;
    }

    /*Store passed in callback*/
    _pchannel->fp_cbc_setup_packet = _fpcb_setupPacket;
    _pchannel->fp_cb_control_out   = _fpCBControlOut;
    
    /*keep track of  current USB configuration*/
    _pchannel->pcurr_usb_config = _pchannel->descriptors.config.puc_data;

    /* store static language descriptor description */
    _pchannel->descriptors.string_language_ids.puc_data = string_desc_language_ids;
    _pchannel->descriptors.string_language_ids.length = string_desc_language_ids[0];

    _pchannel->connected = FALSE;

    /* Read/Write Mode */
    _pchannel->rw_mode = USBF_NORMAL;

    _pchannel->callbacks.p_cb_bin_mfpdone = NULL;
    _pchannel->callbacks.p_cb_bout_mfpdone = NULL;
    _pchannel->callbacks.p_cb_cout_mfpdone = NULL;
    _pchannel->callbacks.p_cb_iin_mfpdone = NULL;

    /*Initialise USB HAL*/
    _pchannel->err = R_USB_HalInit(_pchannel, cb_setup, _fpCBCable, _fpCBError);

    return _pchannel->err;
}
/******************************************************************************
End of function R_USBF_CoreInit
******************************************************************************/

/******************************************************************************
* Function Name   :  cb_setup
* Description     :  Callback from HAL when it has received a setup packet.
*                     Responds with either an CONTROL IN, OUT, ACK or STALL.
*                     If this layer can not deal with the setup packet
*                     on its own it will call an upper layer to see if it
*                     knows how to deal with it.
* Argument        :    _pSetupPacket - buffer containing the setup packet.
* Return value    :    None
******************************************************************************/

static void cb_setup(volatile void *__pchannel, const uint8_t(*_pSetupPacket)[USB_SETUP_PACKET_SIZE])
{
    volatile st_usb_object_t *_pchannel = (volatile st_usb_object_t *)__pchannel;
    _pchannel->err = USB_ERR_OK;

    DEBUG_MSG_HIGH( ("USBCORE: SetupPacket received\r\n"));
    
    /*Populate the Setup Packet structure g_oSetupPacket */
    populate_setup_packet(_pchannel, _pSetupPacket);
    
    /*Process this setup packet*/
    _pchannel->err = process_setup_packet(_pchannel, (uint16_t *)(&_pchannel->cb_setup_num_bytes), (uint8_t **)(&_pchannel->pcdsetup_buffer));
                            
    if(USB_ERR_UNKNOWN_REQUEST == _pchannel->err)
    {
        DEBUG_MSG_LOW( ("USBCORE: Passing SetupPacket up the stack.\r\n"));
        
        /*Can't handle this setup packet*/
        /*Let upper layer try - call registered callback*/
        _pchannel->err = _pchannel->fp_cbc_setup_packet((void *)_pchannel, 
                                                        (setup_packet_t *)(&_pchannel->setup_packet),
                                                        (uint16_t *)(&_pchannel->cb_setup_num_bytes), 
                                                        (uint8_t **)(&_pchannel->pcdsetup_buffer));
    }
    
    if(USB_ERR_OK == _pchannel->err)
    {
        /*Is there a data stage?*/
        if(0 != _pchannel->setup_packet.w_length)
        {
            /* Is this a Data IN or OUT */
            if(_pchannel->setup_packet.bm_request.bit_val.d7)
            {
                DEBUG_MSG_MID(("USBCORE: SetupPacket DATA IN\r\n"));
                
                /*IN*/
                /*Don't send more data than host has requested*/
                if(_pchannel->cb_setup_num_bytes > _pchannel->setup_packet.w_length)
                {
                    _pchannel->cb_setup_num_bytes = _pchannel->setup_packet.w_length;
                }
            
                /*Send Data*/
                R_USB_HalControlIn(_pchannel, _pchannel->cb_setup_num_bytes, _pchannel->pcdsetup_buffer);
            }
            else
            {
                /*OUT*/
                DEBUG_MSG_MID(("USBCORE: SetupPacket DATA OUT\r\n"));
                
                assert(_pchannel->setup_packet.w_length == _pchannel->cb_setup_num_bytes);
                
                R_USB_HalControlOut(_pchannel, _pchannel->cb_setup_num_bytes, _pchannel->pcdsetup_buffer, _pchannel->fp_cb_control_out);
            }
        }
        else
        {
            /*No data stage - just need to send ACK*/
            DEBUG_MSG_MID(("USBCORE: SetupPacket - No data stage.\r\n"));
            
            R_USB_HalControlAck(_pchannel);
        }
    }
    else
    {
        DEBUG_MSG_MID(("USBCORE: SetupPacket - stall.\r\n"));
        
        /*Something wrong with this control pipe so stall it.*/
        R_USB_HalControlStall(_pchannel);
    }
}
/******************************************************************************
End of function cb_setup
******************************************************************************/

/******************************************************************************
* Function Name   :   process_setup_packet
* Description     :   Looks to see if this is a standard setup packet
*                     that this layer can possibly deal with.
*                  
* Argument        :    _pNumBytes: (OUT)If this layer can handle this then
*                          this will be set with the size of the data.
*                          (If there is no data stage then this will be set to zero)
*                     _ppBuffer: (OUT)If this layer can handle this then
*                          this will be set to point to the data(IN) or a buffer(OUT).
*                          (If there is a data stage for this packet).
* Return value    :    None 
******************************************************************************/

static usb_err_t process_setup_packet(volatile st_usb_object_t *_pchannel, uint16_t* _pNumBytes, uint8_t** _ppBuffer)
{
    _pchannel->err = USB_ERR_OK;
    
    switch(_pchannel->setup_packet.bm_request.bit_val.d65)
    {
        case REQUEST_STANDARD:
        {
            /* Standard Type */
            DEBUG_MSG_HIGH( ("USBCORE: REQUEST_STANDARD\r\n"));
            _pchannel->err = process_standard_setup_packet(_pchannel, _pNumBytes, _ppBuffer);
            break;
        }
        case REQUEST_CLASS:
        case REQUEST_VENDOR:
        default:
        {
            DEBUG_MSG_LOW( ("USBCORE: Core can't process this setup packet: %d\r\n", _pchannel->setup_packet.bm_request.bit_val.d65));
        
            /*Unsupported request*/
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }
    
    return _pchannel->err;
}
/******************************************************************************
End of function process_setup_packet
******************************************************************************/

/******************************************************************************
* Function Name   :   process_standard_setup_packet
* Description     :   Checks to see if this standard setup packet can be
*                     handled by this layer.
*                     Handles GET_DESCRIPTOR.
* Argument        :   _pNumBytes: (OUT)If this layer can handle this then
*                          this will be set with the size of the data.
*                          (If there is no data stage then this will be
*                          set to zero)
*                    _ppBuffer: (OUT)If this layer can handle this then
*                          this will be set to point to the data(IN)
*                          or a buffer(OUT).
*                          (If there is a data stage for this packet).
* Return value    :   Error code. - USB_ERR_OK if supported.
******************************************************************************/

static usb_err_t process_standard_setup_packet(volatile st_usb_object_t *_pchannel,
                                            uint16_t* _pNumBytes,
                                            uint8_t** _ppBuffer)
{
    _pchannel->err = USB_ERR_OK;
    switch(_pchannel->setup_packet.b_request)
    {
        case GET_STATUS:
        {
            _pchannel->err = process_get_status(_pchannel, _pNumBytes, (const uint8_t**)_ppBuffer);
            break;
        }
        case CLEAR_FEATURE:
        {
            _pchannel->err = process_clear_feature(_pchannel);
            break;
        }
        case SET_FEATURE:
        {
            _pchannel->err = process_set_feature(_pchannel);
            break;
        }
        case GET_DESCRIPTOR:
        {
            _pchannel->err = process_get_descriptor(_pchannel, _pNumBytes, (const uint8_t**)_ppBuffer);
            break;
        }
        case GET_CONFIGURATION:
        {
            _pchannel->err = process_get_configuration(_pchannel, _pNumBytes, (const uint8_t**)_ppBuffer);
            break;
        }
        case SET_CONFIGURATION:
        {
            DEBUG_MSG_HIGH( ("USBCORE: SET_CONFIGURATION\r\n"));
            break;
        }
        case GET_INTERFACE:
        {
            *_pNumBytes = 0;
            
            _pchannel->usb_status = 0;
            *_ppBuffer = (uint8_t*)&_pchannel->usb_status;
            break;
        }
        case SET_INTERFACE:
        {
            if((0 == _pchannel->setup_packet.w_index) || (1 == _pchannel->setup_packet.w_index))
            {
                /* Do Nothing */
            	;
            }
            else
            {
                _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
            }
            break;
        }
        default:
        {
            /*Unsupported request*/
            DEBUG_MSG_MID( ("USBCORE: Core can't process this standard setup packet\r\n"));
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }
    return _pchannel->err;
}
/******************************************************************************
End of function process_standard_setup_packet
******************************************************************************/

/******************************************************************************
* Function Name   :   process_set_feature
* Description     :   Process a GET_DESCRIPTOR setup packet.
*                     Supports Device, Configuration and String requests.
* 
* Argument        :    _pNumBytes: (OUT)If this layer can handle this then
*                          this will be set with the size of the descriptor.
*                     _ppDescriptor: (OUT)If this layer can handle this then
*                          this will be set to point to the descriptor.
*             
* Return value    :    Error code - USB_ERR_OK if supported. 
******************************************************************************/
static usb_err_t process_set_feature(volatile st_usb_object_t *_pchannel)
{
    _pchannel->err = USB_ERR_OK;

    DEBUG_MSG_HIGH( ("USBCORE: SET_FEATURE\r\n"));
    
    if (((uint8_t)(RECIPIENT_END_POINT == _pchannel->setup_packet.bm_request.bit_val.d40))
        && 
        (ENDPOINT_HALT == _pchannel->setup_packet.w_value))
    {
        switch((uint8_t)(_pchannel->setup_packet.w_index & 0x0F))
        {
            case PIPE0:
            {
                R_LIB_SetPipeNACKtoBUF(_pchannel, PIPE0);
                R_LIB_SetPipeBUFtoSTALL(_pchannel, PIPE0);
                break;
            }
            case PIPE1:
            {
                R_LIB_SetPipeNACKtoBUF(_pchannel, PIPE1);
                R_LIB_SetPipeBUFtoSTALL(_pchannel, PIPE1);
                break;
            }
            case PIPE2:
            {
                R_LIB_SetPipeNACKtoBUF(_pchannel, PIPE2);
                R_LIB_SetPipeBUFtoSTALL(_pchannel, PIPE2);
                break;
            }
            case PIPE6:
            {
                R_LIB_SetPipeNACKtoBUF(_pchannel, PIPE6);
                R_LIB_SetPipeBUFtoSTALL(_pchannel, PIPE6);
                break;
            }
            default:
            {
                DEBUG_MSG_MID( ("USBCORE: Unsupported Pipe requested\r\n"));
                _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
            }
        }
    }
    else
    {
        DEBUG_MSG_MID( ("USBCORE: Unsupported Feature requested\r\n"));
        _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
    }

    return _pchannel->err;
}
/******************************************************************************
End of function process_set_feature
******************************************************************************/

/******************************************************************************
* Function Name   :   process_clear_feature
* Description     :   Process a GET_DESCRIPTOR setup packet.
*                     Supports Device, Configuration and String requests.
* 
* Argument        :    _pNumBytes: (OUT)If this layer can handle this then
*                          this will be set with the size of the descriptor.
*                     _ppDescriptor: (OUT)If this layer can handle this then
*                          this will be set to point to the descriptor.
*             
* Return value    :    Error code - USB_ERR_OK if supported. 
******************************************************************************/
static usb_err_t process_clear_feature(volatile st_usb_object_t *_pchannel)
{
    _pchannel->err = USB_ERR_OK;

    DEBUG_MSG_HIGH( ("USBCORE: CLEAR_FEATURE\r\n"));

    if (FALSE == _pchannel->config.auto_stall_clear)
    {
        return _pchannel->err;
    }
    if (((uint8_t)(RECIPIENT_END_POINT == _pchannel->setup_packet.bm_request.bit_val.d40))
                                    && (ENDPOINT_HALT == _pchannel->setup_packet.w_value))
    {
        switch((uint8_t)(_pchannel->setup_packet.w_index & 0x0F))
        {
            case PIPE0:
            {
                R_LIB_SetPipeSTALLtoBUF(_pchannel, PIPE0);
                break;
            }
            case PIPE1:
            {
                R_LIB_SetPipeSTALLtoBUF(_pchannel, PIPE1);
                break;
            }
            case PIPE2:
            {
                R_LIB_SetPipeSTALLtoBUF(_pchannel, PIPE2);
                break;
            }
            case PIPE6:
            {
                R_LIB_SetPipeSTALLtoBUF(_pchannel, PIPE6);
                break;
            }
            default:
            {
                DEBUG_MSG_MID( ("USBCORE: Unsupported Pipe requested\r\n"));
                _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
            }
        }
    }
    else
    {
        DEBUG_MSG_MID( ("USBCORE: Unsupported Feature requested\r\n"));
        _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
    }

    return _pchannel->err;
}
/******************************************************************************
End of function process_clear_feature
******************************************************************************/

/******************************************************************************
* Function Name   :   process_get_status
* Description     :   Process a GET_DESCRIPTOR setup packet.
*                     Supports Device, Configuration and String requests.
* 
* Argument        :   _pNumBytes: (OUT)wLength = Two
*                     _ppStatus: (OUT)The data returned is the current
*                          status of the specified recipient.
*             
* Return value    :    Error code - USB_ERR_OK if supported. 
******************************************************************************/
static usb_err_t process_get_status(volatile st_usb_object_t *_pchannel,
                                  uint16_t* _pNumBytes,
                                  const uint8_t** _ppStatus)
{
    _pchannel->err = USB_ERR_OK;

    DEBUG_MSG_HIGH( ("USBCORE: GET_STATUS\r\n"));

    /* number of status bytes are 2 for all recipients */
    *_pNumBytes = 2u;

    switch((uint8_t)_pchannel->setup_packet.bm_request.bit_val.d40)
    {
        case RECIPIENT_DEVICE:
        {
            _pchannel->usb_status = 0;
            *_ppStatus = (uint8_t*)&_pchannel->usb_status;
            break;
        }
        case RECIPIENT_INTERFACE:
        {
            _pchannel->usb_status = 0;
            *_ppStatus = (uint8_t*)&_pchannel->usb_status;
            break;
        }
        case RECIPIENT_END_POINT:
        {
            _pchannel->endpoint_status = R_USB_HalIsEndpointStalled(_pchannel, (_pchannel->setup_packet.w_index & 0x0F));
            _pchannel->endpoint_status = (uint16_t)((uint16_t) (((uint16_t) (_pchannel->endpoint_status >> 8)) & 0xff)
            		                               + (uint16_t) (((uint16_t)(_pchannel->endpoint_status << 8)) & 0xff00));
            *_ppStatus = (uint8_t*)&_pchannel->endpoint_status;
            break;
        }
        default:
        {
            /*Unknown recipient*/
            *_pNumBytes = 0;
            *_ppStatus = NULL;
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }

    return _pchannel->err;
}
/******************************************************************************
End of function process_get_status
******************************************************************************/

/******************************************************************************
* Function Name   :   process_get_descriptor
* Description     :   Process a GET_DESCRIPTOR setup packet.
*                     Supports Device, Configuration and String requests.
* 
* Argument        :    _pNumBytes: (OUT)If this layer can handle this then
*                          this will be set with the size of the descriptor.
*                     _ppDescriptor: (OUT)If this layer can handle this then
*                          this will be set to point to the descriptor.
*             
* Return value    :    Error code - USB_ERR_OK if supported. 
******************************************************************************/

static usb_err_t process_get_descriptor(volatile st_usb_object_t *_pchannel,
                                      uint16_t* _pNumBytes,
                                      const uint8_t** _ppDescriptor)
{
    _pchannel->err = USB_ERR_OK;
    
    /*The w_value field of the setup packet,
    specifies the descriptor type in the high byte
    and the descriptor index in the low byte.*/
    
    switch((uint8_t)((_pchannel->setup_packet.w_value >> 8) & 0x00FF))
    {
        case ROOT_DEVICE:
        {
            *_pNumBytes = _pchannel->descriptors.device.length;
            *_ppDescriptor = _pchannel->descriptors.device.puc_data;
            break;
        }
        case CONFIGURATION:
        {
            *_pNumBytes = _pchannel->descriptors.config.length;
            *_ppDescriptor = _pchannel->descriptors.config.puc_data;
            break;
        }
        case STRING:
        {
            _pchannel->err = process_get_descriptor_string(_pchannel, _pNumBytes, _ppDescriptor);
            break;
        }
        case DEVICE_QUALIFIER:
        {
            *_pNumBytes = _pchannel->descriptors.dev_qualifier.length;
            *_ppDescriptor = _pchannel->descriptors.dev_qualifier.puc_data;
            break;
        }
        case OTHER_SPEED_CONFIG:
        {
            *_pNumBytes = _pchannel->descriptors.other_speed_config.length;
            *_ppDescriptor = _pchannel->descriptors.other_speed_config.puc_data;
            break;
        }
        default:
        {
            /*Unknown descriptor request*/
            
            /*Note1: May get a DEVICE_QUALIFIER request - but OK not to handle
            it as we don't support different configurations for high and full speed*/
            
            /*Note2: HID uses this standard GET_DESCRIPTOR to get
            class specific descriptors.*/            
            *_pNumBytes = 0;
            *_ppDescriptor = NULL;
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }
    return _pchannel->err;
}
/******************************************************************************
End of function process_get_descriptor
******************************************************************************/

/******************************************************************************
* Function Name   :    process_get_configuration
* Description     :    Process a GET_DESCRIPTOR setup packet.
*                      Supports Device, Configuration and String requests.
* 
* Argument        :    _pNumBytes: (OUT)If this layer can handle this then
*                          this will be set with the size of the descriptor.
*                      _ppDescriptor: (OUT)If this layer can handle this then
*                          this will be set to point to the descriptor.
*             
* Return value    :    Error code - USB_ERR_OK if supported. 
******************************************************************************/

static usb_err_t process_get_configuration(volatile st_usb_object_t *_pchannel,
                                         uint16_t* _pNumBytes,
                                         const uint8_t** _ppConfigValue)
{
    _pchannel->err = USB_ERR_OK;
    
    DEBUG_MSG_HIGH( ("USBCORE: GET_CONFIGURATION\r\n"));

    _pchannel->device_state = 0;
    R_USB_HalGetDeviceState(_pchannel, (uint8_t *)(&_pchannel->device_state));

    switch(_pchannel->device_state)
    {
        case STATE_DEFAULT:
        {
            /* empty condition */
            break;
        }
        case STATE_ADDRESSED:
        {
            *_pNumBytes = 1u;
            _pchannel->config_value = 0;
            *_ppConfigValue = (uint8_t *)(&_pchannel->config_value);
            break;
        }
        case STATE_CONFIGURED:
        {
            *_pNumBytes = 1u;
            *_ppConfigValue = (uint8_t*)&_pchannel->pcurr_usb_config[CONFIG_TYPE_INDEX];
            break;
        }
        default:
        {
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
            break;
        }
    }

    return _pchannel->err;
}
/******************************************************************************
End of function process_get_configuration
******************************************************************************/

/******************************************************************************
* Function Name   :   process_get_descriptor_string
* Description     :   Process a GET_DESCRIPTOR type STRING.
*                     Handles LanguageID, Manufacturer, Product
*                     and Serial Number requests.
*                     Note: Only supports 1 language ID.
*                 
* Argument        :   _pNumBytes: (OUT)If this layer can handle this then
*                          this will be set with the size of the descriptor.
*                     _ppDescriptor: (OUT)If this layer can handle this then
*                          this will be set to point to the descriptor.
*                  
* Return value    :    Error Code - USB_ERR_OK if supported. 
******************************************************************************/
static usb_err_t process_get_descriptor_string(volatile st_usb_object_t *_pchannel,
                                            uint16_t* _pNumBytes,
                                            const uint8_t** _ppDescriptor)
{
    _pchannel->err = USB_ERR_OK;

    /*The wIndex field of the setup packet is the language ID.
    If wIndex is 0 then this is a request to get the language ID(s) supported*/
    switch(_pchannel->setup_packet.w_index)
    {
        case 0:
        {
            *_pNumBytes = _pchannel->descriptors.string_language_ids.length;
            *_ppDescriptor = _pchannel->descriptors.string_language_ids.puc_data;
        }
        break;
        case STRING_LANGUAGE_ID: /*Get 1st language ID strings*/
        {
            /* What string index is being requested */
            /*The wValue field specifies descriptor index in the low byte.*/
            switch((_pchannel->setup_packet.w_value) & 0x00FF)
            {
                case STRING_iMANUFACTURER:
                {
                    DEBUG_MSG_MID(("String Manufacturer\r\n"));
                
                    *_pNumBytes = _pchannel->descriptors.string_manufacturer.length;
                    *_ppDescriptor = _pchannel->descriptors.string_manufacturer.puc_data;
                    break;
                }
                case STRING_iPRODUCT:
                {
                    DEBUG_MSG_MID(("String Product\r\n"));
                
                    *_pNumBytes = _pchannel->descriptors.string_product.length;
                    *_ppDescriptor = _pchannel->descriptors.string_product.puc_data;
                    break;
                }
                case STRING_iSERIAL:
                {
                    *_pNumBytes = _pchannel->descriptors.string_serial.length;
                    *_ppDescriptor = _pchannel->descriptors.string_serial.puc_data;
                    break;
                }
                default:
                {
                    /*Unknown descriptor request*/
                    DEBUG_MSG_MID(("Error:- Unknown String\r\n"));
                    
                    *_pNumBytes = 0;
                    *_ppDescriptor = NULL;
                    _pchannel->err = USB_ERR_UNKNOWN_REQUEST;

                }
            }
            break;
        }
        default:
        {
            /*Only support 1 language ID*/
            /*Unknown descriptor request*/
            DEBUG_MSG_MID(("Error:- Unknown Language ID\r\n"));

            /*Note1: May get a DEVICE_QUALIFIER request - but OK not to handle
            it as we don't support different configurations for high and full speed*/

            /*Note2: HID uses this standard GET_DESCRIPTOR to get
            class specific descriptors.*/
            *_pNumBytes = 0;
            *_ppDescriptor = NULL;
            _pchannel->err = USB_ERR_UNKNOWN_REQUEST;
        }
    }
    
    return _pchannel->err;
}
/******************************************************************************
End of function process_get_descriptor_string
******************************************************************************/

/******************************************************************************
* Function Name   :   populate_setup_packet
* Description     :   Read the 8 BYTE setup packet into the _pchannel->setup_packet
*                     structure so that it is easier to understand.
* Argument        :   _pSetupPacket - Setup packet in little endian.
* Return value    :   None
******************************************************************************/

static void populate_setup_packet(volatile st_usb_object_t *_pchannel, const uint8_t(*_pSetupPacket)[USB_SETUP_PACKET_SIZE])
{
    _pchannel->setup_packet.bm_request.byte_val = (*_pSetupPacket)[0];
    _pchannel->setup_packet.b_request = (*_pSetupPacket)[1];

    _pchannel->setup_packet.w_value = (uint16_t)(*_pSetupPacket)[2] & 0x00FF;
    _pchannel->setup_packet.w_value = (uint16_t) (_pchannel->setup_packet.w_value | (((uint16_t)(((uint16_t)(*_pSetupPacket)[3]) << 8) & 0xFF00)));

    _pchannel->setup_packet.w_index = (uint16_t)(*_pSetupPacket)[4] & 0x00FF;
    _pchannel->setup_packet.w_index = (uint16_t) (_pchannel->setup_packet.w_index | (((uint16_t)(((uint16_t)(*_pSetupPacket)[5]) << 8) & 0xFF00)));

    _pchannel->setup_packet.w_length = (uint16_t)(*_pSetupPacket)[6] & 0x00FF;
    _pchannel->setup_packet.w_length = (uint16_t) (_pchannel->setup_packet.w_length | (((uint16_t)(((uint16_t)(*_pSetupPacket)[7]) << 8) & 0xFF00)));
}
/******************************************************************************
End of function populate_setup_packet
******************************************************************************/
