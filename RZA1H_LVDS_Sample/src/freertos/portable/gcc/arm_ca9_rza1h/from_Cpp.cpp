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
* http://www.renesas.com/disclaimer
* Copyright (C) 2012 - 2015 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/
/***********************************************************************
* File: from_Cpp.cpp
*    Callback function from C++ runtime library.
*
* - $Rev: $
* - $Date:: 2017-01-17T15:08:51+09:00$
************************************************************************/

extern "C" {
#include "FreeRTOS.h"
}

#include <new>

/***********************************************************************
* Function: operator new
************************************************************************/

void *operator new (size_t size)
{
    void *p=pvPortMalloc(size);
    if (p==0) // did pvPortMalloc succeed?
    {
        throw std::bad_alloc(); // ANSI/ISO compliant behavior
    }
    return p;
}


/***********************************************************************
* Function: operator delete
************************************************************************/

void operator delete (void *p)
{
    vPortFree(p);
}

/***********************************************************************
* Function: operator new []
************************************************************************/

void *operator new [](size_t size)
{
    void *p=pvPortMalloc(size);
    if (p==0) // did pvPortMalloc succeed?
    {
        throw std::bad_alloc(); // ANSI/ISO compliant behavior
    }
    return p;
}


/***********************************************************************
* Function: operator delete []
************************************************************************/

void operator delete[] (void *p)
{
    vPortFree(p);
}


/***********************************************************************
* Variable: __dso_handle
*    C++ ABI - DSO Object Destruction API.
************************************************************************/
//unsigned /* uint32_t*/  __dso_handle;


