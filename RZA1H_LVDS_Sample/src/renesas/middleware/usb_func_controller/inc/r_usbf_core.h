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
* File Name       : r_usbf_core.h
* Version         : 2.00
* Device          : RZA1(H)
* Tool Chain      :
* H/W Platform    : RSK2+RZA!H
* Description     : The USB core sits above the HAL in the USB stack.
*                   Initialises the HAL.
*                   Handles standard setup packets.
* 
* NOTE            : This only supports 1 language ID and that is currently English.
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

#ifndef USB_CORE_H
#define USB_CORE_H

/******************************************************************************
User Includes (Project Level Includes)
******************************************************************************/
/*    Following header file provides structure and prototype definition of USB 
    API's. */
#include "usb.h"

/******************************************************************************
Macros Defines
******************************************************************************/
/* Standard setup Packet Request Codes */
#define    GET_STATUS          (0)
#define    CLEAR_FEATURE       (1)
#define    SET_FEATURE         (3)
#define    SET_ADDRESS         (5)
#define    GET_DESCRIPTOR      (6)
#define    SET_DESCRIPTOR      (7)
#define    GET_CONFIGURATION   (8)
#define    SET_CONFIGURATION   (9)
#define    GET_INTERFACE       (10)
#define    SET_INTERFACE       (11)
#define    SYNCH_FRAME         (12)

/* Descriptor Types */
#define ROOT_DEVICE            (1)
#define CONFIGURATION          (2)
#define STRING                 (3)
#define DEVICE_QUALIFIER       (6)
#define OTHER_SPEED_CONFIG     (7)

/*Setup Packet bmRequest d6..5 values */
#define REQUEST_STANDARD       (0)
#define REQUEST_CLASS          (1)
#define REQUEST_VENDOR         (2)

/*Setup Packet bmRequest d4..0 values */
#define RECIPIENT_DEVICE       (0)
#define RECIPIENT_INTERFACE    (1)
#define RECIPIENT_END_POINT    (2)

/* Set feature options */
#define ENDPOINT_HALT          (0)
#define DEVICE_REMOTE_WAKE_UP  (1)
#define DEVICE_TEST_MODE       (2)

/*String Descriptor Types*/
#ifndef STRING_iMANUFACTURER
#define STRING_iMANUFACTURER   (1)
#endif

#ifndef STRING_iPRODUCT
#define STRING_iPRODUCT        (2)
#endif

#ifndef STRING_iSERIAL
#define STRING_iSERIAL         (3)
#endif

/*index of bConfigurationValue field*/
#define CONFIG_TYPE_INDEX      (5)

/*Size of GET_LINE_CODING response data*/
#define LINE_CODING_DATA_SIZE                (7)

/*Size of SET_CONTROL_LINE_STATE data*/
#define SET_CONTROL_LINE_STATE_DATA_SIZE     (7)

/******************************************************************************
Type Definitions
******************************************************************************/

typedef struct _bit_val
{
    /* Recipient */
    uint8_t d40:5;

    /* Command type "standard" "class" "vendor" */
    uint8_t d65:2;

    /* Data transfer direction */
    uint8_t d7:1;
} bit_val_t;

/*Part of setup packet, the    bmRequestType definition    */
typedef    union 
{    
    uint8_t   byte_val;
    bit_val_t bit_val;
}request_type_t;

/*Setup packet */
typedef struct 
{
    /* Request feature */
    request_type_t    bm_request;

    /* Request number */
    uint8_t b_request;

    /* Field that varies according to request */
    uint16_t w_value;

    /* Field that varies according to request;
     Typically used to pass an index or offset*/
    uint16_t w_index;

    /* Number of bytes to transfer if there is a Data Stage */
    uint16_t w_length;
}setup_packet_t;


/*Call back definition for a received setup packet */
typedef usb_err_t(*CB_SETUP_PACKET)(volatile void *,
                                    setup_packet_t*,
                                    uint16_t*,
                                    uint8_t**);

typedef void(*CB_DONE_OUT)(volatile void *, usb_err_t, uint32_t);

typedef void(*CB_DONE_BULK_IN)(volatile void *, usb_err_t);

typedef void(*CB_DONE_USER_OUT)(usb_err_t, uint32_t);

typedef void(*CB_SETUP)(volatile void *, const uint8_t(*)[USB_SETUP_PACKET_SIZE]);

typedef void(*CB_CABLE)(volatile void *, BOOL);

typedef void(*CB_ERROR)(usb_err_t);

typedef void(*CB_DONE)(usb_err_t);

typedef void(*CB_REPORT_OUT)(uint8_t(*)[]);

typedef enum
{
    USBF_NORMAL = 0,
    USBF_ASYNC
} cdc_rw_mode_t;

typedef struct
{
    cdc_rw_mode_t    mode;
    CB_DONE          pin_done_async;
    CB_DONE_USER_OUT pout_done_async;
}st_usbf_asyn_config_t;

/*Call Backs Registered by USB Core*/
typedef struct
{
    CB_SETUP    p_cb_setup;
    CB_CABLE    p_cb_cable;
    CB_ERROR    p_cb_error;
    CB_DONE_OUT p_cb_cout_mfpdone;
    CB_DONE_OUT p_cb_bout_mfpdone;
    CB_DONE_BULK_IN     p_cb_bin_mfpdone;
    CB_DONE     p_cb_iin_mfpdone;
}st_usbf_p_callbacks_t;


/*State of control pipe*/
typedef enum
{
    STATE_READY,
    STATE_DISCONNECTED,
    STATE_CONTROL_SETUP,
    STATE_CONTROL_IN,
    STATE_CONTROL_OUT
} e_usbf_state_control_t;

/*Data structure used for Control*/
typedef struct
{
    e_usbf_state_control_t device_state;
} st_usbf_control_t;


/*    Descriptor    */
typedef struct
{
    uint16_t length;
    uint8_t* puc_data;
} descriptor_t;

/*Structure that holds Descriptors*/
typedef struct
{
    descriptor_t device;
    descriptor_t config;
    descriptor_t dev_qualifier;
    descriptor_t other_speed_config;

    descriptor_t string_manufacturer;
    descriptor_t string_product;
    descriptor_t string_serial;

    descriptor_t string_language_ids;


    descriptor_t hid_report_in;
    descriptor_t report_in;
    descriptor_t report_out;

}st_usbf_descriptors_t;

typedef struct
{
    CB_REPORT_OUT *pcbout_report;

    BOOL          hi_speed_enable;

    st_usbf_descriptors_t descriptors;

} st_usbf_user_configuration_t;


/******************************************************************************
Local Types
******************************************************************************/
/*Data structure for SET_CONTROL_LINE_STATE*/
typedef struct
{
    uint32_t dw_dte_rate;
    uint8_t b_char_format;
    uint8_t b_parity_type;
    uint8_t b_data_bits;
}SET_CONTROL_LINE_STATE_DATA;

/*Structure for BULK OUT*/
typedef struct
{
    /*Busy Flag*/
    volatile BOOL m_busy;

    /*Number of bytes read*/
    uint32_t m_bytes_read;

    /*Error Status*/
    usb_err_t m_err;

    /*Callback done*/
    CB_DONE_OUT m_cb_done;
}BULK_OUT;

/*Structure for BULK IN*/
typedef struct
{
    /*Busy Flag*/
    volatile BOOL m_busy;

    /*Error Status*/
    usb_err_t m_err;

    /*Callback done*/
    CB_DONE m_cb_done;
}BULK_IN;

typedef struct
{
    /* Current USB Configuration */
    uint8_t* pcurr_usb_config;
    st_usb_hal_config_t config;

    /* USB Status */
    uint16_t status;
    uint16_t usb_status;
    uint8_t  device_state;

    /* Endpoint status */
    uint16_t endpoint_status;

    /* configuration value */
    uint8_t  config_value;

    /*Setup Packet */
    setup_packet_t setup_packet;

    /*Registered CallBacks*/
    CB_SETUP_PACKET       fp_cbc_setup_packet;
    CB_DONE_OUT           fp_cb_control_out;
    st_usbf_p_callbacks_t callbacks;
    st_usbf_descriptors_t descriptors;
    st_usbf_asyn_config_t rw_config;

    uint16_t              descriptors_initialised;

    /* HAL Layer Data */
    /* PIPEn Buffer counter */
    uint32_t  dtcnt[MAX_PIPE_NO + 1];

    /* PIPEn receive data counter */
    uint32_t  rdcnt[MAX_PIPE_NO + 1];

    /* PIPEn Buffer pointer(8bit) */
    uint8_t  *p_dtptr[MAX_PIPE_NO + 1];

    /* Ignore count */
    uint16_t  pipe_ignore[MAX_PIPE_NO + 1];

    /* C/D FIFO | DIR | EPnum */
    uint16_t  pipe_tbl[MAX_PIPE_NO + 1];

    /* data flag */
    uint16_t  pipe_flag[MAX_PIPE_NO + 1];

    /* data size */
    uint32_t  pipe_data_size[MAX_PIPE_NO + 1];

    /* Index of Endpoint Information table */
    uint16_t  ep_table_index[MAX_EP_NO + 1];

    /* Endpoints */
    volatile uint16_t   *pend_pnt[6];  /* MAX_EP_DESCRIPTORS */

    /* Connection Status */
    BOOL connected;

    /* Internal connection status */
    volatile uint8_t usb_connected;

    /* Read/Write Mode */
    cdc_rw_mode_t   rw_mode;

    /* Hi_Speed Mode Enable/Disable */
    BOOL hi_speed_enable;

    /* Error State */
    usb_err_t err;

    /* Function local variables   */
    /* Peripheral  */
    volatile struct st_usb20 *phwdevice;


    uint16_t    buf;
    uint16_t    endflag_k;

    /* Temp Data */
    uint16_t    generic_temp;
    uint16_t    generic_data;
    uint16_t    generic_size;
    uint16_t    generic_even;
    uint16_t    generic_counter;
    uint16_t    generic_buffer;
    uint16_t    generic_buffer2;
    uint16_t    generic_buffer3;
    uint16_t    generic_buffer4;
    uint16_t    generic_bitcheck;

    uint8_t     pgeneric_buffer8;
    uint8_t     setup_cmd_buffer[USB_SETUP_PACKET_SIZE];

    uint16_t    cb_setup_num_bytes;
    uint8_t*    pcdsetup_buffer;

    uint8_t     line_coding[LINE_CODING_DATA_SIZE];

    /*Data structure for SET_CONTROL_LINE_STATE*/
    SET_CONTROL_LINE_STATE_DATA set_control_line_data;

    /*Data Buffer for SET_CONTROL_LINE_STATE data*/
    uint8_t control_line_state_pbuffer[SET_CONTROL_LINE_STATE_DATA_SIZE];

    /*Bulk Out specific*/
    BULK_OUT    bulk_out;

    /*Bulk In specific*/
    BULK_IN     bulk_in;
} st_usb_object_t;

/******************************************************************************
Function Prototypes
******************************************************************************/
usb_err_t R_USBF_CoreInit( volatile st_usb_object_t *_pchannel,
                      uint8_t* _Manufacturer, uint16_t _ManufacturerSize,
                     uint8_t* _Product, uint16_t _ProductSize,
                     uint8_t* _Serial, uint16_t _SerialSize,
                     uint8_t* _DeviceDescriptor, uint16_t _DeviceDescriptorSize,
                     uint8_t* _DeviceQDescriptor, uint16_t _DeviceQDescriptorSize,
                     uint8_t* _ConfigDescriptor, uint16_t _ConfigDescriptorSize,
                     uint8_t* _OtherConfigDescriptor, uint16_t _OtherConfigDescriptorSize,
                     CB_SETUP_PACKET _fpCBSetupPacket,
                     CB_DONE_OUT _fpCBControlOut,
                     CB_CABLE _fpCBCable,
                     CB_ERROR _fpCBError);

#endif /*USB_CORE_H*/
