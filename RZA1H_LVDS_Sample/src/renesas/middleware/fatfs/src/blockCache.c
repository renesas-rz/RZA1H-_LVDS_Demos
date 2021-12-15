/******************************************************************************
* File Name    : blockCache.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNU
* OS           : None
* H/W Platform : RZA1
* Description  : Block based IO cache
******************************************************************************
* History      : DD.MM.YYYY Ver. Description
*              : DD.MM.YYYY 1.00 MAB First Release
******************************************************************************/

/******************************************************************************

  Block based IO cache adapted for use with FullFAT to reduce number of IO calls
  Copyright (C) 2009. Renesas Technology Corp.
  Copyright (C) 2009. Renesas Technology Europe Ltd.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

******************************************************************************/

/***********************************************************************************
System Includes
***********************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/***********************************************************************************
User Includes
***********************************************************************************/

#include "blockCache.h"
#include "types.h"
#include "ff_types.h"
#include "compiler_settings.h"
#include "r_os_abstraction_api.h"
#include "r_cache_l1_rz_api.h"
/***********************************************************************************
Typedefs
***********************************************************************************/

/* The structure if the cache index */
typedef struct _BCIDX
{
    /* The cache status */
    union {
            unsigned long   ulStatus;
        struct {
            /* Set when the cache entry is valid */
            unsigned long   Valid:1;
            /* The used count for the LRU entry replacement */
            unsigned long   Count:31;
        } Field;
    } Status;
    /* The tag */
    unsigned long   ulSector;
} BCIDX,
*PBCIDX;

/* The structure of the cache object */
typedef struct _BCACHE
{
    /* The ID of the MS device to access */
    int     iMsDev;
    int     iLun;
    /* The cache line size in sectors */
    int     iLineSize;
    /* The number of entries in the cache */
    int     iNumEntries;
    /* The block size of the MS device */
    int     iBlockSize;
    /* The total number of sectors on the MS device */
    unsigned long ulNumBlocks;
    /* Pointer to the start of the index */
    PBCIDX  pIndex;
    /* Pointer to the start of cache memory */
    unsigned char *pbyCache;

} BCACHE;

/***********************************************************************************
Function Prototypes
***********************************************************************************/
void dump_sector2 (uint32_t sector, char *buffer);
static unsigned char *bcSearch(PBACHE         pBlkCache,
                               unsigned long  ulSector,
                               PBCIDX        *ppEntry);
static unsigned char *bcGetEntry(PBACHE pBlkCache, PBCIDX *ppEntry);
static int bcInvalidate(PBACHE         pBlkCache,
                       unsigned long  ulSector,
                       unsigned long  ulNumSectors);

extern  int scsiRead10(int      iMsDev,
                       int      iLun,
                       DWORD    dwLBA,
                       WORD     wNumBlocks,
                       PBYTE    pbyDest,
                       size_t   stDestLength,
                       size_t   *pstLengthRead);
extern  int scsiWrite10(int     iMsDev,
                        int     iLun,
                        DWORD   dwLBA,
                        WORD    wNumBlocks,
                        PBYTE   pbySrc,
                        size_t  stSrcLength,
                        size_t  *pstLengthWritten);

/***********************************************************************************
Public Functions
***********************************************************************************/

/**********************************************************************************
Function Name: bcCreate
Description:   Function to create a block cache
Parameters:    IN  iMsDev - The file descriptor of the MS device
               IN  iLun  - The logical unit number of the device
               IN  iLineSize - The cache line size in sectors
               IN  iNumEntries - The number of entries
               IN  iBlockSize - The block size of the device
Return value:  pointer to the block cache or NULL on error
**********************************************************************************/
PBACHE bcCreate(int iMsDev,
                int iLun,
                int iLineSize,
                int iNumEntries,
                int iBlockSize,
                unsigned long ulNumBlocks)
{
    PBACHE  pBlkCache;
    size_t  stSize = sizeof(BCACHE);

    /* Add on the size of the cache memory */
    stSize += (size_t)(iLineSize * iNumEntries * iBlockSize);

    /* Add on the size of the index */
    stSize += (size_t)(sizeof(BCIDX) * (unsigned int)iNumEntries);

    /* Allocate the memory */
    pBlkCache = (PBACHE)R_OS_AllocMem(stSize, R_REGION_LARGE_CAPACITY_RAM);
    if (pBlkCache)
    {
        /* Initialise the data */
        pBlkCache->iMsDev = iMsDev;
        pBlkCache->iLun = iLun;
        pBlkCache->iLineSize = iLineSize;
        pBlkCache->iNumEntries = iNumEntries;
        pBlkCache->iBlockSize = iBlockSize;
        pBlkCache->ulNumBlocks = ulNumBlocks;
        pBlkCache->pIndex =  (PBCIDX)(((char*)pBlkCache) + sizeof(BCACHE));
        pBlkCache->pbyCache = (unsigned char*)((char*)pBlkCache->pIndex + (sizeof(BCIDX) * (unsigned long)iNumEntries));
        memset((int*)pBlkCache->pIndex, 0, (sizeof(BCIDX) * (unsigned long)iNumEntries));
        memset(pBlkCache->pbyCache, 0, (size_t)(iLineSize * iNumEntries * iBlockSize));
    }
    return pBlkCache;
}
/**********************************************************************************
End of function  bcCreate
***********************************************************************************/

/**********************************************************************************
Function Name: bcDestroy
Description:   Function to destroy a block cache
Parameters:    IN  pBlkCache - Pointer to the block cache to destroy
Return value:  none
**********************************************************************************/
void bcDestroy(PBACHE pBlkCache)
{
    R_OS_FreeMem(pBlkCache);
}
/**********************************************************************************
End of function  bcDestroy
***********************************************************************************/

/**********************************************************************************
Function Name: bcRead
Description:   Function to read with cache
Parameters:    IN  pBlkCache - Pointer to the block cache
               IN  pbyBuffer - Pointer to the destinaton buffer memory
               IN  ulSector - The starting sector (block)
               IN  ulNumberOfSectors - The number of sectors (blocks)
Return value:  The number of blocks read
**********************************************************************************/
unsigned long bcRead(PBACHE         pBlkCache,
                     unsigned char *pbyBuffer,
                     unsigned long  ulSector,
                     unsigned long  ulNumberOfSectors)
{
    unsigned long ulCacheSector = (ulSector - (ulSector % (unsigned long)pBlkCache->iLineSize));
    size_t  stLengthRead;

    /* Cache only for small & frequent requests */
    if ((ulNumberOfSectors == 1UL)
    &&  ((ulCacheSector + (unsigned long)pBlkCache->iLineSize) < (unsigned long)pBlkCache->ulNumBlocks))
    {
        PBCIDX  pEntry;
        /* Search the cache for this entry */
        unsigned char *pbyCache = bcSearch(pBlkCache, ulSector, &pEntry);
        if (!pbyCache)
        {
            /* Get a new cache entry */
            volatile size_t  stLength = (size_t)(pBlkCache->iLineSize * pBlkCache->iBlockSize);
            pbyCache = bcGetEntry(pBlkCache, &pEntry);

            R_CACHE_L1_CleanInvalidLine((uint32_t) pbyCache,stLength);
            
            /* Read into the cache memory */
            if (scsiRead10(pBlkCache->iMsDev,
                           pBlkCache->iLun,
                           ulCacheSector,
                           (WORD)pBlkCache->iLineSize,
                           pbyCache,
                           stLength,
                           &stLengthRead))
            {
                /* Device driver error */
                return 0UL;
            }
#ifdef DEBUG
            dump_sector2(ulSector, pbyCache);
#endif

            /* Set the start sector */
            pEntry->ulSector = ulCacheSector;
            /* Set the valid flag */
            pEntry->Status.Field.Valid = TRUE;
            /* Set the sector address */
            pbyCache = pbyCache + ((ulSector - ulCacheSector) * (unsigned long)pBlkCache->iBlockSize);
        }
        /* Copy to the destination */
        memcpy(pbyBuffer, pbyCache, (size_t)pBlkCache->iBlockSize);
        return 1UL;
    }
    /* Read directly into memory */
    if (scsiRead10(pBlkCache->iMsDev,
                   pBlkCache->iLun,
                   ulSector,
                   (WORD)ulNumberOfSectors,
                   pbyBuffer,
                   (size_t)(ulNumberOfSectors * (unsigned long)pBlkCache->iBlockSize),
                   &stLengthRead))
    {
        /* Device driver error */
        return 0UL;
    }
   // dump_sector2(ulSector, pbyBuffer);

    return ulNumberOfSectors;
}
/**********************************************************************************
End of function  bcRead
***********************************************************************************/

/**********************************************************************************
Function Name: bcWrite
Description:   Function to write with cache
Parameters:    IN  pBlkCache - Pointer to the block cache
               IN  pbyBuffer - Pointer to the source buffer memory
               IN  ulSector - The starting sector (block)
               IN  ulNumberOfSectors - The number of sectors (blocks)
Return value:  The number of blocks written
**********************************************************************************/
unsigned long bcWrite(PBACHE         pBlkCache,
                      const unsigned char *pbyBuffer,
                      unsigned long  ulSector,
                      unsigned long  ulNumberOfSectors)
{
    size_t  stLengthWritten;
    /* Invalidate any entries in the cache which are in this area */
    bcInvalidate(pBlkCache, ulSector, ulNumberOfSectors);
    /* Write directly to the device */
    if (scsiWrite10(pBlkCache->iMsDev,
                    pBlkCache->iLun,
                    ulSector,
                    (WORD)ulNumberOfSectors,
                    (PBYTE)pbyBuffer,
                    (size_t)(ulNumberOfSectors * (unsigned long)pBlkCache->iBlockSize),
                    &stLengthWritten))
    {
        /* Device driver error */
        return 0UL;
    }
    return ulNumberOfSectors;
}
/**********************************************************************************
End of function  bcWrite
***********************************************************************************/

/***********************************************************************************
Private Functions
***********************************************************************************/

/**********************************************************************************
Function Name: bcSearch
Description:   Function to search the cache for a valid entry
Parameters:    IN  pBlkCache - Pointer to the block cache
               IN  ulSector - The starting sector (block)
               OUT ppEntry - Pointer to the entry pointer
Return value:  Pointer to the cache memory or NULL if not found
**********************************************************************************/
static unsigned char *bcSearch(PBACHE         pBlkCache,
                               unsigned long  ulSector,
                               PBCIDX        *ppEntry)
{
    PBCIDX  pEntry = pBlkCache->pIndex;
    PBCIDX  pEnd = pEntry + pBlkCache->iNumEntries;
    /* Search each entry in the cache index */
    while (pEntry < pEnd)
    {
        /* If the entry is valid */
        if ((pEntry->Status.Field.Valid)
        /* And the sector is within the line */
        &&  (ulSector >= pEntry->ulSector)
        &&  (ulSector < (unsigned long)((unsigned long)pEntry->ulSector + (unsigned long)pBlkCache->iLineSize)))
        {
            /* Calculate the position in the cache memory */
            unsigned char *pbySrc = (pBlkCache->pbyCache
                                  + ((pEntry - pBlkCache->pIndex)
                                  * pBlkCache->iLineSize * pBlkCache->iBlockSize)
                                  + ((ulSector - (unsigned long)pEntry->ulSector) * (unsigned long)pBlkCache->iBlockSize));
            if (ppEntry)
            {
                *ppEntry = pEntry;
            }
            return pbySrc;
        }
        /* Count the times each entry is searched */
        pEntry->Status.Field.Count++;
        pEntry++;
    }
    return NULL;
}
/**********************************************************************************
End of function  bcSearch
***********************************************************************************/

/**********************************************************************************
Function Name: bcGetEntry
Description:   Function to get a pointer to a free entry
Parameters:    IN  pBlkCache - Pointer to the block cache
               OUT ppEntry - Pointer to an entry pointer
Return value:  Pointer to the cache memory associated with the entry
**********************************************************************************/
static unsigned char *bcGetEntry(PBACHE pBlkCache, PBCIDX *ppEntry)
{
    PBCIDX  pEntry = pBlkCache->pIndex;
    PBCIDX  pResult = pEntry;
    PBCIDX  pEnd = pEntry + pBlkCache->iNumEntries;
    unsigned char *pbySrc = pBlkCache->pbyCache;
    int     iUsedCount = 0;
    /* Search each entry in the cache index */
    while (pEntry < pEnd)
    {
        int iCount = (int)pEntry->Status.Field.Count;
        /* If an entry is not valid it is good to use */
        if (!pEntry->Status.Field.Valid)
        {
            pResult = pEntry;
            break;
        }
        /* Check for the least recently used */
        if (iCount > iUsedCount)
        {
            iUsedCount = iCount;
            pResult = pEntry;
        }
        pEntry++;
    }
    /* Calculate the position in the cache memory */
    pbySrc += ((pResult - pBlkCache->pIndex)
           * pBlkCache->iLineSize * pBlkCache->iBlockSize);
    /* Return the entry */
    *ppEntry = pResult;
    /* Reset the status flags */
    pResult->Status.ulStatus = 0UL;
    /* Return the position */
   return pbySrc;
}
/**********************************************************************************
End of function  bcGetEntry
***********************************************************************************/

/**********************************************************************************
Function Name: bcInvalidate
Description:   Function to invalidate any cache entry within the range
Parameters:    IN  pBlkCache - Pointer to the block cache
               IN  ulSector - The starting sector
               IN  ulNumSectors - The number of sectors
Return value:  0 for success -1 on error
**********************************************************************************/
static int bcInvalidate(PBACHE         pBlkCache,
                        unsigned long  ulSector,
                        unsigned long  ulNumSectors)
{
    PBCIDX  pEntry = pBlkCache->pIndex;
    PBCIDX  pEnd = pEntry + pBlkCache->iNumEntries;
    unsigned long ulEndSector = ulSector + ulNumSectors;
    /* Search each entry in the cache index */
    while (pEntry < pEnd)
    {
        if (pEntry->Status.Field.Valid)
        {
            unsigned long   ulEntryStart = pEntry->ulSector;
            unsigned long   ulEntryEnd = (ulEntryStart + (unsigned long)pBlkCache->iLineSize);
            if (((ulSector >= ulEntryStart) && (ulSector < ulEntryEnd))
            ||  ((ulEndSector >= ulEntryStart) && (ulEndSector < ulEntryEnd)))
            {
                /* Invalidate */
                pEntry->Status.ulStatus = 0;
            }
        }
        pEntry++;
    }
    return 0;
}
/**********************************************************************************
End of function  bcInvalidate
***********************************************************************************/

/***********************************************************************************
End  Of File
***********************************************************************************/


/******************************************************************************
* Function Name: dump_sector
* Description  : Hexadecimal dump a sector (in memory) to the terminal
* Arguments    : sector - the sector number
*              : buffer - pointer to memory containing the sector data
* Return Value : COMMAND_SUCCESS : Success
*              : COMMAND_ERROR   : Error
******************************************************************************/
void dump_sector2 (uint32_t sector, char *buffer)
{
    const uint8_t columns = 16;
    uint32_t offset;
    int16_t row;

    printf("                   ");
    for (int16_t column = 0; column < columns; column++)
    {
        printf("0x%02x ", column);

        if ((column & 0x3) == 0x3)
        {
            printf(" ");
        }
    }

    printf("0123456789ABCDEF\n\r\n\r");

    for (row = 0; row < (512 / columns); row++)
    {
        offset = (sector * 512) + (uint32_t) (row * columns);
        printf("0x%06lx %08ld  ", offset, offset);

        /* print values in hex */
        for (int16_t column = 0; column < columns; column++)
        {
            printf("0x%02x ", buffer[(row * columns) + column]);

            if ((column & 0x3) == 0x3)
            {
                printf(" ");
            }
        }

        /* if printable, then print text */
        for (int16_t column = 0; column < columns; column++)
        {
            char c = buffer[(row * columns) + column];
            if ((c >= ' ') && (c <= 126))
            {
                printf("%c", c);
            }
            else
            {
                printf(" ");
            }
        }
        printf("\n\r");
    }
    printf("\n\r");
}
