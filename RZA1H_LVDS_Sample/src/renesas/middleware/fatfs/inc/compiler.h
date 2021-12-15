/******************************************************************************
* File Name    : Compiler.h
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNU
* OS           : None
* H/W Platform : RZA1
* Description  : Compiler specific defines for abstraction
******************************************************************************
* History      : DD.MM.YYYY Ver. Description
*              : DD.MM.YYYY 1.00 MAB First Release
******************************************************************************/

/******************************************************************************

  Compiler specific defines for abstraction
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

#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

/**********************************************************************************
Defines
***********************************************************************************/

/*
 * Embedded CPU data type definitions
 ************************************
 *
 */
                                    /* Set a few #defines for potential
                                       compilers used */
#define                 MCS      0  /* Hitachi */
#define                 GNU      1  /* Hitachi + many other devices */
#define                 IAR      2  /* Hitachi + some other devices */
#define                 MSV      3  /* Microsoft Visual C */

                                    /* Test the compiler intrinisic defs */
                                    /* GNU compiler - C mode   */
#ifdef __GNUC__
#define COMPILER    GNU
                                    /* GNU compiler - C++ mode */
#elif defined(__GNUG__)
#define COMPILER    GNU
                                    /* IAR compiler (be careful) */
#elif defined __IAR_SYSTEMS_ICC
#define COMPILER    IAR
                                    /* Microsoft c compiler */
#elif defined _MSC_VER
#define COMPILER    MSV
#else
                                    /* MCS compiler */
#define COMPILER    MCS
                                    /* MCS compiler has MSB first even in little
                                       endian mode unless #pragma or command
                                       line switch used to change it */
#define _BITFIELDS_MSB_FIRST_
#endif

/**********************************************************************************
Pragma macros
***********************************************************************************/
                                    /* Visual Cpp */
#if COMPILER == MSV
#define PACK1                       pack(1)
#define UNPACK                      pack()
#else
                                    /* MCS SH & H8S series recently got unified
                                       pragma syntax */
#define PACK1                       # ## pragma pack 1
#define UNPACK                      # ## pragma unpack
#endif

#endif                              /* COMPILER_H_INCLUDED */

/**********************************************************************************
End  Of File
***********************************************************************************/
