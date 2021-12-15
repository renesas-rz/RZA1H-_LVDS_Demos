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
 * File Name    : mbox.c
 * Version      : 1.0
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : FreeRTOS
 * H/W Platform : RSK+
 * Description  : Mail box interface for lwIP
 ******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 First Release
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/

#include <stdlib.h>
#include "compiler_settings.h"
#include "r_task_priority.h"

#include "FreeRTOS.h"
#include "FreeRTOSconfig.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

#include "r_mbox.h"
#include "trace.h"

/*****************************************************************************
 Macro definitions
 ******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/*****************************************************************************
 Function Prototypes
 ******************************************************************************/

/*****************************************************************************
 External Variables
 ******************************************************************************/

/*****************************************************************************
 Public Functions
 ******************************************************************************/

/*****************************************************************************
 Function Name: mboxCreate
 Description:   Function to create a mail box object
 Arguments:     IN  iSize - The size of the mail box
 Return value:  Pointer to the mail box object or NULL on error
 *****************************************************************************/
PMBOX mboxCreate(int iSize)
{
    xQueueHandle pMBox;
    if (!iSize)
    {
        iSize = MBOX_SIZE;
    }

    pMBox = xQueueCreate((uint32_t) iSize, sizeof(void *));
    return (PMBOX) pMBox;
}
/*****************************************************************************
 End of function  mboxCreate
 ******************************************************************************/

/*****************************************************************************
 Function Name: mboxDestroy
 Description:   Function to destroy a mail box object
 Arguments:     IN  pMBox - Pointer to the mail box object
 Return value:  none
 *****************************************************************************/
void mboxDestroy(PMBOX pMBox)
{
    while (uxQueueMessagesWaiting((xQueueHandle)pMBox))
    {
        taskYIELD();
    }
    vQueueDelete((xQueueHandle)pMBox);
}
/*****************************************************************************
 End of function  mboxDestroy
 ******************************************************************************/

/*****************************************************************************
 Function Name: mboxPost
 Description:   Function to post a message
 Arguments:     IN  pMBox - Pointer to the mail box object
 IN  pvMessage - Pointer to the message
 Return value:  none
 *****************************************************************************/
void mboxPost(PMBOX pMBox, void *pvMessage)
{
    while (pdTRUE != xQueueSend((xQueueHandle)pMBox, &pvMessage, 0UL))
    {
        taskYIELD();
    }
}
/*****************************************************************************
 End of function  mboxPost
 ******************************************************************************/

/*****************************************************************************
 Function Name: mboxTryPost
 Description:   Function to try and post a message
 Arguments:     IN  pMBox - Pointer to the mail box object
 IN  pvMessage - Pointer to the message
 Return value:  0 if the message is posted -1 if the mail box is full
 *****************************************************************************/
int mboxTryPost(PMBOX pMBox, void *pvMessage)
{
    if (errQUEUE_FULL == xQueueSend((xQueueHandle)pMBox, &pvMessage, 0UL))
    {
        TRACE(("mboxTryPost: **Error: mail box full\r\n"));
        return -1;
    } else
    {
        return 0;
    }
}
/*****************************************************************************
 End of function  mboxTryPost
 ******************************************************************************/

/*****************************************************************************
 Function Name: mboxFetch
 Description:   Function to fetch mail from a mail box
 Arguments:     IN  pMBox - Pointer to the mail box object
 IN  ppvMessage - Pointer to the message pointer
 IN  uiTimeOut - The maximum time to wait for the message
 Return value:  The number of milliseconds it took to fetch or -1 on timeout
 *****************************************************************************/
int32_t mboxFetch(PMBOX pMBox, void **ppvMessage, uint32_t uiTimeOut)
{
    void    *pvDummy;
    portTickType ulStartTime, ulEndTime;

    /* Get the start time */
    ulStartTime = xTaskGetTickCount();
    if (!ppvMessage)
    {
        ppvMessage = &pvDummy;
    }

    /* Check for a time-out */
    if (uiTimeOut)
    {
        /* Receive with time-out */
        if (pdTRUE == xQueueReceive((xQueueHandle)pMBox,
                                    &(*ppvMessage),
                                    uiTimeOut / portTICK_RATE_MS))
        {
            /* Return the elapsed time in mS */
            ulEndTime = xTaskGetTickCount();
            return (int32_t) ((ulEndTime - ulStartTime) * portTICK_RATE_MS);
        }
        else
        {
            *ppvMessage = NULL;
            return -1;
        }
    }
    else
    {
        /* Receive blocking */
        while (pdTRUE != xQueueReceive((xQueueHandle)pMBox,
                                       &(*ppvMessage),
                                       portMAX_DELAY))
        {
            taskYIELD();
        }

        /* Return the elapsed time in mS */
        ulEndTime = xTaskGetTickCount();
        return (int32_t) ((ulEndTime - ulStartTime) * portTICK_RATE_MS);
    }
}
/*****************************************************************************
 End of function  mboxFetch
 ******************************************************************************/

/*****************************************************************************
 Function Name: mboxTryFetch
 Description:   Function to try and fetch mail from a mail box
 Arguments:     IN  pMBox - Pointer to the mail box object
 IN  ppvMessage - Pointer to the message pointer
 Return value:  0 for success or -1 if there is no message available
 *****************************************************************************/
int32_t mboxTryFetch(PMBOX pMBox, void **ppvMessage)
{
    if (errQUEUE_EMPTY != xQueueReceive((xQueueHandle)pMBox,
                                        &(*ppvMessage),
                                        0UL))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
/*****************************************************************************
 End of function  mboxTryFetch
 ******************************************************************************/

/*****************************************************************************
 Private Functions
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
