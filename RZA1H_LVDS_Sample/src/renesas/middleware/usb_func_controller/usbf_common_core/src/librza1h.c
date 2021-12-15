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

/************************************************ Technical reference data ****
* File Name       : lib
*rza1h.c
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      :
* H/W Platform    : RSK2+RZA!H
* Description     : RZA1H usb register.
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

/******************************************************************************
User Include (Project Level Includes)
******************************************************************************/
/*    Following header file provides structure and prototype definition of USB 
    API's. */
#include "USB.h"
/*    Following header file provides a structure to access on-chip I/O 
    registers. */
#include "iodefine_cfg.h"
/* Following header file provides common defines for widely used items. */
#include "usb_common.h"

#include "rza_io_regrw.h"
#include "usb_iobitmask.h"

/******************************************************************************
Global Variable
******************************************************************************/
void resetmodule(volatile st_usb_object_t *_pchannel);
/******************************************************************************
User Program Code
******************************************************************************/

/******************************************************************************
* Function Name    : resetmodule
* Description    : reset USB Module
* Argument        : None
* Return value    : None
******************************************************************************/

void resetmodule(volatile st_usb_object_t *_pchannel)
{    
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0, 0, USB_SYSCFG_USBE_SHIFT, USB_SYSCFG_USBE);

    /* USB module operation enable */
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0, 1, USB_SYSCFG_USBE_SHIFT, USB_SYSCFG_USBE);
    
    /* disconnect detection only SOF  */

    if(HI_ENABLE == _pchannel->hi_speed_enable)
    {
        /* Hi-Speed Mode */
        rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0, 1, USB_SYSCFG_HSE_SHIFT, USB_SYSCFG_HSE);
    }
    else
    {
        /* Full Speed Mode */
        rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0, 0, USB_SYSCFG_HSE_SHIFT, USB_SYSCFG_HSE);
    }
}
/******************************************************************************
End of function resetmodule
******************************************************************************/

/******************************************************************************
* Function Name    : getBufSize
* Description    : Get Buf size
* Argument        : uint16_t                ; Pipe Number
* Return value    : uint16_t                ; Data Buffer Size
******************************************************************************/

uint16_t getBufSize(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    if(PIPE0 == Pipe)
    {
        /* Max Packet Size */
        _pchannel->generic_size = rza_io_reg_read_16(&_pchannel->phwdevice->DCPMAXP,
                                                     USB_DCPMAXP_MXPS_SHIFT, USB_DCPMAXP_MXPS);
    } 
    else 
    {
        /* pipe select */
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL, Pipe,
                            USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);

        /* read CNTMD */
        if(rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG,
                              USB_PIPECFG_CNTMD_SHIFT, USB_PIPECFG_CNTMD) )
        {            
            _pchannel->generic_buffer = rza_io_reg_read_16(&_pchannel->phwdevice->PIPEBUF,
                                                            USB_PIPEBUF_BUFSIZE_SHIFT, USB_PIPEBUF_BUFSIZE);

            /* Buffer Size */
            _pchannel->generic_size = (uint16_t) (((uint16_t) (_pchannel->generic_buffer + 1)) * PIPEXBUF);
        } 
        else 
        {
            /* Max Packet Size */
            _pchannel->generic_size = rza_io_reg_read_16(&_pchannel->phwdevice->PIPEMAXP,
                                      USB_PIPEMAXP_MXPS_SHIFT, USB_PIPEMAXP_MXPS);
        }
    }
    return _pchannel->generic_size;
}
/******************************************************************************
End of function getBufSize
******************************************************************************/

/******************************************************************************
* Function Name    : getMaxPacketSize
* Description    : Get max packet size
* Argument        : uint16_t                ; Pipe Number
* Return value    : uint16_t                ; Max Packet Size
******************************************************************************/

uint16_t getMaxPacketSize(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    if(PIPE0 ==  Pipe)
    {

        /* Max Packet Size */
        _pchannel->generic_size = rza_io_reg_read_16(&_pchannel->phwdevice->DCPMAXP,
                                                     USB_DCPMAXP_MXPS_SHIFT,
                                                     USB_DCPMAXP_MXPS);
    } 
    else
    {
        /* pipe select */
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL, Pipe,
                            USB_PIPESEL_PIPESEL, USB_PIPESEL_PIPESEL);

        /* Max Packet Size */
        _pchannel->generic_size = rza_io_reg_read_16(&_pchannel->phwdevice->PIPEMAXP,
                                  USB_PIPEMAXP_MXPS_SHIFT, USB_PIPEMAXP_MXPS);
    }
    return _pchannel->generic_size;
}
/******************************************************************************
End of function getMaxPacketSize
******************************************************************************/

/******************************************************************************
* Function Name    : VBINT_StsClear
* Description    : VBINT Interrupt Status Clear
* Argument        : uint16_t                ; Pipe Number
* Return value    : uint16_t                ; Max Packet Size
******************************************************************************/

void VBINT_StsClear(volatile st_usb_object_t *_pchannel)
{
    rza_io_reg_write_16(&_pchannel->phwdevice->INTSTS0,(uint16_t)~BITVBINT, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function VBINT_StsClear
******************************************************************************/

/******************************************************************************
* Function Name    : RESM_StsClear
* Description    : RESM Interrupt Status Clear
* Argument        : None
* Return value    : None
******************************************************************************/

void RESM_StsClear(volatile st_usb_object_t *_pchannel)
{
    /* Status Clear */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTSTS0,(uint16_t)~BITRESM, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function RESM_StsClear
******************************************************************************/

/******************************************************************************
* Function Name    : BCHG_StsClear
* Description    : BCHG Interrupt Status Clear
* Argument        : None
* Return value    : None
******************************************************************************/

void BCHG_StsClear(volatile st_usb_object_t *_pchannel)
{
    /* Status Clear */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTSTS1,(uint16_t)~BITBCHG, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function BCHG_StsClear
******************************************************************************/
