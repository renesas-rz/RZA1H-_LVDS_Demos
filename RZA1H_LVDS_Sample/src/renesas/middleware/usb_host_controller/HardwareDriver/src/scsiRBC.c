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
 * File Name    : scsiRBC.c
 * Version      : 1.50
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : SCSI Reduced Block Commands. See document 97-260r2.pdf
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 First Release
 *              : 28.05.2015 1.50 Modified Endian swaps in Read10 & Write10
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/
#include <string.h>

#include "compiler_settings.h"
#include "scsiRBC.h"
#include "endian.h"
#include "Trace.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/

/* The SCSI command codes that are used by Reduced Block Command devices */
#define SCSI_READ_10                0x28
#define SCSI_READ_CAPACITY_10       0x25
#define SCSI_START_STOP             0x1B
/* SCSI_SYNCHRONISE_CACHE: Not used by this code*/
#define SCSI_SYNCHRONISE_CACHE      0x35
#define SCSI_WRITE_10               0x2A
/* SCSI_VERIFY_10: Not used by this code */
#define SCSI_VERIFY_10              0x2F
/* Commands from the SPC-2 requied for RBC devices */
#define SCSI_INQUIRY                0x12
#define SCSI_TEST_UNIT_READY        0x00
#define SCSI_MODE_SENSE_6           0x1A
#define SCSI_REQUEST_SENSE          0x03
#define SCSI_PRVNT_ALLW_MDM_RMVL    0x1E 
/* SCSI_READ_FORMAT_CAPACITY: Nut used by this code. Specific to MMC devices */
#define SCSI_READ_FORMAT_CAPACITY   0x23
/* Start and stop unit bits */
#define SCSI_SSU_LOAD               (0x03)
#define SCSI_SSU_EJECT              (0x02)
#define SCSI_SSU_START              (0x01)
#define SCSI_SSU_ACTIVE             ((0x01) << 4)
#define SCSI_SSU_STANDBY            ((0x03) << 4)

/******************************************************************************
 Constant Macros
 ******************************************************************************/

/* Key = Sense Key, ASC = Additional Sense Code, ASCQ = Additional Sense Code
 Qualifier */
#define SCSI_SENSE(Key, ASC, ASCQ) (((uint32_t) (Key)<<16) | ((uint32_t) (ASC)<<8) | ((uint32_t) ASCQ))
/* These are some of the codes for reasons why a device is not ready */
/* ErrorCode = SCSI_MEDIA_FORMAT_CORRUPT */
/* 31h 00h DT WRO BK MEDIUM FORMAT CORRUPTED */
#define SCSI_SENSE_FORMAT_CORRUPT   SCSI_SENSE(0x02, 0x31, 0x00)
/* ErrorCode = SCSI_MEDIA_NOT_PRESENT */
/* 3Bh 11h DT WROM BK MEDIUM MAGAZINE NOT ACCESSIBLE */
#define SCSI_SENSE_MNP_MNA          SCSI_SENSE(0x02, 0x3B, 0x11)
/* 3Bh 12h DT WROM BK MEDIUM MAGAZINE REMOVED */
#define SCSI_SENSE_MNP_MR           SCSI_SENSE(0x02, 0x3B, 0x12)
/* 3Bh 15h DT WROM BK MEDIUM MAGAZINE UNLOCKED */
#define SCSI_SENSE_MNP_MU           SCSI_SENSE(0x02, 0x3B, 0x15)
/* 3Ah 00h DT L WROM BK MEDIUM NOT PRESENT */
#define SCSI_SENSE_MNP              SCSI_SENSE(0x02, 0x3A, 0x00)
/* 3Ah 03h DT WROM B MEDIUM NOT PRESENT - LOADABLE */
#define SCSI_SENSE_MNP_LOADABLE     SCSI_SENSE(0x02, 0x3A, 0x03)
/* 3Ah 04h DT WROM B MEDIUM NOT PRESENT - MEDIUM AUXILIARY MEMORY ACCESSIBLE */
#define SCSI_SENSE_MNP_MAMA         SCSI_SENSE(0x02, 0x3A, 0x04)
/* 3Ah 01h DT WROM BK MEDIUM NOT PRESENT - TRAY CLOSED */
#define SCSI_SENSE_MNP_TRAY_CLOSED  SCSI_SENSE(0x02, 0x3A, 0x01)
/* ErrorCode = SCSI_MEDIA_CHANGE */
/* 04h 01h DT LPWROMAEBKVF LOGICAL UNIT IS IN PROCESS OF BECOMING READY */
#define SCSI_SENSE_MOUNTING_MEDIA   SCSI_SENSE(0x02, 0x04, 0x01)
/* 28h 00h DT LPWROMAEBKVF NOT READY TO READY CHANGE, MEDIUM MAY HAVE CHANGED*/
#define SCSI_SENSE_MEDIA_CHANGE     SCSI_SENSE(0x06, 0x28, 0x00)
/* 29h 00h DT LPWROMAEBKVF POWER ON, RESET, OR BUS DEVICE RESET OCCURRED */
#define SCSI_SENSE_MNP_BUS_RESET    SCSI_SENSE(0x06, 0x29, 0x00)

/******************************************************************************
 Function Macros
 ******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

#define SCSI_SENSE_RESPONSE_CODE(p) (*(((uint8_t *)p) + 0))
#define SCSI_SENSE_KEY(p)           (*(((uint8_t *)p) + 2))
#define SCSI_SENSE_ASC(p)           (*(((uint8_t *)p) + 12))
#define SCSI_SENSE_ASCQ(p)          (*(((uint8_t *)p) + 13))

/******************************************************************************
 Typedef definitions
 ******************************************************************************/

#pragma pack (1)
typedef struct _SCSIINQ
{
    union
    {
        uint8_t                                                                                                                                                                                                     bByte;
        struct
        {
#ifdef _BITFIELDS_MSB_FIRST_
            uint8_t Qualifier:3;
            uint8_t Device:5;
#else
            uint8_t Device :5;
            uint8_t Qualifier :3;
#endif
        } Field;
    } Peripheral;

    union
    {
        uint8_t                                                                                                                                                                                    bByte;
        struct
        {
#ifdef _BITFIELDS_MSB_FIRST_
            uint8_t RMB:1;
            uint8_t Reserved:7;
#else
            uint8_t Reserved :7;
            uint8_t RMB :1;
#endif
        } Field;
    } Removable;

    uint8_t                                                                                                                                                                                                                                                                 bVersion;
    uint8_t                                                                                                                                                                                                                                                                 bUnusedAERC;
    uint8_t                                                                                                                                                                                                                                                                 bAdditionalLength;
    uint8_t                                                                                                                                                                                                                                                                 bObsolete1;
    uint8_t                                                                                                                                                                                                                                                                 bObsolete2;
    uint8_t                                                                                                                                                                                                                                                                 bObsolete3;
    uint8_t                                                                                                                                                                                                                                                                 bVendorID[8];
    uint8_t                                                                                                                                                                                                                                                                 bProductID[16];
    uint8_t                                                                                                                                                                                                                                                                 bRevisionID[4];

} SCSIINQ, *PSCSIINQ;

#pragma pack()

/******************************************************************************
 Exported global variables and functions (to be accessed by other files)
 ******************************************************************************/

/******************************************************************************
 Constant Data
 ******************************************************************************/

const MSCMD USB_MS_SCSI_READ_10 =
{
    (int8_t *) "READ 10",
    {
        SCSI_READ_10, 0x00, /* Reserved */
        0x00, 0x00, 0x00, 0x01, /* Logical Block Address MSB-LSB */
        0x00, /* reserved */
        0x00, 0x01, /* Transfer length MSB-LSB in blocks*/
        0x00 /* Control */
    },
	10, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    512, /* The length of data (in bytes) - to be set*/
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_SCSI_READ_CAPACITY_10 =
{
    (int8_t *) "READ CAPACITY 10",
    {
        SCSI_READ_CAPACITY_10, 0x00, /* Reserved */
        0x00, /* Reserved */
        0x00, /* Reserved */
        0x00, /* Reserved */
        0x00, /* Reserved */
        0x00, /* reserved */
        0x00, /* reserved */
        0x00, /* reserved */
        0x00 /* Control */
    },
	10, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    8, /* The length of data */
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_START_UNIT =
{
    (int8_t *) "START",
    {
        SCSI_START_STOP, 0x00, /* Immediate */
        0x00, /* Reserved */
        SCSI_SSU_START, /* SSU */
        0x00 /* Control */
    },
	6, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    0, /* The length of data */
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_STOP_UNIT =
{
    (int8_t *) "STOP",
    {
        SCSI_START_STOP, 0x00, /* Immediate */
        0x00, /* Reserved */
        SCSI_SSU_EJECT, /* SSU */
        0x00 /* Control */
    },
	6, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    0, /* The length of data */
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_WRITE_10 =
{
    (int8_t *) "WRITE 10",
    {
        SCSI_WRITE_10, 0x00, /* Force Unit Access BIT_3 */
        0x00, 0x00, 0x00, 0x01, /* Logical Block Address MSB-LSB */
        0x00, /* reserved */
        0x00, 0x01, /* Transfer length MSB-LSB in blocks*/
        0x00 /* Control */
    },
	10, /* Length of the command block */
    USB_MS_DIRECTION_OUT, /* Transfer direction is OUT */
    512, /* The length of data (in bytes) - to be set */
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_SCSI_UNIT_READY =
{
    (int8_t *) "TEST UNIT READY",
    {
        SCSI_TEST_UNIT_READY, 0x00, /* Reserved */
        0x00, /* Reserved */
        0x00, /* Reserved */
        0x00, /* Reserved */
        0x00 /* Control */
    },
	6, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    0, /* The length of data - just get sense */
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_SCSI_INQUIRY =
{
    (int8_t *) "INQUIRY",
    {
        SCSI_INQUIRY, 0x00, /* EVPD */
        0x00, /* Page Code */
        0x00, 0x36, /* Allocation length */
        0x00 /* Control */
    },
	6, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    36, /* The length of data */
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_SCSI_MODE_SENSE_6 =
{
    (int8_t *) "MODE SENSE",
    {
        SCSI_MODE_SENSE_6, 0x00, /* DBD */
        0x3F, /* Page Code */
        0x00, /* Sub page code */
        4, /* Allocation length */
        0x00 /* Control */
    },
	6, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    4, /* The length of data */
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_SCSI_REQUEST_SENSE =
{
    (int8_t *) "REQUEST SENSE",
    {
        SCSI_REQUEST_SENSE, 0x00, /* DESC = 0 */
        0x00, /* Reserved */
        0x00, /* Reserved */
        18, /* Allocation Length */
        0x00, /* Control */
    },
	6, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    18, /* The length of data */
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_SCSI_PRVNT_REMOVAL =
{
    (int8_t *) "PREVENT MEDIUM REMOVAL",
    {
        SCSI_PRVNT_ALLW_MDM_RMVL, 0x00, /* DESC = 0 */
        0x00, /* Reserved */
        0x00, /* Reserved */
        0x01, /* Control */
    },
	5, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    0, /* The length of data */
    8000UL /* Time out (ms) */
};

const MSCMD USB_MS_SCSI_ALLW_REMOVAL =
{
    (int8_t *) "ALLOW MEDIUM REMOVAL",
    {
        SCSI_PRVNT_ALLW_MDM_RMVL, 0x00, /* DESC = 0 */
        0x00, /* Reserved */
        0x00, /* Reserved */
        0x00, /* Control */
    },
	5, /* Length of the command block */
    USB_MS_DIRECTION_IN, /* Transfer direction is IN */
    0, /* The length of data */
    8000UL /* Time out (ms) */
};

/******************************************************************************
 Public Functions
 ******************************************************************************/

/******************************************************************************
 Function Name: scsiRead10
 Description:   Function to read from the media
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 IN  dwLBA - The logical block address
 IN  wNumBlocks - The number of blocks
 OUT pbyDest - Pointer to the destination memory
 IN  stDestLength - The length of the destination memory
 OUT pstLengthRead - Pointer to the length of data read in bytes
 Return value:  0 for success otherwise error code
 ******************************************************************************/
int scsiRead10 (int iMsDev, int iLun, uint32_t dwLBA, uint16_t wNumBlocks, uint8_t *pbyDest, size_t stDestLength,
        size_t *pstLengthRead)
{
    MSCMD msCmd = USB_MS_SCSI_READ_10;
    int iTry = 3;

    /* Add the Lun into the command */
    msCmd.pbyCB[1] |= (uint8_t) (iLun << 4);

    /* Set the LBA */
#ifdef _BIG
    copyIndirectLong((uint32_t *)&msCmd.pbyCB[2], &dwLBA);
#else
    msCmd.pbyCB[2] = (uint8_t) (dwLBA >> 24);
    msCmd.pbyCB[3] = (uint8_t) (dwLBA >> 16);
    msCmd.pbyCB[4] = (uint8_t) (dwLBA >> 8);
    msCmd.pbyCB[5] = (uint8_t) (dwLBA >> 0);
#endif
    /* Set the number of blocks */
#ifdef _BIG
    copyIndirectShort((uint16_t *)&msCmd.pbyCB[7], &wNumBlocks);
#else
    msCmd.pbyCB[7] = (uint8_t) (wNumBlocks >> 8);
    msCmd.pbyCB[8] = (uint8_t) wNumBlocks;

#endif

    /* Set the length of the destination memory */
    msCmd.stTransferLength = stDestLength;

    /* Issue the read command */
    do
    {
        if (!usbMsCommand(iMsDev, iLun, &msCmd, pbyDest, pstLengthRead))
        {
            return SCSI_OK;
        } TRACE(("scsiRead10: Error in command %d\r\n", iTry));
    } while (iTry--);
    return SCSI_COMMAND_ERROR;
}
/******************************************************************************
 End of function  scsiRead10
 ******************************************************************************/

/******************************************************************************
 Function Name: scsiReadCapacity10
 Description:   Function to read the capacity of the medium
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 OUT pdwNumBlocks - Pointer to the number of blocks
 OUT pdwBlockSize - Pointer to the length of the block
 Return value:  0 for success otherwise error code
 ******************************************************************************/
int scsiReadCapacity10 (int iMsDev, int iLun, uint32_t * pdwNumBlocks, uint32_t * pdwBlockSize)
{
    uint32_t pdwPacket[2];
    UMSERR umsErr;
    MSCMD msCmd = USB_MS_SCSI_READ_CAPACITY_10;

    /* Add the Lun into the command */
    msCmd.pbyCB[1] |= (uint8_t) (iLun << 4);

    /* Issue the capacity command */
    umsErr = usbMsCommand(iMsDev, iLun, &msCmd, pdwPacket, NULL);
    if (umsErr)
    {
        TRACE(("scsiReadCapacity10: Error in command\r\n"));
        return SCSI_COMMAND_ERROR;
    }
    if (pdwNumBlocks)
    {
        /* Add one to convert from last block address to number of blocks */
#ifdef _BIG
        *pdwNumBlocks = pdwPacket[0] + 1;
#else
        swapIndirectLong(pdwNumBlocks, pdwPacket);
        (*pdwNumBlocks)++;
#endif
    }
    if (pdwBlockSize)
    {
#ifdef _BIG
        *pdwBlockSize = pdwPacket[1];
#else
        swapIndirectLong(pdwBlockSize, &pdwPacket[1]);
#endif
    }
    return SCSI_OK;
}
/******************************************************************************
 End of function  scsiReadCapacity10
 ******************************************************************************/

/******************************************************************************
 Function Name: scsiStartStopUnit
 Description:   Function to start and stop the unit
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 IN  bfStart - true to start false to stop
 Return value:  0 for success otherwise error code
 ******************************************************************************/
int scsiStartStopUnit (int iMsDev, int iLun, _Bool bfStart)
{
    UMSERR umsErr;
    if (bfStart)
    {
        umsErr = usbMsCommand(iMsDev, iLun, (PMSCMD) &USB_MS_START_UNIT,
        NULL, NULL);
    }
    else
    {
        umsErr = usbMsCommand(iMsDev, iLun, (PMSCMD) &USB_MS_STOP_UNIT,
        NULL, NULL);
    }
    if (umsErr)
    {
        TRACE(("scsiStartStopUnit: Command failed\r\n"));
        return SCSI_COMMAND_ERROR;
    }

    return SCSI_OK;
}
/******************************************************************************
 End of function  scsiStartStopUnit
 ******************************************************************************/

/******************************************************************************
 Function Name: scsiWrite10
 Description:   Function to write to the media
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 IN  dwLBA - The logical block address
 IN  wNumBlocks - The number of blocks
 OUT pbySrc - Pointer to the source memory
 IN  stSrcLength - The length of the source memory
 OUT pstLengthWritten - Pointer to the length of data written in
 bytes
 Return value:  0 for success otherwise error code
 ******************************************************************************/
int scsiWrite10 (int iMsDev, int iLun, uint32_t dwLBA, uint16_t wNumBlocks, const uint8_t *  pbySrc, size_t stSrcLength,
        size_t *pstLengthWritten)
{
    MSCMD msCmd = USB_MS_WRITE_10;
    /* Add the Lun into the command */
    msCmd.pbyCB[1] |= (uint8_t) (iLun << 4);
    /* Set the LBA */
#ifdef _BIG
    copyIndirectLong((uint32_t *)&msCmd.pbyCB[2], &dwLBA);
#else
    msCmd.pbyCB[2] = (uint8_t) (dwLBA >> 24);
    msCmd.pbyCB[3] = (uint8_t) (dwLBA >> 16);
    msCmd.pbyCB[4] = (uint8_t) (dwLBA >> 8);
    msCmd.pbyCB[5] = (uint8_t) (dwLBA >> 0);
#endif
    /* Set the number of blocks */
#ifdef _BIG
    copyIndirectShort((uint16_t *)&msCmd.pbyCB[7], &wNumBlocks);
#else
    msCmd.pbyCB[7] = (uint8_t) (wNumBlocks >> 8);
    msCmd.pbyCB[8] = (uint8_t) wNumBlocks;
#endif
    /* Set the length of the source memory */
    msCmd.stTransferLength = stSrcLength;
    /* Issue the write command */
    if (usbMsCommand(iMsDev, iLun, &msCmd, (void *)pbySrc, pstLengthWritten))
    {
        TRACE(("scsiWrite10: Command error\r\n"));
        return SCSI_COMMAND_ERROR;
    }

    TRACE(("scsiWrite10: LBA 0x%.8lX, Blocks %d \r\n", dwLBA, wNumBlocks));

    return SCSI_OK;
}
/******************************************************************************
 End of function  scsiWrite10
 ******************************************************************************/

/******************************************************************************
 Function Name: scsiInquire
 Description:   Function to get the Mass Storage device information
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 OUT pMsInf - Pointer to the destination Mass Storage Information
 Return value:  0 for success otherwise error code
 ******************************************************************************/
int scsiInquire (int iMsDev, int iLun, PMSINF pMsInf)
{
    SCSIINQ scsiInquiry;
    UMSERR umsErr;
    /* Issue the inquiry command */
    umsErr = usbMsCommand(iMsDev, iLun, (PMSCMD) &USB_MS_SCSI_INQUIRY, &scsiInquiry,
    NULL);
    if (umsErr)
    {
        TRACE(("scsiInquire: Command error\r\n"));
        return SCSI_COMMAND_ERROR;
    }
    /* Check to see if the device is supported */
    switch (scsiInquiry.Peripheral.Field.Device)
    {
        case 0x00 :
        case 0x0E :
        break;
        default :
            TRACE(("scsiInquire: Protocol not supported\r\n"));
            return SCSI_PROTOCOL_NOT_SUPPORTED;
    }
    /* Fill out the information structure */
    memcpy(pMsInf, &scsiInquiry.bVendorID, 28L);
    pMsInf->bfRemovable = (_Bool) scsiInquiry.Removable.Field.RMB;

    return SCSI_OK;
}
/******************************************************************************
 End of function  scsiInquire
 ******************************************************************************/

/******************************************************************************
 Function Name: scsiTestUnitReady
 Description:   Function to test if a unit is ready for operation
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 OUT pbfReady - Pointer to a ready flag
 Return value:  0 for success otherwise error code
 ******************************************************************************/
int scsiTestUnitReady (int iMsDev, int iLun, _Bool *pbfReady)
{
    UMSERR umsErr;

    umsErr = usbMsCommand(iMsDev, iLun, (PMSCMD) &USB_MS_SCSI_UNIT_READY,
    NULL,
    NULL);
    if (umsErr > USB_MS_ERROR)
    {
        TRACE(("scsiTestUnitReady: Command error\r\n"));
        return SCSI_COMMAND_ERROR;
    }
    else if (umsErr)
    {
        if (pbfReady)
        {
            *pbfReady = false;
        }
    }
    else
    {
        if (pbfReady)
        {
            *pbfReady = true;
        }
    }
    return SCSI_OK;
}
/******************************************************************************
 End of function  scsiTestUnitReady
 ******************************************************************************/

/******************************************************************************
 Function Name: scsiModeSense
 Description:   Function to request the mode sense
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 OUT pbfWriteProtect - Pointer to flag set true if the medium is
 write protected
 Return value:  0 for success otherwise error code
 ******************************************************************************/
int scsiModeSense (int iMsDev, int iLun, _Bool *pbfWriteProtect)
{
    uint32_t dwPacket;
    UMSERR umsErr;
    MSCMD msCmd = USB_MS_SCSI_MODE_SENSE_6;
    size_t stLength;
    /* Add the Lun into the command */
    msCmd.pbyCB[1] |= (uint8_t) (iLun << 4);
    umsErr = usbMsCommand(iMsDev, iLun, &msCmd, &dwPacket, &stLength);
    if (umsErr)
    {
        TRACE(("scsiModeSense: Command error\r\n"));
        return SCSI_COMMAND_ERROR;
    }
    if ((stLength == 0UL) && (pbfWriteProtect))
    {
        *pbfWriteProtect = false;
        return SCSI_OK;
    }
    if (pbfWriteProtect)
    {
#ifdef _BIG
        if (dwPacket & 0x00008000UL)
#else
        if (dwPacket & swapLong(0x00008000UL))
#endif
        {
            *pbfWriteProtect = true;
        }
        else
          {
            *pbfWriteProtect = false;
          }
    }
    return SCSI_OK;
}
/******************************************************************************
 End of function  scsiModeSense
 ******************************************************************************/

/******************************************************************************
 Function Name: scsiRequestSense
 Description:   Function to request the sense of an error
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 Return value:  0 for success otherwise error code
 ******************************************************************************/
int scsiRequestSense (int iMsDev, int iLun)
{
    uint32_t pdwPacket[5];
    UMSERR umsErr;
    MSCMD msCmd = USB_MS_SCSI_REQUEST_SENSE;
    /* Add the Lun into the command */
    msCmd.pbyCB[1] |= (uint8_t) (iLun << 4);
    /* Request the fixed sense */
    umsErr = usbMsCommand(iMsDev, iLun, &msCmd, pdwPacket, NULL);
    /* If we got the sense data */
    if ((!umsErr)
            /* And we got fixed format sense data */
            && ((SCSI_SENSE_RESPONSE_CODE(pdwPacket) == 0xF0) || (SCSI_SENSE_RESPONSE_CODE(pdwPacket) == 0xF1)
                    || (SCSI_SENSE_RESPONSE_CODE(pdwPacket) == 0x70) || (SCSI_SENSE_RESPONSE_CODE(pdwPacket) == 0x71)))
    {
        /* Check the sense codes for the reason for the media not being ready */
        uint32_t dwSenseCode = SCSI_SENSE(SCSI_SENSE_KEY(pdwPacket), SCSI_SENSE_ASC(pdwPacket),
                SCSI_SENSE_ASCQ(pdwPacket));
        switch (dwSenseCode)
        {
            /* No error */
            case SCSI_SENSE(0, 0, 0) :
                return SCSI_OK;
                /* Media format is corrupt - nice */
            case SCSI_SENSE_FORMAT_CORRUPT :
                TRACE(("scsiRequestSense: %d %d SCSI_MEDIA_FORMAT_CORRUPT\r\n",
                                iMsDev, iLun));
                return SCSI_MEDIA_FORMAT_CORRUPT;
                /* The Media is not present */
            default :
            case SCSI_SENSE_MNP_MNA :
            case SCSI_SENSE_MNP_MR :
            case SCSI_SENSE_MNP_MU :
            case SCSI_SENSE_MNP :
            case SCSI_SENSE_MNP_LOADABLE :
            case SCSI_SENSE_MNP_MAMA :
            case SCSI_SENSE_MNP_TRAY_CLOSED :
                TRACE(("scsiRequestSense: %d %d SCSI_MEDIA_NOT_PRESENT"
                                " (0x%.8lX)\r\n", iMsDev, iLun, dwSenseCode));
                return SCSI_MEDIA_NOT_PRESENT;
                /* The media has been inserted and is in the process of becoming
                 ready */
            case SCSI_SENSE_MOUNTING_MEDIA :
            case SCSI_SENSE_MEDIA_CHANGE :
                TRACE(("scsiRequestSense: %d %d SCSI_MEDIA_CHANGE\r\n", iMsDev,
                                iLun));
                return SCSI_MEDIA_CHANGE;
                /* This is returned by a SanDisk cruzer which reports a MAX LUN of
                 1 when it only has one device when the second device is
                 accessed */
            case SCSI_SENSE_MNP_BUS_RESET :
                return SCSI_MEDIA_NOT_AVAILABLE;
        }
    }
    else
    {
        TRACE(("scsiRequestSense: Command error\r\n"));
        return SCSI_COMMAND_ERROR;
    }
}
/******************************************************************************
 End of function  scsiRequestSense
 ******************************************************************************/

/******************************************************************************
 Function Name: scsiPrvntAllwRemoval
 Description:   Function to prevent or allow medium removal
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 IN  bfAllow - true to allow false to prevent medium removal
 Return value:  0 for success otherwise error code
 ******************************************************************************/
int scsiPrvntAllwRemoval (int iMsDev, int iLun, _Bool bfAllow)
{
    UMSERR umsErr;
    if (bfAllow)
    {
        umsErr = usbMsCommand(iMsDev, iLun, (PMSCMD) &USB_MS_SCSI_ALLW_REMOVAL,
        NULL, NULL);
    }
    else
    {
        umsErr = usbMsCommand(iMsDev, iLun, (PMSCMD) &USB_MS_SCSI_PRVNT_REMOVAL,
        NULL, NULL);
    }
    if (umsErr)
    {
        TRACE(("scsiPrvntAllwRemoval: Command failed\r\n"));
        return SCSI_COMMAND_ERROR;
    }

    return SCSI_OK;
}
/******************************************************************************
 End of function  scsiPrvntAllwRemoval
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
