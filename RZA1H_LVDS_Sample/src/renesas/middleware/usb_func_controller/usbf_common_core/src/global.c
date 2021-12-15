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

/****************************************************************************
* File Name       : global.c
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      : HEW, Renesas SuperH Standard Tool chain v9.3
* H/W Platform    : RSK2+SH7269
* Description     : Global variable.
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

/******************************************************************************
User Include (Project Level Includes)
******************************************************************************/
/*    Following header file provides a structure to access on-chip I/O
    registers. */
#include "iodefine_cfg.h"
/*    Following header file provides a structure to access on-chip I/O
    registers. */
#include "compiler_settings.h"
/*    Following header file provides structure and prototype definition of USB 
    API's. */
#include "usb.h"

/******************************************************************************
Global Variables (Common)
******************************************************************************/
/* bit pattern table */
const uint16_t g_util_BitSet[16] = {    0x0001, 0x0002, 0x0004, 0x0008,
                        0x0010, 0x0020, 0x0040, 0x0080,
                        0x0100, 0x0200, 0x0400, 0x0800,
                        0x1000, 0x2000, 0x4000, 0x8000 };

void delay_ms(uint16_t Dcnt);
void delay_us(uint16_t Dcnt);
void pipe_tbl_clear(volatile st_usb_object_t *_pchannel);
void mem_clear(volatile st_usb_object_t *_pchannel);
void mem_tbl_clear(volatile st_usb_object_t *_pchannel);

/******************************************************************************
User Program Code
******************************************************************************/

/*******************************************************************************
* Function Name: pipe_tbl_clear
* Description    : clear Pipe table
* Argument       : none
* Return value   : none
*******************************************************************************/
void pipe_tbl_clear(volatile st_usb_object_t *_pchannel)
{
    for( _pchannel->generic_counter = 0;
         _pchannel->generic_counter < (MAX_PIPE_NO + 1);
         ++_pchannel->generic_counter )
    {
        _pchannel->pipe_tbl[_pchannel->generic_counter] = 0;
    }
}
/******************************************************************************
End of function pipe_tbl_clear
******************************************************************************/

/*******************************************************************************
* Function Name: mem_clear
* Description    : Memory clear
* Argument       : none
* Return value   : none
*******************************************************************************/
void mem_clear(volatile st_usb_object_t *_pchannel)
{
    for( _pchannel->generic_counter = 0;
         _pchannel->generic_counter < (MAX_PIPE_NO + 1);
         ++_pchannel->generic_counter )
    {
        _pchannel->pipe_flag[_pchannel->generic_counter]     = PIPE_IDLE;
        _pchannel->pipe_data_size[_pchannel->generic_counter] = 0;
    }
}
/******************************************************************************
End of function mem_clear
******************************************************************************/

/*******************************************************************************
* Function Name: ep_table_index_clear
* Description    : Endpoint index table clear
* Argument       : none
* Return value   : none
*******************************************************************************/
void ep_table_index_clear(volatile st_usb_object_t *_pchannel)
{
    for( _pchannel->generic_counter = 0;
         _pchannel->generic_counter <= MAX_EP_NO;
         ++_pchannel->generic_counter )
    {
        _pchannel->ep_table_index[_pchannel->generic_counter] = EP_ERROR;
    }
}
/******************************************************************************
End of function ep_table_index_clear
******************************************************************************/

/*******************************************************************************
* Function Name: mem_tbl_clear
* Description    : Clear Memory table
* Argument       : none
* Return value   : none
*******************************************************************************/
void mem_tbl_clear(volatile st_usb_object_t *_pchannel)
{
    for( _pchannel->generic_counter = 0;
         _pchannel->generic_counter < (MAX_PIPE_NO + 1);
         ++_pchannel->generic_counter )
    {
        _pchannel->dtcnt[_pchannel->generic_counter] = 0;
    }

}
/******************************************************************************
End of function mem_tbl_clear
******************************************************************************/

/******************************************************************************
* Function Name   : delay_us
* Description     : Micro-second delay function
* Argument        : None
* Return value    : time - Required micro-seconds
******************************************************************************/
void delay_us(uint16_t Dcnt)
{
    volatile int32_t delay = (Dcnt * 10000);

    while(delay--)
    {
        __asm("nop");
    }
}
/******************************************************************************
End of function delay_us
******************************************************************************/

/******************************************************************************
* Function Name   : delay_ms
* Description     : Milli-second delay function
* Argument        : None
* Return value    : time - Required milli-seconds
******************************************************************************/
void delay_ms(uint16_t Dcnt)
{
    R_OS_TaskSleep((uint32_t)(Dcnt/1000)+1);
}
/******************************************************************************
End of function delay_ms
******************************************************************************/

/******************************************************************************
* Function Name   : ep_to_pipe
* Description     : Endpoint number to pipe number
* Argument        : uint16_t Dir_Ep     ; Direction + Endpoint Number
* Return value    : uint16_t            ; Pipe Number
*                                       ; EP_ERROR
******************************************************************************/
uint16_t ep_to_pipe(volatile st_usb_object_t *_pchannel, uint16_t Dir_Ep)
{
    for( _pchannel->generic_counter = 1;
         _pchannel->generic_counter <= MAX_PIPE_NO;
         ++_pchannel->generic_counter )
    {
        if( (_pchannel->pipe_tbl[_pchannel->generic_counter] & 0x00ff) == Dir_Ep )
        {
            return _pchannel->generic_counter;
        }
    }
    return EP_ERROR;
}
/******************************************************************************
End of function ep_to_pipe
******************************************************************************/
