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
 * File Name    : usbhBulk.c
 * Version      : 1.01
 * Device(s)    : Renesas
 * Tool-Chain   : GNUARM-NONE-EABI v14.02
 * OS           : None
 * H/W Platform : RSK+
 * Description  : USB host functions to handle bulk transfers
 *******************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 01.08.2009 1.00 First Release
 *              : 14.12.2010 1.01 Updated Trace
 ******************************************************************************/

/******************************************************************************
 WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
 OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
 SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
 ******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
 ******************************************************************************/

#include "usbhDriverInternal.h"
#include "hwDmaIf.h"

/******************************************************************************
 Function Macros
 ******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
 Function Prototypes
 ******************************************************************************/

static _Bool usbhStartBulkInTransfer (PUSBTR pRequest, int iPipeNumber);
static void usbhCompleteDmaIn (void *pvRequest);
static void usbhCancelBulkInDma (PUSBTR pRequest);
static _Bool usbhStartBulkOutTransfer (PUSBTR pRequest, int iPipeNumber);
static void usbhContinueBulkOutFifo (PUSBTR pRequest, int iPipeNumber);
static void usbhCompleteDmaOut (void *pvRequest);
static void usbhCancelBulkOutDma (PUSBTR pRequest);

/******************************************************************************
 Exported global variables and functions (to be accessed by other files)
 ******************************************************************************/
extern bool_t dma_available;

/******************************************************************************
 Function Name: usbhStartBulkTransfer
 Description:   Function to start a Bulk transfer
 Arguments:     IN  pRequest - Pointer to the transfer request
 Return value:  true if the transfer was started
 ******************************************************************************/
_Bool usbhStartBulkTransfer (PUSBTR pRequest)
{
    if ((pRequest) && (pRequest->bfInProgress == false))
    {
        /* Allocate a pipe for this transfer */
        int iPipeNumber = usbhAllocPipeNumber(pRequest);

        /* If there is a pipe available */
        if (iPipeNumber != -1)
        {
            PUSBEI pEndpoint = pRequest->pEndpoint;

            /* Set the pipe number */
            pRequest->pInternal = (void*) iPipeNumber;

            /* Initialise the FIFO used count - for idle time-out */
            pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount = 0;

            /* Set the pipe to NAK */
            R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);

            /* Set the PID */
            R_USBH_SetPipePID(pRequest->pUSB, iPipeNumber, pEndpoint->dataPID);

            /* Check the transfer direction */
            if (pEndpoint->transferDirection == USBH_IN)
            {
                /* Start the bulk in transfer */
                return usbhBulkIn(pRequest, iPipeNumber);
            }
            else
            {
                /* Start the bulk out transfer */
                _Bool bfResult = usbhBulkOut(pRequest, iPipeNumber);

                /* Check to see if the transfer was started */
                if (!bfResult)
                {
                    /* Free the pipe */
                    usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
                }
                return bfResult;
            }
        } TRACE(("usbhStartBulkTransfer: Failed to start transfer\r\n"));
    }
    return false;
}
/******************************************************************************
 End of function  usbhStartBulkTransfer
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhBulkIn
 Description:   Function to handle a bulk in transfer
 Arguments:     IN  pRequest - Pointer to the transfer request
 IN  iPipeNumber - The pipe number to use
 Return value:  true if the transfer is in progress
 ******************************************************************************/
_Bool usbhBulkIn (PUSBTR pRequest, int iPipeNumber)
{
    if (pRequest)
    {
        /* Disable the pipe */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);

        /* If it is not in progress then start it */
        if (pRequest->bfInProgress == false)
        {
            return usbhStartBulkInTransfer(pRequest, iPipeNumber);
        }
        else
        {
            return usbhContinueInFifo(pRequest, iPipeNumber);
        }
    }
    return false;
}
/******************************************************************************
 End of function  usbhBulkIn
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhBulkOut
 Description:   Function to handle a bulk out transfer
 Arguments:     IN  pRequest - Pointer to the transfer request
 IN  iPipeNumber - The pipe number to use
 Return value:  true if the transfer is in progress
 ******************************************************************************/
_Bool usbhBulkOut (PUSBTR pRequest, int iPipeNumber)
{
    if (pRequest)
    {
        /* Disable the pipe */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);

        /* If it is not in progress then start it */
        if (pRequest->bfInProgress == false)
        {
            if (usbhStartBulkOutTransfer(pRequest, iPipeNumber))
            {
                return true;
            }
        }

        /* Continue or complete a FIFO transfer and complete a DMA transfer */
        else
        {
            usbhContinueBulkOutFifo(pRequest, iPipeNumber);
            return true;
        }
    }
    return false;
}
/******************************************************************************
 End of function  usbhBulkOut
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhBulkPipeIdle
 Description:   Function to return true if pipe has been idle since the last
 call
 Arguments:     IN  pRequest - Pointer to the transfer request
 IN iPipeNumber - The number of the pipe to check
 Return value:  true if the pipe has been idle
 ******************************************************************************/
_Bool usbhBulkPipeIdle (PUSBTR pRequest, int iPipeNumber)
{
    if (pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount)
    {
        pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount = 0;
        return false;
    }
    else
    {
        uint32_t ulDmaCount;
        if (pRequest->pEndpoint->transferDirection == USBH_OUT)
        {
            ulDmaCount = dmaGetUsbOutCh0Count();
        }
        else
        {
            ulDmaCount = dmaGetUsbInCh1Count();
        }

        if (ulDmaCount == pRequest->pUsbHc->pPipeTrack[iPipeNumber].ulDmaTransCnt)
        {
            return true;
        }
        else
        {
            pRequest->pUsbHc->pPipeTrack[iPipeNumber].ulDmaTransCnt = ulDmaCount;
            return false;
        }
    }

    return true;
}
/******************************************************************************
 End of function  usbhBulkPipeIdle
 ******************************************************************************/

/******************************************************************************
 Private global variables and functions
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhStartBulkInTransfer
 Description:   Function to start a bulk in transfer
 Arguments:     IN  pRequest - Pointer to the transfer request
 IN  iPipeNumber - The pipe number to use
 Return value:  true
 ******************************************************************************/
static _Bool usbhStartBulkInTransfer (PUSBTR pRequest, int iPipeNumber)
{
    PUSB pUSB = pRequest->pUSB;
    int32_t dma_ercd = 0;

    /* If the transfer is greater than four packets. The problem here is that
     if there is further data to be received after the the current transfer
     once the DMA has finished the next two packets will automatically be
     accepted into the pipe FIFO. When the DMA completes the pipe is freed
     and the data in the FIFO is lost. */

    if ((pRequest->stLength >= (size_t) (pRequest->pEndpoint->wPacketSize << 2)) && dma_available)
    /* See if there is a DMA channel available to handle this request */
    {
        uint16_t wNumPackets;

        /* Calculate the length of whole packets to transfer */
        size_t stDmaTransferLength = pRequest->stLength - (pRequest->stLength % pRequest->pEndpoint->wPacketSize);

        /* Subtract two packets from the length to allow then to be brought in
         by the double buffered FIFO without receiving data for the next
         transfer */
        stDmaTransferLength -= (size_t) (pRequest->pEndpoint->wPacketSize << 1);

        /* Calculate the number of packets */
        wNumPackets = (uint16_t) (stDmaTransferLength / pRequest->pEndpoint->wPacketSize);

        /* Set the cancel function */
        pRequest->pCancel = usbhCancelBulkInDma;

        /* Set the DMA transfer length */
        pRequest->stTransferSize = stDmaTransferLength;

        /* Setup the DMA to perform the transfer */
        dmaStartUsbInCh1(pRequest->pMemory, stDmaTransferLength, R_USBH_DmaFIFO(pRequest->pUSB, USBH_IN), pRequest,
                usbhCompleteDmaIn);

        /* Set the pipe DMA FIFO for the transfer */
        R_USBH_DmaReadPipe(pUSB, iPipeNumber, wNumPackets);
        TRACE(("usbhStartBulkInTransfer: Started DMA %lu\r\n",
                        pRequest->stLength));

    }
    else
    {
        /* Set the cancel function */
        pRequest->pCancel = usbhCancelInFifo;

        /* Enable the buffer ready interrupt */
        R_USBH_SetPipeInterrupt(pRequest->pUSB, iPipeNumber, USBH_PIPE_BUFFER_READY);
        TRACE(("usbhStartBulkInTransfer: Started FIFO %lu\r\n",
                        pRequest->stLength));
    }

    /* Enable the not ready interrupt for detection of a STALL condition */
    R_USBH_SetPipeInterrupt(pRequest->pUSB, iPipeNumber, USBH_PIPE_BUFFER_NOT_READY);

    /* Set the pipe endpoint to buffer to start transfer */
    R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);

    /* Show the request is in progress */
    pRequest->bfInProgress = true;

    return true;
}
/******************************************************************************
 End of function  usbhStartBulkInTransfer
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhCompleteDmaIn
 Description:   Function to complete an IN transfer performed by DMA
 Arguments:     IN  pvRequest - Pointer to the transfer request
 Return value:  none
 ******************************************************************************/
static void usbhCompleteDmaIn (void *pvRequest)
{
    PUSBTR pRequest = pvRequest;
    int iPipeNumber = (int) pRequest->pInternal;
    uint8_t *pbyDest;
    size_t stLengthToRead;
    int32_t dma_ercd = 0;

    /* Disable the endpoint */
    R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
    /* Disable the IN DMA FIFO settings */
    R_USBH_StopDmaReadPipe(pRequest->pUSB);

    /* Stop the DMA - Remainder of transfer will now be performed by FIFO */
    dmaStopUsbInCh1();

    /* Update the index */
    pRequest->stIdx += pRequest->stTransferSize;
    /* The double buffered FIFO may have received up to two packets */
    stLengthToRead = pRequest->stLength - pRequest->stIdx;
    /* All of the data must be read from the FIFO to prevent data loss. Space
     was reserved for the last two packets when the transfer was started */
    while ((stLengthToRead) && (pRequest->stTransferSize))
    {
        /* Calculate the destination */
        pbyDest = pRequest->pMemory + pRequest->stIdx;
        /* Read the FIFO */
        pRequest->stTransferSize = R_USBH_ReadPipe(pRequest->pUSB, iPipeNumber, pbyDest, stLengthToRead);
        switch (pRequest->stTransferSize)
        {
            /* More data in FIFO than in room for in destination memory */
            case -2UL :
                TRACE(("USBH_DATA_OVERRUN_ERROR\r\n"));

                /* Set the error code */
                pRequest->errorCode = USBH_DATA_OVERRUN_ERROR;

                /* Clear the buffer of any data */
                R_USBH_ClearPipeFifo(pRequest->pUSB, iPipeNumber);

                /* Complete the request */
                usbhCompleteInFifo(pRequest, iPipeNumber);
                return;

                /* SIE would not release the FIFO */
            case -1UL :

                /* When running tests in single buffered mode this is to be
                 expected because there is no more data in the FIFO to read. */
                usbhCompleteByFIFO(pRequest, iPipeNumber);
                return;

                /* A packet was transferred */
            default :

                /* Show that this request is not idle */
                pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount++;

                /* Update the index */
                pRequest->stIdx += pRequest->stTransferSize;

                /* Transfer may have been completed by length */
                if ((pRequest->stIdx == pRequest->stLength)

                /* or a short packet */
                || (pRequest->stTransferSize < pRequest->pEndpoint->wPacketSize))
                {
                    /* Set the error code */
                    pRequest->errorCode = USBH_NO_ERROR;
                    /* Complete the request */
                    usbhCompleteInFifo(pRequest, iPipeNumber);
                    return;
                }

                /* Update the length remaining */
                stLengthToRead = pRequest->stLength - pRequest->stIdx;

                /* Check to see if there is more data in the FIFO */
                if (R_USBH_DataInFIFO(pRequest->pUSB, iPipeNumber) <= 0)
                {
                    /* The FIFO is empty so continue with standard single buffered
                     transfer */
                    usbhCompleteByFIFO(pRequest, iPipeNumber);
                    return;
                }
            break;
        }
    }

    /* Complete the request */
    usbhCompleteInFifo(pRequest, iPipeNumber);
}
/******************************************************************************
 End of function  usbhCompleteDmaIn
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhCancelBulkInDma
 Description:   Function to cancel a bulk in transfer
 Arguments:     IN  pRequest - Pointer to the transfer request
 Return value:  none
 ******************************************************************************/
static void usbhCancelBulkInDma (PUSBTR pRequest)
{
    PUSB pUSB = pRequest->pUSB;
    int iPipeNumber = (int) pRequest->pInternal;
    int32_t dma_ercd = 0;

    if (iPipeNumber)
    {
        /* Clear the buffer empty interrupt */
        R_USBH_SetPipeInterrupt(pRequest->pUSB, iPipeNumber,
                USBH_PIPE_BUFFER_READY | USBH_PIPE_BUFFER_NOT_READY | USBH_PIPE_INT_DISABLE);
        /* Set the pipe to NAK */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
        /* Update the endpoint data PID toggle bit */
        pRequest->pEndpoint->dataPID = R_USBH_GetPipePID(pRequest->pUSB, iPipeNumber);
        /* Update the transfer length */
        {
            size_t stLengthRemaining = (size_t) (R_USBH_DmaTransac(pUSB, iPipeNumber) * pRequest->pEndpoint->wPacketSize);
            pRequest->stIdx = pRequest->stTransferSize - stLengthRemaining;
        }

        /* Disable DMA to USB association */
        R_USBH_StopDmaPipe(pUSB);

        /* Free the pipe for use by another transfer */
        usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);

        /* Disable the DMA */
        dmaStopUsbInCh1();
    }
    else
    {
        TRACE(("usbhCancelBulkInDma: Invalid pipe number\r\n"));
    }
}
/******************************************************************************
 End of function  usbhCancelBulkInDma
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhStartBulkOutTransfer
 Description:   Function to start a bulk OUT transfer
 Arguments:     IN  pRequest - Pointer to the transfer request
 IN  iPipeNumber - The pipe number to use
 Return value:  true if the transfer is started
 ******************************************************************************/
static _Bool usbhStartBulkOutTransfer (PUSBTR pRequest, int iPipeNumber)
{
    PUSB pUSB = pRequest->pUSB;
    int32_t dma_ercd = 0;

    /* Check to see if this transfer can be handled by DMA */

    if ((pRequest->stLength >= (size_t) pRequest->pEndpoint->wPacketSize) && dma_available)
    {
        /* Calculate the length of whole packets to transfer */
        size_t stDmaTransferLength = pRequest->stLength - (pRequest->stLength % pRequest->pEndpoint->wPacketSize);

        /* Calculate the number of packets */
        uint16_t wNumPackets = (uint16_t) (stDmaTransferLength / pRequest->pEndpoint->wPacketSize);

        /* Set the cancel function */
        pRequest->pCancel = usbhCancelBulkOutDma;

        /* Set the DMA transfer length */
        pRequest->stTransferSize = stDmaTransferLength;

        /* Setup the DMA to perform the transfer */
        dmaStartUsbOutCh0(pRequest->pMemory, stDmaTransferLength, R_USBH_DmaFIFO(pRequest->pUSB, USBH_OUT), pRequest,
                usbhCompleteDmaOut);

        /* Set the pipe DMA FIFO for the transfer */
        R_USBH_DmaWritePipe(pUSB, iPipeNumber, wNumPackets);

        /* Enable the not ready interrupt for detection of a STALL condition */
        R_USBH_SetPipeInterrupt(pRequest->pUSB, iPipeNumber, USBH_PIPE_BUFFER_NOT_READY);

        /* Clear any pending interrupts */
        R_USBH_ClearPipeInterrupt(pRequest->pUSB, iPipeNumber, USBH_PIPE_BUFFER_EMPTY | USBH_PIPE_BUFFER_NOT_READY);

        /* Set the DMA used flag */
        pRequest->pUsbHc->pPipeTrack[iPipeNumber].bfTerminateOutDma = true;

        /* Set the pipe endpoint to buffer to start transfer */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);

        /* The transfer has been started or is in progress */
        pRequest->bfInProgress = true;
        TRACE(("usbhBulkOut: Started PIPE %u DMA %lu\r\n", iPipeNumber, pRequest->stLength));
        return true;
    }
    else
    {
        size_t stLengthWritten;

        /* Set the cancel function */
        pRequest->pCancel = usbhCancelOutFifo;

        /* Write into the pipe FIFO */
        stLengthWritten = R_USBH_WritePipe(pUSB, iPipeNumber, pRequest->pMemory, pRequest->stLength);

        /* Check that it wrote OK */
        if (stLengthWritten != -1U)
        {
            /* Show that the transfer is not idle */
            pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount++;

            /* Enable the buffer empty interrupt &
             Enable the not ready interrupt for detection of a
             STALL condition */
            R_USBH_SetPipeInterrupt(pRequest->pUSB, iPipeNumber, USBH_PIPE_BUFFER_EMPTY | USBH_PIPE_BUFFER_NOT_READY);

            /* Clear any pending interrupts */
            R_USBH_ClearPipeInterrupt(pRequest->pUSB, iPipeNumber, USBH_PIPE_BUFFER_EMPTY | USBH_PIPE_BUFFER_NOT_READY);

            /* Set the current transfer size */
            pRequest->stTransferSize = stLengthWritten;

            /* Set the pipe endpoint to buffer to start transfer */
            R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
                        
            /* The transfer has been started or is in progress */
            pRequest->bfInProgress = true;
            TRACE(("usbhBulkOut: Started FIFO %lu\r\n", pRequest->stLength));
            return true;
        }
    }

    return false;
}
/******************************************************************************
 End of function  usbhStartBulkOutTransfer
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhContinueBulkOutFifo
 Description:   Function to continue a bulk out FIFO transfer
 Arguments:     IN  pRequest - Pointer to the transfer request
 IN  iPipeNumber - The pipe number to use
 Return value:  none
 ******************************************************************************/
static void usbhContinueBulkOutFifo (PUSBTR pRequest, int iPipeNumber)
{
    /* Add on the length transferred */
    pRequest->stIdx += pRequest->stTransferSize;
    /* Check for termination of a DMA transfer */
    if (pRequest->pUsbHc->pPipeTrack[iPipeNumber].bfTerminateOutDma)
    {
        pRequest->pUsbHc->pPipeTrack[iPipeNumber].bfTerminateOutDma = false;
        /* Clear the DMA request bit */
        R_USBH_StopDmaPipe(pRequest->pUSB);
    }
    /* Check for a short packet */
    if ((pRequest->stTransferSize < pRequest->pEndpoint->wPacketSize)
    /* Or all of the data has been written */
    || (pRequest->stIdx == pRequest->stLength))
    {
        /* The transfer is complete */
        usbhCompleteOutFifo(pRequest, iPipeNumber);
    }
    else
    {
        size_t stLengthToWrite, stLengthWritten;
        uint8_t * pbySrc;
        /* Calculate the length to write */
        stLengthToWrite = pRequest->stLength - pRequest->stIdx;
        /* Calculate the source address for the next transfer */
        pbySrc = pRequest->pMemory + pRequest->stIdx;
        /* Write into the pipe FIFO */
        stLengthWritten = R_USBH_WritePipe(pRequest->pUSB, iPipeNumber, pbySrc, stLengthToWrite);
        if (stLengthWritten != -1U)
        {
            /* Show that the transfer is not idle */
            pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount++;
            /* Clear any pending interrupts */
            R_USBH_ClearPipeInterrupt(pRequest->pUSB, iPipeNumber, USBH_PIPE_BUFFER_EMPTY | USBH_PIPE_BUFFER_NOT_READY);
            /* Set the current transfer size */
            pRequest->stTransferSize = stLengthWritten;
            /* Set the pipe endpoint to buffer to start transfer */
            R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
            TRACE(("usbhBulkOut: Continue\r\n"));
        }
        else
        {
            /* Error accessing FIFO for write */
            pRequest->errorCode = USBH_FIFO_WRITE_ERROR;
            TRACE(("usbhBulkOut: Error accessing FIFO\r\n"));
        }
    }
}
/******************************************************************************
 End of function  usbhContinueBulkOutFifo
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhCompleteDmaOut
 Description:   Function to complete an OUT transfer performed by DMA
 Arguments:     IN  pvRequest - Pointer to the transfer request
 Return value:  none
 ******************************************************************************/
static void usbhCompleteDmaOut (void *pvRequest)
{
    PUSBTR pRequest = pvRequest;
    int iPipeNumber = (int) pRequest->pInternal;
    int32_t dma_ercd = 0;

    /* Set the cancel function */
    pRequest->pCancel = usbhCancelOutFifo;

    /* Enable the buffer empty interrupt to complete the transfer */
    R_USBH_SetPipeInterrupt(pRequest->pUSB, iPipeNumber, USBH_PIPE_BUFFER_EMPTY);

    /* Stop the DMA - The remainder of the transfer will now be performed
     with the FIFO */
    dmaStopUsbOutCh0();
}
/******************************************************************************
 End of function  usbhCompleteDmaOut
 ******************************************************************************/

/******************************************************************************
 Function Name: usbhCancelBulkOutDma
 Description:   Function to cancel a bulk out DMA transfer
 Arguments:     IN  pRequest - Pointer to the transfer request
 Return value:  none
 ******************************************************************************/
static void usbhCancelBulkOutDma (PUSBTR pRequest)
{
    PUSB pUSB = pRequest->pUSB;
    int iPipeNumber = (int) pRequest->pInternal;
    int32_t dma_ercd = 0;

    if (iPipeNumber)
    {
        /* Clear the buffer empty interrupt */
        R_USBH_SetPipeInterrupt(pRequest->pUSB, iPipeNumber,
                USBH_PIPE_BUFFER_EMPTY | USBH_PIPE_BUFFER_NOT_READY | USBH_PIPE_INT_DISABLE);
        /* Set the pipe to NAK */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
        /* Update the endpoint data PID toggle bit */
        pRequest->pEndpoint->dataPID = R_USBH_GetPipePID(pRequest->pUSB, iPipeNumber);
        /* Update the transfer length */
        {
            size_t stLengthRemaining = (size_t) (R_USBH_DmaTransac(pUSB, iPipeNumber) * pRequest->pEndpoint->wPacketSize);
            pRequest->stIdx = pRequest->stTransferSize - stLengthRemaining;
        }

        /* Disable DMA to USB association */
        R_USBH_StopDmaPipe(pUSB);

        /* Free the pipe for use by another transfer */
        usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
    }
    else
    {
        TRACE(("usbhCancelBulkOutDma: Invalid pipe number\r\n"));
    }

    /* Disable the DMA */
    dmaStopUsbOutCh0();
}
/******************************************************************************
 End of function  usbhCancelBulkOutDma
 ******************************************************************************/

/******************************************************************************
 End  Of File
 ******************************************************************************/
