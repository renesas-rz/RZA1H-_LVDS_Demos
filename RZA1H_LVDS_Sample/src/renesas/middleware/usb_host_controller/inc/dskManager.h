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
 * @headerfile     dskManager.h
 * @brief          Emulation disk management functions.
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 05.08.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef DSKMANAGER_H_INCLUDED
#define DSKMANAGER_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_USB_DSK_MNG USB Disk Management
 * @brief Emulation disk management functions.
 * 
 * @anchor R_SW_PKG_93_USB_DSK_MNG_SUMMARY
 * @par Summary
 * 
 * This module represents the USB Disk Management functionality of the USB
 * Host Driver.
 * 
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "r_fatfs_abstraction.h"

/******************************************************************************
Typedef definitions
******************************************************************************/

typedef enum _DSKERR
{
    DISK_OK = 0,
    DISK_MEDIA_CHANGE,
    DISK_MEDIA_NOT_PRESENT,
    DISK_MEDIA_NOT_AVAILABLE,
    DISK_FAILED_TO_MOUNT_PARTITION,
    DISK_INVALID_FORMAT,
    DISK_DEVICE_REMOVED,
    DISK_DRIVER_ERROR,
    DISK_NO_MEMORY
    
} DSKERR;

typedef enum _FATT
{
    FAT12 = 0,
    FAT16,
    FAT32,
    UNKNOWN_FORMAT
    
} FATT;

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Function to start the disk management task
 * 
 * @retval        None. 
*/
extern  void dskStartDiskManager(void);

/**
 * @brief         Function to signal the disk manager to look at new PnP drives
 * 
 * @retval        None. 
*/
extern  void dskNewPnPDrive(void);

/**
 * @brief         Function to stop the disk manager task
 * 
 * @retval        None. 
*/
extern  void dskStopDiskManager(void);

/**
 * @brief         Function to mount all available devices
 * 
 * @retval        num_drvs: The number of available drives
*/
extern  int dskMountAllDevices(void);

/**
 * @brief         Function to dismount and close all devices
 * 
 * @retval  None. 
*/
extern  void dskDismountAllDevices(void);

/**
 * @brief         Function to dismout a disk for removal from the system
 * 
 * @param[in]     chDriveLetter: The drive letter to eject
 * 
 * @retval        true: if the disk was ejected
*/
extern  _Bool dskEjectMedia(int8_t chDriveLetter);

/**
 * @brief         Function to get a pointer to the drive object
 * 
 * @param[in]     chDriveLetter: The drive letter of the device
 * 
 * @retval        p_mnger: Pointer to the IO Manager 
 * @retval        NULL:    On failure
*/
extern PDRIVE dskGetDrive(int8_t chDriveLetter);

/**
 * @brief         Function to get a pointer to the drive object & the drive
 * 
 * @param[out]    pchDriveLetter: The drive letter of the device
 * 
 * @retval        p_drv_obj: Pointer to the drive object  
 * @retval        NULL: On failure
*/
extern PDRIVE dskGetFirstDrive(int8_t *pchDriveLetter);

/**
 * @brief         Function to get the state of the media in the drive
 * 
 * @param[in]     chDriveLetter:     The drive letter of the disk
 * @param[out]    pbfMediaPresent:   Pointer to a flag set true if the 
 * @param[out]    pbfWriteProtected: Pointer to a flag set true if the disk
 *                                       
 * @retval        true: If the disk was found
*/
extern  _Bool dskGetMediaState(int8_t  chDriveLetter,
                               _Bool *pbfMediaPresent,
                               _Bool *pbfWriteProtected);

/**
 * @brief         Function find the drive in the drive list
 * 
 * @param[in]     iMsDev: drive number to search for
 * 
 * @retval        p_drv:  PDRIVE the drive if found
 * @retval        NULL:   If Drive not found
*/
PDRIVE get_drive (int drive);


#ifdef __cplusplus
}
#endif

#endif /* DSKMANAGER_H_INCLUDED */


/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
