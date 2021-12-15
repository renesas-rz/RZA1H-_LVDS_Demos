/******************************************************************************
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
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer
*******************************************************************************
* Copyright (C) 2012 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************
* File Name    : drvAcSpeakers.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : None
* H/W Platform : RSK+
* Description  : Very basic USB audio class speaker driver
*******************************************************************************
* History      : DD.MM.YYYY Version Description
*              : 05.08.2010 1.00    First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

#ifndef DRVACSPEAKERS_H_INCLUDED
#define DRVACSPEAKERS_H_INCLUDED

/******************************************************************************
Macro definitions
******************************************************************************/

#define USB_SPK_MAX_CHANNELS            3

/* Audio streaming input terminal */
#define USB_AUDIO_AS_IN_TERM        0x0101
/* A generic speaker output terminal */
#define USB_AUDIO_AS_OUT_TERM       0x0301
/* The Format 1 PCM tag */
#define USB_AUDIO_FORMAT1_PCM       0x0001
/* Audio type descriptor Type 1 */
#define USB_AUDIO_FORMAT_TYPE_1     0x01

/******************************************************************************
Typedef definitions
******************************************************************************/

typedef struct _ASSERVE48
{
    /* Pointer to the device information */
    PUSBDI  pDevice;
    /* The streaming audio interface */
    struct
    {
        uint8_t bTerminalLink;
        PUSBEI  pIsocOutEndpoint;
    } Audio;
    /* The audio control information */
    struct
    {
        uint8_t    byInterface;
        struct
        {
            uint16_t   wTerminalType;
            uint16_t   wChannelConfig;
            uint8_t    bTerminalID;
            uint8_t    bAssocTerminal;
        } In;
        struct
        {
            uint16_t   wTerminalType;
            uint8_t    bTerminalID;
            uint8_t    bSourceID;
        } Out;
        struct
        {
            uint8_t    bUnitID;
            uint8_t    bSourceID;
            uint16_t   pwChannel[USB_SPK_MAX_CHANNELS];
        } Feature;
    } Ctl;
    /* The HID interface */
    struct
    {
        PUSBEI  pIntInEndpoint;
        uint8_t byInterface;
    } Hid;
    /* The supported sample rate */
    uint32_t   dwSampleRate;

} ASSERVE48,
*PASSERVE48;

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Function Name: acLoadDriver
Description:   Function to select and start the desired interface.
Arguments:     IN  pAudioInfo - Pointer to the audio interface information
               OUT ppDeviceDriver - Pointer to the destination driver
Return value:  true if the driver will work with the device
******************************************************************************/

extern  _Bool acLoadDriver(PASSERVE48 pAudioInfo, PDEVICE *ppDeviceDriver);

#ifdef __cplusplus
}
#endif

#endif /* DRVACSPEAKERS_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/