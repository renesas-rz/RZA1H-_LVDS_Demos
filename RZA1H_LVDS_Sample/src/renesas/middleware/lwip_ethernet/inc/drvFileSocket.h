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
 * @headerfile     drvFileSocket.h
 * @brief          API Header for the File Socket Driver
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 04.02.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef DRVFILESOCKET_H_INCLUDED
#define DRVFILESOCKET_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_FileSocket lwIP File Socket
 * @brief A driver to make a lwip socket look like a file stream
 *
 * @anchor R_SW_PKG_93_FILESOCKET_API_SUMMARY
 * @par Summary
 *
 * A driver to make a lwip socket look like a file stream.
 * This is so the console code (which uses the file streams)
 * can be used to make a console over UDP/IP or TCP/IP.
 * 
 * @anchor R_SW_PKG_93_FILESOCKET_API_INSTANCES
 * @par Known Implementations:
 * This driver is used in the RZA1H Software Package.
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
Variable Externs
******************************************************************************/
/** 
 * @var gFileSocketDriver 
 * Table Includes:<BR>
 * "File Socket Driver" - Driver Name <BR>
 * 
 * drvOpen - Opens the File Socket Driver <BR>
 * 
 * drvClose - Closes the File Socket Driver <BR>
 * 
 * drvRead - Reads from the File Socket Driver <BR>
 * 
 * drvWrite - Writes to the Socket Driver <BR>
 * 
 * drvControl - <BR>
 * CTL_SET_LWIP_SOCKET_INDEX: Set the Socket Index <BR>
 * CTL_GET_LWIP_SOCKET_INDEX: Get current Socket Index <BR>
 * CTL_STREAM_REQUIRES_AUTHENTICATION: Not implemented<BR>
 * CTL_STREAM_TCP_CONNECTED: Returns connection status of TCP<BR>
 * CTL_STREAM_SET_CONNECTION_IDLE_CALL_BACK: Set callback function for idle<BR>
 * CTL_STREAM_TCP: Streams TCP<BR>
 * CTL_STREAM_UDP_TCP: Streams UDP<BR>
 * CTL_GET_RX_BUFFER_COUNT: Gets count of currently data <BR>
 * 
 * no_dev_get_version - GetVersion not supported
 */
extern  const st_r_driver_t gFileSocketDriver;

#endif /* DRVFILESOCKET_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
