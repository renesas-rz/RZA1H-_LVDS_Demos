/******************************************************************************
 * File Name    : FATLibrary.c
 * Version      : 1.0
 * Device(s)    : Renesas
 * Tool-Chain   : GNU
 * OS           : None
 * H/W Platform : RZA1
 * Description  : The interface functions to FullFAT file system
 ******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : DD.MM.YYYY 1.00 MAB First Release
 ******************************************************************************/

/******************************************************************************

 Generic API functions for a computer file system adapted for use with FullFAT
 Copyright (C) 2009. Renesas Technology Corp.
 Copyright (C) 2009. Renesas Technology Europe Ltd.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ******************************************************************************/

/***********************************************************************************
 System Includes
 ***********************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "blockCache.h"
#include "compiler_settings.h"
#include "r_fatfs_abstraction.h"
#include "console.h"

/***********************************************************************************
 User Includes
 ***********************************************************************************/

#include "types.h"
#include "ff.h"
#include "ff_types.h"

/***********************************************************************************
 Defines
 ***********************************************************************************/

#define FF_T_FAT12              0x0A
#define FF_T_FAT16              0x0B
#define FF_T_FAT32              0x0C

extern DRIVE drive_0;
/***********************************************************************************
 Typedefs
 ***********************************************************************************/

/***********************************************************************************
 Function Prototypes
 ***********************************************************************************/

/******************************************************************************
 Constant Data
 ***********************************************************************************/

typedef struct
{
    FATERR fatError;
    int    iFullFatError;
} ERRTAB;

ERRTAB gs_error_tab[] =
{
{ FR_OK, 0 }, /* (0) Succeeded */
{ FR_DISK_ERR, 1 }, /* (1) A hard error occurred in the low level disk I/O layer */
{ FR_INT_ERR, 2 }, /* (2) Assertion failed */
{ FR_NOT_READY, 3 }, /* (3) The physical drive cannot work */
{ FR_NO_FILE, 4 }, /* (4) Could not find the file */
{ FR_NO_PATH, 5 }, /* (5) Could not find the path */
{ FR_INVALID_NAME, 6 }, /* (6) The path name format is invalid */
{ FR_DENIED, 7 }, /* (7) Access denied due to prohibited access or directory full */
{ FR_EXIST, 8 }, /* (8) Access denied due to prohibited access */
{ FR_INVALID_OBJECT, 9 }, /* (9) The file/directory object is invalid */
{ FR_WRITE_PROTECTED, 10 }, /* (10) The physical drive is write protected */
{ FR_INVALID_DRIVE, 11 }, /* (11) The logical drive number is invalid */
{ FR_NOT_ENABLED, 12 }, /* (12) The volume has no work area */
{ FR_NO_FILESYSTEM, 13 }, /* (13) There is no valid FAT volume */
{ FR_MKFS_ABORTED, 14 }, /* (14) The f_mkfs() aborted due to any problem */
{ FR_TIMEOUT, 15 }, /* (15) Could not get a grant to access the volume within defined period */
{ FR_LOCKED, 16 }, /* (16) The operation is rejected according to the file sharing policy */
{ FR_NOT_ENOUGH_CORE, 17 }, /* (17) LFN working buffer could not be allocated */
{ FR_TOO_MANY_OPEN_FILES, 18 }, /* (18) Number of open files > FF_FS_LOCK */
{ FR_INVALID_PARAMETER, 19 },
{ -1, -1 }/* (19) Given parameter is invalid */
};

static const char *gsp_error_strings[] =
{ "FR_OK", /* (0) Succeeded */
"FR_DISK_ERR", /* (1) A hard error occurred in the low level disk I/O layer */
"FR_INT_ERR", /* (2) Assertion failed */
"FR_NOT_READY", /* (3) The physical drive cannot work */
"FR_NO_FILE", /* (4) Could not find the file */
"FR_NO_PATH", /* (5) Could not find the path */
"FR_INVALID_NAME", /* (6) The path name format is invalid */
"FR_DENIED", /* (7) Access denied due to prohibited access or directory full */
"FR_EXIST", /* (8) Access denied due to prohibited access */
"FR_INVALID_OBJECT", /* (9) The file/directory object is invalid */
"FR_WRITE_PROTECTED", /* (10) The physical drive is write protected */
"FR_INVALID_DRIVE", /* (11) The logical drive number is invalid */
"FR_NOT_ENABLED", /* (12) The volume has no work area */
"FR_NO_FILESYSTEM", /* (13) There is no valid FAT volume */
"FR_MKFS_ABORTED", /* (14) The f_mkfs() aborted due to any problem */
"FR_TIMEOUT", /* (15) Could not get a grant to access the volume within defined period */
"FR_LOCKED", /* (16) The operation is rejected according to the file sharing policy */
"FR_NOT_ENOUGH_CORE", /* (17) LFN working buffer could not be allocated */
"FR_TOO_MANY_OPEN_FILES", /* (18) Number of open files > FF_FS_LOCK */
"FR_INVALID_PARAMETER" /* (19) Given parameter is invalid */
};

void map_drive_id (char *p_path);
void remove_path_drive (char *path);
static void replace_character (char *string, char find, char replace);

/**********************************************************************************
 Function Name: map_drive_id
 Description:   Function to map drive A to 0, etc
 Parameters:    p_path - the path
 Return value:  None
 **********************************************************************************/
void map_drive_id (char *p_path)
{
    if (p_path[1] == ':')
    {
        if ((p_path[0] >= 'A') && (p_path[0] <= 'H'))
        {
            p_path[0] = (char) (p_path[0] - 'A');
            p_path[0] = (char) (p_path[0] + '0');
        }

        if ((p_path[0] >= 'a') && (p_path[0] <= 'h'))
        {
            p_path[0] = (char) (p_path[0] - 'a');
            p_path[0] = (char) (p_path[0] + '0');
        }
    }
}
/**********************************************************************************
 End of function map_drive_id
 ***********************************************************************************/

/**********************************************************************************
 Function Name: replace_character
 Description:   Function to map drive A to 0, etc
 Parameters:    string - string
 find - character to find
 replace - character to replace it with
 Return value:  None
 **********************************************************************************/
static void replace_character (char *string, char find, char replace)
{
    int i;

    for (i = 0; string[i] != '\0'; i++)
    {
        if (find == string[i])
        {
            string[i] = replace;
        }
    }
}
/**********************************************************************************
 End of function replace_character
 ***********************************************************************************/

/**********************************************************************************
 Function Name: remove_path_drive
 Description:   Remove the drive from the given path
 Parameters:    path - the path
 Return value:  None
 **********************************************************************************/
void remove_path_drive (char *path)
{
    memcpy(path, &path[3], strlen(path) - 2);
}
/**********************************************************************************
 End of function remove_path_drive
 ***********************************************************************************/

/**********************************************************************************
 Function Name: map_filinfo_to_fatentry
 Description:   Map a FILINFO structure onto a FATENTRY structure
 Parameters:    info - pointer to FILINFO structure
 p_fat_entry - pointer to FATENTRY structure
 Return value:  None
 **********************************************************************************/
static void map_filinfo_to_fatentry (FILINFO *info, FATENTRY *p_fat_entry)
{
    p_fat_entry->Filesize = info->fsize;
    p_fat_entry->CreateTime.Year = (unsigned short) ((info->fdate >> 9) + 1980);
    p_fat_entry->CreateTime.Month = (info->fdate >> 5) & 0xf;
    p_fat_entry->CreateTime.Day = info->fdate & 0x1f;
    p_fat_entry->CreateTime.Hour = (info->ftime >> 11) & 0x1f;
    p_fat_entry->CreateTime.Minute = (info->ftime >> 5) & 0x3f;
    p_fat_entry->CreateTime.Second = (info->ftime & 0x1f) << 1;
    p_fat_entry->ModifiedTime = p_fat_entry->CreateTime;
    p_fat_entry->AccessedTime = p_fat_entry->CreateTime;
    p_fat_entry->Attrib = info->fattrib;
    strcpy(p_fat_entry->FileName, info->fname);
}
/**********************************************************************************
 End of function map_filinfo_to_fatentry
 ***********************************************************************************/

/***********************************************************************************
 Public Functions
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_LoadLibrary
 Description:   Function to show if a working FAT library is linked into the project
 Parameters:    none
 Return value:  0 for success
 **********************************************************************************/
int R_FAT_LoadLibrary (void)
{
    return 0;
}
/**********************************************************************************
 End of function R_FAT_LoadLibrary
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_CreateDrive
 Description:   Function to create the drive object
 Parameters:    IN  idMsDev - The file descriptor of the MS device
 IN  iLun - The logical unit number
 IN  dwBlockSize - The block size of the device
 IN  dwNumBlocks - The number of blocks
 Return value:  Pointer to the drive object
 **********************************************************************************/
PDRIVE R_FAT_CreateDrive (int iMsDev, int iLun, unsigned long dwBlockSize, unsigned long dwNumBlocks)
{
    memset( &drive_0, 0, sizeof(DRIVE));
    PDRIVE p_drive = &drive_0;

    if (p_drive)
    {
        p_drive->pBlockCache = bcCreate(iMsDev, iLun, 8, 64, (int) dwBlockSize, dwNumBlocks);
        if ( !p_drive->pBlockCache)
        {
            R_OS_FreeMem(p_drive);
        }
    }

    p_drive->dwBlockSize = dwBlockSize;
    p_drive->dwNumBlocks = dwNumBlocks;
    p_drive->iMsDev = iMsDev;
    p_drive->iLun = iLun;

    return p_drive;
}
/**********************************************************************************
 End of function  R_FAT_CreateDrive
 ***********************************************************************************/

FRESULT R_FAT_DestroyDrive (PDRIVE pDrive)
{
    bcDestroy(pDrive->pBlockCache);
    R_OS_FreeMem(pDrive);

    return 0;
}
/**********************************************************************************
 End of function  R_FAT_DestroyDrive
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_MountPartition
 Description:
 Parameters:    IN  pDrive - Pointer to the drive object
 IN  iPartition - The partition number to mount
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_MountPartition (PDRIVE p_drive)
{
    UINT result;
    char buffer[10];

    p_drive->p_fat_fs = R_OS_AllocMem(sizeof(FATFS), R_REGION_LARGE_CAPACITY_RAM);

    if (NULL == p_drive->p_fat_fs)
    {
        printf("R_OS_AllocMem failed");
    }
    else
    {
        /* Setting third argument to one forces the FATFS to mount */
        sprintf(buffer, "%d:", p_drive->proposed_drive_index);
        result = f_mount(p_drive->p_fat_fs, buffer, 1);

        if (FR_OK == result)
        {

        }
        else
        {
            printf("Mount failed [%d]\n\r", result);
        }
    }

    return FAT_ERROR_NONE;
}
/**********************************************************************************
 End of function  R_FAT_MountPartition
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_GetDriveInfo

 Description:   Function to get information about the disk
 Parameters:    IN  pDrive - Pointer to the drive object
 OUT pDriveInfo - Pointer to the destination drive information
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_GetDriveInfo (char drive_letter, PDRIVE pDrive, PDRIVEINFO pDriveInfo)
{
    DWORD free_clusters;
    DWORD free_sectors;
    DWORD total_sectors;
    char path[3];
    const int sector_size = 512;

    path[0] = drive_letter;
    path[1] = ':';
    path[2] = '\0';
    map_drive_id(path);

    if ((NULL != pDrive) && (NULL != pDriveInfo))
    {
        pDriveInfo->fatType = pDrive->p_fat_fs->fs_type - FS_FAT12;
        f_getfree(path, &free_clusters, &pDrive->p_fat_fs);

        /* Get total sectors and free sectors */
        total_sectors = (pDrive->p_fat_fs->n_fatent - 2) * pDrive->p_fat_fs->csize;
        free_sectors = free_clusters * pDrive->p_fat_fs->csize;

        pDriveInfo->llFreeDisk = free_sectors;
        pDriveInfo->llFreeDisk *= sector_size;
        pDriveInfo->llVolumeSize = total_sectors;
        pDriveInfo->llVolumeSize *= sector_size;
        pDriveInfo->iBlockSize = (int) pDrive->dwBlockSize;
        pDriveInfo->iSectorSizeIn_k = (int) ((sector_size * pDrive->p_fat_fs->csize) / 1024);

        return FR_OK;
    }

    return FR_INVALID_OBJECT;
}
/**********************************************************************************
 End of function  fatGetDriveInfo
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_OpenFile
 Description:   Function to open a file
 Parameters:    IN  pDrive - Pointer to the drive object
 IN  pszFile - Pointer to a file name
 IN  iMode - The mode to open the file with
 Return value:  Pointer to the file object
 **********************************************************************************/
FIL *R_FAT_OpenFile (char *p_path, int iMode)
{
    /* Need to allow space for /0 otherwise strncpy() may be corrupted */
    uint32_t len_path = (strlen(p_path)+1);
    FRESULT result;
    FIL *fp = NULL;

    /* get duplicate of path to allow characters to be replaced */
    uint8_t *p_path_local = (uint8_t *) R_OS_AllocMem(len_path, R_REGION_LARGE_CAPACITY_RAM);

    if (NULL != p_path_local)
    {
    	/* copy the string terminator (which is not included in strlen */
        strncpy((char *)p_path_local, p_path, len_path);

        map_drive_id((char *)p_path_local);

        fp = (FIL *) R_OS_AllocMem(sizeof(FIL), R_REGION_LARGE_CAPACITY_RAM);

        if (NULL == fp)
        {
            printf("R_FAT_OpenFile() malloc() failed\n\r");
        }
        else
        {
            result = f_open(fp, (char *)p_path_local, (BYTE) iMode);
            if (result)
            {
                fp = NULL;
            }
        }

        /* allocation for path not now needed */
        R_OS_FreeMem((void *) p_path_local);
    }

    return (fp);
}
/**********************************************************************************
 End of function  R_FAT_OpenFile
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_CloseFile
 Description:   Function to close a file
 Parameters:    IN  pFile - Pointer to the file object
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_CloseFile (FIL *p_file)
{
    FRESULT result = f_close(p_file);

    R_OS_FreeMem(p_file);

    return R_FAT_ConvertErrorCode(result);
}
/**********************************************************************************
 End of function  R_FAT_CloseFile
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_SizeOfFile
 Description:   Function to get the size of the file
 Parameters:    IN  _pFile - Pointer to the file object
 Return value:  The size of the file
 **********************************************************************************/
FSIZE_t R_FAT_SizeOfFile (FILINFO _pFile)
{
    return _pFile.fsize;
}
/**********************************************************************************
 End of function  R_FAT_SizeOfFile
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_ReadFile
 Description:   Function to read data from a file
 Parameters:    OUT pbyDest - Pointer to the destination memory
 IN  stLength - The number of bytes to read
 IN  pFile - Pointer to the file object
 Return value:  The number of bytes read  < 0 on error
 **********************************************************************************/
int R_FAT_ReadFile (FIL *p_file, void *p_buff, unsigned int no_of_bytes)
{
    unsigned int p_bytes_read = 0;

    f_read(p_file, p_buff, no_of_bytes, &p_bytes_read);

    return (int) p_bytes_read;
}
/**********************************************************************************
 End of function  R_FAT_ReadFile
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_WriteFile
 Description:   Function to wrte data to a file
 Parameters:    IN  pFile - Pointer to the file object
 IN  pbySrc - Pointer to the source memory
 IN  stLength - The number of bytes to write
 Return value:  The number of bytes written < 0 on error
 **********************************************************************************/
int R_FAT_WriteFile (FIL *p_file, unsigned char *p_buff, unsigned int no_of_bytes)
{
    unsigned int bytes_written = 0;
    int return_value;

    return_value = f_write(p_file, p_buff, no_of_bytes, &bytes_written);

    if (no_of_bytes != bytes_written)
    {
        return_value = -1;
    }
    else
    {
        return_value = (int) bytes_written;
    }

    return return_value;
}
/**********************************************************************************
 End of function  R_FAT_WriteFile
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_SeekFile
 Description:   Function to seek to a position in a file
 Parameters:    IN  _pFile - Pointer to the file object
 IN  lOffset - The file offset
 IN  iOrigin - The origin
 OUT pResult - The final position of the file pointer.
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_SeekFile (FIL *p_file, FS_T_UINT32 lOffset, int iOrigin, long *p_result)
{
    UNUSED_PARAM(iOrigin);
    FRESULT err;

    err = f_lseek(p_file, lOffset);

    *p_result = (signed)p_file->fptr;

    return R_FAT_ConvertErrorCode(err);
}
/**********************************************************************************
 End of function  R_FAT_SeekFile
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_FileSize
 Description:   Return the size of  a file
 Parameters:    IN  _pFile - Pointer to the file object
 Return value:  The size of the file
 **********************************************************************************/
FSIZE_t R_FAT_FileSize (FIL *pFile)
{
    FSIZE_t size = f_size(pFile);

    return size;
}
/**********************************************************************************
 End of function R_FAT_SeekFile
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_EndOfFile
 Description:   Function to find out if this is the end of the file
 Parameters:    IN  pFile - Pointer to the file object
 Return value:  non zero at end of the file
 **********************************************************************************/
int R_FAT_EndOfFile (FIL *p_file)
{
    return f_eof(p_file);
}
/**********************************************************************************
 End of function  R_FAT_EndOfFile
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_RemoveFile
 Description:   Function to remove a file
 Parameters:    IN  pDrive - Pointer to the drive object
 IN  pszPath - Pointer to the path and name of the file to remove
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_RemoveFile (char *p_pszPath)
{
    FRESULT result;
    map_drive_id(p_pszPath);

    result = f_unlink(p_pszPath);

    result =  R_FAT_ConvertErrorCode(result);

    return result;
}
/**********************************************************************************
 End of function  R_FAT_RemoveFile
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_FindFirst
 Description:   Function to find the first FAT entry providedby the path
 Parameters:    IN  pDrive - Pointer to the drive object
 OUT pEntry - Pointer to the destination entry
 IN  pszPath - Pointer to the file name
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_FindFirst (DIR *p_dir, FATENTRY *p_fat_entry, char *p_path, const char *p_pattern)
{
    FRESULT err;

    FILINFO file_info;

    map_drive_id(p_path);

    err = f_findfirst(p_dir, &file_info, p_path, p_pattern);

    map_filinfo_to_fatentry( &file_info, p_fat_entry);

    return err;
}
/**********************************************************************************
 End of function  R_FAT_FindFirst
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_FindNext
 Description:   Function to find the next entry
 Parameters:    IN  pDrive - Pointer to the drive object
 OUT p_fatentry - Pointer to the entry
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_FindNext (DIR *p_dir, FATENTRY *p_fatentry)
{
    FRESULT err;

    FILINFO file_info;

    err = f_findnext(p_dir, &file_info);

    map_filinfo_to_fatentry( &file_info, p_fatentry);

    if ('\0' == p_fatentry->FileName[0])
    {
        err = FR_NO_FILE;
    }

    return R_FAT_ConvertErrorCode(err);
}
/**********************************************************************************
 End of function  R_FAT_FindNext
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_RewindFind
 Description:   Function to rewind the find to the first entry
 Parameters:    IN  pDrive - Pointer to the drive object
 OUT pEntry - Pointer to the entry
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_RewindFind (PDRIVE pDrive, PFATENTRY pEntry)
{
    UNUSED_PARAM(pDrive);
    pEntry->CurrentItem = 0;
    return FAT_ERROR_NONE;
}

/**********************************************************************************
 End of function  R_FAT_RewindFind
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_FindDirectory
 Description:   Function to find a directory
 Parameters:    IN  pDrive - Pointer to the drive object
 IN  pszPath - Pointer to the directory path
 IN  stPathLength - The length of the path
 OUT pszOutPath - Pointer to the actual path
 Return value:  non zero on success
 **********************************************************************************/
FRESULT R_FAT_FindDirectory (char *p_path, char *output_path)
{
    UINT retval;

    map_drive_id(p_path);

    retval = f_chdir(p_path);

    f_getcwd(output_path, CMD_MAX_PATH);

    /* replace '/' with '\' */
    replace_character(output_path, '/', '\\');

    remove_path_drive(output_path);

    return (FR_OK == retval);
}
/**********************************************************************************
 End of function  R_FAT_FindDirectory
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_MakeDirectory
 Description:   Function to make a directory
 Parameters:    IN  pDrive - Pointer to the drive object
 IN  pszPath - Pointer to the directory path
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_MakeDirectory (char *p_path)
{
    FATERR err;

    map_drive_id(p_path);

    err = f_mkdir(p_path);

    return R_FAT_ConvertErrorCode(err);
}
/**********************************************************************************
 End of function  R_FAT_MakeDirectory
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_RemoveDirectory
 Description:   Function to remove a directory
 Parameters:    IN  pDrive - Pointer to the drive object
 IN  pszPath - Pointer to the directory path
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_RemoveDirectory (char *p_path)
{
    map_drive_id(p_path);

    return R_FAT_ConvertErrorCode((FATERR) f_unlink(p_path));
}
/**********************************************************************************
 End of function  R_FAT_RemoveDirectory
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_ReName
 Description:   Function to rename a file or directory
 Parameters:    IN  p_old_name - Pointer to the file directory path string
 IN  p_new_name - Pointer to the new name string
 Return value:  0 for success
 **********************************************************************************/
FRESULT R_FAT_ReName (const char *p_old_name, const char *p_new_name)
{
    FATERR err = f_rename(p_old_name, p_new_name);

    /* */
    return R_FAT_ConvertErrorCode((FS_T_INT32) err);
}
/**********************************************************************************
 End of function  R_FAT_ReName
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_FormatDrive
 Description:   Function to format a drive
 Parameters:    IN  pDrive - Pointer to the drive object
 Return value:  0 for succsess
 **********************************************************************************/
FRESULT R_FAT_FormatDrive (PDRIVE pDrive)
{
    UNUSED_PARAM(pDrive);
    printf("NOT SUPPORTED IN FATFS");
    return ( -1);
}
/**********************************************************************************
 End of function  R_FAT_FormatDrive
 ***********************************************************************************/

/*****************************************************************************
 Function Name: fatDriveIsAvailable
 Description:   Function to check to see if the drive is available.
 This is not implemented at the moment and signals that the
 drive is available without checking.
 Parameters:    IN  pDrive - Pointer to the disk information
 Return value:  non zero if the drive is available
 *****************************************************************************/
int R_FAT_DriveIsAvailable (PDRIVE pDrive)
{
    UNUSED_PARAM(pDrive);
    return (1);
}
/*****************************************************************************
 End of function  fatDriveIsAvailable
 ******************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_GetErrorString
 Description:   Function to convert the error code in to a string
 Parameters:    IN  iErrorCode - The error code
 Return value:  The string
 **********************************************************************************/
const char *R_FAT_GetErrorString (FATERR iErrorCode)
{
    if (iErrorCode < FAT_ERROR_UNKNOWN)
    {
        return gsp_error_strings[iErrorCode];
    }

    return "FAT_ERROR_UNKNOWN";
}
/**********************************************************************************
 End of function  R_FAT_GetErrorString
 ***********************************************************************************/

/**********************************************************************************
 Function Name: R_FAT_ConvertErrorCode
 Description:   Function to translate the FullFAT error code to the library code
 Parameters:    IN  iFullFatError - The FullFAT error code
 Return value:  The FAT error code
 **********************************************************************************/
FRESULT R_FAT_ConvertErrorCode (FS_T_INT32 iFatFsError)
{
    uint8_t i;

    for(i=0; i<20; i++)
    {
        if (gs_error_tab[i].fatError == iFatFsError)
        {
            return gs_error_tab[i].iFullFatError;
        }
    }

    return FAT_ERROR_UNKNOWN;
}
/**********************************************************************************
 End of function  R_FAT_ConvertErrorCode
 ***********************************************************************************/

/***********************************************************************************
 End  Of File
 ***********************************************************************************/

