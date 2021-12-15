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
 * Copyright (C) 2016 Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************
 * File Name    : hwusbh_rskrza1_1.c
 * Version      : 1.10
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : USB Host driver hardware interface functions
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 02.04.2015 1.00 First Release
 *              : 09.02.2016 1.10 Updated for GSCE coding standards
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/

#include    "mcu_board_select.h"
#include <stdio.h>
#include <string.h>
    #include    <fcntl.h>
#include "iodefine_cfg.h"
#include "compiler_settings.h"
#include "r_devlink_wrapper.h"
#include "usbHostApi.h"
#include "r_riic_drv_sc_cfg.h"
#include "r_intc.h"
#include "r_task_priority.h"

#include "usb_iobitmask.h"
#include "usb20_iodefine.h"
#include "rza_io_regrw.h"

#include "Trace.h"

#if (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)
/* nothing */
#elif (TARGET_BOARD == TARGET_BOARD_RSK)
#include "riic_cat9554_if.h"
#endif /* TARGET_BOARD */


/******************************************************************************
 Defines
 ******************************************************************************/
/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

#define GPIO_BIT_N1  (1u <<  1)

/******************************************************************************
 Function Prototypes
 ******************************************************************************/
static int_t usb_host_open (st_stream_ptr_t p_stream);
static void usb_host_close (st_stream_ptr_t p_stream);
static int_t usb_host_no_io (st_stream_ptr_t p_stream, uint8_t *p_by_buffer, uint32_t ui_count);
static int_t usb_host_control (st_stream_ptr_t p_stream, uint32_t ctl_code, void *p_ctl_struct);
static os_task_code_t usb_host_enumerator (void *parameters);
static void hw_reset_port (_Bool bf_state);
static void hw_enable_port (_Bool bf_state);
static void hw_suspend_port (_Bool bf_state);
static uint32_t hw_status_port (void);
static void hw_power_port (_Bool bf_state);

/******************************************************************************
 Global Variables
 ******************************************************************************/

/* Flags set in over current interrupt */
static _Bool bf_over_current_port1 = false;

/* Flags to show device attached status */
static _Bool bf_attached1 = false;

/* The data for the host controller driver */
static USBHC usb_hc1;

static bool_t in_use = false;

/* The ID of the enumeration task */
/* OS variables */
static os_task_t *gp_ui_enum_taskid = ((void*)0);

/******************************************************************************
 Constant Data
 ******************************************************************************/

/* Define the driver function table for this device */
const st_r_driver_t g_usb1_host_driver =
{ "USB Host Port 1 Device Driver", usb_host_open, usb_host_close, usb_host_no_io, usb_host_no_io, usb_host_control, usb_host_no_io};

/* Group the port 0 control functions together */
const USBPC g_c_root_port1 =
{ hw_reset_port, hw_enable_port, hw_suspend_port, hw_status_port, hw_power_port, 0, &USB201 };

static const st_devlink_table_t st_device_link_tbl[] =
{
    {   /* USB Mass Storage Class devices */
        "Mass Storage",
        "MS BULK Only Device Driver",
    },
    {   /* USB HID Class keyboard devices */
        "HID Keyboard",
        "HID Keyboard Device Driver",
    },

    {   /* USB HID Class keyboard devices */
        "HID Mouse",
        "HID Mouse Device Driver",
    },

    {   /* Class support for USB CDC ACM Class */
        "USBCDC",
        "CDC Device Driver",
    }
};


/******************************************************************************
 Public Functions
 ******************************************************************************/

/******************************************************************************
 Private Functions
 ******************************************************************************/

/******************************************************************************
 Function Name: usb1_interrupt
 Description:   The interrupt service routine for the USB host driver
 Arguments:     uint32_t status - unused
 Return value:  none
 ******************************************************************************/
static void usb1_interrupt (uint32_t status)
{
    (void) status;

    /* Check for device attach port 0 */
    if (rza_io_reg_read_16(&INTSTS1_1, USB_INTSTS1_ATTCH_SHIFT, USB_INTSTS1_ATTCH) &
        rza_io_reg_read_16(&INTENB1_1, USB_INTENB1_ATTCHE_SHIFT, USB_INTENB1_ATTCHE))
    {
        /* Clear the flag by writing 0 to it */
        rza_io_reg_write_16(&INTSTS1_1, 0, USB_INTSTS1_ATTCH_SHIFT, USB_INTSTS1_ATTCH);
        rza_io_reg_write_16(&INTSTS1_1, 0, USB_INTSTS1_DTCH_SHIFT, USB_INTSTS1_DTCH);
        bf_attached1 = true;

        /* Disable the attach interrupt */
        rza_io_reg_write_16(&INTENB1_1, 0, USB_INTENB1_ATTCHE_SHIFT, USB_INTENB1_ATTCHE);

        /* Enable the detach interrupt */
        rza_io_reg_write_16(&INTENB1_1, 1, USB_INTENB1_DTCHE_SHIFT, USB_INTENB1_DTCHE);
    }

    /* Check for device detach port 0 */
    else if ( rza_io_reg_read_16(&INTSTS1_1, USB_INTSTS1_DTCH_SHIFT, USB_INTSTS1_DTCH) &
               rza_io_reg_read_16(&INTENB1_1, USB_INTENB1_DTCHE_SHIFT, USB_INTENB1_DTCHE))
    {
        /* Clear the flag by writing 0 to it */
        rza_io_reg_write_16(&INTSTS1_1, 0, USB_INTSTS1_ATTCH_SHIFT, USB_INTSTS1_ATTCH);
        rza_io_reg_write_16(&INTSTS1_1, 0, USB_INTSTS1_DTCH_SHIFT, USB_INTSTS1_DTCH);
        bf_attached1 = false;

        /* Disable the detach interrupt */
        rza_io_reg_write_16(&INTENB1_1, 0, USB_INTENB1_DTCHE_SHIFT, USB_INTENB1_DTCHE);

        /* Enable the attach interrupt */
        rza_io_reg_write_16(&INTENB1_1, 1, USB_INTENB1_ATTCHE_SHIFT, USB_INTENB1_ATTCHE);
    }
    else
    {
        /* Do nothing */
    }

    /* Call the driver interrupt handler */
    usbhInterruptHandler(&usb_hc1);
}
/******************************************************************************
 End of function  usb1_interrupt
 ******************************************************************************/

/******************************************************************************
 Function Name: usb_host_open
 Description:   Function to open the host controller
 Arguments:     IN  p_stream - Pointer to the file stream
 Return value:  0 for success otherwise -1
 ******************************************************************************/
static int_t usb_host_open (st_stream_ptr_t p_stream)
{
    (void) p_stream;
    volatile unsigned char dummy;
    int_t ret = (-1);

#if (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)
/* nothing */
#elif (TARGET_BOARD == TARGET_BOARD_RSK)
	uint8_t px_addr, px_data, px_config;
#endif /* TARGET_BOARD */

    if(false == in_use)
    {
        in_use = true;
        ret = 0;

        /* Disable the clock to the USB module */
        CPG.STBCR7 |= CPG_STBCR7_MSTP70;
        dummy = CPG.STBCR7;
        (void) dummy;
        R_OS_TaskSleep(200);

#if (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)
        /* USB VBUS VOLTAGE ACTIVATION  : P7_1, High Output */
        GPIO.PIBC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
        GPIO.PBDC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
        GPIO.PM7    |=   (uint32_t)GPIO_BIT_N1;
        GPIO.PMC7   &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
        GPIO.PIPC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);

        GPIO.PBDC7  &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
        GPIO.P7     |=   (uint32_t)GPIO_BIT_N1;
        GPIO.PM7    &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
#elif (TARGET_BOARD == TARGET_BOARD_RSK)
        /* USB VBUS VOLTAGE ACTIVATION  : PX2_USB_PWR_ENB Low Output */
        /* Defines for Port Expander 2 pins
         PX2_PX1_EN0 = HIGH (DV output from VIO)
         PX2_PX1_EN1 = HIGH (Ethernet)
         PX2_TFT_CS = LOW (TFT Chip Select)
         PX2_PX1_EN3 = HIGH
         PX2_USB_OVR_CUR - Input
         PX2_USB_PWR_ENA = LOW (USB Power enabled)
         PX2_USB_PWR_ENB = LOW (USB Power enabled)
         PX2_PX1_EN7 = LOW (A18..A21 address lines to NOR)
         */
    	R_RIIC_CAT9554_Open();
        px_addr = CAT9554_I2C_PX2;
	    px_data = PX2_PX1_EN0 | PX2_PX1_EN1 | PX2_PX1_EN3;
        px_config = PX2_PX1_EN0 | PX2_PX1_EN1 | PX2_TFT_CS | PX2_PX1_EN3 |
					PX2_USB_PWR_ENA | PX2_USB_PWR_ENB | PX2_PX1_EN7;
	    R_RIIC_CAT9554_Write(px_addr, px_data, px_config);
    	R_RIIC_CAT9554_Close();

#if 0
        /* USB VBUS VOLTAGE ACTIVATION  : PX2_USB_PWR_ENB, High Output */
        {
#define GNUH_RSK_I2C_PX_IO2                (0x42)
#define PX_CMD_WRITE_OUT_REG                (0x01u)

#define PX2_PX1_EN0 (0x01)
#define PX2_PX1_EN1 (0x02)
#define PX2_TFT_CS      (0x04)
#define PX2_PX1_EN3     (0x08)
#define PX2_USB_OVR_CUR (0x10)
#define PX2_USB_PWR_ENA (0x20)
#define PX2_USB_PWR_ENB (0x40)
#define PX2_PX1_EN7     (0x80)

            int_t iic3_handle = ( -1);
            st_r_drv_riic_create_t riic_clock;

        /* Defines for Port Expander 2 pins
         PX2_PX1_EN0 = HIGH (DV output from VIO)
         PX2_PX1_EN1 = HIGH (Ethernet)
         PX2_TFT_CS = LOW (TFT Chip Select)
         PX2_PX1_EN3 = HIGH
         PX2_USB_OVR_CUR - Input
         PX2_USB_PWR_ENA = LOW (USB Power enabled)
         PX2_USB_PWR_ENB = HIGH (USB Power enabled)
         PX2_PX1_EN3 = LOW (A18..A21 address lines to NOR)
         */
            uint8_t writedata = PX2_PX1_EN0|PX2_PX1_EN1|PX2_PX1_EN3|PX2_USB_PWR_ENB;

            /* i2c access control structure */
            st_r_drv_riic_config_t i2c_write;

            /* setup riic configuration */
            riic_clock.frequency = RIIC_FREQUENCY_100KHZ;

            /* configure eeprom device address on i2c channel */
            i2c_write.device_address = GNUH_RSK_I2C_PX_IO2;
            i2c_write.sub_address = PX_CMD_WRITE_OUT_REG;
            i2c_write.number_of_bytes = 1;
            i2c_write.p_data_buffer = (uint8_t *) &writedata;


            iic3_handle = open(DEVICE_INDENTIFIER "iic3", O_RDWR);
            /* Test if riic channel was opened successfully */
            if (0 <= iic3_handle)
            {
                /* Create the I2C channel with the 100kHz clock */
                if (0 == control(iic3_handle, CTL_RIIC_CREATE, &riic_clock))
                {
                    /* riic successfully created and configured */

                    /* read data from eeprom */
                    if (0 == control(iic3_handle, CTL_RIIC_WRITE, &i2c_write))
                    {
                    }
                }
            }

            /* close the I2C channel3 driver */
            close(iic3_handle);
        }
#endif
#endif

        /* Register the interrupt function */
        R_INTC_RegistIntFunc(INTC_ID_USBI1, usb1_interrupt);

        /* Set USB interrupt priority and enable */
        R_INTC_SetPriority( INTC_ID_USBI1, ISR_USBH_PRIORITY);
        R_INTC_Enable(INTC_ID_USBI1);

        /* Open the host driver */
        usbhOpen(&usb_hc1);

        /* Add the root ports to the driver */
        usbhAddRootPort(&usb_hc1, (const PUSBPC)&g_c_root_port1);

        /* Enable the clock to the USB module */
        CPG.STBCR7 &= (uint8_t)~(CPG_STBCR7_MSTP70|CPG_STBCR7_MSTP71);
        dummy = CPG.STBCR7;

        /* Initialise the USB hardware */
        R_USBH_Initialise(&USB201);

        /* Set the bus wait setting */
        rza_io_reg_write_16(&BUSWAIT_1, 3, USB_BUSWAIT_BWAIT_SHIFT, USB_BUSWAIT_BWAIT);

        /* Start the enumeration task, if it has not been started previously */
        if (((void*)0) == gp_ui_enum_taskid)
        {
            uint32_t tindex = 0;
            uint32_t tsize  = 0;

            gp_ui_enum_taskid = R_OS_CreateTask("USB Enumerator", (os_task_code_t) usb_host_enumerator, NULL, R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE, TASK_USB_ENMERATOR_PRI);

            /* Insert Dynamic Device Support */
            /* registerDevice link Table entries */
            tsize = (sizeof(st_device_link_tbl)/sizeof(st_devlink_table_t));
            for(tindex = 0; tindex < tsize; tindex++)
            {
                R_DEVLINK_InsertDeviceLinkTableEntry(st_device_link_tbl[tindex].p_class_link_name, st_device_link_tbl[tindex].p_driver_name);
            }
        }

        /* Enable the attach interrupts for each root port*/
        rza_io_reg_write_16(&INTENB1_1, 1, USB_INTENB1_ATTCHE_SHIFT, USB_INTENB1_ATTCHE);
    }

    return (ret);
}
/******************************************************************************
 End of function  usb_host_open
 ******************************************************************************/

/******************************************************************************
 Function Name: usb_host_close
 Description:   Function to close the host controller
 Arguments:     IN  p_stream - Pointer to the file stream
 Return value:  none
 ******************************************************************************/
static void usb_host_close (st_stream_ptr_t p_stream)
{
    (void) p_stream;
    volatile unsigned char dummy;

#if (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)
/* nothing */
#elif (TARGET_BOARD == TARGET_BOARD_RSK)
	uint8_t px_addr, px_data, px_config;
#endif /* TARGET_BOARD */

    if(true == in_use)
    {
        in_use = false;

        R_INTC_Disable(INTC_ID_USBI1);

        /* Stop the enumeration task */
        R_OS_DeleteTask(gp_ui_enum_taskid);

        gp_ui_enum_taskid = ((void*)0);

        /* Close the host driver */
        usbhClose(&usb_hc1);

        /* Uninitialise the USB hardware */
        R_USBH_Uninitialise(&USB201);

        /* Disable the clock to the USB module */
        CPG.STBCR7 |= CPG_STBCR7_MSTP70;
        dummy = CPG.STBCR7;
        (void) dummy;

#if (TARGET_BOARD == TARGET_BOARD_STREAM_IT2)
        /* USB VBUS VOLTAGE ACTIVATION  : P7_1, High Output */
        GPIO.PIBC7  |= ((uint32_t)GPIO_BIT_N1);
        GPIO.PBDC7  |= ((uint32_t)GPIO_BIT_N1);
        GPIO.PM7    &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
        GPIO.PMC7   |= ((uint32_t)GPIO_BIT_N1);
        GPIO.PIPC7  |= ((uint32_t)GPIO_BIT_N1);

        GPIO.PBDC7  |= ((uint32_t)GPIO_BIT_N1);
        GPIO.P7     &= (uint16_t)(~(uint32_t)GPIO_BIT_N1);
        GPIO.PM7    |= ((uint32_t)GPIO_BIT_N1);
#elif (TARGET_BOARD == TARGET_BOARD_RSK)
        /* USB VBUS VOLTAGE NON-ACTIVATION  : PX2_USB_PWR_ENB High Output */
        /* Defines for Port Expander 2 pins
         PX2_PX1_EN0 = HIGH (DV output from VIO)
         PX2_PX1_EN1 = HIGH (Ethernet)
         PX2_TFT_CS = LOW (TFT Chip Select)
         PX2_PX1_EN3 = HIGH
         PX2_USB_OVR_CUR - Input
         PX2_USB_PWR_ENA = LOW (USB Power enabled)
         PX2_USB_PWR_ENB = HIGH (USB Power disabled)
         PX2_PX1_EN7 = LOW (A18..A21 address lines to NOR)
         */
    	R_RIIC_CAT9554_Open();
        px_addr = CAT9554_I2C_PX2;
	    px_data = PX2_PX1_EN0 | PX2_PX1_EN1 | PX2_PX1_EN3 | PX2_USB_PWR_ENB;
        px_config = PX2_PX1_EN0 | PX2_PX1_EN1 | PX2_TFT_CS | PX2_PX1_EN3 |
					PX2_USB_PWR_ENA | PX2_USB_PWR_ENB | PX2_PX1_EN7;
	    R_RIIC_CAT9554_Write(px_addr, px_data, px_config);
    	R_RIIC_CAT9554_Close();

#if 0
        {
#define GNUH_RSK_I2C_PX_IO2                (0x42)
#define PX_CMD_WRITE_OUT_REG                (0x01u)

#define PX2_PX1_EN0 (0x01)
#define PX2_PX1_EN1 (0x02)
#define PX2_TFT_CS      (0x04)
#define PX2_PX1_EN3     (0x08)
#define PX2_USB_OVR_CUR (0x10)
#define PX2_USB_PWR_ENA (0x20)
#define PX2_USB_PWR_ENB (0x40)
#define PX2_PX1_EN7     (0x80)

            int_t iic3_handle = ( -1);
            st_r_drv_riic_create_t riic_clock;

        /* Defines for Port Expander 2 pins
         PX2_PX1_EN0 = HIGH (DV output from VIO)
         PX2_PX1_EN1 = HIGH (Ethernet)
         PX2_TFT_CS = LOW (TFT Chip Select)
         PX2_PX1_EN3 = HIGH
         PX2_USB_OVR_CUR - Input
         PX2_USB_PWR_ENA = LOW (USB Power enabled)
         PX2_USB_PWR_ENB = HIGH (USB Power enabled)
         PX2_PX1_EN3 = LOW (A18..A21 address lines to NOR)
         */
            uint8_t writedata = PX2_PX1_EN0|PX2_PX1_EN1|PX2_PX1_EN3;

            /* i2c access control structure */
            st_r_drv_riic_config_t i2c_write;

            /* setup riic configuration */
            riic_clock.frequency = RIIC_FREQUENCY_100KHZ;

            /* configure eeprom device address on i2c channel */
            i2c_write.device_address = GNUH_RSK_I2C_PX_IO2;
            i2c_write.sub_address = PX_CMD_WRITE_OUT_REG;
            i2c_write.number_of_bytes = 1;
            i2c_write.p_data_buffer = (uint8_t *) &writedata;


            iic3_handle = open(DEVICE_INDENTIFIER "iic3", O_RDWR);
            /* Test if riic channel was opened successfully */
            if (0 <= iic3_handle)
            {
                /* Create the I2C channel with the 100kHz clock */
                if (0 == control(iic3_handle, CTL_RIIC_CREATE, &riic_clock))
                {
                    /* riic successfully created and configured */

                    /* read data from eeprom */
                    if (0 == control(iic3_handle, CTL_RIIC_WRITE, &i2c_write))
                    {
                    }
                }
            }

            /* close the I2C channel3 driver */
            close(iic3_handle);
        }
#endif
#endif
    }
}
/******************************************************************************
 End of function  usb_host_close
 ******************************************************************************/

/******************************************************************************
 Function Name: usb_host_no_io
 Description:   Function used in place of read and write because operations are
 not supported
 Arguments:     IN  p_stream - Pointer to the file stream
 IN  p_by_buffer - Pointer to the memory
 IN  ui_count - The number of bytes to transfer
 Return value:  0 for success -1 on error
 ******************************************************************************/
static int_t usb_host_no_io (st_stream_ptr_t p_stream, uint8_t *p_by_buffer, uint32_t ui_count)
{
    (void) p_stream;
    (void) p_by_buffer;
    (void) ui_count;

    /* no function */
    return -1;
}
/******************************************************************************
 End of function  usb_host_no_io
 ******************************************************************************/

/******************************************************************************
 Function Name: usb_host_control
 Description:   Function to handle custom control functions for the USB host
 controller
 Arguments:     IN  p_stream - Pointer to the file stream
 IN  ctl_code - The custom control code
 IN  p_ctl_struct - Pointer to the custom control structure
 Return value:  0 for success -1 on error
 ******************************************************************************/
static int_t usb_host_control (st_stream_ptr_t p_stream, uint32_t ctl_code, void *p_ctl_struct)
{
    (void) p_stream;
    (void) ctl_code;
    (void) p_ctl_struct;
    return -1;
}
/******************************************************************************
 End of function  usb_host_control
 ******************************************************************************/

/******************************************************************************
 Function Name: usb_host_enumerator
 Description:   Function to call the enumerator every 1mS
 Arguments:     none
 Return value:  none
 ******************************************************************************/
static os_task_code_t usb_host_enumerator (void *parameters)
{
    UNUSED_PARAM(parameters);
    while (true)
    {
        enumRun();
        R_OS_TaskSleep(1UL);
    }

    return 0;
}
/******************************************************************************
 End of function  usb_host_enumerator
 ******************************************************************************/

/******************************************************************************
 Function Name: hw_reset_port
 Description:   Function to reset root port 1
 Arguments:     IN  bf_state - true to reset
 Return value:  none
 ******************************************************************************/
static void hw_reset_port (_Bool bf_state)
{
    if (bf_state)
    {
        /* Enable high speed */
        rza_io_reg_write_16(&SYSCFG0_1, 1, USB_SYSCFG_HSE_SHIFT, USB_SYSCFG_HSE);

        /* Set only the bus reset bit clearing all other bits */
        rza_io_reg_write_16(&DVSTCTR0_1, 1, USB_DVSTCTR0_USBRST_SHIFT, ACC_16B_MASK);
    }
    else
    {
        /* Clear the bus reset bit */
        rza_io_reg_write_16(&DVSTCTR0_1, 1, USB_DVSTCTR0_UACT_SHIFT, ACC_16B_MASK);
    }
}
/******************************************************************************
 End of function  hw_reset_port
 ******************************************************************************/

/******************************************************************************
 Function Name: hw_enable_port
 Description:   Function to enable / disable root port 0
 Arguments:     IN  bf_state - true to enable
 Return value:
 ******************************************************************************/
static void hw_enable_port (_Bool bf_state)

{
    if (bf_state)
    {
        rza_io_reg_write_16(&DVSTCTR0_1, 1, USB_DVSTCTR0_UACT_SHIFT, USB_DVSTCTR0_UACT);
    }
    else
    {
        rza_io_reg_write_16(&DVSTCTR0_1, 0, USB_DVSTCTR0_UACT_SHIFT, USB_DVSTCTR0_UACT);
    }
}
/******************************************************************************
 End of function  hw_enable_port
 ******************************************************************************/

/******************************************************************************
 Function Name: hw_suspend_port
 Description:   Function to suspend root port 0
 Arguments:     IN  bf_state - true to suspend
 Return value:
 ******************************************************************************/
static void hw_suspend_port (_Bool bf_state)
{
    if (bf_state)
    {
        rza_io_reg_write_16(&DVSTCTR0_1, 0, USB_DVSTCTR0_UACT_SHIFT, USB_DVSTCTR0_UACT);
    }
    else
    {
        rza_io_reg_write_16(&DVSTCTR0_1, 1, USB_DVSTCTR0_RESUME_SHIFT, USB_DVSTCTR0_RESUME);
    }
}
/******************************************************************************
 End of function  hw_suspend_port
 ******************************************************************************/

/******************************************************************************
 Function Name: hw_status_port
 Description:   Function to get the status of port 0
 Arguments:     none
 Return value:  The status of the port as defined by
 USBH_HUB_HIGH_SPEED_DEVICE
 USBH_HUB_LOW_SPEED_DEVICE
 USBH_HUB_PORT_POWER
 USBH_HUB_PORT_RESET
 USBH_HUB_PORT_OVER_CURRENT
 USBH_HUB_PORT_SUSPEND
 USBH_HUB_PORT_ENABLED
 USBH_HUB_PORT_CONNECT_STATUS
 ******************************************************************************/
static uint32_t hw_status_port (void)
{
    uint32_t dw_port_status = 0;

    /* Check the power status */
    /* RSK+ assume that board is configured in host mode with
     power applied. */
    dw_port_status |= USBH_HUB_PORT_POWER;

    /* Check the reset bits */
    if (rza_io_reg_read_16(&DVSTCTR0_1, USB_DVSTCTR0_USBRST_SHIFT, USB_DVSTCTR0_USBRST))
    {
        dw_port_status |= USBH_HUB_PORT_RESET;
    }

    /* Set the attached device speed bits */
    switch (rza_io_reg_read_16(&DVSTCTR0_1, USB_DVSTCTR0_RHST_SHIFT, USB_DVSTCTR0_RHST))
    {
        case 1 :
        {
            dw_port_status |= (USBH_HUB_LOW_SPEED_DEVICE | USBH_HUB_PORT_CONNECT_STATUS);
            break;
        }
        case 2 :
        {
            dw_port_status |= (USBH_HUB_PORT_CONNECT_STATUS);
            break;
        }
        case 3 :
        {
            dw_port_status |= (USBH_HUB_HIGH_SPEED_DEVICE | USBH_HUB_PORT_CONNECT_STATUS);
            break;
        }
        default :
        {
            /* Do nothing */
            break;
        }
    }

    /* Check the enabled state */
    if (rza_io_reg_read_16(&DVSTCTR0_1, USB_DVSTCTR0_UACT_SHIFT, USB_DVSTCTR0_UACT))
    {
        dw_port_status |= USBH_HUB_PORT_ENABLED;
    }
    else
    {
        dw_port_status |= USBH_HUB_PORT_SUSPEND;
    }
    if (bf_attached1)
    {
        dw_port_status |= USBH_HUB_PORT_CONNECT_STATUS;
    }

    /* Check the over-current flag */
    if (bf_over_current_port1)
    {
        dw_port_status |= USBH_HUB_PORT_OVER_CURRENT;
    }

    return dw_port_status;
}
/******************************************************************************
 End of function  hw_status_port
 ******************************************************************************/

/******************************************************************************
 Function Name: hw_power_port
 Description:   Function to control the power to port 0
 Arguments:     IN  bf_state - true to switch the power ON
 Return value:  none
 ******************************************************************************/
static void hw_power_port (_Bool bf_state)
{
    /* Avoid unused variable warning */
    (void) bf_state;

    /* RSK+ Have to assume that board is configured in host mode with
     power applied */
    bf_over_current_port1 = false;
}
/******************************************************************************
 End of function  hw_power_port
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
