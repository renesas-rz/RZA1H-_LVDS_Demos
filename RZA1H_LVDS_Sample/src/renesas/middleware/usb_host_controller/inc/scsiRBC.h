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
* File Name    : scsiRBC.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : None
* H/W Platform : RSK+
* Description  : SCSI Reduced Block Commands. See document 97-260r2.pdf
*******************************************************************************
* History      : DD.MM.YYYY Version Description
*              : 05.08.2010 1.00    First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

#ifndef SCSIRBC_H_INCLUDED
#define SCSIRBC_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "r_typedefs.h"
#include "usbMsBulkOnly.h"

/******************************************************************************
Typedef definitions
******************************************************************************/

typedef enum _SCSIERR
{
    SCSI_OK = 0,
    SCSI_MEDIA_CHANGE,
    SCSI_MEDIA_NOT_PRESENT,
    SCSI_MEDIA_NOT_AVAILABLE,
    SCSI_MEDIA_FORMAT_CORRUPT,
    SCSI_COMMAND_ERROR,
    SCSI_PROTOCOL_NOT_SUPPORTED,
    SCSI_DRIVER_ERROR
    
} SCSIERR;

typedef struct _MSINF
{
    uint8_t    byVendorID[8];          /* The vendor ID */
    uint8_t    byProductID[16];        /* The product ID */
    uint8_t    byRevisionID[4];        /* The revision of the product */
    _Bool      bfRemovable;            /* Set true if the medium is removable */

} MSINF,
*PMSINF;

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

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

extern  int scsiRead10(int       iMsDev,
                       int       iLun,
                       uint32_t  dwLBA,
                       uint16_t  wNumBlocks,
                       uint8_t * pbyDest,
                       size_t    stDestLength,
                       size_t    *pstLengthRead);

/******************************************************************************
Function Name: scsiReadCapacity
Description:   Function to read the capacity of the medium
Arguments:     IN  iMsDev - The mass storage device file descriptor
               IN  iLun - The logical uint number
               OUT pdwNumBlocks - Pointer to the number of blocks
               OUT pdwBlockSize - Pointer to the length of the block
Return value:  0 for success otherwise error code  
******************************************************************************/

extern  int scsiReadCapacity10(int      iMsDev,
                               int      iLun,
                               uint32_t *   pdwNumBlocks,
                               uint32_t *   pdwBlockSize);

/******************************************************************************
Function Name: scsiStartStopUnit
Description:   Function to start and stop the unit
Arguments:     IN  iMsDev - The mass storage device file descriptor
               IN  iLun - The logical uint number
               IN  bfStart - true to start false to stop
Return value:  0 for success otherwise error code
******************************************************************************/

extern  int scsiStartStopUnit(int iMsDev, int iLun, _Bool bfStart);

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

extern  int scsiWrite10(int       iMsDev,
                        int       iLun,
                        uint32_t  dwLBA,
                        uint16_t  wNumBlocks,
                        const uint8_t * pbySrc,
                        size_t    stSrcLength,
                        size_t *  pstLengthWritten);
                
/******************************************************************************
Function Name: scsiInquire
Description:   Function to get the Mass Storage device information
Arguments:     IN  iMsDev - The mass storage device file descriptor
               IN  iLun - The logical uint number
               OUT pMsInf - Pointer to the destination Mass Storage Information
Return value:  0 for success otherwise error code
******************************************************************************/

extern  int scsiInquire(int iMsDev, int iLun, PMSINF pMsInf);

/******************************************************************************
Function Name: scsiTestUnitReady
Description:   Function to test if a unit is ready for operation
Arguments:     IN  iMsDev - The mass storage device file descriptor
               IN  iLun - The logical uint number
               OUT pbfReady - Pointer to a ready flag
Return value:  0 for success otherwise error code
******************************************************************************/

extern int scsiTestUnitReady(int iMsDev, int iLun, _Bool *pbfReady);

/******************************************************************************
Function Name: scsiModeSense
Description:   Function to request the mode sense
Arguments:     IN  iMsDev - The mass storage device file descriptor
               IN  iLun - The logical uint number
               OUT pbfWriteProtect - Pointer to flag set true if the medium is
                                     write protected
Return value:  0 for success otherwise error code
******************************************************************************/

extern  int scsiModeSense(int iMsDev, int iLun, _Bool *pbfWriteProtect);

/******************************************************************************
Function Name: scsiRequestSense
Description:   Function to request the sense of an error
Arguments:     IN  iMsDev - The mass storage device file descriptor
               IN  iLun - The logical uint number
Return value:  0 for success otherwise error code
******************************************************************************/

extern  int scsiRequestSense(int iMsDev, int iLun);

/******************************************************************************
Function Name: scsiPrvntAllwRemoval
Description:   Function to prevent or allow medium removal
Arguments:     IN  iMsDev - The mass storage device file descriptor
               IN  iLun - The logical uint number
               IN  bfAllow - true to allow false to prevent medium removal
Return value:  0 for success otherwise error code
******************************************************************************/

extern  int scsiPrvntAllwRemoval(int iMsDev, int iLun, _Bool bfAllow);

#ifdef __cplusplus
}
#endif

#endif /* SCSIRBC_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
