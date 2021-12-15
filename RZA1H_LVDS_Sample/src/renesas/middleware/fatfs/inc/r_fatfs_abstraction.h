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
 * @headerfile     r_fatfs_abstraction.h
 * @brief          The interface functions to FullFAT file system
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 30.06.2018 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef R_FATFS_ABSTRACTION_H
    #define R_FATFS_ABSTRACTION_H

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_NONOS_MIDDLEWARE
 * @defgroup R_SW_PKG_93_FATFS FAT File System
 * @brief The Renesas FAT FS Abstraction Layer
 *
 * @anchor R_SW_PKG_93_FATFS_API_API_SUMMARY
 * @par Summary
 *
 * This middleware module contains an API Abstraction for the FAT File
 * System Library allowing for portability between different FAT File System
 * Implementations. 
 * 
 * @anchor R_SW_PKG_93_FATFS_API_INSTANCES
 * @par Known Implementations:
 * This driver is used in the RZA1H Software Package.
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/***********************************************************************************
 Defines
 ***********************************************************************************/
    #include "ff.h"
    #include "ff_types.h"
    #include "blockCache.h"

    #define FAT_ATTR_READONLY           (0x01)     /*!< Read Only      - FAT Atribute */  
    #define FAT_ATTR_HIDDEN             (0x02)     /*!< Hidden         - FAT Atribute */
    #define FAT_ATTR_SYSTEM             (0x04)     /*!< System         - FAT Atribute */
    #define FAT_ATTR_VOLID              (0x08)     /*!< Volume ID      - FAT Atribute */
    #define FAT_ATTR_DIR                (0x10)     /*!< Directory      - FAT Atribute */
    #define FAT_ATTR_ARCHIVE            (0x20)     /*!< Archive        - FAT Atribute */
    #define FAT_ATTR_LFN                (0x0F)     /*!< Long File Name - FAT Atribute */

    #define FAT_MODE_READ               (0x01)     /*!< Read            - FAT Mode */     
    #define FAT_MODE_WRITE              (0x02)     /*!< Write           - FAT Mode */
    #define FAT_MODE_APPEND             (0x04)     /*!< Append          - FAT Mode */
    #define FAT_MODE_CREATE             (0x08)     /*!< Create          - FAT Mode */
    #define FAT_MODE_TRUNCATE           (0x10)     /*!< Truncate        - FAT Mode */
    #define FAT_MODE_DIR                (0x80)     /*!< Directory       - FAT Mode */

/******************************************************************************
 Enumerated Types
 ***********************************************************************************/

typedef enum _FATERR
{

    FAT_ERROR_NONE = 0,                          /*!< No Error                         */
    FAT_ERROR_NULL_POINTER,                      /*!< NULL Pointer                     */
    FAT_ERROR_NOT_ENOUGH_MEMORY,                 /*!< Not Enough Memory                */
    FAT_ERROR_DEVICE_DRIVER_FAILED,              /*!< Driver Failure                   */
    FAT_ERROR_DRIVE_BAD_BLKSIZE,                 /*!< Bad Block Size Given             */
    FAT_ERROR_DRIVE_BAD_MEMSIZE,                 /*!< Bad Memory Size Given            */
    FAT_ERROR_DRIVE_DEV_ALREADY_REGD,            /*!< Device Already Registered        */ 
    FAT_ERROR_DRIVE_NO_MOUNTABLE_PARTITION,      /*!< No Mountable Partition Available */
    FAT_ERROR_DRIVE_INVALID_FORMAT,              /*!< Invalid Format                   */
    FAT_ERROR_DRIVE_INVALID_PARTITION_NUM,       /*!< Invalid Partition Number         */
    FAT_ERROR_DRIVE_NOT_FAT_FORMATTED,           /*!< Drive Not FAT Formatted          */
    FAT_ERROR_DRIVE_DEV_INVALID_BLKSIZE,         /*!< Device Invalid Block Size        */
    FAT_ERROR_DRIVE_PARTITION_MOUNTED,           /*!< Partition Mounted                */
    FAT_ERROR_DRIVE_ACTIVE_HANDLES,              /*!< Active Handles                   */
    FAT_ERROR_FILE_ALREADY_OPEN,                 /*!< Drive Already Openned            */
    FAT_ERROR_FILE_NOT_FOUND,                    /*!< File Not Found                   */
    FAT_ERROR_FILE_OBJECT_IS_A_DIR,              /*!< Object is a Directory            */
    FAT_ERROR_FILE_IS_READ_ONLY,                 /*!< File is Read Only                */
    FAT_ERROR_FILE_INVALID_PATH,                 /*!< Invalid File Path                */
    FAT_ERROR_FILE_NOT_OPENED_IN_WRITE_MODE,     /*!< File Not Opened in Write Mode    */
    FAT_ERROR_FILE_NOT_OPENED_IN_READ_MODE,      /*!< File Not Opened in Read Mode     */
    FAT_ERROR_FILE_EXTEND_FAILED,                /*!< File Extend failed               */
    FAT_ERROR_DIR_OBJECT_EXISTS,                 /*!< Object Exists                    */
    FAT_ERROR_DIR_DIRECTORY_FULL,                /*!< Directory Full                   */
    FAT_ERROR_DIR_END_OF_DIR,                    /*!< End of Directory                 */
    FAT_ERROR_DIR_NOT_EMPTY,                     /*!< Directory Not Empty              */
    FAT_ERROR_DIR_INVALID_PATH,                  /*!< Invalid Directory Path           */
    FAT_ERROR_DIR_CANT_EXTEND_ROOT_DIR,          /*!< Can't Extend Root Directory      */
    FAT_ERROR_NO_FREE_CLUSTERS,                  /*!< No Free Clusters                 */
    FAT_ERROR_FILE_DESTINATION_EXISTS,           /*!< File Destination Exists          */
    FAT_ERROR_FILE_SOURCE_NOT_FOUND,             /*!< File Source fot found            */
    FAT_ERROR_FILE_DIR_NOT_FOUND,                /*!< File Directory not found         */
    FAT_ERROR_FILE_COULD_NOT_CREATE_DIRENT,      /*!< File Could Not Create Directory Entry */
    FAT_ERROR_NO_LIBRARY_INSTALLED,              /*!< No Library Installed             */
    FAT_ERROR_UNKNOWN                            /*!< Unknown Error                    */

} FATERR;

typedef enum _FATTYPE
{
    FAT_12 = 0, /*!< 12-bit FAT Logical Filesystem */      
    FAT_16,     /*!< 16-bit FAT Logical Filesystem */  
    FAT_32      /*!< 32-bit FAT Logical Filesystem */
} FATTYPE;

/***********************************************************************************
 Typedefs
 ***********************************************************************************/

typedef struct _FATTIME
{
    unsigned short Year;
    unsigned short Month;
    unsigned short Day;
    unsigned short Hour;
    unsigned short Minute;
    unsigned short Second;

} FATTIME, *PFATTIME;

typedef struct _FATENTRY
{
    char           FileName[260];
    unsigned char  Attrib;
    unsigned int   Filesize;
    unsigned int   ObjectCluster;
    FATTIME        CreateTime;
    FATTIME        ModifiedTime;
    FATTIME        AccessedTime;
    unsigned short CurrentItem;
    unsigned int   DirCluster;
    unsigned int   CurrentCluster;
    unsigned int   AddrCurrentCluster;

} FATENTRY, *PFATENTRY;

typedef struct _DRIVEINFO
{
    FATTYPE   fatType;
    long long llFreeDisk;
    long long llVolumeSize;
    int       iBlockSize;
    int       iSectorSizeIn_k;

} DRIVEINFO, *PDRIVEINFO;

typedef struct _DRIVE *PDRIVE;
typedef struct FIL *PFILE;

extern PDRIVE p_drive_0;
extern int sizeof_drive;

typedef struct _DRIVE
{

    FATFS  *p_fat_fs;
    
    PBACHE pBlockCache; /*!< Pointer to a block cache object to minimise IO */
    
    DWORD  dwBlockSize; /*!< The block size of the device */
    
    DWORD  dwNumBlocks; /*!< The number of blocks */

    int    iMsDev; /*!< The file descriptor for this device */
    
    int8_t      proposed_drive_index; /*!< The proposed drive letter assignment for this device */

    int    iLun; /*!< The Logical Unit Number for this device */

} DRIVE;

/***********************************************************************************
 Public Functions
 ***********************************************************************************/

    #ifdef __cplusplus
extern "C"
{
    #endif

/**
 Function Name: R_FAT_LoadLibrary
 @brief   Function to show if a working FAT library is linked into the project
 Parameters:    none
 @retval  0 for success
 */
int R_FAT_LoadLibrary (void);

/**
 * @brief         Function to create the drive object
 * 
 * @param[in]  idMsDev:      The file descriptor of the MS device
 * @param[in]  iLun:         The logical unit number
 * @param[in]  dwBlockSize:  The block size of the device
 * @param[in]  dwNumBlocks:  The number of blocks
 * 
 * @retval     ptr_drv:      Pointer to the drive object
 */
PDRIVE R_FAT_CreateDrive (int iMsDev, int iLun, unsigned long dwBlockSize, unsigned long dwNumBlocks);

/**
 * @brief       Function to destroy the drive object
 * 
 * @param[in]   pDrive:       Pointer to the drive object to destroy
 * 
 * @retval      0: Success
 */
FRESULT R_FAT_DestroyDrive (PDRIVE pDrive);

/**
 * @brief   Function to mount a FAT partition (0..3)
 *    
 * @param[in]  pDrive:      Pointer to the drive object
 * @param[in]  iPartition:  The partition number to mount
 * 
 * @retval  0:  Success
 */
FRESULT R_FAT_MountPartition (PDRIVE p_drive);

/**
 * @brief   Function to get information about the disk
 *     
 * @param[in]  pDrive:      Pointer to the drive object
 * @param[out] pDriveInfo:  Pointer to the destination drive information
 * 
 * @retval  0:  Success
 */
FRESULT R_FAT_GetDriveInfo (char drive_letter, PDRIVE pDrive, PDRIVEINFO pDriveInfo);

/**
 * @brief   Function to open a file
 * 
 * @param[in]  pDrive:   Pointer to the drive object
 * @param[in]  pszFile:  Pointer to a file name
 * @param[in]  iMode:    The mode to open the file with
 * 
 * @retval     ptr_file: Pointer to the file object
 */
FIL *R_FAT_OpenFile (char *p_path, int iMode);

/**
 * @brief   Function to close a file
 * 
 *     
 * @param[in]  pFile: Pointer to the file object
 * 
 * @retval  0:  Success
 */

FRESULT R_FAT_CloseFile (FIL *p_file);

/**
 * @brief         Function to get the size of the file
 * 
 * @param[in]     pFile:  Pointer to the file object
 * 
 * @retval        file_size: The size of the file
 */

FSIZE_t R_FAT_SizeOfFile (FILINFO _pFile);

/**
 * @brief   Function to read data from a file
 *    
 * @param[out]  pbyDest:   Pointer to the destination memory
 * @param[in]   stLength:  The number of bytes to read
 * @param[in]   pFile:     Pointer to the file object
 * 
 * @retval     bytes_read: The number of bytes read  < 0 on error
 */
int R_FAT_ReadFile (FIL *p_file, void *p_buff, unsigned int no_of_bytes);

/**
 * @brief   Function to write data to a file
 *     
 * @param[in]  pFile:    Pointer to the file object
 * @param[in]  pbySrc:   Pointer to the source memory
 * @param[in]  stLength: The number of bytes to write
 * 
 * @retval  The number of bytes written < 0 on error
 */
int R_FAT_WriteFile (FIL *p_file, unsigned char *p_buff, unsigned int no_of_bytes);

/**
 * @brief     Function to seek to a postion in a file
 * 
 * @param[in]  pFile:    Pointer to the file object
 * @param[in]  lOffset:  The file offset
 * @param[in]  iOrigin:  The origin
 * 
 * @retval    0: for success
 */
FRESULT R_FAT_SeekFile (FIL *p_file, FS_T_UINT32 lOffset, int iOrigin, long *p_result);

/**
 *  @brief         Return the size of  a file
 *  
 *  @param[in]     _pFile: Pointer to the file object
 * 
 *  @retval        file_size: The size of the file
 */
FSIZE_t R_FAT_FileSize (FIL *pFile);

/**
 * @brief   Function to find out if this is the end of the file
 *     
 * @param[in]  pFile:  Pointer to the file object
 * 
 * @retval  non zero at end of the file
 */
int R_FAT_EndOfFile (FIL *p_file);

/**
 * @brief      Function to remove a file
 *    
 * @param[in]  pDrive:   Pointer to the drive object
 * @param[in]  pszPath:  Pointer to the path and name of the file to remove
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_RemoveFile (char *p_pszPath);

/**
 * @brief   Function to find the first FAT entry provided by the path
 *   
 * @param[in]    pDrive:    Pointer to the drive object
 * @param[out]   pEntry:    Pointer to the destination entry
 * @param[in]    pszPath:   Pointer to the file name
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_FindFirst (DIR *p_dir, FATENTRY *p_fat_entry, char *p_path, const char *p_pattern);

/**
 * @brief   Function to find the next entry
 *    
 * @param[in]  pDrive - Pointer to the drive object
 * @param[out] pEntry - Pointer to the entry
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_FindNext (DIR *p_dir, FATENTRY *p_fno);

/**
 * @brief      Function to rewind the find to the first entry 
 * 
 * @param[in]  pDrive - Pointer to the drive object
 * @param[out] pEntry - Pointer to the entry
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_RewindFind (PDRIVE pDrive, PFATENTRY pEntry);

/**
 * @brief   Function to find a directory
 * 
 * @param[in]  pDrive - Pointer to the drive object
 * @param[in]  pszPath - Pointer to the directory path
 * @param[in]  stPathLength - The length of the path
 * @param[out] pszOutPath - Pointer to the actual path
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_FindDirectory (char *p_path, char *output_path);

/**
 * @brief   Function to make a directory
 * 
 * @param[in]  pDrive - Pointer to the drive object
 * @param[in]  pszPath - Pointer to the directory path
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_MakeDirectory (char *p_path);

/**
 * @brief   Function to remove a directory
 *   
 * @param[in]  pDrive:   Pointer to the drive object
 * @param[in]  pszPath:  Pointer to the directory path
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_RemoveDirectory (char *p_path);

/**
 * @brief   Function to rename a file or directory
 *  
 * @param[in]  pszOldName:  Pointer to the file directory path string
 * @param[in]  pszNewName:  Pointer to the new name string
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_ReName (const char *p_old_name, const char *p_new_name);

/**
 * @brief   Function to format a drive
 *     
 * @param[in]  pDrive:  Pointer to the drive object
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_FormatDrive (PDRIVE pDrive);

/**
 * @brief   Function to check to see if the drive is available.
 * 
 * @warning This is not implemented at the moment and signals that the
 *          drive is available without checking.
 *   
 * @param[in]   pDrive:   Pointer to the disk information
 * 
 * @retval     0: Success
 */
int R_FAT_DriveIsAvailable (PDRIVE pDrive);

/**
 * @brief    Function to translate the FullFAT error code to the library code
 *     
 * @param[in]  iFullFatError:  The FullFAT error code
 * 
 * @retval     0: Success
 */
FRESULT R_FAT_ConvertErrorCode (FS_T_INT32 iFullFatError);

/**
 * @brief   Function to convert the error code in to a string
 *    
 * @param[in]  iErrorCode:   The error code
 * 
 * @retval     string:   Error String 
 */
const char *R_FAT_GetErrorString (FATERR iErrorCode);

    #ifdef __cplusplus
}
    #endif

#endif                              /* R_FATFS_ABSTRACTION_H */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/**********************************************************************************
 End  Of File
 ***********************************************************************************/
