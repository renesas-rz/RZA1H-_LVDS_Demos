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

#ifndef BLOCKCACHE_H_INCLUDED
#define BLOCKCACHE_H_INCLUDED

/***********************************************************************************
Typedefs
***********************************************************************************/

typedef struct _BCACHE *PBACHE;

/***********************************************************************************
Public Functions
***********************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************
Function Name: bcCreate
Description:   Function to create a block cache
Parameters:    IN  iMsDev - The file descriptor of the MS device
               IN  iLun  - The logical unit number of the device
               IN  iLineSize - The cache line size in sectors
               IN  iNumEntries - The number of entries
               IN  iBlockSize - The block size of the device
               IN  ulNumBlocks - The total number of blocks
Return value:  pointer to the block cache or NULL on error
**********************************************************************************/

extern  PBACHE bcCreate(int iMsDev,
                        int iLun,
                        int iLineSize,
                        int iNumEntries,
                        int iBlockSize,
                        unsigned long ulNumBlocks);

/**********************************************************************************
Function Name: bcDestroy
Description:   Function to destroy a block cache
Parameters:    IN  pBlkCache - Pointer to the block cache to destroy
Return value:  none
**********************************************************************************/

extern  void bcDestroy(PBACHE pBlkCache);

/**********************************************************************************
Function Name: bcRead
Description:   Function to read with cache
Parameters:    IN  pBlkCache - Pointer to the block cache
               IN  pbyBuffer - Pointer to the destinaton buffer memory
               IN  ulSector - The starting sector (block)
               IN  ulNumberOfSectors - The number of sectors (blocks)
Return value:  The number of blocks read
**********************************************************************************/

extern  unsigned long bcRead(PBACHE         pBlkCache,
                             unsigned char *pbyBuffer, 
                             unsigned long  ulSector,
                             unsigned long  ulNumberOfSectors);

/**********************************************************************************
Function Name: bcWrite
Description:   Function to write with cache
Parameters:    IN  pBlkCache - Pointer to the block cache
               IN  pbyBuffer - Pointer to the source buffer memory
               IN  ulSector - The starting sector (block)
               IN  ulNumberOfSectors - The number of sectors (blocks)
Return value:  The number of blocks written
**********************************************************************************/

extern  unsigned long bcWrite(PBACHE         pBlkCache,
                              const unsigned char *pbyBuffer,
                              unsigned long  ulSector,
                              unsigned long  ulNumberOfSectors);

#ifdef __cplusplus
}
#endif

#endif                              /* BLOCKCACHE_H_INCLUDED */

/**********************************************************************************
End  Of File
***********************************************************************************/
