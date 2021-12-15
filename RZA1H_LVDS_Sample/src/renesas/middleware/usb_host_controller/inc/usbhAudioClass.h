/*******************************************************************************
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
 * and to discontinue the availability of this software. By using this
 * software, you agree to the additional terms and conditions found by
 * accessing the following link:
 * http://www.renesas.com/disclaimer
*******************************************************************************
* Copyright (C) 2018 Renesas Electronics Corporation. All rights reserved.
 *****************************************************************************/
/******************************************************************************
 * @headerfile     usbhAudioClass.h
 * @brief          Very basic USB Audio Class descriptor parser
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 05.08.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef USBHAUDIOCLASS_H_INCLUDED
#define USBHAUDIOCLASS_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_USB_HOST_AUDIO USB Audio Class
 * @brief Very basic USB Audio Class descriptor parser
 * 
 * @anchor R_SW_PKG_93_USB_HOST_AUDIO_SUMMARY
 * @par Summary
 * 
 * This module contais the interface for a basic USB Audio Class parser.  
 * 
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "drvAcSpeakers.h"

/******************************************************************************
Macro definitions
******************************************************************************/

/* Audio Interface Subclass Codes */
#define USB_AUDIO_SUBCLASS_UNDEFINED 0x00
#define USB_AUDIO_AUDIOCONTR0L      0x01
#define USB_AUDIO_AUDIOSTREAMING    0x02
#define USB_AUDIO_MIDISTREAMING     0x03

#define USB_AUDIO_SAMPLING_FREQ_CONTROL 0x01

/* Audio Interface Protocol Codes */
#define USB_AUDIO_UNDEFINED 0x20

/* Audio class specific descriptors */
#define USB_AUDIO_DESCR_UNDEFINED   0x00
#define USB_AUDIO_DEVICE            0x21
#define USB_AUDIO_CONFIGURATION     0x22
#define USB_AUDIO_STRING            0x23
#define USB_AUDIO_INTERFACE         0x24
#define USB_AUDIO_ENDPOINT          0x25

/* Audio Class-Specific AC Interface Descriptor Subtypes */
#define USB_AUDIO_HEADER            0x01
#define USB_AUDIO_INPUT_TERMINAL    0x02
#define USB_AUDIO_OUTPUT_TERMINAL   0x03
#define USB_AUDIO_MIXER_UNIT        0x04
#define USB_AUDIO_SELECTOR_UNIT     0x05
#define USB_AUDIO_FEATURE_UNIT      0x06
#define USB_AUDIO_PROCESSING_UNIT   0x07
#define USB_AUDIO_EXTENSION_UNIT    0x08

/* Audio Class-Specific AS Interface Descriptor Subtypes */
#define USB_AUDIO_GENERAL           0x01
#define USB_AUDIO_FORMAT_TYPE       0x02
#define USB_AUDIO_FORMAT_SPECIFIC   0x03

/* Processing Unit Process Types*/
#define USB_AUDIO_UNDEFINED_PROC    0x00
#define USB_AUDIO_UPDOWNMIX_PROC    0x01
#define USB_AUDIO_DB_PROLOGIC_PROC  0x02
#define USB_AUDIO_3D_ST_EXT_PROC    0x03
#define USB_AUDIO_REVERB_PROC       0x04
#define USB_AUDIO_CHORUS_PROC       0x05
#define USB_AUDIO_COMPRESSOR_PROC   0x06

/* Audio Class-Specific Endpoint Descriptor Subtypes */
#define USB_AUDIO_GENERAL           0x01

/* Audio Class-Specific Request Codes */
#define USB_AUDIO_REQUEST_SET_CUR   0x01
#define USB_AUDIO_REQUEST_GET_CUR   0x81
#define USB_AUDIO_REQUEST_SET_MIN   0x02
#define USB_AUDIO_REQUEST_GET_MIN   0x82
#define USB_AUDIO_REQUEST_SET_MAX   0x03
#define USB_AUDIO_REQUEST_GET_MAX   0x83
#define USB_AUDIO_REQUEST_SET_RES   0x04
#define USB_AUDIO_REQUEST_GET_RES   0x84
#define USB_AUDIO_REQUEST_SET_MEM   0x05
#define USB_AUDIO_REQUEST_GET_MEM   0x85
#define USB_AUDIO_REQUEST_GET_STAT  0xFF

/* Terminal Control Selectors */
#define USB_AUDIO_COPY_PROTECT_CTRL 0x01

/* Feature Unit Control Selectors */
#define USB_AUDIO_MUTE_CTRL         0x01
#define USB_AUDIO_VOLUME_CTRL       0x02
#define USB_AUDIO_BASS_CTRL         0x03
#define USB_AUDIO_MID_CTRL          0x04
#define USB_AUDIO_TREBLE_CTRL       0x05
#define USB_AUDIO_GRAPHIC_EQ_CTRL   0x06
#define USB_AUDIO_AUTO_GAIN_CTRL    0x07
#define USB_AUDIO_DELAY_CTRL        0x08
#define USB_AUDIO_BASS_BOOST_CTRL   0x09
#define USB_AUDIO_LOUDNESS_CTRL     0x0A

/* Up/Down-mix Processing Unit Control Selectors */
#define USB_AUDIO_MIX_ENABLE        0x01
#define USB_AUDIO_MODE_SELECT_CTRL  0x02

/* Dolby Prologic Processing Unit Control Selectors */
#define USB_AUDIO_DOLBY_ENABLE      0x01
#define USB_AUDIO_MODE_SELECT_CTRL  0x02

/* Extender Processing Unit Control Selectors */
#define USB_AUDIO_3D_ENABLE         0x01
#define USB_AUDIO_SPACIOUSNESS_CTRL 0x03

/* Reverberation Processing Unit Control Selectors */
#define USB_AUDIO_REVERB_ENABLE     0x01
#define USB_AUDIO_REVERB_LEVEL_CTRL 0x02
#define USB_AUDIO_REVERB_TIME_CTRL  0x03
#define USB_AUDIO_REVERB_FEEDBACK_CTRL 0x04

/* Chorus Processing Unit Control Selectors */
#define USB_AUDIO_CHORUS_ENABLE     0x01
#define USB_AUDIO_CHORUS_LEVEL_CTRL 0x02
#define USB_AUDIO_CHORUS_RATE_CTRL  0x03

/* Control Selector Value */
#define USB_AUDIO_CHORUS_DEPTH_CTRL 0x04

/* Dynamic Range Compressor Processing Unit Control Selectors */
#define USB_AUDIO_COMP_ENABLE       0x01
#define USB_AUDIO_COMP_RATIO_CTRL   0x02
#define USB_AUDIO_COMP_LIMIT_CTRL   0x03
#define USB_AUDIO_COMP_THRES_CTRL   0x04
#define USB_AUDIO_COMP_ATTACK_TIME  0x05
#define USB_AUDIO_COMP_RELEASE_TIME 0x06

/* Extension Unit Control Selectors */
#define USB_AUDIO_XU_ENABLE         0x01

/* Endpoint Control Selectors */
#define USB_AUDIO_SAMP_FREQ_CTRL    0x01
#define USB_AUDIO_PITCH_CTRL        0x02

/* The control bit map flags */
#define USB_AUDIO_CTL_BM_MUTE       BIT_0
#define USB_AUDIO_CTL_BM_VOLUME     BIT_1
#define USB_AUDIO_CTL_BM_BASS       BIT_2
#define USB_AUDIO_CTL_BM_MID        BIT_3
#define USB_AUDIO_CTL_BM_TREBLE     BIT_4
#define USB_AUDIO_CTL_BM_EQ         BIT_5
#define USB_AUDIO_CTL_BM_AGC        BIT_6
#define USB_AUDIO_CTL_BM_DELAY      BIT_7
#define USB_AUDIO_CTL_BM_BASS_BOOST BIT_8
#define USB_AUDIO_CTL_BM_LOUDNESS   BIT_9

/* The control interface Class-Specific Request Codes */
#define USB_AUDIO_CTL_SET_CUR       0x01
#define USB_AUDIO_CTL_GET_CUR       0x81
#define USB_AUDIO_CTL_SET_MIN       0x02
#define USB_AUDIO_CTL_GET_MIN       0x82
#define USB_AUDIO_CTL_SET_MAX       0x03
#define USB_AUDIO_CTL_GET_MAX       0x83
#define USB_AUDIO_CTL_SET_RES       0x04
#define USB_AUDIO_CTL_GET_RES       0x84
#define USB_AUDIO_CTL_SET_MEM       0x05
#define USB_AUDIO_CTL_GET_MEM       0x85
#define USB_AUDIO_CTL_GET_STAT      0xFF

/* The control interface Feature Unit Control Selectors */
#define USB_AUDIO_CTL_MUTE          0x01
#define USB_AUDIO_CTL_VOLUME        0x02
#define USB_AUDIO_CTL_BASS          0x03
#define USB_AUDIO_CTL_MID           0x04
#define USB_AUDIO_CTL_TREBLE        0x05
#define USB_AUDIO_CTL_EQ            0x06
#define USB_AUDIO_CTL_AGC           0x07
#define USB_AUDIO_CTL_DELAY         0x08
#define USB_AUDIO_CTL_BASS_BOOST    0x09
#define USB_AUDIO_CTL_LOUDNESS      0x0A

/******************************************************************************
Typedef definitions
******************************************************************************/

/* NOTE: Only a subset of the Audio Class & Audio Streaming (AC,AS) descriptors
         are defined here */

#pragma pack(1)

/** The Audio Class Interface Header Descriptor */
typedef struct
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDescriptorSubType;
    uint16_t bcdADC;
    uint16_t wTotalLength;
    uint8_t  bInCollection;
    uint8_t  baInterfaceNr;

} UABACHD,
*PUABACHD;

/** The Audio Class Feature Unit Descriptor */
typedef struct
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDescriptorSubtype;
    uint8_t  bUnitID;
    uint8_t  bSourceID;
    uint8_t  bControlSize;
    /** This is the first member in an array */
    uint8_t  bmaControls;
    /** iFeature - String descriptor index not in structure due to variable
                  position */
} USBACFU,
*PUSBACFU;

/** The Audio Class Input Terminal Descriptor */
typedef struct
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDescriptorSubtype;
    uint8_t  bTerminalID;
    uint16_t wTerminalType;
    uint8_t  bAssocTerminal;
    uint8_t  bNrChannels;
    uint16_t wChannelConfig;
    uint8_t  iChannelNames;
    uint8_t  iTerminal;

} USBACIT,
*PUSBACIT;

/** The Audio Class Output Terminal Descriptor */
typedef struct
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDescriptorSubtype;
    uint8_t  bTerminalID;
    uint16_t wTerminalType;
    uint8_t  bAssocTerminal;
    uint8_t  bSourceID;
    uint8_t  iTerminal;

} USBACOT,
*PUSBACOT;

/** Class-Specific AS Interface Descriptor */
typedef struct
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDescriptorSubType;
    uint8_t  bTerminalLink;
    uint8_t  bDelay;
    uint16_t wFormatTag;

} USBASID,
*PUSBASID;

/** The Type 1 Format Type Descriptor */
typedef struct
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDescriptorSubType;
    uint8_t  bFormatType;
    uint8_t  bNrChannels;
    uint8_t  bSubframeSize;
    uint8_t  bBitResolution;
    uint8_t  bSamFreqType;
    uint8_t  bSamFreq;

} USBASTD,
*PUSBASTD;

/** The Standard AS Isochronous Synch Endpoint Descriptor */
typedef struct
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
    uint8_t  bRefresh;
    uint8_t  bSyncAddress;

} USBASEP,
*PUSBASEP;

#pragma pack()

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Description:   Function to check the configuration descriptor
 * @param[in]     pDevice:           Pointer to the device information
 * @param[in]     pbyConfig:         Pointer to the configuration descriptor
 * @param[in]     bfCreateInterface: Flag set true to create interface
 *                                   information
 * @param[out]    ppDeviceDriver:    Pointer to the destination driver
 * @param[out]    pAudioInfo:        Pointer to the audio information
 * 
 * @retval        true: if the sample driver will work with the device
*/
extern  _Bool usbhLoadAudioClass(PUSBDI     pDevice,
                                 uint8_t    *pbyConfig,
                                 _Bool      bfCreateInterface,
                                 PDEVICE    *ppDeviceDriver,
                                 PASSERVE48 pAudioInfo);

#ifdef __cplusplus
}
#endif

#endif /* USBHAUDIOCLASS_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
