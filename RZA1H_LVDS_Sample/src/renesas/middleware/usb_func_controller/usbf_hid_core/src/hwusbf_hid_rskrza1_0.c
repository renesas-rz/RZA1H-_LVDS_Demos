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
 * File Name    : hwusbf_hid_rskrza1_0
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
#include "usb_iobitmask.h"

/*    Following header file provides definition common to Upper and Low Level USB
    driver. */
#include "usb_common.h"

/*    Following header file provides definition for Low level driver. */
#include "r_usb_hal.h"

#include "r_usbf_core.h"

#include "usbf_hid_rskrza1_if.h"

#include "FreeRTOSConfig.h"

#include "trace.h"

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Defines
 ******************************************************************************/
/* The root port control functions */
#define GPIO_BIT_N1  (1u <<  1)

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

static int_t  usbf_hid_open (st_stream_ptr_t pStream);
static void   usbf_hid_close (st_stream_ptr_t pStream);
//static int_t  usbf_hid_no_io (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int_t  usbf_hid_read (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int_t  usbf_hid_write (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount);
static int_t  usbf_hid_control (st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct);

static void   usbf_hid_interrupt_handler_isr (uint32_t value);
static int_t ref_count = 0;

/* usb function channel configuration used buy the driver */
static volatile st_usb_object_t    channel;

/* configuration used buy the driver*/
static st_usbf_user_configuration_t config = {0};

/******************************************************************************
 Constant Data
 ******************************************************************************/

/* Define the driver function table for this device */

const st_r_driver_t g_usbf0_hid_driver =
{ "USB Func HID Port 0 Device Driver",
   usbf_hid_open,
   usbf_hid_close,
   usbf_hid_read,
   usbf_hid_write,
   usbf_hid_control,
   no_dev_get_version
};


/******************************************************************************
 Global Variables
 ******************************************************************************/

/******************************************************************************
 Public Functions
 ******************************************************************************/
void delay_ms(uint16_t Dcnt);

/******************************************************************************
 Private Functions
 ******************************************************************************/

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

    /* USB VBUS VOLTAGE ACTIVATION  : P7_1, High Output */
    GPIO.PIBC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
    GPIO.PBDC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
    GPIO.PM7    |=   (uint32_t)GPIO_BIT_N1;
    GPIO.PMC7   &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
    GPIO.PIPC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);

    GPIO.PBDC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
    GPIO.P7     |=   (uint32_t)GPIO_BIT_N1;
    GPIO.PM7    &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);


    /*Initialise the USB HID Class*/
    if( USB_ERR_OK == R_USB_HidInit(&channel, (uint8_t(*)[])config.descriptors.report_in.puc_data, (CB_REPORT_OUT)config.pcbout_report))
    {
        /* Initialized USB interrupt set priority lvl*/
        R_INTC_RegistIntFunc(INTC_ID_USBI0, usbf_hid_interrupt_handler_isr);
        R_INTC_SetPriority(INTC_ID_USBI0, ISR_USBF_HID_IRQ_PRIORITY);
        R_INTC_Enable(INTC_ID_USBI0);
    }
    else
    {
        printf("usbf_cdc_open R_USBHID_Init ERROR\r\n");
    }
}
/*******************************************************************************
End of function start_device
*******************************************************************************/

/*******************************************************************************
* Function Name: stop_device
* Description  : Disables the HID device
* Arguments    : none
* Return Value : none
*******************************************************************************/
static void stop_device(void)
{
    /* USB interrupt disable */
    R_INTC_Disable(INTC_ID_USBI0);

    rza_io_reg_write_8((uint8_t *)&CPG.STBCR7,
                            1,
                            CPG_STBCR7_MSTP71_SHIFT,
                            CPG_STBCR7_MSTP71);

    delay_ms(10);

}
/*******************************************************************************
End of function stop_device
*******************************************************************************/

/******************************************************************************
 Function Name: usbf_hid_open
 Description:   Function to open the host controller
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  0 for success otherwise -1
 ******************************************************************************/
static int_t usbf_hid_open (st_stream_ptr_t pStream)
{
    (void) pStream;

    /* ATTACH DEVICE EXTENSTION TO CHANNEL */

    if(NULL == g_usb_devices_events[0])
    {
        eventCreate(g_usb_devices_events, R_USB_SUPPORTED_CHANNELS);
    }

    if(eventSet(g_usb_devices_events[0]))
    {
        if(0 == ref_count )
        {
            ref_count++;

            /* Reset Configuration */
               memset(&config,0,sizeof(config));
               memset((st_usb_object_t *)&channel,0,sizeof(channel));

            channel.phwdevice = &USB200;

            /* Reset peripheral */
            rza_io_reg_write_8((uint8_t *)&CPG.STBCR7,
                                  1,
                                  CPG_STBCR7_MSTP71_SHIFT,
                                  CPG_STBCR7_MSTP71);

            delay_ms(10);
            return 0;
        }
        else
        {
            /* device in use */
            eventReset(g_usb_devices_events[1]);
        }
    }

    /* low level peripheral in use */
    return -1;
}
/******************************************************************************
 End of function  usbf_hid_open
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_hid_close
 Description:   Function to close the host controller
 Arguments:     IN  pStream - Pointer to the file stream
 Return value:  none
 ******************************************************************************/
static void usbf_hid_close (st_stream_ptr_t pStream)
{
    (void) pStream;

    if(0 != ref_count )
    {
        ref_count--;
        eventReset(g_usb_devices_events[0]);
    }
}
/******************************************************************************
 End of function  usbf_hid_close
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_hid_read
 Description:   Function used to write data to host
 Arguments:     IN  pStream - Pointer to the file stream
                IN  pbyBuffer - Pointer to the memory
                IN  uiCount - The number of bytes to transfer
 Return value:  Number of bytes read
 ******************************************************************************/
static int_t usbf_hid_read (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    int_t ret = -1;

    /* File stream is not used */
    UNUSED_PARAM(pStream);
    UNUSED_PARAM(pbyBuffer);
    UNUSED_PARAM(uiCount);

    return ret;
}
/******************************************************************************
 End of function  usbf_hid_read
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_hid_write
 Description:   Function used to write data to host
 Arguments:     IN  pStream - Pointer to the file stream
                IN  pbyBuffer - Pointer to the memory
                IN  uiCount - The number of bytes to transfer
 Return value:  0 for success -1 on error
 ******************************************************************************/
static int_t usbf_hid_write (st_stream_ptr_t pStream, uint8_t *pbyBuffer, uint32_t uiCount)
{
    int_t ret = 0;

    UNUSED_PARAM(pStream);
    UNUSED_PARAM(pbyBuffer);
    UNUSED_PARAM(uiCount);

    return ret;
}
/******************************************************************************
 End of function  usbf_hid_write
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_hid_control
 Description:   Function to handle custom control functions for the USB host
 controller
 Arguments:       IN  pStream - Pointer to the file stream
                  IN  ctlCode    - The custom control code
                  IN  pCtlStruct - Pointer to the custom control structure
 Return value:   0 or greater for success, -1 on error
 ******************************************************************************/
static int_t usbf_hid_control (st_stream_ptr_t pStream, uint32_t ctlCode, void *pCtlStruct)
{
    int_t ret = -1;

    /* File stream is not used */
    (void) pStream;

    if (ref_count)
    {
        switch(ctlCode)
        {
            case CTL_USBF_IS_CONNECTED:
            {
                ret = R_USB_HidIsConnected(&channel);
            }
            break;
            case CTL_USBF_GET_CONFIGURATION:
            {
                /* copy active configuration to user */
                memcpy(((st_usbf_user_configuration_t *)pCtlStruct), &config, sizeof(config));
                ret = 0;
            }
            break;
            case CTL_USBF_SET_CONFIGURATION:
            {
                /* copy user configuration to active */
                memcpy(&config, ((st_usbf_user_configuration_t *)pCtlStruct), sizeof(config));
                memcpy((void*)(&channel.descriptors), ((st_usbf_descriptors_t *)&config.descriptors), sizeof(config.descriptors));
                channel.hi_speed_enable = config.hi_speed_enable;

                channel.descriptors_initialised = 1;
                ret = 0;
            }
            break;
            case CTL_USBF_SEND_HID_REPORTIN:
            {
            	R_USB_HidReportIn(&channel, (uint8_t(*)[])pCtlStruct);
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
 End of function  usbf_hid_control
 ******************************************************************************/

/******************************************************************************
 Function Name: usbf_hid_interrupt_handler_isr
 Description:   Function to handle peripheral interrupts
 Arguments:     value - associated data may not be used
 Return value:  none
 ******************************************************************************/
static void   usbf_hid_interrupt_handler_isr (uint32_t value)
{
    UNUSED_PARAM(value);

    if(0 != ref_count )
    {
        R_USB_HalIsr(&channel);
    }
}
/*******************************************************************************
End of function usbf_hid_interrupt_handler_isr
*******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
