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
 * Copyright (C) 2010 Renesas Electronics Corporation. All rights reserved.    */
/******************************************************************************
 * File Name    : fileDriver.c
 * Version      : 1.00
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : The driver for a file on a disk.
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 MAB First Release
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "fileDriver.h"
#include "dskManager.h"
#include "wild_compare.h"
#include "trace.h"
#include "ff.h"
#include "r_fatfs_abstraction.h"
#include "dev_drv.h"

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

static int fileOpen (st_stream_ptr_t p_stream);
static void fileClose (st_stream_ptr_t p_stream);
static int fileRead (st_stream_ptr_t p_stream, uint8_t *pbyBuffer, uint32_t uiCount);
static int fileWrite (st_stream_ptr_t p_stream, uint8_t *pbyBuffer, uint32_t uiCount);
static int fileControl (st_stream_ptr_t p_stream, uint32_t ctlCode, void *pCtlStruct);
static int_t fileGetVersion (st_stream_ptr_t p_stream, st_ver_info_t *p_ver_info);

/******************************************************************************
 Constant Data
 ******************************************************************************/

static const st_drv_info_t gs_hld_info =
{
    {
        ((STDIO_FILE_DRIVER_RZ_HLD_VERSION_MAJOR << 16) + STDIO_FILE_DRIVER_RZ_HLD_VERSION_MINOR)
    },
    STDIO_FILE_DRIVER_RZ_HLD_BUILD_NUM,
    STDIO_FILE_DRIVER_RZ_HLD_DRV_NAME
};


/* Define the driver function table for this */
const st_r_driver_t gFatfsDriver =
{ "FatFS library file driver IO wrapper", fileOpen, fileClose, fileRead, fileWrite, fileControl, fileGetVersion};

///* Characters that are not allowed in a file name */
//static const int8_t pszIllegalFileNameCharacters[] =
//{ "<>:\"/\\|?*" };
//
///* File names that are reserved under windows */
//static const char * const pszIllegalFileNames[] =
//{"AUX", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "CON", "LPT1", "LPT2", "LPT3", "LPT4",
//        "LPT5", "LPT6", "LPT7", "LPT8", "LPT9", "NUL", "PRN", ".", "..", ""};

/******************************************************************************
 Public Functions
 ******************************************************************************/


/******************************************************************************
 Function Name: fileLoadDriver
 Description:   Function to load a file driver for a file
 Arguments:     IN  pStream - Pointer to the file stream
 IN  pszFileName - Pointer to a file name
 IN  iMode - The file mode
 Return value:  Pointer to the file driver or NULL if not found
 ******************************************************************************/
st_r_driver_t* fileLoadDriver (st_stream_ptr_t stream_ptr, char * pszFileName, int iMode)
{
    int8_t chDrive = (int8_t) toupper(*pszFileName);

    /* The first character of the file name must be the drive letter */

    /* drive may have been mapped to a digit for FatFS - map it back to a letter */
    if ((chDrive >= '0') && (chDrive <= '8'))
    {
        chDrive = (int8_t) (chDrive - '0');
        chDrive = (int8_t) (chDrive + 'A');
    }

    PDRIVE pDrive = dskGetDrive(chDrive);

    if (pDrive)
    {
        int iffMode = 0;

        /* The ':' character delimits the drive from the file path and name */
        char * pszPath = strstr(pszFileName, ":");

        if (pszPath)
        {
            FIL *pFile;

            /* Check for write protection */
            if ((iMode & O_RDWR) || (iMode & O_WRONLY))
            {
                _Bool bfWriteProtect = true;

                /* Check that the medium is not write protected */
                dskGetMediaState(chDrive, NULL, &bfWriteProtect);
                if (bfWriteProtect)
                {
                    /* Error - read only file system!
                     EROFS - Not defined in the Renesas library which
                     appears to be missing from errno.h */
                    errno = -30;
                    return NULL;
                }
            }

            if (0 != (iMode & O_WRONLY))
            {
                iffMode |= (FA_WRITE);
            }

            if (0 != (iMode & O_RDWR))
            {
                iffMode |= (FA_READ | FA_WRITE);
            }

            /* O_RDONLY is 0, but FatFS requires FA_READ to be set when reading the file */
            if (0 == iffMode)
            {
                iffMode |= FA_READ;
            }

            if (0 != (iMode & O_APPEND))
            {
                iffMode |= (FA_OPEN_APPEND);
            }

            if (0 != (iMode & O_TRUNC))
            {
                iffMode |= (FA_CREATE_ALWAYS);
            }

//            if (0 != (iMode & O_CREAT))
//            {
//                iffMode |= (FA_CREATE_ALWAYS);
//            }

            /* Open the file */
            pFile = R_FAT_OpenFile((char *) pszFileName, iffMode);

            if (pFile)
            {
                /* Set the stream extension pointer to the file */
                stream_ptr->p_extension = pFile;

                /* Return a pointer to the driver */
                return (st_r_driver_t *) &gFatfsDriver;
            }
        }
    }
    else
    {
        TRACE(("fileLoadDriver: %s Drive not found\r\n", pszFileName));
        return NULL;
    }

    TRACE(("fileLoadDriver: %s File not found\r\n", pszFileName));
    return NULL;
}
/******************************************************************************
 End of function  fileLoadDriver
 ******************************************************************************/

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Function Name: fileOpen
 Description:   Function to open a file
 Arguments:     IN  pStream - Pointer to the IOStream
 Return value:  0 for success otherwise -1
 ******************************************************************************/
static int fileOpen (st_stream_ptr_t stream_ptr)
{
    /* Check that there is a pointer to the FullFAT file object */
    if (stream_ptr->p_extension)
    {
        /* The open was done in the "fileLoadDriver" function */
        return 0;
    }

    return -1;
}
/******************************************************************************
 End of function  fileOpen
 ******************************************************************************/

/******************************************************************************
 Function Name: fileClose
 Description:   Function to close a file
 Arguments:     IN  pStream - Pointer to the IOStream
 Return value:  none
 ******************************************************************************/
static void fileClose (st_stream_ptr_t stream_ptr)
{
    /* Close the file stream */
    FIL * pFile = stream_ptr->p_extension;
    R_FAT_CloseFile(pFile);
}
/******************************************************************************
 End of function  fileClose
 ******************************************************************************/

/******************************************************************************
 Function Name: fileRead
 Description:   Function to read from a file
 Arguments:     IN  pStream - Pointer to the IOStream
 IN  pbyBuffer - Pointer to the destination buffer
 IN  uiCount - The number of bytes to read
 Return value:  The number of bytes read, or -1 on EOF
 ******************************************************************************/
static int fileRead (st_stream_ptr_t stream_ptr, uint8_t *pbyBuffer, uint32_t uiCount)
{
    FIL* pFile = stream_ptr->p_extension;

    /* Check for end of file */
    if (R_FAT_EndOfFile(pFile))
    {
        /* Set the EOF flag */
        // TODO: Set the EOF marker in the FILE* structure?
        return 0;
    }
    else
    {
        /* Clear the EOF flag */
        // TODO: Clear the EOF marker in the FILE* structure?
        /* Return the number of bytes read */
        int bytes_read = R_FAT_ReadFile(pFile, (void *) pbyBuffer, uiCount);
        return bytes_read;
    }
}
/******************************************************************************
 End of function  fileRead
 ******************************************************************************/

/******************************************************************************
 Function Name: fileWrite
 Description:   Function to write to a file
 Arguments:     IN  pStream - Pointer to the IOStream
 IN  pbyBuffer - Pointer to the source buffer
 IN  uiCount - The number of bytes to write
 Return value:  The number of bytes written or -1 on error
 ******************************************************************************/
static int fileWrite (st_stream_ptr_t stream_ptr, uint8_t *pbyBuffer, uint32_t uiCount)
{
    FIL* pFile = stream_ptr->p_extension;
    return R_FAT_WriteFile(pFile, (void *) pbyBuffer, uiCount);

}
/******************************************************************************
 End of function  fileWrite
 ******************************************************************************/

/******************************************************************************
 Function Name: fileControl
 Description:   Function to handle file specific controls
 Arguments:     IN  pStream - Pointer to the file stream
 IN  ctlCode - The custom control code
 IN  pCtlStruct - Pointer to the custorm control structure
 Return value:  0 for success and -1 on error
 ******************************************************************************/
static int fileControl (st_stream_ptr_t stream_ptr, uint32_t ctlCode, void *pCtlStruct)
{
    FIL * pFile = stream_ptr->p_extension;

    switch (ctlCode)
    {
        case CTL_FILE_SEEK :
        {
            if (pCtlStruct)
            {
                PFILESEEK pSeek = (PFILESEEK) pCtlStruct;
                FRESULT fatError = R_FAT_SeekFile(pFile, (unsigned)pSeek->lOffset, pSeek->iBase, &pSeek->lResult);

                if (!fatError)
                {
                    /* Check for end of file */
                    if (R_FAT_EndOfFile(pFile))

                    {
                        /* Set the EOF flag */
                        // TODO: Set the EOF marker in the FILE* structure
                    }
                    else
                    {
                        /* Clear the EOF flag */
                        // TODO: Clear the EOF marker in the FILE* structure
                    }
                    return 0;
                }
                else
                {
                    /* Set the error flag */
                    errno = EBADF;
                    // TODO: Set the error flag in the FILE* structure
                    break;
                }
            }

            break;
        }

        case CTL_FILE_SIZE :
        {
            if (pCtlStruct)
            {
              *((uint32_t *) pCtlStruct) = R_FAT_FileSize(pFile);

                return 0;
            }

            break;
        }

        default :
        {
            return -1;
        }
    }

    return -1;
}
/******************************************************************************
 End of function  fileControl
 ******************************************************************************/

///******************************************************************************
// Function Name: fileCheckInvalidCharacters
// Description:   Function to check for invalid file name characters for Windows
// Arguments:     IN  pszFileName - Pointer to the file name to check
// Return value:  false if the file name has invalid characters
// ******************************************************************************/
//static _Bool fileCheckInvalidCharacters (char *pszFileName)
//{
//    int8_t *pszChar = (int8_t*) pszIllegalFileNameCharacters;
//    size_t stLength = strlen(pszFileName);
//
//    while (*pszChar)
//    {
//        if (memchr(pszFileName, (int) *pszChar, stLength))
//        {
//            return false;
//        }
//        pszChar++;
//    }
//    return true;
//}
///******************************************************************************
// End of function  fileCheckInvalidCharacters
// ******************************************************************************/
//
///******************************************************************************
// Function Name: fileCheckInvalidNames
// Description:   Function to check for reserved windows file names
// Arguments:     IN  pszFilePathAndName - Pointer to the file name
// Return value:  false if file name is reserved
// ******************************************************************************/
//static _Bool fileCheckInvalidNames (char *pszFilePathAndName)
//{
//    char ** pszNameList = (char **) pszIllegalFileNames;
//
//    while (*pszNameList)
//    {
//        if (!strcmp(pszFilePathAndName, *pszNameList))
//        {
//            return false;
//        }
//
//        pszNameList++;
//    }
//
//    return true;
//}
///******************************************************************************
// End of function  fileCheckInvalidNames
// ******************************************************************************/

/******************************************************************************
 Function Name: fileCheckName
 Description:   Function to check that the file name is valid for Windows
 Arguments:     IN  pszFilePathAndName - Pointer to the path and file
 Return value:  true if the file name is valid for Windows
 ******************************************************************************/
// C5177 - Function provided for reference
//static _Bool fileCheckName (char *pszFilePathAndName)
//{
//    char *pszFileName = NULL;
//
//    /* Get the last \ - file name should follow */
//    while (*pszFilePathAndName)
//    {
//        if (*pszFilePathAndName == '\\')
//        {
//            pszFileName = pszFilePathAndName;
//        }
//        pszFilePathAndName++;
//    }
//
//    if ((pszFileName) && (*(pszFileName + 1)))
//    {
//        /* Point at the file name */
//        pszFileName++;
//
//        /* Check for invalid characters */
//        if (fileCheckInvalidCharacters(pszFileName))
//        {
//            /* Check for reserved names */
//            if (fileCheckInvalidNames(pszFileName))
//            {
//                /* Check that it does not end with a space */
//                if (!wildCompare("* ", pszFileName))
//                {
//                    /* Check that it does not end with a . */
//                    if (!wildCompare("*.", pszFileName))
//                    {
//                        return true;
//                    }
//                }
//            }
//        }
//    }
//
//    return false;
//}
///******************************************************************************
// End of function  fileCheckName
// ******************************************************************************/

/*******************************************************************************
 * Function Name: fileGetVersion
 * Description  : Provides build information even if driver fails to open
 *                version information is updated by developer
 * Arguments    : none
 * Return Value : DEVDRV_SUCCESS (never fails)
 ******************************************************************************/
static int_t fileGetVersion (st_stream_ptr_t p_stream, st_ver_info_t *p_ver_info)
{
    (void) p_stream;

    p_ver_info->hld.version.sub.major = gs_hld_info.version.sub.major;
    p_ver_info->hld.version.sub.minor = gs_hld_info.version.sub.minor;
    p_ver_info->hld.build = gs_hld_info.build;
    p_ver_info->hld.p_szdriver_name = gs_hld_info.p_szdriver_name;

    /* Obtain version information from Low layer Driver */
    //R_Blah_GetVersion(p_ver_info);

    return (DEVDRV_SUCCESS);
}
/*******************************************************************************
 End of function fileGetVersion
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
