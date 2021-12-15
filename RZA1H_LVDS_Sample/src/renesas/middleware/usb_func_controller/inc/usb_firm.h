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
* File Name       : usb_firm.h
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      : HEW, Renesas SuperH Standard Tool chain v9.3
* H/W Platform    : RSK2+SH7269
* Description     : Define USB Module Value.
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

#ifndef USB_FIRM_H_INCLUDED
#define USB_FIRM_H_INCLUDED

/******************************************************************************
Macro Defines
******************************************************************************/
#define    USB_ON                 (1)
#define    USB_OFF                (0)

#define    USB_YES                (1)
#define USB_NO                    (0)

#define    USB_SUCCESS            (0)
#define    USB_ERROR              (-1)

/* Pipe define */
/* PIPE0 ... PIPE9 */
#define    MAX_PIPE_NO            (9u)
/* PIPE 0 */
#define      PIPE0                ((uint16_t)0x0000)
/* PIPE 1 */
#define      PIPE1                ((uint16_t)0x0001)
/* PIPE 2 */
#define      PIPE2                ((uint16_t)0x0002)
/* PIPE 3 */
#define      PIPE3                ((uint16_t)0x0003)
/* PIPE 4 */
#define      PIPE4                ((uint16_t)0x0004)
/* PIPE 5 */
#define      PIPE5                ((uint16_t)0x0005)
/* PIPE 6 */
#define      PIPE6                ((uint16_t)0x0006)
/* PIPE 7 */
#define      PIPE7                ((uint16_t)0x0007)
/* PIPE 8 */
#define      PIPE8                ((uint16_t)0x0008)
/* PIPE 9 */
#define      PIPE9                ((uint16_t)0x0009)
/* Undecided pipe number */
#define      NOPIPE               ((uint16_t)0x00FF)
#define      USEPIPE              ((uint16_t)0x00FE)

/* Pipe configuration table define */
/* Pipe configuration table length */
#define      EPL                  (5u)
/* b15-14: Transfer type */
#define      TYPFIELD             (0xC000u)
/* Isochronous */
#define      ISO                  (0xC000u)
/* Interrupt */
#define      INT                  (0x8000u)
/* Bulk */
#define      BULK                 (0x4000u)

/* b10: Buffer ready interrupt mode select */
#define      BFREFIELD            (0x0400u)
#define      BFREON               (0x0400u)
#define      BFREOFF              (0x0000u)

/* b9: Double buffer mode select */
#define      DBLBFIELD            (0x0200u)
#define      DBLBON               (0x0200u)
#define      DBLBOFF              (0x0000u)

/* b8: Continuous transfer mode select */
#define      CNTMDFIELD           (0x0100u)
#define      CNTMDON              (0x0100u)
#define      CNTMDOFF             (0x0000u)

/* b7: Transfer end NAK */
#define      SHTNAKFIELD          (0x0080u)
#define      SHTNAKON             (0x0080u)
#define      SHTNAKOFF            (0x0000u)

/* b4: Transfer direction select */
#define      DIRFIELD             (0x0010u)
/* HOST OUT */
#define      DIR_H_OUT            (0x0010u)
/* PERI IN */
#define      DIR_P_IN             (0x0010u)
/* HOST IN */
#define      DIR_H_IN             (0x0000u)
/* PERI OUT */
#define      DIR_P_OUT            (0x0000u)
/* b3-0: Endpoint number select */
#define      EPNUMFIELD           (0x000Fu)
/* EP0 EP1 ... EP15 */
#define      MAX_EP_NO            (9u)

/* Endpoints     */
#define      EP0                  (0x0000u)
#define      EP1                  (0x0001u)
#define      EP2                  (0x0002u)
#define      EP3                  (0x0003u)
#define      EP4                  (0x0004u)
#define      EP5                  (0x0005u)
#define      EP6                  (0x0006u)
#define      EP7                  (0x0007u)
#define      EP8                  (0x0008u)
#define      EP9                  (0x0009u)
#define      EP10                 (0x000Au)
#define      EP11                 (0x000Bu)
#define      EP12                 (0x000Cu)
#define      EP13                 (0x000Du)
#define      EP14                 (0x000Eu)
#define      EP15                 (0x000Fu)

/*  Pipe Status  */
#define      PIPE_IDLE            (0x00)
#define      PIPE_WAIT            (0x01)
#define      PIPE_DONE            (0x02)
#define      PIPE_NORES           (0x03)
#define      PIPE_STALL           (0x04)

#define      BUF_READY            (0)
#define      BUF_BUSY             (1)

#define      FULL_SPEED           (0)
#define      HI_SPEED             (1)

#define      FIFO_USE             (0x3000)
#define      C_FIFO_USE           (0x0000)
#define      D0_FIFO_USE          (0x1000)
#define      D1_FIFO_USE          (0x2000)

#define      BUF_SIZE(x)          ((uint16_t)((uint16_t)(((x) / 64) - 1) << 10))

/* Device connect information */
#define      ATTACH               ((uint16_t)0x01)
#define      DETACH               ((uint16_t)0x00)

/* FIFO port & access define */
/* CFIFO CPU transfer */
#define      CUSE                 ((uint16_t)0)
/* D0FIFO CPU transfer */
#define      D0USE                ((uint16_t)1)
/* D0FIFO DMA transfer */
#define      D0DMA                ((uint16_t)2)
/* D1FIFO CPU transfer */
#define      D1USE                ((uint16_t)3)
/* D1FIFO DMA transfer */
#define      D1DMA                ((uint16_t)4)

/* Peripheral defines */
/* Configuration Descriptor  DEFINE */
/* Reserved(set to 1) */
#define      CF_RESERVED          (0x80)
/* Self Powered */
#define      CF_SELF              (0x40)
/* Remote Wakeup */
#define      CF_RWUP              (0x20)

/* Endpoint Descriptor    DEFINE */
/* Out Endpoint */
#define      EP_OUT               (0x00)
/* In  Endpoint */
#define      EP_IN                (0x80)
/* Control       Transfer */
#define      EP_CNTRL             (0x00)
/* Isochronous Transfer */
#define      EP_ISO               (0x01)
/* Bulk           Transfer */
#define      EP_BULK              (0x02)
/* Interrupt   Transfer */
#define      EP_INT               (0x03)

/* index of bMAXPacketSize */
#define      DEV_MAX_PKT_SIZE     (7)
/* index of bNumConfigurations */
#define      DEV_NUM_CONFIG       (17)

#define      ALT_NO               (255)

#define      EP_ERROR             (0xFF)
#define      SOFTWARE_CHANGE      (0)

/* FIFO read / write result */
/* FIFO not ready */
#define      FIFOERROR            ((uint16_t)USB_ERROR)
/* end of write ( but packet may not be outputting ) */
#define      WRITEEND             ((uint16_t)0x00)
/* end of write ( send short packet ) */
#define      WRITESHRT            ((uint16_t)0x01)
/* write continues */
#define      WRITING              ((uint16_t)0x02)
/* end of read */
#define      READEND              ((uint16_t)0x00)
/* insufficient ( receive short packet ) */
#define      READSHRT             ((uint16_t)0x01)
/* read continues */
#define      READING              ((uint16_t)0x02)
/* buffer size over */
#define      READOVER             ((uint16_t)0x03)

/******************************************************************************
Global Variables
******************************************************************************/


#endif        /* USB_FIRM_H_INCLUDED    */
