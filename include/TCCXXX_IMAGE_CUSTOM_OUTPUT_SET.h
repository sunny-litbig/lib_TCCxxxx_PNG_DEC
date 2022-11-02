// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) Telechips Inc.
 */
/******************************************************************************
 file name : TCCXXX_IMAGE_CUSTOM_OUTPUT_SET.h (V1.55)
******************************************************************************/

#ifndef __TCCXXX_IMAGE_CUSTOM_OUTPUT_SET_H__
#define __TCCXXX_IMAGE_CUSTOM_OUTPUT_SET_H__

typedef struct {
	int Comp_1;
	int Comp_2;
	int Comp_3;
	int Comp_4;		//V.1.52~
	int Offset;
	int x;
	int y;
	int Src_Fmt;
}IM_PIX_INFO;

#define IM_SRC_YUV		0
#define IM_SRC_RGB		1
#define IM_SRC_OTHER	2

extern unsigned int	IM_2nd_Offset;
extern unsigned int	IM_3rd_Offset;
extern unsigned int	IM_LCD_Half_Stride;
extern unsigned int	IM_LCD_Addr;

void TCC_CustomOutput_RGB565(IM_PIX_INFO out_info);
void TCC_CustomOutput_RGB888_With_Alpha(IM_PIX_INFO out_info);
void TCC_CustomOutput_RGB888(IM_PIX_INFO out_info);
void TCC_CustomOutput_YUV420(IM_PIX_INFO out_info);
void TCC_CustomOutput_YUV444(IM_PIX_INFO out_info);

void TCC_CustomOutput_RGB888_With_Alpha_ISDBT(IM_PIX_INFO out_info); //PNGDEC_SUPPORT_ISDBT_PNG

#endif //__TCCXXX_IMAGE_CUSTOM_OUTPUT_SET_H__
