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
 * @headerfile     fileDriver.h
 * @brief          The driver for a file on a disk.
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 05.08.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef FILEDRIVER_H_INCLUDED
#define FILEDRIVER_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_USB_FILE USB File
 * @brief USB function CDC hardware interface functions
 * 
 * @anchor R_SW_PKG_93_USB_FILE_SUMMARY
 * @par Summary
 * 
 * This module contain the interface to the USB File functionality
 * allowing the USB module to interact with files using POSIX API. 
 * Read, Write, Open, Close and Control. 
 * 
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "r_devlink_wrapper.h"

/******************************************************************************
Function Prototypes
******************************************************************************/

/******************************************************************************
 Macro definitions
 ******************************************************************************/

/** Unique ID. Assigned by requirements */
//#define STDIO_FILE_DRIVER_RZ_HLD_UID                (56)


#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 Macro definitions
 ******************************************************************************/

#define STDIO_FILE_DRIVER_RZ_HLD_DRV_NAME ("HLD FILE")

/** Major Version Number of API.
 * Updated by product owner */
#define STDIO_FILE_DRIVER_RZ_HLD_VERSION_MAJOR      (1)

/** Minor Version Number of API.
 * Updated by developer */
#define STDIO_FILE_DRIVER_RZ_HLD_VERSION_MINOR      (0)

/** Build Number of API.
 * Generated during customer release */
#define STDIO_FILE_DRIVER_RZ_HLD_BUILD_NUM          (1000)

/** Unique ID. Assigned by requirements */
//#define STDIO_FILE_DRIVER_RZ_HLD_UID                (56)


/**
 * @brief         Function to load a file driver for a file
 * 
 * @param[in]     pStream:     Pointer to the file stream
 * @param[in]     pszFileName: Pointer to a file name
 * @param[in]     iMode:       The file mode
 * 
 * @retval        file_ptr: Pointer to the file driver
 * @retval        NULL:     If driver not found
*/
extern  st_r_driver_t*
fileLoadDriver (st_stream_ptr_t st_stream_ptr_t, char * pszFileName, int iMode);


/** 
 * @var gFatfsDriver 
 * Table Includes:<BR>
 * "FatFS library file driver IO wrapper" - Driver Name <BR>
 * 
 * fileOpen - Opens the file Driver <BR>
 * 
 * fileClose - Closes the file Driver <BR>
 * 
 * fileRead - Reads from the file Driver <BR>
 * 
 * fileWrite - Writes to the file Driver <BR>
 * 
 * fileControl - <BR>
 * CTL_FILE_SEEK: Returns position of a file <BR>
 * CTL_FILE_SIZE: Returns the size of a file <BR>
 * 
 * fileGetVersion - GetVersion of file driver
 */
extern  const st_r_driver_t gFatfsDriver;

#ifdef __cplusplus
}
#endif

#endif /* FILEDRIVER_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
