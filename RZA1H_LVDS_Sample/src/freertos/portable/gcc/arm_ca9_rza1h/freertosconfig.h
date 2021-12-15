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

/* Standard includes. */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "application_cfg.h"
#include "r_task_priority.h"
#include "r_os_abstraction_api.h"

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

/*
 * The FreeRTOS Cortex-A port implements a full interrupt nesting model.
 *
 * Interrupts that are assigned a priority at or below
 * configMAX_API_CALL_INTERRUPT_PRIORITY (which counter-intuitively in the ARM
 * generic interrupt controller [GIC] means a priority that has a numerical
 * value above configMAX_API_CALL_INTERRUPT_PRIORITY) can call FreeRTOS safe API
 * functions and will nest.
 *
 * Interrupts that are assigned a priority above
 * configMAX_API_CALL_INTERRUPT_PRIORITY (which in the GIC means a numerical
 * value below configMAX_API_CALL_INTERRUPT_PRIORITY) cannot call any FreeRTOS
 * API functions, will nest, and will not be masked by FreeRTOS critical
 * sections (although it is necessary for interrupts to be globally disabled
 * extremely briefly as the interrupt mask is updated in the GIC).
 *
 * FreeRTOS functions that can be called from an interrupt are those that end in
 * "FromISR".  FreeRTOS maintains a separate interrupt safe API to enable
 * interrupt entry to be shorter, faster, simpler and smaller.
 *
 * The Renesas RZ implements 32 unique interrupt priorities.  For the purpose of
 * setting configMAX_API_CALL_INTERRUPT_PRIORITY 31 represents the lowest
 * priority.
 */
#define configMAX_API_CALL_INTERRUPT_PRIORITY	(R_HARDWARE_API_ISR_PRIORITY)

#define configUSE_PORT_OPTIMISED_TASK_SELECTION	1
#define configAPPLICATION_ALLOCATED_HEAP        1
#define configUSE_TICKLESS_IDLE					0

#define configUSE_PREEMPTION					1
#define configUSE_IDLE_HOOK						0
#define configUSE_TICK_HOOK						0
#define configCPU_CLOCK_HZ                      100000000UL
#define configPERIPHERAL_CLOCK_HZ               ( 33333333UL )
#define configPERIPHERAL_CLOCK0_HZ              ( 33333333UL )
#define configPERIPHERAL_CLOCK1_HZ              ( 66666666UL )
#define configTICK_RATE_HZ                      ( ( portTickType ) 1000 )
#define configMINIMAL_STACK_SIZE				( ( unsigned short ) 160 )
#define configSMALL_STACK_SIZE                  ( ( unsigned short ) 2048 )
#define configDEFAULT_STACK_SIZE                ( ( unsigned short ) 4096 )
#define configTOTAL_HEAP_SIZE					( ( size_t ) ( R_TOTAL_MEMORY_AVAILABLE) )
#define configMAX_PRIORITIES                    ( R_TASK_NUMBER_OF_PRIORITIES )
#define configMAX_TASK_NAME_LEN                 ( 24 )
#define configUSE_TRACE_FACILITY				1
#define configUSE_16_BIT_TICKS					0
#define configIDLE_SHOULD_YIELD					1
/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                   0
#define configUSE_MUTEXES						1
#define configGENERATE_RUN_TIME_STATS           1
#define configCHECK_FOR_STACK_OVERFLOW			2
#define configUSE_RECURSIVE_MUTEXES				1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_MALLOC_FAILED_HOOK			0
#define configUSE_APPLICATION_TASK_TAG			0
#define configUSE_QUEUE_SETS			        1
#define configUSE_COUNTING_SEMAPHORES			1
#define configMEMORY_TYPE_FOR_ALLOCATOR         (0)
#define configMAX_CO_ROUTINE_PRIORITIES         ( 2 )

/* Software timer definitions. */
#define configUSE_TIMERS				1
#define configTIMER_TASK_PRIORITY		( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH		5
#define configTIMER_TASK_STACK_DEPTH	( configMINIMAL_STACK_SIZE * 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	1
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_eTaskGetState                   0
#define INCLUDE_pcTaskGetTaskName               0
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle       0
#define INCLUDE_uxTaskGetStackHighWaterMark     0
#define INCLUDE_xTaskGetIdleTaskHandle          0

/* This demo makes use of one or more example stats formatting functions.  These
format the raw data provided by the uxTaskGetSystemState() function in to human
readable ASCII form.  See the notes in the implementation of vTaskList() within
FreeRTOS/Source/tasks.c for limitations. */
#define configUSE_STATS_FORMATTING_FUNCTIONS	1

/* Prevent C code being included in assembly files when the IAR compiler is
used. */
#ifndef __IASMARM__
	/* Run time stats gathering definitions. */
	unsigned long ulGetRunTimeCounterValue( void );
	void vInitialiseRunTimeStats( void );

	#define configGENERATE_RUN_TIME_STATS	1
	#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() vInitialiseRunTimeStats()
	#define portGET_RUN_TIME_COUNTER_VALUE() ulGetRunTimeCounterValue()

	/* The size of the global output buffer that is available for use when there
	are multiple command interpreters running at once (for example, one on a UART
	and one on TCP/IP).  This is done to prevent an output buffer being defined by
	each implementation - which would waste RAM.  In this case, there is only one
	command interpreter running. */
	#define configCOMMAND_INT_MAX_OUTPUT_SIZE 2096

	/* Normal assert() semantics without relying on the provision of an assert.h
	header file. */
	#define configASSERT( x ) if( (x) == 0 ) { R_OS_AssertCalled( __FILE__, __LINE__ ); }



	/****** Hardware specific settings. *******************************************/

	/*
	 * The application must provide a function that configures a peripheral to
	 * create the FreeRTOS tick interrupt, then define configSETUP_TICK_INTERRUPT()
	 * in FreeRTOSConfig.h to call the function.  This file contains a function
	 * that is suitable for use on the Renesas RZ MPU.  FreeRTOS_Tick_Handler() must
	 * be installed as the peripheral's interrupt handler.
	 */
	void vConfigureTickInterrupt( void );
	#define configSETUP_TICK_INTERRUPT() vConfigureTickInterrupt()
#endif /* __IASMARM__ */

/* The following constants describe the hardware, and are correct for the
Renesas RZ MPU. */
#define configINTERRUPT_CONTROLLER_BASE_ADDRESS	0xE8201000
#define configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET 0x1000
#define configUNIQUE_INTERRUPT_PRIORITIES		32

/* Map the FreeRTOS IRQ and SVC/SWI handlers to the names used in the C startup
code (which is where the vector table is defined). */
#define FreeRTOS_IRQ_Handler IRQ_Handler
#define FreeRTOS_SWI_Handler SWI_Handler

/* Free all memory allocated by the task when it is deleted */
/*
extern size_t exFreeByTaskID(void *pvTaskHandle);
#define portCLEAN_UP_TCB( pxTCB ) exFreeByTaskID(pxTCB)
*/

#if 0 // todo RC
/* Use a FreeRTOS trace macro to estimate the CPU usage */
extern void sriMeasureCpu(void);
#define traceTASK_SWITCHED_IN   sriMeasureCpu
#endif

#endif /* FREERTOS_CONFIG_H */

