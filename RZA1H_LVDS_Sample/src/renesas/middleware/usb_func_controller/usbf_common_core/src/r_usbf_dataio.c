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
* File Name       : R_USBF_Dataio.c
* Version         : 1.00
* Device          : RZA1H
* Tool Chain      : GNU 4.9-GNUARM-NONE
* H/W Platform    : RSK+RZA1H
* Description     : USB fifo read/write
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
/*    Following header file provides a structure of HAL Layer. */
#include "r_usb_hal.h"
/* Following header file provides common defines for widely used items. */
#include "usb_common.h"

#include "r_lib_int.h"
#include "r_intc.h"

#include "compiler_settings.h"
#include "rza_io_regrw.h"
#include "usb_iobitmask.h"


#include "trace.h"
/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
    #undef TRACE
    #define TRACE(x)
#endif

#define USB_DELAY_CONST (4)
#define R_USBF_NEEDS_BYTE_SELECT (R_OPTION_ENABLE)

#if R_USBF_NEEDS_BYTE_SELECT
/* Needed for access in USBf module */
typedef enum {
    L = 0, H = 1,
    LL= 0, LH = 1, HL = 2, HH = 3,
} e_usbf_byte_select_t;
#endif

/******************************************************************************
Global Variable
******************************************************************************/

/******************************************************************************
User Program Code
******************************************************************************/

#define FIFO_CHANGE_CHECK_TIMES   (20)    /* us wait */

/******************************************************************************
 * Function Name: usb_delay_1us
 * Description  : Wait (nearly) 1us by loop.
 * Arguments    : time : specify wait time (us order).
 * Return Value : none
 ******************************************************************************/
static void usb_delay_1us (uint16_t time)
{
    /* Expect CPU I clock is 400MHz or lesser */
    /* And this loop takes nearly 10cycle per loop */
    volatile uint32_t i = time;
    i *= 40;

    while (i > 0)
    {
        i--;
    }
}
/*****************************************************************************
 End of function  usb_delay_1us
 ******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioBufWriteC
* Description     : Buffer Write
* Argument        : uint16_t  Pipe    ; Pipe Number
* Return value    : uint16_t ;   READEND  :Read end
*                                READSHRT :short data
*                                READING  :Continue of data read
*                                READOVER :buffer over
*                                FIFOERROR:FIFO error
******************************************************************************/
uint16_t R_USBF_DataioBufWriteC(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    if(PIPE0 == Pipe)
    {
        _pchannel->generic_buffer    = R_USBF_DataioFPortChange1(_pchannel, Pipe,CUSE,BITISEL);
    }
    else
    {
        _pchannel->generic_buffer    = R_USBF_DataioFPortChange1(_pchannel, Pipe,CUSE,USB_NO);
    }

    /* Data buffer size */
    _pchannel->generic_temp    = getBufSize(_pchannel, Pipe);

    if(FIFOERROR == _pchannel->generic_buffer)
    {
        /* FIFO access error */
        return    (FIFOERROR);
    }

    /* Max Packet Size */
    _pchannel->generic_data    = getMaxPacketSize(_pchannel, Pipe);

    if( _pchannel->dtcnt[Pipe] <= (uint32_t)_pchannel->generic_temp)
    {
        /* write continues */
        _pchannel->endflag_k = WRITEEND;
        _pchannel->generic_counter = (uint16_t)_pchannel->dtcnt[Pipe];
        if (0 == _pchannel->generic_counter)
        {
            /* Null Packet is end of write */
            _pchannel->endflag_k = WRITESHRT;
        }
        if (( (_pchannel->generic_counter%_pchannel->generic_temp)!=0 ) ||
              (_pchannel->generic_counter ==  _pchannel->generic_data))
        {
            /* Short Packet is end of write */
            _pchannel->endflag_k = WRITESHRT;
        }
    }
    else
    {
        /* write continues */
        _pchannel->endflag_k = WRITING;
        _pchannel->generic_counter = _pchannel->generic_temp;
    }

    R_USBF_DataioCFifoWrite(_pchannel, Pipe,_pchannel->generic_counter);

    if( _pchannel->dtcnt[Pipe] < (uint32_t)_pchannel->generic_temp)
    {
        _pchannel->dtcnt[Pipe] = 0;
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->CFIFOCTR, USB_CFIFOCTR_BVAL_SHIFT, USB_CFIFOCTR_BVAL))
        {
            /* Short Packet */
            rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR, BITBVAL, ACC_16B_SHIFT, ACC_16B_MASK);
        }
    }
    else
    {
        _pchannel->dtcnt[Pipe] -= _pchannel->generic_counter;
    }

    /* End or Err or Continue */
    return    (_pchannel->endflag_k);
}
/******************************************************************************
End of function R_USBF_DataioBufWriteC
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioBufWrite
* Description     : Buffer Write
* Argument        : uint16_t  Pipe    ; Pipe Number
* Return value    : uint16_t ;   READEND  :Read end
*                                READSHRT :short data
*                                READING  :Continue of data read
*                                READOVER :buffer over
*                                FIFOERROR:FIFO error
******************************************************************************/
uint16_t R_USBF_DataioBufWrite(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* Ignore count clear */
    _pchannel->pipe_ignore[Pipe]    = 0;

    if( (_pchannel->pipe_tbl[Pipe] & FIFO_USE) == D0_FIFO_USE )
    {
        _pchannel->endflag_k    = R_USBF_DataioBufWriteD0(_pchannel, Pipe);
    }
    else
    {
        _pchannel->endflag_k    = R_USBF_DataioBufWriteC(_pchannel, Pipe);
    }

    switch( _pchannel->endflag_k )
    {
    /* Continue of data write */
    case    WRITING:

        /* Enable Ready Interrupt */
        R_LIB_EnableIntR(_pchannel, Pipe);
        break;

    /* End of data write */
    case    WRITEEND:

    /* End of data write ? */
        if((PIPE2 == Pipe) && (!_pchannel->config.bulk_in_no_short_packet))
        {
            /* Disable Empty Interrupt */
            R_LIB_EnableIntE(_pchannel, Pipe);
        }
        break;

    case    WRITESHRT:

        /* Disable Ready Interrupt */
        R_LIB_DisableIntR(_pchannel, Pipe);

        /* Enable Empty Interrupt */
        R_LIB_EnableIntE(_pchannel, Pipe);

        /*GIC bug */
        /* Clear BVAL */
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR, BITBVAL, ACC_16B_SHIFT, ACC_16B_MASK);

        if((PIPE2 == Pipe) && (_pchannel->config.bulk_in_no_short_packet))
        {
            /* Disable Empty Interrupt */
            R_LIB_DisableIntE(_pchannel, Pipe);
        }
        break;

    /* FIFO access error */
    case    FIFOERROR:
    default:

        /* Disable Ready Interrupt */
        R_LIB_DisableIntR(_pchannel, Pipe);


        /* Disable Empty Interrupt */
        R_LIB_DisableIntE(_pchannel, Pipe);
        _pchannel->pipe_flag[Pipe] = PIPE_IDLE;
        break;
    }

    /* End or Err or Continue */
    return    (_pchannel->endflag_k);
}
/******************************************************************************
End of function R_USBF_DataioBufWrite
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioBufWriteD0
* Description     : Buffer Write D0FIFIO
* Argument        : uint16_t  Pipe    ; Pipe Number
* Return value    : uint16_t ;   READEND  :Read end
*                                READSHRT :short data
*                                READING  :Continue of data read
*                                READOVER :buffer over
*                                FIFOERROR:FIFO error
* Return value    : None
******************************************************************************/
uint16_t R_USBF_DataioBufWriteD0(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* use to store Data Buffer size */
    uint16_t    size;

    /* use to store number of data packet to be write */
    uint16_t    count;

    /* use to point to correct buffer */
    uint16_t    buffer;

    /* use to store maximum packet size */
    uint16_t    mxps;

    buffer    = R_USBF_DataioFPortChange1(_pchannel, Pipe,D0USE,USB_NO);

    /* FIFO access error */
    if(FIFOERROR == buffer)
    {
        return    (FIFOERROR);
    }

    /* Data buffer size */
    size    = getBufSize(_pchannel, Pipe);

    /* Max Packet Size */
    mxps    = getMaxPacketSize(_pchannel, Pipe);

    if( _pchannel->dtcnt[Pipe] <= (uint32_t)size)
    {
        /* write continues */
        _pchannel->endflag_k = WRITEEND;
        count = (uint16_t)_pchannel->dtcnt[Pipe];
        if (0 == count)
        {
            /* Null Packet is end of write */
            _pchannel->endflag_k = WRITESHRT;
        }
        if (( (count%mxps)!=0 ) || (count ==  mxps))
        {
            /* Short Packet is end of write */
            _pchannel->endflag_k = WRITESHRT;
        }
    }
    else
    {
        /* write continues */
        _pchannel->endflag_k = WRITING;
        count = size;
    }

    R_USBF_DataioD0FifoWrite(_pchannel, Pipe,count);

    if( _pchannel->dtcnt[Pipe] < (uint32_t)size)
    {
        _pchannel->dtcnt[Pipe] = 0;
        if(0  == rza_io_reg_read_16(&_pchannel->phwdevice->D0FIFOCTR, USB_DnFIFOCTR_BVAL_SHIFT, USB_DnFIFOCTR_BVAL))
        {
            /* Short Packet */
            rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOCTR, BITBVAL, ACC_16B_SHIFT, ACC_16B_MASK);
        }
    }

    else
    {
        _pchannel->dtcnt[Pipe] -= count;
    }

    /* End or Err or Continue */
    return    (_pchannel->endflag_k);
}
/******************************************************************************
End of function R_USBF_DataioBufWriteD0
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioSendStart
* Description     : Send Data start
* Argument        : uint16_t  Pipe    ; Pipe Number
*                   uint32_t Bsize    ; Data Size
*                   uint8_t  *Table   ; Data Table Address
* Return value    : uint16_t ;   READEND  :Read end
*                                READSHRT :short data
*                                READING  :Continue of data read
*                                READOVER :buffer over
*                                FIFOERROR:FIFO error
******************************************************************************/
uint16_t R_USBF_DataioSendStart(volatile st_usb_object_t *_pchannel,
                    uint16_t Pipe,
                    uint32_t Bsize,
                    uint8_t *Table)
{
    _pchannel->dtcnt[Pipe] = Bsize;
    _pchannel->p_dtptr[Pipe] = (uint8_t *)Table;

    /* Ignore count clear */
    _pchannel->pipe_ignore[Pipe]    = 0;
    _pchannel->pipe_flag[Pipe] = PIPE_WAIT;

    rza_io_reg_write_16(&_pchannel->phwdevice->BEMPSTS,  (uint16_t)~(g_util_BitSet[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->BRDYSTS,  (uint16_t)~(g_util_BitSet[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->endflag_k = R_USBF_DataioBufWrite(_pchannel, Pipe);
    if(FIFOERROR != _pchannel->endflag_k)
    {
        /* Set BUF */
        R_LIB_SetBUF(_pchannel, Pipe);
    }

    /* End or Err or Continue */
    return    (_pchannel->endflag_k);
}
/******************************************************************************
End of function R_USBF_DataioSendStart
******************************************************************************/


/******************************************************************************
* Function Name   : R_USBF_DataioReceiveStart
* Description     : Receive Data start
* Argument        : uint16_t  Pipe    ; Pipe Number
*                   uint32_t Bsize    ; Data Size
*                   uint8_t  *Table   ; Data Table Address
* Return value    : None
******************************************************************************/
void R_USBF_DataioReceiveStart(volatile st_usb_object_t *_pchannel,
                   uint16_t  _pipe,
                   uint32_t  _bsize,
                   uint8_t * _ptable)
{
    DEBUG_MSG_HIGH(("DATAIO: R_USBF_DataioReceiveStart\r\n"));
    if(D0_FIFO_USE == (_pchannel->pipe_tbl[_pipe] & (FIFO_USE)))
    {
        R_USBF_DataioReceiveStartD0(_pchannel, _pipe, _bsize, _ptable);
    }
    else
    {
        R_USBF_DataioReceiveStartC (_pchannel, _pipe, _bsize, _ptable);
    }
}
/******************************************************************************
End of function R_USBF_DataioReceiveStart
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioReceiveStartC
* Description     : Receive Data start CFIFO
* Argument        : uint16_t  Pipe    ; Pipe Number
*                   uint32_t Bsize    ; Data Size
*                   uint8_t  *Table   ; Data Table Address
* Return value    : None
******************************************************************************/
void R_USBF_DataioReceiveStartC(volatile st_usb_object_t *_pchannel,
                     uint16_t Pipe,
                     uint32_t Bsize,
                     uint8_t *Table)
{
    DEBUG_MSG_HIGH(("DATAIO: R_USBF_DataioReceiveStartC: Pipe : %d\r\n", Pipe));

    /* Set NAK */
    R_LIB_SetNAK(_pchannel, Pipe);
    _pchannel->dtcnt[Pipe] = Bsize;
    _pchannel->p_dtptr[Pipe] = (uint8_t *)Table;

    /* Ignore count clear */
    _pchannel->pipe_ignore[Pipe]    = 0;

    _pchannel->rdcnt[Pipe] = Bsize;
    _pchannel->pipe_data_size[Pipe] = Bsize;
    _pchannel->pipe_flag[Pipe] = PIPE_WAIT;

    /* Set BUF */
    R_LIB_SetBUF(_pchannel, Pipe);

    /* Enable Ready Interrupt */
    R_LIB_EnableIntR(_pchannel, Pipe);
}
/******************************************************************************
End of function R_USBF_DataioReceiveStartC
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioReceiveStartD0
* Description     : Receive Data start D0FIFO
* Argument        : uint16_t  Pipe    ; Pipe Number
*                   uint32_t Bsize    ; Data Size
*                   uint8_t  *Table   ; Data Table Address
* Return value    : None
******************************************************************************/
void R_USBF_DataioReceiveStartD0(volatile st_usb_object_t *_pchannel,
                      uint16_t Pipe,
                      uint32_t Bsize,
                      uint8_t *Table)
{
    uint16_t trncnt;
    uint16_t mxps;

    /* Set NAK */
    R_LIB_SetNAK(_pchannel, Pipe);
    _pchannel->dtcnt[Pipe] = Bsize;
    _pchannel->p_dtptr[Pipe] = (uint8_t *)Table;
    _pchannel->rdcnt[Pipe] = Bsize;
    _pchannel->pipe_data_size[Pipe] = Bsize;
    _pchannel->pipe_flag[Pipe] = PIPE_WAIT;

    /* Ignore count clear */
    _pchannel->pipe_ignore[Pipe]    = 0;

    R_USBF_DataioFPortChange2(_pchannel, Pipe, D0USE, USB_NO);

    /* Max Packet Size */
    mxps = getMaxPacketSize(_pchannel, Pipe);
    trncnt = (uint16_t)(Bsize / mxps);
    if( Bsize % mxps ) {
    	trncnt++;
    }
    R_LIB_ClrTRCLR(_pchannel, Pipe);
    R_LIB_SetTRENB(_pchannel, Pipe);
    R_LIB_SetTRN(_pchannel, Pipe,trncnt);

    /* Set BUF */
    R_LIB_SetBUF(_pchannel, Pipe);

    /* Enable Ready Interrupt */
    R_LIB_EnableIntR(_pchannel, Pipe);
}
/******************************************************************************
End of function R_USBF_DataioReceiveStartD0
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioBufRead
* Description     : Buffer Read
* Argument        : uint16_t  Pipe        ; Pipe Number
* Return value    : uint16_t ;   READEND  :Read end
*                                READSHRT :short data
*                                READING  :Continue of data read
*                                READOVER :buffer over
*                                FIFOERROR:FIFO error
******************************************************************************/
uint16_t R_USBF_DataioBufRead(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* Ignore count clear */
    _pchannel->pipe_ignore[Pipe]    = 0;
    if( (_pchannel->pipe_tbl[Pipe] & FIFO_USE) == D0_FIFO_USE )
    {
        _pchannel->endflag_k    = R_USBF_DataioBufReadD0(_pchannel, Pipe);
    }
    else
    {
        _pchannel->endflag_k    = R_USBF_DataioBufReadC(_pchannel, Pipe);
    }
    switch( _pchannel->endflag_k )
    {
        /* Continue of data read */
        case    READING:
            R_LIB_EnableIntR(_pchannel, Pipe);
            break;

        /* End of data read */
        case    READEND:

        /* End of data read */
        case    READSHRT:
            R_LIB_DisableIntR(_pchannel, Pipe);

            /*GIC bug */
            /* Clear BVAL */
            rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR, BITBVAL, ACC_16B_SHIFT, ACC_16B_MASK);

            _pchannel->pipe_flag[Pipe] = PIPE_IDLE;
            _pchannel->pipe_data_size[Pipe] -= _pchannel->dtcnt[Pipe];

            /* Data Count */
            _pchannel->rdcnt[Pipe] -= _pchannel->dtcnt[Pipe];
            break;

        /* buffer over */
        case    READOVER:

            /* Clear BVAL */
            rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR,  BITBCLR, ACC_16B_SHIFT, ACC_16B_MASK);

            /* Disable Ready Interrupt */
            R_LIB_DisableIntR(_pchannel, Pipe);
            _pchannel->pipe_flag[Pipe] = PIPE_IDLE;
            _pchannel->pipe_data_size[Pipe] -= _pchannel->dtcnt[Pipe];

            /* Data Count */
            _pchannel->rdcnt[Pipe] -= _pchannel->dtcnt[Pipe];
            break;

        /* FIFO access error */
        case    FIFOERROR:
        default:

            /* Disable Ready Interrupt */
            R_LIB_DisableIntR(_pchannel, Pipe);
            _pchannel->pipe_flag[Pipe] = PIPE_IDLE;
            break;
    }

    /* End or Err or Continue */
    return    (_pchannel->endflag_k);
}
/******************************************************************************
End of function R_USBF_DataioBufRead
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioBufReadC
* Description     : Buffer Read CFIFO
* Argument        : uint16_t  Pipe        ; Pipe Number
* Return value    : uint16_t   ; READEND  :Read end
*                              ; READSHRT :short data
*                              ; READING  :Continue of data read
*                              ; READOVER :buffer over
*                              ; FIFOERROR:FIFO error
******************************************************************************/
uint16_t R_USBF_DataioBufReadC(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* use to store number of data packet to be write */
    uint16_t    count;

    /* use to point to correct buffer */
    uint16_t    buffer;

    /* use to store maximum packet size */
    uint16_t    mxps;

    /* use to store maximum packet size */
    uint16_t    dtln;

    buffer    = R_USBF_DataioFPortChange1(_pchannel, Pipe,CUSE,USB_NO);
    if(FIFOERROR == buffer)
    {
        /* FIFO access error */
        return    (FIFOERROR);
    }
    dtln    = (uint16_t)(buffer & BITDTLN);

    /* Max Packet Size */
    mxps    = getMaxPacketSize(_pchannel, Pipe);

    if( _pchannel->dtcnt[Pipe] < dtln )
    {
        /* Buffer Over ? */
        _pchannel->endflag_k = READOVER;
        count = (uint16_t)_pchannel->dtcnt[Pipe];
    }
    else if( _pchannel->dtcnt[Pipe] == dtln )
    {
        /* just Receive Size */
        _pchannel->endflag_k = READEND;
        count = dtln;
        if (0 == count)
        {
            /* Null Packet received */
            _pchannel->endflag_k = READSHRT;
        }
        if ( (count%mxps)!=0 )
        {
            /* Short Packet received */
            _pchannel->endflag_k = READSHRT;
        }
    }

    /* continuous data received */
    else
    {
        _pchannel->endflag_k = READING;
        count = dtln;
        if (0 == count)
        {
            /* Null Packet receive */
            _pchannel->endflag_k = READSHRT;
        }
        if ( (count%mxps)!=0 )
        {
            /* Short Packet receive */
            _pchannel->endflag_k = READSHRT;
        }
    }

    /* 0 length packet */
    if(0 == count)
    {
        /* Clear BVAL */
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR,  BITBCLR, ACC_16B_SHIFT, ACC_16B_MASK);
    }
    else
    {
        R_USBF_DataioCFifoRead(_pchannel, Pipe,count);
    }

    _pchannel->dtcnt[Pipe] -= count;

    /* End or Err or Continue */
    return    (_pchannel->endflag_k);
}
/******************************************************************************
End of function R_USBF_DataioBufReadC
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioBufReadD0
* Description     : Buffer Read D0FIFO
* Argument        : uint16_t  Pipe        ; Pipe Number
* Return value    : uint16_t   ; READEND  :Read end
*                              ; READSHRT :short data
*                              ; READING  :Continue of data read
*                              ; READOVER :buffer over
*                              ; FIFOERROR:FIFO error
* Return value    : None
******************************************************************************/
uint16_t R_USBF_DataioBufReadD0(volatile st_usb_object_t *_pchannel, uint16_t Pipe)
{
    /* use to store number of data packet to be write */
    uint16_t    count;

    /* use to point to correct buffer */
    uint16_t    buffer;

    /* use to store maximum packet size */
    uint16_t    mxps;

    /* use to store maximum packet size */
    uint16_t    dtln;

    buffer    = R_USBF_DataioFPortChange1(_pchannel, Pipe,D0USE,USB_NO);

    /* FIFO access error */
    if(FIFOERROR == buffer)
    {
        return    (FIFOERROR);
    }
    dtln    = (uint16_t)(buffer & BITDTLN);

    /* Max Packet Size */
    mxps    = getMaxPacketSize(_pchannel, Pipe);

    if( _pchannel->dtcnt[Pipe] < dtln )
    {
        /* Buffer Over ? */
        _pchannel->endflag_k = READOVER;
        count = (uint16_t)_pchannel->dtcnt[Pipe];
    }

    /* just Receive Size */
    else if( _pchannel->dtcnt[Pipe] == dtln )
    {
        _pchannel->endflag_k = READEND;
        count = dtln;
        if (0 == count)
        {
            /* Null Packet receive */
            _pchannel->endflag_k = READSHRT;
        }
        if ( (count%mxps)!=0 )
        {
            /* Short Packet receive */
            _pchannel->endflag_k = READSHRT;
        }
    }

    /* continuous Receive data */
    else
    {
        _pchannel->endflag_k = READING;
        count = dtln;
        if (0 == count)
        {
            /* Null Packet receive */
            _pchannel->endflag_k = READSHRT;
        }
        if ( (count%mxps)!=0 )
        {
            /* Short Packet receive */
            _pchannel->endflag_k = READSHRT;
        }
    }

    /* 0 length packet */
    if(0 == count)
    {
        /* Clear BVAL */
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR,  BITBCLR, ACC_16B_SHIFT, ACC_16B_MASK);
    }
    else
    {
        R_USBF_DataioD0FifoRead(_pchannel, Pipe,count);
    }

    _pchannel->dtcnt[Pipe] -= count;

    /* End or Err or Continue */
    return    (_pchannel->endflag_k);
}
/******************************************************************************
End of function R_USBF_DataioBufReadD0
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioFPortChange1
* Description     : Change pipe
* Argument        : uint16_t  Pipe    ; Pipe Number
*                   uint16_t fifosel  ; Select FIFO
*                   uint16_t isel     ; FIFO Access Direction
* Return value    : uint16_t          ; CFIFOCTR/D0FIFOCTR/D1FIFOCTR Reg Value
*                                     ; FIFOERROR(Error)
******************************************************************************/
uint16_t R_USBF_DataioFPortChange1(volatile st_usb_object_t *_pchannel, uint16_t Pipe, uint16_t fifosel, uint16_t isel)
{
    uint16_t buf;
    volatile uint16_t    i;
    uint16_t    loop;

    switch(fifosel)
    {
        case    CUSE:
            _pchannel->generic_buffer2  = rza_io_reg_read_16(&_pchannel->phwdevice->CFIFOSEL, ACC_16B_SHIFT, ACC_16B_MASK);
            _pchannel->generic_buffer2 &= (uint16_t)~(BITISEL | BITCURPIPE);
            _pchannel->generic_buffer2 |= (uint16_t)(isel | Pipe);
            rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL, _pchannel->generic_buffer2, ACC_16B_SHIFT, ACC_16B_MASK);
            for ( loop = 0; loop < FIFO_CHANGE_CHECK_TIMES; loop++ ) {
                buf = rza_io_reg_read_16(&_pchannel->phwdevice->CFIFOSEL, ACC_16B_SHIFT, ACC_16B_MASK);
                if ( (buf & (uint16_t)(BITISEL | BITCURPIPE)) == (uint16_t)(isel | Pipe) ) {
                    /* Channel selection of FIFO is completed */
                    break;
                }
                usb_delay_1us(1);
            }
            if ( loop >= FIFO_CHANGE_CHECK_TIMES ) {
                return( FIFOERROR );
            }
            break;
        case    D0USE:
        case    D0DMA:
            rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  Pipe, USB_DnFIFOSEL_CURPIPE_SHIFT, USB_DnFIFOSEL_CURPIPE);
            for ( loop = 0; loop < FIFO_CHANGE_CHECK_TIMES; loop++ ) {
                buf = rza_io_reg_read_16(&_pchannel->phwdevice->D0FIFOSEL, ACC_16B_SHIFT, ACC_16B_MASK);
                if ( (buf & (uint16_t)(BITCURPIPE)) == Pipe ) {
                    /* Channel selection of FIFO is completed */
                    break;
                }
                usb_delay_1us(1);
            }
            if ( loop >= FIFO_CHANGE_CHECK_TIMES ) {
                return( FIFOERROR );
            }
            break;
        case    D1USE:
        case    D1DMA:
            rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  Pipe, USB_DnFIFOSEL_CURPIPE_SHIFT, USB_DnFIFOSEL_CURPIPE);
            for ( loop = 0; loop < FIFO_CHANGE_CHECK_TIMES; loop++ ) {
                buf = rza_io_reg_read_16(&_pchannel->phwdevice->D1FIFOSEL, ACC_16B_SHIFT, ACC_16B_MASK);
                if ( (buf & (uint16_t)(BITCURPIPE)) == Pipe ) {
                    /* Channel selection of FIFO is completed */
                    break;
                }
                usb_delay_1us(1);
            }
            if ( loop >= FIFO_CHANGE_CHECK_TIMES ) {
                return( FIFOERROR );
            }
            break;
        default:
            break;
    }

    /* Cautions !!!
     * Depending on the external bus speed of CPU, you may need to wait for 100ns here.
     * For details, please look at the data sheet.     */

    usb_delay_1us(1);

    for(i=0;i<USB_DELAY_CONST;i++)
    {
        switch(fifosel)
        {
            case    CUSE:
                _pchannel->generic_buffer2  = rza_io_reg_read_16(&_pchannel->phwdevice->CFIFOCTR, ACC_16B_SHIFT, ACC_16B_MASK);
                break;

            case    D0USE:
            case    D0DMA:
                _pchannel->generic_buffer2  = rza_io_reg_read_16(&_pchannel->phwdevice->D0FIFOCTR, ACC_16B_SHIFT, ACC_16B_MASK);
                break;

            case    D1USE:
            case    D1DMA:
                _pchannel->generic_buffer2  = rza_io_reg_read_16(&_pchannel->phwdevice->D1FIFOCTR, ACC_16B_SHIFT, ACC_16B_MASK);
                break;

            default:
                _pchannel->generic_buffer2    = 0;
                break;
        }

        if((uint16_t)(_pchannel->generic_buffer2 & BITFRDY) == BITFRDY)
        {
            return    (_pchannel->generic_buffer2);
        }

        /* Cautions !!!
         * Depending on the external bus speed of CPU, you may need to wait for 100ns here.
         * For details, please look at the data sheet.     */
        usb_delay_1us(1);
    }
    return    (FIFOERROR);
}
/******************************************************************************
End of function R_USBF_DataioFPortChange1
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioFPortChange2
* Description     : Change pipe
* Argument        : uint16_t Pipe      ; Pipe Number
*                   uint16_t fifosel   ; Select FIFO
*                   uint16_t isel      ; FIFO Access Direction
* Return value    : None
******************************************************************************/
void R_USBF_DataioFPortChange2(volatile st_usb_object_t *_pchannel, uint16_t Pipe, uint16_t fifosel, uint16_t isel)
{
    uint16_t buf, loop;

    switch(fifosel)
    {
        case    CUSE:
            _pchannel->generic_buffer2  = rza_io_reg_read_16(&_pchannel->phwdevice->CFIFOSEL, ACC_16B_SHIFT, ACC_16B_MASK);
            _pchannel->generic_buffer2 &= (uint16_t)~(BITISEL | BITCURPIPE);
            _pchannel->generic_buffer2 |= (uint16_t)(isel | Pipe);

            /* ISEL=1, CURPIPE=0 */
            rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  _pchannel->generic_buffer2, ACC_16B_SHIFT, ACC_16B_MASK);
            for ( loop = 0; loop < FIFO_CHANGE_CHECK_TIMES; loop++ ) {
                buf = rza_io_reg_read_16(&_pchannel->phwdevice->CFIFOSEL, ACC_16B_SHIFT, ACC_16B_MASK);
                if ( (buf & (uint16_t)(BITISEL | BITCURPIPE)) == (uint16_t)(isel | Pipe) ) {
                    /* Channel selection of FIFO is completed */
                    break;
                }
                usb_delay_1us(1);
            }
            if ( loop >= FIFO_CHANGE_CHECK_TIMES ) {
                while(1);
            }
            break;

        case    D0USE:
        case    D0DMA:
            rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  Pipe, USB_DnFIFOSEL_CURPIPE_SHIFT, USB_DnFIFOSEL_CURPIPE);
            for ( loop = 0; loop < FIFO_CHANGE_CHECK_TIMES; loop++ ) {
                buf = rza_io_reg_read_16(&_pchannel->phwdevice->D0FIFOSEL, ACC_16B_SHIFT, ACC_16B_MASK);
                if ( (buf & (uint16_t)(BITCURPIPE)) == Pipe ) {
                    /* Channel selection of FIFO is completed */
                    break;
                }
                usb_delay_1us(1);
            }
            if ( loop >= FIFO_CHANGE_CHECK_TIMES ) {
                while(1);
            }
            break;

        case    D1USE:
        case    D1DMA:
            rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  Pipe, USB_DnFIFOSEL_CURPIPE_SHIFT, USB_DnFIFOSEL_CURPIPE);
            for ( loop = 0; loop < FIFO_CHANGE_CHECK_TIMES; loop++ ) {
                buf = rza_io_reg_read_16(&_pchannel->phwdevice->D1FIFOSEL, ACC_16B_SHIFT, ACC_16B_MASK);
                if ( (buf & (uint16_t)(BITCURPIPE)) == Pipe ) {
                    /* Channel selection of FIFO is completed */
                    break;
                }
                usb_delay_1us(1);
            }
            if ( loop >= FIFO_CHANGE_CHECK_TIMES ) {
                while(1);
            }
            break;

        default:
            break;
    }

    /* Cautions !!!
     * Depending on the external bus speed of CPU, you may need to wait for 450ns here.
     * For details, please look at the data sheet.     */
    usb_delay_1us(1);
}
/******************************************************************************
End of function R_USBF_DataioFPortChange2
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioCFifoWrite
* Description     : CFIFO write
* Argument        : uint16_t Pipe      ; Pipe Number
*                   uint16_t count     ; Data Size(Byte)
* Return value    : none
* Return value    : None
******************************************************************************/
void R_USBF_DataioCFifoWrite(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count)
{
    TRACE(("R_USBF_DataioCFifoWrite [%d Bytes]", count));

    if(NULL == _pchannel->p_dtptr[Pipe])
    {
        /* Null pointer is passed as pointer to dummy buffer to discard data */
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  MBW_32, USB_CFIFOSEL_MBW_SHIFT, USB_CFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_32(&_pchannel->phwdevice->CFIFO.UINT32,  *((uint32_t *)_pchannel->p_dtptr[Pipe]), ACC_32B_SHIFT, ACC_32B_MASK);
            TRACE(("[0x%08x]",*((uint32_t *)_pchannel->p_dtptr[Pipe])));
        }
        if( (count % 2) != 0 )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->CFIFO.UINT8[HH],  *((uint8_t *)_pchannel->p_dtptr[Pipe]), ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%02x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
        }
        else if( (count % 4) != 0 )
        {
            rza_io_reg_write_16(&_pchannel->phwdevice->CFIFO.UINT16[H],  *((uint16_t *)_pchannel->p_dtptr[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
            TRACE(("[0x%04x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    else if((((uint32_t)_pchannel->p_dtptr[Pipe]) & 0x00000001) != 0)
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  MBW_8, USB_CFIFOSEL_MBW_SHIFT, USB_CFIFOSEL_MBW);
        for( _pchannel->generic_even = count; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->CFIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%02x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else if(((((uint32_t)_pchannel->p_dtptr[Pipe]) & 0x00000003) != 0) || ((count % 4) == 3))
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  MBW_16, USB_CFIFOSEL_MBW_SHIFT, USB_CFIFOSEL_MBW);
        for( _pchannel->generic_even = count/2; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_16(&_pchannel->phwdevice->CFIFO.UINT16[H],  *((uint16_t *)_pchannel->p_dtptr[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
            TRACE(("[0x%04x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 2;
        }
        if( (count % 2) != 0 )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->CFIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%02x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  MBW_32, USB_CFIFOSEL_MBW_SHIFT, USB_CFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_32(&_pchannel->phwdevice->CFIFO.UINT32,  *((uint32_t *)_pchannel->p_dtptr[Pipe]), ACC_32B_SHIFT, ACC_32B_MASK);
            TRACE(("[0x%08x]",*((uint32_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 4;
        }

        if( (count % 2) != 0 )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->CFIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%02x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 1;
        }
        else if( (count % 4) != 0 )
        {
            rza_io_reg_write_16(&_pchannel->phwdevice->CFIFO.UINT16[H],  *((uint16_t *)_pchannel->p_dtptr[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
            TRACE(("[0x%04x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 2;
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    TRACE((" COMPLETE\r\n"));

}
/******************************************************************************
End of function R_USBF_DataioCFifoWrite
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioCFifoRead
* Description     : CFIFO read
* Argument        : uint16_t Pipe      ; Pipe Number
*                   uint16_t count     ; Data Size(Byte)
* Return value    : None
******************************************************************************/
void R_USBF_DataioCFifoRead(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count)
{
    TRACE(("R_USBF_DataioCFifoRead [%d Bytes]", count));

    if(NULL == _pchannel->p_dtptr[Pipe])
    {
        /* Null pointer is passed as pointer to dummy buffer to discard data */
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  MBW_32, USB_CFIFOSEL_MBW_SHIFT, USB_CFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *((uint32_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_32(&_pchannel->phwdevice->CFIFO.UINT32, ACC_32B_SHIFT, ACC_32B_MASK);
            TRACE(("[0x%2x]",*((uint32_t *)_pchannel->p_dtptr[Pipe])));

        }
        if( (count % 2) != 0 )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->CFIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%2x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
        }
        else if( (count % 4) != 0 )
        {
            *((uint16_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_16(&_pchannel->phwdevice->CFIFO.UINT16[H], ACC_16B_SHIFT, ACC_16B_MASK);
            TRACE(("[0x%2x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    else if(((uint32_t)_pchannel->p_dtptr[Pipe] & 0x00000001) != 0)
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  MBW_8, USB_CFIFOSEL_MBW_SHIFT, USB_CFIFOSEL_MBW);
        for( _pchannel->generic_even = count; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->CFIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%2x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else if(((((uint32_t)_pchannel->p_dtptr[Pipe]) & 0x00000003) != 0) || ((count % 4) == 3))
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  MBW_16, USB_CFIFOSEL_MBW_SHIFT, USB_CFIFOSEL_MBW);
        for( _pchannel->generic_even = count/2; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *((uint16_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_16(&_pchannel->phwdevice->CFIFO.UINT16[H], ACC_16B_SHIFT, ACC_16B_MASK);
            TRACE(("[0x%2x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 2;
        }
        if( (count % 2) != 0 )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->CFIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%2x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  MBW_32, USB_CFIFOSEL_MBW_SHIFT, USB_CFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *((uint32_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_32(&_pchannel->phwdevice->CFIFO.UINT32, ACC_32B_SHIFT, ACC_32B_MASK);
            TRACE(("[0x%2x]",*((uint32_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 4;
        }
        if( (count % 2) != 0 )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->CFIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
        else if( (count % 4) != 0 )
        {
            *((uint16_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_16(&_pchannel->phwdevice->CFIFO.UINT16[H], ACC_16B_SHIFT, ACC_16B_MASK);
            TRACE(("[0x%2x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 2;
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    _pchannel->p_dtptr[Pipe];
    TRACE((" COMPLETE\r\n"));
}
/******************************************************************************
End of function R_USBF_DataioCFifoRead
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioD0FifoWrite
* Description     : D0FIFO write
* Argument        : uint16_t Pipe      ; Pipe Number
*                   uint16_t count     ; Data Size(Byte)
* Return value    : None
******************************************************************************/
void R_USBF_DataioD0FifoWrite(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count)
{
    TRACE(("R_USBF_DataioD0FifoWrite [%d Bytes]", count));

    if(NULL == _pchannel->p_dtptr[Pipe])
    {
        /* Null pointer is passed as pointer to dummy buffer to discard data */
        rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  MBW_32, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_32(&_pchannel->phwdevice->D0FIFO.UINT32,  *((uint32_t *)_pchannel->p_dtptr[Pipe]), ACC_32B_SHIFT, ACC_32B_MASK);
            TRACE(("[0x%2x]",*((uint32_t *)_pchannel->p_dtptr[Pipe])));
        }
        if( (count % 2) != 0 )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->D0FIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%2x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
        }
        else if( (count % 4) != 0 )
        {
            rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFO.UINT16[H],  *((uint16_t *)_pchannel->p_dtptr[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
            TRACE(("[0x%2x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    else if((((uint32_t)_pchannel->p_dtptr[Pipe]) & 0x00000001) != 0)
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  MBW_8, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->D0FIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%2x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else if(((((uint32_t)_pchannel->p_dtptr[Pipe]) & 0x00000003) != 0) || ((count % 4) == 3))
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  MBW_16, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/2; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFO.UINT16[H],  *((uint16_t *)_pchannel->p_dtptr[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
            TRACE(("[0x%2x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 2;
        }
        if( (count % 2) != 0 )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->D0FIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%2x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  MBW_32, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_32(&_pchannel->phwdevice->D0FIFO.UINT32,  *((uint32_t *)_pchannel->p_dtptr[Pipe]), ACC_32B_SHIFT, ACC_32B_MASK);
            TRACE(("[0x%2x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 4;
        }
        if( (count % 2) != 0 )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->D0FIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            TRACE(("[0x%2x]",*((uint8_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 1;
        }
        else if( (count % 4) != 0 )
        {
            rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFO.UINT16[H],  *((uint16_t *)_pchannel->p_dtptr[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
            TRACE(("[0x%2x]",*((uint16_t *)_pchannel->p_dtptr[Pipe])));
            _pchannel->p_dtptr[Pipe] += 2;
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    TRACE((" COMPLETE\r\n"));
}
/******************************************************************************
End of function R_USBF_DataioD0FifoWrite
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioD0FifoRead
* Description     : D0FIFO read
* Argument        : uint16_t Pipe      ; Pipe Number
*                   uint16_t count     ; Data Size(Byte)
* Return value    : None
******************************************************************************/
void R_USBF_DataioD0FifoRead(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count)
{
    if(NULL == _pchannel->p_dtptr[Pipe])
    {
        /* Null pointer is passed as pointer to dummy buffer to discard data */
        rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  MBW_32, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *((uint32_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_32(&_pchannel->phwdevice->D0FIFO.UINT32, ACC_32B_SHIFT, ACC_32B_MASK);
        }
        if( (count % 2) != 0 )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->D0FIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
        }
        else if( (count % 4) != 0 )
        {
            *((uint16_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_16(&_pchannel->phwdevice->D0FIFO.UINT16[H], ACC_16B_SHIFT, ACC_16B_MASK);
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    else if(((uint32_t)_pchannel->p_dtptr[Pipe] & 0x00000001) != 0)
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  MBW_8, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->D0FIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else if(((((uint32_t)_pchannel->p_dtptr[Pipe]) & 0x00000003) != 0) || ((count % 4) == 3))
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  MBW_16, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/2; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *((uint16_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_16(&_pchannel->phwdevice->D0FIFO.UINT16[H], ACC_16B_SHIFT, ACC_16B_MASK);
            _pchannel->p_dtptr[Pipe] += 2;
        }
        if( (count % 2) != 0 )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->D0FIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  MBW_32, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *((uint32_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_32(&_pchannel->phwdevice->D0FIFO.UINT32, ACC_32B_SHIFT, ACC_32B_MASK);
            _pchannel->p_dtptr[Pipe] += 4;
        }
        if( (count % 2) != 0 )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->D0FIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
        else if( (count % 4) != 0 )
        {
            *((uint16_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_16(&_pchannel->phwdevice->D0FIFO.UINT16[H], ACC_16B_SHIFT, ACC_16B_MASK);
            _pchannel->p_dtptr[Pipe] += 2;
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    TRACE((" COMPLETE\r\n"));
}
/******************************************************************************
End of function R_USBF_DataioD0FifoRead
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioD1FifoWrite
* Description     : D1FIFO write
* Argument        : uint16_t Pipe      ; Pipe Number
*                   uint16_t count     ; Data Size(Byte)
* Return value    : None
******************************************************************************/
void R_USBF_DataioD1FifoWrite(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count)
{
    if(NULL == _pchannel->p_dtptr[Pipe])
    {
        /* Null pointer is passed as pointer to dummy buffer to discard data */
        rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  MBW_32, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_32(&_pchannel->phwdevice->D1FIFO.UINT32,  *((uint32_t *)_pchannel->p_dtptr[Pipe]), ACC_32B_SHIFT, ACC_32B_MASK);
        }
        if( (count % 2) != 0 )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->D1FIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
        }
        else if( (count % 4) != 0 )
        {
            rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFO.UINT16[H],  *((uint16_t *)_pchannel->p_dtptr[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    else if(((uint32_t)_pchannel->p_dtptr[Pipe] & 0x00000001) != 0)
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  MBW_8, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->D1FIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else if(((((uint32_t)_pchannel->p_dtptr[Pipe]) & 0x00000003) != 0) || ((count % 4) == 3))
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  MBW_16, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/2; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFO.UINT16[H],  *((uint16_t *)_pchannel->p_dtptr[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
            _pchannel->p_dtptr[Pipe] += 2;
        }
        if( (count % 2) != 0 )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->D1FIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  MBW_32, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            rza_io_reg_write_32(&_pchannel->phwdevice->D1FIFO.UINT32,  *((uint32_t *)_pchannel->p_dtptr[Pipe]), ACC_32B_SHIFT, ACC_32B_MASK);
            _pchannel->p_dtptr[Pipe] += 4;
        }
        if( (count % 2) != 0 )
        {
            rza_io_reg_write_8(&_pchannel->phwdevice->D1FIFO.UINT8[HH],  *_pchannel->p_dtptr[Pipe], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
        else if( (count % 4) != 0 )
        {
            rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFO.UINT16[H],  *((uint16_t *)_pchannel->p_dtptr[Pipe]), ACC_16B_SHIFT, ACC_16B_MASK);
            _pchannel->p_dtptr[Pipe] += 2;
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
}
/******************************************************************************
End of function R_USBF_DataioD1FifoWrite
******************************************************************************/

/******************************************************************************
* Function Name   : R_USBF_DataioD1FifoRead
* Description     : D1FIFO read
* Argument        : uint16_t Pipe      ; Pipe Number
*                   uint16_t count     ; Data Size(Byte)
* Return value    : None
******************************************************************************/
void R_USBF_DataioD1FifoRead(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count)
{
    if(NULL == _pchannel->p_dtptr[Pipe])
    {
        /* Null pointer is passed as pointer to dummy buffer to discard data */
        rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  MBW_32, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *((uint32_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_32(&_pchannel->phwdevice->D1FIFO.UINT32, ACC_32B_SHIFT, ACC_32B_MASK);
        }
        if( (count % 2) != 0 )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->D1FIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);

        }
        else if( (count % 4) != 0 )
        {
            *((uint16_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_16(&_pchannel->phwdevice->D1FIFO.UINT16[H], ACC_16B_SHIFT, ACC_16B_MASK);
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
    else if(((uint32_t)_pchannel->p_dtptr[Pipe] & 0x00000001) != 0)
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  MBW_8, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->D1FIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else if(((((uint32_t)_pchannel->p_dtptr[Pipe]) & 0x00000003) != 0) || ((count % 4) == 3))
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  MBW_16, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/2; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *((uint16_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_16(&_pchannel->phwdevice->D1FIFO.UINT16[H], ACC_16B_SHIFT, ACC_16B_MASK);
            _pchannel->p_dtptr[Pipe] += 2;
        }
        if( (count % 2) != 0 )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->D1FIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
    }
    else
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  MBW_32, USB_DnFIFOSEL_MBW_SHIFT, USB_DnFIFOSEL_MBW);
        for( _pchannel->generic_even = count/4; _pchannel->generic_even; --_pchannel->generic_even )
        {
            *((uint32_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_32(&_pchannel->phwdevice->D1FIFO.UINT32, ACC_32B_SHIFT, ACC_32B_MASK);
            _pchannel->p_dtptr[Pipe] += 4;
        }
        if( (count % 2) != 0 )
        {
            *_pchannel->p_dtptr[Pipe]  = rza_io_reg_read_8(&_pchannel->phwdevice->D1FIFO.UINT8[HH], ACC_8B_SHIFT, ACC_8B_MASK);
            _pchannel->p_dtptr[Pipe] += 1;
        }
        else if( (count % 4) != 0 )
        {
            *((uint16_t *)_pchannel->p_dtptr[Pipe])  = rza_io_reg_read_16(&_pchannel->phwdevice->D1FIFO.UINT16[H], ACC_16B_SHIFT, ACC_16B_MASK);
            _pchannel->p_dtptr[Pipe] += 2;
        }
        else
        {
            /* Do Nothing */
            ;
        }
    }
}
/******************************************************************************
End of function R_USBF_DataioD1FifoRead
******************************************************************************/
