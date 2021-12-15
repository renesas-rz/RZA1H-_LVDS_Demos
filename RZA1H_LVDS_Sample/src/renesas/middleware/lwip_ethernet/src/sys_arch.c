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
 * File Name    : sys_arch.c
 * Version      : 1.0
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : FreeRTOS
 * H/W Platform : RSK+
 * Description  : System architecture interface for lwIP
 ******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 First Release
 *              : 31.03.2011 1.01 Added ASSERTS to lwIP memory allocation
 *              : DD.MM.YYYY ?.?? Modified API for lwIP V1.4.1
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/

#include "compiler_settings.h"
#include "r_mbox.h"
#include "arch/sys_arch.h"
#include "trace.h"
#include "lwip/sys.h"
#include "r_os_abstraction_api.h"

/*****************************************************************************
 Imported global variables and functions (from other files)
 ******************************************************************************/
#define DEFAULT_MBOX_SIZE_PRV_   (2048)

/* Addresses provide by the linker */

/*****************************************************************************
 Public Functions
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_init
 Description:   Function to initialise any system specific things for the
 operation of lwIP
 Arguments:     none
 Return value:  none
 *****************************************************************************/
void sys_init (void)
{
    ;
}
/*****************************************************************************
 End of function  sys_init
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_mbox_new
 Description:   Function to create a new mail box
 Arguments:     OUT p_mbox - Pointer to the mail box object or NULL on error
 IN  size - The size of the mail box
 Return value:  0 for success or -1 on error
 *****************************************************************************/
err_t sys_mbox_new (sys_mbox_t *p_mbox, int size)
{
    sys_mbox_t mbox = mboxCreate(size);
    if (mbox)
    {
        *p_mbox = mbox;
        return 0;
    }

    return ( -1);
}
/*****************************************************************************
 End of function  sys_mbox_new
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_mbox_post
 Description:   Function to post a message to a mail box
 Arguments:     IN  p_mbox - Pointer to the mail box
 IN  msg - The pointer to post
 Return value:  none
 *****************************************************************************/
void sys_mbox_post (sys_mbox_t *p_mbox, void *p_msg)
{
    mboxPost( *p_mbox, p_msg);
}
/*****************************************************************************
 End of function  sys_mbox_post
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_mbox_trypost
 Description:   Function to try and post a message
 Arguments:     IN  p_mbox - Pointer to the mail box
 IN  msg - The pointer to post
 Return value:  0 for success
 *****************************************************************************/
err_t sys_mbox_trypost (sys_mbox_t *p_mbox, void *p_msg)
{
    return (err_t) mboxTryPost( *p_mbox, p_msg);
}
/*****************************************************************************
 End of function  sys_mbox_trypost
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_arch_mbox_fetch
 Description:   Function to fetch mail from a mail box
 Arguments:     IN  p_mbox - Pointer to the mail box
 IN  msg - Pointer to the destination mail box pointer
 IN  timeout - Time out in mS
 Return value:  The number of miliseconds it took to fetch
 *****************************************************************************/
uint32_t sys_arch_mbox_fetch (sys_mbox_t *p_mbox, void **msg, uint32_t timeout)
{
    return ((uint32_t) mboxFetch( *p_mbox, msg, timeout));
}
/*****************************************************************************
 End of function  sys_arch_mbox_fetch
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_arch_mbox_tryfetch
 Description:   Function to try and fetch mail from a mail box
 Arguments:     IN  p_mbox - Pointer to the mail box object
 IN  msg - Pointer to the message pointer
 Return value:  0 for success or -1 if there is no message available
 *****************************************************************************/
uint32_t sys_arch_mbox_tryfetch (sys_mbox_t *p_mbox, void **msg)
{
    return ((uint32_t) mboxTryFetch( *p_mbox, msg));
}
/*****************************************************************************
 End of function  sys_arch_mbox_tryfetch
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_mbox_free
 Description:   Function to free a mail box object
 Arguments:     IN  p_mbox - Pointer to the mail box to free
 Return value:  none
 *****************************************************************************/
void sys_mbox_free (sys_mbox_t *p_mbox)
{
    mboxDestroy( *p_mbox);
}
/*****************************************************************************
 End of function  sys_mbox_free
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_mbox_valid
 Description:   Function to check for a valid mail box
 Arguments:     IN  p_mbox - Pointer to the mail box to check
 Return value:  1 for a valid mail box 0 for an invalid mail box
 *****************************************************************************/
int sys_mbox_valid (sys_mbox_t *p_mbox)
{
    if ( *p_mbox)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
/*****************************************************************************
 End of function  sys_mbox_valid
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_mbox_set_invalid
 Description:   Function to mark a mail box as invalid
 Arguments:     IN  p_mbox - Pointer to the mail box to mark as invalid
 Return value:  1 for a valid mail box 0 for an invalid mail box
 *****************************************************************************/
void sys_mbox_set_invalid (sys_mbox_t *p_mbox)
{
    if ( *p_mbox)
    {
        *p_mbox = NULL;
    }
}
/*****************************************************************************
 End of function  sys_mbox_set_invalid
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_thread_new
 Description:   Function to start a new task
 Arguments:     IN  name - Pointer to the task name string
 IN  thread - Pointer to the function
 IN  arg - Pointer to the argument passed to the task
 IN  stacksize - The size of stack to use
 IN  prio - The priority of the task
 Return value: The task ID or -1U on error
 *****************************************************************************/
sys_thread_t sys_thread_new (const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    os_task_t *p_task = NULL;


    p_task = R_OS_CreateTask(name, (os_task_code_t) thread, arg, (size_t) stacksize, (int_t) prio);

    if (NULL != p_task)
    {
        return ((sys_thread_t) p_task);
    }

    return ( -1U);
}
/*****************************************************************************
 End of function  sys_thread_new
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_sem_new
 Description:   Function to create a new mutex event
 Arguments:     OUT p_sem - Pointer to the destination semaphore
 IN  count - A particularly aptly named parameter that is set to
 1 when the semaphore is to be created and set.
 The function of this is both well documented and
 obvious from the name.
 Return value:  0 for success
 *****************************************************************************/
err_t sys_sem_new (sys_sem_t *p_sem, uint8_t count)
{
    bool_t ret_value = false;

    /* cast to semaphore_t */
    semaphore_t psem = (semaphore_t) p_sem;

    /* expecting content of pointer to be assigned to to the handle of the semaphore. */
    ret_value = R_OS_CreateSemaphore(psem, 1);

    /* If the semaphore was created */
    if (true == ret_value)
    {
        /* If count is set to 1 then set the semaphore */
        if (0 != count)
        {
            ret_value = R_OS_WaitForSemaphore(psem, R_OS_ABSTRACTION_PRV_EV_WAIT_INFINITE);
        }

        /* pass semaphore to calling function as correct typedef */
        *p_sem = (sys_sem_t) *psem;

        return (0);
    }
    return ( -1);
}
/*****************************************************************************
 End of function  sys_sem_new
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_arch_sem_wait
 Description:   Function to wait for a mutex event
 Arguments:     IN  p_sem - Pointer to the event object
 IN  timeout - Time out in mS, 0 forever
 Return value:  The number of mS elapsed
 *****************************************************************************/
uint32_t sys_arch_sem_wait (sys_sem_t *p_sem, uint32_t timeout)
{
    /***********************************************************************************************************************
     * Function Name: R_OS_WaitForSemaphore
     * Description  : Blocks operation until one of the following occurs:
     *              :      A timeout occurs
     *              :     The associated semaphore has been set
     * Arguments    : semaphore_ptr - Pointer to a associated semaphore
     *              : timeout       - Maximum time to wait for associated event to occur
     * Return Value : The function returns TRUE if the semaphore object was successfully set. Otherwise, FALSE is returned
     **********************************************************************************************************************/
    bool_t ret_value = R_OS_WaitForSemaphore((semaphore_t) p_sem, timeout);
    return ret_value;
}
/*****************************************************************************
 End of function  sys_arch_sem_wait
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_sem_signal
 Description:   Function to release a mutex event
 Arguments:     IN  p_sem - The semaphore to release
 Return value:  none
 *****************************************************************************/
void sys_sem_signal (sys_sem_t *p_sem)
{
    /** OS Abstraction DeleteSemaphore Function
     *  @brief     Release a semaphore, freeing freeing it to be used by another task.
     *  @param[in] semaphore_ptr Pointer to a associated semaphore.
     *  @return    none.
     */
    R_OS_ReleaseSemaphore((semaphore_t) p_sem);
}
/*****************************************************************************
 End of function  sys_sem_signal
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_sem_free
 Description:   Function to free a semaphore object
 Arguments:     IN  p_sem - Pointer to the semaphore object
 Return value:  none
 *****************************************************************************/
void sys_sem_free (sys_sem_t *p_sem)
{
    /** OS Abstraction DeleteSemaphore Function
     *  @brief     Delete a semaphore, freeing any associated resources.
     *  @param[in] semaphore_ptr Pointer to a associated semaphore.
     *  @return    none.
     */
    R_OS_DeleteSemaphore((semaphore_t) p_sem);
}
/*****************************************************************************
 End of function  sys_sem_free
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_sem_valid
 Description:   Function to check for a semaphore object
 Arguments:     IN  p_sem - Pointer to the semaphore object
 Return value:  1 for a valid semaphore 0 for an invalid semaphore
 *****************************************************************************/
int sys_sem_valid (sys_sem_t *p_sem)
{
    if (NULL != (*p_sem))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
/*****************************************************************************
 End of function  sys_sem_valid
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_sem_set_invalid
 Description:   Function to mark semaphore object as invalis
 Arguments:     IN  sem - Pointer to the semaphore object
 Return value:  none
 *****************************************************************************/
void sys_sem_set_invalid (sys_sem_t *p_sem)
{
    /* Do not set invalid here. This is done by sys_sem_free / R_OS_DeleteSempahore */
    /** OS Abstraction DeleteSemaphore Function
         *  @brief     Delete a semaphore, freeing any associated resources.
         *  @param[in] semaphore_ptr Pointer to a associated semaphore.
         *  @return    none.
         */
        R_OS_DeleteSemaphore((semaphore_t) p_sem);
}
/*****************************************************************************
 End of function  sys_sem_set_invalid
 *****************************************************************************/

/*****************************************************************************
 Function Name: sys_arch_protect
 Description:   The interrupt mask protection system used by lwIP
 Arguments:     none
 Return value:  The current value of the interrupt mask
 *****************************************************************************/
sys_prot_t sys_arch_protect (void)
{
    return ((int) R_OS_SysLock(NULL));
}
/*****************************************************************************
 End of function  sys_arch_protect
 ******************************************************************************/

/*****************************************************************************
 Function Name: sys_arch_unprotect
 Description:   Function to restore the interrup mask
 Arguments:     IN  pval - The interrup mask setting
 Return value:  none
 *****************************************************************************/
void sys_arch_unprotect (sys_prot_t pval)
{
    R_OS_SysUnlock(NULL, pval);
}
/*****************************************************************************
 End of function  sys_arch_unprotect
 ******************************************************************************/

/*****************************************************************************
 Function Name: lwip_malloc
 Description:   Function to allocate memory
 Arguments:     IN  stLength - The length of memory to allocate
 Return value:  Pointer to the memory or NULL on error
 *****************************************************************************/
void *lwip_malloc (size_t stLength)
{
    void *pvResult;

    pvResult = R_OS_AllocMem(stLength, R_REGION_UNCACHED_RAM);
//    printf("lwip_malloc [%d] bytes @ [0x%08x]\r\n", stLength, pvResult);
//    TRACE(("LWAloc 0x%08x : %d \r\n",pvResult, stLength));
    return (pvResult);
}
/*****************************************************************************
 End of function  lwip_malloc
 ******************************************************************************/

/*****************************************************************************
 Function Name: lwip_realloc
 Description:   Function to re-allocate memory
 Arguments:     IN  pvReAlloc - Pointer to the memory to re-allocate
 IN  stLength - The new length
 Return value:  Pointer to the memory or NULL on error
 *****************************************************************************/
void *lwip_realloc (void *pvReAlloc, size_t stLength)
{
    lwip_free(pvReAlloc);
    return (lwip_malloc(stLength));
}
/*****************************************************************************
 End of function  lwip_realloc
 ******************************************************************************/

/*****************************************************************************
 Function Name: lwip_free
 Description:   Function to free memory
 Arguments:     IN  pvFree - Pointer to the memory to free
 Return value:  none
 *****************************************************************************/
void lwip_free (void *pvFree)
{
//    TRACE(("LWFree 0x%08x \r\n", pvFree));
    R_OS_FreeMem(pvFree);
}
/*****************************************************************************
 End of function  lwip_free
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
