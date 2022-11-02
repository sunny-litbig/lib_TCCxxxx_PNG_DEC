// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) Telechips Inc.
 */
/******************************************************************************
 file name : TCCXXX_PNG_DEC_format.h
******************************************************************************/
#ifndef __TCCXXX_PNG_DEC_FORMAT_H__
#define __TCCXXX_PNG_DEC_FORMAT_H__

#include "TCCXXX_PNG_DEC_Ctrl.h"
#include "TCCXXX_PNG_DEC.h"
#include "TCCXXX_IMAGE_CUSTOM_OUTPUT_SET.h"

extern unsigned int	PD_OF_IM_2nd_Offset;
extern unsigned int	PD_OF_IM_3rd_Offset;
extern unsigned int	PD_OF_IM_LCD_Half_Stride;
extern unsigned int	PD_OF_IM_LCD_Addr;

extern void PNG_OF_rgb565_internal(IM_PIX_INFO out_info);
extern void PNG_OF_rgb888withAlpha_internal(IM_PIX_INFO out_info);
extern void PNG_OF_rgb888_internal(IM_PIX_INFO out_info);
extern void PNG_OF_yuv420_internal(IM_PIX_INFO out_info);
extern void PNG_OF_yuv444_internal(IM_PIX_INFO out_info);

#endif //__TCCXXX_PNG_DEC_FORMAT_H__
