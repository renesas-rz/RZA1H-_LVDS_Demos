/*******************************************************************************
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
 * http://www.renesas.com/disclaimer*
 * Copyright (C) 2018 Renesas Electronics Corporation. All rights reserved.
 *******************************************************************************/

/*******************************************************************************
* File Name   : r_sdk_camera_graphics.c
* Version     : 1.00 <- Optional as long as history is shown below
* Description : This module provides function of register value changing
                regarding image quality adjustment.
******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version Description
*         : 01.04.2017 1.05 first version
******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "r_typedefs.h"

#include "r_rvapi_header.h"
#include "lcd_panel.h"
#include "mcu_board_select.h"
#include "r_image_config.h"
#include "r_vdc_portsetting.h"
#include "r_display_init.h"
#include "r_sdk_camera_graphics.h"

/******************************************************************************
Macro definitions
******************************************************************************/
#define GRAPHICS_TSK_PRI   (osPriorityNormal)

/* Display area */
#define     DISP_AREA_HS                (0u)
#define     DISP_AREA_VS                (0u)
#define     DISP_AREA_HW                (800u)
#define     DISP_AREA_VW                (480u)


#define     DISP_BUFFER_STRIDE         (((DISP_AREA_HW * 2u) + 31u) & ~31u)
#define     DISP_BUFFER_HEIGHT         (DISP_AREA_VW)
#define     DISP_BUFFER_NUM            (2u)

void sdk_camera_graphics_sample_task(void const *pArg);

/******************************************************************************
Typedef definitions
******************************************************************************/

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

/******************************************************************************
Exported global variables (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/
uint8_t graphic_buffer[DISP_BUFFER_NUM][DISP_BUFFER_STRIDE * DISP_BUFFER_HEIGHT] __attribute__ ((section(".VRAM_SECTION0")));

/***********************************************************************************************************************
 * Function Name: graphics_sample_task
 * Description  : Creates touch screen task
 * Arguments    : PCOMSET pCOM
 * Return Value : none
 ***********************************************************************************************************************/
void sdk_camera_graphics_sample_task(void const *pArg)
{
    UNUSED_PARAM(pArg);
    vdc_error_t error;
    vdc_channel_t vdc_ch = VDC_CHANNEL_0;
    uint8_t data = 0x00;

    /***********************************************************************/
    /* display init (VDC5 output setting) */
    /***********************************************************************/
    {
        error = r_display_init (vdc_ch);
    }

    /***********************************************************************/
    /* display buffer clear */
    /***********************************************************************/
    memset( &graphic_buffer[0], 0x00,  DISP_BUFFER_STRIDE * DISP_BUFFER_HEIGHT );
    memset( &graphic_buffer[1], 0x00,  DISP_BUFFER_STRIDE * DISP_BUFFER_HEIGHT );

    /***********************************************************************/
    /* Graphic Layer 0 VDC_GR_FORMAT_YCBCR422 */
    /***********************************************************************/
    {
        gr_surface_disp_config_t gr_disp_cnf;

        gr_disp_cnf.layer_id  = VDC_LAYER_ID_0_RD; /* Layer ID                        */
        gr_disp_cnf.fb_buff   = (void *)graphic_buffer[0]; /* Frame buffer address            */
        gr_disp_cnf.fb_stride = DISP_BUFFER_STRIDE; /* Frame buffer stride             */

        /* Display Area               */
        gr_disp_cnf.disp_area.hs_rel = DISP_AREA_HS;
        gr_disp_cnf.disp_area.hw_rel = DISP_AREA_HW;
        gr_disp_cnf.disp_area.vs_rel = DISP_AREA_VS;
        gr_disp_cnf.disp_area.vw_rel = DISP_AREA_VW;

        gr_disp_cnf.read_format   = VDC_GR_FORMAT_RGB565; /* Read Format                     */
        gr_disp_cnf.read_ycc_swap = VDC_GR_YCCSWAP_Y0CBY1CR; /* Read Swap for YCbCr422     */
        gr_disp_cnf.read_swap     = VDC_WR_RD_WRSWA_NON; /* Read Swap 8bit/16bit/32bit */

        gr_disp_cnf.clut_table = NULL;  /* Setting if Read Format is CLUT. */

        gr_disp_cnf.disp_mode = VDC_DISPSEL_CURRENT;   /* Display mode select        */

        error = R_RVAPI_GraphCreateSurfaceVDC(vdc_ch, &gr_disp_cnf);
    }

    /* Image Quality Adjustment */
    if (VDC_OK == error)
    {
        error = r_image_quality_adjustment(vdc_ch);
    }

    /* Enable signal output */
    if (VDC_OK == error)
    {
        /* Wait for register update */
        R_OS_TaskSleep (16);
        R_RVAPI_DispPortSettingVDC(vdc_ch, &VDC_LcdPortSetting);
    }

    while (1)
    {
    	data++;
        memset( &graphic_buffer[1], data,  DISP_BUFFER_STRIDE * DISP_BUFFER_HEIGHT );
        R_RVAPI_GraphChangeSurfaceVDC(vdc_ch,VDC_LAYER_ID_0_RD, &graphic_buffer[1]);
    	R_OS_TaskSleep (1000);

    	data++;
        memset( &graphic_buffer[0], data,  DISP_BUFFER_STRIDE * DISP_BUFFER_HEIGHT );
        R_RVAPI_GraphChangeSurfaceVDC(vdc_ch,VDC_LAYER_ID_0_RD, &graphic_buffer[0] );
    	R_OS_TaskSleep (1000);
    }
}
