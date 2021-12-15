/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "compiler_settings.h"

#include "diskio.h"        /* FatFs lower layer API */
#include "ff_types.h"
#include "r_fatfs_abstraction.h"
#include "blockCache.h"
#include "control.h"
#include "dskManager.h"

#define FS_ERR_DRIVER_FATAL_ERROR    (-11)

/* Definitions of physical drive number for each drive */
#define DEV_RAM        0    /* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC        1    /* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB        2    /* Example: Map USB MSD to physical drive 2 */

#define DSK_CACHE_SIZE              (1024 * 64)
#define DSK_MAX_BLOCK_TRANSFER      ((WORD)((1024 * 32)))


static FS_T_SINT32 dskWriteBlocks(const FS_T_UINT8    *pbyBuffer,
                                  FS_T_UINT32   dwSectorAddress,
                                  FS_T_UINT32   dwNumBlocks,
                                  PDRIVE        pDrive);
static FS_T_SINT32 dskReadBlocks(FS_T_UINT8     *pbyBuffer,
                                 FS_T_UINT32    dwSectorAddress,
                                 FS_T_UINT32    dwNumBlocks,
                                 PDRIVE         pDrive);
void dump_sector (uint32_t sector, char *buffer);

DRIVE drive_0;
PDRIVE p_drive_0 = &drive_0;
int sizeof_drive = sizeof(DRIVE);

/**********************************************************************************
Function Name: dskWriteBlocks
Description:   Function to write blocks to the deivce
Parameters:    IN  pbyBuffer - Pointer to the source data
               IN  dwSectorAddress - The sector address
               IN  dwNumBlocks - The number of blocks to write
               IN  pDrive - Pointer to the disk information
Return value:  The number of blocks written
**********************************************************************************/
static FS_T_SINT32 dskWriteBlocks(const FS_T_UINT8    *pbyBuffer,
                                  FS_T_UINT32   dwSectorAddress,
                                  FS_T_UINT32   dwNumBlocks,
                                  PDRIVE        pDrive)
{
    WORD wNumBlocks;
    FS_T_SINT32 lReturn = (FS_T_SINT32) dwNumBlocks;

    while (dwNumBlocks)
    {
        /* Calculate the number of blocks to write - SCSI 10 byte commands only
           has a 16bit number for the number of sectors */
        if (dwNumBlocks > DSK_MAX_BLOCK_TRANSFER)
        {
            wNumBlocks = DSK_MAX_BLOCK_TRANSFER;
        }
        else
        {
            wNumBlocks = (WORD) dwNumBlocks;
        }

        /* Write to the device through the cache */
        if (!bcWrite(pDrive->pBlockCache,
                     pbyBuffer,
                     dwSectorAddress,
                     (DWORD)wNumBlocks))
        {
            return FS_ERR_DRIVER_FATAL_ERROR;
        }

        dwNumBlocks -= (DWORD) wNumBlocks;
        dwSectorAddress += wNumBlocks;
        pbyBuffer += (pDrive->dwBlockSize * wNumBlocks);
    }

    return lReturn;
}
/**********************************************************************************
End of function  dskWriteBlocks
***********************************************************************************/

/**********************************************************************************
Function Name: dskReadBlocks
Description:   Function to read blocks from the device
Parameters:    IN  pbyBuffer - Pointer to the destination data
               IN  dwSectorAddress - The sector address
               IN  dwNumBlocks - The number of blocks to read
               IN  pDrive - Pointer to the disk information
Return value:  The number of blocks read
**********************************************************************************/
static FS_T_SINT32 dskReadBlocks(FS_T_UINT8     *pbyBuffer,
                                 FS_T_UINT32    dwSectorAddress,
                                 FS_T_UINT32    dwNumBlocks,
                                 PDRIVE         pDrive)
{
    WORD wNumBlocks;
    FS_T_SINT32 lReturn = (FS_T_SINT32) dwNumBlocks;

    while (dwNumBlocks)
    {
        /* Calculate the number of blocks to write - SCSI 10 byte commands only
           has a 16bit number for the number of sectors */
        if (dwNumBlocks > DSK_MAX_BLOCK_TRANSFER)
        {
            wNumBlocks = DSK_MAX_BLOCK_TRANSFER;
        }
        else
        {
            wNumBlocks = (WORD) dwNumBlocks;
        }

        /* Write to the device through the cache */
        if (!bcRead(pDrive->pBlockCache,
                    pbyBuffer,
                    dwSectorAddress,
                    (DWORD) wNumBlocks))
        {
            return FS_ERR_DRIVER_FATAL_ERROR;
        }

        dwNumBlocks -= (DWORD) wNumBlocks;
        dwSectorAddress += wNumBlocks;
        pbyBuffer += (pDrive->dwBlockSize * wNumBlocks);
    }

    return lReturn;
}
/**********************************************************************************
End of function  dskReadBlocks
***********************************************************************************/

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

/* Physical drive number to identify the drive */
DSTATUS disk_status (BYTE pdrv)
{
    UNUSED_PARAM(pdrv);
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Initialise a Drive                                                    */
/*-----------------------------------------------------------------------*/

/* Physical drive number to identify the drive */
DSTATUS disk_initialize(BYTE pdrv)
{
    UNUSED_PARAM(pdrv);
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE pdrv,        /* Physical drive number to identify the drive */
    BYTE *buff,        /* Data buffer to store read data */
    DWORD sector,    /* Start sector in LBA */
    UINT count        /* Number of sectors to read */
)
{
    UINT result;

    PDRIVE p_drive =  get_drive(pdrv);

    result = (UINT)dskReadBlocks(buff, sector, count, p_drive);

#ifdef DEBUG
    dump_sector(sector,buff);
    printf("disk_read(), sector %d, count %d\r\n", sector, count);

#endif
    if (result == count)
    {
        return RES_OK;
    }

    return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
    BYTE pdrv,            /* Physical drive number to identify the drive */
    const BYTE *buff,    /* Data to be written */
    DWORD sector,        /* Start sector in LBA */
    UINT count            /* Number of sectors to write */
)
{
    UINT blocks_written;

#ifdef DEBUG
    printf("disk_write(), sector %d, count %d\r\n", sector, count);
#endif

    PDRIVE p_drive =  get_drive(pdrv);

    blocks_written = (UINT)dskWriteBlocks(buff, sector, count, p_drive);

    if (blocks_written == count)
    {
        return RES_OK;
    }

    return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE pdrv,        /* Physical drive number (0..) */
    BYTE cmd,        /* Control code */
    void *buff        /* Buffer to send/receive control data */
)
{
    (void) cmd;
    (void) pdrv;
    (void) buff;

    return 0;
}

DWORD get_fattime (void)
{
    int rtc_handle = (-1);
    DATE date;
    DWORD returned_date = 0;

    rtc_handle = open(DEVICE_INDENTIFIER "rtc", O_RDWR);

    control(rtc_handle, CTL_GET_DATE, &date);

    if((-1) != rtc_handle)
    {
        close(rtc_handle);
    }

    returned_date |= (DWORD)( date.Field.Second/2);
    returned_date |= (DWORD)( date.Field.Minute   << 5);
    returned_date |= (DWORD)( date.Field.Hour     << 11);
    returned_date |= (DWORD)( date.Field.Day      << 16);
    returned_date |= (DWORD)( date.Field.Month    << 21);
    returned_date |= (DWORD)((date.Field.Year - 1980) << 25);

    return returned_date;
}

/******************************************************************************
* Function Name: dump_sector
* Description  : Hexadecimal dump a sector (in memory) to the terminal
* Arguments    : sector - the sector number
*              : buffer - pointer to memory containing the sector data
* Return Value : COMMAND_SUCCESS : Success
*              : COMMAND_ERROR   : Error
******************************************************************************/
void dump_sector (uint32_t sector, char *buffer)
{
    const uint8_t columns = 16;
    uint32_t offset;
    int16_t row;

    printf("Sector %d:\r\n\r\n", (int)sector);

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
