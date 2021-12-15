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
* File Name        : usb_hal.c
* Version         : 1.00
* Device         : RZ/A1H ( )
* Tool Chain     : HEW, Renesas SuperH Standard Tool chain v9.3
* H/W Platform    : RSK2+SH7269
* Description     : Hardware Abstraction Layer (HAL)
*
*                   Provides a hardware independent API to the USB peripheral
*                   on the SH7269.
*                   Supports:-
*                    Control IN, Control OUT, Bulk IN, Bulk OUT, Interrupt IN.
*
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

/******************************************************************************
System Includes (Project Level Includes)
******************************************************************************/
/* The following header file defines the assert() macro */
#include <assert.h>

/******************************************************************************
User Includes (Project Level Includes)
******************************************************************************/
/* Following header file provides rte type definitions. */
#include "stdint.h"

/*    Following header file provides a structure to access on-chip I/O
    registers. */
#include "iodefine_cfg.h"

/*    Following header file provides definition for Low level driver. */
#include "r_usb_hal.h"

/* Following header file provides common defines for widely used items. */
#include "usb_common.h"

#include "r_lib_int.h"

#include "usb.h"

#include "rza_io_regrw.h"            /* Low level register read/write header */
#include "r_intc.h"

#include "usb_iobitmask.h"

#include "trace.h"

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
    #undef TRACE
    #define TRACE(x)
#endif

/******************************************************************************
Type Definitions
******************************************************************************/

/******************************************************************************
Global Variables
******************************************************************************/

/*Control Pipe Data*/
static volatile st_usbf_control_t usb_control =
{
    /*StateControl*/
    STATE_DISCONNECTED
};

/******************************************************************************
Function Prototypes
******************************************************************************/
static void hw_init(volatile st_usb_object_t *channel);
static void hw_close(volatile st_usb_object_t *channel);
static void waitmodule(volatile  st_usb_object_t *_pchannel);
static void p_usb_attach( volatile st_usb_object_t *_pchannel);

/*Endpoint 0*/
static void handle_setup_cmd( volatile  st_usb_object_t *_pchannel );

/******************************************************************************
User Program Code
******************************************************************************/

/******************************************************************************
* Function Name    : waitmodule
* Description    : Enables & resets the USB module.
* Argument        : None
* Return value    : None
******************************************************************************/
static void waitmodule( volatile  st_usb_object_t *_pchannel)
{
    volatile uint16_t    buf;

    do
    {
        rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  BITUSBE, ACC_16B_SHIFT, ACC_16B_MASK);
        buf  = rza_io_reg_read_16(&_pchannel->phwdevice->SYSCFG0, USB_SYSCFG_USBE_SHIFT, USB_SYSCFG_USBE);
    } while( BITUSBE != buf );

    /* SW reset */
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0, 0, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function waitmodule
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalInit
* Description     :   Initialise this USB HAL layer.
*                     This must be called before using any other function.
*                     Enables the USB peripheral ready for enumeration.
*
* Argument        :   _p_cb_setup: Callback function, Setup Packet received.
*                     _p_cb_cable: Callback function, Cable Connected/Disconnected.
*                     _p_cb_error: Callback function, Error occurred.
*
* Return value    :    Error code.
******************************************************************************/
usb_err_t R_USB_HalInit(volatile st_usb_object_t *_pchannel,
                    CB_SETUP _p_cb_setup,
                    CB_CABLE _p_cb_cable,
                    CB_ERROR _p_cb_error)
{
    _pchannel->err = USB_ERR_OK;
    _pchannel->usb_connected = 0;

    /*Check parameters are not NULL*/
    if( (NULL == _p_cb_setup)  ||
        ((NULL == _p_cb_cable) ||
        (NULL == _p_cb_error)) )
    {
        _pchannel->err = USB_ERR_PARAM;
    }
    else
    {
        /*Store CallBack function pointers*/
        _pchannel->callbacks.p_cb_setup = _p_cb_setup;
        _pchannel->callbacks.p_cb_cable = _p_cb_cable;
        _pchannel->callbacks.p_cb_error = _p_cb_error;

    }

    if(USB_ERR_OK == _pchannel->err)
    {
        /*Initialise the USB module.
        Enable USB interrupts in driver */
        hw_init(_pchannel);
    }

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalInit
******************************************************************************/

/******************************************************************************
* Function Name  :   R_USB_HalClose
* Description    :   Closes this USB HAL layer.
* Argument       :   None
* Return value   :   Error code.
******************************************************************************/
usb_err_t R_USB_HalClose(volatile st_usb_object_t *_pchannel)
{
    _pchannel->err = USB_ERR_OK;

    /*Release the USB module.
      This includes enabling USB interrupts*/
    hw_close(_pchannel);
    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalClose
******************************************************************************/

/******************************************************************************
* Function Name   :    R_USB_HalControlAck
* Description     :    ACK a control IN.
*                     Used to ACK a setup packet that doesn't have a data stage.
*
*                     Note: This must only be called in response to a setup
*                     packet.
* Argument        :    None
* Return value    :    Error code.
******************************************************************************/
usb_err_t R_USB_HalControlAck(volatile st_usb_object_t *_pchannel)
{
    _pchannel->err = USB_ERR_OK;

    usb_control.device_state = STATE_READY;

    DEBUG_MSG_LOW(("USBHAL: - Control ACK\r\n"));

    /* Set BUF */
    R_LIB_SetBUF(_pchannel, PIPE0);
    rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,  1, USB_DCPCTR_CCPL_SHIFT, USB_DCPCTR_CCPL);

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalControlAck
******************************************************************************/

/******************************************************************************
* Function Name   :    R_USB_HalControlIn
* Description     :    Sends supplied data to host and then handles
*                     receiving ACK from host.
*                     Note: This must only be called in response to a setup
*                     packet.
*
*                     Note: Often Windows interrupts this ControlIN with
*                     another setup packet - so this may not get all sent.
*
* Argument        :    _num_bytes:     Number of bytes to send.
*                     _pbuffer:    Data Buffer.
* Return value    :    Error code.
******************************************************************************/
usb_err_t R_USB_HalControlIn(volatile st_usb_object_t *_pchannel, uint16_t _num_bytes, const uint8_t* _pbuffer)
{
    _pchannel->err = USB_ERR_OK;

    /*Check state*/
    if (STATE_CONTROL_SETUP != usb_control.device_state)
    {
        /*State error - didn't expect this now*/
        _pchannel->err = USB_ERR_STATE;
        DEBUG_MSG_HIGH(("USBHAL: Control IN State Error\r\n"));
    }
    else
    {
        usb_control.device_state = STATE_CONTROL_IN;

        DEBUG_MSG_MID(("USBHAL: CONTROL IN start, %u bytes.\r\n", _num_bytes));

        /*Store parameters*/
        _pchannel->dtcnt[PIPE0] = _num_bytes;
        _pchannel->p_dtptr[PIPE0] = (uint8_t*)_pbuffer;

        /* Select the FIFO for the PIPE0 */
        R_USBF_DataioFPortChange2(_pchannel, PIPE0,CUSE,BITISEL);

        /* Buffer Clear */
        rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR,  BITBCLR, ACC_16B_SHIFT, ACC_16B_MASK);

        /* Send Data from Buffer to the Host */
        _pchannel->endflag_k    = R_USBF_DataioBufWriteC(_pchannel, PIPE0);


        /* Peripheral Control sequence */
        switch( _pchannel->endflag_k )
        {
        /* Continue of data write */
        case    WRITING:
            break;

        /* End of data write */
        case    WRITEEND:

        /* End of data write */
        case    WRITESHRT:

            /* Disable Ready Interrupt */
            R_LIB_DisableIntR(_pchannel, PIPE0);

            /* Enable Empty Interrupt */
            R_LIB_EnableIntE(_pchannel, PIPE0);
            break;

        /* FIFO access error */
        case    FIFOERROR:
        default:

            /* Disable Ready Interrupt */
            R_LIB_DisableIntR(_pchannel, PIPE0);

            /* Disable Empty Interrupt */
            R_LIB_DisableIntE(_pchannel, PIPE0);
            _pchannel->pipe_flag[PIPE0] = PIPE_IDLE;
            break;
        }

        /* Set BUF */
        R_LIB_SetBUF(_pchannel, PIPE0);
        rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR,   0x01, USB_DCPCTR_CCPL_SHIFT, USB_DCPCTR_CCPL);

        /* End or Err or Continue */
        _pchannel->err = (int16_t)_pchannel->endflag_k;

        usb_control.device_state = STATE_READY;
    }

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalControlIn
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalControlOut
* Description     :   This sets up the HAL to receives CONTROL OUT
*                     data from the host and to then send an ACK.
*
*                     If a short packet is received before _num_bytes
*                     have been read then this will finish. The actual
*                     number of bytes read will be specified in the CB.
*
*                     Note: This must only be called in response to a setup
*                     packet.
*
* Argument        :   _num_bytes:     Number of bytes to receive.
*                     _pbuffer:    Data Buffer.
*                     _CBDone:    Callback called when OUT has completed.
* Return value    :   Error code.
******************************************************************************/
usb_err_t R_USB_HalControlOut(volatile st_usb_object_t *_pchannel,
                             uint16_t _num_bytes,
                             uint8_t* _pbuffer,
                             CB_DONE_OUT _CBDone)
{
    _pchannel->err = USB_ERR_OK;

    /* Check state */
    if (STATE_CONTROL_SETUP != usb_control.device_state)
    {
        /* State error - didn't expect this now */
        _pchannel->err = USB_ERR_STATE;
        DEBUG_MSG_HIGH(("USBHAL: Control OUT State Error\r\n"));
    }
    else
    {
        usb_control.device_state = STATE_CONTROL_OUT;

        DEBUG_MSG_MID(("USBHAL: CONTROL OUT start, %u bytes.\r\n", _num_bytes));

        /*Store parameters*/
        _pchannel->dtcnt[PIPE0] = _num_bytes;
        _pchannel->p_dtptr[PIPE0] = _pbuffer;
        _pchannel->callbacks.p_cb_cout_mfpdone = _CBDone;

        /* Set BUF */
        R_LIB_SetBUF(_pchannel, 0);
        rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR, 0x01, USB_DCPCTR_CCPL_SHIFT, USB_DCPCTR_CCPL);

        /* Enable Ready Interrupt */
        R_LIB_EnableIntR(_pchannel, 0);
    }

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalControlOut
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalBulkOut
* Description     :   This sets up the HAL to receives BULK OUT
*                     data from the host.
*
*                     If a short packet is received before _num_bytes
*                     have been read then this will finish. The actual
*                     number of bytes read will be specified in the CB.
*
* Argument        :   _num_bytes:     Number of bytes to receive.
*                     _pbuffer:    Data Buffer.
*                     _CBDone:    Callback called when OUT has completed.
* Return value    :    Error code.
******************************************************************************/
usb_err_t R_USB_HalBulkOut(volatile st_usb_object_t *_pchannel,
                          uint32_t _num_bytes,
                          uint8_t* _pbuffer,
                          CB_DONE_OUT _CBDone)
{
    _pchannel->err = USB_ERR_OK;
    uint8_t pipe;
    uint16_t buffer;

    /*Check cable is connected*/
    if(STATE_DISCONNECTED == usb_control.device_state)
    {
        _pchannel->err = USB_ERR_NOT_CONNECTED;
        DEBUG_MSG_MID(("USBHAL: BULK OUT - Not Connected\r\n"));
    }
    else
    {
        pipe = PIPE1;
        buffer = R_LIB_GetPid(_pchannel, pipe);
        if( PID_STALL == buffer )
        {
            DEBUG_MSG_MID(("USBHAL: USBHAL_Bulk_OUT - PIPE1 STALLED\r\n"));
            return (USB_ERR_OK);
        }

        switch( _pchannel->pipe_flag[pipe] )
        {
            case PIPE_IDLE:
                R_USBF_DataioReceiveStart(_pchannel, pipe, _num_bytes, _pbuffer);
                break;

            case PIPE_WAIT:
                break;

            case PIPE_DONE:
                _pchannel->pipe_flag[pipe] = PIPE_IDLE;
                break;
            case PIPE_STALL:
                break;

            case PIPE_NORES:
                break;

            default:
                break;
        }
        _pchannel->callbacks.p_cb_bout_mfpdone = _CBDone;
    }

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalBulkOut
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalBulkIn
* Description     :   Sends supplied data to host using BULK IN
*                     and then calls specified callback.
* Argument        :    _num_bytes:     Number of bytes to receive.
*                     _pbuffer:    Data Buffer.
*                     _CBDone:    Callback called when IN has completed.
*
* Return value    :    Error code.
******************************************************************************/
usb_err_t R_USB_HalBulkIn(volatile st_usb_object_t *_pchannel, uint32_t _num_bytes, const uint8_t* _pbuffer, CB_DONE_BULK_IN _CBDone)
{
    _pchannel->err = USB_ERR_OK;

    /*Check cable is connected*/
    if(STATE_DISCONNECTED == usb_control.device_state)
    {
        _pchannel->err = USB_ERR_NOT_CONNECTED;
        DEBUG_MSG_MID(("USBHAL: BULK IN - Not Connected\r\n"));
    }
    else
    {
        /*Store parameters*/
        _pchannel->dtcnt[PIPE2] = _num_bytes;
        _pchannel->p_dtptr[PIPE2] = (uint8_t*)_pbuffer;
        _pchannel->callbacks.p_cb_bin_mfpdone = _CBDone;

        /* Ignore count clear */
        _pchannel->pipe_ignore[PIPE2] = 0;
        _pchannel->pipe_flag[PIPE2] = PIPE_WAIT;
        _pchannel->endflag_k    = R_USBF_DataioBufWrite(_pchannel, PIPE2);

        /* Peripheral Control sequence */
        switch( _pchannel->endflag_k )
        {
            case    WRITEEND:

                /* Set BUF */
                R_LIB_SetBUF(_pchannel, PIPE2);
                break;
            case    WRITESHRT:

                /* Set BUF */
                R_LIB_SetBUF(_pchannel, PIPE2);
                if (NULL != _pchannel->callbacks.p_cb_bin_mfpdone)
                {
                    _pchannel->callbacks.p_cb_bin_mfpdone(_pchannel,USB_ERR_OK);
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

        /* End or Err or Continue */
        _pchannel->err = (int16_t)_pchannel->endflag_k;

        usb_control.device_state = STATE_READY;
    }

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalBulkIn
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalInterruptIn
*
* Description     :   Sends supplied data to host using INTERRUPT IN
*                     and then calls specified callback.
* Argument        :   _num_bytes:     Number of bytes to receive.
*                     _pbuffer:    Data Buffer.
*                     _CBDone:    Callback called when IN has completed.
*
* Return value    :    Error code.
******************************************************************************/

usb_err_t R_USB_HalInterruptIn(volatile st_usb_object_t *_pchannel,
                              uint32_t _num_bytes,
                              const uint8_t* _pbuffer,
                              CB_DONE _CBDone)
{
    _pchannel->err = USB_ERR_OK;

    /*Check cable is connected*/
    if(STATE_DISCONNECTED == usb_control.device_state)
    {
        _pchannel->err = USB_ERR_NOT_CONNECTED;
        DEBUG_MSG_MID(("USBHAL: INT IN - Not Connected\r\n"));
    }
    else
    {
        /*Store parameters*/
        _pchannel->dtcnt[PIPE6] = _num_bytes;
        _pchannel->p_dtptr[PIPE6] = (uint8_t*)_pbuffer;
        _pchannel->callbacks.p_cb_iin_mfpdone = _CBDone;

        /* Write data from buffer to the Host */
        _pchannel->endflag_k    = R_USBF_DataioBufWriteC(_pchannel, PIPE6);

        /* Peripheral Control sequence */
        switch( _pchannel->endflag_k )
        {
            case    WRITING:

                /* Enable Empty Interrupt */
                R_LIB_EnableIntE(_pchannel, PIPE6);
                break;

            /* FIFO access error */
            case    FIFOERROR:
                break;

            default:
                break;
        }

        /* Set BUF */
        R_LIB_SetBUF(_pchannel, PIPE6);

        /* End or Err or Continue */
        _pchannel->err = (int16_t)_pchannel->endflag_k;
        _pchannel->callbacks.p_cb_iin_mfpdone(0);
        usb_control.device_state = STATE_READY;
    }

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalInterruptIn
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalControlStall
* Description     :   Generate a stall on the Control Endpoint and then
*                     automatically clears it.
* Argument        :    -
* Return value    :    -
******************************************************************************/

void R_USB_HalControlStall(volatile st_usb_object_t *_pchannel)
{
    DEBUG_MSG_MID(("USBHAL: - Control_Stall\r\n"));
    R_LIB_SetSTALL(_pchannel, PIPE0);
}
/******************************************************************************
End of function R_USB_HalControlStall
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalBulkInStall
* Description     :   Generate a stall on the BULK IN Endpoint and then
*                     automatically clears it.
* Argument        :    None
* Return value    :    None
******************************************************************************/

void R_USB_HalBulkInStall(volatile st_usb_object_t *_pchannel)
{
    DEBUG_MSG_MID(("USBHAL: - Bulk_IN_Stall\r\n"));
    R_LIB_SetSTALL(_pchannel, PIPE2);
}
/******************************************************************************
End of function R_USB_HalBulkInStall
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalBulkOutStall
* Description     :   Generate a stall on the BULK OUT Endpoint and then
*                     automatically clears it.
* Argument        :-
* Return value    :    -
******************************************************************************/

void R_USB_HalBulkOutStall(volatile st_usb_object_t *_pchannel)
{
    DEBUG_MSG_MID(("USBHAL: - Bulk_OUT_Stall\r\n"));
    R_LIB_SetSTALL(_pchannel, PIPE1);
}
/******************************************************************************
End of function R_USB_HalBulkOutStall
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalInterruptInStall
* Description     :   Generate a stall on the Interrupt IN Endpoint and then
*                     automatically clears it.
* Argument        :    None
* Return value    :    None
******************************************************************************/

void R_USB_HalInterruptInStall(volatile st_usb_object_t *_pchannel)
{
    DEBUG_MSG_MID(("USBHAL: - Interrupt_IN_Stall\r\n"));
    R_LIB_SetSTALL(_pchannel, PIPE6);
}
/******************************************************************************
End of function R_USB_HalInterruptInStall
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalGetDeviceState
* Description     :   Return the USB address assigned by host to hardware
*                     register.
*
* Argument        :    address: USB address assigned by host.
* Return value    :    Error code.
******************************************************************************/

usb_err_t R_USB_HalGetDeviceState(volatile st_usb_object_t *_pchannel, uint8_t* state)
{
    /* Get device state */
    *state  = (uint8_t) rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, (uint16_t)USB_INTSTS0_DVSQ_SHIFT,USB_INTSTS0_DVSQ);

    DEBUG_MSG_MID( ("USBHAL: - Current device state %d\r\n",
    rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, USB_INTSTS0_DVSQ_SHIFT, USB_INTSTS0_DVSQ)));

    return USB_ERR_OK;
}
/******************************************************************************
End of function R_USB_HalGetDeviceState
******************************************************************************/

/******************************************************************************
* Function Name   :    R_USB_HalIsEndpointStalled
* Description     :    Return the USB address assigned by host to hardware
*                      register.
* Argument        :    address: USB address assigned by host.
* Return value    :    Error code.
******************************************************************************/
BOOL R_USB_HalIsEndpointStalled(volatile st_usb_object_t *_pchannel, uint8_t pipe)
{
    uint16_t pipe_state;

    /* Get pipe state */
    pipe_state = R_LIB_GetPid(_pchannel, pipe);

    DEBUG_MSG_MID( ("USBHAL: - Current endpoint state %d\r\n", pipe_state));

    if(pipe_state > 1)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
/******************************************************************************
End of function R_USB_HalIsEndpointStalled
******************************************************************************/

/******************************************************************************
* Function Name   :   R_USB_HalCancel
* Description     :   Cancel any current operations.
*                     Perform a HAL reset.
*                     Any pending "Done" callbacks will then be called with
*                     supplied error code.
*
*                     Note: This is automatically called when the cable
*                     is disconnected.
*
* Argument        :    void
* Return value    :    Error code.
******************************************************************************/

usb_err_t R_USB_HalCancel(volatile st_usb_object_t *_pchannel)
{
    _pchannel->err = USB_ERR_OK;

    DEBUG_MSG_MID( ("USBHAL: - Cancel called\r\n"));

    /*Reset HAL */
    R_USB_HalReset(_pchannel);

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalCancel
******************************************************************************/

/******************************************************************************
* Function Name   : p_usb_bus_reset
* Description     : Reset the USB interface.
* Argument        : None
* Return value    : None
******************************************************************************/

static void p_usb_bus_reset(volatile st_usb_object_t *_pchannel)
{
    DEBUG_MSG_HIGH( ("USBHAL: Bus Reset\r\n"));

    /* CFIFO Port Select Register  (0x1E) */
    R_USBF_DataioFPortChange2(_pchannel, PIPE0, CUSE, BITISEL);
    rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOSEL,  (BITMBW_16 | BITISEL), ACC_16B_SHIFT, ACC_16B_MASK);

    /* Clear the CFIFO Buffer */
    rza_io_reg_write_16(&_pchannel->phwdevice->CFIFOCTR,  0x4000, ACC_16B_SHIFT, ACC_16B_MASK);

    /* DCP Configuration Register  (0x5C) */
    rza_io_reg_write_16(&_pchannel->phwdevice->DCPCFG,  0x0000, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->DCPCFG,  0x1, USB_DCPCFG_DIR_SHIFT, USB_DCPCFG_DIR);

    /* DCP Maxpacket Size Register (0x5E) */
    rza_io_reg_write_16(&_pchannel->phwdevice->DCPMAXP,  (uint16_t)CONTROL_OUT_PACKET_SIZE, USB_DCPMAXP_MXPS_SHIFT, USB_DCPMAXP_MXPS);

    /* D0FIFO Port Select Register (0x24) */
    rza_io_reg_write_16(&_pchannel->phwdevice->D0FIFOSEL,  BITMBW_16, ACC_16B_SHIFT, ACC_16B_MASK);

    /* D1FIFO Port Select Register (0x2A) */
    rza_io_reg_write_16(&_pchannel->phwdevice->D1FIFOSEL,  BITMBW_16, ACC_16B_SHIFT, ACC_16B_MASK);
}
/******************************************************************************
End of function p_usb_bus_reset
******************************************************************************/

/******************************************************************************
* Function Name   : p_chk_vbus_sts
* Description     : Check the status of VBUS
* Argument        : None
* Return value    : 1 - VBUS ON        0 - VBUS OFF
******************************************************************************/

static uint16_t p_chk_vbus_sts(volatile st_usb_object_t *_pchannel)
{
    /* Temporary storage for repeated read checking for valid signal states. */
    uint16_t    buf1;
    uint16_t    buf2;
    uint16_t    buf3;

    do
    {
        /* USB module sets the VBSTS bit to indicate the VBUS pin input value. When the VBUS
           interrupt is generated, you must repeat reading the VBSTS bit until the same value
           is read several times to eliminate chattering. */
        buf1  = rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, USB_INTSTS0_VBSTS_SHIFT, USB_INTSTS0_VBSTS);
        delay_us(100);
        buf2  = rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, USB_INTSTS0_VBSTS_SHIFT, USB_INTSTS0_VBSTS);
        delay_us(100);
        buf3  = rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, USB_INTSTS0_VBSTS_SHIFT, USB_INTSTS0_VBSTS);
        delay_us(100);
    }
    while( (buf1 != buf2)|| (buf2 != buf3) );

    if( USB_OFF == buf1 )
    {
        return USB_OFF;
    }

    return USB_ON;
}
/******************************************************************************
End of function p_chk_vbus_sts
******************************************************************************/

/******************************************************************************
* Function Name    : p_usb_attach
* Description    : Configures the USB module.
* Argument        : None
* Return value    : None
******************************************************************************/

static void p_usb_attach( volatile st_usb_object_t *_pchannel)
{
    DEBUG_MSG_HIGH( ("USBHAL: Setting USB Registers \r\n"));

    /* UPLLE Bit is required to be set but it is only available in USB0 ctrl reg set.

      Place USB0 in to suspend mode, set UPLLE and recover from suspend mode*/
    rza_io_reg_write_16(&_pchannel->phwdevice->SUSPMODE,  0, USB_SUSPMODE_SUSPM_SHIFT, USB_SUSPMODE_SUSPM);

    /*
    BIT's UCKSEL & UPLLE are shared between USB0 & USB1
    Must be set in USB0 when USB0 is used.
    Must be set in USB0 even when USB1 is used.*/
     rza_io_reg_write_16(&USB200.SYSCFG0, 0, USB_SYSCFG_UCKSEL_SHIFT, USB_SYSCFG_UCKSEL);
     rza_io_reg_write_16(&USB200.SYSCFG0, 1, USB_SYSCFG_UPLLE_SHIFT, USB_SYSCFG_UPLLE);

     delay_ms(10);

     /*
    USB Host sets 3 Wait states
    2 Access cycles required for USB - Table in section 29.3.2
    Total Access Cycles = 3 + 2 = 5

    P1 Clock = 66.67MHz (see peripheral_init_basic.c)
    Period = 1/ (66.67 * 10^6) = 15ns
    With 5 access cycles this is = 75ns
    Requirement is >67ns (from Table in section 29.3.2) */
    rza_io_reg_write_16(&_pchannel->phwdevice->BUSWAIT, 0x03, ACC_16B_SHIFT, ACC_16B_MASK);
    rza_io_reg_write_16(&_pchannel->phwdevice->SUSPMODE, 1, USB_SUSPMODE_SUSPM_SHIFT, USB_SUSPMODE_SUSPM);

     if(HI_ENABLE == _pchannel->hi_speed_enable)
     {
         /* Hi-Speed Mode */
        rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  1, USB_SYSCFG_HSE_SHIFT, USB_SYSCFG_HSE);
     }
     else
     {
         /* Full Speed Mode */
         rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  0, USB_SYSCFG_HSE_SHIFT, USB_SYSCFG_HSE);
     }

    /* USB function controller is selected */
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  0x0, USB_SYSCFG_DCFM_SHIFT, USB_SYSCFG_DCFM);

    /* D- and D+ ara not pulled down */
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  0x0, USB_SYSCFG_DRPD_SHIFT, USB_SYSCFG_DRPD);

    /* D+ is pulled up */
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  0x1, USB_SYSCFG_DPRPU_SHIFT, USB_SYSCFG_DPRPU);

    /* USB module operation is enable */
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  0x1, USB_SYSCFG_USBE_SHIFT, USB_SYSCFG_USBE);

    delay_ms(10);

}
/******************************************************************************
End of function p_usb_attach
******************************************************************************/


/******************************************************************************
* Function Name   : p_enable_int_module
* Description     : Enable the default set of interrupts.
*                   Some interrupts will be enabled later as they are required.
* Argument        : None
* Return value    : None
*******************************************************************************/

static void p_enable_int_module(volatile st_usb_object_t *_pchannel)
{
    _pchannel->buf  = rza_io_reg_read_16(&_pchannel->phwdevice->INTENB0, ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->buf |= (BITVBSE | (BITDVSE | (BITCTRE | (BITBEMP | (BITNRDY | BITBRDY )))));
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,  _pchannel->buf, ACC_16B_SHIFT, ACC_16B_MASK);

    R_LIB_EnableIntE(_pchannel, PIPE0);
}
/******************************************************************************
End of function p_enable_int_module
******************************************************************************/

/******************************************************************************
* Function Name    : p_usb_detach
* Description    : Reset the USB interface.
* Argument        : None
* Return value    : None
******************************************************************************/

static void p_usb_detach(volatile st_usb_object_t *_pchannel)
{
    DEBUG_MSG_HIGH( ("USBHAL: USB detached\r\n"));
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  0, USB_SYSCFG_DPRPU_SHIFT, USB_SYSCFG_DPRPU);
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  0, USB_SYSCFG_USBE_SHIFT, USB_SYSCFG_USBE);
    rza_io_reg_write_16(&_pchannel->phwdevice->SYSCFG0,  1, USB_SYSCFG_USBE_SHIFT, USB_SYSCFG_USBE);

    /* Interrupt Enable */
    p_enable_int_module(_pchannel);
}
/******************************************************************************
End of function p_usb_detach
******************************************************************************/

/******************************************************************************
* Function Name   : p_usb_resume
* Description     : USB resume function
* Argument        : None
* Return value    : None
******************************************************************************/

static void p_usb_resume(volatile st_usb_object_t *_pchannel)
{
    /* RESM Status Clear */
    RESM_StsClear(_pchannel);

    /* RESM Interrupt Disable */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,  0, USB_INTENB0_RSME_SHIFT, USB_INTENB0_RSME);
}
/******************************************************************************
End of function p_usb_resume
******************************************************************************/

/******************************************************************************
 * Function Name: cpu_delay_1us
 * Description  : Wait (nearly) 1us by loop.
 * Arguments    : time : specify wait time (us order).
 * Return Value : none
 ******************************************************************************/
static void cpu_delay_1us (uint16_t time)
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
/*******************************************************************************
 End of function cpu_delay_1us
 ******************************************************************************/

/******************************************************************************
* Function Name   : R_USB_HalIsr
* Description     : Processes the USB interrupts
* Argument        : None
* Return value    : None
******************************************************************************/
void R_USB_HalIsr( volatile st_usb_object_t *_pchannel)
{
    _pchannel->err;

    /* Temporary storage for repeated read checking for valid signal states. */
    uint16_t buf1;
    uint16_t buf2;
    uint16_t buf3;

    /* interrupt status */
    uint16_t    int_sts0;
    uint16_t    int_sts1;
    uint16_t    int_sts2;
    uint16_t    int_sts3;

    /* interrupt enable */
    uint16_t    int_enb0;
    uint16_t    int_enb2;
    uint16_t    int_enb3;
    uint16_t    int_enb4;

    /* USB module sets the INTSTS bit to indicate the Interrupt status. When the
       interrupt is generated, you must repeat reading the VBSTS bit until the same value
       is read several times to eliminate chattering. */
    do
    {
        buf1 = rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, ACC_16B_SHIFT, ACC_16B_MASK);
        cpu_delay_1us(10);
        buf2 = rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, ACC_16B_SHIFT, ACC_16B_MASK);
        cpu_delay_1us(10);
        buf3 = rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, ACC_16B_SHIFT, ACC_16B_MASK);
    } while (((buf1 & BITVBSTS) != (buf2 & BITVBSTS)) || ((buf2 & BITVBSTS) != (buf3 & BITVBSTS)));

    int_sts0 = buf1;

    if(!(int_sts0 & (((BITVBINT|BITRESM)|(BITSOFR|BITDVST))|((BITCTRT|BITBEMP)|(BITNRDY|BITBRDY)))))
    {
        DEBUG_MSG_HIGH( ("\r\nUSBHAL: NO INTERRUPT OF INTEREST\r\n"));
        return;
    }

    int_sts1  = rza_io_reg_read_16(&_pchannel->phwdevice->BRDYSTS, ACC_16B_SHIFT, ACC_16B_MASK);
    int_sts2  = rza_io_reg_read_16(&_pchannel->phwdevice->NRDYSTS, ACC_16B_SHIFT, ACC_16B_MASK);
    int_sts3  = rza_io_reg_read_16(&_pchannel->phwdevice->BEMPSTS, ACC_16B_SHIFT, ACC_16B_MASK);
    int_enb0  = rza_io_reg_read_16(&_pchannel->phwdevice->INTENB0, ACC_16B_SHIFT, ACC_16B_MASK);
    int_enb2  = rza_io_reg_read_16(&_pchannel->phwdevice->BRDYENB, ACC_16B_SHIFT, ACC_16B_MASK);
    int_enb3  = rza_io_reg_read_16(&_pchannel->phwdevice->NRDYENB, ACC_16B_SHIFT, ACC_16B_MASK);
    int_enb4  = rza_io_reg_read_16(&_pchannel->phwdevice->BEMPENB, ACC_16B_SHIFT, ACC_16B_MASK);


    if( (int_sts0 & BITSOFR) && (int_enb0 & BITSOFR) )
    {
        /* Clear SOF Interrupt Status */
        rza_io_reg_write_16(&_pchannel->phwdevice->INTSTS0,  (uint16_t)~BITSOFR, ACC_16B_SHIFT, ACC_16B_MASK);
        TRACE(("BITSOFR"));
    }

    /* Processing PIPE0 data */
    if((int_sts0 & BITBEMP) && ((int_enb0 & BITBEMP) && ((int_sts3 & int_enb4) & BITBEMP0)))
    {
        R_LIB_BempInt(_pchannel, int_sts3, int_enb4);
        TRACE(("P_BEMP_int(_pchannel, int_sts3, int_enb4)"));
    }
    else if((int_sts0 & BITNRDY) && ((int_enb0 & BITNRDY) && ((int_sts2 & int_enb3) & BITNRDY0)))
    {
        R_LIB_IntnInt(_pchannel, int_sts2, int_enb3);
        TRACE(("P_INTN_int(_pchannel, int_sts2, int_enb3)"));
    }

    else if((int_sts0 & BITBRDY) && ((int_enb0 & BITBRDY) && ((int_sts1 & int_enb2) & BITBRDY0)))
    {
        R_LIB_IntrInt(_pchannel, int_sts1, int_enb2);
        TRACE(("P_INTR_int(_pchannel, int_sts1, int_enb2)"));
    }

    /* Processing whithout PIPE0 data */
    else if((int_sts0 & BITBEMP) && ((int_enb0 & BITBEMP) && (int_sts3 & int_enb4)))
    {
        R_LIB_BempInt(_pchannel, int_sts3, int_enb4);
        TRACE(("P_BEMP_int(_pchannel, int_sts3, int_enb4)"));
    }

    else if((int_sts0 & BITNRDY) && ((int_enb0 & BITNRDY) && (int_sts2 & int_enb3)))
    {
        R_LIB_IntnInt(_pchannel, int_sts2, int_enb3);
        TRACE(("P_INTN_int(_pchannel, int_sts2, int_enb3)"));
    }

    /*Interrupt for Endpoint*/
    else if((int_sts0 & BITBRDY) && ((int_enb0 & BITBRDY) && (int_sts1 & int_enb2)))
    {
        R_LIB_IntrInt(_pchannel, int_sts1, int_enb2);
        TRACE(("P_INTR_int(_pchannel, int_sts1, int_enb2)"));
    }
    else if((int_sts0 & BITDVST) && (int_enb0 & BITDVSE))
    {
        /* clear state change interrupt */
        rza_io_reg_write_16(&_pchannel->phwdevice->INTSTS0,  (uint16_t)~BITDVST, ACC_16B_SHIFT, ACC_16B_MASK);
        switch(rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, USB_INTSTS0_DVSQ_SHIFT, USB_INTSTS0_DVSQ))
        {
            case STATE_POWERED:
                TRACE(("STATE_POWERED"));
                break;

            case STATE_DEFAULT:
                TRACE(("STATE_DEFAULT"));
                break;

            case STATE_ADDRESSED:
                TRACE(("STATE_ADDRESSED"));
                break;

            case STATE_CONFIGURED:
                TRACE(("STATE_CONFIGURED"));
                if(FALSE == _pchannel->connected)
                {
                    _pchannel->callbacks.p_cb_cable(_pchannel, TRUE);
                }
                R_LIB_DoSQCLR(_pchannel, PIPE1);
                R_LIB_DoSQCLR(_pchannel, PIPE2);
                break;

            default:
                TRACE(("case default"));
                break;
        }
    }
    else
    {
        /* Do Nothing */
        ;
    }

    if( (int_sts0 & BITCTRT) && (int_enb0 & BITCTRE) )
    {
        TRACE(("CTSQ read handle_setup_cmd"));

        /* CTSQ read */
        int_sts0  = rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, ACC_16B_SHIFT, ACC_16B_MASK);

        TRACE((": INTSTS0 0x%x", (rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, ACC_16B_SHIFT, ACC_16B_MASK) & 0xFF)));

        rza_io_reg_write_16(&_pchannel->phwdevice->INTSTS0,  (uint16_t)~BITCTRT, ACC_16B_SHIFT, ACC_16B_MASK);

        TRACE((": INTSTS0 0x%x", (rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, ACC_16B_SHIFT, ACC_16B_MASK) & 0xFF) ));

        /* Setup command receive complete */
        if ( (int_sts0 & BITVALID) != 0 ) {
            handle_setup_cmd(_pchannel);
        }
    }
    else if( (int_sts0 & BITRESM) && (int_enb0 & BITRSME) )
    {
        p_usb_resume(_pchannel);
        TRACE(("p_usb_resume"));
    }
    else if( (int_sts0 & BITVBINT) && (int_enb0 & BITVBSE) )
    {
        TRACE(("Status Clear STATE_READY USBHAL_Reset"));

        /* Status Clear */
        VBINT_StsClear(_pchannel);
        if( p_chk_vbus_sts(_pchannel) == USB_ON )
        {
            /* USB Bus Connected */
            usb_control.device_state = STATE_READY;

            /*Reset HAL*/
            R_USB_HalReset(_pchannel);
        }
        else
        {
            TRACE(("Status Clear STATE_DISCONNECTED USBHAL_Cancel"));

            /* USB Bus Disconnected */
            usb_control.device_state = STATE_DISCONNECTED;

            /* cancel all transfer that are in progress */
            _pchannel->err = R_USB_HalCancel(_pchannel);

            /*Call Registered Callback*/
            _pchannel->callbacks.p_cb_cable(_pchannel, FALSE);
            _pchannel->usb_connected = 0;
        }
    }
    else
    {
        /* Do Nothing */
        ;
    }
    TRACE(("\r\n"));


}
/******************************************************************************
End of function R_USB_HalIsr
******************************************************************************/

/******************************************************************************
* Function Name   :   handle_setup_cmd
* Description     :   Have received a Control Setup packet.
*                     Use registered callback to inform user.
* Argument        :   None
* Return value    :   None
******************************************************************************/

static void handle_setup_cmd( volatile  st_usb_object_t *_pchannel )
{
    DEBUG_MSG_MID(("\r\nUSBHAL: - Setup Received\r\n"));

    /* Cast to uint16_t is on an absolute bit field value stored in the macro, so safe. */
    rza_io_reg_write_16(&_pchannel->phwdevice->INTSTS0,  (uint16_t)~BITVALID, ACC_16B_SHIFT, ACC_16B_MASK);

    _pchannel->generic_buffer  = rza_io_reg_read_16(&_pchannel->phwdevice->USBREQ, ACC_16B_SHIFT, ACC_16B_MASK);

    _pchannel->generic_buffer2  = rza_io_reg_read_16(&_pchannel->phwdevice->USBVAL, ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer3  = rza_io_reg_read_16(&_pchannel->phwdevice->USBINDX, ACC_16B_SHIFT, ACC_16B_MASK);
    _pchannel->generic_buffer4  = rza_io_reg_read_16(&_pchannel->phwdevice->USBLENG, ACC_16B_SHIFT, ACC_16B_MASK);

    /* dump packet information */
    TRACE(("\r\nUSBHAL - Setup Received"));
    TRACE((": BUF = 0x%04x :", _pchannel->generic_buffer));
    TRACE((": INTSTS0 0x%x", (rza_io_reg_read_16(&_pchannel->phwdevice->INTSTS0, ACC_16B_SHIFT, ACC_16B_MASK) & 0xFF) ));
    TRACE((": bmRequest 0x%x", (rza_io_reg_read_16(&_pchannel->phwdevice->USBREQ, ACC_16B_SHIFT, ACC_16B_MASK) & 0xFF) ));
    TRACE((": bRequest %d", ((rza_io_reg_read_16(&_pchannel->phwdevice->USBREQ, ACC_16B_SHIFT, ACC_16B_MASK) >> 8) & 0xFF) ));
    TRACE((": wValue 0x%x", (rza_io_reg_read_16(&_pchannel->phwdevice->USBVAL, ACC_16B_SHIFT, ACC_16B_MASK)) ));
    TRACE((": wIndex %d", rza_io_reg_read_16(&_pchannel->phwdevice->USBINDX, ACC_16B_SHIFT, ACC_16B_MASK) ));
    TRACE((": wLength %d", rza_io_reg_read_16(&_pchannel->phwdevice->USBLENG, ACC_16B_SHIFT, ACC_16B_MASK) ));
    TRACE((": Direction %d:\r\n", rza_io_reg_read_16(&_pchannel->phwdevice->DCPCFG, USB_DCPCFG_DIR_SHIFT, USB_DCPCFG_DIR) ));

    DEBUG_MSG_HIGH(("USBHAL: bmRequest: 0x%x\r\n", (rza_io_reg_read_16(&_pchannel->phwdevice->USBREQ, ACC_16B_SHIFT, ACC_16B_MASK) & 0xFF) ));
    DEBUG_MSG_HIGH(("USBHAL: bRequest: %d\r\n", ((rza_io_reg_read_16(&_pchannel->phwdevice->USBREQ, ACC_16B_SHIFT, ACC_16B_MASK) >> 8 ) & 0xFF) ));
    DEBUG_MSG_HIGH(("USBHAL: wValue: 0x%x\r\n", (rza_io_reg_read_16(&_pchannel->phwdevice->USBVAL, ACC_16B_SHIFT, ACC_16B_MASK)) ));
    DEBUG_MSG_HIGH(("USBHAL: wIndex: %d\r\n", (rza_io_reg_read_16(&_pchannel->phwdevice->USBINDX, ACC_16B_SHIFT, ACC_16B_MASK)) ));
    DEBUG_MSG_HIGH(("USBHAL: wLength: %d\r\n", (rza_io_reg_read_16(&_pchannel->phwdevice->USBLENG, ACC_16B_SHIFT, ACC_16B_MASK)) ));
    DEBUG_MSG_HIGH(("USBHAL: Direction: %d\r\n", (rza_io_reg_read_16(&_pchannel->phwdevice->DCPCFG, , USB_DCPCFG_DIR_SHIFT, USB_DCPCFG_DIR)) ));

    /* Set_Configuration request is handled in HAL before using registered callback */
    if (0x0900 == _pchannel->generic_buffer)
    {
        /* Set BUF */
        rza_io_reg_write_16(&_pchannel->phwdevice->INTSTS0, (uint16_t)~BITVALID, ACC_16B_SHIFT, ACC_16B_MASK);
        R_LIB_SetBUF(_pchannel, PIPE0);
        rza_io_reg_write_16(&_pchannel->phwdevice->DCPCTR, 0x01, USB_DCPCTR_CCPL_SHIFT, USB_DCPCTR_CCPL);

        if(!_pchannel->usb_connected)
        {
            /*Call Registered Callback*/
            _pchannel->callbacks.p_cb_cable(_pchannel, TRUE);
            _pchannel->usb_connected++;
            TRACE(("USBHAL: USB CONNECTED\r\n"));
        }
    }
    else
    {
        /* Cast is selecting the required 8 bit value from the 16 bit source value. Caution for PORTING _ ENDIAN. */
        _pchannel->setup_cmd_buffer[0] = (uint8_t)(_pchannel->generic_buffer & 0x00FF);

        /* Cast is selecting the required 8 bit value from the 16 bit source value. Caution for PORTING _ ENDIAN. */
        _pchannel->setup_cmd_buffer[1] = (uint8_t)((_pchannel->generic_buffer & 0xFF00) >> 8);

        /* Cast is selecting the required 8 bit value from the 16 bit source value. Caution for PORTING _ ENDIAN. */
        _pchannel->setup_cmd_buffer[2] = (uint8_t)(_pchannel->generic_buffer2 & 0x00FF);

        /* Cast is selecting the required 8 bit value from the 16 bit source value. Caution for PORTING _ ENDIAN. */
        _pchannel->setup_cmd_buffer[3] = (uint8_t)((_pchannel->generic_buffer2 & 0xFF00) >> 8);

        /* Cast is selecting the required 8 bit value from the 16 bit source value. Caution for PORTING _ ENDIAN. */
        _pchannel->setup_cmd_buffer[4] = (uint8_t)(_pchannel->generic_buffer3 & 0x00FF);

        /* Cast is selecting the required 8 bit value from the 16 bit source value. Caution for PORTING _ ENDIAN. */
        _pchannel->setup_cmd_buffer[5] = (uint8_t)((_pchannel->generic_buffer3 & 0xFF00) >> 8);

        /* Cast is selecting the required 8 bit value from the 16 bit source value. Caution for PORTING _ ENDIAN. */
        _pchannel->setup_cmd_buffer[6] = (uint8_t)(_pchannel->generic_buffer4 & 0x00FF);

        /* Cast is selecting the required 8 bit value from the 16 bit source value. Caution for PORTING _ ENDIAN. */
        _pchannel->setup_cmd_buffer[7] = (uint8_t)((_pchannel->generic_buffer4 & 0xFF00) >> 8);

        usb_control.device_state = STATE_CONTROL_SETUP;

        /*NOTE: Expect to continue with a Data Stage
         - either Control IN or Control OUT or an ACK,
        User of this HAL layer will set this up when handling the Setup callback*/

        /*Call Registered Callback*/
        _pchannel->callbacks.p_cb_setup(_pchannel, (const uint8_t(*)[USB_SETUP_PACKET_SIZE])_pchannel->setup_cmd_buffer);
    }
}
/******************************************************************************
End of function handle_setup_cmd
******************************************************************************/

/******************************************************************************
* Function Name    : R_USB_HalResetEp
* Description    : Reset endpoint
* Argument        : uint16_t Con_Num        ; Configuration Number
* Return value    : None
******************************************************************************/

void R_USB_HalResetEp(volatile st_usb_object_t *_pchannel, uint16_t Con_Num)
{
    uint16_t        pipe;
    uint16_t        ep;
    uint16_t        index;
    uint16_t        *ptbl;

    (void) Con_Num;

    DEBUG_MSG_HIGH( ("USBHAL: %d no. end point reset\r\n", Con_Num));

    /* Pipe Setting */
    ptbl = (uint16_t *)(_pchannel->pend_pnt[0]);

    for( ep = 1; ep <= MAX_EP_NO; ++ep )
    {
        index = (uint16_t)(EPL * ((uint16_t) (ep - 1)));

        pipe = (uint16_t)(ptbl[index+0] & BITCURPIPE);
        _pchannel->pipe_tbl[pipe] = (uint16_t)(((ptbl[index + 1] & BITDIR) << 3)
                    | ((uint16_t) (ep | (ptbl[index + 0] & FIFO_USE))));

        /* PIPE Configuration */
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPESEL,  pipe, ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPECFG,  ptbl[index+1], ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPEBUF,  ptbl[index+2], ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPEMAXP,  ptbl[index+3], ACC_16B_SHIFT, ACC_16B_MASK);
        rza_io_reg_write_16(&_pchannel->phwdevice->PIPEPERI,  ptbl[index+4], ACC_16B_SHIFT, ACC_16B_MASK);

        /* Buffer Clear */
        /* SQCLR=1 */
        R_LIB_DoSQCLR(_pchannel, pipe);
    }
}
/******************************************************************************
End of function R_USB_HalResetEp
******************************************************************************/


/******************************************************************************
Function Name    : R_USB_HalConfigGet
Description        : Gets the current HAL configuration.
Parameters        : None
Return value    : Configuration structure.
******************************************************************************/

const st_usb_hal_config_t* R_USB_HalConfigGet(volatile st_usb_object_t *_pchannel)
{
    return (const st_usb_hal_config_t*)&_pchannel->config;
}
/******************************************************************************
End of function R_USB_HalConfigGet
******************************************************************************/

/******************************************************************************
Function Name    : R_USB_HalConfigSet
Description        : Sets the HAL configuration.
                  This only need to be used if the default configuration
                  is not suitable.
Parameters        : _pConfig: New Configuration.
Return value    : Error code
******************************************************************************/
usb_err_t R_USB_HalConfigSet(volatile st_usb_object_t *_pchannel, st_usb_hal_config_t* _pConfig)
{
    _pchannel->config = *_pConfig;

    return USB_ERR_OK;
}
/******************************************************************************
End of function R_USB_HalConfigSet
******************************************************************************/


/******************************************************************************
* Function Name   :   R_USB_HalReset
* Description     :   Resets the HAL.
*                     Resets the HAL to default state.
*
*                     Can be used after an unexpected error or when wanting to
*                     cancel a pending operation where you don't want the
*                     'done' callbacks called.
*
*                     Note: Used internally to reset HAL after detecting the USB
*                     cable has been disconnected/connected.
*                     1. Reset State and Busy flags and clear "done" CBs.
*                     (CBs are not called!)
*                     2. Clear all FIFOs
*                     3. Clear stalls.
*                     4. Enable default interrupts.
* Argument        :   -
* Return value    :   Error code.
******************************************************************************/
usb_err_t R_USB_HalReset( volatile st_usb_object_t *_pchannel )
{
    _pchannel->err = USB_ERR_OK;

    DEBUG_MSG_MID( ("USBHAL: - Resetting HAL\r\n"));

    /*If connected then go to ready state*/
    if(STATE_DISCONNECTED != usb_control.device_state)
    {
        usb_control.device_state = STATE_READY;
    }

    /* Clear Software variables and arrays */
    pipe_tbl_clear(_pchannel);
    mem_tbl_clear(_pchannel);
    mem_clear(_pchannel);

    /* Enable Interrupts */
    p_enable_int_module(_pchannel);

    return _pchannel->err;
}
/******************************************************************************
End of function R_USB_HalReset
******************************************************************************/

/******************************************************************************
* Function Name   : hw_init
* Description     : Enable and Initialise the USB peripheral.
* Argument        : None
* Return value    : None
******************************************************************************/

static void hw_init( volatile st_usb_object_t *_pchannel )
{
    /* Initialise HAL Config */
    _pchannel->config.auto_stall_clear = TRUE;
    _pchannel->config.bulk_in_no_short_packet = FALSE;

    /* USB SW reset */
    waitmodule(_pchannel);

    /* Configure the pull up on USB pins    */
    p_usb_attach(_pchannel);

    /* Clear Software variables and arrays */
    pipe_tbl_clear(_pchannel);
    mem_tbl_clear(_pchannel);
    mem_clear(_pchannel);

    p_usb_bus_reset(_pchannel);

    /* configure and Reset Endpoints */
    R_USB_HalResetEp(_pchannel, 1);

    /* Enable Interrupts */
    p_enable_int_module(_pchannel);
}
/******************************************************************************
End of function hw_init
******************************************************************************/

/******************************************************************************
* Function Name    : p_disable_int_module
* Description    : Enable the default set of interrupts.
*                  Some interrupts will be enabled later as they are required.
* Argument        : None
* Return value    : None
*******************************************************************************/

static void p_disable_int_module(volatile st_usb_object_t *_pchannel)
{
    rza_io_reg_write_16(&_pchannel->phwdevice->INTENB0,  0, ACC_16B_SHIFT, ACC_16B_MASK);

    R_LIB_DisableIntE(_pchannel, PIPE0);
}
/******************************************************************************
End of function p_disable_int_module
******************************************************************************/


/******************************************************************************
* Function Name   : hw_close
* Description     : Enable and Initialise the USB peripheral.
* Argument        : None
* Return value    : None
******************************************************************************/
static void hw_close(volatile st_usb_object_t *_pchannel)
{
    /* USB SW reset */
    waitmodule(_pchannel);

    /* Configure the pull up on USB pins    */
    p_usb_detach(_pchannel);

    /* Clear Software variables and arrays */
    pipe_tbl_clear(_pchannel);
    mem_tbl_clear(_pchannel);
    mem_clear(_pchannel);

    p_usb_bus_reset(_pchannel);

    /* configure and Reset Endpoints */
    R_USB_HalResetEp(_pchannel, 1);

    /* Enable Interrupts */
    p_disable_int_module(_pchannel);
}
/******************************************************************************
End of function hw_close
******************************************************************************/
