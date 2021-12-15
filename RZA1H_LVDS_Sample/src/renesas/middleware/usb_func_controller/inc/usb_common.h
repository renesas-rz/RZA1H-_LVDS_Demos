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
* File Name       : usb_common.h
* Version         : 1.00
* Device          : RZA1(H)
* Tool Chain      : HEW, Renesas SuperH Standard Tool chain v9.3
* H/W Platform    : RSK2+SH7269
* Description     : Common USB definitions.
******************************************************************************/

/******************************************************************************
* History         : 12.11.2009 Ver. 1.00 First Release
******************************************************************************/

/******************************************************************************
System Includes (Project Level Includes)
******************************************************************************/
#include <stdio.h> /*printf*/

/******************************************************************************
User Includes (Project Level Includes)
******************************************************************************/
/* Following header file provides rte type definitions. */
#include <stdio.h> /*printf*/
#include "stdint.h"
#include "r_event.h"
#include "trace.h"

#ifndef USB_COMMON_H
#define USB_COMMON_H

#define R_USBF_PMOD_DISABLE (0)
#define R_USBF_PMOD_ENABLE  (1)

#define R_USBF_USE_PMOD (R_USBF_PMOD_ENABLE)


/* Controls definition of byte access for USB registers, may have been defined in inc\iodefine.h */
#define R_USBF_BYTE_SELECT_DISABLE (0)
#define R_USBF_BYTE_SELECT_ENABLE  (1)

/******************************************************************************
Macros Defines
******************************************************************************/
/* define number of actual channels supported in thia device */
#ifndef R_USB_SUPPORTED_CHANNELS
#define R_USB_SUPPORTED_CHANNELS (2)
#endif /* R_USB_SUPPORTED_CHANNELS */

/*A Setup Packet is always 8 bytes*/
#define USB_SETUP_PACKET_SIZE (8)

/* Comment this line out to turn ON module trace in this file */
/* #undef _TRACE_ON_ */

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
Global Variables (Common)
******************************************************************************/
/* Terminal out mode */
#define    PRINT_USE                (0)
#define    PRINT_NOT_USE            (1)
#define    PRINT_MODE               (PRINT_USE)

/* SPEED mode  overridden by channel.hi_speed_enable */
#define    HI_DISABLE               (0)
#define    HI_ENABLE                (1)
#define    SPEED_MODE               (HI_ENABLE)

/* Max pipe error count */
#define    PIPEERR                  (3)

/*Debug messages*/
/*The debug level - decides what level of message will be output*/
#define DEBUG_LEVEL_NONE    (0) /*No Debug*/
#define DEBUG_LEVEL_LOW     (1) /*Low amount of debug info.*/
#define DEBUG_LEVEL_MID     (2) /*Medium amount of debug info.*/
#define DEBUG_LEVEL_HIGH    (3) /*High amount of debug info.*/

/*Set the current level of debug output using one of the #defines above.*/
/*WARNING NOTE: Too high a level of debug message output may cause problems
with some drivers, where they cause the interrupt handling to take too long.*/
#define DEBUG_LEVEL         (DEBUG_LEVEL_NONE)

/*MACROS used to output DEBUG messages depending upon set DEBUG_LEVEL
1. DEBUG_MSG_HIGH:    Output msg if DEBUG_LEVEL is DEBUG_LEVEL_HIGH
2. DEBUG_MSG_MID:     Output msg if DEBUG_LEVEL is DEBUG_LEVEL_MID or higher.
3. DEBUG_MSG_LOW:     Output msg if DEBUG_LEVEL is DEBUG_LEVEL_LOW or higher.*/

/*Only allow debug messages in a debug build*/
#ifndef RELEASE   
    #if DEBUG_LEVEL >= DEBUG_LEVEL_HIGH
        #define DEBUG_MSG_HIGH(x) TRACE(x)
    #else
        #define DEBUG_MSG_HIGH(x)
    #endif

    #if DEBUG_LEVEL >= DEBUG_LEVEL_MID
        #define DEBUG_MSG_MID(x) TRACE(x)
    #else
        #define DEBUG_MSG_MID(x)
    #endif

    #if DEBUG_LEVEL >= DEBUG_LEVEL_LOW
        #define DEBUG_MSG_LOW(x) TRACE(x)
    #else
        #define DEBUG_MSG_LOW(x)
    #endif
#else 
    /*RELEASE */
    #define DEBUG_MSG_HIGH(x)
    #define DEBUG_MSG_MID(x)
    #define DEBUG_MSG_LOW(x)
#endif

#ifndef TRUE
#define TRUE      (1)
#endif

#ifndef FALSE
#define FALSE     (0)
#endif

/******************************************************************************
USB Error Values
******************************************************************************/
#define USB_ERR_OK                  ((usb_err_t)0)
#define USB_ERR_FAIL                ((usb_err_t)-1)
#define USB_ERR_PARAM               ((usb_err_t)-2)
#define USB_ERR_STATE               ((usb_err_t)-3)
#define USB_ERR_BULK_OUT            ((usb_err_t)-4)
#define USB_ERR_BULK_OUT_NO_BUFFER  ((usb_err_t)-5)
#define USB_ERR_CONTROL_OUT         ((usb_err_t)-6)
#define USB_ERR_NOT_CONNECTED       ((usb_err_t)-7)
#define USB_ERR_UNKNOWN_REQUEST     ((usb_err_t)-8)
#define USB_ERR_INVALID_REQUEST     ((usb_err_t)-9)
#define USB_ERR_CANCEL              ((usb_err_t)-10)
#define USB_ERR_BUSY                ((usb_err_t)-11)


/* System Configuration Register */
/* SYSCFG */
/* b10: USB Module clock enable */
#define    BITSCKE                (0x0400u)
/* b7: Hi-speed enable */
#define    BITHSE                (0x0080u)
/* b6: Controller function select  */
#define    BITDCFM                (0x0040u)
/* b5: D- pull down control */
#define    BITDMRPD            (0x0020u)
/* b4: D+ pull up control */
#define    BITDPRPU            (0x0010u)
/* b0: USB module operation enable */
#define    BITUSBE                (0x0001u)

/* System Configuration Status Register */
/* SYSSTS */
/* LNST: D+, D- line status */
/* b5: SOF enable */
#define    BITSOFEN             (0x0020u)
/* b1-0: D+, D- line status  */
#define    BITLNST              (0x0003u)
/* SE1 */
#define      SE1                (0x0003u)
/* K State */
#define      KSTS               (0x0002u)
/* J State */
#define      JSTS               (0x0001u)
/* SE0 */
#define      SE0                (0x0000u)

/* Device State Control Register */
/* DVSTCTR */
/* b8: Remote wakeup */
#define    BITWKUP              (0x0100u)
/* b7: Remote wakeup sense */
#define    BITRWUPE             (0x0080u)
/* b6: USB reset enable */
#define    BITUSBRST            (0x0040u)
/* b5: Resume enable */
#define    BITRESUME            (0x0020u)
/* b4: USB bus enable */
#define    BITUACT              (0x0010u)
/* b1-0: Reset handshake status */
#define    BITRHST              (0x0003u)
/* Hi/Full Decision */
#define      RHDECI             (0x0002u)
/* Hi-Speed mode */
#define      HSMODE             (0x0003u)
/* Full-Speed mode */
#define      FSMODE             (0x0002u)
/* Low-Speed mode */
#define      LSMODE             (0x0001u)
/* HS handshake is processing */
#define      HSPROC             (0x0004u)

/* Test Mode Register */
/* TESTMODE */
/* UTST: Test select */
/* b3-0: Test select */
#define    BITUTST              (0x000Fu)
/* HOST TEST Packet */
#define      H_TST_PACKET       (0x000Cu)
/* HOST TEST SE0 NAK */
#define      H_TST_SE0_NAK      (0x000Bu)
/* HOST TEST K */
#define      H_TST_K            (0x000Au)
/* HOST TEST J */
#define      H_TST_J            (0x0009u)
/* HOST Normal Mode */
#define      H_TST_NORMAL       (0x0000u)
/* PERI TEST Packet */
#define      P_TST_PACKET       (0x0004u)
/* PERI TEST SE0 NAK */
#define      P_TST_SE0_NAK      (0x0003u)
/* PERI TEST K */
#define      P_TST_K            (0x0002u)
/* PERI TEST J */
#define      P_TST_J            (0x0001u)
/* PERI Normal Mode */
#define      P_TST_NORMAL       (0x0000u)

/* Data Pin Configuration Register */
/* CFBCFG */
/* D0FBCFG */
/* D1FBCFG */
/* FEND: Big endian mode */
/* b9: TENDE Transfer end DMA enable */
#define    BITTENDE             (0x0010u)
/* b8: Big endian mode */
#define    BITBEND              (0x0100u)
/* little endian */
#define      BYTE_LITTLE        (0x0000u)
/* big endian */
#define      BYTE_BIG           (0x0100u)
/* b3-0: FIFO wait */
#define    BITFWAIT             (0x000Fu)

/* CFIFO/DxFIFO Port Select Register */
/* CFIFOSEL */
/* D0FIFOSEL */
/* D1FIFOSEL */
/* b15: Read count mode */
#define    BITRCNT              (0x8000u)
/* b14: Buffer rewind */
#define    BITREW               (0x4000u)
/* b13: DMA buffer clear mode */
/* (D0FIFOSEL,D1FIFOSEL) */
#define    BITDCLRM             (0x2000u)

/* b12: DREQ output enable */
/* (D0FIFOSEL,D1FIFOSEL) */
#define    BITDREQE             (0x1000u)
/* b11,b10: Maximum bit width for FIFO access */
#define    BITMBW               (0x0C00u)
/*  8bit */
#define      MBW_8              (0x0000u)
/* 16bit */
#define      MBW_16             (0x0001u)
/* 32bit */
#define      MBW_32             (0x0002u)
/*  8bit */
#define    BITMBW_8             (0x0000u)
/* 16bit */
#define    BITMBW_16            (0x0400u)
/* 32bit */
#define    BITMBW_32            (0x0800u)

/* b5: DCP FIFO port direction select */
#define    BITISEL              (0x0020u)
/* b3-0: PIPE select */
#define    BITCURPIPE           (0x000Fu)

/* CFIFO/DxFIFO Port Control Register */
/* CFIFOCTR */
/* D0FIFOCTR */
/* D1FIFOCTR */
/* b15: Buffer valid flag */
#define    BITBVAL              (0x8000u)
/* b14: Buffer clear */
#define    BITBCLR              (0x4000u)
/* b13: FIFO ready */
#define    BITFRDY              (0x2000u)
/* b11-0: FIFO received data length */
#define    BITDTLN              (0x0FFFu)


/* Interrupt Enable Register 0 */
/* INTENB0 */
/* b15: VBUS interrupt */
#define    BITVBSE              (0x8000u)
/* b14: Resume interrupt */
#define    BITRSME              (0x4000u)
/* b13: Frame update interrupt */
#define    BITSOFE              (0x2000u)
/* b12: Device state transition interrupt */
#define    BITDVSE              (0x1000u)
/* b11: Control transfer stage transition interrupt */
#define    BITCTRE              (0x0800u)
/* b10: Buffer empty interrupt */
#define    BITBEMPE             (0x0400u)
/* b9: Buffer not ready interrupt */
#define    BITNRDYE             (0x0200u)
/* b8: Buffer ready interrupt */
#define    BITBRDYE             (0x0100u)

/* Interrupt Enable Register 1 */
/* INTENB1 */
/* b14: USB us chenge interrupt */
#define    BITBCHGE             (0x4000u)
/* b12: Detach sense interrupt */
#define    BITDTCHE             (0x1000u)
/* b11: atach sense interrupt */
#define    BITATTCHE            (0x0800u)
/* b6: EOF Error Detection error sense interrupt */
#define    BITEOFERRE           (0x0040u)
/* b5: SETUP IGNORE interrupt */
#define    BITSIGNE             (0x0020u)
/* b4: SETUP ACK interrupt */
#define    BITSACKE             (0x0010u)
/* b2: BRDY clear timing */
#define    BITBRDYM             (0x0004u)

/* Interrupt Status Register 0 */
/* INTSTS0 */
/* b15: VBUS interrupt INTSTS0 */
#define    BITVBINT             (0x8000u)
/* b14: Resume interrupt */
#define    BITRESM              (0x4000u)
/* b13: SOF frame update interrupt */
#define    BITSOFR              (0x2000u)
/* b12: Device state transition interrupt */
#define    BITDVST              (0x1000u)
/* b11: Control transfer stage transition interrupt */
#define    BITCTRT              (0x0800u)
/* b10: Buffer empty interrupt */
#define    BITBEMP              (0x0400u)
/* b9: Buffer not ready interrupt */
#define    BITNRDY              (0x0200u)
/* b8: Buffer ready interrupt */
#define    BITBRDY              (0x0100u)
/* b7: VBUS input port */
#define    BITVBSTS             (0x0080u)
/* b6-4: Device state */
#define    BITDVSQ              (0x0070u)
/* Suspend Configured */
#define      DS_SPD_CNFG        (0x0070u)
/* Suspend Address */
#define      DS_SPD_ADDR        (0x0060u)
/* Suspend Default */
#define      DS_SPD_DFLT        (0x0050u)
/* Suspend Powered */
#define      DS_SPD_POWR        (0x0040u)
/* Suspend */
#define      DS_SUSP            (0x0040u)
/* Configured */
#define      DS_CNFG            (0x0030u)
/* Address */
#define      DS_ADDS            (0x0020u)
/* Default */
#define      DS_DFLT            (0x0010u)
/* Powered */
#define      DS_POWR            (0x0000u)
/* b5-4: Device state */
#define    BITDVSQS             (0x0030u)
/* b3: Setup packet detected flag */
#define    BITVALID             (0x0008u)
/* b2-0: Control transfer stage */
#define    BITCTSQ              (0x0007u)
/* Sequence error */
#define      CS_SQER            (0x0006u)
/* Control write nodata status stage */
#define      CS_WRND            (0x0005u)
/* Control write status stage */
#define      CS_WRSS            (0x0004u)
/* Control write data stage */
#define      CS_WRDS            (0x0003u)
/* Control read status stage */
#define      CS_RDSS            (0x0002u)
/* Control read data stage */
#define      CS_RDDS            (0x0001u)
/* Idle or setup stage */
#define      CS_IDST            (0x0000u)

/* Interrupt Status Register 1 */
/* INTSTS1 */
/* b14: USB bus chenge interrupt */
#define    BITBCHG              (0x4000u)
/* b12: Detach sense interrupt */
#define    BITDTCH              (0x1000u)
/* b12: atach sense interrupt */
#define    BITATTCH             (0x0800u)
/* b12: EOF Error sense interrupt */
#define    BITEODERRE           (0x0040u)
/* b5: Setup ignore interrupt */
#define    BITSIGN              (0x0020u)
/* b4: Setup acknowledge interrupt */
#define    BITSACK              (0x0010u)

/* BRDY Interrupt Enable/Status Register */
/* BRDYENB */
/* BRDYSTS */
/* b9: PIPE9 */
#define    BITBRDY9             (0x0200u)
/* b8: PIPE8 */
#define    BITBRDY8             (0x0100u)
/* b7: PIPE7 */
#define    BITBRDY7             (0x0080u)
/* b6: PIPE6 */
#define    BITBRDY6             (0x0040u)
/* b5: PIPE5 */
#define    BITBRDY5             (0x0020u)
/* b4: PIPE4 */
#define    BITBRDY4             (0x0010u)
/* b3: PIPE3 */
#define    BITBRDY3             (0x0008u)
/* b2: PIPE2 */
#define    BITBRDY2             (0x0004u)
/* b1: PIPE1 */
#define    BITBRDY1             (0x0002u)
/* b1: PIPE0 */
#define    BITBRDY0             (0x0001u)

/* NRDY Interrupt Enable/Status Register */
/* NRDYENB */
/* NRDYSTS */
/* b9: PIPE9 */
#define    BITNRDY9             (0x0200u)
/* b8: PIPE8 */
#define    BITNRDY8             (0x0100u)
/* b7: PIPE7 */
#define    BITNRDY7             (0x0080u)
/* b6: PIPE6 */
#define    BITNRDY6             (0x0040u)
/* b5: PIPE5 */
#define    BITNRDY5             (0x0020u)
/* b4: PIPE4 */
#define    BITNRDY4             (0x0010u)
/* b3: PIPE3 */
#define    BITNRDY3             (0x0008u)
/* b2: PIPE2 */
#define    BITNRDY2             (0x0004u)
/* b1: PIPE1 */
#define    BITNRDY1             (0x0002u)
/* b1: PIPE0 */
#define    BITNRDY0             (0x0001u)

/* BEMP Interrupt Enable/Status Register */
/* BEMPENB */
/* BEMPSTS */
/* b9: PIPE9 */
#define    BITBEMP9             (0x0200u)
/* b8: PIPE8 */
#define    BITBEMP8             (0x0100u)
/* b7: PIPE7 */
#define    BITBEMP7             (0x0080u)
/* b6: PIPE6 */
#define    BITBEMP6             (0x0040u)
/* b5: PIPE5 */
#define    BITBEMP5             (0x0020u)
/* b4: PIPE4 */
#define    BITBEMP4             (0x0010u)
/* b3: PIPE3 */
#define    BITBEMP3             (0x0008u)
/* b2: PIPE2 */
#define    BITBEMP2             (0x0004u)
/* b1: PIPE1 */
#define    BITBEMP1             (0x0002u)
/* b0: PIPE0 */
#define    BITBEMP0             (0x0001u)

/* Frame Number Register */
/* FRMNUM */
/* b15: Overrun error */
#define    BITOVRN              (0x8000u)
/* b14: Received data error */
#define    BITCRCE              (0x4000u)
/* b10-0: Frame number */
#define    BITFRNM              (0x07FFu)

/* Micro Frame Number Register */
/* UFRMNUM */
/* b2-0: Micro frame number */
#define    BITUFRNM             (0x0007u)

/* USB Request Value Register */
/* USBVAL */
#define    DT_TYPE              (0xFF00u)
#define    DT_INDEX             (0x00FFu)
#define    CONF_NUM             (0x00FFu)
#define    ALT_SET              (0x00FFu)

/* USB Request Index Register */
/* USBINDX */
/* b15-0: wIndex */
#define    WMASK_INDEX          (0xFFFFu)
/* b15-b8: Test Mode Selectors */
#define    TEST_SELECT          (0xFF00u)
/* Test_J */
#define      TEST_J             (0x0100u)
/* Test_K */
#define      TEST_K             (0x0200u)
/* Test_SE0_NAK */
#define      TEST_SE0_NAK       (0x0300u)
/* Test_Packet */
#define      TEST_PACKET        (0x0400u)
/* Test_Force_Enable */
#define      TEST_FORCE_ENABLE  (0x0500u)
/* Standard test selectors */
#define      TEST_STSelectors   (0x0600u)
/* Reserved */
#define      TEST_Reserved      (0x4000u)
/* Vendor-specific test modes */
#define      TEST_VSTModes      (0xC000u)
/* b7: Endpoint Direction */
#define    EP_DIR               (0x0080u)
#define      EP_DIR_IN          (0x0080u)
#define      EP_DIR_OUT         (0x0000u)

/* Default Control Pipe Maxpacket Size Register */
/* DCPMAXP */
/* DEVSEL: Device address select */
/* Device address 0 */
#define      DEVICE_0           (0x0000u)
/* Device address 1 */
#define      DEVICE_1           (0x4000u)
/* Device address 2 */
#define      DEVICE_2           (0x8000u)
/* Device address 3 */
#define      DEVICE_3           (0xA000u)
/* b6-0: Maxpacket size of default control pipe */
#define      BITDCPMXPS         (0x007Fu)


/* Default Control Pipe Configuration Register */
/* DCPCFG PIPECFG*/
/* Pipe Configuration Register */
/* PIPECFG */
/* b15-14: Transfer type */
#define    BITTYPE              (0xC000u)
/* b10: Buffer ready interrupt mode select */
#define    BITBFRE              (0x0400u)
/* b9: Double buffer mode select */
#define BITDBLB                 (0x0200u)
/* b8: Continuous transfer mode select */
#define    BITCNTMD             (0x0100u)
/* b7: DCP Transfer end NAK */
#define    BITSHTNAK            (0x0080u)
/* b4: Control transfer DIR select */
#define    BITDIR               (0x0010u)
/* b3-0: Endpoint number select */
#define    BITEPNUM             (0x000Fu)

/* Default Control Pipe Control Register */
/* DCPCTR */
/* Pipex Control Register */
/* PIPE1CTR */
/* PIPE2CTR */
/* PIPE3CTR */
/* PIPE4CTR */
/* PIPE5CTR */
/* PIPE6CTR */
/* PIPE7CTR */
/* b15: Buffer status */
#define    BITBSTS              (0x8000u)
/* b14: Send USB request  */
#define    BITSUREQ             (0x4000u)
/* b14: IN buffer monitor (Only for PIPE1 to 5) */
#define    BITINBUFM            (0x4000u)
/* b13: Status clear for spilt transcation */
#define    BITCSCLR             (0x2000u)
/* b12: Complete spilt Status */
#define    BITCSSTS             (0x0200u)
/* b10: auto responce mode */
#define    BITATREPM            (0x0400u)
/* b9: Out buffer auto clear mode */
#define    BITACLRM             (0x0200u)
/* b8: Sequence toggle bit clear */
#define    BITSQCLR             (0x0100u)
/* b7: Sequence toggle bit set */
#define    BITSQSET             (0x0080u)
/* b6: Sequence toggle bit monitor */
#define    BITSQMON             (0x0040u)
/* b5: Pipe Busy */
#define    BITBUSY              (0x0020u)
/* b4: PING token Issue enable  */
#define    BITPINGE             (0x0010u)
/* b2: Enable control transfer complete */
#define    BITCCPL              (0x0004u)
/* b1-0: Response PID */
#define    BITPID               (0x0003u)

/* PID: Response PID */
/* STALL */
#define      PID_STALL3         (0x0003u)
#define      PID_STALL          (0x0002u)
/* BUF */
#define      PID_BUF            (0x0001u)
/* NAK */
#define      PID_NAK            (0x0000u)

/* Pipe Window Select Register */
/* PIPESEL */
/* b3-0: Pipe select */
#define    BITPIPESEL           (0x000Fu)
/* Pipe Buffer Configuration Register */
/* PIPEBUF */
/* b14-10: Pipe buffer size */
#define    BITBUFSIZE           (0x7C00u)
/* b6-0: Pipe buffer number */
#define    BITBUFNMB            (0x007Fu)
#define    PIPE0BUF             (256u)
#define    PIPEXBUF             (64u)

/* Pipe Maxpacket Size Register */
/* PIPEMAXP */
/* b15-12: Device select */
#define    BITDEVSEL            (0xF000u)
/* b10-0: Maxpacket size */
#define    BITMXPS              (0x07FFu)

/* Pipe Cycle Configuration Register */
/* PIPEPERI */
/* b12: Isochronous in-buffer flush mode select */
#define    BITIFIS              (0x1000u)
/* b2-0: Isochronous interval */
#define    BITIITV              (0x0007u)

/* interrupt level USB */
#define USBF_INTERRUPT_PRIORITY (ISR_USBF_IRQ_PRIORITY)

/******************************************************************************
Type Definitions
******************************************************************************/
typedef uint8_t BOOL;
typedef int16_t usb_err_t;

typedef struct
{
    /* Set TRUE if don't need a BULK IN to end on a short packet. */
    /* Mass Storage doesn't use short packets to end a BULK IN.
    Default FALSE*/
    BOOL bulk_in_no_short_packet;

    /* m_bAutoStall: Set FALSE to prevent a stall condition
    being automatically cleared after a stall has been issued.
    Default TRUE*/
    BOOL auto_stall_clear;
}st_usb_hal_config_t;

extern volatile struct st_usb_n *USB;
extern PEVENT          g_usb_devices_events[R_USB_SUPPORTED_CHANNELS];

#endif /*USB_COMMON_H*/
