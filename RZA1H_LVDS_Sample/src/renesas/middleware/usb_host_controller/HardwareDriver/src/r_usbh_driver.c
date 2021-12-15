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
 * File Name    : r_usbh_driver.c
 * Version      : 1.03
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI
 * OS           : None
 * H/W Platform : RSK+
 * Description  : USB Host device driver for 597 type IP
 *******************************************************************************
 * History      : DD.MM.YYYY Version Description
 *              : 04.02.2010 1.00    First Release
 *              : 10.06.2010 1.01    Updated type definitions
 *              : 14.12.2010 1.02    Added wait for FIFO function & corrected
 *                                   some register accesses.
 *              : 18.02.2016 1.03    Hub with control pipe, sometimes stalls
 *                                   awaiting access - added BCLR reset.
 *              : 08.08.2017 1.20 Updated to 32-bit register access style
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
#include "r_usbh_driver.h"
#include "iodefine_cfg.h"
#include "Trace.h"

#include "usb_iobitmask.h"
#include "rza_io_regrw.h"             /*Register Access */


/******************************************************************************
 Constant macro definitions
 ******************************************************************************/
#include "Trace.h"

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/* DMA only available on pipes 1, 2, 3, 4 and 5 - Pipes 6 - 9 the DMA can
 only fill one packet (Interrupt transfers) and is not supported by this
 driver. */
#define USBH_NUM_DMA_ENABLED_PIPES  5

/* FIFO Configuration */
#define USB_PIPEBUF_BUFSIZE_FUNC(x)      ((uint16_t)(((x) / 64) - 1) << 10)
#define USB_PIPEBUF_BUFNMB_FUNC(x)       ((uint16_t)((x) & 0xFF))

#define USB_FIFO_CONFIG_SIZE        (sizeof(gcFifoConfig) /\
                                     sizeof(struct _FIFOCFG))
/* CFIFOSEL register */
#define USB_CFIFOSEL_MBW_FUNC(x)         ((uint16_t)(((x) & 0x03) << 10))
#define USB_CFIFOSEL_CURPIPE_FUNC(x)     ((uint16_t)((uint16_t)x & 0x0F))

/* CFIFOCTR register */
#define USB_CFIFOCTR_DTLN_FUNC(x)        ((uint16_t)((x) & 0x0FFF))

/* D0FIFOSEL register */
#define USB_D0FIFOSEL_MBW(x)        ((uint16_t)(((x) & 0x03) << 10))
#define USB_D0FIFOSEL_CURPIPE(x)    ((uint16_t)((uint16_t)x & 0x0F))

/* D0FIFOCTR register */
#define USB_D0FIFOCTR_DTLN          ((uint16_t)((x) & 0x0FFF))

/* D1FIFOSEL register */
#define USB_D1FIFOSEL_MBW(x)        ((uint16_t)(((x) & 0x03) << 10))
#define USB_D1FIFOSEL_CURPIPE(x)    ((uint16_t)((uint16_t)x & 0x0F))

/* D1FIFOCTR register */
#define USB_D1FIFOCTR_DTLN          ((uint16_t)((x) & 0x0FFF))

/******************************************************************************
 Function  macro definitions
 ******************************************************************************/

#define USB_DEVADD(p,n)             ((PDEVADDR)(((uint8_t*)p) + gpstDevAddr[n]))
#define USB_PIPECTR(p,n)            ((PPIPECTR)(((uint8_t*)p) + gpstPipeCtr[n]))
#define USB_PIPETRE(p,n)            ((PPIPETRE)(((uint8_t*)p) + gpstPipeTre[n]))
#define USB_PIPETRN(p,n)            ((PIPENTRN)(((uint8_t*)p) + gpstPipeTrn[n]))

/* Modification of the PIPESEL, CFIFOSEL, D0FIFOSEL and D1FIFOSEL registers
 causes the function of other registers to change.
 Read back from the register to make sure that out of order execution
 does not mean other registers are accessed before this one is set. */

#define USB_PIPESEL(p,n)            do {rza_io_reg_write_16(&(p)->PIPESEL, (uint16_t) (n), USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);}\
                                    while (rza_io_reg_read_16(&(p)->PIPESEL, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL) != (n))

#define USB_CFIFOSEL(p,n)           (r_usbh_cfiosel((p), (n)))

#define USB_D0FIFOSEL(p,n)          do {rza_io_reg_write_16(&(p)->D0FIFOSEL, (n), NO_SHIFT, GENERIC_16B_MASK);}\
                                    while (rza_io_reg_read_16(&(p)->D0FIFOSEL, NO_SHIFT, GENERIC_16B_MASK) != (n))

#define USB_D1FIFOSEL(p,n)          do {rza_io_reg_write_16(&(p)->D1FIFOSEL, (n), NO_SHIFT, GENERIC_16B_MASK);}\
                                    while (rza_io_reg_read_16(&(p)->D1FIFOSEL, NO_SHIFT, GENERIC_16B_MASK) != (n))

/******************************************************************************
 Typedef definitions
 ******************************************************************************/

/* Define the structure of the Device Address Configuration Register */
typedef volatile union _DEVADDR
{
    uint16_t                                                                                                                                                                    WORD;
    struct
    {
        uint16_t RTPOR :1;
        uint16_t :5;
        uint16_t USBSPD :2;
        uint16_t HUBPORT :3;
        uint16_t UPPHUB :4;
        uint16_t :1;
    } BIT;
} DEVADDR, *PDEVADDR;

/* Define a structure of pointers to the pipe control registers */
typedef volatile struct _PIPEREG
{
    /* Pipe control registers 1..9 */
    volatile union _PIPENCTR
    {
        uint16_t                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     WORD;
        struct
        {
            uint16_t PID :2;
            uint16_t :3;
            uint16_t PBUSY :1;
            uint16_t SQMON :1;
            uint16_t SQSET :1;
            uint16_t SQCLR :1;
            uint16_t ACLRM :1;
            uint16_t ATREPM :1; /* Not applicable to pipes 6..9 */
            uint16_t :1;
            uint16_t CSSTS :1;
            uint16_t CSCLR :1;
            uint16_t INBUFM :1; /* Not applicable to pipes 6..9 */
            uint16_t BSTS :1;
        } BIT;
    }*PPIPECTR;

    /* Pipe transaction counter enable registers 1..5 */
    volatile union _PIPENTRE
    {
        uint16_t WORD;
        struct
        {
            uint16_t :8;
            uint16_t TRCLR :1;
            uint16_t TRENB :1;
            uint16_t :6;
        } BIT;
    }*PPIPETRE;

    /* Pipe transaction counter register 1..5 */
    volatile uint16_t                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         *PIPENTRN;
} PIPEREG, *PPIPEREG;

/* The structure of the pipe control registers */
typedef volatile union
{
    uint16_t WORD;
    struct
    {
        uint16_t PID :2;
        uint16_t :3;
        uint16_t PBUSY :1;
        uint16_t SQMON :1;
        uint16_t SQSET :1;
        uint16_t SQCLR :1;
        uint16_t ACLRM :1;
        uint16_t ATREPM :1; /* Not applicable to pipes 6..9 */
        uint16_t :1;
        uint16_t CSSTS :1;
        uint16_t CSCLR :1;
        uint16_t INBUFM :1; /* Not applicable to pipes 6..9 */
        uint16_t BSTS :1;

    } BIT;
}*PPIPECTR;

/* Define the structure of the DMA transaction control register */
typedef volatile union
{
    uint16_t WORD;
    struct
    {
        uint16_t :8;
        uint16_t TRCLR :1;
        uint16_t TRENB :1;
        uint16_t :6;

    } BIT;
}*PPIPETRE;

/* The structure of the DMA transaction counter register */
typedef volatile uint16_t *PIPENTRN;

/******************************************************************************
 Imported global variables and functions (from other files)
 ******************************************************************************/

/******************************************************************************
 Exported global variables and functions (to be accessed by other files)
 ******************************************************************************/

/******************************************************************************
 Private Constant Data
 ******************************************************************************/
#if USBH_HIGH_SPEED_SUPPORT == 1
/* Define the the FIFO buffer configuration */
static const struct _FIFOCFG
{
    /* FIFO buffer size and number settings */
    uint16_t wPipeBuf;

}

gcFifoConfig[] =
{
    /* Pipe 0 (not in table) always uses buffer number 0..3 */
    /* Pipes 6..9 have fixed FIFO locations taking 4,5,6 & 7 */
    /* Therefore start buffer allocation at 8. In this case
     1kByte of FIFO has been assigned to pipes 1 to 5 and the
     buffer size set to 512. This means they can all operate as
     double buffered high speed bulk endpoints */
    /* Pipe 1 - ISOC & BULK */

	/* Buffer Size */
    {USB_PIPEBUF_BUFSIZE_FUNC(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB_FUNC(8)},

	/* Pipe 2 - ISOC & BULK */
    /* Buffer Size */
    {USB_PIPEBUF_BUFSIZE_FUNC(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB_FUNC(24)},

	/* Pipe 3 - BULK */
    /* Buffer Size */
    {USB_PIPEBUF_BUFSIZE_FUNC(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB_FUNC(40)},

	/* Pipe 4 - BULK */
    /* Buffer Size */
    {USB_PIPEBUF_BUFSIZE_FUNC(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB_FUNC(56)},

	/* Pipe 5 - BULK */
    /* Buffer Size */
    {USB_PIPEBUF_BUFSIZE_FUNC(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB_FUNC(72)},

	/* Pipe 6 - INTERRUPT - Fixed to buffer 4 */
    /* Buffer Size */
    {USB_PIPEBUF_BUFSIZE_FUNC(64) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB_FUNC(4)},

	/* Pipe 7 - INTERRUPT - Fixed to buffer 5 */
    /* Buffer Size */
    {USB_PIPEBUF_BUFSIZE_FUNC(64) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB_FUNC(5)},

	/* Pipe 8 - INTERRUPT - Fixed to buffer 6 */
    /* Buffer Size */
    {USB_PIPEBUF_BUFSIZE_FUNC(64) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB_FUNC(6)},

	/* Pipe 9 - INTERRUPT - Fixed to buffer 7 */
    /* Buffer Size */
    {USB_PIPEBUF_BUFSIZE_FUNC(64) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB_FUNC(7)},
};
#endif

/* Table of offsets to the device address registers */
#if USBH_HIGH_SPEED_SUPPORT == 1
static const size_t gpstDevAddr[] =
{ (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD0,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD1,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD2,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD3,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD4,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD5,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD6,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD7,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD8,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADD9,
        (size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DEVADDA };

#else
    #error "Support for the Full Speed USB Peripheral is not included."
#endif

static const size_t gpstPipeCtr[] =
{
/* Pipe 0 is not valid */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).DCPCTR,

/* Pipe 1 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE1CTR,

/* Pipe 2 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE2CTR,

/* Pipe 3 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE3CTR,

/* Pipe 4 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE4CTR,

/* Pipe 5 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE5CTR,

/* Pipe 6 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE6CTR,

/* Pipe 7 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE7CTR,

/* Pipe 8 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE8CTR,

/* Pipe 9 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE9CTR };

static const size_t gpstPipeTre[] =
{
/* Pipe 0 - Control pipe does not have a transaction counter */
0UL,

/* Pipe 1 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE1TRE,

/* Pipe 2 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE2TRE,

/* Pipe 3 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE3TRE,

/* Pipe 4 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE4TRE,

/* Pipe 5 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE5TRE,

/* Pipe 6 - Pipes for interrupt transfer don't have transaction counters */
0UL,

/* Pipe 7 */
0UL,

/* Pipe 8 */
0UL,

/* Pipe 9 */
0UL, };

static const size_t gpstPipeTrn[] =
{
/* Pipe 0 - Control pipe does not have a transaction counter */
0UL,

/* Pipe 1 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE1TRN,

/* Pipe 2 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE2TRN,

/* Pipe 3 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE3TRN,

/* Pipe 4 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE4TRN,

/* Pipe 5 */
(size_t) &(*(struct USBH_HOST_STRUCT_TAG*) 0).PIPE5TRN,

/* Pipe 6 - Pipes for interrupt transfer don't have transaction counters */
0UL,

/* Pipe 7 */
0UL,

/* Pipe 8 */
0UL,

/* Pipe 9 */
0UL };

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

#if USBH_HIGH_SPEED_SUPPORT == 1
static void r_usbh_config_fifo (PUSB pUSB);
#endif
static int r_usbh_wait_fifo (PUSB pUSB, int iPipeNumber, int iCountOut);
static void r_usbh_init_pipes (PUSB pUSB);
static void r_usbh_write_fifo (PUSB pUSB, void *pvSrc, size_t stLength);
static void r_usbh_read_fifo (PUSB pUSB, void *pvDest, size_t stLength);
static void r_usbh_cfiosel (PUSB pUSB, uint16_t usCFIFOSEL);

/******************************************************************************
 Renesas Abstracted Host Driver API functions
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_Initialise
 Description:   Function to initialise the USB peripheral for operation as the
 Host Controller
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 Return value:  none
 ******************************************************************************/
void R_USBH_Initialise (PUSB pUSB)
{

    uint8_t usb0_prev_suspended;
    uint8_t usb1_prev_suspended;

#if USBH_HIGH_SPEED_SUPPORT == 1

    /* UPLLE Bit is required to be set but only
     * available in USB0 ctrl reg set.
     *
     * Check if its set, if not place USB1 and USB0 in to suspend
     * mode, set UPLLE and recover from suspend mode
     */

    if (0 == rza_io_reg_read_16(pUSB->SYSCFG0, USB_SYSCFG_UPLLE_SHIFT,  USB_SYSCFG_UPLLE))
    {
        /* Store suspend status, so it can be recovered later */
        usb0_prev_suspended = (uint8_t) rza_io_reg_read_16(&SUSPMODE_0, USB_SUSPMODE_SUSPM_SHIFT,  USB_SUSPMODE_SUSPM);
        usb1_prev_suspended = (uint8_t) rza_io_reg_read_16(&SUSPMODE_1, USB_SUSPMODE_SUSPM_SHIFT,  USB_SUSPMODE_SUSPM);

        rza_io_reg_write_16(&SUSPMODE_0, 0x0, USB_SUSPMODE_SUSPM_SHIFT,  USB_SUSPMODE_SUSPM);

        rza_io_reg_write_16(&SUSPMODE_1, 0x0, USB_SUSPMODE_SUSPM_SHIFT,  USB_SUSPMODE_SUSPM);


        rza_io_reg_write_16(&SYSCFG0_0, 0x1, USB_SYSCFG_UPLLE_SHIFT,  USB_SYSCFG_UPLLE);

        /* Recover USB0 suspend mode */
        if (0 != usb0_prev_suspended)
        {
            rza_io_reg_write_16(&SUSPMODE_0, 0x1, USB_SUSPMODE_SUSPM_SHIFT,  USB_SUSPMODE_SUSPM);
        }

        /* Recover USB1 suspend mode */
        if (0 != usb1_prev_suspended)
        {
            rza_io_reg_write_16(&SUSPMODE_1, 0x1, USB_SUSPMODE_SUSPM_SHIFT,  USB_SUSPMODE_SUSPM);
        }
    }


    /* Enable host controller function */
    rza_io_reg_write_16(&pUSB->SYSCFG0, (USB_SYSCFG_HSE | USB_SYSCFG_DCFM | USB_SYSCFG_DRPD | USB_SYSCFG_USBE),
                        NO_SHIFT, (USB_SYSCFG_HSE | USB_SYSCFG_DCFM | USB_SYSCFG_DRPD | USB_SYSCFG_USBE));
    

    /* Exit suspend mode */
    rza_io_reg_write_16(&pUSB->SUSPMODE, 0x1, USB_SUSPMODE_SUSPM_SHIFT, USB_SUSPMODE_SUSPM);

    /* Configure the pipe FIFO buffer assignment */
    r_usbh_config_fifo(pUSB);

    /* Initialise the pipes */
    r_usbh_init_pipes(pUSB);

    /* Enable the interrupts */
    rza_io_reg_write_16(&pUSB->INTENB0, (USB_INTENB0_SOFE | USB_INTENB0_BEMPE | USB_INTENB0_BRDYE),
                        NO_SHIFT, (USB_INTENB0_SOFE | USB_INTENB0_BEMPE | USB_INTENB0_BRDYE));

    rza_io_reg_write_16(&pUSB->INTENB1, 0x0, NO_SHIFT, GENERIC_16B_MASK);

#else
    #error "Support for the Full Speed USB Peripheral is not included."
#endif
}
/******************************************************************************
 End of function  R_USBH_Initialise
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_Uninitialise
 Description:   Function to uninitialise the USB peripheral for operation as the
 host controller
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 Return value:  none
 ******************************************************************************/
void R_USBH_Uninitialise (PUSB pUSB)
{

#if USBH_HIGH_SPEED_SUPPORT == 1
    /* Disable host controller function */
    rza_io_reg_write_16(&pUSB->SYSCFG0, 0x0, NO_SHIFT,
                        (USB_SYSCFG_HSE | USB_SYSCFG_DCFM | USB_SYSCFG_DRPD | USB_SYSCFG_USBE));

    rza_io_reg_write_16(&pUSB->SYSCFG0, 0x0, NO_SHIFT, (USB_SYSCFG_HSE | USB_SYSCFG_DRPD));


#else
    #error "Support for the Full Speed USB Peripheral is not included."
#endif
    /* Disable the interrupts */
    rza_io_reg_write_16(&pUSB->INTENB0, 0x0, NO_SHIFT, GENERIC_16B_MASK);
    rza_io_reg_write_16(&pUSB->INTENB1, 0x0, NO_SHIFT, GENERIC_16B_MASK);
}
/******************************************************************************
 End of function  R_USBH_Uninitialise
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_ConfigurePipe
 Description:   Function to configure the pipe for the endpoint's transfer
 characteristics
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The pipe number to configure
 IN  transferType - The type of transfer
 IN  transferDirection - The direction of transfer
 IN  byDeviceAddress - The address of the device
 IN  byEndpointNumber - The endpoint address with the direction
 bit masked
 IN  byInterval - The endpoint poll interval for Isochronus and
 interrupt transfers only
 IN  wPacketSize - The endpoint packet size
 Return value:  The pipe number or -1 on error
 ******************************************************************************/
int R_USBH_ConfigurePipe (PUSB pUSB, int iPipeNumber, USBTT transferType, USBDIR transferDirection,
        uint8_t byDeviceAddress, uint8_t byEndpointNumber, uint8_t byInterval, uint16_t wPacketSize)
{
    /* This function is not for the control pipe */
    if (iPipeNumber <= 0)
    {
        return -1;
    }

    /* Set pipe PID to NAK before setting the configuration */
    USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;

    /* Select the pipe */
    USB_PIPESEL(pUSB, iPipeNumber);

    /* Set the transfer direction */
    if (transferDirection == USBH_OUT)
    {
        rza_io_reg_write_16(&pUSB->PIPECFG, 0x1, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR);

        /* Switch double buffering on for bulk OUT transfers */
#if USBH_HIGH_SPEED_SUPPORT == 1
        /*
         ** POSSIBLE BUG IN FULL SPEED VARIANT OF 597 IP **
         There is a possible bug in the Full speed variant of the 597 IP.
         When double buffering is enabled and a data of length equal to the
         value set in the PIPEMAXP MXPS field is written to the FIFO
         spurious data is transmitted. Please check errata's and ECN & remove
         conditional compile when fixed.
         */
        if (transferType == USBH_BULK)
        {
            rza_io_reg_write_16(&pUSB->PIPECFG, 0x1, USB_PIPECFG_DBLB_SHIFT, USB_PIPECFG_DBLB);
        }
#endif
    }
    else if (transferDirection == USBH_IN)
    {
        /* Clear the pipe FIFO in preparation for transfer */
        rza_io_reg_write_16(&pUSB->CFIFOCTR, 0x1, USB_CFIFOCTR_BCLR_SHIFT, USB_CFIFOCTR_BCLR);

        /* Set the transfer direction */
        rza_io_reg_write_16(&pUSB->PIPECFG, 0x0, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR);

        /* Check for an isochronus transfer */
        if (transferType == USBH_ISOCHRONOUS)
        {
            /* Set the Isochronus in buffer flush bit */
            rza_io_reg_write_16(&pUSB->PIPEPERI, 0x1, USB_PIPEPERI_IFIS_SHIFT, USB_PIPEPERI_IFIS);
        }
    }
    else
    {
        /* This function does not work for control functions */
        return -1;
    }

    /* Set the transfer type */
    switch (transferType)
    {
        case USBH_ISOCHRONOUS :
            rza_io_reg_write_16(&pUSB->PIPECFG, 0x3, USB_PIPECFG_TYPE_SHIFT, USB_PIPECFG_TYPE);

            /* Set the isoc schedule interval */
            rza_io_reg_write_16(&pUSB->PIPEPERI, ((uint16_t) (byInterval - 1) & USB_PIPEPERI_IITV), USB_PIPEPERI_IITV_SHIFT, USB_PIPEPERI_IITV);

            /* Set the packet size */
            rza_io_reg_write_16(&pUSB->PIPEMAXP, ((wPacketSize) & USB_PIPEMAXP_MXPS), USB_PIPEMAXP_MXPS_SHIFT, USB_PIPEMAXP_MXPS);
        break;
        case USBH_BULK :
            rza_io_reg_write_16(&pUSB->PIPEPERI, 0x0, NO_SHIFT, GENERIC_16B_MASK);
            rza_io_reg_write_16(&pUSB->PIPECFG, 0x1, USB_PIPECFG_TYPE_SHIFT, USB_PIPECFG_TYPE);

            /* Set the packet size */
            rza_io_reg_write_16(&pUSB->PIPEMAXP, (wPacketSize & USB_PIPEMAXP_MXPS), USB_PIPEMAXP_MXPS_SHIFT, USB_PIPEMAXP_MXPS);
        break;
        case USBH_INTERRUPT :
            rza_io_reg_write_16(&pUSB->PIPECFG, 0x2, USB_PIPECFG_TYPE_SHIFT, USB_PIPECFG_TYPE);

            /* Set the interrupt schedule interval */
            rza_io_reg_write_16(&pUSB->PIPEPERI, ((uint16_t) (byInterval - 1) & USB_PIPEPERI_IITV), USB_PIPEPERI_IITV_SHIFT, USB_PIPEPERI_IITV);

            /* Set the packet size */
            rza_io_reg_write_16(&pUSB->PIPEMAXP, (wPacketSize & USB_PIPEMAXP_MXPS), USB_PIPEMAXP_MXPS_SHIFT, USB_PIPEMAXP_MXPS);
        break;
        default :

            /* This function does not work for control functions */
            return -1;
    }

    /* Set the endpoint number */
    rza_io_reg_write_16(&pUSB->PIPECFG, (byEndpointNumber & USB_PIPECFG_EPNUM), USB_PIPECFG_EPNUM_SHIFT, USB_PIPECFG_EPNUM);

    /* Set the device address */
    rza_io_reg_write_16(&pUSB->PIPEMAXP, byDeviceAddress,USB_PIPEMAXP_DEVSEL_SHIFT, USB_PIPEMAXP_DEVSEL);
    return iPipeNumber;
}
/******************************************************************************
 End of function  R_USBH_ConfigurePipe
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_UnconfigurePipe
 Description:   Function to unconfigure a pipe so it can be used for another
 transfer
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe to unconfigure
 Return value:  none
 ******************************************************************************/
void R_USBH_UnconfigurePipe (PUSB pUSB, int iPipeNumber)
{

    if (iPipeNumber > 0)
    {
        /* Disable the pipe */
        USB_PIPECTR(pUSB, iPipeNumber)->WORD = 0;

        /* Clear the FIFO */
        R_USBH_ClearPipeFifo(pUSB, iPipeNumber);

        /* Select the pipe */
        USB_PIPESEL(pUSB, iPipeNumber);

        /* Clear the settings */
        rza_io_reg_write_16(&pUSB->PIPECFG, 0x0, NO_SHIFT, GENERIC_16B_MASK);
        rza_io_reg_write_16(&pUSB->PIPEPERI, 0x0, NO_SHIFT, GENERIC_16B_MASK);
        rza_io_reg_write_16(&pUSB->PIPEMAXP, 0x0, NO_SHIFT, GENERIC_16B_MASK);
    }
}
/******************************************************************************
 End of function  R_USBH_UnconfigurePipe
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_ClearPipeFifo
 Description:   Function to clear the pipe FIFO
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe to clear
 Return value:  none
 ******************************************************************************/
void R_USBH_ClearPipeFifo (PUSB pUSB, int iPipeNumber)
{

    if (iPipeNumber > 0)
    {
        /* If the pipe is active */
        if (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID)
        {
            int iCountOut = 1000;

            /* Make sure that it is not busy */
            while (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PBUSY)
            {
                if (iCountOut)
                {
                    iCountOut--;
                }
                else
                {
                    break;
                }
            }
        }
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
    }
    else if (iPipeNumber == 0)
    {
        /* No express method for the DCP, select DCP */
#if USBH_FIFO_BIT_WIDTH == 32
        USB_CFIFOSEL(pUSB, USB_CFIFOSEL_MBW_FUNC(2));
#else
        USB_CFIFOSEL(pUSB, USB_CFIFOSEL_MBW_FUNC(1));
#endif
        /* Set the buffer clear bit */
        rza_io_reg_write_16(&pUSB->CFIFOCTR, 0x1, USB_CFIFOCTR_BCLR_SHIFT, USB_CFIFOCTR_BCLR);
    }
}
/******************************************************************************
 End of function  R_USBH_ClearPipeFifo
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_SetPipePID
 Description:   Function to set the pipe PID
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe
 IN  dataPID - The Data PID setting
 Return value:  none
 ******************************************************************************/
void R_USBH_SetPipePID (PUSB pUSB, int iPipeNumber, USBDP dataPID)
{
    if (iPipeNumber > 0)
    {
        /* Set the pipe to NAK */
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
        /* Set the endpoint PID */
        if (dataPID == USBH_DATA1)
        {
            TRACE(("R_USBH_SetPipePID: Pipe %u (1)\r\n", iPipeNumber));
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.SQSET = 1;
        }
        else
        {
            TRACE(("R_USBH_SetPipePID: Pipe %u (0)\r\n", iPipeNumber));
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.SQCLR = 1;
        }
    }
    else if (iPipeNumber == 0)
    {
        /* Disable the control pipe */
        rza_io_reg_write_16(&pUSB->DCPCTR, 0x0,USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);

        /* Set the data PID */
        if (dataPID == USBH_DATA1)
        {
            rza_io_reg_write_16(&pUSB->DCPCTR, 0x1, USB_DCPCTR_SQSET_SHIFT, USB_DCPCTR_SQSET);
        }
        else
        {
            rza_io_reg_write_16(&pUSB->DCPCTR, 0x1, USB_DCPCTR_SQCLR_SHIFT, USB_DCPCTR_SQCLR);
        }
    }
}
/******************************************************************************
 End of function  R_USBH_SetPipePID
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_GetPipePID
 Description:   Function to get the pipe PID
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe
 Return value:  The state of the data PID
 ******************************************************************************/
USBDP R_USBH_GetPipePID (PUSB pUSB, int iPipeNumber)
{

    if (iPipeNumber > 0)
    {
        if (USB_PIPECTR(pUSB, iPipeNumber)->BIT.SQMON)
        {
            TRACE(("R_USBH_GetPipePID: Pipe %u SQMON = 1\r\n",
                            iPipeNumber));

            return USBH_DATA1;
        }
        else
        {
            TRACE(("R_USBH_GetPipePID: Pipe %u SQMON = 0\r\n",
                            iPipeNumber));

            return USBH_DATA0;
        }
    }
    else if (iPipeNumber == 0)
    {
        if (rza_io_reg_read_16(&pUSB->DCPCTR, USB_DCPCTR_SQMON_SHIFT, USB_DCPCTR_SQMON))
        {
            return USBH_DATA1;
        }
        else
        {
            return USBH_DATA0;
        }
    }
    return USBH_DATA0;
}
/******************************************************************************
 End of function  R_USBH_GetPipePID
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_GetPipeStall
 Description:   Function to get the pipe stall state
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe
 Return value:  true if the pipe has received a stall token
 ******************************************************************************/
_Bool R_USBH_GetPipeStall (PUSB pUSB, int iPipeNumber)
{

    if (iPipeNumber > 0)
    {
        if (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID > 1)
        {
            return true;
        }
    }
    else if (iPipeNumber == 0)
    {
        if (rza_io_reg_read_16(&pUSB->DCPCTR, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID) > 1)
        {
            return true;
        }
    }
    return false;
}
/******************************************************************************
 End of function  R_USBH_GetPipeStall
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_EnablePipe
 Description:   Function to Enable (ACK) or Disable (NAK) the pipe
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe
 IN  bfEnable - True to enable transfers on the pipe
 Return value:  none
 ******************************************************************************/
void R_USBH_EnablePipe (PUSB pUSB, int iPipeNumber, _Bool bfEnable)
{

    if (bfEnable)
    {
        /* Enable the pipe */
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 1;
    }
    else
    {
        /* Disable the pipe */
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;

    }
}
/******************************************************************************
 End of function  R_USBH_EnablePipe
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_SetPipeInterrupt
 Description:   Function to set or clear the desired pipe interrupts
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe
 IN  buffIntType - The interrupt type
 Return value:  none
 ******************************************************************************/
void R_USBH_SetPipeInterrupt (PUSB pUSB, int iPipeNumber, USBIP buffIntType)
{
    uint16_t wISet, wIClear;
    if (iPipeNumber >= 0)
    {
        wISet = (uint16_t) (1 << iPipeNumber);
    }
    else
    {
        wISet = 0x3FF;
    }
    wIClear = (uint16_t) ~wISet;

    /* Pipe buffer empty interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_EMPTY)
    {
        if (buffIntType & USBH_PIPE_INT_DISABLE)
        {
            rza_io_reg_write_16(&pUSB->BEMPENB, 0x0, NO_SHIFT, (uint16_t)~wIClear);
        }
        else
        {
            rza_io_reg_write_16(&pUSB->BEMPENB, wISet, NO_SHIFT, (uint16_t)~GENERIC_16B_MASK);
        }
    }

    /* Pipe buffer ready interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_READY)
    {
        if (buffIntType & USBH_PIPE_INT_DISABLE)
        {
            rza_io_reg_write_16(&pUSB->BRDYENB, 0x0, NO_SHIFT, (uint16_t)~wIClear);
        }
        else
        {
            rza_io_reg_write_16(&pUSB->BRDYENB, wISet, NO_SHIFT, (uint16_t)~GENERIC_16B_MASK);
        }
    }

    /* Pipe buffer not ready interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_NOT_READY)
    {
        if (buffIntType & USBH_PIPE_INT_DISABLE)
        {
            rza_io_reg_write_16(&pUSB->NRDYENB, 0x0, NO_SHIFT, (uint16_t)~wIClear);
        }
        else
        {
            rza_io_reg_write_16(&pUSB->NRDYENB, wISet, NO_SHIFT, (uint16_t)~GENERIC_16B_MASK);
        }
    }
}
/******************************************************************************
 End of function  R_USBH_SetPipeInterrupt
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_ClearPipeInterrupt
 Description:   Function to clear the desired pipe interrupt status flag
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe
 IN  buffIntType - The interrupt type
 Return value:  none
 ******************************************************************************/
void R_USBH_ClearPipeInterrupt (PUSB pUSB, int iPipeNumber, USBIP buffIntType)
{

    uint16_t wIClear = (uint16_t) ~((uint16_t) (1 << iPipeNumber));

    /* Pipe buffer empty interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_EMPTY)
    {
        rza_io_reg_write_16(&pUSB->BEMPSTS, wIClear, NO_SHIFT, GENERIC_16B_MASK);
    }

    /* Pipe buffer ready interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_READY)
    {
        rza_io_reg_write_16(&pUSB->BRDYSTS, wIClear, NO_SHIFT, GENERIC_16B_MASK);
    }

    /* Pipe buffer not ready interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_NOT_READY)
    {
        rza_io_reg_write_16(&pUSB->NRDYSTS, wIClear, NO_SHIFT, GENERIC_16B_MASK);
    }
}
/******************************************************************************
 End of function  R_USBH_ClearPipeInterrupt
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_GetPipeInterrupt
 Description:   Function to get the state of the pipe interrupt status flag
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe or USBH_PIPE_NUMBER_ANY
 IN  buffIntType - The interrupt type
 IN  bfQualify - Set true to qualify with enabled interrupts
 Return value:  true if the flag is set.
 ******************************************************************************/
_Bool R_USBH_GetPipeInterrupt (PUSB pUSB, int iPipeNumber, USBIP buffIntType, _Bool bfQualify)
{
    if (iPipeNumber >= 0)
    {
        uint16_t wIMask = (uint16_t) (1 << iPipeNumber);
        if (bfQualify)
        {
            switch (buffIntType)
            {
                case USBH_PIPE_BUFFER_EMPTY :
                    if (wIMask & rza_io_reg_read_16(&pUSB->BEMPSTS, NO_SHIFT, GENERIC_16B_MASK) &
                        rza_io_reg_read_16(&pUSB->BEMPENB, NO_SHIFT, GENERIC_16B_MASK) )
                    {
                        return true;
                    }
                break;
                case USBH_PIPE_BUFFER_READY :
                    if (wIMask & rza_io_reg_read_16(&pUSB->BRDYSTS, NO_SHIFT, GENERIC_16B_MASK) &
                        rza_io_reg_read_16(&pUSB->BRDYENB, NO_SHIFT, GENERIC_16B_MASK) )
                    {
                        return true;
                    }
                break;
                case USBH_PIPE_BUFFER_NOT_READY :
                    if (wIMask & rza_io_reg_read_16(&pUSB->NRDYSTS, NO_SHIFT, GENERIC_16B_MASK) &
                        rza_io_reg_read_16(&pUSB->NRDYENB, NO_SHIFT, GENERIC_16B_MASK) )
                    {
                        return true;
                    }
                break;

                case USBH_PIPE_INT_DISABLE:
				{
                	/* do nothing */
                	break;
				}
                case USBH_PIPE_INT_ENABLE:
				{
                	/* do nothing */
                	break;
				}
            }
            return false;
        }
        else
        {
            switch (buffIntType)
            {
                case USBH_PIPE_BUFFER_EMPTY :
                    if (wIMask & rza_io_reg_read_16(&pUSB->BEMPSTS, NO_SHIFT, GENERIC_16B_MASK))
                    {
                        return true;
                    }
                break;
                case USBH_PIPE_BUFFER_READY :
                    if (wIMask & rza_io_reg_read_16(&pUSB->BRDYSTS, NO_SHIFT, GENERIC_16B_MASK))
                    {
                        return true;
                    }
                break;
                case USBH_PIPE_BUFFER_NOT_READY :
                    if (wIMask & rza_io_reg_read_16(&pUSB->NRDYSTS, NO_SHIFT, GENERIC_16B_MASK))
                    {
                        return true;
                    }
                break;

                case USBH_PIPE_INT_DISABLE:
				{
                	/* do nothing */
                	break;
				}
                case USBH_PIPE_INT_ENABLE:
				{
                	/* do nothing */
                	break;
				}
            }
            return false;
        }
    }
    else
    {
        if (bfQualify)
        {
            switch (buffIntType)
            {
                case USBH_PIPE_BUFFER_EMPTY :
                    if (rza_io_reg_read_16(&pUSB->BEMPSTS, NO_SHIFT, GENERIC_16B_MASK) &
                        rza_io_reg_read_16(&pUSB->BEMPENB, NO_SHIFT, GENERIC_16B_MASK) )
                    {
                        return true;
                    }
                break;
                case USBH_PIPE_BUFFER_READY :
                    if (rza_io_reg_read_16(&pUSB->BRDYSTS, NO_SHIFT, GENERIC_16B_MASK) &
                        rza_io_reg_read_16(&pUSB->BRDYENB, NO_SHIFT, GENERIC_16B_MASK) )
                    {
                        return true;
                    }
                break;
                case USBH_PIPE_BUFFER_NOT_READY :
                    if (rza_io_reg_read_16(&pUSB->NRDYSTS, NO_SHIFT, GENERIC_16B_MASK) &
                        rza_io_reg_read_16(&pUSB->NRDYENB, NO_SHIFT, GENERIC_16B_MASK) )
                    {
                        return true;
                    }
                break;

                case USBH_PIPE_INT_DISABLE:
				{
                	/* do nothing */
                	break;
				}
                case USBH_PIPE_INT_ENABLE:
				{
                	/* do nothing */
                	break;
				}
            }
            return false;
        }
        else
        {
            switch (buffIntType)
            {
                case USBH_PIPE_BUFFER_EMPTY :
                    if (rza_io_reg_read_16(&pUSB->BEMPSTS, NO_SHIFT, GENERIC_16B_MASK))
                    {
                        return true;
                    }
                break;
                case USBH_PIPE_BUFFER_READY :
                    if (rza_io_reg_read_16(&pUSB->BRDYSTS, NO_SHIFT, GENERIC_16B_MASK))
                    {
                        return true;
                    }
                break;
                case USBH_PIPE_BUFFER_NOT_READY :
                    if (rza_io_reg_read_16(&pUSB->NRDYSTS, NO_SHIFT, GENERIC_16B_MASK))
                    {
                        return true;
                    }
                break;

                case USBH_PIPE_INT_DISABLE:
				{
                	/* do nothing */
                	break;
				}
                case USBH_PIPE_INT_ENABLE:
				{
                	/* do nothing */
                	break;
				}
            }
            return false;
        }
    }
}
/******************************************************************************
 End of function  R_USBH_GetPipeInterrupt
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_InterruptStatus
 Description:   Function to get the interrupt status
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  intStatus - The interrupt status to get
 IN  bfAck - true to acknowledge the status
 Return value:  true if the flag is set.
 ******************************************************************************/
_Bool R_USBH_InterruptStatus (PUSB pUSB, USBIS intStatus, _Bool bfAck)
{
    switch (intStatus)
    {
        case USBH_INTSTS_SOFR :
        {
            if (rza_io_reg_read_16(&pUSB->INTSTS0, USB_INTSTS0_SOFR_SHIFT, USB_INTSTS0_SOFR))
            {
                if (bfAck)
                {
                    rza_io_reg_write_16(&pUSB->INTSTS0, (uint16_t)~USB_INTSTS0_SOFR, NO_SHIFT, GENERIC_16B_MASK);
                }
                return true;
            }
            break;
        }
        case USBH_INTSTS_PIPE_BUFFER_EMPTY :
        {
            if (rza_io_reg_read_16(&pUSB->INTSTS0, USB_INTSTS0_BEMP_SHIFT, USB_INTSTS0_BEMP))
            {
                return true;
            }
            break;
        }
        case USBH_INTSTS_PIPE_BUFFER_READY :
        {
            if (rza_io_reg_read_16(&pUSB->INTSTS0, USB_INTSTS0_BRDY_SHIFT, USB_INTSTS0_BRDY))
            {
                return true;
            }
            break;
        }
        case USBH_INTSTS_PIPE_BUFFER_NOT_READY :
        {
            if (rza_io_reg_read_16(&pUSB->INTSTS0, USB_INTSTS0_NRDY_SHIFT, USB_INTSTS0_NRDY))
            {
                return true;
            }
            break;
        }
        case USBH_INTSTS_SETUP_ERROR :
        {
            if (rza_io_reg_read_16(&pUSB->INTSTS1, USB_INTSTS1_SIGN_SHIFT, USB_INTSTS1_SIGN))
            {
                if (bfAck)
                {
                    rza_io_reg_write_16(&pUSB->INTSTS1, (uint16_t)~USB_INTSTS1_SIGN, NO_SHIFT, GENERIC_16B_MASK);
                }
                return true;
            }
            break;
        }
        case USBH_INTSTS_SETUP_COMPLETE :
        {
            if (rza_io_reg_read_16(&pUSB->INTSTS1, USB_INTSTS1_SACK_SHIFT, USB_INTSTS1_SACK))
            {
                if (bfAck)
                {
                    rza_io_reg_write_16(&pUSB->INTSTS1, (uint16_t)~USB_INTSTS1_SACK, NO_SHIFT, GENERIC_16B_MASK);
                }
                return true;
            }
            break;
        }
    }
    return false;
}
/******************************************************************************
 End of function  R_USBH_InterruptStatus
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_GetFrameNumber
 Description:   Function to get the frame and micro frame numbers
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 OUT pwFrame - Pointer to the destinaton frame number
 OUT pwMicroFrame - Pointer to the destination micro frame number
 Return value:  none
 ******************************************************************************/
void R_USBH_GetFrameNumber (PUSB pUSB, uint16_t *pwFrame, uint16_t *pwMicroFrame)
{

    if (pwFrame)
    {
        *pwFrame = (uint16_t) rza_io_reg_read_16(&pUSB->FRMNUM, USB_FRMNUM_FRNM_SHIFT, USB_FRMNUM_FRNM);
    }
    if (pwMicroFrame)
    {
#if USBH_HIGH_SPEED_SUPPORT == 1
        *pwMicroFrame = (uint16_t) rza_io_reg_read_16(&pUSB->UFRMNUM, USB_UFRMNUM_UFRNM_SHIFT, USB_UFRMNUM_UFRNM);
#else
        *pwMicroFrame = 0;
#endif
    }
}
/******************************************************************************
 End of function  R_USBH_GetFrameNumber
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_WritePipe
 Description:   Function to write to an endpoint pipe FIFO
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The pipe number to write to
 IN  pbySrc - Pointer to the source memory (FIFO size alligned)
 IN  stLength - The length to write
 Return value:  The number of bytes written or -1 on FIFO access error
 ******************************************************************************/
size_t R_USBH_WritePipe (PUSB pUSB, int iPipeNumber, uint8_t *pbySrc, size_t stLength)
{
    size_t stMaxPacket;
    uint16_t usCFIFOSEL;
    int iTrys = 2;

    /* If we are not writing to the DCP */
    if (iPipeNumber > 0)
    {
        /* Select the endpoint FIFO for desired access width */
        usCFIFOSEL = (uint16_t) (
#if USBH_FIFO_BIT_WIDTH == 32
                        USB_CFIFOSEL_MBW_FUNC(2)
#else
                        USB_CFIFOSEL_MBW_FUNC(1)
#endif
#ifdef _BIG
                        | USB_CFIFOSEL_BIG_END
#endif
|        USB_CFIFOSEL_CURPIPE_FUNC(iPipeNumber));
        USB_CFIFOSEL(pUSB, usCFIFOSEL);
    }
    else
    {

        usCFIFOSEL = (uint16_t)(
#if USBH_FIFO_BIT_WIDTH == 32
        (USB_CFIFOSEL_MBW_FUNC(2)
#else
                (USB_CFIFOSEL_MBW_FUNC(1)
#endif
#ifdef _BIG
                        | USB_CFIFOSEL_BIG_END
#endif
                        | USB_CFIFOSEL_ISEL));
        /* For control pipe clear the buffer - this does both SIE and CPU
         side */
        do
        {
            rza_io_reg_write_16(&pUSB->DCPCTR, 0x0, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            rza_io_reg_write_16(&pUSB->CFIFOCTR, 0x1, USB_CFIFOCTR_BCLR_SHIFT, USB_CFIFOCTR_BCLR);
        }while (rza_io_reg_read_16(&pUSB->DCPCTR, USB_DCPCTR_PBUSY_SHIFT, USB_DCPCTR_PBUSY));

        USB_CFIFOSEL(pUSB, usCFIFOSEL);
    }

    /* Wait for access to the FIFO */
    while (r_usbh_wait_fifo(pUSB, iPipeNumber, 50))
    {
        /* Give it two attempts */
        iTrys--;
        if (!iTrys)
        {
            /* Module would not release FIFO */
            TRACE(("USB Module would not release FIFO %d\r\n"));
            return -1UL;
        }
        else
        {
            USB_CFIFOSEL(pUSB, usCFIFOSEL);
        }
    }

    /* Check length */
    if (stLength)
    {
        USB_PIPESEL(pUSB, iPipeNumber);

        /* Get the maximum packet size */
        if (iPipeNumber)
        {
            stMaxPacket = rza_io_reg_read_16(&pUSB->PIPEMAXP, USB_PIPEMAXP_MXPS_SHIFT, USB_PIPEMAXP_MXPS);
        }
        else
        {
            stMaxPacket = rza_io_reg_read_16(&pUSB->DCPMAXP, USB_DCPMAXP_MXPS_SHIFT, USB_DCPMAXP_MXPS);
        }

        /* Write at maximum one packet */
        if (stLength > stMaxPacket)
        {
            stLength = stMaxPacket;
        }

        /* Write the data into the FIFO */
        r_usbh_write_fifo(pUSB, pbySrc, stLength);

        /* If it is a short packet then the buffer must be validated */
        if (stLength < stMaxPacket)
        {
            rza_io_reg_write_16(&pUSB->CFIFOCTR, USB_CFIFOCTR_BVAL, NO_SHIFT, (uint16_t)~GENERIC_16B_MASK);
        }
    }
    else
    {

        /* Send zero length packet */
        rza_io_reg_write_16(&pUSB->CFIFOCTR, (USB_CFIFOCTR_BVAL | USB_CFIFOCTR_BCLR), NO_SHIFT, GENERIC_16B_MASK);
    }
    return stLength;
}
/******************************************************************************
 End of function  R_USBH_WritePipe
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_DmaWritePipe
 Description:   Function to configure a pipe for the DMA to write to it
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The pipe to configure
 IN  wNumPackets - The number of packets to be written
 Return value:  0 for success -1 on error
 ******************************************************************************/
int R_USBH_DmaWritePipe (PUSB pUSB, int iPipeNumber, uint16_t wNumPackets)
{
    UNUSED_PARAM(wNumPackets);
    if ((iPipeNumber > 0) && (iPipeNumber <= USBH_NUM_DMA_ENABLED_PIPES))
    {
        uint16_t usDFIFOSEL;

        /* Clear the transaction counter */
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRCLR = 1;
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRCLR = 0;

        /* Page 1610 of manual For the pipe in the transmitting direction, set these bits to 0"
        Set the transaction count */
        *USB_PIPETRN(pUSB, iPipeNumber) = 0;

        /* Set the DFIFOSEL register */
        usDFIFOSEL = (uint16_t) (
#if USBH_FIFO_BIT_WIDTH == 32
                USB_D0FIFOSEL_MBW(2)
#else
                        USB_D0FIFOSEL_MBW(1)
#endif
#ifdef _BIG
                        | USB_D0FIFOSEL_BIG_END
#endif
|        USB_D0FIFOSEL_CURPIPE(iPipeNumber));
        USB_D0FIFOSEL(pUSB, usDFIFOSEL);

        /* Enable DMA on this FIFO */
        rza_io_reg_write_16(&pUSB->D0FIFOSEL, USB_DnFIFOSEL_DREQE, NO_SHIFT, (uint16_t)~GENERIC_16B_MASK);

        /* Enable Transfer End Sampling - specific setting to dual core DMA */
#if USBH_HIGH_SPEED_SUPPORT == 1

        /* Enable Transfer End Sampling */
        rza_io_reg_write_16(&pUSB->D0FBCFG, USB_DnFBCFG_TENDE, NO_SHIFT, GENERIC_16B_MASK);
#endif

        /* Enable the transaction counter */
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRENB = 0;
        return 0;
    }
    return -1;
}
/******************************************************************************
 End of function  R_USBH_DmaWritePipe
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_StopDmaPipe
 Description:   Function to unconfigure a pipe from a DMA transfer
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 Return value:  none
 ******************************************************************************/
void R_USBH_StopDmaPipe (PUSB pUSB)
{

#if USBH_FIFO_BIT_WIDTH == 32
    USB_D0FIFOSEL(pUSB, USB_CFIFOSEL_MBW_FUNC(2));
    rza_io_reg_write_16(&pUSB->D0FBCFG, 0x0, USB_DnFBCFG_TENDE_SHIFT, USB_DnFBCFG_TENDE);        /* Disable DMA Transfer End Sampling */
#else
    USB_D0FIFOSEL(pUSB, USB_CFIFOSEL_MBW_FUNC(1));
#endif
}
/******************************************************************************
 End of function  R_USBH_StopDmaPipe
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_ReadPipe
 Description:   Function to read from an endpoint pipe FIFO
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The pipe number to read from
 IN  pbyDest - Pointer to the destination memory
 (FIFO size aligned)
 IN  stLength - The length of the destination memory
 NOTE: This function uses 16 or 32 bit access so make sure that the memory is
 appropriately aligned. Make sure there is a multiple of two or four
 bytes in length too.
 Return value:  The number of bytes read from the FIFO or -1 on FIFO access
 error and -2 on overrun error
 ******************************************************************************/
size_t R_USBH_ReadPipe (PUSB pUSB, int iPipeNumber, uint8_t *pbyDest, size_t stLength)
{
    size_t stAvailable;
    int iReadFIFOStuck = 100;
    uint16_t usCFIFOSEL = (uint16_t) (
#if USBH_FIFO_BIT_WIDTH == 32
            USB_CFIFOSEL_MBW_FUNC(2)
#else
                    USB_CFIFOSEL_MBW_FUNC(1)
#endif
#ifdef _BIG
                    | USB_CFIFOSEL_BIG_END
#endif
|    USB_CFIFOSEL_CURPIPE_FUNC(iPipeNumber));

    /* Make sure that the pipe is not busy */
    while (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PBUSY)
    {
        ; /* Wait for the pipe to stop action */
    }

    /* Select the FIFO */
    USB_CFIFOSEL(pUSB, usCFIFOSEL);

    /* Wait for access to FIFO */
    while ( (rza_io_reg_read_16(&pUSB->CFIFOSEL, USB_CFIFOSEL_CURPIPE_SHIFT, USB_CFIFOSEL_CURPIPE) != iPipeNumber) ||
            (rza_io_reg_read_16(&pUSB->CFIFOCTR, USB_CFIFOCTR_FRDY_SHIFT, USB_CFIFOCTR_FRDY) == 0) )
    {
        USB_CFIFOSEL(pUSB, usCFIFOSEL);

        /* Prevent from getting stuck in this loop */
        iReadFIFOStuck--;
        if (!iReadFIFOStuck)
        {
            TRACE(("FIFO Stuck %d\r\n", iReadFIFOStuck));

            /* Clear the pipe */
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
            rza_io_reg_write_16(&pUSB->CFIFOCTR, 0x1, USB_CFIFOCTR_BCLR_SHIFT, USB_CFIFOCTR_BCLR);
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
            return -1UL;
        }
    }
    USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;

    /* Get the length of data available to read */
    stAvailable = rza_io_reg_read_16(&pUSB->CFIFOCTR, USB_CFIFOCTR_DTLN_SHIFT, USB_CFIFOCTR_DTLN);
    if (stAvailable)
    {
        /* Only read the quantity of data in the FIFO */
        if (stLength >= stAvailable)
        {
            /* Read the data from the FIFO */
            stLength = stAvailable;
            r_usbh_read_fifo(pUSB, pbyDest, stLength);
        }
        else
        {
            /* There is more data in the FIFO than available!
             This is an overrun error */

            /* If you have arrived here, then check one packet Rx packet has not
             * been missed. This can be the case if compiler *Speed Optimisation*
             * has not been turned on.
             */

            return -2UL;
        }
    }
    else
    {
        /* We have read a zero length packet & need to acknowledge it */
        rza_io_reg_write_16(&pUSB->CFIFOCTR, 0x1, USB_CFIFOCTR_BCLR_SHIFT, USB_CFIFOCTR_BCLR);
        stLength = 0UL;
    }
    return stLength;
}
/******************************************************************************
 End of function  R_USBH_ReadPipe
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_DataInFIFO
 Description:   Function to check the FIFO for more received data
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The pipe number to check
 Return value:  The number of bytes in the FIFO or NULL on error
 ******************************************************************************/
int R_USBH_DataInFIFO (PUSB pUSB, int iPipeNumber)
{

    int iReadFIFOStuck = 100;
    uint16_t usCFIFOSEL = (uint16_t) (
#if USBH_FIFO_BIT_WIDTH == 32
            USB_CFIFOSEL_MBW_FUNC(2)
#else
                    USB_CFIFOSEL_MBW_FUNC(1)
#endif
#ifdef _BIG
                    | USB_CFIFOSEL_BIG_END
#endif
|    USB_CFIFOSEL_CURPIPE_FUNC(iPipeNumber));

    /* Stop any further transactions */
    USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;

    /* Make sure that the pipe is not busy */
    while (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PBUSY)
    {
        /* Wait for the pipe to stop action */
    }

    /* Select the FIFO */
    USB_CFIFOSEL(pUSB, usCFIFOSEL);

    /* Wait for access to FIFO */
    while ( (rza_io_reg_read_16(&pUSB->CFIFOSEL, USB_CFIFOSEL_CURPIPE_SHIFT, USB_CFIFOSEL_CURPIPE) != iPipeNumber) ||
            (rza_io_reg_read_16(&pUSB->CFIFOCTR, USB_CFIFOCTR_FRDY_SHIFT, USB_CFIFOCTR_FRDY) == 0) )
    {
        /* Prevent from getting stuck in this loop */
        iReadFIFOStuck--;
        if (!iReadFIFOStuck)
        {
            return -1;
        }
    }
    return (int) rza_io_reg_read_16(&pUSB->CFIFOCTR, USB_CFIFOCTR_DTLN_SHIFT, USB_CFIFOCTR_DTLN);
}
/******************************************************************************
 End of function  R_USBH_DataInFIFO
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_DmaReadPipe
 Description:   Function to configure a pipe for the DMA to read from it
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The pipe to configure
 IN  wNumPackets - The number of packets to be read
 Return value:  0 for success -1 on error
 ******************************************************************************/
int R_USBH_DmaReadPipe (PUSB pUSB, int iPipeNumber, uint16_t wNumPackets)
{
    /* DMA only available on pipes 1, 2, 3, 4 and 5 - Pipes 6 - 9 the DMA can
     only fill one packet (Interrupt transfers) and another function
     should be written to handle DMA transfers on these pipes */
    if ((iPipeNumber > 0) && (iPipeNumber < 6))
    {
        uint16_t usDFIFOSEL;

        /* Select the pipe */
        USB_PIPESEL(pUSB, iPipeNumber);

        /* Enable double buffering for DMA */
#if USBH_HIGH_SPEED_SUPPORT == 1
        /*
         ** POSSIBLE BUG IN FULL SPEED VARIANT OF 597 IP **
         There is a possible bug in the Full speed variant of the 597 IP.
         When double buffering is enabled and a data of length equal to the
         value set in the PIPEMAXP MXPS field is read from the FIFO
         the same data is read twice . Please check errata's and ECN &
         remove conditional compile when fixed.
         */
        rza_io_reg_write_16(&pUSB->PIPECFG, 0x1, USB_PIPECFG_DBLB_SHIFT, USB_PIPECFG_DBLB);
#endif
        /* Disable the transaction counter */
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRENB = 0;

        /* Clear the transaction counter */
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRCLR = 1;
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRCLR = 0;

        /* Set the transaction count */
        *USB_PIPETRN(pUSB, iPipeNumber) = wNumPackets;

        /* Set the DFIFOSEL register */
        usDFIFOSEL = (uint16_t) (
#if USBH_FIFO_BIT_WIDTH == 32
                USB_D0FIFOSEL_MBW(2)
#else
                        USB_D0FIFOSEL_MBW(1)
#endif
#ifdef _BIG
                        | USB_D0FIFOSEL_BIG_END
#endif
|        USB_D0FIFOSEL_CURPIPE(iPipeNumber));
        USB_D1FIFOSEL(pUSB, usDFIFOSEL);

        /* Enable DMA on this FIFO */
        rza_io_reg_write_16(&pUSB->D1FIFOSEL, USB_DnFIFOSEL_DREQE, NO_SHIFT, (uint16_t)~GENERIC_16B_MASK);
#if USBH_HIGH_SPEED_SUPPORT == 1

        /* Enable Transfer End Sampling */
        rza_io_reg_write_16(&pUSB->D1FBCFG, USB_DnFBCFG_TENDE, NO_SHIFT, GENERIC_16B_MASK);

#endif

        /* Enable the transaction counter */
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRENB = 1;

        return 0;
    }
    return -1;
}
/******************************************************************************
 End of function  R_USBH_DmaReadPipe
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_StopDmaReadPipe
 Description:   Function to unconfigure a pipe from a DMA read
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 Return value:  none
 ******************************************************************************/
void R_USBH_StopDmaReadPipe (PUSB pUSB)
{
#if USBH_FIFO_BIT_WIDTH == 32
    USB_D1FIFOSEL(pUSB, USB_CFIFOSEL_MBW_FUNC(2));
#else
    USB_D1FIFOSEL(pUSB, USB_CFIFOSEL_MBW_FUNC(1));
#endif
}
/******************************************************************************
 End of function  R_USBH_StopDmaReadPipe
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_DmaFIFO
 Description:   Function to get a pointer to the DMA FIFO
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  transferDirection - The direction of transfer
 Return value:  Pointer to the DMA FIFO
 ******************************************************************************/
void *R_USBH_DmaFIFO (PUSB pUSB, USBDIR transferDirection)
{

    if (transferDirection)
    {
        return (void*) &pUSB->D1FIFO;
    }
    else
    {
        return (void*) &pUSB->D0FIFO;
    }
}

/******************************************************************************
 End of function  R_USBH_DmaFIFO
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_DmaTransac
 Description:   Function to get the value of the transaction counter
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe
 Return value:  The value of the transaction counter
 ******************************************************************************/
uint16_t R_USBH_DmaTransac (PUSB pUSB, int iPipeNumber)
{
    uint16_t usTransac = 0;
    if ((iPipeNumber > 0) && (iPipeNumber <= USBH_NUM_DMA_ENABLED_PIPES))
    {
        usTransac = *USB_PIPETRN(pUSB, iPipeNumber);
    }
    return usTransac;
}
/******************************************************************************
 End of function  R_USBH_DmaTransac
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_SetDevAddrCfg
 Description:   Function to set the appropriate Device Address Configuration
 Register for a given device
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  byDeviceAddress - The address of the device
 IN  pbyHubAddress - If the device is on a hub port then this
 must point to the address of the hub
 IN  byHubPort - If the device is on a hub then this must be set
 to the index of the port on the hub that the
 device is attached to
 IN  byRootPort - The index of the root port - if it is a 1 port
 device this must be set to 0
 IN  transferSpeed - The transfer speed of the device
 Return value:  true if succesful
 ******************************************************************************/
_Bool R_USBH_SetDevAddrCfg (PUSB pUSB, uint8_t byDeviceAddress, uint8_t *pbyHubAddress, uint8_t byHubPort,
        uint8_t byRootPort, USBTS transferSpeed)
{
    if (byDeviceAddress <= USBH_MAX_NUM_PIPES)
    {
        /* Get a pointer to the appropriate register */
        volatile PDEVADDR pDEVADDR = USB_DEVADD(pUSB, byDeviceAddress);

        /* Set the device transfer speed */
        switch (transferSpeed)
        {
            default :
            case USBH_FULL :
                pDEVADDR->BIT.USBSPD = 2;
            break;
            case USBH_SLOW :
                pDEVADDR->BIT.USBSPD = 1;
            break;
            case USBH_HIGH :
                pDEVADDR->BIT.USBSPD = 3;
            break;
        }

        /* If the device is connected to a hub */
        if (pbyHubAddress)
        {
            /* Set the address of the hub */
            pDEVADDR->BIT.UPPHUB = (unsigned) (*pbyHubAddress & 0xf);

            /* Set the index of the port on the hub */
            pDEVADDR->BIT.HUBPORT = (unsigned) (byHubPort & 0x7);

            /* Set the index of the root port - hubs are only supported on the root ports */
            pDEVADDR->BIT.RTPOR = (unsigned) (byRootPort & 0x1);
        }
        else
        {
            /* Set to 0 when on a root port */
            pDEVADDR->BIT.UPPHUB = 0;
            pDEVADDR->BIT.HUBPORT = 0;
            pDEVADDR->BIT.RTPOR = (unsigned) (byRootPort & 0x1);
        }
        return true;
    }
    return false;
}
/******************************************************************************
 End of function  R_USBH_SetDevAddrCfg
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_PipeReady
 Description:   Function to check to see if the pipe is ready
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The number of the pipe
 Return value:  true if the pipe is ready for use
 ******************************************************************************/
_Bool R_USBH_PipeReady (PUSB pUSB, int iPipeNumber)
{

    if (iPipeNumber > 0)
    {
        if (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PBUSY)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (iPipeNumber == 0)
    {
        if (rza_io_reg_read_16(&pUSB->DCPCTR, USB_DCPCTR_PBUSY, USB_DCPCTR_PBUSY)) 
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}
/******************************************************************************
 End of function  R_USBH_PipeReady
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_ClearSetupRequest
 Description:   Function to clear the setup request
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 Return value:  none
 ******************************************************************************/
void R_USBH_ClearSetupRequest (PUSB pUSB)
{
    rza_io_reg_write_16(&pUSB->DCPCTR, 0x1, USB_DCPCTR_SUREQCLR_SHIFT, USB_DCPCTR_SUREQCLR);
#if USBH_HIGH_SPEED_SUPPORT == 1
    rza_io_reg_write_16(&pUSB->DCPCTR, 0x1, USB_DCPCTR_PINGE_SHIFT, USB_DCPCTR_PINGE);
#endif
}
/******************************************************************************
 End of function  R_USBH_ClearSetupRequest
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_EnableSetupInterrupts
 Description:   Function to clear the setup request
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  bfEnable - true to enable false to disable
 Return value:  none
 ******************************************************************************/
void R_USBH_EnableSetupInterrupts (PUSB pUSB, _Bool bfEnable)
{

    if (bfEnable)
    {
        /* Enable the setup transaction interrupts */
        rza_io_reg_write_16(&pUSB->INTENB1, 0x1, USB_INTENB1_SIGNE_SHIFT, USB_INTENB1_SIGNE);
        rza_io_reg_write_16(&pUSB->INTENB1, 0x1, USB_INTENB1_SACKE_SHIFT, USB_INTENB1_SACKE);
    }
    else
    {
        /* Disable the interrupt requests */
        rza_io_reg_write_16(&pUSB->INTENB1, 0x0, USB_INTENB1_SIGNE_SHIFT, USB_INTENB1_SIGNE);
        rza_io_reg_write_16(&pUSB->INTENB1, 0x0, USB_INTENB1_SACKE_SHIFT, USB_INTENB1_SACKE);
    }
}
/******************************************************************************
 End of function  R_USBH_EnableSetupInterrupts
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_SetSOF_Speed
 Description:   Function to set the SOF speed to match the device speed
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  transferSpeed - Select the speed of the device.
 Return value:  none
 ******************************************************************************/
void R_USBH_SetSOF_Speed (PUSB pUSB, USBTS transferSpeed)
{

    /* Set the SOF output for a low speed device */
    if (transferSpeed == USBH_SLOW)
    {
        rza_io_reg_write_16(&pUSB->SOFCFG, 0x1, USB_SOFCFG_TRNENSEL_SHIFT, USB_SOFCFG_TRNENSEL);
    }
    else
    {
        rza_io_reg_write_16(&pUSB->SOFCFG, 0x0, USB_SOFCFG_TRNENSEL_SHIFT, USB_SOFCFG_TRNENSEL);
    }
}
/******************************************************************************
 End of function  R_USBH_SetSOF_Speed
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_SetControlPipeDirection
 Description:   Function to set the direction of transfer of the control pipe
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  transferDirection - the direction of transfer
 Return value:  none
 ******************************************************************************/
void R_USBH_SetControlPipeDirection (PUSB pUSB, USBDIR transferDirection)
{

    if (transferDirection)
    {
        rza_io_reg_write_16(&pUSB->DCPCFG, 0x0, USB_DCPCFG_DIR_SHIFT, USB_DCPCFG_DIR);

    }
    else
    {
        rza_io_reg_write_16(&pUSB->DCPCFG, 0x1, USB_DCPCFG_DIR_SHIFT, USB_DCPCFG_DIR);
    }
}
/******************************************************************************
 End of function  R_USBH_SetControlPipeDirection
 ******************************************************************************/

/******************************************************************************
 Function Name: R_USBH_SendSetupPacket
 Description:   Function to send a setup packet
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 IN  byAddress - The address of the device to receive the packet
 IN  wPacketSize - The size of the device's endpoint
 IN  bmRequestType - The request bit map
 IN  bRequest - The request
 IN  wValue - The request value
 IN  wIndex - The request index
 IN  wLength - The request length
 Return value:  none
 ******************************************************************************/
void R_USBH_SendSetupPacket (PUSB pUSB, uint8_t byAddress, uint16_t wPacketSize, uint8_t bmRequestType,
        uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{

    /* Make sure that the pipe PID is set to NAK */
    rza_io_reg_write_16(&pUSB->DCPCTR, 0x0, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);

    /* Set the device address */
    rza_io_reg_write_16(&pUSB->DCPMAXP, byAddress, USB_DCPMAXP_DEVSEL_SHIFT, USB_DCPMAXP_DEVSEL);

    /* Set the endpoint packet size */
    rza_io_reg_write_16(&pUSB->DCPMAXP, wPacketSize, USB_DCPMAXP_MXPS_SHIFT, USB_DCPMAXP_MXPS);

    /* Put the setup data into the registers */
    rza_io_reg_write_16(&pUSB->USBREQ, bmRequestType, USB_USBREQ_BMREQUESTTYPE_SHIFT, USB_USBREQ_BMREQUESTTYPE);
    rza_io_reg_write_16(&pUSB->USBREQ, bRequest, USB_USBREQ_BREQUEST_SHIFT, USB_USBREQ_BREQUEST);
    rza_io_reg_write_16(&pUSB->USBVAL, wValue, USB_USBVAL_SHIFT, USB_USBVAL);
    rza_io_reg_write_16(&pUSB->USBINDX, wIndex, USB_USBINDX_SHIFT, USB_USBINDX);
    rza_io_reg_write_16(&pUSB->USBLENG, wLength, USB_USBLENG_SHIFT, USB_USBLENG);

    /* Submit the request */
    rza_io_reg_write_16(&pUSB->DCPCTR, 0x1, USB_DCPCTR_SUREQ_SHIFT, USB_DCPCTR_SUREQ);
}
/******************************************************************************
 End of function  R_USBH_SendSetupPacket
 ******************************************************************************/

/******************************************************************************
 Private Functions
 ******************************************************************************/

/******************************************************************************
 Function Name: r_usbh_wait_fifo
 Description:   Function to wait for the FRDY bit to be set indicating that the
 FIFO is ready for access
 Parameters:    IN  pUSB - Pointer to the Host Controller hardware
 IN  iPipeNumber - The pipe number
 IN  iCountOut - The number of times to check the FRDY flag
 Return value:  0 for success
 ******************************************************************************/
static int r_usbh_wait_fifo (PUSB pUSB, int iPipeNumber, int iCountOut)
{
    /* Wait for access to FIFO */
    while (rza_io_reg_read_16(&pUSB->CFIFOCTR, USB_CFIFOCTR_FRDY_SHIFT, USB_CFIFOCTR_FRDY) == 0)
    {
        /* Prevent from getting stuck in this loop */
        iCountOut--;
        if (!iCountOut)
        {
            /* Clear the pipe */
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
            rza_io_reg_write_16(&pUSB->CFIFOCTR, 1, USB_CFIFOCTR_BCLR_SHIFT, USB_CFIFOCTR_BCLR);     /* BCLR, register seems to want a full length access */
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
            return -1;
        }
    }

    /*Always clear pipe */
     rza_io_reg_write_16(&pUSB->CFIFOCTR, 0x1, USB_CFIFOCTR_BCLR_SHIFT, USB_CFIFOCTR_BCLR);
    return 0;
}
/******************************************************************************
 End of function  r_usbh_wait_fifo
 ******************************************************************************/

/******************************************************************************
 Function Name: r_usbh_config_fifo
 Description:   Function to configure the FIFO buffer allocation
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 Return value:  none
 ******************************************************************************/
#if USBH_HIGH_SPEED_SUPPORT == 1
static void r_usbh_config_fifo (PUSB pUSB)
{
    int iEndpoint;
    for (iEndpoint = 0; iEndpoint < (int) USB_FIFO_CONFIG_SIZE; iEndpoint++)
    {
        /* Select the pipe */
        USB_PIPESEL(pUSB, iEndpoint + 1);

        /* Set the buffer parameters */
        rza_io_reg_write_16(&pUSB->PIPEBUF, gcFifoConfig[iEndpoint].wPipeBuf, NO_SHIFT, GENERIC_16B_MASK);
    }
}
#endif
/******************************************************************************
 End of function  r_usbh_config_fifo
 ******************************************************************************/

/******************************************************************************
 Function Name: r_usbh_init_pipes
 Description:   Function to initialise the pipe assignment data
 Arguments:     IN  pUSB - Pointer to the Host Controller hardware
 Return value:  none
 ******************************************************************************/
static void r_usbh_init_pipes (PUSB pUSB)
{

    int iPipeNumber;

    /* Disable the control pipe */
    rza_io_reg_write_16(&pUSB->DCPCTR, 0x0,USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);

    /* Disable the remainder of the pipes */
    for (iPipeNumber = 1; iPipeNumber < USBH_MAX_NUM_PIPES; iPipeNumber++)
    {
        /* Disable the pipe */
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
    }
}
/******************************************************************************
 End of function  r_usbh_init_pipes
 ******************************************************************************/

/******************************************************************************
 Function Name: r_usbh_write_fifo
 Description:   Function to write to a FIFO
 Parameters:    IN  pUSB - Pointer to the Host Controller hardware
 IN  pvSrc - Pointer to the source memory
 IN  stLength - The number of bytes to write
 Return value:  none
 ******************************************************************************/
static void r_usbh_write_fifo (PUSB pUSB, void *pvSrc, size_t stLength)
{
    if ((pUSB) && (pvSrc))
    {
        size_t stBytes;
        if (((size_t) pvSrc & 3UL) == 0)
        {
            size_t stDwords;
            /* Peripheral with 32 bit access to FIFO */
            uint32_t *pulSrc = pvSrc;

            /* Calculate the number of DWORDS */
            stDwords = stLength >> 2;

            /* Calculate the number of BYTES */
            stBytes = stLength - (stDwords << 2);

            /* Write the DWORDs */
            while (stDwords--)
            {
                rza_io_reg_write_32((uint32_t *)&pUSB->CFIFO, *pulSrc++, USB_CFIFO_FIFOPORT_SHIFT, USB_CFIFO_FIFOPORT);
            }

            /* If there are bytes */
            if (stBytes)
            {
                uint8_t *pbySrc = (uint8_t*) pulSrc;

                /*     Depending on Endian (set via BIGEND bit) 8-bit FIFO access is via
                 *  a different address.
                 */
#ifdef _BIG
                uint8_t *pbyDest = (uint8_t*)&pUSB->CFIFO;
#else
                uint8_t *pbyDest = (uint8_t*)(((uint8_t *)(&pUSB->CFIFO) + 3)); /*TODO*/
#endif

                /* Select byte access */
                rza_io_reg_write_16(&pUSB->CFIFOSEL, 0x0, NO_SHIFT, USB_CFIFOSEL_MBW);

                /* Write the bytes */
                while (stBytes--)
                {
                    *pbyDest = *pbySrc++;
                }
            }
        }
        else if (((size_t) pvSrc & 1UL) == 0)
        {
            size_t stWords;
            /* Peripheral with 16 bit access to FIFO */
            uint16_t *pusSrc = pvSrc;
            /* Calculate the number of WORDS */
            stWords = stLength >> 1;
            /* Calculate the number of BYTES */
            stBytes = stLength - (stWords << 1);
            /* Write the words */
            while (stWords--)
            {
                rza_io_reg_write_16((uint16_t*)&pUSB->CFIFO, *pusSrc++, NO_SHIFT, GENERIC_16B_MASK);
            }

            /* If there are bytes */
            if (stBytes)
            {
                uint8_t *pbySrc = (uint8_t*)pusSrc;
                /*     Depending on Endian (set via BIGEND bit) 8-bit FIFO access is via
                 *  a different address.
                 */
#ifdef _BIG
                uint8_t *pbyDest = (uint8_t*)&pUSB->CFIFO;
#else
                uint8_t *pbyDest = (uint8_t*)(((uint8_t *)(&pUSB->CFIFO) + 3)); /*TODO*/
#endif
                /* Select byte access */
                rza_io_reg_write_16(&pUSB->CFIFOSEL, 0x0, NO_SHIFT, USB_CFIFOSEL_MBW);
                /* Write the bytes */
                while (stBytes--)
                {
                    *pbyDest = *pbySrc++;
                }
            }
        }
        else
        {
            uint8_t *pbySrc = (uint8_t*)pvSrc;
            /*     Depending on Endian (set via BIGEND bit) 8-bit FIFO access is via
             *  a different address.
             */
#ifdef _BIG
            uint8_t *pbyDest = (uint8_t*)&pUSB->CFIFO;
#else
            uint8_t *pbyDest = (uint8_t*)(((uint8_t *)(&pUSB->CFIFO) + 3));
#endif

            /* Calculate the number of BYTES */
            stBytes = stLength;
            /* Select byte access */
            rza_io_reg_write_16(&pUSB->CFIFOSEL, 0x0, NO_SHIFT, USB_CFIFOSEL_MBW);
            /* Write the bytes */
            while (stBytes--)
            {
                *pbyDest = *pbySrc++;
            }
        }
    }
}
/******************************************************************************
 End of function  r_usbh_write_fifo
 ******************************************************************************/

/******************************************************************************
 Function Name: r_usbh_read_fifo
 Description:   Function to read from a FIFO
 Parameters:    IN  pUSB - Pointer to the Host Controller hardware
 OUT pvDest - Pointer to the destination memory
 IN  stLength - The number of bytes to read
 Return value:  none
 ******************************************************************************/
static void r_usbh_read_fifo (PUSB pUSB, void *pvDest, size_t stLength)
{
#if USBH_FIFO_BIT_WIDTH == 32
    if ((pUSB) && (pvDest))
    {
        uint8_t *pbyDest = (uint8_t*) pvDest;

        /* Adjust the length for a number of DWORDS */
        while (stLength)
        {
            uint32_t data;
            uint32_t loop;
            data = rza_io_reg_read_32((uint32_t *)&pUSB->CFIFO, USB_CFIFO_FIFOPORT_SHIFT, USB_CFIFO_FIFOPORT);
            for (loop = 0; loop < sizeof(uint32_t); loop++)
            {
                if(stLength)
                {
        #ifdef _BIG
                    *pbyDest++ = (uint8_t)(data>>24);
                    data <<= 8;
        #else
                    *pbyDest++ = (uint8_t)data;
                    data >>= 8;
        #endif
                    stLength--;
                }
            }
        }
    }
#else
    if ((pUSB) && (pvDest))
    {
        uint8_t *pbyDest = (uint8_t*) pvDest;

        /* Adjust the length for a number of DWORDS */
        while (stLength)
        {
            uint16_t data;
            uint32_t loop;
            data = rza_io_reg_read_16(&pUSB->CFIFO, USB_CFIFO_FIFOPORT_SHIFT, USB_CFIFO_FIFOPORT);
            for (loop = 0; loop < sizeof(uint16_t); loop++)
            {
                if(stLength)
                {
        #ifdef _BIG
                    *pbyDest++ = (uint8_t)(data>>8);
                    data <<= 8;
        #else
                    *pbyDest++ = (uint8_t)data;
                    data >>= 8;
        #endif
                    stLength--;
                }
            }
        }
    }
#endif
}
/******************************************************************************
 End of function  r_usbh_read_fifo
 ******************************************************************************/

/*****************************************************************************
 * Function Name: r_usbh_cfiosel
 * Description  : Function to select the pipe
 * Arguments    : IN  pUSB - Pointer to the USB hardware
 *                IN  usCFIFOSEL - The value for the CFIFOSEL register
 * Return Value : none
 ******************************************************************************/
static void r_usbh_cfiosel (PUSB pUSB, uint16_t usCFIFOSEL)
{

    /* Under some circumstances if the pipe FIFO is assigned to the Serial
     Interface Engine the pipe number will not be set. Prevent from getting
     stuck in this loop */
    int iCount = 10;
    do
    {
        rza_io_reg_write_16(&pUSB->CFIFOSEL, usCFIFOSEL, NO_SHIFT, GENERIC_16B_MASK);
        if (iCount)
        {
            iCount--;
        }
        else
        {
            break;
        }
    } while (rza_io_reg_read_16(&pUSB->CFIFOSEL, NO_SHIFT, GENERIC_16B_MASK) != usCFIFOSEL);
}
/*****************************************************************************
 End of function  r_usbh_cfiosel
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
