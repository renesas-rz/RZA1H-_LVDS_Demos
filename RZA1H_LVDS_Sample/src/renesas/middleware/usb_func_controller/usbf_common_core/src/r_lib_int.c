/******************************************************************************
* DISCLAIMER                                                                      
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products-> No other uses are authorized->
* This software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws->
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES
* REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY,
* INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT->  ALL SUCH WARRANTIES ARE EXPRESSLY
* DISCLAIMED->
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
* FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS
* AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES->
* Renesas reserves the right, without notice, to make changes to this
* software and to discontinue the availability of this software->
* By using this software, you agree to the additional terms and
* conditions found by accessing the following link:
* http://www->renesas->com/disclaimer
******************************************************************************
* Copyright (C) 2010 Renesas Electronics Corporation-> All rights reserved-> */
/******************************************************************************
* File Name       : r_lib_int.c
* Version         : 1->00
* Device          : RZA1H
* Tool Chain      :
* H/W Platform    :
* Description     : USB Interrupt Register Handlier
******************************************************************************/

/******************************************************************************
* History         : 17->01->2016 Ver-> 1->00 First Release
******************************************************************************/

/******************************************************************************
User Include (Project Level Includes)
******************************************************************************/
/*    Following header file provides structure and prototype definition of USB 
    API's-> */
#include "usb.h"
/*    Following header file provides a structure to access on-chip I/O 
    registers. */
#include "iodefine_cfg.h"
/*    Following header file provides a structure of HAL Layer. */
#include "r_usb_hal.h"
/* Following header file provides common defines for widely used items. */
#include "usb_common.h"

#include "r_lib_int.h"

#include "rza_io_regrw.h"
#include "usb_iobitmask.h"

/******************************************************************************
Global Variable
******************************************************************************/

/******************************************************************************
User Program Code
******************************************************************************/

/******************************************************************************
* Function Name   : intr_int_pipe
* Description     : INTR interrupt
* Argument        : uint16_t Status      ; BRDYSTS Register Value
*                   uint16_t Int_enbl    ; BRDYENB Register Value
* Return value    : None
******************************************************************************/
static void intr_int_pipe(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl)
{
    if( (Status & BITBRDY1) && (Int_enbl & BITBRDY1) ) 
    {
        /* BRDY Status Clear */
        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY1, ACC_16B_SHIFT, ACC_16B_MASK);

        /* BEMP Status Clear */
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP1, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,   PIPE1, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);
        if( 0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR) )
        {
            _pchannel->endflag_k = R_USBF_DataioBufRead(_pchannel, PIPE1);
            switch( _pchannel->endflag_k )
            {
                case    FIFOERROR:
                    if(NULL != _pchannel->callbacks.p_cb_error)
                    {
                        _pchannel->callbacks.p_cb_error(USB_ERR_BULK_OUT_NO_BUFFER);
                    }
                    break;
                case    READEND:                                
                case    READSHRT:                                
                case    READOVER:    
                    if (NULL != _pchannel->callbacks.p_cb_bout_mfpdone)
                    {
                        _pchannel->callbacks.p_cb_bout_mfpdone((volatile void *)_pchannel, 0,_pchannel->rdcnt[PIPE1]);
                    }
                    break;
                default:
                    break;
            }
        }    
        else 
        {
            _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, PIPE1);
        }
    }
    if( (Status & BITBRDY2) && (Int_enbl & BITBRDY2) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY2, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP2, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,   PIPE2, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR) )
        {
            _pchannel->endflag_k = R_USBF_DataioBufRead(_pchannel, PIPE2);
        } 
        else 
        {
            _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, PIPE2);
            switch( _pchannel->endflag_k )
            {                
                case    WRITEEND:

                    /* Set BUF */
                    R_LIB_SetBUF(_pchannel, PIPE2);
                    if (NULL != _pchannel->callbacks.p_cb_bin_mfpdone)
                    {    
                        _pchannel->callbacks.p_cb_bin_mfpdone(_pchannel, USB_ERR_OK);
                    
                    }
                    break;
                case    WRITESHRT:

                        /* Set BUF */
                    R_LIB_SetBUF(_pchannel, PIPE2);
                    if (NULL != _pchannel->callbacks.p_cb_bin_mfpdone)
                    {    
                        _pchannel->callbacks.p_cb_bin_mfpdone(_pchannel, USB_ERR_OK);
                    
                    }
                    break;
                case    FIFOERROR:

                    /* Set BUF */
                    R_LIB_SetBUF(_pchannel, PIPE2);
                    _pchannel->callbacks.p_cb_error(USB_ERR_BULK_OUT_NO_BUFFER);
                    break;
                default:
                    break;
            }            
        } 
    }
    if( (Status & BITBRDY3) && (Int_enbl & BITBRDY3) )
    {

        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY3, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP3, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,   PIPE3, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR))
        {
            _pchannel->endflag_k = R_USBF_DataioBufRead(_pchannel, PIPE3);
        } 
        else
        {
            _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, PIPE3);
        }
    }
    if( (Status & BITBRDY4) && (Int_enbl & BITBRDY4) ) {
        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY4, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP4, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,   PIPE4, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR))
        {
            _pchannel->endflag_k = R_USBF_DataioBufRead(_pchannel, PIPE4);
        } 
        else
        {
            _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, PIPE4);
        }
    }
    if( (Status & BITBRDY5) && (Int_enbl & BITBRDY5) )
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY5, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP5, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,   PIPE5, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR))
        {
            _pchannel->endflag_k = R_USBF_DataioBufRead(_pchannel, PIPE5);
        } 
        else
        {
            _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, PIPE5);
        }
    }
    if( (Status & BITBRDY6) && (Int_enbl & BITBRDY6) )
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY6, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP6, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,   PIPE6, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR))
        {
            _pchannel->endflag_k = R_USBF_DataioBufRead(_pchannel, PIPE6);
        } 
        else
        {
            _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, PIPE6);
            switch( _pchannel->endflag_k )
            {
            case    WRITEEND:
                    if (NULL != _pchannel->callbacks.p_cb_iin_mfpdone)
                    {
                        _pchannel->callbacks.p_cb_iin_mfpdone(USB_ERR_OK);
                    }
                    break;
            case    FIFOERROR:
                    _pchannel->callbacks.p_cb_iin_mfpdone(USB_ERR_BULK_OUT_NO_BUFFER);
                    break;
            default:
                    break;
            }             
        }
    }
    if( (Status & BITBRDY7) && (Int_enbl & BITBRDY7) )
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY7, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP7, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,   PIPE7, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR))
        {
            _pchannel->endflag_k = R_USBF_DataioBufRead(_pchannel, PIPE7);
        } 
        else
        {
            _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, PIPE7);
        }
    }
    if( (Status & BITBRDY8) && (Int_enbl & BITBRDY8) )
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY8, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP8, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,   PIPE8, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR))
        {
            _pchannel->endflag_k = R_USBF_DataioBufRead(_pchannel, PIPE8);
        } 
        else
        {
            _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, PIPE8);
        }
    }
    if( (Status & BITBRDY9) && (Int_enbl & BITBRDY9) )
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY9, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP9, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,   PIPE9, USB_PIPESEL_PIPESEL_SHIFT, USB_PIPESEL_PIPESEL);
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPECFG, USB_PIPECFG_DIR_SHIFT, USB_PIPECFG_DIR))
        {
            _pchannel->endflag_k = R_USBF_DataioBufRead(_pchannel, PIPE9);
        } 
        else
        {
            _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, PIPE9);
        }
    }
}
/******************************************************************************
End of function intr_int_pipe
******************************************************************************/   

/******************************************************************************
* Function Name   : intn_int_pipe
* Description     : INTN interrupt
* Argument        : uint16_t Status      ; NRDYSTS Register Value
*                   uint16_t Int_enbl    ; NRDYENB Register Value
* Return value    : None
******************************************************************************/
static void intn_int_pipe(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl)
{
    _pchannel->generic_bitcheck    = (uint16_t)(Status & Int_enbl);

    rza_io_reg_write_16(&_pchannel->phwdevice->NRDYSTS,  (uint16_t)~Status, ACC_16B_SHIFT, ACC_16B_MASK);

    for( _pchannel->generic_counter = PIPE1; _pchannel->generic_counter <= PIPE9; _pchannel->generic_counter++ )
    {
        /* interrupt check */
        if( (_pchannel->generic_bitcheck&g_util_BitSet[_pchannel->generic_counter])==g_util_BitSet[_pchannel->generic_counter] )
        {
            _pchannel->generic_buffer    = R_LIB_GetPid(_pchannel, _pchannel->generic_counter);

            /* STALL ? */
            if(PID_STALL == _pchannel->generic_buffer)
            {
                _pchannel->pipe_flag[PIPE1] = PIPE_STALL;
            }
            else
            {
                /* Ignore count */
                _pchannel->pipe_ignore[_pchannel->generic_counter]++;
                if(PIPEERR == _pchannel->pipe_ignore[_pchannel->generic_counter])
                {
                    _pchannel->pipe_flag[_pchannel->generic_counter] = PIPE_NORES;
                }

                /* PIPE0 Send IN or OUT token */
                else
                {
                    R_LIB_SetBUF(_pchannel, _pchannel->generic_counter);
                }
            }
        }
    }

}
/******************************************************************************
End of function intn_int_pipe
******************************************************************************/   

/******************************************************************************
* Function Name   : bemp_int_pipe
* Description     : BEMP interrupt
* Argument        : uint16_t Status      ; BEMPSTS Register Value
*                   uint16_t Int_enbl    ; BEMPENB Register Value
* Return value    : None
******************************************************************************/
static void bemp_int_pipe(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl)
{
    if( (Status & BITBEMP1) && (Int_enbl & BITBEMP1) )
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP1, ACC_16B_SHIFT, ACC_16B_MASK);
        _pchannel->generic_buffer = R_LIB_GetPid(_pchannel, PIPE1);
        if(PID_STALL == _pchannel->generic_buffer)
        {
            _pchannel->pipe_flag[PIPE1] = PIPE_STALL;
        } 
        else 
        {
            if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPE1CTR, USB_PIPEnCTR_1_5_INBUFM_SHIFT, USB_PIPEnCTR_1_5_INBUFM))
            {
                /* Disable BEMP Interrupt */
                R_LIB_DisableIntE(_pchannel, PIPE1);

                /* End */
                _pchannel->pipe_flag[PIPE1] = PIPE_DONE;
            }
        }
    }
    if( (Status & BITBEMP2) && (Int_enbl & BITBEMP2) )
     {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP2, ACC_16B_SHIFT, ACC_16B_MASK);
        _pchannel->generic_buffer = R_LIB_GetPid(_pchannel, PIPE2);
        if(_pchannel->config.bulk_in_no_short_packet)
        {
            R_USBF_DataioBufWrite(_pchannel, PIPE2);
        }
        else
        {
            if(PID_STALL == _pchannel->generic_buffer)
            {
                _pchannel->pipe_flag[PIPE2] = PIPE_STALL;
            } 
            else 
            {
                if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPE2CTR, USB_PIPEnCTR_1_5_INBUFM_SHIFT, USB_PIPEnCTR_1_5_INBUFM))
                {
                    /* Disable BEMP Interrupt */
                    R_LIB_DisableIntE(_pchannel, PIPE2);

                    /* End */
                    _pchannel->pipe_flag[PIPE2] = PIPE_DONE;
                }
            }
        }
    }
    if( (Status & BITBEMP3) && (Int_enbl & BITBEMP3) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP3, ACC_16B_SHIFT, ACC_16B_MASK);
        _pchannel->generic_buffer = R_LIB_GetPid(_pchannel, PIPE3);
        if(PID_STALL == _pchannel->generic_buffer)
        {
            _pchannel->pipe_flag[PIPE3] = PIPE_STALL;
        } 
        else 
        {
            if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPE3CTR, USB_PIPEnCTR_1_5_INBUFM_SHIFT, USB_PIPEnCTR_1_5_INBUFM))
            {
                /* Disable BEMP Interrupt */
                R_LIB_DisableIntE(_pchannel, PIPE3);

                /* End */
                _pchannel->pipe_flag[PIPE3] = PIPE_DONE;
            }
        }
    }
    if( (Status & BITBEMP4) && (Int_enbl & BITBEMP4) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP4, ACC_16B_SHIFT, ACC_16B_MASK);
        _pchannel->generic_buffer = R_LIB_GetPid(_pchannel, PIPE4);
        if(PID_STALL == _pchannel->generic_buffer)
        {
            _pchannel->pipe_flag[PIPE4] = PIPE_STALL;
        } 
        else 
        {
            if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPE4CTR, USB_PIPEnCTR_1_5_INBUFM_SHIFT, USB_PIPEnCTR_1_5_INBUFM))
            {
                /* Disable BEMP Interrupt */
                R_LIB_DisableIntE(_pchannel, PIPE4);

                /* End */
                _pchannel->pipe_flag[PIPE4] = PIPE_DONE;
            }
        }
    }
    if( (Status & BITBEMP5) && (Int_enbl & BITBEMP5) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP5, ACC_16B_SHIFT, ACC_16B_MASK);
        _pchannel->generic_buffer = R_LIB_GetPid(_pchannel, PIPE5);
        if(PID_STALL == _pchannel->generic_buffer)
        {
            _pchannel->pipe_flag[PIPE5] = PIPE_STALL;
        } 
        else 
        {
            if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->PIPE5CTR, USB_PIPEnCTR_1_5_INBUFM_SHIFT, USB_PIPEnCTR_1_5_INBUFM))
            {
                /* Disable BEMP Interrupt */
                R_LIB_DisableIntE(_pchannel, PIPE5);

                /* End */
                _pchannel->pipe_flag[PIPE5] = PIPE_DONE;
            }
        }
    }
    if( (Status & BITBEMP6) && (Int_enbl & BITBEMP6) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP6, ACC_16B_SHIFT, ACC_16B_MASK);
        _pchannel->generic_buffer = R_LIB_GetPid(_pchannel, PIPE6);
        if(PID_STALL == _pchannel->generic_buffer)
        {
            _pchannel->pipe_flag[PIPE6] = PIPE_STALL;
        } 
        else {
            /* Disable BEMP Interrupt */
            R_LIB_DisableIntE(_pchannel, PIPE6);

            /* End */
            _pchannel->pipe_flag[PIPE6] = PIPE_DONE;
        }
    }
    if( (Status & BITBEMP7) && (Int_enbl & BITBEMP7) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP7, ACC_16B_SHIFT, ACC_16B_MASK);
        _pchannel->generic_buffer = R_LIB_GetPid(_pchannel, PIPE7);
        if(PID_STALL == _pchannel->generic_buffer)
        {
            _pchannel->pipe_flag[PIPE7] = PIPE_STALL;
        } 
        else 
        {
            /* Disable BEMP Interrupt */
            R_LIB_DisableIntE(_pchannel, PIPE7);

            /* End */
            _pchannel->pipe_flag[PIPE7] = PIPE_DONE;
        }
    }
    if( (Status & BITBEMP8) && (Int_enbl & BITBEMP8) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP8, ACC_16B_SHIFT, ACC_16B_MASK);
        _pchannel->generic_buffer = R_LIB_GetPid(_pchannel, PIPE8);
        if(PID_STALL == _pchannel->generic_buffer)
        {
            _pchannel->pipe_flag[PIPE8] = PIPE_STALL;
        } 
        else 
        {
            /* Disable BEMP Interrupt */
            R_LIB_DisableIntE(_pchannel, PIPE8);

            /* End */
            _pchannel->pipe_flag[PIPE8] = PIPE_DONE;
        }
    }
    if( (Status & BITBEMP9) && (Int_enbl & BITBEMP9) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP9, ACC_16B_SHIFT, ACC_16B_MASK);
        _pchannel->generic_buffer = R_LIB_GetPid(_pchannel, PIPE9);
        if(PID_STALL == _pchannel->generic_buffer)
        {
            _pchannel->pipe_flag[PIPE9] = PIPE_STALL;
        } 
        else 
        {
            /* Disable BEMP Interrupt */
            R_LIB_DisableIntE(_pchannel, PIPE9);

            /* End */
            _pchannel->pipe_flag[PIPE9] = PIPE_DONE;
        }
    }
}
/******************************************************************************
End of function bemp_int_pipe
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_IntrInt
* Description     : Configure INTR interrupt
* Argument        : uint16_t Status      ; BRDYSTS Register Value
*                   uint16_t Int_enbl    ; BRDYENB Register Value
* Return value    : None
******************************************************************************/
void R_LIB_IntrInt( volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl)
{

    DEBUG_MSG_HIGH(("\r\nLIBINT: Status: 0x%x Enable: 0x%x\r\n", Status, Int_enbl));

    if( (Status & BITBRDY0) && (Int_enbl & BITBRDY0) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~BITBRDY0, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  PIPE0, USB_CFIFOSEL_CURPIPE_SHIFT, USB_CFIFOSEL_CURPIPE);
        switch( R_USBF_DataioBufReadC(_pchannel, PIPE0) )
        {
            /* Continue of data read */
            case    READING:

            /* End of data read */
            case    READEND:

                /* PID = BUF */
                if (NULL != _pchannel->callbacks.p_cb_cout_mfpdone)
                {
                    _pchannel->callbacks.p_cb_cout_mfpdone((volatile void *)_pchannel, USB_ERR_OK,_pchannel->dtcnt[PIPE0]);
                }
                break;

            /* End of data read */
            case    READSHRT:                        
                R_LIB_DisableIntR(_pchannel, PIPE0);
                if (NULL != _pchannel->callbacks.p_cb_cout_mfpdone)
                {
                    _pchannel->callbacks.p_cb_cout_mfpdone((volatile void *)_pchannel, USB_ERR_OK,_pchannel->dtcnt[PIPE0]);
                }
                break;

            /* FIFO access error */
            case    READOVER:

                /* Buffer Clear */
                rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR,  BITBCLR, ACC_16B_SHIFT, ACC_16B_MASK);
                R_LIB_DisableIntR(_pchannel, PIPE0);

                /* Req Error */
                R_LIB_SetSTALL(_pchannel, PIPE0);
                break;

            /* FIFO access error */
            case    FIFOERROR:
                if(NULL != _pchannel->callbacks.p_cb_error)
                {
                    _pchannel->callbacks.p_cb_error(USB_ERR_CONTROL_OUT);
                }
                break;
            default:
                R_LIB_DisableIntR(_pchannel, PIPE0);

                /* Req Error */
                R_LIB_SetSTALL(_pchannel, PIPE0);
                break;
        }
    } 
    else 
    {
        intr_int_pipe(_pchannel, Status, Int_enbl);
    }
}
/******************************************************************************
End of function R_LIB_IntrInt
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_IntnInt
* Description     : Configure INTN interrupt
* Argument        : uint16_t Status      ; NRDYSTS Register Value
*                   uint16_t Int_enbl    ; NRDYENB Register Value
* Return value    : None
******************************************************************************/
void R_LIB_IntnInt(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl)
{
    if( (Status & BITNRDY0) && (Int_enbl & BITNRDY0) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->NRDYSTS,  (uint16_t)~BITNRDY0, ACC_16B_SHIFT, ACC_16B_MASK);
    } 
    else 
    {
        intn_int_pipe(_pchannel, Status, Int_enbl);
    }
}
/******************************************************************************
End of function R_LIB_IntnInt
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_BempInt
* Description     : Configure BEMP interrupt
* Argument        : uint16_t Status      ; BEMPSTS Register Value
*                   uint16_t Int_enbl    ; BEMPENB Register Value
* Return value    : None
******************************************************************************/
void R_LIB_BempInt(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl)
{

    if( (Status & BITBEMP0) && (Int_enbl & BITBEMP0) ) 
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~BITBEMP0, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  PIPE0, USB_CFIFOSEL_CURPIPE_SHIFT, USB_CFIFOSEL_CURPIPE);
        R_USBF_DataioBufWriteC(_pchannel, PIPE0);
    } 
    else 
    {
        bemp_int_pipe(_pchannel, Status, Int_enbl);
    }
}
/******************************************************************************
End of function R_LIB_BempInt
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_EnableIntR
* Description     : Enables interrupts
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_EnableIntR(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* Save Value */
    _pchannel->generic_temp  = rza_io_reg_read_16(&_pchannel->phwdevice->INTENB0, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,  
    (uint16_t)(_pchannel->generic_temp & (uint16_t)~((BITBEMPE| BITNRDYE)|BITBRDYE)), ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->BRDYENB, ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer |= g_util_BitSet[Pipe];
    rza_io_reg_write_16(&_pchannel->phwdevice->BRDYENB, _pchannel->generic_buffer, ACC_16B_SHIFT, ACC_16B_MASK);

    /* Interrupt Enable */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0, _pchannel->generic_temp, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function R_LIB_EnableIntR
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_DisableIntR
* Description     : Disables interrupts
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_DisableIntR(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* Save Value */
    _pchannel->generic_temp  = rza_io_reg_read_16(&_pchannel->phwdevice->INTENB0, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,
    (uint16_t)(_pchannel->generic_temp & (uint16_t)~((BITBEMPE| BITNRDYE)|BITBRDYE)), ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->BRDYENB, ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer &= (uint16_t)~(g_util_BitSet[Pipe]);
    rza_io_reg_write_16(&_pchannel->phwdevice->BRDYENB,  _pchannel->generic_buffer, ACC_16B_SHIFT, ACC_16B_MASK);

    /* Interrupt Disable */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0, _pchannel->generic_temp, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function R_LIB_DisableIntR
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_EnableIntE
* Description     : Enable BEMP
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_EnableIntE(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* Save Value */
    _pchannel->generic_temp  = rza_io_reg_read_16(&_pchannel->phwdevice->INTENB0, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,
    (uint16_t)(_pchannel->generic_temp & (uint16_t)~((BITBEMPE| BITNRDYE)|BITBRDYE)), ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->BEMPENB, ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer |= g_util_BitSet[Pipe];
    rza_io_reg_write_16(&_pchannel->phwdevice->BEMPENB, _pchannel->generic_buffer, ACC_16B_SHIFT, ACC_16B_MASK);

    /* Interrupt Enable */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0, _pchannel->generic_temp, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function R_LIB_EnableIntE
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_DisableIntE
* Description     : Disable BEMP
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_DisableIntE(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* Value Save */
    _pchannel->generic_temp  = rza_io_reg_read_16(&_pchannel->phwdevice->INTENB0, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,  
    (uint16_t)(_pchannel->generic_temp & (uint16_t)~((BITBEMPE| BITNRDYE)|BITBRDYE)), ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->BEMPENB, ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer &= (uint16_t)~(g_util_BitSet[Pipe]);
    rza_io_reg_write_16(&_pchannel->phwdevice->BEMPENB, _pchannel->generic_buffer, ACC_16B_SHIFT, ACC_16B_MASK);

    /* Interrupt Enable */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0, _pchannel->generic_temp, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function R_LIB_DisableIntE
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_EnableIntN
* Description     : Enable INTN
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_EnableIntN(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* Value Save */
    _pchannel->generic_temp  = rza_io_reg_read_16(&_pchannel->phwdevice->INTENB0, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,
    (uint16_t)(_pchannel->generic_temp & (uint16_t)~((BITBEMPE| BITNRDYE)|BITBRDYE)), ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->NRDYENB, ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer |= g_util_BitSet[Pipe];
    rza_io_reg_write_16(&_pchannel->phwdevice->NRDYENB, _pchannel->generic_buffer, ACC_16B_SHIFT, ACC_16B_MASK);

    /* Interrupt Enable */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,  _pchannel->generic_temp, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function R_LIB_EnableIntN
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_DisableIntN
* Description     : Disable INTN
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_DisableIntN(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* Value Save */
    _pchannel->generic_temp  = rza_io_reg_read_16(&_pchannel->phwdevice->INTENB0, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,
    (uint16_t)(_pchannel->generic_temp & (uint16_t)~((BITBEMPE| BITNRDYE)|BITBRDYE)), ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->NRDYENB, ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer &= (uint16_t)~(g_util_BitSet[Pipe]);
    rza_io_reg_write_16(&_pchannel->phwdevice->NRDYENB,  _pchannel->generic_buffer, ACC_16B_SHIFT, ACC_16B_MASK);

    /* Interrupt Enable */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,  _pchannel->generic_temp, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function R_LIB_DisableIntN
******************************************************************************/

/******************************************************************************
* Function Name   : is_hi_speed
* Description     : Check current speed
* Argument        : none
* Return value    : uint16_t          ; USB_YES : Hi-Speed
*                                     ; NO      : Full-Speed
* Return value    : None
******************************************************************************/
static uint16_t is_hi_speed(volatile st_usb_object_t *_pchannel)
{
    if((int16_t)HSMODE  == rza_io_reg_read_16(&_pchannel->phwdevice->DVSTCTR0, USB_DVSTCTR0_RHST_SHIFT, USB_DVSTCTR0_RHST))
    {
        /* Hi-Speed mode */
        return USB_YES;                                    
    }

    /* Full-Speed mode */
    return USB_NO;                                        
}
/******************************************************************************
End of function is_hi_speed
******************************************************************************/


/******************************************************************************
* Function Name   : get_bus_speed
* Description     : Get current bus speed
* Argument        : none
* Return value    : None
******************************************************************************/
uint16_t get_bus_speed(volatile st_usb_object_t *_pchannel)
{
    return rza_io_reg_read_16(&_pchannel->phwdevice->DVSTCTR0, USB_DVSTCTR0_RHST_SHIFT, USB_DVSTCTR0_RHST);
}
/******************************************************************************
End of function get_bus_speed
******************************************************************************/


///******************************************************************************
//* Function Name   : is_hi_speed_enable
//* Description     : Check Hi-Speed enable bit
//* Argument        : none
//* Return value    : uint16_t          ; USB_YES : Hi-Speed Enable
//*                                     ; NO      : Hi-Speed Disable
//******************************************************************************/
//static uint16_t is_hi_speed_enable(volatile st_usb_object_t *_pchannel)
//{
//    if(1  == rza_io_reg_read_16(&_pchannel->phwdevice->SYSCFG0, USB_SYSCFG_HSE_SHIFT, USB_SYSCFG_HSE))
//    {
//        /* Hi-Speed Enable */
//        return USB_YES;
//    }
//
//    /* Hi-Speed Disable */
//    return USB_NO;
//}
///******************************************************************************
//End of function is_hi_speed_enable
//******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_SetBUF
* Description     : Set pipe PID_BUF
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_SetBUF(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE0:
            rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  PID_BUF, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            break;
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE6:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE7:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE8:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE9:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_SetBUF
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_SetNAK
* Description     : Set pipe PID_NAK
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_SetNAK(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE0:
            rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  PID_NAK, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            break;
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE6:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE7:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE8:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE9:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_SetNAK
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_SetSTALL
* Description     : Set pipe PID_STALL
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_SetSTALL(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE0:
        rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  PID_STALL, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            break;
        case PIPE1:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  PID_STALL, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE2:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  PID_STALL, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE3:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  PID_STALL, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE4:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  PID_STALL, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE5:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  PID_STALL, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE6:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  PID_STALL, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE7:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  PID_STALL, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE8:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  PID_STALL, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE9:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  PID_STALL, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_SetSTALL
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_SetPipeNACKtoBUF
* Description     : Set pipe PID_STALL
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_SetPipeNACKtoBUF(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE0:
        rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  PID_BUF, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            break;
        case PIPE1:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE2:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE3:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE4:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE5:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE6:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE7:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE8:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE9:
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_SetPipeNACKtoBUF
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_SetPipeSTALLtoBUF
* Description     : Set pipe PID_STALL
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_SetPipeSTALLtoBUF(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE0:
            rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  PID_NAK, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  PID_BUF, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            break;
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE6:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE7:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE8:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE9:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  PID_BUF, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_SetPipeSTALLtoBUF
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_SetPipeBUFtoSTALL
* Description     : Set pipe PID_STALL
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_SetPipeBUFtoSTALL(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE0:
            rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  PID_STALL3, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            break;
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  PID_STALL3, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  PID_STALL3, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  PID_STALL3, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  PID_STALL3, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  PID_STALL3, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE6:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  PID_STALL3, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE7:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  PID_STALL3, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE8:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  PID_STALL3, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE9:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  PID_STALL3, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_SetPipeBUFtoSTALL
******************************************************************************/

/******************************************************************************
* Function Name   : R_LIB_SetTRN
* Description     : transaction  Count for PIpe
* Argument        : uint16_t Pipe          ; Pipe Number
* Argument        : uint16_t trncnt        ; transaction Count
* Return value    : None
******************************************************************************/
void R_LIB_SetTRN(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t trncnt)
{
    switch( Pipe ) 
    {
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1TRN,  trncnt, ACC_16B_SHIFT, ACC_16B_MASK);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2TRN,  trncnt, ACC_16B_SHIFT, ACC_16B_MASK);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3TRN,  trncnt, ACC_16B_SHIFT, ACC_16B_MASK);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4TRN,  trncnt, ACC_16B_SHIFT, ACC_16B_MASK);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5TRN,  trncnt, ACC_16B_SHIFT, ACC_16B_MASK);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_SetTRN
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_SetTRENB
* Description     : transaction Counter Clear for PIpe
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_SetTRENB(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1TRE,  1, USB_PIPEnTRE_TRENB_SHIFT, USB_PIPEnTRE_TRENB);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2TRE,  1, USB_PIPEnTRE_TRENB_SHIFT, USB_PIPEnTRE_TRENB);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3TRE,  1, USB_PIPEnTRE_TRENB_SHIFT, USB_PIPEnTRE_TRENB);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4TRE,  1, USB_PIPEnTRE_TRENB_SHIFT, USB_PIPEnTRE_TRENB);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5TRE,  1, USB_PIPEnTRE_TRENB_SHIFT, USB_PIPEnTRE_TRENB);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_SetTRENB
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_ClrTRCLR
* Description     : transaction Counter Clear for PIpe
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_ClrTRCLR(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1TRE,  1, USB_PIPEnTRE_TRCLR_SHIFT, USB_PIPEnTRE_TRCLR);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2TRE,  1, USB_PIPEnTRE_TRCLR_SHIFT, USB_PIPEnTRE_TRCLR);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3TRE,  1, USB_PIPEnTRE_TRCLR_SHIFT, USB_PIPEnTRE_TRCLR);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4TRE,  1, USB_PIPEnTRE_TRCLR_SHIFT, USB_PIPEnTRE_TRCLR);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5TRE,  1, USB_PIPEnTRE_TRCLR_SHIFT, USB_PIPEnTRE_TRCLR);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_ClrTRCLR
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_ClrSTALL
* Description     : Clear pipe PID_STALL
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_ClrSTALL(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE0:
            rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  PID_NAK, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            break;
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE6:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE7:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE8:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE9:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  PID_NAK, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_ClrSTALL
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_GetPid
* Description     : Get Pipe PID
* Argument        : uint16_t Pipe           ; Pipe Number
* Return value    : uint16_t                ; PID
******************************************************************************/
uint16_t R_LIB_GetPid(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE0:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->DCPCTR, USB_DCPCTR_PID_SHIFT, USB_DCPCTR_PID);
            break;
        case PIPE1:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->PIPE1CTR, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE2:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->PIPE2CTR, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE3:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->PIPE3CTR, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE4:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->PIPE4CTR, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE5:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->PIPE5CTR, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE6:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->PIPE6CTR, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE7:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->PIPE7CTR, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE8:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->PIPE8CTR, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        case PIPE9:
            _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->PIPE9CTR, USB_PIPEnCTR_1_5_PID_SHIFT, USB_PIPEnCTR_1_5_PID);
            break;
        default:
            _pchannel->generic_buffer = 0;
            break;
    }
    return    _pchannel->generic_buffer;
}
/******************************************************************************
End of function R_LIB_GetPid
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_DoSQCLR
* Description     : Do SQCLR
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_DoSQCLR(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    switch( Pipe ) 
    {
        case PIPE0:
            rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  1, USB_DCPCTR_SQCLR_SHIFT, USB_DCPCTR_SQCLR);
            break;
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  1, USB_PIPEnCTR_1_5_SQCLR_SHIFT, USB_PIPEnCTR_1_5_SQCLR);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  1, USB_PIPEnCTR_1_5_SQCLR_SHIFT, USB_PIPEnCTR_1_5_SQCLR);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  1, USB_PIPEnCTR_1_5_SQCLR_SHIFT, USB_PIPEnCTR_1_5_SQCLR);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  1, USB_PIPEnCTR_1_5_SQCLR_SHIFT, USB_PIPEnCTR_1_5_SQCLR);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  1, USB_PIPEnCTR_1_5_SQCLR_SHIFT, USB_PIPEnCTR_1_5_SQCLR);
            break;
        case PIPE6:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  1, USB_PIPEnCTR_1_5_SQCLR_SHIFT, USB_PIPEnCTR_1_5_SQCLR);
            break;
        case PIPE7:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  1, USB_PIPEnCTR_1_5_SQCLR_SHIFT, USB_PIPEnCTR_1_5_SQCLR);
            break;
        case PIPE8:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  1, USB_PIPEnCTR_1_5_SQCLR_SHIFT, USB_PIPEnCTR_1_5_SQCLR);
            break;
        case PIPE9:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  1, USB_PIPEnCTR_1_5_SQCLR_SHIFT, USB_PIPEnCTR_1_5_SQCLR);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_DoSQCLR
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_CD2SFIFO
* Description     : CFIFO buffer clear
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_CD2SFIFO(volatile st_usb_object_t *_pchannel, uint16_t pipe)
{
    rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,  pipe, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->PIPECFG,  0, USB_PIPECFG_DBLB_SHIFT, USB_PIPECFG_DBLB);
    
    if(is_hi_speed(_pchannel) == USB_YES)
    {
        /* wait 125us */
        delay_us(125);                    
    } 

    else 
    {
        /* wait 1ms */
        delay_ms(1);                    
    }

    R_USBF_DataioFPortChange2(_pchannel, pipe, CUSE, USB_NO);
}
/******************************************************************************
End of function R_LIB_CD2SFIFO
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_CS2DFIFO
* Description     : CFIFO set double buffer mode
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_CS2DFIFO(volatile st_usb_object_t *_pchannel, uint16_t pipe)
{
    rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,  pipe, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->PIPECFG,  1, USB_PIPECFG_DBLB_SHIFT, USB_PIPECFG_DBLB);
}
/******************************************************************************
End of function R_LIB_CS2DFIFO
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_CFIFOCLR
* Description     : FIFO clear buffer
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_CFIFOCLR(volatile st_usb_object_t *_pchannel, uint16_t pipe)
{
    if(USEPIPE == pipe)
    {
        R_USBF_DataioFPortChange2(_pchannel, PIPE0, D0USE, USB_NO);
        R_USBF_DataioFPortChange2(_pchannel, PIPE0, D1USE, USB_NO);
        for( _pchannel->generic_counter = PIPE1; _pchannel->generic_counter <= PIPE9; _pchannel->generic_counter++ )
        {
            R_LIB_CFIFOCLR2(_pchannel, _pchannel->generic_counter);
        }
    } 
    else {
        if((int16_t)pipe  == rza_io_reg_read_16(&_pchannel->phwdevice->D0FIFOSEL, USB_DnFIFOSEL_CURPIPE_SHIFT, USB_DnFIFOSEL_CURPIPE))
        {
            R_USBF_DataioFPortChange2(_pchannel, PIPE0, D0USE, USB_NO);
        }
        if((int16_t)pipe  == rza_io_reg_read_16(&_pchannel->phwdevice->D1FIFOSEL, USB_DnFIFOSEL_CURPIPE_SHIFT, USB_DnFIFOSEL_CURPIPE))
        {
            R_USBF_DataioFPortChange2(_pchannel, PIPE0, D1USE, USB_NO);
        }
        R_LIB_CFIFOCLR2(_pchannel, pipe);
    }
}
/******************************************************************************
End of function R_LIB_CFIFOCLR
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_CFIFOCLR2
* Description     : FIFO clear buffer
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_CFIFOCLR2(volatile st_usb_object_t *_pchannel, uint16_t pipe)
{
    R_USBF_DataioFPortChange2(_pchannel, pipe, CUSE, USB_NO);
    rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR,  BITBCLR, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR,  BITBCLR, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function R_LIB_CFIFOCLR2
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_SetACLRM
* Description     : Set Auto Buffer Clear Mode
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_SetACLRM(volatile st_usb_object_t *_pchannel, uint16_t pipe)
{
    switch( pipe ) 
    {
        case PIPE0:
            break;
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  1, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  1, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  1, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  1, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  1, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE6:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  1, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE7:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  1, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE8:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  1, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE9:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  1, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_SetACLRM
******************************************************************************/


/******************************************************************************
* Function Name   : R_LIB_ClrACLRM
* Description     : Clear Auto Buffer Clear Mode
* Argument        : uint16_t Pipe            ; Pipe Number
* Return value    : None
******************************************************************************/
void R_LIB_ClrACLRM(volatile st_usb_object_t *_pchannel, uint16_t pipe)
{
    switch( pipe ) 
    {
        case PIPE0:
            break;
        case PIPE1:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE1CTR,  0, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE2:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE2CTR,  0, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE3:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE3CTR,  0, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE4:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE4CTR,  0, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE5:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE5CTR,  0, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE6:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE6CTR,  0, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE7:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE7CTR,  0, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE8:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE8CTR,  0, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        case PIPE9:
            rza_io_reg_write_16(&_pchannel->phwdevice->PIPE9CTR,  0, USB_PIPEnCTR_1_5_ACLRM_SHIFT, USB_PIPEnCTR_1_5_ACLRM);
            break;
        default:
            break;
    }
}
/******************************************************************************
End of function R_LIB_ClrACLRM
******************************************************************************/
