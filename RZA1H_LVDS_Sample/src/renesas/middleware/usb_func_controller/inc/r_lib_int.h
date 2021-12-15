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
* File Name       : r_lib_int.h
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      :
* H/W Platform    : RSK2+RZA!H
* Description     : Define USB library functions and variables
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/


/******************************************************************************
User Includes (Project Level Includes)
******************************************************************************/
/* Following header file provides rte type definitions. */
#include "stdint.h"
/* Following header file provides common defines for widely used items. */
#include "usb_common.h"

#include "usb_firm.h"

#ifndef R_LIB_INT_H_INCLUDED
#define R_LIB_INT_H_INCLUDED


/******************************************************************************
Function Prototypes for function mode
******************************************************************************/
extern void            mem_clear(volatile st_usb_object_t *_pchannel);
extern void            resetmodule(volatile st_usb_object_t *_pchannel);

extern uint16_t        get_bus_speed(volatile st_usb_object_t *_pchannel);

extern void            delay_us(uint16_t Dcnt);
extern void            delay_ms(uint16_t Dcnt);

extern void            R_USBF_DataioReceiveStart(volatile st_usb_object_t *_pchannel,
                                     uint16_t  _pipe,
                                     uint32_t  _bsize,
                                     uint8_t * _ptable);

extern uint16_t        Send_Start(volatile st_usb_object_t *_pchannel,
                                  uint16_t Pipe,
                                  uint32_t Bsize,
                                  uint8_t *Table);

extern uint16_t        R_USBF_DataioBufWrite(volatile st_usb_object_t *_pchannel, uint16_t Pipe);

extern uint16_t        R_USBF_DataioBufRead(volatile st_usb_object_t *_pchannel, uint16_t Pipe);

extern void            R_USBF_DataioReceiveStartC(volatile st_usb_object_t *_pchannel,
                                       uint16_t Pipe,
                                       uint32_t Bsize,
                                       uint8_t *Table);

extern void            R_USBF_DataioReceiveStartD0(volatile st_usb_object_t *_pchannel,
                                        uint16_t Pipe,
                                        uint32_t Bsize,
                                        uint8_t *Table);

extern uint16_t        R_USBF_DataioBufWriteC(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern uint16_t        R_USBF_DataioBufWriteD0(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern uint16_t        R_USBF_DataioBufReadC(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern uint16_t        R_USBF_DataioBufReadD0(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
uint16_t               R_USBF_DataioSendStart(volatile st_usb_object_t *_pchannel, uint16_t Pipe, uint32_t Bsize, uint8_t *Table);
extern void            mem_tbl_clear(volatile st_usb_object_t *_pchannel);
extern void            pipe_tbl_clear(volatile st_usb_object_t *_pchannel);

extern void            R_USBF_DataioCFifoWrite(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count);
extern void            R_USBF_DataioCFifoRead(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count);
extern void            R_USBF_DataioD0FifoWrite(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count);
extern void            R_USBF_DataioD0FifoRead(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count);
extern void            R_USBF_DataioD1FifoWrite(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count);
extern void            R_USBF_DataioD1FifoRead(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t count);

extern void            R_LIB_IntrInt(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl);
extern void            R_LIB_IntnInt(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl);
extern void            R_LIB_BempInt(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl);
extern void            R_LIB_EnableIntR(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_DisableIntR(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_EnableIntE(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_DisableIntE(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_EnableIntN(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_DisableIntN(volatile st_usb_object_t *_pchannel, uint16_t Pipe);


extern uint16_t        R_LIB_GetPid(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_SetBUF(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_SetNAK(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_SetSTALL(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_ClrSTALL(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_DoSQCLR(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_CD2SFIFO(volatile st_usb_object_t *_pchannel, uint16_t pipe);
extern void            R_LIB_CS2DFIFO(volatile st_usb_object_t *_pchannel, uint16_t pipe);
extern void            R_LIB_CFIFOCLR(volatile st_usb_object_t *_pchannel, uint16_t pipe);
extern void            R_LIB_CFIFOCLR2(volatile st_usb_object_t *_pchannel, uint16_t pipe);
extern void            R_LIB_SetACLRM(volatile st_usb_object_t *_pchannel, uint16_t pipe);
extern void            R_LIB_ClrACLRM(volatile st_usb_object_t *_pchannel, uint16_t pipe);
extern void            R_LIB_ClrTRCLR(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_SetTRENB(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_SetTRN(volatile st_usb_object_t *_pchannel, uint16_t Pipe,uint16_t trncnt);
extern void            R_LIB_SetPipeBUFtoSTALL(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_SetPipeSTALLtoBUF(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            R_LIB_SetPipeNACKtoBUF(volatile st_usb_object_t *_pchannel, uint16_t Pipe);

extern void            EnableIntR(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            DisableIntR(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            EnableIntE(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            DisableIntE(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            EnableIntN(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern void            DisableIntN(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
#endif        /* R_LIB_INT_H_INCLUDED*/
