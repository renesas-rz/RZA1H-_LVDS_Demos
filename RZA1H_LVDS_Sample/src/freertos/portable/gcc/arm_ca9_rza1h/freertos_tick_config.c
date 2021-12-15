/*
 * FreeRTOS Kernel V10.0.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "Task.h"



/* OSTM Driver Header */
#include "r_ostm_drv_api.h"


#include "intc_iobitmask.h"  // TODO remove

#include "intc_iodefine.h"  // TODO remove

#include "control.h"

#define runtimeCLOCK_SCALE_SHIFT	( 9UL )
#define runtimeOVERFLOW_BIT			( 1UL << ( 32UL - runtimeCLOCK_SCALE_SHIFT ) )

/* To make casting to the ISR prototype expected by the Renesas GIC drivers. */
typedef void (*ISR_FUNCTION)( uint32_t );

/* Handle to the OSTM ch0 interface, only valid once the channel has been opened and configured (using CTL_OSTM_CREATE_TIMER) */
static int_t gs_freertos_timer_ch0 = -1;

/* Handle to the OSTM ch1 interface, only valid once the channel has been opened and configured (using CTL_OSTM_CREATE_TIMER) */
static int_t gs_freertos_timer_ch1 = -1;

/*
 * The application must provide a function that configures a peripheral to
 * create the FreeRTOS tick interrupt, then define configSETUP_TICK_INTERRUPT()
 * in FreeRTOSConfig.h to call the function.  This file contains a function
 * that is suitable for use on the Renesas RZ MPU.
 */
void vConfigureTickInterrupt(void)
{
   uint32_t temp;
   st_r_drv_ostm_config_t config = {0}; /* force structure to initialise */

   config.channel     = R_CH0;

   /*  */
   config.frequency   = configPERIPHERAL_CLOCK_HZ / configTICK_RATE_HZ;

   /*  */
   config.callback_fn = (ISR_FUNCTION) FreeRTOS_Tick_Handler;

   /*  */
   config.mode        = OSTM_MODE_INTERVAL;

   /* Open the ostm driver never close it. */
   gs_freertos_timer_ch0 = direct_open("ostm0",0);

   /* Only continue if the drive has successfully been opened */
   configASSERT(((-1) != gs_freertos_timer_ch0));

   /* Only continue if the drive has been successfully created */
   configASSERT(DRV_ERROR != direct_control (gs_freertos_timer_ch0, CTL_OSTM_CREATE_TIMER, &config));

   /* Configure binary point */
   temp = INTC.ICCBPR & ~INTC_ICCBPR_Binarypoint;
   INTC.ICCBPR = temp | (0 << INTC_ICCBPR_Binarypoint_SHIFT);

   /* Only continue if the drive has been successfully created */
   configASSERT(DRV_ERROR != direct_control (gs_freertos_timer_ch0, CTL_OSTM_START_TIMER, NULL));
}
/***********************************************************************************************************************
 End of function vConfigureTickInterrupt
 **********************************************************************************************************************/

/*
 * Crude implementation of a run time counter used to measure how much time
 * each task spends in the Running state.
 */
unsigned long ulGetRunTimeCounterValue( void )
{
   static unsigned long ulLastCounterValue = 0UL, ulOverflows = 0;
   uint32_t ulValueNow;

   configASSERT(((-1) != gs_freertos_timer_ch1));

   /* the st_stream_ptr_t parameter must not be used */
   direct_read(gs_freertos_timer_ch1, (uint8_t *)&ulValueNow, ((sizeof(ulValueNow))/(sizeof(uint8_t))));

   /* Has the value overflowed since it was last read. */
   if( ulValueNow < ulLastCounterValue )
   {
      ulOverflows++;
   }

   ulLastCounterValue = ulValueNow;

   /* There is no prescale on the counter, so simulate in software. */
   ulValueNow >>= runtimeCLOCK_SCALE_SHIFT;
   ulValueNow += ( runtimeOVERFLOW_BIT * ulOverflows );

   return ulValueNow;
}

/***********************************************************************************************************************
 * Function Name: vInitialiseRunTimeStats
 * Description  : Something timer related
 * Arguments    : none
 * Return Value : none
 **********************************************************************************************************************/
void vInitialiseRunTimeStats( void )
{
    st_r_drv_ostm_config_t config = {0}; /* force structure to initialise */

    config.channel     = R_CH1;

    /*  */
    config.mode        = OSTM_MODE_COMPARE;


    /* Note the rest of the configuration can be left in it's default state.
     * The control to set mode OSTM_MODE_INTERVAL or OSTM_MODE_COMPARE is managed by the SC configuration,
     * though this might change as the driver continues to develop.  */

    /* Open the ostm driverm never close it. */
    gs_freertos_timer_ch1 = direct_open("ostm1",0);

    /* Only continue if the drive has successfully been opened */
    configASSERT(((-1) != gs_freertos_timer_ch1));

    /* Only continue if the drive has been successfully created */
    configASSERT(DRV_ERROR != direct_control (gs_freertos_timer_ch1, CTL_OSTM_CREATE_TIMER, &config));

   /* Only continue if the drive has been successfully created */
   configASSERT(DRV_ERROR != direct_control (gs_freertos_timer_ch1, CTL_OSTM_START_TIMER, NULL));
}
/***********************************************************************************************************************
 End of function vInitialiseRunTimeStats
 **********************************************************************************************************************/

