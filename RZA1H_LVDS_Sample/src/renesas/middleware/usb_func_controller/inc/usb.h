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
* File Name       : usb.h
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      : HEW, Renesas SuperH Standard Tool chain v9.3
* H/W Platform    : RSK2+SH7269
* Description     : USB driver header file
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

#ifndef R_USB_H__
#define R_USB_H__

/******************************************************************************
User Includes (Project Level Includes)
******************************************************************************/
/*    Following header file provides definition USB firmware definition */
#include "usb_firm.h"

/*    Following header file provides definition common to Upper and Low Level USB 
    driver. */
#include "usb_common.h"

/*    Following header file provides definition common to USB Core driver. */
#include "r_usbf_core.h"

/******************************************************************************
Type Definitions
******************************************************************************/

/* configuration Number */
extern const uint16_t  g_util_BitSet[16];

/******************************************************************************
Function Prototypes (Common)
******************************************************************************/

extern void            VBINT_StsClear(volatile st_usb_object_t *_pchannel);
extern void            RESM_StsClear(volatile st_usb_object_t *_pchannel);
extern void            BCHG_StsClear(volatile st_usb_object_t *_pchannel);

extern uint16_t        getBufSize(volatile st_usb_object_t *_pchannel, uint16_t Pipe);
extern uint16_t        getMaxPacketSize(volatile st_usb_object_t *_pchannel, uint16_t Pipe);

extern uint16_t        R_USBF_DataioFPortChange1(volatile st_usb_object_t *_pchannel, uint16_t Pipe, uint16_t fifosel, uint16_t isel);
extern void            R_USBF_DataioFPortChange2(volatile st_usb_object_t *_pchannel, uint16_t Pipe, uint16_t fifosel, uint16_t isel);

/******************************************************************************
Function Prototypes (Common)
******************************************************************************/
extern void            ep_table_index_clear(volatile st_usb_object_t *_pchannel);

extern void            P_INTR_int(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl);
extern void            P_INTN_int(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl);
extern void            P_BEMP_int(volatile st_usb_object_t *_pchannel, uint16_t Status, uint16_t Int_enbl);

extern void            P_usb_resume(volatile st_usb_object_t *_pchannel);

extern uint16_t        ep_to_pipe(volatile st_usb_object_t *_pchannel, uint16_t Dir_Ep);

/* set PIPEn Configuration register */
extern void            R_USB_HalResetEp(volatile st_usb_object_t *_pchannel, uint16_t Con_Num);


#endif     /* R_USB_H__ */
