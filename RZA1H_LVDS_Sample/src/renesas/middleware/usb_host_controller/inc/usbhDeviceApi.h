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
 * @headerfile     usbhDeviceApi.h
 * @brief          The API for device drivers using the USB host controller
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 05.08.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef USBHDEVICEAPI_H_INCLUDED
#define USBHDEVICEAPI_H_INCLUDED

/**************************************************************************//**
 * @ingroup R_SW_PKG_93_USB_HOST_API 
 * @{
 *****************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "r_typedefs.h"
#include "ddusbh.h"

/******************************************************************************
Macro definitions
******************************************************************************/

#define REQ_IDLE_TIME_OUT_INFINITE  -1UL

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Function to get a device pointer
 * 
 * @param[in]     pszLinkName: Pointer to the device link name
 * 
 * @retval        ptr_device:  Pointer to the device 
 * @retval        NULL:        For invalid parameter
*/
extern PUSBDI usbhGetDevice(int8_t * pszLinkName);

/**
 * @brief         Function to open a device endpoint
 * 
 * @param[in]     pDevice:    The device to get
 * @param[in]     byEndpoint: The endpoint index to open
 * 
 * 
 * @retval        ptr_end_point: Pointer to the endpoint
 * @retval        NULL:          If not found
*/
extern  PUSBEI usbhGetEndpoint(PUSBDI pDevice, uint8_t byEndpoint);

/**
 * @brief         Function to start data transfer on an endpoint
 * 
 * @note          Requires mutually exclusive access to protect list
 * 
 * @param[in]     pDevice:       Pointer to the device
 * @param[in]     pRequest:      Pointer to the transfer request struct
 * @param[in]     pEndpoint:     Pointer to the endpoint to use
 * @param[in]     pMemory:       Pointer to the memory to transfer
 * @param[in]     stLength:      The length of memory to transfer
 * @param[in]     dwIdleTimeOut: The request idle time out in mS
 *
 * 
 * @retval        true: if the request was added 
*/
extern  _Bool usbhStartTransfer(PUSBDI   pDevice,
                                PUSBTR   pRequest,
                                PUSBEI   pEndpoint,
                                uint8_t *pMemory,
                                size_t   stLength,
                                uint32_t dwIdleTimeOut);

/**
 * @brief         Function to start an isochronous transfer with a packet size
 *                variation schedule
 * 
 * @note          Requires mutually exclusive access to protect lists
 * 
 * @param[in]     pDevice:         Pointer to the device
 * @param[in]     pRequest:        Pointer to the transfer request struct
 * @param[in]     pIsocPacketSize: Pointer to the isoc packet size variation
 *                                 data
 * @param[in]     pEndpoint:       Pointer to the endpoint struct
 * @param[in]     pMemory:         Pointer to the memory to transfer
 * @param[in]     stLength:        The length of memory to transfer
 * @param[in]     dwIdleTimeOut:   The request idle time out in mS
 * 
 * @retval        true: If the request was added
*/
extern  _Bool usbhIsocTransfer(PUSBDI   pDevice,
                               PUSBTR   pRequest,
                               PUSBIV   pIsocPacketSize,
                               PUSBEI   pEndpoint,
                               uint8_t  *pMemory,
                               size_t   stLength,
                               uint32_t dwIdleTimeOut);

/**
 * @brief         Function to cancel a transfer
 * 
 * @note          Requires mutually exclusive access to protect list         
 * 
 * @param[in]     pRequest: Pointer to the request to cancel
 * 
 * @retval        true: If the request was cancelled
*/
extern  _Bool usbhCancelTransfer(PUSBTR pRequest);

/**
 * @brief         Function to cancel all transfer requests for a device
 * 
 * @param[in]     pDevice - Pointer to the device to cancel all transfer
 *                          requests.
 * 
 * @retval        num_requests: The number of requests cancelled
*/
extern  int usbhCancelAllTransferReqests(PUSBDI pDevice);

/**
 * @brief         Function to send a device request
 * 
 * @param[in]     pRequest:       Pointer to the request
 * @param[in]     pDevice:        Pointer to the device
 * @param[in]     bmRequestType:  The request type
 * @param[in]     bRequest:       The request
 * @param[in]     wValue:         The Value
 * @param[in]     wIndex:         The Index
 * @param[in]     wLength:        The length of the data
 * @param[in/out] pbyData:        Pointer to the data
 * 
 * @retval        0:        For success 
 * @retval        ER_CODE:  Error code
*/
extern  REQERR usbhDeviceRequest(PUSBTR    pRequest,
                                 PUSBDI    pDevice,
                                 uint8_t   bmRequestType,
                                 uint8_t   bRequest,
                                 uint16_t  wValue,
                                 uint16_t  wIndex,
                                 uint16_t  wLength,
                                 uint8_t  *pbyData);

/**
 * @brief         Function to cancel all control requests for a device
 * 
 * @param[in]     pDevice: Pointer to the device to cancel all transfer
 *                         requests
 * 
 * @retval        num_reqs: The number of requests cancelled
*/
extern  int usbhCancelAllControlRequests(PUSBDI pDevice);

/**
 * @brief         Function to find out if the request is in progress
 * 
 * @param[in]     pRequest - Pointer to the request
 * 
 * @retval        true: If the transfer is in progress
*/
extern  _Bool usbhTransferInProgress(PUSBTR pRequest);

/**
 * @brief         Function to send a device request
 * @param[in]     pRequest:      Pointer to the request
 * @param[in]     pDevice:       Pointer to the device
 * @param[in]     bmRequestType: The request type
 * @param[in]     bRequest:      The request
 * @param[in]     wValue:        The Value
 * @param[in]     wIndex:        The Index
 * @param[in]     wLength:       The length of the data
 * @param[in/out] pbyData:       Pointer to the data
 * 
 * @retval        0:        For success 
 * @retval        ER_CODE:  Error code
 */
extern REQERR _usbhDeviceRequest (PUSBTR pRequest, PUSBDI pDevice, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue,
        uint16_t wIndex, uint16_t wLength, uint8_t *pbyData);

#ifdef __cplusplus
}
#endif

#endif /* USBHDEVICEAPI_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
