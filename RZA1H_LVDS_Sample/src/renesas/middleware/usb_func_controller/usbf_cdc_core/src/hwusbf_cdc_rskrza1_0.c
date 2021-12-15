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
 * File Name    : hwusbf_cdc_rskrza1_0
 * Version      : 1.00
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : USB function CDC driver hardware interface functions
 *              : Channel 0
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 09.02.2016 1.00 First Release
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

#include "iodefine_cfg.h"
#include "r_intc.h"
#include "r_task_priority.h"
#include "r_devlink_wrapper.h"
#include "rza_io_regrw.h"


/*    Following header file provides definition common to Upper and Low Level USB
    driver. */
#include "usb_common.h"

/*    Following header file provides definition for Low level driver. */
#include "r_usb_hal.h"

/*    Following header file provides definition for USB CDC applicaton. */
#include "r_usb_cdc.h"

#include "r_usbf_core.h"
#include "trace.h"

/* Comment this line out to turn ON module trace in this file */
//#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Defines
 ******************************************************************************/

/* USB Function driver must have higher priority than timer for enumerator */
#define USBF_CDC_INTERRUPT_PRIORITY (configMAX_API_CALL_INTERRUPT_PRIORITY + 1)

/* The root port control functions */
#define GPIO_BIT_N1  (1u <<  1)

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

static int_t  usbf_cdc_open (st_stream_ptr_t pStream);
static void   usbf_cdc_close (st_stream_ptr_t pStream);
//static int_t  usbf_cdc_no_io (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int_t  usbf_cdc_read (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int_t  usbf_cdc_write (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int_t  usbf_cdc_control (st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct);
static void   usbf_cdc_interrupt_handler_isr (uint32_t value);

static volatile st_usb_object_t channel;
static int_t           ref_count = 0;

/* configuration used buy the driver*/
static st_usbf_user_configuration_t config = {};

/******************************************************************************
 Constant Data
 ******************************************************************************/

/* Define the driver function table for this device */

const st_r_driver_t g_usbf0_cdc_driver =
{ "USB Func CDC Port 0 Device Driver",
   usbf_cdc_open,
   usbf_cdc_close,
   usbf_cdc_read,
   usbf_cdc_write,
   usbf_cdc_control,
   no_dev_get_version
};


/******************************************************************************
 Global Variables
 ******************************************************************************/

/******************************************************************************
 Public Functions
 ******************************************************************************/

/******************************************************************************
 Private Functions
 ******************************************************************************/


/******************************************************************************
Function Name : ch0_readcb
Description   : Callback called when a USBCDC_Read_Async request
                has completed. i.e. Have read some data.
Parameters:     _err: Error code.
Return value:   -
*******************************************************************************/
static void ch0_readcb(volatile st_usb_object_t *_pchannel, usb_err_t _err, uint32_t NumBytes)
{
    (void) _pchannel;

    if(NULL != channel.rw_config.pout_done_async)
    {
        channel.rw_config.pout_done_async(_err, NumBytes);
    }
    else
    {
        TRACE(("ch0_readcb not connected \r\n"));
    }
}
/*******************************************************************************
End of function ch0_readcb
*******************************************************************************/

/******************************************************************************
Function Name : ch0_writecb
Description   : Callback called when a USBCDC_Write_Async request
                has completed. i.e. Have written some data.
Parameters:     _err: Error code.
Return value:   -
*******************************************************************************/
static void ch0_writecb(usb_err_t _err)
{
    if(NULL != channel.rw_config.pin_done_async)
    {
        channel.rw_config.pin_done_async(_err);
    }
    else
    {
        TRACE(("ch0_writecb not connected \r\n"));
    }
}
/*******************************************************************************
End of function ch0_writecb
*******************************************************************************/

/*******************************************************************************
* Function Name: start_device
* Description  : Initialises the HID device with specified configuration
* Arguments    : none
* Return Value : none
*******************************************************************************/
static void start_device(void)
{
    rza_io_reg_write_8((uint8_t *)&CPG.STBCR7,
                            0,
                            CPG_STBCR7_MSTP71_SHIFT,
                            CPG_STBCR7_MSTP71);

    R_OS_TaskSleep(200);

    /* USB VBUS VOLTAGE DISABLE : P7_1, Low Output */
    GPIO.PIBC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
    GPIO.PBDC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
    GPIO.PM7    |=   (uint32_t)GPIO_BIT_N1;
    GPIO.PMC7   &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
    GPIO.PIPC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);

    /* Disabled for CDC sample as supplying 5 volts from a target to PC/source is NOT recommended (BAD)
     * In this sample we are using a USB Host port to allow USB function connection */
    GPIO.P7     &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
    GPIO.PM7    &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);


    /* Force High speed USB */
    channel.hi_speed_enable = 1;

    /*Initialise the USB CDC Class*/
    if(USB_ERR_OK == R_USB_CdcInit(&channel))
    {
        /* Initialized USB interrupt set priority lvl*/
        R_INTC_RegistIntFunc(INTC_ID_USBI0, usbf_cdc_interrupt_handler_isr);
        R_INTC_SetPriority(INTC_ID_USBI0, USBF_INTERRUPT_PRIORITY);
        R_INTC_Enable(INTC_ID_USBI0);
    }
}
/*******************************************************************************
End of function start_device
*******************************************************************************/

/*******************************************************************************
* Function Name: stop_device
* Description  : Disables the CDC device
* Arguments    : none
* Return Value : none
*******************************************************************************/
static void stop_device(void)
{
    /* USB cancel active transaction */
    R_USB_CdcCancel(&channel);

    /* USB close */
    R_USB_HalClose(&channel);

    /* USB interrupt disable */
    R_INTC_Disable(INTC_ID_USBI0);

    /* Force host (if connected) to rescan for devices */
//    R_USB_HalInit(&channel, NULL, NULL, NULL);

    rza_io_reg_write_8((uint8_t *)&CPG.STBCR7,
                            1,
                            CPG_STBCR7_MSTP71_SHIFT,
                            CPG_STBCR7_MSTP71);

    R_INTC_SetPriority(INTC_ID_USBI0, ISR_ENTRY_UNUSED);

    R_OS_TaskSleep(100);

}
/*******************************************************************************
End of function stop_device
*******************************************************************************/

/******************************************************************************
 Function Name: usbf_cdc_open
 Description:   Function to open the host controller
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  0 for success otherwise -1
 ******************************************************************************/
static int_t usbf_cdc_open (st_stream_ptr_t pStream)
{
    UNUSED_PARAM(pStream);

    if(NULL == g_usb_devices_events[0])
    {
        eventCreate(g_usb_devices_events, R_USB_SUPPORTED_CHANNELS);
    }

    if(eventSet(g_usb_devices_events[0]))
    {
        if(0 == ref_count )
        {
            ref_count++;

            memset(&config,0,sizeof(config));
            memset((st_usb_object_t *)&channel,0,sizeof(channel));

            channel.phwdevice = &USB200;

            /* Reset peripheral */
            rza_io_reg_write_8((uint8_t *)&CPG.STBCR7,
                                  1,
                                  CPG_STBCR7_MSTP71_SHIFT,
                                  CPG_STBCR7_MSTP71);

            return 0;
        }
        else
        {
            /* device in use */
            eventReset(g_usb_devices_events[0]);
            return -1;
        }
    }
    else
    {
        /* low level peripheral in use */
        return -1;
    }}
/******************************************************************************
 End of function  usbf_cdc_open
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_cdc_close
 Description:   Function to close the host controller
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  none
 ******************************************************************************/
static void usbf_cdc_close (st_stream_ptr_t pStream)
{
    UNUSED_PARAM(pStream);

    if(0 != ref_count )
    {
        ref_count--;

        eventReset(g_usb_devices_events[0]);
        memset(&config,0,sizeof(config));
        memset((st_usb_object_t *)&channel,0,sizeof(channel));

    }
}
/******************************************************************************
 End of function  usbf_cdc_close
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_cdc_read
 Description:   Function used to write data to host
 Arguments:     IN  pStream - Pointer to the file stream
                IN  pbyBuffer - Pointer to the memory
                IN  uiCount - The number of bytes to transfer
 Return value:  Number of bytes read
 ******************************************************************************/
static int_t usbf_cdc_read (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    int_t ret = -1;

    /* File stream is not used */
    (void) pStream;


    if (ref_count)
    {
        switch(channel.rw_config.mode)
        {
            case USBF_ASYNC:
            {
                ret = R_USB_CdcReadAsync(&channel, uiCount, pbyBuffer, (CB_DONE_OUT)ch0_readcb);
                if(0 != ret)
                {
                    ret = R_USB_CdcReadAsync(&channel, uiCount, pbyBuffer, (CB_DONE_OUT)ch0_readcb);
                }
            }
            break;
            case USBF_NORMAL:
            {
                uint32_t sizein = 0;
                if(USB_ERR_OK == R_USB_CdcRead(&channel, uiCount, pbyBuffer, &sizein))
                {
                    ret = (int_t) sizein;
                }
            }
            break;
            default:
            {
                ret = -1;
            }
        }
    }
    return ret;
}
/******************************************************************************
 End of function  usbf_cdc_read
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_cdc_write
 Description:   Function used to write data to host
 Arguments:     IN  pStream - Pointer to the file stream
                IN  pbyBuffer - Pointer to the memory
                IN  uiCount - The number of bytes to transfer
 Return value:  0 for success -1 on error
 ******************************************************************************/
static int_t usbf_cdc_write (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    int_t ret = 0;

    /* File stream is not used */
    (void) pStream;


    if (ref_count)
    {
        switch(channel.rw_config.mode)
        {
            case USBF_ASYNC:
            {
                ret = R_USB_CdcWriteAsync(&channel, uiCount, pbyBuffer, ch0_writecb);
            }
            break;
            case USBF_NORMAL:
            {
                ret = R_USB_CdcWrite(&channel, uiCount, pbyBuffer);
            }
            break;
            default:
            {
                ret = -1;
            }
        }
    }
    return ret;
}
/******************************************************************************
 End of function  usbf_cdc_write
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_cdc_control
 Description:   Function to handle custom control functions for the USB host
 controller
 Arguments:       IN  pStream - Pointer to the file stream
                  IN  ctlCode    - The custom control code
                  IN  pCtlStruct - Pointer to the custom control structure
 Return value:   0 or greater for success, -1 on error
 ******************************************************************************/
static int_t usbf_cdc_control (st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct)
{
    int_t ret = 0;

    /* File stream is not used */
    (void) pStream;

    if (ref_count)
    {
        switch(ctlCode)
        {
            case CTL_USBF_IS_CONNECTED:
            {
                ret = R_USB_CdcIsConnected(&channel);
            }
            break;
            case CTL_USBF_SET_RW_MODE:
            {
                switch(((st_usbf_asyn_config_t *)pCtlStruct)->mode)
                {
                    case USBF_ASYNC:
                    {
                        channel.rw_config.mode = USBF_ASYNC;
                        channel.rw_config.pin_done_async = ((st_usbf_asyn_config_t *)pCtlStruct)->pin_done_async;
                        channel.rw_config.pout_done_async = ((st_usbf_asyn_config_t *)pCtlStruct)->pout_done_async;
                    }
                    break;
                    case USBF_NORMAL:
                    {
                        channel.rw_config.mode = USBF_NORMAL;
                        channel.rw_config.pin_done_async = NULL;
                        channel.rw_config.pout_done_async = NULL;
                    }
                    break;
                    default:
                    {
                        ret = -1;
                    }
                }
            }
            break;
            case CTL_USBF_START:
            {
                start_device();
            }
            break;
            case CTL_USBF_STOP:
            {
                stop_device();
            }
            break;
            default:
            {
                ret = -1;
            }
        }
    }
    return ret;
}
/******************************************************************************
 End of function  usbf_cdc_control
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_cdc_interrupt_handler_isr
 Description:   Function to handle peripheral interrupts
 Arguments:     value - associated data may not be used
 Return value:  none
 ******************************************************************************/
static void   usbf_cdc_interrupt_handler_isr (uint32_t value)
{
    value = 0;

    if(value != (uint32_t)ref_count)
    {
        R_USB_HalIsr(&channel);
    }
}
/*******************************************************************************
End of function usbf_cdc_interrupt_handler_isr
*******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
