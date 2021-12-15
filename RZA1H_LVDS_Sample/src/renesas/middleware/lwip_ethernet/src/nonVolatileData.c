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
* File Name    : nonVolatileData.c
* Version      : 1.01
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : None
* H/W Platform : Renesas
* Description  : Functions to store and recall data from non-volatile memory
*******************************************************************************
* History      : DD.MM.YYYY Version Description
*              : 04.02.2010 1.00    First Release
*              : 10.06.2010 1.01    Updated type definitions
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

/*
    The EEPROM is used to hold the Media Access Control Address (MAC) for
    the Ethernet controller. This address is programmed during production
    into the first 7 bytes of the memory.

    The remainder of the memory can be used for storing other non volatile
    parameters. In this case the first two bytes of each data entry contain
    a the length and the type of the data. The first entry on the list is
    positioned at NV_USER_DATA_START_OFFSET.

    The restrictions are that there may not be more than 254 data types and
    the maximum length of data is 246 bytes.
*/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "nonVolatileData.h"

/******************************************************************************
Typedef definitions
******************************************************************************/
/* Define the structure of the data header */
typedef struct _NVHDR
{
    uint8_t         byLength;
    uint8_t         byDataType;

} NVHDR,
*PNVHDR;

/******************************************************************************
Macro definitions
******************************************************************************/

#define NV_USER_DATA_START_OFFSET   8UL
                                    /* Data header + 16 bit CRC */
#define NV_MIN_DATA_LENGTH          (sizeof(NVHDR) + sizeof(uint16_t))
                                    /* 0x0 and OxFF are reserved as list
                                       termination values */
#define NV_MAX_DATA_LENGTH          0xFE

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/* The data tables for the CRC function */
const uint8_t       gpbyC16l[] =
{
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
};

const uint8_t       gbyC16h[] =
{
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2,
    0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04,
    0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
    0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8,
    0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
    0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6,
    0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10,
    0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE,
    0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
    0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA,
    0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C,
    0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0,
    0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62,
    0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
    0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE,
    0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
    0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C,
    0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76,
    0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54,
    0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
    0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98,
    0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A,
    0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86,
    0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40,
};

/******************************************************************************
Function Prototypes
******************************************************************************/

static int nvStoreData(NVDT nvDataType, const void  *pvSrc, size_t stLength, FILE *pFile);
static uint16_t vnCalcCrC(const uint8_t *pbyData, size_t stLength);

/******************************************************************************
Public Functions
******************************************************************************/

/*****************************************************************************
* Function Name: nvLoad
* Description  : Function to load settings from non volatile memory
* Arguments    : IN  nvDataType - The data type to load
*                OUT pvDest - Pointer to the destination
*                IN  stLength - The length of the destination memory
* Return Value : 0 for success -1 on error
*****************************************************************************/
int nvLoad(NVDT nvDataType, void *pvDest, size_t stLength)
{
    FILE *pFile = fopen("\\\\.\\eeprom", "r");

    if (pFile)
    {
        long lFilePos = NV_USER_DATA_START_OFFSET;

        /* Seek to the position where the user data starts */
        if (fseek(pFile, lFilePos, SEEK_SET) == 0)
        {
            /* Until the end of the list */
            while (1)
            {
                /* Get the length of the entry */
                int iLength = fgetc(pFile);

                /* Check that it is valid */
                if ((iLength > (int) NV_MIN_DATA_LENGTH) && (iLength <= NV_MAX_DATA_LENGTH))
                {
                    /* Check for a type and length match */
                    NVDT thisType = fgetc(pFile);

                    if ((thisType == nvDataType) && (stLength >= ((size_t) iLength - NV_MIN_DATA_LENGTH)))
                    {
                        /* A 16 bit CRC is added at the end of the data section */
                        size_t stRead = (size_t) iLength - NV_MIN_DATA_LENGTH;

                        /* Seek to the start of the data */
                        fseek(pFile, lFilePos + (long) sizeof(NVHDR), SEEK_SET);

                        /* Read all but the CRC into the destination memory */
                        if (fread(pvDest, 1UL, stRead, pFile) == stRead)
                        {
                            uint16_t usFileCRC;
                            uint16_t usCalcCRC;

                            /* Get the CRC from the file */
                            fread(&usFileCRC, 1UL, sizeof(uint16_t), pFile);

                            /* Calculate the CRC of the data */
                            usCalcCRC = vnCalcCrC(pvDest, stRead);

                            /* Check the CRC's match */
                            if (usFileCRC == usCalcCRC)
                            {
                                /* Close the file - valid data found */
                                fclose(pFile);
                                return 0;
                            }
                        }
                    }

                    /* Add on the length of this data segment and seek to the next */
                    lFilePos += iLength;

                    /* Seek from the current position to the next entry on the list */
                    if (fseek(pFile, lFilePos, SEEK_SET))
                    {
                        break;
                    }
                }
                else
                {
                    /* Invalid length terminates list & search */
                    break;
                }
            }
        }

        fclose(pFile);
    }

    return -1;
}
/*****************************************************************************
End of function  nvLoad
******************************************************************************/

/*****************************************************************************
* Function Name: nvStore
* Description  : Function to store settings to non volatile memory
* Arguments    : IN  nvDataType - The data type to load
*                IN  pvSrc - Pointer to the data structure to store
*                IN  stLength - The length of the destination memory
* Return Value :
*****************************************************************************/
int nvStore(NVDT nvDataType, const void *pvSrc, size_t stLength)
{
    int iResult = -1;
    FILE *pFile = fopen("\\\\.\\eeprom", "r+");
    uint8_t byData = 0xA5;

    if (pFile)
    {
        long lFilePos = NV_USER_DATA_START_OFFSET;

        /* Seek to the position where the user data starts */
        if (fseek(pFile, lFilePos, SEEK_SET) == 0)
        {
            /* Until the end of the list */
            while (1)
            {
                /* Get the length of the entry */
                int iLength = fgetc(pFile);

                /* Check that it is valid */
                if ((iLength > (int) NV_MIN_DATA_LENGTH) && (iLength <= NV_MAX_DATA_LENGTH))
                {
                    /* Check for a type and length match */
                    NVDT thisType = fgetc(pFile);

                    if ((thisType == nvDataType) &&  (stLength >= ((size_t) iLength - NV_MIN_DATA_LENGTH)))
                    {
                        /* Settings already exist so overwrite them */
                        if (0 == fseek(pFile, lFilePos, SEEK_SET))
                        {
                            iResult = nvStoreData(nvDataType, pvSrc, stLength, pFile);
                        }

                        byData = 0xA5;
                        fseek(pFile, 0UL, SEEK_SET);
                        fwrite(&byData, 1UL, 1UL, pFile);
                        fclose(pFile);
                        return iResult;
                    }
                    else
                    {
                        /* Add on the length of this data segment and seek to the next */
                        lFilePos += iLength;

                        /* Seek from the current position to the next entry on the list */
                        if (fseek(pFile, lFilePos, SEEK_SET))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    /* Invalid length terminates list & search */
                    break;
                }
            }

            /* The entry was not found in the list so add to the end */
            if (fseek(pFile, lFilePos, SEEK_SET) == 0)
            {
                iResult = nvStoreData(nvDataType, pvSrc, stLength, pFile);
            }
        }

        /* The Ethernet controller will only auto-load the MAC address
           if the first byte in the EEROM is 0xA5. Make sure that it is
           always present */
        byData = 0xA5;
        fseek(pFile, 0UL, SEEK_SET);
        fwrite(&byData, 1UL, 1UL, pFile);
        fclose(pFile);
    }

    return iResult;
}
/*****************************************************************************
End of function  nvStore
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/

/*****************************************************************************
* Function Name: nvStoreData
* Description  : Function to write the data to the current fil position
* Arguments    : IN  nvDataType - The data type to load
*                IN  pvSrc - Pointer to the data structure to store
*                IN  stLength - The length of the destination memory
*                IN  pFile - The file to write to
* Return Value : 0 for success -1 on error
*****************************************************************************/
static int nvStoreData(NVDT nvDataType, const void *pvSrc, size_t stLength, FILE *pFile)
{
    uint16_t usCRC = vnCalcCrC(pvSrc, stLength);

    /* Write the length of the data + the length of the header + the length of the CRC */
    fputc((int) (stLength + NV_MIN_DATA_LENGTH), pFile);

    /* Write the type of data - last byte of header */
    fputc(nvDataType, pFile);

    /* Write the data itself */
    if (fwrite(pvSrc, 1UL, stLength, pFile) == stLength)
    {
        /* Write the CRC */
        fwrite(&usCRC, 1UL, sizeof(uint16_t), pFile);

        /* Write to the device now */
        fflush(pFile);
        return 0;
    }

    return -1;
}
/*****************************************************************************
End of function  nvStoreData
******************************************************************************/

/*****************************************************************************
* Function Name: vnCalcCrC
* Description  : Function to calaculate a 16bit CRC check number
* Arguments    : IN  pbyData - Pointer to the data to sum
*                IN  stLength - The length of the data
* Return Value : The 16bit CRC
*****************************************************************************/
static uint16_t vnCalcCrC(const uint8_t *pbyData, size_t stLength)
{
    uint8_t byTableValue;
    uint16_t usCRC = 0;

    while (stLength--)
    {
        byTableValue = (uint8_t) (((uint8_t) usCRC) ^ *pbyData++);
        usCRC = (uint16_t) (((gbyC16h[byTableValue]) << 8)
              + (gpbyC16l[byTableValue] ^ (uint8_t) ((usCRC) >> 8)));
    }

    return usCRC;
}
/*****************************************************************************
End of function  vnCalcCrC
******************************************************************************/

/*****************************************************************************
End  Of File
******************************************************************************/
