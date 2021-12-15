/******************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only
 * intended for use with Renesas products. No other uses are authorized.
 * This software is owned by Renesas Electronics Corporation and is  protected
 * under all applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES
 * REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY,
 * INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR  A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE  EXPRESSLY
 * DISCLAIMED.
 * TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
 * ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE  LIABLE
 * FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
 * FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS
 * AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this
 * software and to discontinue the availability of this software.
 * By using this software, you agree to the additional terms and
 * conditions found by accessing the following link:
 * http://www.renesas.com/disclaimer
 *******************************************************************************
 * Copyright (C) 2016 Renesas Electronics Corporation. All rights reserved.
 ******************************************************************************
 * File Name    : hwDmaIf.c
 * Version      : 2.00
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : This module interfaces with the dma driver to provide the
 *                support required functionality. Channels used for dma
 *                are now allocated in the dma driver file:
 *                drivers\r_dmac\inc\r_dmac_drv_sc_cfg.h
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 28.05.2015 1.0  First Release
 *              : 28.01.2016 1.10 Updated API to provide a level of HW
 *                                abstraction. USB functions still here for
 *                                legacy reasons. Improved GSCE compliance.
 *              : 28.10.2019 2.00 Move interface from direct access to using
 *                                the r_dmac driver.
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 User Includes
 ******************************************************************************/
#include <fcntl.h>

#include "iodefine_cfg.h"
#include "compiler_settings.h"

#include "trace.h"
#include "hwDmaIf.h"
#include "r_intc.h"
#include "r_task_priority.h"

#include "dmac_iobitmask.h"

#ifdef R_SELF_LOAD_MIDDLEWARE_USB_HOST_CONTROLLER
#include "ddusbh.h"
#include "usbhConfig.h"
#include "usb20_iodefine.h"
#include "r_cache_l1_rz_api.h"
#endif

#include "r_dmac_drv_api.h"


/******************************************************************************
 Function Macros
 ******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/* R_OS_SysLock / R_OS_SysUnlock are required when nested interrupts are supported in your
 * device. They are not required when nesting is impossible */
/* Remove comment on line below to turn nested interrupt support in this file */
#define NESTED_SUPPORT (1)

/******************************************************************************
 Function Prototypes
 *****************************************************************************/
static void INT_D1FIFO0_COMPLETE (uint32_t dummy);
static void INT_D0FIFO1_COMPLETE (uint32_t dummy);
static void INT_D1FIFO1_COMPLETE (uint32_t dummy);
static void dmaFreeVectD0FIFO0 (void);
static void dmaFreeVectD0FIFO1 (void);
static void dmaFreeVectD1FIFO0 (void);
static void dmaFreeVectD1FIFO1 (void);

/******************************************************************************
 Global Variables
 *****************************************************************************/

/* The parameters for the call-back functions */
static void *pv_param_ch0 = NULL;
static void *pv_param_ch1 = NULL;

/* The call back function pointers */
static void (*gpf_complete0) (void *) = NULL;
static void (*gpf_complete1) (void *) = NULL;

/* The vector free functions */
static void (*gpfFreeVectUsbOutCh0) (void) = NULL;
static void (*gpfFreeVectUsbInCh1) (void) = NULL;

/* The length of the transfers used in USB functions*/
static size_t gst_lengthCh0 = 0UL;
static size_t gstTransferLengthCh0 = 0UL;
static size_t gst_lengthCh1 = 0UL;
static size_t gstTransferLengthCh1 = 0UL;

/* DMA driver */
static int_t dma_usb_out_handle = -1;
static int_t dma_usb_in_handle = -1;

static void *ch0out_source;
static void *ch0out_destination;
static void *inch1_source;
static void *inch1_destination;

/******************************************************************************
 Public Functions
 *****************************************************************************/

/******************************************************************************
 * Function Name: usbOpenDmaDriver
 * Description  : Open the DMA driver for USBN out and in
 * Arguments    : none
 * Return Value : DRV_SUCCESS or DRV_ERROR
 *****************************************************************************/
int_t usbOpenDmaDriver(void)
{
    if (dma_usb_out_handle < 0)
    {
        dma_usb_out_handle = open(DEVICE_INDENTIFIER "dma_usb_out", O_WRONLY);
    }

    if (dma_usb_in_handle < 0)
    {
        dma_usb_in_handle = open(DEVICE_INDENTIFIER "dma_usb_in", O_RDONLY);
    }

    if ((dma_usb_out_handle < 0) || (dma_usb_out_handle < 0))
    {
        return (DRV_ERROR);
    }

    return (DRV_SUCCESS);
}
/******************************************************************************
 End of function dmaStartUsbOutCh0
 *****************************************************************************/

/******************************************************************************
 Function Name: dmaStartUsbOutCh0
 Description:   Function to start DMA channel 0 for a USB OUT transfer
 This is where the DMAC writes to the designated pipe FIFO.
 In this implementation the assignment is DMA Channel 0 always
 uses the USB D0FIFO.

 Direction: Buffer memory --> USB Channel

 Arguments:
 IN  pvSrc - Pointer to the 4 byte aligned source memory
 (CAUTION: If using internal SRAM ensure this is a *Mirrored Address* (6xxxxxxx)
 native (2xxxxxxx) addresses corrupts data when DMA transfer crosses blocks)
 IN  st_length - The length of data to transfer
 IN  p_fifo - Pointer to the destination FIFO
 IN  pv_param - Pointer to pass to the completion routine
 IN  pf_complete - Pointer to the completion routine
 Return value:  none
 ******************************************************************************/
void dmaStartUsbOutCh0 (void *pvSrc, size_t st_length, void *p_fifo, void *pv_param, void (*pf_complete)(void *pv_param))
{
    static st_r_drv_dmac_config_t dma_config;

	/* check the DMA driver */
    if (dma_usb_out_handle < 0)
    {
        return;
    }

#if NESTED_SUPPORT
    int_t imask = R_OS_SysLock(NULL);
#endif

	ch0out_source = pvSrc;
	ch0out_destination = p_fifo;

    PUSBTR prequest = (PUSBTR) pv_param;

    /* configure the DMA transfer */
    if ((&USB200) == prequest->pUSB)
    {
    	dma_config.config.resource = DMA_RS_USB0_DMA0_TX;
        gpfFreeVectUsbOutCh0 = dmaFreeVectD0FIFO0;
    }
    else
    {
    	dma_config.config.resource = DMA_RS_USB0_DMA1_TX;
        gpfFreeVectUsbOutCh0 = dmaFreeVectD0FIFO1;
    }

    R_CACHE_L1_CleanInvalidLine((uint32_t) pvSrc, st_length);

    dma_config.config.source_width = DMA_DATA_SIZE_4;                         /* DMA transfer unit size (source) - 32 bits */
    dma_config.config.destination_width = DMA_DATA_SIZE_4;                    /* DMA transfer unit size (destination) - 32 bits */
    dma_config.config.source_address_type = DMA_ADDRESS_INCREMENT;            /* DMA address type (source) */
    dma_config.config.destination_address_type = DMA_ADDRESS_FIX;             /* DMA address type (destination) */
    dma_config.config.direction = DMA_REQUEST_SOURCE;                         /* DMA transfer direction will be set by the driver */
    dma_config.config.source_address = pvSrc;                                 /* Source Address */
    dma_config.config.destination_address = p_fifo;                           /* Destination Address */
    dma_config.config.count = st_length;                                      /* length */
    dma_config.config.p_dmaComplete = INT_D0FIFO1_COMPLETE;                   /* set callback function (DMA end interrupt) */

    control(dma_usb_out_handle, CTL_DMAC_SET_CONFIGURATION, (void *) &dma_config);

    TRACE(("hwDmaif: DMA out len = 0x%.8lX from address 0x%.8lX\r\n", gstTransferLengthCh0, pvSrc));

    /* Set the pipe number and completion routine */
    pv_param_ch0 = pv_param;
    gpf_complete0 = pf_complete;
    gstTransferLengthCh0 = st_length;

    /* Set the remainder which will need to be processed by another transfer */
    gst_lengthCh0 = st_length - gstTransferLengthCh0;

    /* trigger the DMA transfer */
    control(dma_usb_out_handle, CTL_DMAC_ENABLE, NULL);

#if NESTED_SUPPORT
    R_OS_SysUnlock(NULL, imask);
#endif
}
/******************************************************************************
 End of function dmaStartUsbOutCh0
 *****************************************************************************/

/******************************************************************************
 Function Name: dmaGetUsbOutCh0Count
 Description:   Function to get the current DMA transfer count
 Parameters:    none
 Return value:  The value of the transfer count register
 *****************************************************************************/
unsigned long dmaGetUsbOutCh0Count (void)
{
    uint32_t transfer_byte_count;

    control(dma_usb_out_handle, CTL_DMAC_GET_TRANSFER_BYTE_COUNT, (void *) &transfer_byte_count);

    return (transfer_byte_count);
}
/******************************************************************************
 End of function dmaGetUsbOutCh0Count
 *****************************************************************************/

/******************************************************************************
 Function Name: dmaStopUsbOutCh0
 Description:   Function to stop a USB OUT DMA transfer on channel 0
 Parameters:    none
 Return value:  none
 *****************************************************************************/
void dmaStopUsbOutCh0 (void)
{
    TRACE(("dmaStopUsbOutCh0: Called\r\n"));

#if NESTED_SUPPORT
    int_t imask = R_OS_SysLock(NULL);
#endif

    control(dma_usb_out_handle, CTL_DMAC_DISABLE, NULL);

    if (gpfFreeVectUsbOutCh0)
    {
        gpfFreeVectUsbOutCh0();
        gpfFreeVectUsbOutCh0 = NULL;
    }

#if NESTED_SUPPORT
    R_OS_SysUnlock(NULL, imask);
#endif
}
/******************************************************************************
 End of function dmaStopUsbOutCh0
 *****************************************************************************/

/******************************************************************************
 Function Name: dmaStartUsbInCh1
 Description:   Function to start DMA channel 1 for a USB IN transfer
 This is where the DMAC writes to the designated pipe FIFO.
 In this implementation the assignment is DMA Channel 1 always
 uses the USB D1FIFO.

 Direction: USB Channel --> Buffer Memory

 Arguments:
 IN  pv_dest - Pointer to the 4 byte aligned destination memory
 (CAUTION: If using internal SRAM ensure this is a *Mirrored Address* (6xxxxxxx)
 native (2xxxxxxx) addresses corrupts data when DMA transfer crosses blocks)

 IN  st_length - The length of data to transfer
 IN  p_fifo - Pointer to the source FIFO
 IN  pv_param - Pointer to pass to the completion routine
 IN  pf_complete - Pointer to the completion routine
 Return value:  none
 *****************************************************************************/
void dmaStartUsbInCh1 (void *pv_dest, size_t st_length, void *p_fifo, void *pv_param, void (*pf_complete) (void *pv_param))
{
    static st_r_drv_dmac_config_t dma_config;

#if NESTED_SUPPORT
    int_t imask = R_OS_SysLock(NULL);
#endif

    /* check the DMA driver */
    if (dma_usb_in_handle < 0)
    {
        return;
    }

    PUSBTR prequest = (PUSBTR) pv_param;

    inch1_source = p_fifo;
    inch1_destination = pv_dest;

    /* configure the DMA transfer */
    if ((&USB200) == prequest->pUSB)
    {
    	dma_config.config.resource = DMA_RS_USB0_DMA0_RX;
        gpfFreeVectUsbInCh1 = dmaFreeVectD1FIFO0;
    }
    else
    {
    	dma_config.config.resource = DMA_RS_USB0_DMA1_RX;
        gpfFreeVectUsbInCh1 = dmaFreeVectD1FIFO1;
    }

    R_CACHE_L1_CleanInvalidLine((uint32_t) pv_dest, st_length);

    dma_config.config.source_width = DMA_DATA_SIZE_4;                         /* DMA transfer unit size (source) - 32 bits */
    dma_config.config.destination_width = DMA_DATA_SIZE_4;                    /* DMA transfer unit size (destination) - 32 bits */
    dma_config.config.source_address_type = DMA_ADDRESS_FIX;                  /* DMA address type (source) */
    dma_config.config.destination_address_type = DMA_ADDRESS_INCREMENT;       /* DMA address type (destination) */
    dma_config.config.direction = DMA_REQUEST_SOURCE;                         /* DMA transfer direction will be set by the driver */
    dma_config.config.source_address = p_fifo;                                /* Source Address */
    dma_config.config.destination_address = pv_dest;                          /* Destination Address */
    dma_config.config.count = st_length;                                      /* length */
    dma_config.config.p_dmaComplete = INT_D1FIFO0_COMPLETE;                   /* set callback function (DMA end interrupt) */

    control(dma_usb_in_handle, CTL_DMAC_SET_CONFIGURATION, (void *) &dma_config);

    TRACE(("hwDmaif: DMA in len = 0x%.8lX to address 0x%.8lX Int: %d\r\n", gstTransferLengthCh1, pv_dest, 0));

    /* Set the pipe number and completion routine */
    pv_param_ch1 = pv_param;
    gpf_complete1 = pf_complete;
    gstTransferLengthCh1 = st_length;

    /* Set the remainder which will need to be processed by another transfer */
    gst_lengthCh1 = st_length - gstTransferLengthCh1;

    /* trigger the DMA transfer */
    control(dma_usb_in_handle, CTL_DMAC_ENABLE, NULL);

#if NESTED_SUPPORT
    R_OS_SysUnlock(NULL, imask);
#endif
}
/******************************************************************************
 End of function dmaStartUsbInCh1
 ******************************************************************************/

/******************************************************************************
 Function Name: dmaGetUsbInCh1Count
 Description:   Function to get the USB DMA transfer on channel 1
 Parameters:    none
 Return value:  The value of the transfer count register
 *****************************************************************************/
unsigned long dmaGetUsbInCh1Count (void)
{
    uint32_t transfer_byte_count;

    control(dma_usb_in_handle, CTL_DMAC_GET_TRANSFER_BYTE_COUNT, (void *) &transfer_byte_count);

    return (transfer_byte_count);
}
/******************************************************************************
 End of function dmaGetUsbInCh1Count
 *****************************************************************************/

/******************************************************************************
 Function Name: dmaStopUsbInCh1
 Description:   Function to stop a USB OUT DMA transfer on channel 1
 Parameters:    none
 Return value:  none
 *****************************************************************************/
void dmaStopUsbInCh1 (void)
{
    TRACE(("dmaStopUsbInCh1: Called\r\n"));

    control(dma_usb_in_handle, CTL_DMAC_DISABLE, NULL);

    if (gpfFreeVectUsbInCh1)
    {
        gpfFreeVectUsbInCh1();
        gpfFreeVectUsbInCh1 = NULL;
    }
}
/******************************************************************************
 End of function dmaStopUsbInCh1
 *****************************************************************************/

/******************************************************************************
 Private Functions
 *****************************************************************************/

/******************************************************************************
 * Function Name: dmaCompleteUsbCh0Out
 * Description  : Function to complete the USB DMA out transfer
 * Arguments    : none
 * Return Value : none
 *****************************************************************************/
static void dmaCompleteUsbCh0Out (void)
{
	/* Check to see if there is more data to be transferred */
    if (gst_lengthCh0)
    {
        /* Start another transfer continuing with the same source and
         destination addresses */
        dmaStartUsbOutCh0(ch0out_source, gst_lengthCh0,
        		          ch0out_destination, pv_param_ch0, gpf_complete0);
    }
    else
    {
        /* If there is a completion routine */
        if (gpf_complete0)
        {
            /* Call it */
            gpf_complete0(pv_param_ch0);
        }
    }
}
/******************************************************************************
 End of function dmaCompleteUsbCh0Out
 *****************************************************************************/

/******************************************************************************
 * Function Name: dmaCompleteUsbCh1Indir
 * Description  : Function to complete the USB DMA in transfer
 * Arguments    : none
 * Return Value : none
 *****************************************************************************/
static void dmaCompleteUsbCh1In (void)
{
    TRACE(("dmaCompleteUsbCh1In: Called\r\n"));

    /*  Software Countermeasure for DMA Restrictions are not required because
     the following conditions are satisfied:
     1. DMA/DTC is in block transfer mode when DISEL = 0
     2. The number of receive blocks matches the value set in the USB
     so that another transfer request is not generated until this
     interrupt handler is vectored */
    /* Check to see if there is more data to be transferred */
    if (gst_lengthCh1)
    {
        /* Start another transfer continuing with the same source and
         destination addresses */
        dmaStartUsbInCh1(inch1_source, gst_lengthCh1,
        		         inch1_destination, pv_param_ch1, gpf_complete1);
    }
    else
    {
        /* If there is a completion routine */
        if (gpf_complete1)
        {
            /* Call it */
            gpf_complete1(pv_param_ch1);
        }
    }
}
/******************************************************************************
 End of function dmaCompleteUsbCh1In
 *****************************************************************************/

/******************************************************************************
 Function Name: INT_D1FIFO0_COMPLETE
 Description:   USB_D1FIFO0 (USB IN  - device to host) end of transfer
 interrupt
 Parameters:    none
 Return value:  none
 ******************************************************************************/
static void INT_D1FIFO0_COMPLETE (uint32_t dummy)
{
    (void) dummy;
    dmaCompleteUsbCh1In();
}
/******************************************************************************
 End of function INT_D1FIFO0_COMPLETE
 ******************************************************************************/

/******************************************************************************
 Function Name: INT_D0FIFO1_COMPLETE
 Description:   USB_D0FIFO1 (USB OUT  - host to device) end of transfer
 interrupt
 Parameters:    none
 Return value:  none
 ******************************************************************************/
static void INT_D0FIFO1_COMPLETE (uint32_t dummy)
{
    (void) dummy;

    dmaCompleteUsbCh0Out();
}
/******************************************************************************
 End of function  INT_D0FIFO1_COMPLETE
 *****************************************************************************/

/******************************************************************************
 Function Name: INT_D1FIFO1_COMPLETE
 Description:   USB_D1FIFO1 (USB IN  - device to host) end of transfer
 interrupt
 Parameters:    none
 Return value:  none
 *****************************************************************************/
static void INT_D1FIFO1_COMPLETE (uint32_t dummy)
{
    UNUSED_PARAM(dummy);
    dmaCompleteUsbCh1In();
}
/******************************************************************************
 End of function  INT_D1FIFO1_COMPLETE
 *****************************************************************************/

/******************************************************************************
 * Function Name: dmaFreeVectD0FIFO0
 * Description  : Function to free the D0FIFO0 vector
 * Arguments    : none
 * Return Value : none
 *****************************************************************************/
static void dmaFreeVectD0FIFO0 (void)
{
    /* Do Nothing */
}
/******************************************************************************
 End of function  dmaFreeVectD0FIFO0
 *****************************************************************************/

/******************************************************************************
 * Function Name: dmaFreeVectD0FIFO1
 * Description  : Function to free the D0FIFO1 vector
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
static void dmaFreeVectD0FIFO1 (void)
{
    /* Do Nothing */
}
/******************************************************************************
 End of function  dmaFreeVectD0FIFO1
 *****************************************************************************/

/******************************************************************************
 * Function Name: dmaFreeVectD1FIFO0
 * Description  : Function to free the D1FIFO0 vector
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
static void dmaFreeVectD1FIFO0 (void)
{
    /* Do Nothing */
}
/******************************************************************************
 End of function dmaFreeVectD1FIFO0
 *****************************************************************************/

/******************************************************************************
 * Function Name: dmaFreeVectD1FIFO1
 * Description  : Function to free the D1FIFO1 vector
 * Arguments    : none
 * Return Value : none
 *****************************************************************************/
static void dmaFreeVectD1FIFO1 (void)
{
    /* Do Nothing */
}
/******************************************************************************
 End of function dmaFreeVectD1FIFO1
 *****************************************************************************/

/******************************************************************************
 End of file
 *****************************************************************************/
