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
* Copyright (C) 2011 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************
* File Name    : semaphore.c
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : FreeRTOS
* H/W Platform : RSK+
* Description  : semaphore required for lwIP
*******************************************************************************
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

#include "semaphore.h"
#include "r_timer.h"
#include "r_event.h"
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
External Variables
******************************************************************************/

extern int_t g_itimer;

/*****************************************************************************
Public Functions
******************************************************************************/

/*****************************************************************************
Function Name: semCreate
Description:   Function to create a semaphore object
Arguments:     none
Return value:  Pointer to the semaphore object or NULL on error
*****************************************************************************/
PSEMT semCreate(void)
{
    PEVENT  pEvent = NULL;
    eventCreate(&pEvent, 1);
    return (PSEMT)pEvent;
}
/*****************************************************************************
End of function  semCreate
******************************************************************************/

/*****************************************************************************
Function Name: semDestroy
Description:   Function to destroy a semaphore object
Arguments:     IN  pSem - Pointer to the semaphore object
Return value:  none
*****************************************************************************/
void semDestroy(PSEMT pSem)
{
    eventDestroy((PPEVENT)&pSem, 1);
}
/*****************************************************************************
End of function  semDestroy
******************************************************************************/

/*****************************************************************************
Function Name: semWait
Description:   Function to wait for a semaphore
Arguments:     IN  pSem - Pointer to the semaphore
               IN  uiTimeOut - The time out in mS
Return value:  The number of mS waited before the semaphore was set
*****************************************************************************/
uint32_t semWait(PSEMT pSem, uint32_t uiTimeOut)
{
    PEVENT      pEvent = (PEVENT)pSem;
    uint32_t    uiResult = 0;
    TMSTMP      startTime;

    /* Get a time stamp now */
    if (control(g_itimer, CTL_GET_TIME_STAMP, &startTime))
    {
        TRACE(("semWait: **Error: Failed to get time stamp\r\n"));

        /* If the timer is not working block until set and return 0 */
        eventWait(&pEvent, 1, true);
    }
    else
    {
        TMSTMP  endTime;

        /* If there is a time-out */
        if (uiTimeOut)
        {
            PEVENT  ppEventList[2];
            ppEventList[0] = pEvent;

            /* Start a timer for the desired time-out */
           // ppEventList[1] = timerStartEvent(SINGLE_SHOT_TIMER,        /* TODO: this function does not exist */
                                           //  (uint32_t)uiTimeOut);

            /* Wait for either the semaphore signal or the time-out */
            if (eventWait(ppEventList, 2, true))
            {
                /* The timer expired - stop it to free the resource */
                //timerStopEvent(ppEventList[1]);                        /* TODO: this function does not exist */

                /* Return the time-out value (SYS_ARCH_TIMEOUT) */
                return -1U;
            }

            /* The event was set - stop it to free the resource */
          //  timerStopEvent(ppEventList[1]);
        }
        else
        {
            /* Wait forever only on the semaphore signal */
            eventWait(&pEvent, 1, true);
        }

        /* Get the time now */
        control(g_itimer, CTL_GET_TIME_STAMP, &endTime);

        /* Check to see if the count has wrapped from FFFFFFFF to 0 */
        if (endTime.ulMilisecond >= startTime.ulMilisecond)
        {
            /* Normal */
            uiResult = (uint32_t)(endTime.ulMilisecond - startTime.ulMilisecond);
        }
        else
        {
            /* ulMilisecond has wrapped about 0 */
            //TODO: Overflow value is wrong
            uiResult = (uint32_t)((0xFFFFFFFFUL - startTime.ulMilisecond)
                     + endTime.ulMilisecond);
        }
    }
    return uiResult;
}
/*****************************************************************************
End of function  semWait
******************************************************************************/

/*****************************************************************************
Function Name: semSet
Description:   Function to set a semaphore and wake a waiting task
Arguments:     IN  pSem - Pointer to the semaphore
Return value:  none
*****************************************************************************/
void semSet(PSEMT pSem)
{
    PEVENT  pEvent = (PEVENT)pSem;

    /* Signal all tasks waiting on the event */
    eventSet(pEvent);
}
/*****************************************************************************
End of function  semSet
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
