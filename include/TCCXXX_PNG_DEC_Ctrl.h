// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) Telechips Inc.
 */
/***********************************************************
 File name : TCCXXX_PNG_DEC_Ctrl.h
***********************************************************/

#ifndef __TCCXXX_PNG_DEC_CTRL___
#define __TCCXXX_PNG_DEC_CTRL___


/***************************************************
 Control Options
***************************************************/
/* control Compiler
		0: None
		1: C compiler
		4: Linux
*/
#define	TCPNGD_CTRL_COMPILER	0	
#if TCPNGD_CTRL_COMPILER	== 0
	#if defined(__arm)				//ADS 1.2 predefined keyword
		#define TCPNGD_COMPILER_ARM
		#define TCPNGD_COMPILER_ADS1_2
	#elif defined (_WIN32_WCE) && defined (ARM)
		#define TCPNGD_COMPILER_ARM
		#define TCPNGD_COMPILER_WINCE
	#elif defined(__LINUX__ )//defined(__GNUC__) && defined(__arm__)
		#define TCPNGD_COMPILER_ARM
		#define TCPNGD_COMPILER_LINUX
	#elif  defined(_WIN32)
		#define	TCPNGD_COMPILER_C
	//#else
	//	[ERROR: Unknown]
	#endif
#elif TCPNGD_CTRL_COMPILER	== 1
	#define	TCPNGD_COMPILER_C
#elif TCPNGD_CTRL_COMPILER	== 2
	#define TCPNGD_COMPILER_ARM
	#define TCPNGD_COMPILER_ADS1_2
#elif TCPNGD_CTRL_COMPILER	== 3
	#define TCPNGD_COMPILER_ARM
	#define TCPNGD_COMPILER_WINCE
#elif TCPNGD_CTRL_COMPILER	== 4
	#define TCPNGD_COMPILER_ARM
	#define TCPNGD_COMPILER_LINUX
#else
	[ERROR: Compiler control]
#endif

#if defined(TCPNGD_COMPILER_WINCE) || defined(TCPNGD_COMPILER_LINUX)
	#define TCPNGD_INLINEASM_EXCLUSION
#endif


/***************************************************
 Lib. version
***************************************************/
#define TCPNGD_LIB_VERION_VALUE			200
#define TCPNGD_LIB_VERION_VALUE_HW_ON	0 //0:off, 1:on

#define TCPNGD_LIB_VERNAME_VAR 			TC_CODEC_PNGDEC_ ## \
										xxxx_ ## \
										libVer_ ## \
										200_0										
//example: TC_CODEC_PNGDEC_xxxx_libVer_200_0

#define TCPNGD_LIB_VERSION_CHIP	0x0000 //SW only or debug
#define TCPNGD_PTE_TYPE			0

#define TCPNGD_LIB_VERSION  (\
	((unsigned int)(TCPNGD_LIB_VERSION_CHIP|TCPNGD_LIB_VERION_VALUE_HW_ON)<<16)\
	|TCPNGD_LIB_VERION_VALUE )



/***************************************************
 modification
***************************************************/

#define PNGDEC_SUPPORT_ISDBT_PNG

#define PNGDEC_REPORT_BITDEPTH

#define PNGDEC_CHECK_CHUNK
#if defined(PNGDEC_CHECK_CHUNK)
	#define PNGDEC_CHUNK_IHDR	0
	#define PNGDEC_CHUNK_PLTE	1
	#define PNGDEC_CHUNK_IDAT	2
	#define PNGDEC_CHUNK_IEND	3
	#define PNGDEC_CHUNK_TRNS	4
	#define PNGDEC_CHUNK_CHRM	5
	#define PNGDEC_CHUNK_GAMA	6
	#define PNGDEC_CHUNK_ICCP	7
	#define PNGDEC_CHUNK_SBIT	8
	#define PNGDEC_CHUNK_SRBG	9
	#define PNGDEC_CHUNK_ITXT	10
	#define PNGDEC_CHUNK_TEXT	11
	#define PNGDEC_CHUNK_ZTXT	12
	#define PNGDEC_CHUNK_BKGD	13
	#define PNGDEC_CHUNK_HIST	14
	#define PNGDEC_CHUNK_PHYS	15
	#define PNGDEC_CHUNK_SPLT	16
	#define PNGDEC_CHUNK_TIME	17

	#define PNGDEC_CHUNK_FRAC	18
	#define PNGDEC_CHUNK_GIFG	19
	#define PNGDEC_CHUNK_GIFT	20
	#define PNGDEC_CHUNK_GIFX	21
	#define PNGDEC_CHUNK_OFFS	22
	#define PNGDEC_CHUNK_PCAL	23
	#define PNGDEC_CHUNK_SCAL	24
	#define PNGDEC_CHUNK_STER	25
#endif

#define PNGDEC_MEMSET_INTERNAL			//for memset

#define PNGDEC_ABS_INTERNAL
#if	defined(PNGDEC_ABS_INTERNAL)		//for abs()
#	define PNGD_ABS(x) ((x)<0)?(-(x)):(x)
#else
#	define PNGD_ABS(x) abs(x)
#endif

#define PNGDEC_CHECK_EOF
#define PNGDEC_CHECK_EOF_2
#ifdef PNGDEC_CHECK_EOF_2
#define PNGDEC_FREAD_FAILURE_RET	1	// 0: default(old) : 읽어야 할 크기만큼 읽지 못할때 '0'
#else
#define PNGDEC_FREAD_FAILURE_RET	0	// 0: default(old) : 읽어야 할 크기만큼 읽지 못할때 '0'
#endif

#define PNGDEC_SKIP_TRNS_BEFORE_PLTE

#define PNGDEC_MOD_MEM_ALIGN

#define PNGDEC_MOD_DIV0

//#define PNGDEC_MOD_RGB_COMP4_RESET

/* modified structure huft */
#if defined(TCPNGD_COMPILER_LINUX) && !defined(PNGDEC_MOD_TYPE_STRUCT_HUFT)
#define PNGDEC_MOD_TYPE_STRUCT_HUFT
#endif

/* Add chunks : PNGEXT 1.2.0 or 1.3.0 (skip) */
//	PNGEXT 1.2.0(21 Nov. 2000): fRAc, gIFg, gIFt, gIFx, oFFs, pCAL, sCAL
//	PNGEXT 1.3.0(30 Aug. 2006): sTER  
//#define PNGDEC_ADD_ANCILLARY_CHUNK_EXT	120130


/* optim. : memory */
#define PNGDEC_OPTI_INSTANCE_MEM
	/*	Ver.	Code		Data		Instance Buff
		1.53	26268(26K)	50060(49K)	0
		1.54	26380(27K)	668(1K)		49412(49K)
	*/

#define PNGDEC_MOD_API20081013
#if defined(PNGDEC_MOD_API20081013)
#	define PNGDEC_OPTI_INSTANCE_MEM	
#endif 


/* stability: Check Bit_depth */
#define PNGDEC_STABILITY_BIT_DEPTH	// 1,2,4,8,16


/* stability: error */
#define PNGDEC_STABILITY_ERROR_HANDLE
#ifdef PNGDEC_STABILITY_ERROR_HANDLE

	/* stability: skip - Unknown ancillary chunk (skip or error ?)*/
	#define PNGDEC_STABILITY_READ_UNKNOWN_ANC_CHUNK_SKIP
			//for pngTest.png

#endif //PNGDEC_STABILITY_ERROR_HANDLE


#endif //__TCCXXX_PNG_DEC_CTRL___

