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
 * Copyright (C) 2016 Renesas Electronics Corporation. All rights reserved.    */
/******************************************************************************
 * File Name    : r_usbh_drv_comms_class.c
 * Version      : 1.10
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : Device driver for CDC ACM Class USB
 *
 * Notes:   This driver has been tested with Microchip MCP2200 device.
 *          Please note there are various revisions of this device and data loss
 *          has been seen with revision A1 under heavy load. It is
 *          recommended to use the latest revision (A2 at time of writing).
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 29.01.2011 1.00 First Release
 *              : 27.01.2016 1.10 Updated for RZA1 and GSCE Standards
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "iodefine_cfg.h"

#include "compiler_settings.h"
#include "control.h"
#include "ddusbh.h"
#include "usbhDeviceApi.h"
#include "r_event.h"
#include "r_cbuffer.h"
#include "r_sci_drv_api.h"
#include "r_usbh_drv_comms_class.h"
#include "endian.h"
#include "r_task_priority.h"
#include "trace.h"
#include "usbhEnum.h"

/******************************************************************************
 Macro Definitions
 ******************************************************************************/

#define TASK_CDC_RX_PRI             (8)
#define TASK_CDC_INTER_PRI          (5)

/* Set this to suit the data size in your
 application. */
#define SCI_RX_BUFFER_SIZE          (4096UL)
#define SCI_DEFAULT_TIME_OUT        (100UL)
#define SCI_DEFAULT_BAUD            (57600UL)
#define SCI_RX_PACKET_SIZE          (64UL)
#define SCI_BREAK_DURATION          (0xFFFF)    /* Duration for break signal in ms.
                                                   0xFFFF - break asserted until deasserted */
#define SCI_BREAK_DEASSERT          (0x0000)    /* Deassert any break in progress */

#define CDC_DATA_INTERFACE_CLASS    (0x0A)

/* CDC Class Requests IDs from Table 46: Class-Specific Request Codes */
#define SCI_SEND_ENCAP_COMMAND      (0x00)
#define SCI_GET_ENCAP_RESPONSE      (0x01)
#define SCI_SET_COMM_FEATURE        (0x02)
#define SCI_GET_COMM_FEATURE        (0x03)
#define SCI_CLEAR_COMM_FEATURE      (0x04)

#define SCI_SET_LINE_CODING         (0x20)
#define SCI_GET_LINE_CODING         (0x21)
#define SCI_SET_CONTROL_LINE_STATE  (0x22)

#define SCI_SEND_BREAK              (0x23)

/* Defines for Serial State Packet */
#define SCI_STATE_SIZE              (10)  /* Size of a serial state interrupt packet */
#define SCI_STATE_REQ_TYPE          (0xA1)
#define SCI_SERIAL_STATE            (0x20)     /* PSTN Subclass Notification Code */

/* From Table 51: Control Signal Bitmap Values for SetControlLineState */
#define SCI_CDC_RTS                 (BIT_1)
#define SCI_CDC_DTR                 (BIT_0)

/* Comment this line out to turn ON module trace in this file */

#undef _TRACE_ON_
#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Enumerated Types
 ******************************************************************************/

/******************************************************************************
 Typedef definitions
 ******************************************************************************/

typedef enum
{
    SCI_IDLE = 0, SCI_WAIT_RX_DATA
} scist_t;

/* Structure for UART State Bitmap */
/* Reference  USB Spec. CDC PSTN Subclass R1.2 Table 31 */
#pragma pack (1)
typedef struct
{
    uint8_t over_run :1;
    uint8_t parity :1;
    uint8_t framing :1;
    uint8_t ring_signal :1;
    uint8_t break_b :1;
    uint8_t tx_carrier :1;
    uint8_t rx_carrier :1;
    uint16_t :9;
} uart_state_t;
#pragma pack()
typedef uart_state_t* puart_state_t;

/* The class control structure for SET_CONTROL_LINE_STATE
 from Table 50: Line Coding Structure */
#pragma pack (1)
typedef struct
{
    uint32_t dw_dte_rate;
    uint8_t bchar_format;
    uint8_t bparity_type;
    uint8_t bdata_bits;
} mctl_t;
typedef mctl_t* pmctl_t;
#pragma pack()

typedef struct
{
    /* The transfer request structures */
    USBTR write_request;
    USBTR read_request;
    USBTR line_request;
    USBTR device_request;
    FTDERR last_error; /* The last error code */
    PUSBDI pdevice; /* Pointer to the device and endpoints */
    PUSBEI pout_endpoint;
    PUSBEI pin_endpoint;
    PUSBEI pint_endpoint;
    uint32_t dwtimeout_ms; /* The current time-out */
    int8_t pszstream_name[DEVICE_MAX_STRING_SIZE]; /* The stream name of the device */
    SCICFG sci_config; /* The current configuration */
    uart_state_t uart_state;
    uint8_t break_in_progress; /* Current state of a break request */

    /* A timer is used to create a lightweight thread which polls the RX
     endpoint. The data that is received is put into a circular buffer */
    PCBUFF pcbuffer;

    os_task_t * ui_rx_task_id;   /* The task polling the RX endpoint */
    os_task_t * ui_int_task_id;  /* The task polling the interrupt IN endpoint */
    scist_t sci_state; /* The state of the RX polling function */
    USBEC rx_error_code;
    scist_t int_state; /* The state of the interrupt in polling function */

    /* Not used, here to ensure the following buffer is aligned to a 32 bit
     boundary - USB peripheral on the RX has 16 bit FIFOs but this code
     may be ported to a device with a 32 bit FIFOs. */
    uint32_t dwalign;
    uint8_t pby_rx_buffer[SCI_RX_PACKET_SIZE]; /* The place where the data is delivered */
    uint8_t pby_int_buffer[SCI_RX_PACKET_SIZE]; /* Buffer for the interrupt IN line status data */
} cdc_t;
typedef cdc_t* pcdc_t;

typedef struct
{
    uint32_t dw_baud;
} baudrates_t;

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Function Prototypes
 ******************************************************************************/
static int_t drv_open(st_stream_ptr_t p_stream);
static void drv_close(st_stream_ptr_t p_stream);
static int_t drv_read(st_stream_ptr_t p_stream, uint8_t *p_bybuffer, uint32_t ui_count);
static int_t drv_write(st_stream_ptr_t p_stream, uint8_t *p_bybuffer, uint32_t ui_count);
static int_t drv_control(st_stream_ptr_t p_stream, uint32_t ctl_code, void *p_ctl_struct);
static int32_t scidevice_request(pcdc_t p_scidrv, uint8_t bm_request_type, uint8_t b_request, uint16_t w_value,
        uint16_t w_index, uint16_t w_length, uint8_t *pby_data);
static FTDERR sci_configure(pcdc_t p_scidrv, PSCICFG p_config);
static FTDERR sci_sendbreak(pcdc_t p_scidrv, CTLCODE ctl_code);
static PUSBEI drv_get_endpoint(PUSBDI pdevice, USBDIR transfer_direction, USBTT transfer_type);
static void sci_rx_poll_task(pcdc_t p_scidrv);
static void sci_int_poll_task(pcdc_t p_scidrv);

/******************************************************************************
 Constant Data
 ******************************************************************************/

/* The error string table to match the error codes for the driver */
static const char * const psz_error_string[] =
{ "SCI_NO_ERROR", "SCI_RX_ERROR", "SCI_REQUEST_ERROR", "SCI_NOT_CLEAR_TO_SEND", "SCI_OVERRUN_ERROR",
  "SCI_CONFIGURATION_ERROR", "SCI_INVALID_CONFIGURATION", "SCI_INVALID_CONTROL_CODE", "SCI_INVALID_PARAMETER",
  "SCI_TX_TIME_OUT" };

/* The table of supported baud rates */
static const baudrates_t baud_rates[] =
{
     /* Other supported baud rates can be added here, if required */
     {
      300UL,
     },
     {
         600UL,
     },
     {
         1200UL,
     },
     {
         2400UL,
     },
     {
         4800UL,
     },
     {
         9600UL,
     },
     {
         19200UL,
     },
     {
         38400UL,
     },
     {
         57600UL,
     },
     {
         115200UL,
     }
};

/* Define the default configuration of this device */
static const SCICFG gsci_default_config =
{
 /* Baud rate */
 SCI_DEFAULT_BAUD,

 /* Line Coding - asynchronous with (N,8,1) & RTS/DTR asserted */
 (SCI_PARITY_NONE | SCI_DATA_BITS_EIGHT | SCI_ONE_STOP_BIT | SCI_DTR_ASSERT | SCI_RTS_ASSERT)
};

/* Define the driver function table for this */
const st_r_driver_t g_cdc_driver =
{ "CDC Device Driver", drv_open, drv_close, drv_read, drv_write, drv_control, no_dev_get_version };

/******************************************************************************
 Public Functions
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_LoadCommsDeviceClass
 Description:   Function to check the config descriptor and get hold of the
 data class interface descriptor
 Parameters:    IN pdevice - Pointer to the device information
 IN pbyConfig - Pointer to the configuration descriptor
 Return value:  True if the data class interface was found and the
 data endpoints found
 ******************************************************************************/
_Bool R_USBH_LoadCommsDeviceClass(PUSBDI pdevice, uint8_t *pby_config)
{
    /* Format the provided pointer as a USB Descriptor */
    PUSBDESC pdesc = (PUSBDESC) pby_config;

    /* Sanity check - make sure that this is a config descriptor */
    if (USB_CONFIGURATION_DESCRIPTOR_TYPE == pdesc->Common.bDescriptorType)
    {
        uint8_t * pby_end;
        uint16_t w_length;

        /* Get the total length of the descriptor */
#ifdef _BIG
        swapIndirectShort(&w_length, &pdesc->Configuration.wTotalLength);
#else
        copyIndirectShort (&w_length, &pdesc->Configuration.wTotalLength);
#endif

        /* Set a pointer to the end of the descriptor */
        pby_end = pby_config + w_length;

        /* Search the entire configuration descriptor */
        while (pby_config < pby_end)
        {
            pdesc = (PUSBDESC) pby_config;
            if (CDC_DATA_INTERFACE_CLASS == pdesc->Interface.bInterfaceClass)
            {
                /* Create the information for this interface */
                if (usbhCreateInterfaceInformation (pdevice, (PUSBIF) pdesc))
                {
                    return true;
                }
            }
            else if (pdesc->Common.bLength)
            {
                /* Skip over any unknown or unused descriptors */
                pby_config += pdesc->Common.bLength;
            }
            else
            {
                /* No more descriptors to process and CDC data Interface Class not found*/
                return false;
            }
        }
    }
    return false;
}
/******************************************************************************
 End of function  R_USBH_LoadCommsDeviceClass
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_CDCLoadDriver
 Description:   Function to select the desired interface and start the
 Arguments:     IN  p_cdc_info - Pointer to the interface information
 OUT ppdeviceDriver - Pointer to the destination driver
 Return value:  true if the driver will work with the device
 ******************************************************************************/
_Bool R_USBH_CDCLoadDriver(pcdc_connection_t p_cdc_info, st_r_driver_t** ppdeviceDriver)
{
    /* Standard API - avoid unused warning */
    (void) p_cdc_info;

    *ppdeviceDriver = (st_r_driver_t *) &g_cdc_driver;
    return true;
}

/******************************************************************************
 End of function  R_USBH_CDCLoadDriver
 ******************************************************************************/

/******************************************************************************
 Private Functions
 ******************************************************************************/

/******************************************************************************
 Function Name: drv_open
 Description:   Function to open the driver
 Arguments:     IN  p_stream - Pointer to the file stream
 Return value:  0 for success otherwise -1
 ******************************************************************************/
static int_t drv_open(st_stream_ptr_t p_stream)
{
    /* Get a pointer to the device information from the host driver for this
     device */
    PUSBDI pdevice = usbhGetDevice((int8_t *) p_stream->p_stream_name);
    if (pdevice)
    {
        /* Get the interrupt endpoint for status notification changes */
        PUSBEI p_int_in = drv_get_endpoint (pdevice, USBH_IN, USBH_INTERRUPT);

        /* Get the bulk endpoints for the data comms */
        PUSBEI p_in = drv_get_endpoint (pdevice, USBH_IN, USBH_BULK);
        PUSBEI p_out = drv_get_endpoint (pdevice, USBH_OUT, USBH_BULK);

        /* If the device has the expected endpoints */
        if (((p_out) && (p_in)) && (p_int_in))
        {
            /* Allocate the memory for the driver */
            p_stream->p_extension = R_OS_AllocMem (sizeof(cdc_t), R_REGION_LARGE_CAPACITY_RAM);
            if (p_stream->p_extension)
            {
                pcdc_t p_sci_drv = p_stream->p_extension;
                memset (p_sci_drv, 0, sizeof(cdc_t)); /* Initialise the driver data */

                strncpy ((char*) p_sci_drv->pszstream_name, (char*) p_stream->p_stream_name,
                DEVICE_MAX_STRING_SIZE); /* Copy in the link name */

                /* Set the device and endpoint information found */
                p_sci_drv->pdevice = pdevice;
                p_sci_drv->pout_endpoint = p_out;
                p_sci_drv->pin_endpoint = p_in;
                p_sci_drv->pint_endpoint = p_int_in;
                p_sci_drv->dwtimeout_ms = SCI_DEFAULT_TIME_OUT;
                p_sci_drv->int_state = SCI_IDLE;
                p_sci_drv->sci_state = SCI_IDLE;

                /* Add the events to the transfer requests */
                R_OS_CreateEvent (&p_sci_drv->read_request.ioSignal);
                R_OS_CreateEvent (&p_sci_drv->line_request.ioSignal);
                R_OS_CreateEvent (&p_sci_drv->write_request.ioSignal);
                R_OS_CreateEvent (&p_sci_drv->device_request.ioSignal);

                p_sci_drv->sci_config = gsci_default_config; /* Set the default configuration */

                sci_configure (p_sci_drv, &p_sci_drv->sci_config); /* Configure the device with default configuration */

                /* Make the circular buffer */
                p_sci_drv->pcbuffer = cbCreate (SCI_RX_BUFFER_SIZE);
                if (p_sci_drv->pcbuffer)
                {

                    /* Start a task to poll RX endpoint */
                    p_sci_drv->ui_rx_task_id = R_OS_CreateTask("CDC Rx", (os_task_code_t) sci_rx_poll_task, p_sci_drv,
                            R_OS_ABSTRACTION_PRV_SMALL_STACK_SIZE, TASK_CDC_RX_PRI);

                    /* Start a task to poll IN endpoint */
                    p_sci_drv->ui_int_task_id = R_OS_CreateTask("CDC In", (os_task_code_t) sci_int_poll_task, p_sci_drv,
                            R_OS_ABSTRACTION_PRV_SMALL_STACK_SIZE, TASK_CDC_INTER_PRI);

                    TRACE(("sciOpen: Opened device %s\r\n", p_stream->p_stream_name));
                    return SCI_NO_ERROR;
                }
                else
                {
                    TRACE(("sciOpen: Failed to allocate buffer\r\n"));
                    R_OS_FreeMem(p_stream->p_extension);
                }
            }
            else
            {
                TRACE(("sciOpen: Failed to allocate memory\r\n"));
            }
        }
        else
        {
            TRACE(("sciOpen: Failed to find bulk endpoints\r\n"));
        }
    }
    else
    {
        TRACE(("sciOpen: Failed to find device %s\r\n", p_stream->p_stream_name));
    }

    return -1;
}
/******************************************************************************
 End of function  drv_open
 ******************************************************************************/

/******************************************************************************
 Function Name: drv_close
 Description:   Function to close the driver
 Arguments:     IN  p_stream - Pointer to the file stream
 Return value:  none
 ******************************************************************************/
static void drv_close(st_stream_ptr_t p_stream)
{
    /*All the resources created during the Driver open are released */
    pcdc_t p_sci_drv;

    p_sci_drv = p_stream->p_extension;

    TRACE(("sciClose:\r\n"));

    if (NULL != p_stream->p_extension)
    {
        /* Null the pointer (this is needed is there is a subsequent close call */
        p_stream->p_extension = NULL;

        /* Stop the receive task */
        R_OS_DeleteTask (p_sci_drv->ui_rx_task_id);
        R_OS_DeleteTask (p_sci_drv->ui_int_task_id);

        /* Stop any pending transfer requests */
        usbhCancelTransfer (&p_sci_drv->read_request);
        usbhCancelTransfer (&p_sci_drv->line_request);
        usbhCancelTransfer (&p_sci_drv->write_request);

        /* Remove the event handlers */
        R_OS_DeleteEvent (&p_sci_drv->read_request.ioSignal);
        R_OS_DeleteEvent (&p_sci_drv->line_request.ioSignal);
        R_OS_DeleteEvent (&p_sci_drv->write_request.ioSignal);
        R_OS_DeleteEvent (&p_sci_drv->device_request.ioSignal);

        /* Remove the Circular Buffer */
        cbDestroy (p_sci_drv->pcbuffer);

        /* Release the allocated driver memory */
        R_OS_FreeMem(p_stream->p_extension);
    }
}
/******************************************************************************
 End of function  drv_close
 ******************************************************************************/

/*****************************************************************************
 Function Name: drv_read
 Description:   Function to read button presses
 Arguments:     IN  p_stream - Pointer to the file stream
 IN  p_bybuffer - Pointer to the source memory
 IN  ui_count - The number of bytes to write
 Return value:  The length of data written -1 on error
 ******************************************************************************/
static int_t drv_read(st_stream_ptr_t p_stream, uint8_t *p_bybuffer, uint32_t ui_count)
{
    pcdc_t p_sci_drv = p_stream->p_extension;
    if (p_sci_drv)
    {
        int32_t i_read = 0;

        /* Read the requested amount of data from the Circular Buffer */
        while ((ui_count--) && (cbGet (p_sci_drv->pcbuffer, p_bybuffer++)))
        {
            i_read++;
        }

        /* Report the amount of data read, which is not necessarily the amount
         requested */
        return (i_read);
    }
    return -1;
}
/*****************************************************************************
 End of function  drv_read
 ******************************************************************************/

/******************************************************************************
 Function Name: drv_write
 Description:   Function to write to the device
 Arguments:     IN  p_stream - Pointer to the file stream
 IN  p_bybuffer - Pointer to the source memory
 IN  ui_count - The number of bytes to write
 Return value:  The length of data written -1 on error
 ******************************************************************************/
static int_t drv_write(st_stream_ptr_t p_stream, uint8_t *p_bybuffer, uint32_t ui_count)
{
    pcdc_t p_sci_drv = p_stream->p_extension;
    if (p_sci_drv)
    {
        _Bool bfresult = true;

        /* Check that the CTS signal is asserted if configured. */
#if HARDWARE_FLOW_CONTROL_IMPLIMENTED
        if ((p_sci_drv->sci_config.dwConfig & SCI_RTS_CTS_ENABLE)
                && (/* Future Extension: check CTS line status here & define HARDWARE_FLOW_CONTROL_IMPLIMENTED */))
        {
            p_sci_drv->last_error = SCI_NOT_CLEAR_TO_SEND;
            return 0;
        }
#endif

        /* Start the transfer */
        bfresult = usbhStartTransfer (p_sci_drv->pdevice, &p_sci_drv->write_request, p_sci_drv->pout_endpoint,
                                      p_bybuffer, (size_t) ui_count, p_sci_drv->dwtimeout_ms);

        /* If the transfer was started */
        if (bfresult)
        {
            /* Wait for the transfer to complete or time-out.
             This function can block until the write completes. */
            R_OS_WaitForEvent(&p_sci_drv->write_request.ioSignal, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);

            /* Check the result of the transfer */
            if (p_sci_drv->write_request.errorCode)
            {
                TRACE(("sciWrite: %d\r\n", p_sci_drv->write_request.errorCode));

                /* Set the error code */
                if (REQ_IDLE_TIME_OUT == p_sci_drv->write_request.errorCode)    /* SSGRZAISRC-76 */
                {
                    p_sci_drv->last_error = SCI_TX_TIME_OUT;
                }
                else
                {
                    p_sci_drv->last_error = SCI_REQUEST_ERROR;
                    return -1;
                }
            }
            return (int) p_sci_drv->write_request.uiTransferLength; /* return the length transferred */
        }
        else
        {
            TRACE(("sciWrite: Failed to start transfer\r\n"));
            p_sci_drv->last_error = SCI_REQUEST_ERROR;
        }
    }
    return -1;
}
/******************************************************************************
 End of function  drv_write
 ******************************************************************************/

/******************************************************************************
 Function Name: scidevice_request
 Description:   Function to send a device request
 Arguments:     IN  p_scidrv - Pointer to the driver extension
 IN  bRequest - The request
 IN  bm_request_type - The request type
 IN  w_value - The Value
 IN  w_index - The Index
 IN  pby_data - The length of the data
 IN/OUT pby_data - Pointer to the data
 Return value:  SCI_NO_ERROR for success or SCI_REQUEST_ERROR if in error
 ******************************************************************************/
static int32_t scidevice_request(pcdc_t p_scidrv, uint8_t bm_request_type, uint8_t b_request, uint16_t w_value,
        uint16_t w_index, uint16_t w_length, uint8_t *pby_data)
{
    /* Use the usbhDeviceAPI to access the device */
    if (usbhDeviceRequest (&p_scidrv->device_request, p_scidrv->pdevice, bm_request_type, b_request, w_value, w_index,
                           w_length, pby_data))
    {
        /* The request failed */
        p_scidrv->last_error = SCI_REQUEST_ERROR;
        return SCI_REQUEST_ERROR;
    }
    return SCI_NO_ERROR;
}
/******************************************************************************
 End of function  scidevice_request
 ******************************************************************************/

/*****************************************************************************
 Function Name: sci_baud_is_supported
 Description:   Function to check that the baud rate is valid
 Parameters:    IN  dwBaud - The desired baud rate
 Return value:  true if the baud is valid
 *****************************************************************************/
static _Bool sci_baud_is_supported(uint32_t dwBaud)
{
    size_t st_num_baud_rates = ((sizeof(baud_rates)) / (sizeof(baudrates_t)));
    while (st_num_baud_rates--)
    {
        if (baud_rates[st_num_baud_rates].dw_baud == dwBaud)
        {
            return true;
        }
    }
    return false;
}
/*****************************************************************************
 End of function  sci_baud_is_supported
 ******************************************************************************/

/*****************************************************************************
 Function Name: sci_sub_config_linecodeing
 Description:   Function to configure line coding element based on passed
 dwConfig bit loaded field.
 Parameters:    IN  dwconfig - Bit loaded requested line configuration
 OUT p_line_coding - pointer to line coding element to be set
 Return value:  none
 *****************************************************************************/
static void sci_sub_config_linecodeing(mctl_t* p_line_coding, uint32_t dwconfig)
{
    if (dwconfig & SCI_TWO_STOP_BIT)
    {
        p_line_coding->bchar_format = 2;
    }
    else if (dwconfig & SCI_ONE_HALF_STOP_BIT)
    {
        p_line_coding->bchar_format = 1;
    }
    else
    {
        /* SCI_ONE_STOP_BIT */
        p_line_coding->bchar_format = 0;
    }
    if (dwconfig & SCI_PARITY_ODD)
    {
        p_line_coding->bparity_type = 1;
    }
    else if (dwconfig & SCI_PARITY_EVEN)
    {
        p_line_coding->bparity_type = 2;
    }
    else
    {
        /* SCI_PARITY_NONE */
        p_line_coding->bparity_type = 0;
    }
    if (dwconfig & SCI_DATA_BITS_SEVEN)
    {
        p_line_coding->bdata_bits = 7;
    }
    else
    {
        /* SCI_DATA_BITS_EIGHT */
        p_line_coding->bdata_bits = 8;
    }
}
/*****************************************************************************
 End of function  sci_sub_config_linecodeing
 ******************************************************************************/

/*****************************************************************************
 Function Name: sci_configure
 Description:   Function to configure the serial port
 Parameters:    IN  p_scidrv - Pointer to the driver extension
 IN  p_config - Pointer to the configuration
 Return value:  0 for success or error code
 *****************************************************************************/
static FTDERR sci_configure(pcdc_t p_scidrv, PSCICFG p_config)
{
    int32_t option_count = 0;
    uint16_t ctrl_line_val = 0;
    USBRQ usb_request;
    mctl_t line_coding;

    /* Check to make sure that the configuration is valid for this device
     by checking the unsupported line coding first*/
    if (((((((p_config->dwConfig & SCI_SYNCHRONOUS ) || (p_config->dwConfig & SCI_MULTI_PROCESSOR ))
            || (p_config->dwConfig & SCI_SYNCCLOCK_INPUT )) || (p_config->dwConfig & SCI_X16CLOCK_INPUT ))
            || (p_config->dwConfig & SCI_ASYNCLOCK_OUTPUT )) || (p_config->dwConfig & SCI_TXD_NRTS_ENABLE ))
            || (p_config->dwConfig & SCI_RXD_NCTS_ENABLE ))
    {
        return SCI_INVALID_CONFIGURATION;
    }

    /* Check that only one parity option has been set */
    if (p_config->dwConfig & SCI_PARITY_ODD)
    {
        option_count++;
    }
    if (p_config->dwConfig & SCI_PARITY_EVEN)
    {
        option_count++;
    }
    if (p_config->dwConfig & SCI_PARITY_NONE)
    {
        option_count++;
    }
    if (option_count > 1)
    {
        return SCI_INVALID_CONFIGURATION;
    }

    /* Check that the baud rate is OK */
    if (!sci_baud_is_supported (p_config->dwBaud))
    {
        return SCI_INVALID_CONFIGURATION;
    }

    /* Take a copy of the configuration */
    p_scidrv->sci_config = *p_config;

    /* Configure the device */
#ifdef _BIG
    swapIndirectLong(&line_coding.dw_dte_rate, &p_config->dwBaud);
#else
    copyIndirectLong (&line_coding.dw_dte_rate, &p_config->dwBaud);
#endif

    /* Configure line coding parameter */
    sci_sub_config_linecodeing (&line_coding, p_config->dwConfig);

    /* Control Line - DTR */
    if (p_config->dwConfig & SCI_DTR_ASSERT)
    {
        ctrl_line_val |= SCI_CDC_DTR;
    }

    /* Control Line - RTS */
    if (p_config->dwConfig & SCI_RTS_ASSERT)
    {
        ctrl_line_val |= SCI_CDC_RTS;
    }

    /* Format the class request */
    usb_request.Field.Direction = USB_HOST_TO_DEVICE;
    usb_request.Field.Type = USB_CLASS_REQUEST;
    usb_request.Field.Recipient = USB_RECIPIENT_INTERFACE;
    if (!scidevice_request (p_scidrv, usb_request.bmRequestType,
    SCI_SET_LINE_CODING,
                            0, 0, sizeof(mctl_t), (uint8_t*) &line_coding))
    {
        /* Set the control lines */
        if (!scidevice_request (p_scidrv, usb_request.bmRequestType,
        SCI_SET_CONTROL_LINE_STATE,
                                ctrl_line_val, 0, 0,
                                NULL))
        {
            return SCI_NO_ERROR;
        }
    }
    return SCI_CONFIGURATION_ERROR;
}
/*****************************************************************************
 End of function  sci_configure
 ******************************************************************************/

/*****************************************************************************
 Function Name: sci_sendbreak
 Description:   Function to send a serial break
 Parameters:    IN  p_scidrv - Pointer to the driver extension
 IN ctl_code - set/clear control code
 Return value:  0 for success or error code
 *****************************************************************************/
static FTDERR sci_sendbreak(pcdc_t p_scidrv, CTLCODE ctl_code)
{
    USBRQ usb_request;
    FTDERR ret_val = SCI_CONFIGURATION_ERROR;

    /* Format the class request */
    usb_request.Field.Direction = USB_HOST_TO_DEVICE;
    usb_request.Field.Type = USB_CLASS_REQUEST;
    usb_request.Field.Recipient = USB_RECIPIENT_INTERFACE;

    if (CTL_SCI_SET_BREAK == ctl_code)
    {
        /* Send Break Request */
        if (!scidevice_request (p_scidrv, usb_request.bmRequestType,
        SCI_SEND_BREAK,
                                SCI_BREAK_DURATION, 0, 0,
                                NULL))
        {
            /* Note there is no way to detect a timed break in
             * progress from the UART. break_in_progress is used
             * as a toggle flag, toggled on break assert/deassert
             * - therefore only valid for infinite break durations.
             */
            p_scidrv->break_in_progress = 1;
            ret_val = SCI_NO_ERROR;
        }
    }
    else if (CTL_SCI_CLEAR_BREAK == ctl_code)
    {
        /* Send Break Request */
        if (!scidevice_request (p_scidrv, usb_request.bmRequestType,
        SCI_SEND_BREAK,
                                SCI_BREAK_DEASSERT, 0, 0,
                                NULL))
        {
            p_scidrv->break_in_progress = 0;
            ret_val = SCI_NO_ERROR;
        }
    }
    else
    {
        /* Not a valid request */
        ret_val = SCI_INVALID_PARAMETER;
    }

    return ret_val;
}
/*****************************************************************************
 End of function  sci_sendbreak
 ******************************************************************************/

/******************************************************************************
 Function Name: drv_control_sub1
 Description:   Subordinate function to drv_control to reduce function length
 Function to handle custom controls for the device.
 Arguments:     IN  p_stream - Pointer to the file stream
 IN  ctl_code - The custom control code
 IN  p_ctl_struct - Pointer to the custom control structure
 Return value:  0 for success -1 on error
 ******************************************************************************/
static int_t drv_control_sub1(st_stream_ptr_t p_stream, CTLCODE ctl_code, void *p_ctl_struct)
{
    int_t ret_val = 0; // Assume Success
    pcdc_t p_sci_drv = p_stream->p_extension;
    switch (ctl_code)
    {
        /* All device class codes */
        case CTL_GET_LAST_ERROR:
        {
            /* Make sure a valid pointer was specified */
            if (p_ctl_struct)
            {
                *((PFTDERR) p_ctl_struct) = p_sci_drv->last_error;
                p_sci_drv->last_error = SCI_NO_ERROR;
            }
            else
            {
                p_sci_drv->last_error = SCI_INVALID_PARAMETER;
                ret_val = -1;
            }
            break;
        }
        case CTL_GET_ERROR_STRING:
        {
            /* Make sure a valid pointer was specified */
            if (p_ctl_struct)
            {
                PERRSTR p_errstr = (PERRSTR) p_ctl_struct;
                if (p_errstr->iErrorCode < SCI_NUM_ERRORS)
                {
                    p_errstr->pszErrorString = (int8_t*) psz_error_string[p_errstr->iErrorCode];
                }
                else
                {
                    ret_val = -1;
                    p_sci_drv->last_error = SCI_INVALID_PARAMETER;
                }
            }
            else
            {
                p_sci_drv->last_error = SCI_INVALID_PARAMETER;
                ret_val = -1;
            }
            break;
        }
        case CTL_USB_ATTACHED:
        {
            if (p_ctl_struct) // Make sure a valid pointer was specified
            {
                if ((strcmp ((char*) p_sci_drv->pszstream_name, (char*) p_stream->p_stream_name) == 0)
                        && (usbhGetDevice ((int8_t *) p_stream->p_stream_name)))
                {
                    *((_Bool *) p_ctl_struct) = true;
                }
                else
                {
                    *((_Bool *) p_ctl_struct) = false;
                }
            }
            else
            {
                p_sci_drv->last_error = SCI_INVALID_PARAMETER;
                ret_val = -1;
            }
            break;
        }
        case CTL_SCI_SET_CONFIGURATION:
        {
            /* Make sure a valid pointer was specified */
            if (p_ctl_struct)
            {
                /* Attempt to re-configure the device */
                p_sci_drv->last_error = sci_configure (p_sci_drv, (PSCICFG) p_ctl_struct);

                /* If it fails */
                if (p_sci_drv->last_error)
                {
                    /* Revert to the previous configuration */
                    sci_configure (p_sci_drv, &p_sci_drv->sci_config);
                    ret_val = -1;
                }
                else
                {  /* Take a copy of the configuration */
                    p_sci_drv->sci_config = *((PSCICFG) p_ctl_struct);
                }
            }
            else
            {
                p_sci_drv->last_error = SCI_INVALID_PARAMETER;
                ret_val = -1;
            }
            break;
        }
        default:

            /* No action */
        break;
    }
    return (ret_val);
}
/*****************************************************************************
 End of function  drv_control_sub1
 ******************************************************************************/

/******************************************************************************
 Function Name: drv_control_sub2
 Description:   Subordinate function to drv_control to reduce function length
 Function to handle custom controls for the device.
 Arguments:     IN  p_stream - Pointer to the file stream
 IN  ctl_code - The custom control code
 IN  p_ctl_struct - Pointer to the custom control structure
 Return value:  0 for success -1 on error or
                number of bytes in Rx buffer for CTL_GET_RX_BUFFER_COUNT
 ******************************************************************************/
static int_t drv_control_sub2(st_stream_ptr_t p_stream, CTLCODE ctl_code, void *p_ctl_struct)
{
    int_t ret_val = 0; // Assume Success
    pcdc_t p_sci_drv = p_stream->p_extension;
    switch (ctl_code)
    {
        case CTL_SET_TIME_OUT:
        {
            if (p_ctl_struct)
            {
                p_sci_drv->dwtimeout_ms = *((uint32_t *) p_ctl_struct);
            }
            else
            {
                p_sci_drv->dwtimeout_ms = SCI_DEFAULT_TIME_OUT;
            }
            break;
        }
        case CTL_GET_TIME_OUT:
        {
            if (p_ctl_struct)
            {
                *((uint32_t *) p_ctl_struct) = p_sci_drv->dwtimeout_ms;
            }
            else
            {
                p_sci_drv->last_error = SCI_INVALID_PARAMETER;
                ret_val = -1;
            }
            break;
        }
        case CTL_SCI_GET_LINE_STATUS:
        {
            if (p_ctl_struct)
            {
                /* Populate structure with data from where ever it can be gleaned */
                SCILST sci_lst;
                sci_lst.clearToSend = 0; /* Not supported in CDC ACM */
                sci_lst.dataSetReady = p_sci_drv->uart_state.tx_carrier;
                sci_lst.ringIndicator = p_sci_drv->uart_state.ring_signal;
                sci_lst.receiveLineSignalDetect = p_sci_drv->uart_state.rx_carrier;
                sci_lst.dataReady = 0; /* Not supported in CDC ACM */
                sci_lst.overrunError = p_sci_drv->uart_state.over_run;
                sci_lst.parityError = p_sci_drv->uart_state.parity;
                sci_lst.frameError = p_sci_drv->uart_state.framing;
                sci_lst.breakSignal = p_sci_drv->uart_state.break_b;
                sci_lst.breakOutput = (unsigned) (p_sci_drv->break_in_progress & 0x1);
                *((PSCILST) p_ctl_struct) = sci_lst;

                /* Now status has been read, clear down the UART status Structure */
                memset (&p_sci_drv->uart_state, 0, sizeof(uart_state_t));
            }
            else
            {
                p_sci_drv->last_error = SCI_INVALID_PARAMETER;
                ret_val = -1;
            }
            break;
        }
        case CTL_SCI_PURGE_BUFFERS:
        {
            /* This purge clears the local RX circular buffer */
            /* Future Extension: Issue class clear buffers request */
            cbClear (p_sci_drv->pcbuffer);
            break;
        }
        case CTL_SCI_SET_BREAK:
        {
            ret_val = (sci_sendbreak (p_sci_drv, CTL_SCI_SET_BREAK));
            break;
        }
        case CTL_SCI_CLEAR_BREAK:
        {
            ret_val = (sci_sendbreak (p_sci_drv, CTL_SCI_CLEAR_BREAK));
            break;
        }
        case CTL_GET_RX_BUFFER_COUNT:
        {
            ret_val = (int) cbUsed (p_sci_drv->pcbuffer);
            break;
        }
        case CTL_SCI_GET_CONFIGURATION:
        {
            if (p_ctl_struct)
            {
                /* Return the configuration of the device */
                *((PSCICFG) p_ctl_struct) = p_sci_drv->sci_config;
            }
            else
            {
                p_sci_drv->last_error = SCI_INVALID_PARAMETER;
                ret_val = -1;
            }
            break;
        }
        default:
        break;  // No Action
    }
    return (ret_val);
}
/*****************************************************************************
 End of function  drv_control_sub2
 ******************************************************************************/

/******************************************************************************
 Function Name: drv_control
 Description:   Function to handle custom controls for the device
 Arguments:     IN  p_stream - Pointer to the file stream
 IN  ctl_code - The custom control code
 IN  p_ctl_struct - Pointer to the custorm control structure
 Return value:  0 for success -1 on error or
                number of bytes in Rx buffer for CTL_GET_RX_BUFFER_COUNT
 ******************************************************************************/
static int_t drv_control(st_stream_ptr_t p_stream, uint32_t ctl_code, void *p_ctl_struct)
{
    int_t ret_val = 0;  // Assume success
    pcdc_t p_sci_drv = p_stream->p_extension;

    switch (ctl_code)
    {
        case CTL_GET_LAST_ERROR:
        case CTL_GET_ERROR_STRING:
        case CTL_USB_ATTACHED:
        case CTL_SCI_SET_CONFIGURATION:
        {
            ret_val = drv_control_sub1 (p_stream, ctl_code, p_ctl_struct);
            break;
        }

        case CTL_SET_TIME_OUT:
        case CTL_GET_TIME_OUT:
        case CTL_SCI_GET_LINE_STATUS:
        case CTL_SCI_PURGE_BUFFERS:
        case CTL_SCI_SET_BREAK:
        case CTL_SCI_CLEAR_BREAK:
        case CTL_GET_RX_BUFFER_COUNT:
        case CTL_SCI_GET_CONFIGURATION:
        {
            ret_val = drv_control_sub2 (p_stream, ctl_code, p_ctl_struct);
            break;
        }

        default:
        {
            p_sci_drv->last_error = SCI_INVALID_CONTROL_CODE;
            ret_val = -1;
            break;
        }
    }

    return (ret_val);
}
/******************************************************************************
 End of function  drv_control
 ******************************************************************************/

/******************************************************************************
 Function Name: drv_get_endpoint
 Description:   Function to get a pointer to an endpoint of particular transfer
 type and direction
 Arguments:     IN  pdevice - Pointer to the deivice information
 IN  transfer_direction - The endpoint transfer direction
 IN  transfer_type - The endpoint transfer type required
 Return value:  Pointer to the endpoint information or NULL if not found
 ******************************************************************************/
static PUSBEI drv_get_endpoint(PUSBDI pdevice, USBDIR transfer_direction, USBTT transfer_type)
{
    PUSBEI p_endpoint;
    if (USBH_SETUP != transfer_direction)
    {
        p_endpoint = pdevice->pEndpoint;
        while (p_endpoint)
        {
            if ((p_endpoint->transferDirection == transfer_direction) && (p_endpoint->transferType == transfer_type))
            {
                return p_endpoint;
            }
            p_endpoint = p_endpoint->pNext;
        }
    }
    return NULL;
}
/******************************************************************************
 End of function  drv_get_endpoint
 ******************************************************************************/

/*****************************************************************************
 Function Name: sci_proc_rx_data
 Description:   Function to process the received data
 Parameters:    IN  p_scidrv - Pointer to the driver data
 Return value:  none
 *****************************************************************************/
static void sci_proc_rx_data(pcdc_t p_scidrv)
{
    uint32_t uilength = p_scidrv->read_request.uiTransferLength;

    if (uilength)
    {
        uint8_t *pbybata = (uint8_t*) &p_scidrv->pby_rx_buffer;

        /* Buffer the received serial data */
        while (uilength--)
        {
            /* Put in the receive circular buffer */
            if (!cbPut (p_scidrv->pcbuffer, *pbybata++))
            {
                p_scidrv->last_error = SCI_OVERRUN_ERROR;
                uilength = 0;   // Exit the while loop
            }
        }
    }
}
/*****************************************************************************
 End of function  sci_proc_rx_data
 ******************************************************************************/

/*****************************************************************************
 Function Name: sci_start_rx_transfer
 Description:   Function to start the RX transfer
 Parameters:    IN  p_scidrv - Pointer to the driver data
 Return value:  true of the transfer was started
 *****************************************************************************/
static _Bool sci_start_rx_transfer(pcdc_t p_scidrv)
{
    _Bool bfresult;

    /* Start a new transfer */
    bfresult = usbhStartTransfer (p_scidrv->pdevice, &p_scidrv->read_request, p_scidrv->pin_endpoint,
                                  p_scidrv->pby_rx_buffer, (size_t) SCI_RX_PACKET_SIZE,
                                  REQ_IDLE_TIME_OUT_INFINITE);
    return bfresult;
}
/*****************************************************************************
 End of function  sci_start_rx_transfer
 ******************************************************************************/

/*****************************************************************************
 Function Name: sci_rx_poll_task
 Description:   Function to poll the IN endpoint for RX data
 Parameters:    IN  p_scidrv - Pointer to the driver data
 Return value:  none
 *****************************************************************************/
static void sci_rx_poll_task(pcdc_t p_scidrv)
{
    while (1)
    {
        switch (p_scidrv->sci_state)
        {
            case SCI_IDLE:
            {
                /* Start a transfer */
                if (sci_start_rx_transfer (p_scidrv))
                {
                    p_scidrv->sci_state = SCI_WAIT_RX_DATA;
                }
                break;
            }
            case SCI_WAIT_RX_DATA:
            {
                /* Wait for the signal to complete */
                R_OS_WaitForEvent(&p_scidrv->read_request.ioSignal, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);

                /* Check the request error code */
                if (p_scidrv->read_request.errorCode)
                {
                    /* The Rx request reported an error */

                    /* OS Delay - allows other tasks (such as enumerator) to run
                     * before scheduling next Rx request.*/
                    R_OS_TaskSleep(1);

                    /* Set the error code for return to read function */
                    p_scidrv->last_error = SCI_RX_ERROR;
                }
                else
                {
                    /* Process the received data */
                    sci_proc_rx_data (p_scidrv);
                }

                /* Start the next transfer */
                if (!sci_start_rx_transfer (p_scidrv))
                {
                    /* There was a problem starting the transfer */
                    p_scidrv->sci_state = SCI_IDLE;
                }
                break;
            }
            default:
            {
                /* Do nothing */
            }
        }
    }
}
/*****************************************************************************
 End of function  sci_rx_poll_task
 ******************************************************************************/

/*****************************************************************************
 Function Name: sci_start_int_transfer
 Description:   Function to start the RX transfer
 Parameters:    IN  p_scidrv - Pointer to the driver data
 Return value:  true of the transfer was started
 *****************************************************************************/
static _Bool sci_start_int_transfer(pcdc_t p_scidrv)
{
    _Bool bfresult;

    /* Start a new transfer */
    bfresult = usbhStartTransfer (p_scidrv->pdevice, &p_scidrv->line_request, p_scidrv->pint_endpoint,
                                  p_scidrv->pby_int_buffer, (size_t) p_scidrv->pint_endpoint->wPacketSize,
                                  REQ_IDLE_TIME_OUT_INFINITE);
    return bfresult;
}
/*****************************************************************************
 End of function  sci_start_int_transfer
 ******************************************************************************/

/*****************************************************************************
 Function Name: sci_int_poll_task
 Description:   Function to poll the interrupt IN endpoint for line status
 Parameters:    IN  p_scidrv - Pointer to the driver data
 Return value:  none
 *****************************************************************************/
static void sci_int_poll_task(pcdc_t p_scidrv)
{
    while (1)
    {
        switch (p_scidrv->int_state)
        {
            case SCI_IDLE:
            {
                /* Start a transfer */
                if (sci_start_int_transfer (p_scidrv))
                {
                    p_scidrv->int_state = SCI_WAIT_RX_DATA;
                }
                break;
            }
            case SCI_WAIT_RX_DATA:
            {
                /* Wait for the signal to complete */
                R_OS_WaitForEvent(&p_scidrv->line_request.ioSignal, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);

                /* Check the request error code */
                if (!p_scidrv->line_request.errorCode)
                {
                    /* Display the raw received message, this is likely to
                     * be caused by a change in Serial State (e.g. parity error)
                     * For more details of the encoding, please refer to section
                     * 6.5.4 of Universal Serial Bus Communications
                     * Class Subclass Specification for PSTN Devices Revision 1.2.
                     */
#ifdef DISPLAY_LINE_STATUS_STRING
                    /* Optional debug to display the received string on the console */
                    dbgPrintBuffer(p_scidrv->pby_int_buffer, p_scidrv->line_request.uiTransferLength);
#endif

                    /* Check if the packet received is a SERIAL_STATE message */
                    /* and decode */
                    if (((SCI_STATE_SIZE == p_scidrv->line_request.uiTransferLength)
                            && (SCI_STATE_REQ_TYPE == (*(uint8_t*) p_scidrv->pby_int_buffer)))
                            && (SCI_SERIAL_STATE == (*(uint8_t*) (p_scidrv->pby_int_buffer + 1))))
                    {
                        p_scidrv->uart_state = *(puart_state_t) (p_scidrv->pby_int_buffer + 8);
                    }
                }

                /* Start the next transfer */
                if (!sci_start_int_transfer (p_scidrv))
                {
                    p_scidrv->int_state = SCI_IDLE;
                }
                break;
            }
            default:
            {
                /* Do nothing */
            }
        }
    }
}
/*****************************************************************************
 End of function  sci_int_poll_task
 ******************************************************************************/
/******************************************************************************
 End  Of File
 ******************************************************************************/
