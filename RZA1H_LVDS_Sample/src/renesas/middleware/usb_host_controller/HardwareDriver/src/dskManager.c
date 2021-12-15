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
 * Copyright (C) 2016 Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************
 * File Name    : dskManager.c
 * Version      : 1.03
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : The disk management functions.
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 First Release
 *              : 10.11.2010 1.01 Corrected ATTACHED event define
 *              : 14.12.2010 1.02 Handled unknown partitions
 *              : 12.01.2016 1.03 Handle GetMaxLUN Stall conditions
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "compiler_settings.h"
#include "r_os_abstraction_api.h"
#include "r_fatfs_abstraction.h"

#include "r_task_priority.h"
#include "control.h"
#include "scsiRBC.h"
#include "dskManager.h"
#include "Trace.h"
#include "r_os_abstraction_api.h"

/******************************************************************************
 Macro definitions
 ******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Typedef definitions
 ******************************************************************************/

typedef enum _DRVSTA
{
    DRIVE_NO_MEDIA = 0, DRIVE_READY, DRIVE_MEDIA_ERROR, DRIVE_DEVICE_ERROR

} DRVSTA;

typedef struct _DSKLST *PDSKLST;

typedef struct _DSKLST
{
    PDSKLST    pNext;           /* Pointer to the next disk on the list */
    DRVSTA     driveState;      /* The current state of the drive */
    char       chDriveLetter;   /* The drive letter of this disk */
    int        iMsDev;          /* The file descriptor for this device */
    int        iLun;            /* The Logical Unit Number for this device */
    int        iMaxLun;         /* The max LUN for this device */
    PDRIVE     pDrive;          /* Pointer to the FAT drive object that
                                   manages the disk */
    int        iErrorCode;      /* The last IO error code */

    /* The physical geometry of the media */
    struct
    {
        uint32_t dwNumBlocks;
        uint32_t dwBlockSize;
    } mediaGeometry;

    /* The attributes of the media */
    _Bool                                                                                    bfWriteProtect;

} DSKLST;

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/
DSKERR dskMountPartition (PDSKLST pDisk, int iPartition);

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

static void dskManager (void);
static PDRIVE dskGetDriveFromLetter (int8_t chDriveLetter);
static void dskFree (PDSKLST pDisk);
static DSKERR dskMountDevice (int iMsDev);
static void dskDismountDevice (int iMsDev);
static DSKERR dskMountUnit (int iMsDev, int iLun, PDSKLST pDisk, _Bool bfAdd);
static _Bool dskRemoveDisk (PDSKLST pDisk);
static int dskCountAttachedDrives (int iMsDev);

/******************************************************************************
 Global Variables
 ******************************************************************************/

static PDSKLST gpDiskList = NULL;
static int initaliser =  R_OS_ABSTRACTION_PRV_INVALID_HANDLE;
static os_task_t *guiTaskID = &initaliser;
static event_t gpevNewDrive = NULL;

/******************************************************************************
 Public Functions
 ******************************************************************************/

/*****************************************************************************
 Function Name: dskStartDiskManager
 Description:   Function to start the disk management task
 Arguments:     none
 Return value:  none
 *****************************************************************************/
void dskStartDiskManager (void)
{
    if (*(int*)(guiTaskID) == R_OS_ABSTRACTION_PRV_INVALID_HANDLE)
    {
        /* Create the event to signal the task */
        R_OS_CreateEvent(&gpevNewDrive);

        /* Create the task */
        (guiTaskID) = R_OS_CreateTask("Disk Manager",
                                      (os_task_code_t)dskManager,
                                      NULL,
                                      R_OS_ABSTRACTION_PRV_DEFAULT_STACK_SIZE,
                                      TASK_DISK_MANAGER_PRI);
    }
}
/*****************************************************************************
 End of function  dskStartDiskManager
 ******************************************************************************/

/*****************************************************************************
 Function Name: dskNewPnPDrive
 Description:   Function to signal the disk manager to look at new PnP drives
 Arguments:     none
 Return value:  none
 *****************************************************************************/
void dskNewPnPDrive (void)
{
    /* If the manager task has not been started then start it */
    if (*(int*)guiTaskID == R_OS_ABSTRACTION_PRV_INVALID_HANDLE)
    {
        dskStartDiskManager();
    }

    /* Signal the task to look at the attached disks */
    R_OS_SetEvent(&gpevNewDrive);

}
/*****************************************************************************
 End of function  dskNewPnPDrive
 ******************************************************************************/

/*****************************************************************************
 Function Name: dskStopDiskManager
 Description:   Function to stop the disk manager task
 Arguments:     none
 Return value:  none
 *****************************************************************************/
void dskStopDiskManager (void)
{
    R_OS_SysWaitAccess();

    dskDismountAllDevices();

    R_OS_DeleteTask(&guiTaskID);

    *(int*)(guiTaskID) = R_OS_ABSTRACTION_PRV_INVALID_HANDLE;

    R_OS_DeleteEvent(&gpevNewDrive);

    R_OS_SysWaitAccess();
}
/*****************************************************************************
 End of function  dskStopDiskManager
 ******************************************************************************/

/******************************************************************************
 Function Name: dskMountAllDevices
 Description:   Function to mount all available devices
 Arguments:     none
 Return value:  The number of available drives
 ******************************************************************************/
int dskMountAllDevices (void)
{
    int iDiskCount = 0;
    volatile int_t dsk_state = 0;

    R_OS_SysWaitAccess();

    /* Check that a FAT library has been linked into the project */
    if (R_FAT_LoadLibrary() == 0)
    {
        PDSKLST pListTop = gpDiskList;
        int iMsDev;

        /* First look for any existing drives that could be mounted because
           media has been inserted, or any disks that should be removed */
        while (pListTop)
        {
            /* Check that the device is still attached */
            _Bool bfAttached = false;

            /* Do a check to make sure that the device is attached */
            control(pListTop->iMsDev, CTL_USB_ATTACHED, &bfAttached);
            if (bfAttached)
            {
                /* If the drive previously had no media */
                if ((pListTop->chDriveLetter == '?') &&
                    (pListTop->driveState == DRIVE_NO_MEDIA))
                {
                    /* Attempt to mount it */
                    dskMountUnit(pListTop->iMsDev,
                                 pListTop->iLun,
                                 pListTop,
                                 false);
                }
                /* Advance to the next disk on the list */
                pListTop = pListTop->pNext;
            }
            else
            {
                /* Remove the disk if it has been disconnected */
                dskDismountDevice(pListTop->iMsDev);
                /* Start at the top of the list again */
                pListTop = gpDiskList;
            }
        }

        /* Now look for any other MS class devices */
        do
        {
            /* Open the first available MS class device */
            iMsDev = open(DEVICE_INDENTIFIER "Mass Storage", O_RDWR);

            /* If a device was found */
            if (iMsDev > 0)
            {
                /* Try to mount it */
                dsk_state = dskMountDevice(iMsDev);

                if (dsk_state > DISK_MEDIA_NOT_PRESENT)
                {
                    dskDismountDevice(iMsDev);
                    close(iMsDev);
                    break;
                }
            }
        } while (iMsDev > 0);
        /* Count the number of disks */
        pListTop = gpDiskList;
        while (pListTop)
        {
            /* If the drive is not a ? then count it */
            if (pListTop->chDriveLetter != '?')
            {
                iDiskCount++;
            }
            pListTop = pListTop->pNext;
        }
    }
    else
    {
        printf("No FAT library is linked into the project\r\n");
    }
    R_OS_SysReleaseAccess();
    return iDiskCount;
}
/******************************************************************************
 End of function  dskMountAllDevices
 ******************************************************************************/

/******************************************************************************
 Function Name: dskDismountAllDevices
 Description:   Function to dismount and close all devices
 Arguments:     none
 Return value:  none
 ******************************************************************************/
void dskDismountAllDevices (void)
{
    PDSKLST *ppDiskList;
    R_OS_SysWaitAccess();
    ppDiskList = &gpDiskList;
    while (*ppDiskList)
    {
        int iMsDev = (*ppDiskList)->iMsDev;
        dskDismountDevice(iMsDev);
        close(iMsDev);
        ppDiskList = &gpDiskList;
    }
    R_OS_SysReleaseAccess();
}
/******************************************************************************
 End of function  dskDismountAllDevices
 ******************************************************************************/

/******************************************************************************
 Function Name: dskEjectMedia
 Description:   Function to dismout a disk for removal from the system
 Arguments:     IN  chDriveLetter - The drive letter to eject
 Return value:  true if the disk was ejected
 ******************************************************************************/
_Bool dskEjectMedia (int8_t chDriveLetter)
{
    PDSKLST *ppDiskList;
    _Bool bfResult = false;
    R_OS_SysWaitAccess();
    ppDiskList = &gpDiskList;
    /* Look for the drive letter in the list */
    while ((*ppDiskList) && ((*ppDiskList)->chDriveLetter != chDriveLetter))
    {
        ppDiskList = &(*ppDiskList)->pNext;
    }
    /* If it is found */
    if (*ppDiskList)
    {
        /* Get a pointer to the disk */
        PDSKLST pDiskToEject = *ppDiskList;
        /* If this is the only logical unit or partition in used */
        if (dskCountAttachedDrives(pDiskToEject->iMsDev) == 1)
        {
            /* Close the device */
            close(pDiskToEject->iMsDev);
            /* Remove the device all together */
            dskDismountDevice(pDiskToEject->iMsDev);
        }
        else
        {
            /* Destroy the IOMAN */
            dskFree(pDiskToEject);
            /* Set the media as ejected */
            pDiskToEject->driveState = DRIVE_NO_MEDIA;
            pDiskToEject->chDriveLetter = '?';
        }
        bfResult = true;
    }
    R_OS_SysReleaseAccess();
    return bfResult;
}
/******************************************************************************
 End of function  dskEjectMedia
 ******************************************************************************/

/******************************************************************************
 Function Name: dskGetDrive
 Description:   Function to get a pointer to the drive object
 Arguments:     IN  chDriveLetter - The drive letter of the device
 Return value:  Pointer to the drive object or NULL on failure
 ******************************************************************************/
PDRIVE dskGetDrive (int8_t chDriveLetter)
{
    PDRIVE pDrive = NULL;
    R_OS_SysWaitAccess();
    pDrive = dskGetDriveFromLetter(chDriveLetter);
    R_OS_SysReleaseAccess();
    return pDrive;
}
/******************************************************************************
 End of function  dskGetDrive
 ******************************************************************************/

/*****************************************************************************
 Function Name: dskGetFirstDrive
 Description:   Function to get a pointer to the drive object & the drive
 Arguments:     OUT pchDriveLetter - The drive letter of the device
 Return value:  Pointer to the drive object or NULL on failure
 ******************************************************************************/
PDRIVE dskGetFirstDrive (int8_t *pchDriveLetter)
{
    PDRIVE pDrive = NULL;
    PDSKLST pDiskList;
    pDiskList = gpDiskList;
    while (pDiskList)
    {
        if (pDiskList->chDriveLetter != '?')
        {
            _Bool bfAttached = false;

            /* Do a check to make sure that the device is attached */
            control(pDiskList->iMsDev, CTL_USB_ATTACHED, &bfAttached);
            if (bfAttached)
            {
                if (!pDiskList->pNext)
                {
                    /* The device is attached and we can use it */
                    pDrive = pDiskList->pDrive;
                    if (pchDriveLetter)
                    {
                        *pchDriveLetter = pDiskList->chDriveLetter;
                    }
                    break;
                }
            }
            else
            {
                /* The device is not attached and should be dismounted */
                dskEjectMedia(pDiskList->chDriveLetter);
            }
        }
        pDiskList = pDiskList->pNext;
    }
    return pDrive;
}
/*****************************************************************************
 End of function  dskGetFirstDrive
 ******************************************************************************/

/******************************************************************************
 Function Name: dskGetMediaState
 Description:   Function to get the state of the media in the drive
 Arguments:     IN  chDriveLetter - The drive letter of the disk
 OUT pbfMediaPresent - Pointer to a flag set true if the
 OUT pbfWriteProtected - Pointer to a flag set true if the disk is
 Return value:  true if the disk was found
 ******************************************************************************/
_Bool dskGetMediaState (int8_t chDriveLetter, _Bool *pbfMediaPresent, _Bool *pbfWriteProtected)
{
    PDSKLST pDiskList;
    _Bool bfResult = false;
    R_OS_SysWaitAccess();
    pDiskList = gpDiskList;
    while (pDiskList)
    {
        if (pDiskList->chDriveLetter == chDriveLetter)
        {
            /* Get the current write protect status */
            if (scsiModeSense(pDiskList->iMsDev, pDiskList->iLun, &pDiskList->bfWriteProtect))
            {
                /* Media sense command failed - assume R/W and continue. */
                pDiskList->bfWriteProtect = false;
                pDiskList->driveState = DRIVE_READY;
            }
            if (pbfMediaPresent)
            {
                if (pDiskList->driveState == DRIVE_READY)
                {
                    *pbfMediaPresent = true;
                }
                else
                {
                    *pbfMediaPresent = false;
                }
            }
            if (pbfWriteProtected)
            {
                *pbfWriteProtected = pDiskList->bfWriteProtect;
            }
            bfResult = true;
            break;
        }
        pDiskList = pDiskList->pNext;
    }
    R_OS_SysReleaseAccess();
    return bfResult;
}
/******************************************************************************
 End of function  dskGetMediaState
 ******************************************************************************/

/******************************************************************************
 Private Functions
 ******************************************************************************/

/*****************************************************************************
 Function Name: dskManager
 Description:   Task to manage the connection of PnP drives
 Arguments:     none
 Return value:  none
 *****************************************************************************/
void dskManager (void)
{
    /* Wait for the disk to be added to the device list */
    /* Until the task is destroyed */
    while (true)
    {
        /* Wait for the new drive event to be set */
        R_OS_WaitForEvent(&gpevNewDrive, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);

        /* Reset the signal */
        R_OS_ResetEvent(&gpevNewDrive);

        /* Wait for the disk to be added to the device list */
        R_OS_TaskSleep(50UL);

        /* Mount all devices */
        dskMountAllDevices();
    }
}
/*****************************************************************************
 End of function  dskManager
 ******************************************************************************/

/*****************************************************************************
 Function Name: dskGetDriveFromLetter
 Description:   Function to get a pointer to the drive object
 Arguments:     IN  chDriveLetter - The drive letter of the device
 Return value:  Pointer to the drive object or NULL on failure
 ******************************************************************************/
static PDRIVE dskGetDriveFromLetter (int8_t chDriveLetter)
{
    PDRIVE pDrive = NULL;
    PDSKLST pDiskList;
    pDiskList = gpDiskList;
    while (pDiskList)
    {
        if (pDiskList->chDriveLetter == chDriveLetter)
        {
            _Bool bfAttached = false;

            /* Do a check to make sure that the device is attached */
            control(pDiskList->iMsDev, CTL_USB_ATTACHED, &bfAttached);
            if (bfAttached)
            {
                /* The device is attached and we can use it */
                pDrive = pDiskList->pDrive;
                break;
            }
            else
            {
                /* The device is not attached and should be dismounted */
                dskEjectMedia(chDriveLetter);
                break;
            }
        }
        pDiskList = pDiskList->pNext;
    }
    return pDrive;
}
/*****************************************************************************
 End of function  dskGetDriveFromLetter
 ******************************************************************************/

/******************************************************************************
 Function Name: dskFree
 Description:   Function to free the memory allocated for a disk
 Arguments:     pDisk - Pointer to the disk object
 Return value:  none
 ******************************************************************************/
static void dskFree (PDSKLST pDisk)
{
    if (pDisk->pDrive)
    {
        R_FAT_DestroyDrive(pDisk->pDrive);
        pDisk->pDrive = NULL;
    }
}
/******************************************************************************
 End of function  dskFree
 ******************************************************************************/

/******************************************************************************
 Function Name: dskMountDevice
 Description:   Function to mount a disk
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 Return value:  0 for success otherwize error code
 ******************************************************************************/
static DSKERR dskMountDevice (int iMsDev)
{
    uint8_t byMaxLun = 0;
    uint8_t byLun;

    /* LUN field is 3 bits on the SCSI interface and 4 bits on the USB
     interface */
    DSKERR pDskError[16];

    /* Reset the device */
    if (usbMsClassReset(iMsDev))
    {
        return DISK_DRIVER_ERROR;
    }

    /* This is required for some slower USB MS Devices */
    R_OS_TaskSleep(300UL);

    /* Get the max number of logical units for this device */
    if (usbMsClassGetMaxLun(iMsDev, &byMaxLun))
    {
       TRACE(("dskMountDevice: Get Max LUN returned an Error, device may have stalled GetMaxLUN query,"
              "reference USB Mass Storage Class Rev 1.0, section 3.2; Assuming One LUN is available"));

       byMaxLun = 0;
    }

    /* This is required for a USB card reader */
    R_OS_TaskSleep(300UL);

    /* Try to mount each logical unit exposed by the device */
    for (byLun = 0; byLun <= byMaxLun; byLun++)
    {
        DSKLST newDisk;
        memset(&newDisk, 0, sizeof(DSKLST));
        newDisk.iMaxLun = (int) byMaxLun;

        /* Attempt to mount the unit and create the device */
        pDskError[byLun] = dskMountUnit(iMsDev, (int) byLun, &newDisk, true);
    }

    /* If an error, other than the media not being inserted occurred,
       report it */
    for (byLun = 0; byLun <= byMaxLun; byLun++)
    {
        if (pDskError[byLun] > DISK_MEDIA_NOT_AVAILABLE)
        {
            return pDskError[byLun];
        }
    }

return DISK_OK;
}
/******************************************************************************
 End of function  dskMountDevice
 ******************************************************************************/

/******************************************************************************
 Function Name: dskDismountDevice
 Description:   Function to dismount a device
 Arguments:     IN  idMsDev - The file descriptor of the device to dismount
 Return value:  none
 ******************************************************************************/
static void dskDismountDevice (int iMsDev)
{
    PDSKLST *ppDiskList = &gpDiskList;
    while (*ppDiskList)
    {
        /* Look for the device in the list */
        if ((*ppDiskList)->iMsDev == iMsDev)
        {
            dskRemoveDisk(*ppDiskList);
            ppDiskList = &gpDiskList;
        }
        else
        {
            ppDiskList = &(*ppDiskList)->pNext;
        }
    }
}
/******************************************************************************
 End of function  dskDismountDevice
 ******************************************************************************/

/******************************************************************************
 Function Name: dskAssignDriveLetter
 Description:   Function to assign a new drive letter
 Arguments:     none
 Return value:  The drive letter to use or -1 if none available
 ******************************************************************************/
static int8_t dskAssignDriveLetter (void)
{
    int8_t chDriveLetter = 'A';
    PDSKLST pDiskList = gpDiskList;
    while (pDiskList)
    {
        /* Check all drivers on the list for this letter */
        if (pDiskList->chDriveLetter == chDriveLetter)
        {
            /* Bump the letter */
            chDriveLetter++;
            /* Start from the top of the list */
            pDiskList = gpDiskList;
            /* no addresses available */
            if (chDriveLetter > 'Z')
            {
                return -1;
            }
        }
        else
        {
            pDiskList = pDiskList->pNext;
        }
    }
    return chDriveLetter;
}
/******************************************************************************
 End of function  dskAssignDriveLetter
 ******************************************************************************/

/******************************************************************************
 Function Name: dskMountPartition
 Description:   Function to mount a partition
 Arguments:     IN  pDisk - Pointer to the disk information
 IN  iPartition - The partition to mount
 Return value:  0 for success otherwize error code
 ******************************************************************************/
DSKERR dskMountPartition (PDSKLST pDisk, int iPartition)
{
    UNUSED_PARAM(iPartition);

    /* If this disk does not have a FAT drive object then create one */
    if (!pDisk->pDrive)
    {
         pDisk->pDrive = (PDRIVE) R_FAT_CreateDrive(pDisk->iMsDev,
                                                    pDisk->iLun,
                                                    pDisk->mediaGeometry.dwBlockSize,
                      pDisk->mediaGeometry.dwNumBlocks);
    }

    if (pDisk->pDrive)
    {
        /* Proposed_drive_index for this device needed by mount command */
        /* If the drive fails to mount this proposed_drive_index shall be re-used.*/
        /* Magic Number 65 is the ASCII code for 'A' being the first posible
           drive letter. */
        pDisk->pDrive->proposed_drive_index = (int8_t)(dskAssignDriveLetter() - (int8_t)65);

        /* Ask the FAT library to mount the disk */
        if (R_FAT_MountPartition(pDisk->pDrive))
        {
                 R_FAT_DestroyDrive(pDisk->pDrive);
            pDisk->pDrive = NULL;
            return DISK_FAILED_TO_MOUNT_PARTITION;
        }
        TRACE(("dskMountPartition: "
                        "iMsDev = %d, iLun = %d, bfWriteProtect %d, "
                        "dwBlockSize = %u, dwNumBlocks = %u\r\n",
                        pDisk->iMsDev, pDisk->iLun, pDisk->bfWriteProtect,
                        pDisk->mediaGeometry.dwBlockSize,
                        pDisk->mediaGeometry.dwNumBlocks));
        return DISK_OK;
    }
    TRACE(("dskMountPartition: No Memory\r\n"));
    return DISK_NO_MEMORY;
}
/******************************************************************************
 End of function  dskMountPartition
 ******************************************************************************/

/******************************************************************************
 Function Name: dskAddDisk
 Description:   Function to add a disk to the list
 Arguments:     IN  pDisk - Pointer to the disk to add
 Return value:  0 for success or error code
 ******************************************************************************/
static DSKERR dskAddDisk (PDSKLST pDisk)
{
    pDisk->pDrive = R_OS_AllocMem((size_t)sizeof_drive, R_REGION_LARGE_CAPACITY_RAM);

    if (NULL == pDisk->pDrive)
    {
        return DISK_NO_MEMORY;
    }

    memcpy(pDisk->pDrive, p_drive_0, (size_t)sizeof_drive);
    PDSKLST pNewDisk = (PDSKLST) R_OS_AllocMem(sizeof(DSKLST), R_REGION_LARGE_CAPACITY_RAM);
    if (pNewDisk)
    {
        /* Copy the disk info */
        *pNewDisk = *pDisk;
        /* Add to the top of the disk list */
        pNewDisk->pNext = gpDiskList;
        gpDiskList = pNewDisk;
        /* Set the drive letter for a "hidden" disk */
        pNewDisk->chDriveLetter = '?';
        /* If the unit is ready */
        if (pNewDisk->driveState == DRIVE_READY)
        {
            int8_t chDrive = dskAssignDriveLetter();
            if (chDrive > 0)
            {
                /* Assign a drive letter to this disk */
                pNewDisk->chDriveLetter = chDrive;
            }
        }
        TRACE(("dskAddDisk: %c @ 0x%p\r\n", pNewDisk->chDriveLetter, pNewDisk));
        return DISK_OK;
    }
    return DISK_NO_MEMORY;
}
/******************************************************************************
 End of function  dskAddDisk
 ******************************************************************************/

/******************************************************************************
 Function Name: dskCountAttachedDrives
 Description:   Function to find if the device has any other active units or partitions
 Arguments:     IN  iMsDev - The ID of the device to find
 Return value:  The number of active disks associated with this device
 ******************************************************************************/
static int dskCountAttachedDrives (int iMsDev)
{
    PDSKLST pDiskList = gpDiskList;
    int iCount = 0;
    while (pDiskList)
    {
        /* Look for the device in the list */
        if (pDiskList->iMsDev == iMsDev)
        {
            iCount++;
        }
        pDiskList = pDiskList->pNext;
    }
    return iCount;
}
/******************************************************************************
 End of function  dskCountAttachedDrives
 ******************************************************************************/

/******************************************************************************
 Function Name: dskRemoveDisk
 Description:   Function to remove a drive letter
 Arguments:     IN  pDisk - Pointer to the disk to remove
 Return value:  true if the drive was removed
 ******************************************************************************/
static _Bool dskRemoveDisk (PDSKLST pDisk)
{
    PDSKLST *ppDiskList = &gpDiskList;
    /* Look for the drive letter in the list */
    while ((*ppDiskList) && ((*ppDiskList) != pDisk))
    {
        ppDiskList = &(*ppDiskList)->pNext;
    }
    /* If it is found */
    if (*ppDiskList)
    {
        /* Get a pointer to the disk */
        PDSKLST pDiskToRemove = *ppDiskList;
        TRACE(("dskRemoveDisk: %c @ 0x%p\r\n", pDiskToRemove->chDriveLetter,
                        pDiskToRemove));
        /* Destroy the disk */
        dskFree(pDiskToRemove);
        /* Remove the disk from the list */
        *ppDiskList = pDiskToRemove->pNext;
        /* Free the disk object */
        R_OS_FreeMem(pDiskToRemove);

        return true;
    }
    return false;
}
/******************************************************************************
 End of function  dskRemoveDisk
 ******************************************************************************/

/******************************************************************************
 Function Name: dskTestUnitReady
 Description:   Function to check if the unit is ready
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 Return value:  0 for success otherwize error code
 ******************************************************************************/
static DSKERR dskTestUnitReady (int iMsDev, int iLun)
{
    _Bool bfReady;
    DSKERR dskError = DISK_DRIVER_ERROR;

    /* Check to see if the unit is ready. This is expected to fail because the
     drive should report that the media has been changed */
    if (!scsiTestUnitReady(iMsDev, iLun, &bfReady))
    {
        /* If it is not ready */
        if (!bfReady)
        {
            SCSIERR scsiError;
            scsiError = scsiRequestSense(iMsDev, iLun);
            switch (scsiError)
            {
                case SCSI_OK :
                    TRACE(("dskTestUnitReady: DISK_OK\r\n"));
                    dskError = DISK_OK;
                break;

                case SCSI_MEDIA_CHANGE :
                    TRACE(("dskTestUnitReady: DISK_MEDIA_CHANGE\r\n"));
                    dskError = DISK_MEDIA_CHANGE;
                break;

                case SCSI_MEDIA_NOT_PRESENT :
                    TRACE(("dskTestUnitReady: SCSI_MEDIA_NOT_PRESENT\r\n"));
                    dskError = DISK_MEDIA_NOT_PRESENT;
                break;

                case SCSI_MEDIA_NOT_AVAILABLE :
                    TRACE(("dskTestUnitReady: SCSI_MEDIA_NOT_AVAILABLE\r\n"));
                    dskError = DISK_MEDIA_NOT_AVAILABLE;
                break;

                default :
                    TRACE(("dskTestUnitReady: DISK_DRIVER_ERROR\r\n"));
                    dskError = DISK_DRIVER_ERROR;
                break;
            }
        }
        else
        {
            dskError = DISK_OK;
        }
    }
    return dskError;
}
/******************************************************************************
 End of function  dskTestUnitReady
 ******************************************************************************/

/******************************************************************************
 Function Name: dskMountUnit
 Description:   Function to mount a logical unit exposed by a device
 Arguments:     IN  iMsDev - The mass storage device file descriptor
 IN  iLun - The logical uint number
 IN  pDisk - Pointer to the disk information
 IN  bfAdd - Flag set true to add the disk
 Return value:  0 for success otherwize error code
 ******************************************************************************/
static DSKERR dskMountUnit (int iMsDev, int iLun, PDSKLST pDisk, _Bool bfAdd)
{
    int iRetryCount = 5;
    int iPartition;
    DSKERR dskError = DISK_OK;
    TRACE(("dskMountUnit: ID %d LUN %d\r\n", iMsDev, iLun));

    /* Wait for the unit to become ready */
    while (iRetryCount--)
    {
        dskError = dskTestUnitReady(iMsDev, iLun);
        if (dskError > DISK_MEDIA_NOT_AVAILABLE)
        {
            return dskError;
        }

        /* Stop if it is ready */
        else if (!dskError)
        {
            break;
        }
    }

    if (iRetryCount == 0)
    {
        return dskError;
    }

    /* Assign file descriptor and logical unit number */
    pDisk->iMsDev = iMsDev;
    pDisk->iLun = iLun;

    /* Check for media in the drive */
    if ((dskError == DISK_MEDIA_NOT_PRESENT) ||
        (dskError == DISK_MEDIA_NOT_AVAILABLE))
    {
        /* Set the drive state */
        if (dskError == DISK_MEDIA_NOT_PRESENT)
        {
            pDisk->driveState = DRIVE_NO_MEDIA;
        }
        else
        {
            pDisk->driveState = DRIVE_MEDIA_ERROR;
        }
        if (bfAdd)
        {
            dskAddDisk(pDisk);
        }
    }
    else
    {
        /* Get the media geometry */
        if (scsiReadCapacity10(iMsDev, iLun,
                                       &pDisk->mediaGeometry.dwNumBlocks,
                                       &pDisk->mediaGeometry.dwBlockSize))
        {
            return DISK_DRIVER_ERROR;
        } TRACE(("Capacity: Blocks %lu Size %lu\r\n",
                        pDisk->mediaGeometry.dwNumBlocks,
                        pDisk->mediaGeometry.dwBlockSize));

        /* Extra test unit ready required for Rion NA-28 */
        dskError = dskTestUnitReady(iMsDev, iLun);

        /* Get the media attributes */
        if (scsiModeSense(iMsDev, iLun, &pDisk->bfWriteProtect))
        {
            /* Media sense command failed - assume R/W and continue. */
            pDisk->bfWriteProtect = false;
        }

        /* Set the drive state */
        pDisk->driveState = DRIVE_READY;

        /* Attempt to mount each partition - JUST FIRST ONE FOR NOW */
        for (iPartition = 0; iPartition < 1; iPartition++)
        {
            /* Try to mount each partition */
            if (!dskMountPartition(pDisk, iPartition))
            {
                /* If FAT library mounted it then add it to the list */
                if (bfAdd)
                {
                    dskAddDisk(pDisk);
                }
                else
                {
                    /* Set the drive letter for a "hidden" disk */
                    pDisk->chDriveLetter = '?';
                    /* If the unit is ready */
                    if (pDisk->driveState == DRIVE_READY)
                    {
                        int8_t chDrive = dskAssignDriveLetter();
                        if (chDrive > 0)
                        {
                            /* Assign a drive letter to this disk */
                            pDisk->chDriveLetter = chDrive;
                        }
                    }
                }
            }
            else
            {
                TRACE(("Unknown Partition %d\r\n",
                                dskTestUnitReady(iMsDev, iLun)));
                if (bfAdd)
                {
                    /* Set the drive letter for a "hidden" disk */
                    pDisk->chDriveLetter = '?';
                    dskAddDisk(pDisk);
                }
            }
        }
    }
    return DISK_OK;
}
/******************************************************************************
 End of function  dskMountUnit
 ******************************************************************************/

/*****************************************************************************
Function Name: get_drive
Description:   Function find the drive in the drive list
Arguments:     iMsDev - drive number to search for
Return value:  PDRIVE the drive if found, NULL if not
*****************************************************************************/
PDRIVE get_drive (int drive)
{
    PDSKLST pListTop = gpDiskList;

    while (pListTop)
    {
        if (drive == pListTop->chDriveLetter - 'A')
        {
            return pListTop->pDrive;
        }

        /* Advance to the next disk on the list */
        pListTop = pListTop->pNext;
    }

    return p_drive_0;
}
/*****************************************************************************
End of function get_drive
******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
