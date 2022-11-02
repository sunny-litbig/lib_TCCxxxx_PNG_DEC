// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) Telechips Inc.
 */

#include "TCCXXX_PNG_DEC_Ctrl.h"
#ifdef PNGDEC_STABILITY_ERROR_HANDLE
	#include "TCCXXX_PNG_DEC_ErrorDef.h"
#endif

#include "TCCXXX_PNG_DEC.h"
#include "TCCXXX_PNG_TYPES.h"
#include "TCCXXX_PNG_DEC_format.h"

/*******************************************************************/
/**************************Structure Defines************************/
/*******************************************************************/

//Huffman Code Structure
#if !defined(PNGDEC_MOD_TYPE_STRUCT_HUFT)
typedef struct _huft huft;
typedef struct _huft{
	uint16 e;		/* number of extra bits or operation */
	uint16 b;		/* number of bits in this code or subcode */

	union
	{
		uint16 n;	/* literal, length base, or distance base */
		huft *t;	/* pointer to next level of table */
	}v;
}huft;
#else
typedef struct _huft{
	uint16 e;		/* number of extra bits or operation */
	uint16 b;		/* number of bits in this code or subcode */

	union
	{
		uint16 n;	/* literal, length base, or distance base */
		void *t;	/* pointer to next level of table */
	}v;
}huft;
#endif

//Palette Structure 
#define PD_PLTE_TABLE_STRUCT_SIZE	4
typedef struct{
	uint8 R;
	uint8 G;
	uint8 B;
	uint8 Alpha;
}PD_PLTE_TABLE;

typedef int (DECODE_IMAGE_BASEDON_BIT_DEPTH) (void);
typedef DECODE_IMAGE_BASEDON_BIT_DEPTH * Decode_Func_Ptr;

/*******************************************************************/
/************************Macro Defines******************************/
/*******************************************************************/
//Flag
#define		PD_FALSE			0
#define		PD_TRUE			1

//Defiltering Mode
#define		PD_Absolute_Perform		0
#define		PD_Conditional_Skip		1
#define		PD_Absolute_Skip		2

//Return values of Functions in PNG Decoder
#define		PD_PROCESS_DONE		0
#define		PD_PROCESS_ERROR		-1
#define		PD_PROCESS_CONTINUE		1
#define		PD_PROCESS_EOF		2

//PNG ID Marker Definitions (8 bytes)
#define		PD_MARKER_PNG1		0x89504E47
#define		PD_MARKER_PNG2		0x0D0A1A0A


//Critical Chunk Marker Definitions
#define		PD_MARKER_IHDR		0x49484452	// 7 Mar 95    PNG 1.2 (21 November 2000)
#define		PD_MARKER_PLTE		0x504C5445	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_IDAT		0x49444154	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_IEND		0x49454E44	// 7 Mar 95    PNG 1.2

//Ancillary Chunk Marker Definitions
#define		PD_MARKER_tRNS		0x74524E53	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_cHRM		0x6348524D	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_gAMA		0x67414D41	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_iCCP		0x69434350	//17 Aug 98    PNG 1.2
#define		PD_MARKER_sBIT		0x73424954	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_sRGB		0x73524742	// 6 Nov 96    PNG 1.2
#define		PD_MARKER_tEXt		0x74455874	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_zTXt		0x7A545874
#define		PD_MARKER_iTXt		0x69545874	// 9 Feb 99    PNG 1.2
#define		PD_MARKER_bKGD		0x624B4744	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_hIST		0x68495354	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_pHYs		0x70485973	// 7 Mar 95    PNG 1.2
#define		PD_MARKER_sPLT		0x73504C54	// 9 Dec 96    PNG 1.2
#define		PD_MARKER_tIME		0x74494D45	// 7 Mar 95    PNG 1.2
#if defined(PNGDEC_ADD_ANCILLARY_CHUNK_EXT120130)	
#define		PD_MARKER_fRAc		0x66524163	// 7 Mar 95[1] PNGEXT 1.2.0
#define		PD_MARKER_gIFg		0x67494667	// 7 Mar 95    PNGEXT 1.2.0
#define		PD_MARKER_gIFt		0x67494674	// 7 Mar 95[2] PNGEXT 1.2.0
#define		PD_MARKER_gIFx		0x67494678	// 7 Mar 95    PNGEXT 1.2.0
#define		PD_MARKER_oFFs		0x6F464673	// 7 Mar 95    PNGEXT 1.2.0
#define		PD_MARKER_pCAL		0x7043414C	//28 Jan 97    PNGEXT 1.2.0
#define		PD_MARKER_sCAL		0x7343414C	// 7 Mar 95    PNGEXT 1.2.0

#define		PD_MARKER_sTER		0x73544552	//15 Apr 06    PNGEXT 1.3.0 (30 August 2006)
#endif


//FLEVEL in ZLIB : Compression Level
#define		PD_FLEVEL_FASTEST		0
#define		PD_FLEVEL_FAST		1
#define		PD_FLEVEL_DEFAULT		2
#define		PD_FLEVEL_SLOWEST		3

//Color Types Definitions
#define		PD_COLOR_GREY			0
#define		PD_COLOR_TRUE			2
#define		PD_COLOR_INDEX		3
#define		PD_COLOR_GREY_ALPHA		4
#define		PD_COLOR_TRUE_ALPHA		6

//Filter Types Definitions
#define		PD_FILT_NONE			0
#define		PD_FILT_SUB			1
#define		PD_FILT_UP			2
#define		PD_FILT_AVR			3
#define		PD_FILT_PAETH			4

//Interlace Method Definitions
#define		PD_INTERLACE_NONE		0
#define		PD_INTERLACE_ADAM		1

//Deflate Type Definitions
#define		PD_DEFLATE_NOCOMP		0
#define		PD_DEFLATE_FIXHUFF		1
#define		PD_DEFLATE_VARHUFF		2

//Job Specification
#define		PD_JOB_BUILD_FIXHUFF		0
#define		PD_JOB_BUILD_VARHUFF		1
#define		PD_JOB_DECODE_COPY			2
#define		PD_JOB_DECODE_BLOCK			3
#define		PD_JOB_DECODE_HEADER		4
#define		PD_JOB_DECODE_BLOCK_HEADER	5
#define		PD_JOB_DECODE_IMAGE			6
#define		PD_JOB_DECODE_IMAGE_RESIZE	7
#define		PD_JOB_SEARCH_IDAT			8
#define		PD_JOB_DECODE_INIT			9

//Huffman Table Specification
#define		PD_HUFF_HASH_MAX_SIZE	1440
#define		PD_HUFF_BIT_MAX			16
#define		PD_HUFF_CODE_MAX		288

//Ancilary Definitions

#define		PD_COMP_NUM				3
#define		PD_DONE_YET				0
#define		PD_DONE_ALREADY			1
#define		PD_IHDR_CHUNK_SIZE		13
#define		PD_RING_QUEUE_MASK		0x00007FFF //Mask for 32Kb

#define		PD_INPUTBUF_SIZE		(2048)											//  2048 bytes
#define		PD_INPUTBUF_SIZE2		(PD_INPUTBUF_SIZE * 2)							//  4096 bytes
#define		PD_PLTE_TABLE_IDX		(256)
#define		PD_PLTE_TABLE_SIZE		(PD_PLTE_TABLE_STRUCT_SIZE * PD_PLTE_TABLE_IDX)	//  1024 bytes500
#define		PD_DEFLATE_BUF_LEN		(32768)											// 32768 bytes
#define		PD_HASH_HEAP_SIZE		(PD_HUFF_HASH_MAX_SIZE * 8)	/* == sizeof(huft) 
																	PD_HUFF_HASH_MAX_SIZE 1000 =>  8000 bytes
																	PD_HUFF_HASH_MAX_SIZE 1440 => 11520 bytes(+3520bytes, 20081008)
																*/
#define		PD_INSTANCE_BUF_SIZE	(PD_INPUTBUF_SIZE2 + PD_PLTE_TABLE_SIZE + PD_DEFLATE_BUF_LEN + PD_HASH_HEAP_SIZE)
									//49412 bytes ( = 4096 + 1024 + 32768 + 11520 + 4 /* for align. */)


/*******************************************************************/
/************************Functions Predefines**************************/
/*******************************************************************/


/*******************************************************************/
/***************************RO Variables*****************************/
/*******************************************************************/
// # const : 364 bytes

static const uint32 PDRO_Bit_Mask[17] = {	0x00000000, 0x00000001, 0x00000003, 0x00000007, 
											0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F, 
											0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
											0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF, 
											0x0000FFFF };

//Order of the bit length code lengths
static const uint8 PDRO_Length_Order[20]={	16, 17, 18,  0,  8,  7,  9,  6, 
											10,  5, 11,  4, 12,  3, 13,  2, 
											14,  1, 15,  0/*dummy*/ };

//Copy lengths for literal codes 257..285
static const uint16 PDRO_Liter_Len[32] = {	  3,   4,   5,   6,   7,   8,   9,  10, 
											 11,  13,  15,  17,  19,  23,  27,  31, 
											 35,  43,  51,  59,  67,  83,  99, 115, 
											131, 163, 195, 227, 258,   0,   0,   0/*dummy*/ };

//Extra bits for literal codes 257..285 //99==invalid
static const uint16 PDRO_Liter_Ext[32] = {	0, 0, 0, 0, 0,  0,  0, 0, 
											1, 1, 1, 1, 2,  2,  2, 2, 
											3, 3, 3, 3, 4,  4,  4, 4, 
											5, 5, 5, 5, 0, 99, 99, 0/*dummy*/  };

//Copy offsets for distance codes 0..29(Note: see note #13 above about the 258 in this list.)
static const uint16 PDRO_Dist_Len[30]={	   1,    2,    3,     4,     5,     7,    9,   13, 
										  17,   25,   33,    49,    65,    97,  129,  193, 
										 257,  385,  513,   769,  1025,  1537, 2049, 3073,
										4097, 6145, 8193, 12289, 16385, 24577 };

//Extra bits for distance codes
static const uint16 PDRO_Dist_Ext[30] = {	 0,  0,  0,  0,  1,  1,  2,  2, 
											 3,  3,  4,  4,  5,  5,  6,  6, 
											 7,  7,  8,  8,  9,  9, 10, 10, 
											11, 11, 12, 12, 13, 13 };

//Masks for different Bit Depth Decoding
static const uint16 PDRO_Depth_Mask[16] = {	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
					 						0xC0, 0x30, 0x0C, 0x03,	0xF0, 0x0F,	0xFF, 0x00/*dummy*/ };

static const uint16 PDRO_Depth_Shift[16] = {	7, 6, 5, 4, 3, 2, 1, 0,
												6, 4, 2, 0,	4, 0, 0, 0/*dummy*/ };

//RO data for ADAM7 Interlaced Mode
static const uint8 PDRO_Hor_Start[8] = {0, 4, 0, 2, 0, 1, 0, 0/*dummy*/};
static const uint8 PDRO_Ver_Start[8] = {0, 0, 4, 0, 2, 0, 1, 0/*dummy*/};
static const uint8 PDRO_Hor_Incre[8] = {8, 8, 4, 4, 2, 2, 1, 0/*dummy*/};
static const uint8 PDRO_Ver_Incre[8] = {8, 8, 8, 4, 4, 2, 2, 0/*dummy*/};


/*******************************************************************/
/******************************Variables******************************/
/*******************************************************************/

#if defined(PNGDEC_CHECK_CHUNK)
volatile int32	PD_nPngDecCheck_Chunk;
#endif

#if defined(PNGDEC_STABILITY_ERROR_HANDLE)
static int32	PD_nPngDecErrorCode;
#endif

// Variables for PC Version
static uint32		PCD_Global_Pix_Pos;

//IO_Related
static uint32		PD_2nd_Strm;
static int16		PD_Valid_Bit; //c
static int16		PD_Cur_Buf;
static PD_CALLBACKS	PD_callbacks;
static void *		PD_Datasource;
static BYTE *		PD_FileBuf_Ptr;
static int32		PD_Read_Point;

#if defined(PNGDEC_CHECK_EOF_2)
static uint32		PD_TotFileSize;
static uint32		PD_ReadFileBytes;
static uint32		PD_Read_Point_Max;
#endif

#if defined(PNGDEC_OPTI_INSTANCE_MEM)
static BYTE *		PD_File_Buf;		//[PD_INPUTBUF_SIZE2]; //4096 bytes
#else
static BYTE		PD_File_Buf[PD_INPUTBUF_SIZE2];
#endif

//Output_Related
static PD_CUSTOM_DECODE	PD_Out_Struct;			//Structure for Output Image Formatting
static uint32		PD_LCD_Width;
static uint32		PD_LCD_Height;
static uint16 *		PD_Pixel_Map_Hor;
static uint16 *		PD_Pixel_Map_Ver;
static uint16		PD_Top_Offset;				//Distance from the top of LCD
static uint16		PD_Left_Offset;				//Distance from the left of LCD
static uint16		PD_Resized_Width;			//Image Width resized according to LCD
static uint16		PD_Resized_Height;			//Image Height resized according to LCD
static uint8		PD_Image_Smaller_LCD;		//Image is smaller than LCD when this is set
static uint8		PD_Alpha_Available;			//If this field is set, Use of alpha data is allowed.
static uint8		PD_Alpha_Use;				//If this field is set, output alpha data.

//ADAM7 Interlaced Mode Related
static uint8		PD_Current_Pass;
static uint16		PD_ADAM7_Width;				//Image Width of Each Resized Pass
static uint16		PD_ADAM7_Height;			//Image Height of Each Resized Pass
static uint16		PD_Remaining_Row;			//Image Row to be decoded
static uint16		PD_Remaining_Row_ADAM7;		//Image Row to be decoded by interlaced mode

//Header Parsing Related
static uint32		PD_Chunk_Size;
static uint32		PD_Used_Byte;

//Image Information
static uint32		PD_Global_Width;			//Image Width
static uint32		PD_Global_Height;			//Image Height
static uint8		PD_Bit_Depth;
static uint8		PD_Compo_Num;
static uint8		PD_Color_Type;
static uint8		PD_Compression_Method;
static uint16		PD_Filter_Method;		//c
static uint8		PD_Interlace_Method;
static uint8		PD_Bpp;						//The number of bits for one pixel

//Pallete Related
#if defined(PNGDEC_OPTI_INSTANCE_MEM)
static PD_PLTE_TABLE *	PD_Plte;
#else
static PD_PLTE_TABLE	PD_Plte[PD_PLTE_TABLE_IDX];			//1024 bytes
#endif
static uint32		PD_Plte_Entry_Num;

//Decoding Related
static Decode_Func_Ptr	PNG_Decode_Image;		//Function Pointer for Variation of Bit depth
static uint16		PD_Len2Copy;				//To continue copy after image decoding
static uint16		PD_Dist2Copy;				//Ditto
static uint8		PD_Still_Decoding;			//Ditto
static uint8		PD_Last_IDAT;				//Indicates that there is no more IDAT Chunk
static uint8		PD_Cur_Job;					//Save Next Procedure among Decoding Routine
static uint8		PD_Prev_Job;				//Save previous Procedure among Decoding Routine
static uint16		PD_Window_Size;				//The size of window used for refering (MAX == 32KB)
static uint8		PD_ZLIB_Flevel;				//Degree of Compression in ZLIB structure
static uint8		PD_Last_Block;				//Indicates whether current block is the last block or not
static uint32		PD_Ptr_Block_Dec;			//Filled Bytes in Deflate buffer by BLOCK decoding
static uint32		PD_Ptr_Image_Dec;			//Used bytes in Deflate Buffer for Image decoding 
#if defined(PNGDEC_OPTI_INSTANCE_MEM)
static uint8 *		PD_Deflate_Buf;	//32KB Deflate Buffer for Literal Refering
#else
static uint8		PD_Deflate_Buf[PD_DEFLATE_BUF_LEN];	//32KB Deflate Buffer for Literal Refering
#endif
static uint8 *		PD_Up_Scanline;				//Upper Scanline for Filtering
static uint8 *		PD_Diag_Scanline;			//Diagonal Scanline for Filtering
static uint16		PD_Scanline_Size;			//The number of bytes for one Scanline
static uint16		PD_Global_Scanline_Size;		//The number of bytes for one Scanline
static uint16		PD_Deflate_Type;//c			//0 : copy, 1 : Fixed huffman, 2 : Dynamic Huffman
static uint16		PD_Scaler;					//Bit Depth Scaling
static uint16 * 	PD_Data_Mask;				//Mask for bpp
static uint16 * 	PD_Data_Shift;				//Mask for bpp

//Huffman Table Related
#if defined(PNGDEC_OPTI_INSTANCE_MEM)
static uint8 *		PD_Heap;	//(20081008:11520 bytes) from 8000 bytes
#else
static uint8		PD_Heap[PD_HASH_HEAP_SIZE];	//(20081008:11520 bytes) from 8000 bytes
#endif
static unsigned long		PD_Heap_Ptr;				//Indicates current position of remaining heap memory
static uint32		PD_Heap_Used_Size;			//Used Heap Size for Huffman Table
static uint32		PD_Hash_Size;				//The number of used "Huffman Table Structure"
static huft *		PD_Huff_Liter;				//Literal or Length Huffman Table
static huft *		PD_Huff_Dist;				//Distance Huffman Table
static uint32		PD_FixHuff_Done;	//c		//Whether Fixed Huffman Table is already built up or not
static int			PD_Lookup_Bit_Literal;			//Minimum Look-up Bit for Literal or Length Huffman Table
static int			PD_Lookup_Bit_Distance;			//Minimum Look-up Bit for Distance Huffman Table

//Temporary Variable
static uint32		PD_Row;
static uint32		PD_Resize_Ver_Idx;


#if defined(PNGDEC_OPTI_INSTANCE_MEM)
//////////////////////
//Instance buffer allocation
//////////////////////
static int PNG_Init_instanceMem(char * pAddr, unsigned int iLength)
{
	unsigned long iAddr;

	iAddr = (unsigned long) pAddr;

	PD_File_Buf = NULL;
	PD_Plte = NULL;
	PD_Deflate_Buf = NULL;
	PD_Heap = NULL;

	if( (pAddr == NULL) || (iLength < PD_INSTANCE_BUF_SIZE) ) {
		return PD_INSTANCE_BUF_SIZE;
	}

	if( iAddr & 0x3 ) { 
		iLength -= (4- (iAddr&0x3) );
		iAddr = ( ( (iAddr>>2) + 1 ) << 2 );		
	}

	PD_File_Buf = (unsigned char *)iAddr;;
	iAddr += PD_INPUTBUF_SIZE2;
	PD_Plte = (PD_PLTE_TABLE *)iAddr;
	iAddr += PD_PLTE_TABLE_SIZE;
	PD_Deflate_Buf = (unsigned char *)iAddr;
	iAddr += PD_DEFLATE_BUF_LEN;
	PD_Heap = (unsigned char *)iAddr;

	return 0;	//success
}
#endif

//////////////////////
//Input Related Functions
//////////////////////

static uint8 _read_byte(void)
{
	BYTE ret;

	if (PD_Read_Point == PD_INPUTBUF_SIZE)
	{
		int tRet = 0;
	#if defined(PNGDEC_CHECK_EOF_2)
		int iReadBytes;
	#endif
	
		PD_Read_Point=0;

		if(PD_Cur_Buf==1)
		{
			PD_FileBuf_Ptr = PD_File_Buf;

		#if defined(PNGDEC_CHECK_EOF_2)
			iReadBytes = PD_INPUTBUF_SIZE;
			if( (PD_ReadFileBytes+PD_INPUTBUF_SIZE) > PD_TotFileSize )
			{
				iReadBytes = PD_TotFileSize - PD_ReadFileBytes;
				if( iReadBytes > PD_INPUTBUF_SIZE ) {
					iReadBytes = PD_INPUTBUF_SIZE;
				}
			}
			if( iReadBytes > 0 ) {	
				PD_Read_Point_Max = iReadBytes + PD_INPUTBUF_SIZE;
				tRet = (PD_callbacks.read_func)( \
							&PD_File_Buf[PD_INPUTBUF_SIZE], \
							1, iReadBytes, \
							PD_Datasource);
				if( tRet < PNGDEC_FREAD_FAILURE_RET ) {
					PD_nPngDecErrorCode = PD_RETURN_DECODE_FAIL;
					return 0;
				}else
				{
					PD_ReadFileBytes += iReadBytes;
				}
			}else
			{
				if( PD_Read_Point_Max > PD_INPUTBUF_SIZE )
				{
					PD_Read_Point_Max -= PD_INPUTBUF_SIZE;
				}
				else
				{
					PD_nPngDecErrorCode = TC_PNGDEC_ERR_STREAM_READING;
					return 0;
				}
			}
		#else
			tRet = (PD_callbacks.read_func)( \
							&PD_File_Buf[PD_INPUTBUF_SIZE], \
							1, PD_INPUTBUF_SIZE, \
							PD_Datasource);

			if( tRet < PNGDEC_FREAD_FAILURE_RET ) {
				PD_nPngDecErrorCode = TC_PNGDEC_ERR_STREAM_READING;
				return 0;
			}
		#endif

			PD_Cur_Buf=0;
		}else
		{
			PD_FileBuf_Ptr = &PD_File_Buf[PD_INPUTBUF_SIZE];

		#if defined(PNGDEC_CHECK_EOF_2)
			iReadBytes = PD_INPUTBUF_SIZE;
			if( (PD_ReadFileBytes+PD_INPUTBUF_SIZE) > PD_TotFileSize )
			{
				iReadBytes = PD_TotFileSize - PD_ReadFileBytes;
				if( iReadBytes > PD_INPUTBUF_SIZE )
				{
					iReadBytes = PD_INPUTBUF_SIZE;
				}
			}
			if( iReadBytes > 0 )
			{	
				PD_Read_Point_Max = iReadBytes + PD_INPUTBUF_SIZE;
				tRet = (PD_callbacks.read_func)( \
							PD_File_Buf, \
							1, iReadBytes, \
							PD_Datasource);
				if( tRet < PNGDEC_FREAD_FAILURE_RET ) {
					PD_nPngDecErrorCode = PD_RETURN_DECODE_FAIL;
					return 0;
				}else {
					PD_ReadFileBytes += iReadBytes;
				}
			}else
			{
				if( PD_Read_Point_Max > PD_INPUTBUF_SIZE )
				{
					PD_Read_Point_Max -= PD_INPUTBUF_SIZE;
				}
				else
				{
					PD_nPngDecErrorCode = TC_PNGDEC_ERR_STREAM_READING;
					return 0;
				}
			}
		#else
			tRet = (PD_callbacks.read_func)( \
						PD_File_Buf, \
						1, PD_INPUTBUF_SIZE, \
						PD_Datasource);
			if( tRet < PNGDEC_FREAD_FAILURE_RET ) {
				PD_nPngDecErrorCode = TC_PNGDEC_ERR_STREAM_READING;
				return 0;
			}
		#endif

			PD_Cur_Buf=1;
		}
	}

#if defined(PNGDEC_CHECK_EOF_2)
	if( PD_Read_Point > PD_Read_Point_Max )
	{
		PD_nPngDecErrorCode = TC_PNGDEC_ERR_STREAM_READING;
		return 0;
	}
	ret=PD_FileBuf_Ptr[PD_Read_Point++];
#else
	ret=PD_FileBuf_Ptr[PD_Read_Point++];
#endif

	return ret;
}


static void PNG_Init_IO(void)
{

#if defined(PNGDEC_CHECK_EOF_2)
	int iReadBytes;

	iReadBytes = PD_INPUTBUF_SIZE;
	if( PD_TotFileSize < PD_INPUTBUF_SIZE )
		iReadBytes = PD_TotFileSize;
	(PD_callbacks.read_func)(PD_File_Buf, 1, iReadBytes, PD_Datasource);
	PD_ReadFileBytes = iReadBytes;
	PD_Read_Point_Max = iReadBytes;

	if( PD_TotFileSize > PD_ReadFileBytes )
	{
		iReadBytes = PD_TotFileSize - PD_ReadFileBytes;
		if( iReadBytes > PD_INPUTBUF_SIZE )
			iReadBytes = PD_INPUTBUF_SIZE;
		(PD_callbacks.read_func)(&PD_File_Buf[PD_INPUTBUF_SIZE], 1, iReadBytes, PD_Datasource);
		PD_ReadFileBytes += iReadBytes;
		PD_Read_Point_Max += iReadBytes;
	}	

#else
	(PD_callbacks.read_func)(PD_File_Buf, PD_INPUTBUF_SIZE, 1, PD_Datasource);
	(PD_callbacks.read_func)(&PD_File_Buf[PD_INPUTBUF_SIZE], PD_INPUTBUF_SIZE, 1, PD_Datasource);
#endif

	PD_Read_Point = 0;
	PD_Cur_Buf = 0;
	PD_FileBuf_Ptr = PD_File_Buf;

	PD_Valid_Bit = 0;
	PD_2nd_Strm = 0;
}


/*******************************************************************/
/*********************Functional Macro Defines**************************/
/*******************************************************************/
//Bitstream Related Macro Functions
#define NEEDBITS(bits)						\
{								\
	while(PD_Valid_Bit < bits)					\
	{							\
		PD_2nd_Strm += _read_byte() << PD_Valid_Bit;	\
		PD_Valid_Bit += 8;				\
	}							\
}

#define READBITS(bits, temp)					\
{								\
	temp = (uint16)(PD_2nd_Strm & PDRO_Bit_Mask[bits]);	\
	DROPBITS(bits);						\
}

#define SHOWBITS(bits, temp)					\
{								\
	temp = (uint16)(PD_2nd_Strm & PDRO_Bit_Mask[bits]);	\
}

#define DROPBITS(bits)			\
{					\
	PD_2nd_Strm >>= bits;		\
	PD_Valid_Bit -= bits;		\
}

#define READWORD(v)			\
{					\
	int temp;				\
	NEEDBITS(8);			\
	READBITS(8, v);			\
	NEEDBITS(8);			\
	READBITS(8, temp);		\
	v = (v << 8 | temp);		\
	NEEDBITS(8);			\
	READBITS(8, temp);		\
	v = (v << 8 | temp);		\
	NEEDBITS(8);			\
	READBITS(8, temp);		\
	v = (v << 8 | temp);		\
}

#define READBYTE(v)			\
{					\
	NEEDBITS(8);			\
	READBITS(8, v);			\
}

#define SKIPWORD()			\
{					\
	NEEDBITS(16);			\
	DROPBITS(16);			\
	NEEDBITS(16);			\
	DROPBITS(16);			\
}

#define SKIPHWORD()			\
{					\
	NEEDBITS(16);			\
	DROPBITS(16);			\
}

#define SKIPBYTE()			\
{					\
	NEEDBITS(8);			\
	DROPBITS(8);			\
}

//Ring-Queue Related Macro Functions
#define QUEUE_PUSH(v)	\
{\
	PD_Deflate_Buf[PD_RING_QUEUE_MASK & (PD_Ptr_Block_Dec++)] = v;\
}

#if defined(PNGDEC_OPT_CHECK_DEFLATE_BUF_SIZE)
#define QUEUE_POP_NOMASK(v)	\
{\
	v = PD_Deflate_Buf[(PD_Ptr_Image_Dec++)];\
}
#endif

#define QUEUE_POP(v)	\
{\
	v = PD_Deflate_Buf[PD_RING_QUEUE_MASK & (PD_Ptr_Image_Dec++)];\
}

#define QUEUE_PUSH_CHECK(size)	\
{\
	size = PD_DEFLATE_BUF_LEN - (PD_Ptr_Block_Dec - PD_Ptr_Image_Dec);\
}

#define QUEUE_POP_CHECK(size)	\
{\
	size = PD_Ptr_Block_Dec - PD_Ptr_Image_Dec;\
}

#define QUEUE_VISIT(v, d)	\
{\
	v = PD_Deflate_Buf[(PD_Ptr_Block_Dec - d) & PD_RING_QUEUE_MASK];\
}

#if !defined(PNGDEC_ABS_INTERNAL)
#define DE_PAETH(a, b, c, v)		\
{					\
	int pa = abs(b - c);		\
	int pb = abs(a - c);		\
	int pc = abs(a + b - 2 * c);		\
	if(pa <= pb && pa <= pc)		\
		v = a;			\
	else				\
	{				\
		if(pb <= pc)		\
			v = b;		\
		else			\
			v = c;		\
	}				\
}
#else //!defined(PNGDEC_ABS_INTERNAL)
#define DE_PAETH(a, b, c, v)		\
{					\
	int pa = PNGD_ABS(b - c);		\
	int pb = PNGD_ABS(a - c);		\
	int pc = PNGD_ABS(a + b - 2 * c);		\
	if(pa <= pb && pa <= pc)		\
		v = a;			\
	else				\
	{				\
		if(pb <= pc)		\
			v = b;		\
		else			\
			v = c;		\
	}				\
}
#endif //!defined(PNGDEC_ABS_INTERNAL)


#if !defined(PNGDEC_MEMSET_INTERNAL)
#define PNGD_MEMSET(X,Y,Z)	memset(X,Y,Z)
#else //!defined(PNGDEC_MEMSET_INTERNAL)
/*	void *memset(void*, int, size_t)
	constraints
	- int(value) : ragne of 0 to 255
	- size_t : int type

*/
#define PNGD_MEMSET(X,Y,Z)								\
{														\
	char *pAddr = (char*)X;								\
	char cVal = (char)Y;								\
	unsigned int iSize = (unsigned int)Z;				\
	if( iSize < 8 )										\
	{													\
		unsigned int i;									\
		for( i=0; i<iSize; i++)							\
		{												\
			pAddr[i] = cVal;							\
		}												\
	}else												\
	{													\
		int i, iSize4;									\
		int	itmp;										\
		unsigned long iAddr;							\
		unsigned int iVal;								\
		unsigned int *pAddr4;							\
		iAddr = (unsigned long)pAddr;					\
		/*4-byte align. for writing in 1 bytes */		\
		itmp = (int)(iAddr & 0x3);						\
		if( itmp )										\
		{												\
			itmp = 4 - itmp;							\
			for(i=0; i<itmp; i++)						\
			{											\
				*pAddr++ = cVal;						\
			}											\
			iSize -= itmp;								\
			iAddr = (unsigned long)pAddr;				\
		}												\
		pAddr4 = (unsigned int*)pAddr;					\
		/* Writing in 4 bytes */						\
		iSize4 = (iSize>>2);							\
		iVal = (unsigned int)cVal;						\
		iVal = (cVal<<24)|(cVal<<16)|(cVal<<8)|(iVal);	\
		for(i=0;i<iSize4;i++)							\
		{												\
			*pAddr4++ = iVal;							\
		}												\
		/* Writing in 1 bytes for remained data */		\
		iSize -= (iSize4<<2);							\
		pAddr += (iSize4<<2);							\
		if( iSize )										\
		{												\
			for(i=0; i<iSize; i++ )						\
			{											\
				*pAddr++ = cVal;						\
			}											\
		}												\
	}													\
}
#endif //!defined(PNGDEC_MEMSET_INTERNAL)


/*******************************************************************/
/*************************Functions Defines****************************/
/*******************************************************************/

//////////////////////
//Initialization Related
//////////////////////
static void PNG_Init_Variable(void)
{
	PD_FixHuff_Done = PD_DONE_YET;
	PD_Last_IDAT = PD_DONE_YET;
	PCD_Global_Pix_Pos = 0;	
	PD_Ptr_Block_Dec = 0;
	PD_Ptr_Image_Dec = 0;
#if defined(PNGDEC_CHECK_CHUNK)
	PD_nPngDecCheck_Chunk = 0;
#endif	
	PD_Still_Decoding = PD_DONE_ALREADY;
	PD_Hash_Size = 0;
	PD_Heap_Ptr = PD_Heap;
	PD_Heap_Used_Size = 0;
	PD_Last_Block = 0;
	PD_Row = 0;
	PD_Resize_Ver_Idx = 0;
	PD_Current_Pass = 0;
	PD_Remaining_Row = 0;
	PD_Alpha_Available = 0;
#if defined(PNGDEC_STABILITY_ERROR_HANDLE)
	PD_nPngDecErrorCode = 0; //init.
#endif
}

//Generation of Resized Pixel Map
static int PNG_Init_ADAM7_Map(int pass)
{
	if(PD_Current_Pass != pass)
	{
		PD_Row = 0;
		PD_Current_Pass = pass;
		PD_Resize_Ver_Idx = 0;
		PNGD_MEMSET(PD_Diag_Scanline, 0, PD_Global_Scanline_Size);
		
		switch(PD_Current_Pass)
		{
		case 1:
			PD_ADAM7_Width = (PD_Global_Width + 7) / 8;
			PD_ADAM7_Height = (PD_Global_Height + 7) / 8;
			break;
		case 2:
			PD_ADAM7_Width = (PD_Global_Width + 3) / 8;
			PD_ADAM7_Height = (PD_Global_Height + 7) / 8;
			break;
		case 3:
			PD_ADAM7_Width = (PD_Global_Width + 7) / 8 + (PD_Global_Width + 3) / 8;
			PD_ADAM7_Height = (PD_Global_Height + 3) / 8;
			break;
		case 4:
			PD_ADAM7_Width = (PD_Global_Width + 1) / 8 + (PD_Global_Width + 5) / 8;
			PD_ADAM7_Height = (PD_Global_Height + 7) / 8 + (PD_Global_Height + 3) / 8;
			break;
		case 5:
			PD_ADAM7_Width = (PD_Global_Width + 1) / 2;
			PD_ADAM7_Height = (PD_Global_Height + 1) / 8 + (PD_Global_Height + 5) / 8;
			break;
		case 6:
			PD_ADAM7_Width = PD_Global_Width / 2;
			PD_ADAM7_Height = (PD_Global_Height + 1) / 2;
			break;
		case 7:
			PD_ADAM7_Width = PD_Global_Width;
			PD_ADAM7_Height = PD_Global_Height / 2;
			break;
		default:
			return PD_PROCESS_ERROR;
		}

		if(PD_ADAM7_Width == 0 || PD_ADAM7_Height == 0)//If pass image is NULL, skip current pass
			PNG_Init_ADAM7_Map(PD_Current_Pass + 1);

		PD_Remaining_Row = PD_ADAM7_Height;
		PD_Remaining_Row_ADAM7 = 0;
		PD_Scanline_Size = ((PD_ADAM7_Width * PD_Bit_Depth * PD_Compo_Num - 1) >> 3) + 1;
	}
	return PD_PROCESS_DONE;
}

//Initialize Heap Memory and Scaler Factors

static void PNG_Init_Heap(void)
{
	int i;
	uint32 hor_ratio;
	uint32 ver_ratio;
	
	//Memory for Upper Scanline
	PD_Diag_Scanline = (uint8 *)(PD_Out_Struct.Heap_Memory);
	PD_Up_Scanline = PD_Diag_Scanline + PD_Bpp;

	//Set Resizing Factor
	if(PD_Out_Struct.MODIFY_IMAGE_POS)
	{
		PD_Left_Offset = PD_Out_Struct.IMAGE_POS_X;
		PD_Top_Offset = PD_Out_Struct.IMAGE_POS_Y;
	}
	else
	{
		PD_Left_Offset = (PD_LCD_Width - PD_Resized_Width) >> 1;
		PD_Top_Offset = (PD_LCD_Height - PD_Resized_Height) >> 1;
	}

	if(PD_Image_Smaller_LCD != PD_TRUE)
	{
		//Memory for Resizing Matrix
	#if !defined(PNGDEC_MOD_MEM_ALIGN)
		PD_Pixel_Map_Hor = (uint16 *)((uint32)PD_Up_Scanline + PD_Scanline_Size + PD_Bpp);
		PD_Pixel_Map_Ver = (uint16 *)((uint32)PD_Pixel_Map_Hor + PD_Resized_Width * 2);
	#else
		PD_Pixel_Map_Hor = (uint16 *)((((uint32)PD_Up_Scanline + PD_Scanline_Size + PD_Bpp + 3)>>1)<<1);
		PD_Pixel_Map_Ver = (uint16 *)((((uint32)PD_Pixel_Map_Hor + PD_Resized_Width * 2 + 3)>>1)<<1);
	#endif

		hor_ratio = (PD_Global_Width << 16) / PD_Resized_Width;
		ver_ratio = (PD_Global_Height << 16) / PD_Resized_Height;

		for(i = 0;i < PD_Resized_Width;i++) {
			PD_Pixel_Map_Hor[i] = (uint16)((i * hor_ratio) >> 16);
		}

		for(i = 0;i < PD_Resized_Height;i++) {
			PD_Pixel_Map_Ver[i] = (uint16)((i * ver_ratio) >> 16);	
		}
	}

	if(PD_Interlace_Method == PD_INTERLACE_ADAM)
		PNG_Init_ADAM7_Map(PD_Current_Pass + 1);
}

//////////////////////
//Error Resilience Related
//////////////////////
static int PNG_Check_CRC(void)
{
	_read_byte();
	_read_byte();
	_read_byte();
	_read_byte();

	if( PD_nPngDecErrorCode < 0 ) {
		return PD_PROCESS_ERROR;
	}

	return PD_PROCESS_DONE;
}


static int PNG_Skip_Current_Chunk(void)
{
	unsigned int i;

	while(PD_Chunk_Size >= 512)
	{
		for(i = 0;i < (512 >> 2);i++)	
		{
			_read_byte();
			_read_byte();
			_read_byte();
			_read_byte();

		#if defined(PNGDEC_CHECK_EOF)
			if( PD_nPngDecErrorCode < 0 ) {
				return PD_PROCESS_ERROR;
			}
		#endif
		}
			
		PD_Chunk_Size -= 512;
	}
	for(i=0;i<PD_Chunk_Size;i++) {
		_read_byte();
		#if defined(PNGDEC_CHECK_EOF)
			if( PD_nPngDecErrorCode < 0 ) {
				return PD_PROCESS_ERROR;
			}
		#endif
	}

	return PNG_Check_CRC();
}


static int PNG_Parse_tRNS_Chunk(void)
{
	unsigned int i;
	for(i = 0;i < PD_Chunk_Size;i++)
		READBYTE(PD_Plte[i].Alpha);
	for(;i < 256;i++)
		PD_Plte[i].Alpha = 255;

	PD_Alpha_Available = 1;
	return PNG_Check_CRC();
}


static int PNG_Search_IDAT_Chunk(unsigned int mode)
{
	int iteration = 50;
	uint32 stream_out;

	PD_Used_Byte = 0;
	
	while(iteration--)
	{
		if(mode)
		{
			if(PD_Valid_Bit >= 8)
			{
				PD_Used_Byte--;
				PD_Valid_Bit -= 8;
				PD_Chunk_Size = (PD_2nd_Strm & (0xFF << PD_Valid_Bit)) >> PD_Valid_Bit;
				PD_2nd_Strm = PD_2nd_Strm & PDRO_Bit_Mask[PD_Valid_Bit];
			}else
			{
				PD_Chunk_Size = _read_byte();
			}
			
			if(PD_Valid_Bit >= 8)
			{
				PD_Used_Byte--;
				PD_Valid_Bit -= 8;
				PD_Chunk_Size = (PD_Chunk_Size << 8) | ((PD_2nd_Strm & (0xFF << PD_Valid_Bit)) >> PD_Valid_Bit);
				PD_2nd_Strm = PD_2nd_Strm & PDRO_Bit_Mask[PD_Valid_Bit];
			}else
			{
				PD_Chunk_Size = (PD_Chunk_Size << 8) | _read_byte();
			}
			
			if(PD_Valid_Bit >= 8)
			{
				PD_Used_Byte--;
				PD_Valid_Bit -= 8;
				PD_Chunk_Size = (PD_Chunk_Size << 8) | ((PD_2nd_Strm & (0xFF << PD_Valid_Bit)) >> PD_Valid_Bit);
				PD_2nd_Strm = PD_2nd_Strm & PDRO_Bit_Mask[PD_Valid_Bit];
			}else
			{
				PD_Chunk_Size = (PD_Chunk_Size << 8) | _read_byte();
			}
			
			if(PD_Valid_Bit >= 8)
			{
				PD_Used_Byte--;
				PD_Valid_Bit -= 8;
				PD_Chunk_Size = (PD_Chunk_Size << 8) | ((PD_2nd_Strm & (0xFF << PD_Valid_Bit)) >> PD_Valid_Bit);
				PD_2nd_Strm = PD_2nd_Strm & PDRO_Bit_Mask[PD_Valid_Bit];
			}else
			{
				PD_Chunk_Size = (PD_Chunk_Size << 8) | _read_byte();
			}
		}
		else
		{
			PD_Chunk_Size = _read_byte();
			PD_Chunk_Size = (PD_Chunk_Size << 8) | _read_byte();
			PD_Chunk_Size = (PD_Chunk_Size << 8) | _read_byte();
			PD_Chunk_Size = (PD_Chunk_Size << 8) | _read_byte();
		}

		stream_out = _read_byte();
		stream_out = (stream_out << 8) | _read_byte();
		stream_out = (stream_out << 8) | _read_byte();
		stream_out = (stream_out << 8) | _read_byte();

		if( PD_nPngDecErrorCode < 0 ) {
			return PD_PROCESS_ERROR;
		}
		
		switch(stream_out)
		{
		case PD_MARKER_cHRM:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_CHRM);
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_gAMA:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_GAMA );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_iCCP:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_ICCP );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_sBIT:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_SBIT );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_sRGB:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_SRBG );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_iTXt:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_ITXT );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_tEXt:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_TEXT );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_zTXt:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_ZTXT );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_bKGD:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_BKGD );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_hIST:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_HIST );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_pHYs:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_PHYS );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_sPLT:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_SPLT );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_tIME:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_TIME );
				PNG_Skip_Current_Chunk();
				break;
			#endif
	#if defined(PNGDEC_ADD_ANCILLARY_CHUNK_EXT120130)
		case PD_MARKER_fRAc:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_FRAC );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_gIFg:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_GIFG );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_gIFt:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_GIFT );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_gIFx:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_GIFX );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_oFFs:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_OFFS );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_pCAL:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_PCAL );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_sCAL:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_SCAL );
				PNG_Skip_Current_Chunk();
				break;
			#endif
		case PD_MARKER_sTER:			
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_STER );
				PNG_Skip_Current_Chunk();
				break;
			#endif
	#endif //defined(PNGDEC_ADD_ANCILLARY_CHUNK_EXT120130)
			PNG_Skip_Current_Chunk();
			break;
		case PD_MARKER_PLTE:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_PLTE );
			#endif
			PNG_Skip_Current_Chunk();
			break;
		case PD_MARKER_tRNS:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_TRNS );
			#endif
			PNG_Parse_tRNS_Chunk();
			break;
		case PD_MARKER_IHDR:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_IHDR );
			#endif
			return PD_PROCESS_ERROR;
		case PD_MARKER_IEND:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_IEND );
			#endif
			return PD_PROCESS_EOF;
		case PD_MARKER_IDAT:
			#if defined(PNGDEC_CHECK_CHUNK)
				PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_IDAT );
			#endif
			return PD_PROCESS_DONE;
		default:
		#if !defined(PNGDEC_STABILITY_READ_UNKNOWN_ANC_CHUNK_SKIP)
			return PD_PROCESS_ERROR;
		#else
			PNG_Skip_Current_Chunk();
			break;
		#endif
		}
	}
	return PD_PROCESS_ERROR;
}


#define NEEDBITS_IDAT(bits)						\
{									\
	while(PD_Valid_Bit < bits)						\
	{								\
		if(PD_Used_Byte++ == PD_Chunk_Size)			\
		{							\
			PNG_Check_CRC();				\
			PNG_Search_IDAT_Chunk(0);			\
			PD_Used_Byte++;				\
		}							\
		PD_2nd_Strm += _read_byte() << PD_Valid_Bit;		\
		PD_Valid_Bit += 8;					\
	}								\
}

static int PNG_Check_Adler32(void)
{
	int temp = PD_Valid_Bit & 7;

	NEEDBITS_IDAT(temp);
	DROPBITS(temp);
	
	NEEDBITS_IDAT(16);
	DROPBITS(16);
	
	NEEDBITS_IDAT(16);
	DROPBITS(16);
	
	return PD_PROCESS_DONE;
}

static void * PNG_Malloc(uint32 size)
{
	uint32 ret = PD_Heap_Ptr;
	PD_Heap_Ptr += size;
	PD_Heap_Used_Size += size;
	if(PD_Heap_Used_Size > PD_HASH_HEAP_SIZE)
	{
		return (void *)NULL;
	}
	else
	{
		return (void *)ret;
	}
}


static int PNG_BuildUp_HuffTable(uint16 * code_length, uint32 code_num, uint32 code_num_simple,
					uint16 * val_non_simple, uint16 * bit_non_simple, huft ** vld,
					int * lookup_bit)
{
	uint32	bit_leng_count, num_repeat;
	uint32	i, j;

	uint16	bit_leng_count_tbl[PD_HUFF_BIT_MAX + 1];
	uint16	bit_offsets[PD_HUFF_BIT_MAX + 1];
	uint16	bit_order_val[PD_HUFF_CODE_MAX];
	uint16	cur_num_entry;
	
	int	max_leng_code, level_tbl, cur_bit_num;
	int	bits_per_tbl, prev_bits, num_dummy;

	huft tbl_entry;
	huft *pCurTbl;	
	huft *huff_tbl_stack[PD_HUFF_BIT_MAX];
	
	uint16 *pCBV;
	uint16 *pX;
	
	PNGD_MEMSET(bit_leng_count_tbl, 0, sizeof(bit_leng_count_tbl));
	
	i = code_num;
	pCBV = code_length;
	do {
		bit_leng_count_tbl[*pCBV]++;
		pCBV++;
	} while(--i);
	
	bits_per_tbl = *lookup_bit;
	for( j = 1; j <= PD_HUFF_BIT_MAX; j++ )
	{
		if( bit_leng_count_tbl[j] )
		{
			break;
		}
	}
	cur_bit_num = j;
	if( (uint32)bits_per_tbl < j )
	{
		bits_per_tbl = j;
	}

	for( i = PD_HUFF_BIT_MAX; i; i-- )
	{
		if( bit_leng_count_tbl[i] )
		{
			break;
		}
	}

	max_leng_code = i;
	if( (uint32)bits_per_tbl > i )
	{
		bits_per_tbl = i;
	}
	*lookup_bit = bits_per_tbl;

	for( num_dummy = 1 << j; j < i; j++ )
	{
		if( (num_dummy -= bit_leng_count_tbl[j]) < 0 )
		{
			return PD_PROCESS_ERROR;
		}

		num_dummy *= 2;
	}

	if( (num_dummy -= bit_leng_count_tbl[i]) < 0 )
	{
		return PD_PROCESS_ERROR;
	}
	bit_leng_count_tbl[i] += num_dummy;

	bit_offsets[1] = 0;
	pCBV = bit_leng_count_tbl + 1;
	pX = bit_offsets + 2;
	j = 0;	
	while( --i )
	{
		*pX++ = (j += *pCBV++);
	}
	
	i = 0;
	pCBV = code_length;
	do {
		if( (j = *pCBV++) != 0 )
		{
			bit_order_val[bit_offsets[j]++] = i;
		}
	} while( ++i < code_num );

	level_tbl = -1;
	bit_offsets[0] = 0;
	huff_tbl_stack[0] = (huft *)NULL;
	pCBV = bit_order_val;	
	pCurTbl = (huft *)NULL;
	prev_bits = -bits_per_tbl;	
	for( i=0, cur_num_entry=0; cur_bit_num <= max_leng_code; cur_bit_num++ )
	{
		bit_leng_count = bit_leng_count_tbl[cur_bit_num];
		while( bit_leng_count-- )
		{
			while( cur_bit_num > prev_bits + bits_per_tbl )
			{
				level_tbl++;
				prev_bits += bits_per_tbl;
				cur_num_entry = max_leng_code - prev_bits;
				cur_num_entry = cur_num_entry > (unsigned)bits_per_tbl?bits_per_tbl:cur_num_entry;
				if( (num_repeat = 1 << (j = cur_bit_num - prev_bits)) > bit_leng_count + 1 )
				{
					num_repeat -= bit_leng_count + 1;
					pX = bit_leng_count_tbl + cur_bit_num;
					while( ++j < cur_num_entry )
					{
						if( (num_repeat <<= 1) <= *++pX )
						{
							break;
						}
						num_repeat -= *pX;
					}
				}
				cur_num_entry = 1 << j;

				pCurTbl = (huft *)PNG_Malloc((cur_num_entry + 1)*sizeof(huft));
				if( pCurTbl == (huft *)NULL )
				{
					return PD_PROCESS_ERROR;
				}

				PD_Hash_Size += cur_num_entry + 1;
				*vld = pCurTbl + 1;
				*(vld = &(pCurTbl->v.t)) = (huft *)NULL;
				huff_tbl_stack[level_tbl] = ++pCurTbl;
				if( level_tbl )
				{
					bit_offsets[level_tbl] = i;
					tbl_entry.v.t = pCurTbl;
					tbl_entry.e = (uint16)(16 + j);
					tbl_entry.b = (uint16)bits_per_tbl;					
					j = i >> (prev_bits - bits_per_tbl);
					huff_tbl_stack[level_tbl-1][j] = tbl_entry;
				}
			}

			tbl_entry.b = (uint16)(cur_bit_num - prev_bits);
			if( pCBV >= bit_order_val + code_num )
			{
				tbl_entry.e = 0x63;
			}
			else if( *pCBV < code_num_simple )
			{
				tbl_entry.v.n = (uint16)(*pCBV);
				tbl_entry.e = (uint16)(*pCBV < 0x100 ? 0x10 : 0x0F);				
				pCBV++;
			}
			else
			{
				tbl_entry.e = (unsigned short)bit_non_simple[*pCBV - code_num_simple];
				tbl_entry.v.n = val_non_simple[*pCBV++ - code_num_simple];
			}

			num_repeat = 1 << (cur_bit_num - prev_bits);
			for( j = i >> prev_bits; j < cur_num_entry; j += num_repeat )
			{
				pCurTbl[j] = tbl_entry;
			}

			for( j = 1 << (cur_bit_num - 1); i & j; j >>= 1 )
			{
				i ^= j;
			}
			i ^= j;

			while( (i & ((1 << prev_bits) - 1)) != bit_offsets[level_tbl] )
			{
				prev_bits -= bits_per_tbl;
				level_tbl--;
			}
		}
	}

	return (num_dummy != 0 && max_leng_code != 1);
}


static int PNG_Generate_FixHuff_Table(void)
{
	int i;
	int msg_ret;
	uint16 code_length[288];

	PD_Heap_Ptr = PD_Heap;
	PD_Heap_Used_Size = 0;

	for (i = 0; i < 144; i++)
		code_length[i] = 8;
	for (; i < 256; i++)
		code_length[i] = 9;
	for (; i < 280; i++)
		code_length[i] = 7;
	for (; i < 288; i++)
		code_length[i] = 8;
	
	PD_Lookup_Bit_Literal = 7;
	msg_ret = PNG_BuildUp_HuffTable(code_length, 288, 257, PDRO_Liter_Len, PDRO_Liter_Ext, &PD_Huff_Liter, &PD_Lookup_Bit_Literal);
	if(msg_ret == PD_PROCESS_ERROR || msg_ret == PD_PROCESS_CONTINUE)
		return PD_PROCESS_ERROR;

	for (i = 0; i < 30; i++)
		code_length[i] = 5;
	
	PD_Lookup_Bit_Distance = 5;
	msg_ret = PNG_BuildUp_HuffTable(code_length, 30, 0, PDRO_Dist_Len, PDRO_Dist_Ext, &PD_Huff_Dist, &PD_Lookup_Bit_Distance);
	if(msg_ret == PD_PROCESS_ERROR)
		return PD_PROCESS_ERROR;

	PD_FixHuff_Done = PD_DONE_ALREADY;
	
	return PD_PROCESS_DONE;
}

static int PNG_Generate_VarHuff_Table(void)
{
	int i;
	int j;
	int msg_ret;
	uint16 bit_length[286 + 30];
	
	uint32 num_literal;
	uint32 num_distance;
	uint32 num_bit_length;
	uint32 num_total_code;
	uint32 length;
	uint8 temp;

	huft * len_for_huffcode;
	int lookup_bit_for_len;
	huft * dists_for_huffcode;

	PD_FixHuff_Done = PD_DONE_YET;
	PD_Heap_Ptr = PD_Heap;
	PD_Heap_Used_Size = 0;
	
	NEEDBITS_IDAT(14);
	READBITS(5, temp);
	num_literal = temp + 257;
	READBITS(5, temp);
	num_distance = temp+ 1;
	READBITS(4, temp);
	num_bit_length = temp + 4;

	if(num_literal > 286 || num_distance > 30)
		return PD_PROCESS_ERROR;

	for(j = 0;j < num_bit_length;j++)
	{
		NEEDBITS_IDAT(3);
		READBITS(3, temp);
		bit_length[PDRO_Length_Order[j]] = temp;
	}
	for(;j < 19;j++)
		bit_length[PDRO_Length_Order[j]] = 0;

	lookup_bit_for_len = 7;
	msg_ret = PNG_BuildUp_HuffTable(bit_length, 19, 19, NULL, NULL, &len_for_huffcode, &lookup_bit_for_len);
	if(msg_ret == PD_PROCESS_ERROR)
		return PD_PROCESS_ERROR;

	num_total_code = num_literal + num_distance;
	length = i = 0;

	while((uint32)i < num_total_code)
	{
		NEEDBITS_IDAT(lookup_bit_for_len);
		SHOWBITS(lookup_bit_for_len, temp);
		dists_for_huffcode = len_for_huffcode + (uint32)temp;
		DROPBITS(dists_for_huffcode->b);
		j = dists_for_huffcode->v.n;
		if(j < 16)
			bit_length[i++] = length = j;
		else if(j == 16)
		{
			NEEDBITS_IDAT(2);
			READBITS(2, temp);
			j = 3 + temp;
			if((uint32)i + j > num_total_code)
				return PD_PROCESS_ERROR;
			while(j--)
				bit_length[i++] = length;
		}
		else if(j == 17)
		{
			NEEDBITS_IDAT(3);
			READBITS(3, temp);
			j = 3 + temp;
			if((uint32)i + j > num_total_code)
				return PD_PROCESS_ERROR;
			while(j--)
				bit_length[i++] = 0;
			length = 0;
		}
		else
		{
			NEEDBITS_IDAT(7);
			READBITS(7, temp);
			j = 11 + temp;
			if((uint32)i + j > num_total_code)
				return PD_PROCESS_ERROR;
			while(j--)
				bit_length[i++] = 0;
			length = 0;
		}
	}

	if( PD_nPngDecErrorCode < 0 ) {
		return PD_PROCESS_ERROR;
	}

	PD_Hash_Size = 0;
	PD_Heap_Ptr = PD_Heap;
	PD_Heap_Used_Size = 0;

	PD_Lookup_Bit_Literal = 9;
	msg_ret = PNG_BuildUp_HuffTable(bit_length, num_literal, 257, PDRO_Liter_Len, PDRO_Liter_Ext,
						&PD_Huff_Liter, &PD_Lookup_Bit_Literal);
	if(msg_ret != PD_PROCESS_DONE)
		return PD_PROCESS_ERROR;

	PD_Lookup_Bit_Distance = 6;
	msg_ret = PNG_BuildUp_HuffTable(bit_length + num_literal, num_distance, 0, PDRO_Dist_Len, PDRO_Dist_Ext,
						&PD_Huff_Dist, &PD_Lookup_Bit_Distance);
	if(msg_ret == PD_PROCESS_ERROR)
		return PD_PROCESS_ERROR;
		
	return PD_PROCESS_DONE;
}

static int PNG_Defiltering(uint16 row_size, uint32 mode)
{
#if !defined(PNGDEC_OPT_DEFILTERING)
	int i = 0, j;
	int paeth_pred;
	uint8 data;
	
	if( (mode == PD_Absolute_Skip) ||
		(mode == PD_Conditional_Skip) && 
		((PD_Deflate_Buf[PD_RING_QUEUE_MASK & (PD_Ptr_Image_Dec + row_size + 1)] == PD_FILT_NONE) || 
		 (PD_Deflate_Buf[PD_RING_QUEUE_MASK & (PD_Ptr_Image_Dec + row_size + 1)] == PD_FILT_SUB ))
		)
	{
		PD_Ptr_Image_Dec += (row_size + 1);
	}
	else
	{
		QUEUE_POP(PD_Filter_Method);

		switch(PD_Filter_Method)
		{
		case PD_FILT_NONE:
			for(i = 0;i < row_size;i++)
			{
				QUEUE_POP(data);
				PD_Up_Scanline[i] = data;
			}
			break;
		case PD_FILT_UP:
			for(i = 0;i < row_size;i++)
			{
				QUEUE_POP(data);
				PD_Up_Scanline[i] = ((int)data + (int)(PD_Up_Scanline[i])) & 0xFF;
			}
			break;
		case PD_FILT_SUB:
			for(i = 0;i < row_size;i++)
			{
				QUEUE_POP(data);
				PD_Up_Scanline[i] = ((int)data + (int)(PD_Up_Scanline[i - PD_Bpp])) & 0xFF;
			}
			break;
		case PD_FILT_AVR:
			for(i = 0;i < row_size;i++)
			{
				QUEUE_POP(data);
				PD_Up_Scanline[i] = ((int)data + (((int)(PD_Up_Scanline[i]) + (int)(PD_Up_Scanline[i - PD_Bpp])) >> 1)) & 0xFF;
			}
			break;
		case PD_FILT_PAETH:
			{
				int iteration = row_size / PD_Bpp - 1;
				int prev_val[] = {0, 0, 0, 0, 0, 0, 0, 0};
				for(j = 0;j < PD_Bpp;j++)
				{
					DE_PAETH(prev_val[j], (int)(PD_Up_Scanline[i]), (int)(PD_Diag_Scanline[i]), paeth_pred);
					QUEUE_POP(data);
					prev_val[j] = ((int)data + paeth_pred) & 0xFF;
					i++;
				}
				while(iteration--)
				{
					for(j = 0;j < PD_Bpp;j++)
					{
						DE_PAETH(prev_val[j], (int)(PD_Up_Scanline[i]), (int)(PD_Diag_Scanline[i]), paeth_pred);
						PD_Up_Scanline[i - PD_Bpp] = prev_val[j];
						QUEUE_POP(data);
						prev_val[j] = ((int)data + paeth_pred) & 0xFF;
						i++;
					}
				}
				for(j = 0;j < PD_Bpp;j++)
				{
					PD_Up_Scanline[i - PD_Bpp] = prev_val[j];
					i++;
				}
			}
			break;
		default :
			return PD_PROCESS_ERROR;
		}
	}
	return PD_PROCESS_DONE;
#else
	int i = 0;	
	uint8 data;

#	if defined(PNGDEC_OPT_CHECK_DEFLATE_BUF_SIZE)
	int iSelPopfn = 0;
#	endif
	
	if( mode != PD_Absolute_Perform )
	{
		uint8 temp = PD_Deflate_Buf[PD_RING_QUEUE_MASK & (PD_Ptr_Image_Dec + row_size + 1)];	
		if(temp <= PD_FILT_SUB ) // PD_FILT_NONE or PD_FILT_SUB
		{
			PD_Ptr_Image_Dec += (row_size + 1);
			return PD_PROCESS_DONE;
		}
	}

#	if defined(PNGDEC_OPT_CHECK_DEFLATE_BUF_SIZE)
	if(  (PD_Ptr_Image_Dec + row_size + 1) < PD_DEFLATE_BUF_LEN ) {
		iSelPopfn = 1;
	}
#	endif

	QUEUE_POP(PD_Filter_Method);
	
	switch(PD_Filter_Method)
	{
	case PD_FILT_NONE:
	#	if defined(PNGDEC_OPT_CHECK_DEFLATE_BUF_SIZE)
		if( iSelPopfn ) {
			for(;i < row_size;i++) {
				QUEUE_POP_NOMASK(data);
				PD_Up_Scanline[i] = data;
			}
		} else {
			for(;i < row_size;i++) {
				QUEUE_POP(data);
				PD_Up_Scanline[i] = data;
			}
		}
	#	else
		for(;i < row_size;i++) {
			QUEUE_POP(data);
			PD_Up_Scanline[i] = data;
		}
	#	endif
		break;
	case PD_FILT_UP:
	#	if defined(PNGDEC_OPT_CHECK_DEFLATE_BUF_SIZE)
		if( iSelPopfn ) {
			for(;i < row_size;i++) {
				QUEUE_POP_NOMASK(data);
				PD_Up_Scanline[i] = ((int)data + (int)(PD_Up_Scanline[i])) & 0xFF;
			}
		} else {
			for(;i < row_size;i++) {
				QUEUE_POP(data);
				PD_Up_Scanline[i] = ((int)data + (int)(PD_Up_Scanline[i])) & 0xFF;
			}
		}
	#	else
		for(;i < row_size;i++) {
			QUEUE_POP(data);
			PD_Up_Scanline[i] = ((int)data + (int)(PD_Up_Scanline[i])) & 0xFF;
		}
	#	endif
		break;
	case PD_FILT_SUB:
	#	if defined(PNGDEC_OPT_CHECK_DEFLATE_BUF_SIZE)
		if( iSelPopfn ) {
			for(;i < row_size;i++) {
				QUEUE_POP_NOMASK(data);
				PD_Up_Scanline[i] = ((int)data + (int)(PD_Up_Scanline[i - PD_Bpp])) & 0xFF;
			}			
		} else {
			for(;i < row_size;i++) {
				QUEUE_POP(data);
				PD_Up_Scanline[i] = ((int)data + (int)(PD_Up_Scanline[i - PD_Bpp])) & 0xFF;
			}
		}
	#	else
		for(;i < row_size;i++) {
			QUEUE_POP(data);
			PD_Up_Scanline[i] = ((int)data + (int)(PD_Up_Scanline[i - PD_Bpp])) & 0xFF;
		}
	#	endif
		break;
	case PD_FILT_AVR:
	#	if defined(PNGDEC_OPT_CHECK_DEFLATE_BUF_SIZE)
		if( iSelPopfn ) {
			for(;i < row_size;i++) {
				QUEUE_POP_NOMASK(data);
				PD_Up_Scanline[i] = ((int)data + (((int)(PD_Up_Scanline[i]) + (int)(PD_Up_Scanline[i - PD_Bpp])) >> 1)) & 0xFF;
			}			
		} else {
			for(;i < row_size;i++) {
				QUEUE_POP(data);
				PD_Up_Scanline[i] = ((int)data + (((int)(PD_Up_Scanline[i]) + (int)(PD_Up_Scanline[i - PD_Bpp])) >> 1)) & 0xFF;
			}
		}
	#	else
		for(;i < row_size;i++) {
			QUEUE_POP(data);
			PD_Up_Scanline[i] = ((int)data + (((int)(PD_Up_Scanline[i]) + (int)(PD_Up_Scanline[i - PD_Bpp])) >> 1)) & 0xFF;
		}
	#	endif
		break;
	case PD_FILT_PAETH:
		{
			int j, paeth_pred;
			int iteration = row_size / PD_Bpp - 1;
			int prev_val[] = {0, 0, 0, 0, 0, 0, 0, 0};

	#	if defined(PNGDEC_OPT_CHECK_DEFLATE_BUF_SIZE)
			if( iSelPopfn ) {
				for(j = 0;j < PD_Bpp;j++)
				{
					DE_PAETH(prev_val[j], (int)(PD_Up_Scanline[i]), (int)(PD_Diag_Scanline[i]), paeth_pred);
					QUEUE_POP_NOMASK(data);
					prev_val[j] = ((int)data + paeth_pred) & 0xFF;
					i++;
				}
				while(iteration--)
				{
					for(j = 0;j < PD_Bpp;j++)
					{
						DE_PAETH(prev_val[j], (int)(PD_Up_Scanline[i]), (int)(PD_Diag_Scanline[i]), paeth_pred);
						PD_Up_Scanline[i - PD_Bpp] = prev_val[j];
						QUEUE_POP_NOMASK(data);
						prev_val[j] = ((int)data + paeth_pred) & 0xFF;
						i++;
					}
				}
				for(j = 0;j < PD_Bpp;j++)
				{
					PD_Up_Scanline[i - PD_Bpp] = prev_val[j];
					i++;
				}
				
			} else {
				for(j = 0;j < PD_Bpp;j++)
				{
					DE_PAETH(prev_val[j], (int)(PD_Up_Scanline[i]), (int)(PD_Diag_Scanline[i]), paeth_pred);
					QUEUE_POP(data);
					prev_val[j] = ((int)data + paeth_pred) & 0xFF;
					i++;
				}
				while(iteration--)
				{
					for(j = 0;j < PD_Bpp;j++)
					{
						DE_PAETH(prev_val[j], (int)(PD_Up_Scanline[i]), (int)(PD_Diag_Scanline[i]), paeth_pred);
						PD_Up_Scanline[i - PD_Bpp] = prev_val[j];
						QUEUE_POP(data);
						prev_val[j] = ((int)data + paeth_pred) & 0xFF;
						i++;
					}
				}
				for(j = 0;j < PD_Bpp;j++)
				{
					PD_Up_Scanline[i - PD_Bpp] = prev_val[j];
					i++;
				}			
			}
	#	else
			for(j = 0;j < PD_Bpp;j++)
			{
				DE_PAETH(prev_val[j], (int)(PD_Up_Scanline[i]), (int)(PD_Diag_Scanline[i]), paeth_pred);
				QUEUE_POP(data);
				prev_val[j] = ((int)data + paeth_pred) & 0xFF;
				i++;
			}
			while(iteration--)
			{
				for(j = 0;j < PD_Bpp;j++)
				{
					DE_PAETH(prev_val[j], (int)(PD_Up_Scanline[i]), (int)(PD_Diag_Scanline[i]), paeth_pred);
					PD_Up_Scanline[i - PD_Bpp] = prev_val[j];
					QUEUE_POP(data);
					prev_val[j] = ((int)data + paeth_pred) & 0xFF;
					i++;
				}
			}
			for(j = 0;j < PD_Bpp;j++)
			{
				PD_Up_Scanline[i - PD_Bpp] = prev_val[j];
				i++;
			}
	#	endif			
		}
		break;
	default :
		return PD_PROCESS_ERROR;
	}
	return PD_PROCESS_DONE;
#endif
}


static int Image_Origin_Grey(void)
{
	int i;
	int processed_byte;
	uint32 prepared_bytes, num_row;

	IM_PIX_INFO out_struct;

	QUEUE_POP_CHECK(prepared_bytes);
	prepared_bytes--;
	num_row = prepared_bytes / (PD_Scanline_Size + 1);
	out_struct.Src_Fmt = IM_SRC_RGB; //IM_SRC_YUV

	if(PD_Bit_Depth == 8 || PD_Bit_Depth == 16)
	{
		uint32 offset;
		if(PD_Bit_Depth == 8)
			offset = 1;
		else
			offset = 2;

		while(num_row--)
		{
			out_struct.y = (PCD_Global_Pix_Pos / PD_Global_Width) + PD_Top_Offset;
			if(out_struct.y >= PD_LCD_Height)
			{
				PD_Last_IDAT = PD_DONE_ALREADY;
				return PD_PROCESS_DONE;
			}

			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;

			processed_byte = 0;

			if(PD_Color_Type == PD_COLOR_GREY_ALPHA)
			{
				for(i = 0;i < PD_Global_Width;i++)
				{
					out_struct.x = (PCD_Global_Pix_Pos++ % PD_Global_Width) + PD_Left_Offset;
					if(out_struct.x >= PD_LCD_Width)
					{
						PCD_Global_Pix_Pos += (PD_Global_Width - i - 1);
						break;
					}
					out_struct.Comp_1 = out_struct.Comp_2 = out_struct.Comp_3 = PD_Up_Scanline[processed_byte];
					processed_byte += offset;
					if(PD_Alpha_Use == 1)
						out_struct.Comp_4 = PD_Up_Scanline[processed_byte];
					processed_byte += offset;
									
					out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
					(PD_Out_Struct.write_func)(out_struct);
				}
			}
			else
			{
				for(i = 0;i < PD_Global_Width;i++)
				{
					out_struct.x = (PCD_Global_Pix_Pos++ % PD_Global_Width) + PD_Left_Offset;
					if(out_struct.x >= PD_LCD_Width)
					{
						PCD_Global_Pix_Pos += (PD_Global_Width - i - 1);
						break;
					}
					out_struct.Comp_1 = out_struct.Comp_2 = out_struct.Comp_3 = PD_Up_Scanline[processed_byte];
					processed_byte += offset;

					out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
					(PD_Out_Struct.write_func)(out_struct);
				}
			}
		}
	}
	else//PD_Bit_Depth == 1 or 2 or 4
	{
		int ppb;//pixel per byte
		int idx_mask;

		ppb = 8 / PD_Bit_Depth;
		idx_mask = ppb - 1;
		
		while(num_row--)
		{
			out_struct.y = (PCD_Global_Pix_Pos / PD_Global_Width) + PD_Top_Offset;
			if(out_struct.y >= PD_LCD_Height)
			{
				PD_Last_IDAT = PD_DONE_ALREADY;
				return PD_PROCESS_DONE;
			}

			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;

			for(i = 0;i < PD_Global_Width;i++)
			{
				out_struct.x = (PCD_Global_Pix_Pos++ % PD_Global_Width) + PD_Left_Offset;
				if(out_struct.x >= PD_LCD_Width)
				{
					PCD_Global_Pix_Pos += (PD_Global_Width - i - 1);
					break;
				}
				out_struct.Comp_1 = out_struct.Comp_2 = out_struct.Comp_3 = 
					((PD_Up_Scanline[i / ppb] & PD_Data_Mask[i & idx_mask])
						>> PD_Data_Shift[i & idx_mask]) * PD_Scaler;
				
				out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
				(PD_Out_Struct.write_func)(out_struct);
			}
		}
	}
	return PD_PROCESS_DONE;
}

static int Image_Origin_True(void)
{
	int i;
	uint32 prepared_bytes, num_row;
	uint32 processed_byte;
	uint32 offset;

	IM_PIX_INFO out_struct;
	out_struct.Src_Fmt = IM_SRC_RGB;

	QUEUE_POP_CHECK(prepared_bytes);
	prepared_bytes--;
	num_row = prepared_bytes / (PD_Scanline_Size + 1);

	if(PD_Bit_Depth == 8)
		offset = 1;
	else
		offset = 2;

	while(num_row--)
	{
		out_struct.y = (PCD_Global_Pix_Pos / PD_Global_Width) + PD_Top_Offset;
		if(out_struct.y >= PD_LCD_Height)
		{
			PD_Last_IDAT = PD_DONE_ALREADY;
			return PD_PROCESS_DONE;
		}

		if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
			return PD_PROCESS_ERROR;

		processed_byte = 0;

		if(PD_Color_Type == PD_COLOR_TRUE_ALPHA)
		{
			for(i = 0;i < PD_Global_Width;i++)
			{
				out_struct.x = (PCD_Global_Pix_Pos++ % PD_Global_Width) + PD_Left_Offset;
				if(out_struct.x >= PD_LCD_Width)
				{
					PCD_Global_Pix_Pos += (PD_Global_Width - i - 1);
					break;
				}
				out_struct.Comp_1= PD_Up_Scanline[processed_byte];
				processed_byte += offset;
				out_struct.Comp_2= PD_Up_Scanline[processed_byte];
				processed_byte += offset;
				out_struct.Comp_3= PD_Up_Scanline[processed_byte];
				processed_byte += offset;
				if(PD_Alpha_Use == 1)
					out_struct.Comp_4 = PD_Up_Scanline[processed_byte];
			#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
				else
					out_struct.Comp_4 = 0;
			#endif
				processed_byte += offset;
				
				out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
				(PD_Out_Struct.write_func)(out_struct);
			}
		}
		else
		{
			#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
			out_struct.Comp_4 = 0;
			#endif

			for(i = 0;i < PD_Global_Width;i++)
			{
				out_struct.x = (PCD_Global_Pix_Pos++ % PD_Global_Width) + PD_Left_Offset;
				if(out_struct.x >= PD_LCD_Width)
				{
					PCD_Global_Pix_Pos += (PD_Global_Width - i - 1);
					break;
				}
				out_struct.Comp_1= PD_Up_Scanline[processed_byte];
				processed_byte += offset;
				out_struct.Comp_2= PD_Up_Scanline[processed_byte];
				processed_byte += offset;
				out_struct.Comp_3= PD_Up_Scanline[processed_byte];
				processed_byte += offset;
				
				out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
				(PD_Out_Struct.write_func)(out_struct);
			}
		}
	}
	return PD_PROCESS_DONE;
}

static int Image_Origin_Indexed(void)
{
	int i;
	int processed_byte;
	int ppb;//pixel per byte
	int idx_mask;
	uint32 prepared_bytes, num_row;
	uint32 index;

	IM_PIX_INFO out_struct;
	out_struct.Src_Fmt = IM_SRC_RGB;

	QUEUE_POP_CHECK(prepared_bytes);
	prepared_bytes--;
	num_row = prepared_bytes / (PD_Scanline_Size + 1);
	

	ppb = 8 / PD_Bit_Depth;
	idx_mask = ppb - 1;
	while(num_row--)
	{
		out_struct.y = (PCD_Global_Pix_Pos / PD_Global_Width) + PD_Top_Offset;
		if(out_struct.y >= PD_LCD_Height)
		{
			PD_Last_IDAT = PD_DONE_ALREADY;
			return PD_PROCESS_DONE;
		}

		if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
			return PD_PROCESS_ERROR;

		processed_byte = -1;
		for(i = 0;i < PD_Global_Width;i++)
		{
			out_struct.x = (PCD_Global_Pix_Pos++ % PD_Global_Width) + PD_Left_Offset;
			if(out_struct.x >= PD_LCD_Width)
			{
				PCD_Global_Pix_Pos += (PD_Global_Width - i - 1);
				break;
			}
			if((uint32)(i & idx_mask) == 0)
				processed_byte++;
			index = (PD_Up_Scanline[i / ppb] & PD_Data_Mask[i & idx_mask]) >> PD_Data_Shift[i & idx_mask];
			out_struct.Comp_1 = PD_Plte[index].R;
			out_struct.Comp_2 = PD_Plte[index].G;
			out_struct.Comp_3 = PD_Plte[index].B;

			if(PD_Alpha_Use == 1)
				out_struct.Comp_4 = PD_Plte[index].Alpha;
		#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
			else
				out_struct.Comp_4 = 0;
		#endif
			
			out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
			(PD_Out_Struct.write_func)(out_struct);
		}
	}
	return PD_PROCESS_DONE;
}


static int Image_Resize_Grey(void)
{
	int i;
	uint32 prepared_bytes, num_row;
	uint32 offset = (PD_Bit_Depth == 8)?1:2;

	IM_PIX_INFO out_struct;
	out_struct.Src_Fmt = IM_SRC_RGB;

	QUEUE_POP_CHECK(prepared_bytes);
	prepared_bytes--;
	num_row = prepared_bytes / (PD_Scanline_Size + 1);

	if(PD_Bit_Depth == 8 || PD_Bit_Depth == 16)
	{
		while(num_row--)
		{
			if(PD_Row == PD_Pixel_Map_Ver[PD_Resize_Ver_Idx])
			{
				out_struct.y = (PCD_Global_Pix_Pos / PD_Resized_Width) + PD_Top_Offset;
				if(out_struct.y >= PD_LCD_Height)
				{
					PD_Last_IDAT = PD_DONE_ALREADY;
					return PD_PROCESS_DONE;
				}

				if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
					return PD_PROCESS_ERROR;
				PD_Resize_Ver_Idx++;

				if(PD_Color_Type == PD_COLOR_GREY_ALPHA)
				{
					for(i = 0;i < PD_Resized_Width;i++)
					{
						out_struct.x = (PCD_Global_Pix_Pos++ % PD_Resized_Width) + PD_Left_Offset;
						if(out_struct.x >= PD_LCD_Width)
						{
							PCD_Global_Pix_Pos += (PD_Resized_Width - i - 1);
							break;
						}
						out_struct.Comp_1 = 
						out_struct.Comp_2 = 
						out_struct.Comp_3 = PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp];

						if(PD_Alpha_Use == 1)
							out_struct.Comp_4 = PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp + offset];
						
						out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
						(PD_Out_Struct.write_func)(out_struct);
					}
				}
				else
				{
					for(i = 0;i < PD_Resized_Width;i++)
					{
						out_struct.x = (PCD_Global_Pix_Pos++ % PD_Resized_Width) + PD_Left_Offset;
						if(out_struct.x >= PD_LCD_Width)
						{
							PCD_Global_Pix_Pos += (PD_Resized_Width - i - 1);
							break;
						}
						out_struct.Comp_1 = 
						out_struct.Comp_2 = 
						out_struct.Comp_3 = PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp];
						
						out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
						(PD_Out_Struct.write_func)(out_struct);
					}
				}
			}
			else
			{
				if(PNG_Defiltering(PD_Scanline_Size, PD_Conditional_Skip) == PD_PROCESS_ERROR)
					return PD_PROCESS_ERROR;
			}
			PD_Row++;
		}
	}
	else//PD_Bit_Depth == 1 or 2 or 4
	{
		int ppb;//pixel per byte
		int idx_mask;
		int index;

		ppb = 8 / PD_Bit_Depth;
		idx_mask = ppb - 1;
		
		while(num_row--)
		{
			if(PD_Row == PD_Pixel_Map_Ver[PD_Resize_Ver_Idx])
			{
				out_struct.y = (PCD_Global_Pix_Pos / PD_Resized_Width) + PD_Top_Offset;
				if(out_struct.y >= PD_LCD_Height)
				{
					PD_Last_IDAT = PD_DONE_ALREADY;
					return PD_PROCESS_DONE;
				}

				if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
					return PD_PROCESS_ERROR;
				PD_Resize_Ver_Idx++;

				for(i = 0;i < PD_Resized_Width;i++)
				{
					out_struct.x = (PCD_Global_Pix_Pos++ % PD_Resized_Width) + PD_Left_Offset;
					if(out_struct.x >= PD_LCD_Width)
					{
						PCD_Global_Pix_Pos += (PD_Resized_Width - i - 1);
						break;
					}
					index = PD_Pixel_Map_Hor[i];
					out_struct.Comp_1 =
					out_struct.Comp_2 = 
					out_struct.Comp_3 = (
						(PD_Up_Scanline[index / ppb] & PD_Data_Mask[index & idx_mask])
						>> PD_Data_Shift[index & idx_mask] ) * PD_Scaler;
					
					out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
					(PD_Out_Struct.write_func)(out_struct);
				}
			}
			else
			{
				if(PNG_Defiltering(PD_Scanline_Size, PD_Conditional_Skip) == PD_PROCESS_ERROR)
					return PD_PROCESS_ERROR;
			}
			PD_Row++;
		}
	}
	return PD_PROCESS_DONE;
}


static int Image_Resize_True(void)
{
	int i;
	uint32 prepared_bytes, num_row;
	uint32 offset;
	IM_PIX_INFO out_struct;

	out_struct.Src_Fmt = IM_SRC_RGB;

	QUEUE_POP_CHECK(prepared_bytes);
	prepared_bytes--;
	num_row = prepared_bytes / (PD_Scanline_Size + 1);

	if(PD_Bit_Depth == 8)
		offset = 1;
	else
		offset = 2;

	while(num_row--)
	{
		if(PD_Row == PD_Pixel_Map_Ver[PD_Resize_Ver_Idx])
		{
			out_struct.y = (PCD_Global_Pix_Pos / PD_Resized_Width) + PD_Top_Offset;
			if(out_struct.y >= PD_LCD_Height)
			{
				PD_Last_IDAT = PD_DONE_ALREADY;
				return PD_PROCESS_DONE;
			}

			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
			PD_Resize_Ver_Idx++;
			if(PD_Color_Type == PD_COLOR_TRUE_ALPHA)
			{
				for(i = 0;i < PD_Resized_Width;i++)
				{
					out_struct.x = (PCD_Global_Pix_Pos++ % PD_Resized_Width) + PD_Left_Offset;					
					if(out_struct.x >= PD_LCD_Width)
					{
						PCD_Global_Pix_Pos += (PD_Resized_Width - i - 1);
						break;
					}

					out_struct.Comp_1= PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp];
					out_struct.Comp_2= PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp + offset];
					out_struct.Comp_3= PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp + offset * 2];
					if(PD_Alpha_Use == 1)
						out_struct.Comp_4 = PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp + offset * 3];
				#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
					else
						out_struct.Comp_4 = 0;
				#endif
						
					
					out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
					(PD_Out_Struct.write_func)(out_struct);
				}
			}
			else
			{
			#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
				out_struct.Comp_4 = 0;
			#endif

				for(i = 0;i < PD_Resized_Width;i++)
				{
					out_struct.x = (PCD_Global_Pix_Pos++ % PD_Resized_Width) + PD_Left_Offset;
					if(out_struct.x >= PD_LCD_Width)
					{
						PCD_Global_Pix_Pos += (PD_Resized_Width - i - 1);
						break;
					}
					out_struct.Comp_1= PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp];
					out_struct.Comp_2= PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp + offset];
					out_struct.Comp_3= PD_Up_Scanline[PD_Pixel_Map_Hor[i] * PD_Bpp + offset * 2];
					
					out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
					(PD_Out_Struct.write_func)(out_struct);
				}
			}			
		}
		else
		{
			if(PNG_Defiltering(PD_Scanline_Size, PD_Conditional_Skip) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
		}
		PD_Row++;
	}
	return PD_PROCESS_DONE;
}

static int Image_Resize_Indexed(void)
{
	int i;
	int ppb;//pixel per byte
	int idx_mask;
	uint32 prepared_bytes, num_row;
	uint32 index;

	IM_PIX_INFO out_struct;
	out_struct.Src_Fmt = IM_SRC_RGB;

	QUEUE_POP_CHECK(prepared_bytes);
	prepared_bytes--;
	num_row = prepared_bytes / (PD_Scanline_Size + 1);
	

	ppb = 8 / PD_Bit_Depth;
	idx_mask = ppb - 1;
	while(num_row--)
	{
		if(PD_Row == PD_Pixel_Map_Ver[PD_Resize_Ver_Idx])
		{
			out_struct.y = (PCD_Global_Pix_Pos / PD_Resized_Width) + PD_Top_Offset;
			if(out_struct.y >= PD_LCD_Height)
			{
				PD_Last_IDAT = PD_DONE_ALREADY;
				return PD_PROCESS_DONE;
			}

			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
			PD_Resize_Ver_Idx++;

			for(i = 0;i < PD_Resized_Width;i++)
			{
				out_struct.x = (PCD_Global_Pix_Pos++ % PD_Resized_Width) + PD_Left_Offset;
				if(out_struct.x >= PD_LCD_Width)
				{
					PCD_Global_Pix_Pos += (PD_Resized_Width - i - 1);
					break;
				}
				
				index = PD_Pixel_Map_Hor[i];
				index = (PD_Up_Scanline[index / ppb] & PD_Data_Mask[index & idx_mask]) >> PD_Data_Shift[index & idx_mask];
				out_struct.Comp_1 = PD_Plte[index].R;
				out_struct.Comp_2 = PD_Plte[index].G;
				out_struct.Comp_3 = PD_Plte[index].B;

				if(PD_Alpha_Use == 1)
					out_struct.Comp_4 = PD_Plte[index].Alpha;
			#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
				else
					out_struct.Comp_4 = 0;
			#endif
				
				out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
				(PD_Out_Struct.write_func)(out_struct);
			}
		}
		else
		{
			if(PNG_Defiltering(PD_Scanline_Size, PD_Conditional_Skip) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
		}
		PD_Row++;
	}
	return PD_PROCESS_DONE;
}

static int Image_Origin_Grey_ADAM7(void)
{
	int i;
	int processed_byte;
	uint32 prepared_bytes, num_row;
	uint32 offset;
	uint32 hor_inc, ver_inc, hor_offset, ver_offset;
	int ppb;//pixel per byte
	int idx_mask;
	IM_PIX_INFO out_struct;
	out_struct.Src_Fmt = IM_SRC_RGB;
	
	if(PD_Bit_Depth == 8)
		offset = 1;
	else
		offset = 2;

	ppb = 8 / PD_Bit_Depth;
	idx_mask = ppb - 1;

	while(1)
	{
		if(PD_Remaining_Row == 0)
			PNG_Init_ADAM7_Map(PD_Current_Pass + 1);

		if(PD_Current_Pass > 7)
			break;

		QUEUE_POP_CHECK(prepared_bytes);
		prepared_bytes--;
		num_row = prepared_bytes / (PD_Scanline_Size + 1);

		if(num_row == 0)
			break;

		if(num_row > PD_Remaining_Row)
			num_row = PD_Remaining_Row;
		PD_Remaining_Row -= num_row;

		hor_inc = PDRO_Hor_Incre[PD_Current_Pass - 1];
		ver_inc = PDRO_Ver_Incre[PD_Current_Pass - 1];
		hor_offset = PDRO_Hor_Start[PD_Current_Pass - 1] + PD_Left_Offset;
		ver_offset = PDRO_Ver_Start[PD_Current_Pass - 1] + PD_Top_Offset;

		while(num_row--)
		{
			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
			out_struct.y = PD_Row * ver_inc + ver_offset;
			if(out_struct.y < PD_LCD_Height)
			{
				processed_byte = 0;
				if(PD_Color_Type == PD_COLOR_GREY_ALPHA)
				{
					for(i = 0;i < PD_ADAM7_Width;i++)
					{
						out_struct.x = i * hor_inc + hor_offset;
						if(out_struct.x >= PD_LCD_Width)
						{
							break;
						}
						
						out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
						
						if(PD_Bit_Depth < 8)
						{
							out_struct.Comp_1 = out_struct.Comp_2 = out_struct.Comp_3 = 
								((PD_Up_Scanline[i / ppb] & PD_Data_Mask[i & idx_mask])
									>> PD_Data_Shift[i & idx_mask]) * PD_Scaler;
						}
						else
						{
							out_struct.Comp_1 = out_struct.Comp_2 = out_struct.Comp_3 = PD_Up_Scanline[processed_byte];
							processed_byte += offset;
							if(PD_Alpha_Use == 1)
								out_struct.Comp_4 = PD_Up_Scanline[processed_byte];
							processed_byte += offset;
						}				
						(PD_Out_Struct.write_func)(out_struct);
					}
				}
				else
				{
					for(i = 0;i < PD_ADAM7_Width;i++)
					{
						out_struct.x = i * hor_inc + hor_offset;
						if(out_struct.x >= PD_LCD_Width)
						{
							break;
						}
						if(PD_Bit_Depth < 8)
						{
							out_struct.Comp_1 = out_struct.Comp_2 = out_struct.Comp_3 = 
								((PD_Up_Scanline[i / ppb] & PD_Data_Mask[i & idx_mask])
									>> PD_Data_Shift[i & idx_mask]) * PD_Scaler;
						}
						else
						{
							out_struct.Comp_1 = out_struct.Comp_2 = out_struct.Comp_3 = PD_Up_Scanline[processed_byte];
							processed_byte += offset;
						}				
						out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
						(PD_Out_Struct.write_func)(out_struct);
					}
				}
			}
			PD_Row++;
		}
	}
	return PD_PROCESS_DONE;
}

static int Image_Origin_True_ADAM7(void)
{
	int i;
	int processed_byte;
	uint32 hor_inc, ver_inc, hor_offset, ver_offset;
	uint32 prepared_bytes, num_row;
	uint32 offset;

	IM_PIX_INFO out_struct;
	out_struct.Src_Fmt = IM_SRC_RGB;

	if(PD_Bit_Depth == 8)
		offset = 1;
	else
		offset = 2;
	
	while(1)
	{
		if(PD_Remaining_Row == 0)
			PNG_Init_ADAM7_Map(PD_Current_Pass + 1);

		QUEUE_POP_CHECK(prepared_bytes);
		prepared_bytes--;
		num_row = prepared_bytes / (PD_Scanline_Size + 1);
		
		if(num_row == 0)
			break;

		if(num_row > PD_Remaining_Row)
			num_row = PD_Remaining_Row;

		PD_Remaining_Row -= num_row;
		
		hor_inc = PDRO_Hor_Incre[PD_Current_Pass - 1];
		ver_inc = PDRO_Ver_Incre[PD_Current_Pass - 1];
		hor_offset = PDRO_Hor_Start[PD_Current_Pass - 1] + PD_Left_Offset;
		ver_offset = PDRO_Ver_Start[PD_Current_Pass - 1] + PD_Top_Offset;
		
		while(num_row--)
		{
			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
			out_struct.y = PD_Row * ver_inc + ver_offset;
			if(out_struct.y < PD_LCD_Height)
			{
				processed_byte = 0;
				if(PD_Color_Type == PD_COLOR_TRUE_ALPHA)
				{
					for(i = 0;i < PD_ADAM7_Width;i++)
					{
						out_struct.x = i * hor_inc + hor_offset;
						if(out_struct.x >= PD_LCD_Width)
						{
							break;
						}
						out_struct.Comp_1= PD_Up_Scanline[processed_byte];
						processed_byte += offset;
						out_struct.Comp_2= PD_Up_Scanline[processed_byte];
						processed_byte += offset;
						out_struct.Comp_3= PD_Up_Scanline[processed_byte];
						processed_byte += offset;
						if(PD_Alpha_Use == 1)
							out_struct.Comp_4 = PD_Up_Scanline[processed_byte];
					#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
						else
							out_struct.Comp_4 = 0;
					#endif

						processed_byte += offset;
						
						out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
						(PD_Out_Struct.write_func)(out_struct);
					}
				}
				else
				{
				#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
					out_struct.Comp_4 = 0;
				#endif

					for(i = 0;i < PD_ADAM7_Width;i++)
					{
						out_struct.x = i * hor_inc + hor_offset;
						if(out_struct.x >= PD_LCD_Width)
							break;
						out_struct.Comp_1= PD_Up_Scanline[processed_byte];
						processed_byte += offset;
						out_struct.Comp_2= PD_Up_Scanline[processed_byte];
						processed_byte += offset;
						out_struct.Comp_3= PD_Up_Scanline[processed_byte];
						processed_byte += offset;
						
						out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
						(PD_Out_Struct.write_func)(out_struct);
					}
				}
			}
			PD_Row++;
		}
	}
	return PD_PROCESS_DONE;
}

static int Image_Origin_Indexed_ADAM7(void)
{
	int i;
	int processed_byte;
	int ppb;//pixel per byte
	int idx_mask;
	uint32 prepared_bytes, num_row;
	uint32 index;
	uint32 hor_inc, ver_inc, hor_offset, ver_offset;

	IM_PIX_INFO out_struct;
	out_struct.Src_Fmt = IM_SRC_RGB;

	ppb = 8 / PD_Bit_Depth;
	idx_mask = ppb - 1;
	while(1)
	{
		if(PD_Remaining_Row == 0)
			PNG_Init_ADAM7_Map(PD_Current_Pass + 1);

		QUEUE_POP_CHECK(prepared_bytes);
		prepared_bytes--;
		num_row = prepared_bytes / (PD_Scanline_Size + 1);

		if(num_row == 0)
			break;

		if(num_row > PD_Remaining_Row)
			num_row = PD_Remaining_Row;

		PD_Remaining_Row -= num_row;

		hor_inc = PDRO_Hor_Incre[PD_Current_Pass - 1];
		ver_inc = PDRO_Ver_Incre[PD_Current_Pass - 1];
		hor_offset = PDRO_Hor_Start[PD_Current_Pass - 1] + PD_Left_Offset;
		ver_offset = PDRO_Ver_Start[PD_Current_Pass - 1] + PD_Top_Offset;
		
		while(num_row--)
		{
			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
			out_struct.y = PD_Row * ver_inc + ver_offset;
			if(out_struct.y < PD_LCD_Height)
			{
				processed_byte = -1;
				for(i = 0;i < PD_ADAM7_Width;i++)
				{
					out_struct.x = i * hor_inc + hor_offset;
					if(out_struct.x >= PD_LCD_Width)
						break;
					if((uint32)(i & idx_mask) == 0)
						processed_byte++;
					index = (PD_Up_Scanline[i / ppb] & PD_Data_Mask[i & idx_mask]) >> PD_Data_Shift[i & idx_mask];
					out_struct.Comp_1 = PD_Plte[index].R;
					out_struct.Comp_2 = PD_Plte[index].G;
					out_struct.Comp_3 = PD_Plte[index].B;

					if(PD_Alpha_Use == 1)
						out_struct.Comp_4 = PD_Plte[index].Alpha;
				#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
					else
						out_struct.Comp_4 = 0;
				#endif
					
					out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
					(PD_Out_Struct.write_func)(out_struct);
				}
			}
			PD_Row++;
		}
	}
	return PD_PROCESS_DONE;
}

static int Image_Resize_Grey_ADAM7(void)
{
	int i;
	uint32 hor_inc, ver_inc, hor_offset, ver_offset;
	uint32 prepared_bytes, num_row;
	uint32 temp;
	uint32 offset;
	IM_PIX_INFO out_struct;
	int ppb = 8 / PD_Bit_Depth;
	int idx_mask = ppb - 1;
	int category;

	out_struct.Src_Fmt = IM_SRC_RGB;
	
	if(PD_Bit_Depth == 8 || PD_Bit_Depth == 16)
	{
		if(PD_Bit_Depth == 0)
			offset = 1;
		else
			offset = 2;
		category = 0;
	}
	else
		category = 1;
	
	while(1)
	{
		if(PD_Remaining_Row == 0 && PD_Resize_Ver_Idx == PD_ADAM7_Height)
			PNG_Init_ADAM7_Map(PD_Current_Pass + 1);

		QUEUE_POP_CHECK(prepared_bytes);
		prepared_bytes--;
		num_row = prepared_bytes / (PD_Scanline_Size + 1);
		
		if(num_row == 0)
			break;

		if(num_row > PD_Remaining_Row)
			num_row = PD_Remaining_Row;

		PD_Remaining_Row -= num_row;
		
		hor_inc = PDRO_Hor_Incre[PD_Current_Pass - 1];
		ver_inc = PDRO_Ver_Incre[PD_Current_Pass - 1];
		hor_offset = PDRO_Hor_Start[PD_Current_Pass - 1];
		ver_offset = PDRO_Ver_Start[PD_Current_Pass - 1];

		for(;PD_Row < PD_Resized_Height;PD_Row++)
		{
			if((PD_Pixel_Map_Ver[PD_Row] - ver_offset) % ver_inc == 0)
			{
				temp = (PD_Pixel_Map_Ver[PD_Row] - ver_offset) / ver_inc;
				while(PD_Resize_Ver_Idx != temp)
				{
					if(PNG_Defiltering(PD_Scanline_Size, PD_Conditional_Skip) == PD_PROCESS_ERROR)
						return PD_PROCESS_ERROR;
					num_row--;
					PD_Resize_Ver_Idx++;
					if(num_row == 0)
						return PD_PROCESS_DONE;
				}

				if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
					return PD_PROCESS_ERROR;
				num_row--;
				PD_Resize_Ver_Idx++;
				
				out_struct.y = PD_Row + PD_Top_Offset;
				if(out_struct.y < PD_LCD_Height)
				{
					if(PD_Color_Type == PD_COLOR_GREY_ALPHA)
					{
						for(i = 0;i < PD_Resized_Width;i++)
						{
							if((PD_Pixel_Map_Hor[i] - hor_offset) % hor_inc == 0)
							{
								out_struct.x = i + PD_Left_Offset;
								if(out_struct.x >= PD_LCD_Width)
									break;
								temp = (PD_Pixel_Map_Hor[i] - hor_offset) / hor_inc;
								out_struct.Comp_1 = 
								out_struct.Comp_2 = 
								out_struct.Comp_3 = PD_Up_Scanline[temp * PD_Bpp];

								if(PD_Alpha_Use == 1)
									out_struct.Comp_4 = PD_Up_Scanline[temp * PD_Bpp + offset];
								
								out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
								(PD_Out_Struct.write_func)(out_struct);
							}
						}
					}
					else
					{
						for(i = 0;i < PD_Resized_Width;i++)
						{
							if((PD_Pixel_Map_Hor[i] - hor_offset) % hor_inc == 0)
							{
								out_struct.x = i + PD_Left_Offset;
								if(out_struct.x >= PD_LCD_Width)
									break;
								temp = (PD_Pixel_Map_Hor[i] - hor_offset) / hor_inc;
								if(category == 0)
									out_struct.Comp_1 = 
									out_struct.Comp_2 = 
									out_struct.Comp_3 = PD_Up_Scanline[temp * PD_Bpp];
								else
									out_struct.Comp_1 = 
									out_struct.Comp_2 = 
									out_struct.Comp_3 = ((PD_Up_Scanline[temp / ppb] & PD_Data_Mask[temp & idx_mask]) >> PD_Data_Shift[temp & idx_mask]) * PD_Scaler;
								
								out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
								(PD_Out_Struct.write_func)(out_struct);
							}
						}
					}
				}
				if(num_row == 0)
				{
					PD_Row++;
					return PD_PROCESS_DONE;
				}
			}
		}
		while(PD_Resize_Ver_Idx != PD_ADAM7_Height)
		{
			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Skip) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
			num_row--;
			PD_Resize_Ver_Idx++;
			if(num_row == 0)
			{
				PD_Row++;
				return PD_PROCESS_DONE;
			}
		}

	}
	return PD_PROCESS_DONE;
}


static int Image_Resize_True_ADAM7(void)
{
	int i;
	uint32 hor_inc, ver_inc, hor_offset, ver_offset;
	uint32 prepared_bytes, num_row;
	uint32 offset;
	uint32 temp;

	IM_PIX_INFO out_struct;
	out_struct.Src_Fmt = IM_SRC_RGB;

	if(PD_Bit_Depth == 8)
		offset = 1;
	else
		offset = 2;
	
	while(1)
	{
		if(PD_Remaining_Row == 0 && PD_Resize_Ver_Idx == PD_ADAM7_Height)
			PNG_Init_ADAM7_Map(PD_Current_Pass + 1);

		QUEUE_POP_CHECK(prepared_bytes);
		prepared_bytes--;
		num_row = prepared_bytes / (PD_Scanline_Size + 1);
		
		if(num_row == 0)
			break;

		if(num_row > PD_Remaining_Row)
			num_row = PD_Remaining_Row;

		PD_Remaining_Row -= num_row;
		
		hor_inc = PDRO_Hor_Incre[PD_Current_Pass - 1];
		ver_inc = PDRO_Ver_Incre[PD_Current_Pass - 1];
		hor_offset = PDRO_Hor_Start[PD_Current_Pass - 1];
		ver_offset = PDRO_Ver_Start[PD_Current_Pass - 1];

		for(;PD_Row < PD_Resized_Height;PD_Row++)
		{
			if((PD_Pixel_Map_Ver[PD_Row] - ver_offset) % ver_inc == 0)
			{
				temp = (PD_Pixel_Map_Ver[PD_Row] - ver_offset) / ver_inc;
				while(PD_Resize_Ver_Idx != temp)
				{
					if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
						return PD_PROCESS_ERROR;
					num_row--;
					PD_Resize_Ver_Idx++;
					if(num_row == 0)
						return PD_PROCESS_DONE;
				}

				if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
					return PD_PROCESS_ERROR;
				num_row--;
				PD_Resize_Ver_Idx++;

				out_struct.y = PD_Row + PD_Top_Offset;
				if(out_struct.y < PD_LCD_Height)
				{
					if(PD_Color_Type == PD_COLOR_TRUE_ALPHA)
					{
						for(i = 0;i < PD_Resized_Width;i++)
						{
							if((PD_Pixel_Map_Hor[i] - hor_offset) % hor_inc == 0)
							{
								out_struct.x = i + PD_Left_Offset;
								if(out_struct.x >= PD_LCD_Width)
									break;
								temp = (PD_Pixel_Map_Hor[i] - hor_offset) / hor_inc;
								out_struct.Comp_1 = PD_Up_Scanline[temp * PD_Bpp];
								out_struct.Comp_2 = PD_Up_Scanline[temp * PD_Bpp + offset];
								out_struct.Comp_3 = PD_Up_Scanline[temp * PD_Bpp + offset * 2];

								if(PD_Alpha_Use == 1)
									out_struct.Comp_4 = PD_Up_Scanline[temp * PD_Bpp + offset * 3];
							#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
								else
									out_struct.Comp_4 = 0;
							#endif
								
								out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
								(PD_Out_Struct.write_func)(out_struct);
							}
						}
					}
					else
					{
					#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
						out_struct.Comp_4 = 0;
					#endif

						for(i = 0;i < PD_Resized_Width;i++)
						{
							if((PD_Pixel_Map_Hor[i] - hor_offset) % hor_inc == 0)
							{
								out_struct.x = i + PD_Left_Offset;
								if(out_struct.x >= PD_LCD_Width)
									break;
								temp = (PD_Pixel_Map_Hor[i] - hor_offset) / hor_inc;
								out_struct.Comp_1 = PD_Up_Scanline[temp * PD_Bpp];
								out_struct.Comp_2 = PD_Up_Scanline[temp * PD_Bpp + offset];
								out_struct.Comp_3 = PD_Up_Scanline[temp * PD_Bpp + offset * 2];
								
								out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
								(PD_Out_Struct.write_func)(out_struct);
							}
						}
					}
				}
				if(num_row == 0)
				{
					PD_Row++;
					return PD_PROCESS_DONE;
				}
			}
		}
		while(PD_Resize_Ver_Idx != PD_ADAM7_Height)
		{
			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Skip) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
			num_row--;
			PD_Resize_Ver_Idx++;
			if(num_row == 0)
			{
				PD_Row++;
				return PD_PROCESS_DONE;
			}
		}

	}
	return PD_PROCESS_DONE;
}

static int Image_Resize_Indexed_ADAM7(void)
{
	int i;
	uint32 hor_inc, ver_inc, hor_offset, ver_offset;
	uint32 prepared_bytes, num_row;
	uint32 temp;

	int ppb;//pixel per byte
	int idx_mask;

	IM_PIX_INFO out_struct;
	out_struct.Src_Fmt = IM_SRC_RGB;

	ppb = 8 / PD_Bit_Depth;
	idx_mask = ppb - 1;

	while(1)
	{
		if(PD_Remaining_Row == 0 && PD_Resize_Ver_Idx == PD_ADAM7_Height)
			PNG_Init_ADAM7_Map(PD_Current_Pass + 1);

		QUEUE_POP_CHECK(prepared_bytes);
		prepared_bytes--;
		num_row = prepared_bytes / (PD_Scanline_Size + 1);
		
		if(num_row == 0)
			break;

		if(num_row > PD_Remaining_Row)
			num_row = PD_Remaining_Row;

		PD_Remaining_Row -= num_row;
		
		hor_inc = PDRO_Hor_Incre[PD_Current_Pass - 1];
		ver_inc = PDRO_Ver_Incre[PD_Current_Pass - 1];
		hor_offset = PDRO_Hor_Start[PD_Current_Pass - 1];
		ver_offset = PDRO_Ver_Start[PD_Current_Pass - 1];

		for(;PD_Row < PD_Resized_Height;PD_Row++)
		{
			if((PD_Pixel_Map_Ver[PD_Row] - ver_offset) % ver_inc == 0)
			{
				temp = (PD_Pixel_Map_Ver[PD_Row] - ver_offset) / ver_inc;
				while(PD_Resize_Ver_Idx != temp)
				{
					if(PNG_Defiltering(PD_Scanline_Size, PD_Conditional_Skip) == PD_PROCESS_ERROR)
						return PD_PROCESS_ERROR;
					num_row--;
					PD_Resize_Ver_Idx++;
					if(num_row == 0)
						return PD_PROCESS_DONE;
				}

				if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Perform) == PD_PROCESS_ERROR)
					return PD_PROCESS_ERROR;
				num_row--;
				PD_Resize_Ver_Idx++;

				out_struct.y = PD_Row + PD_Top_Offset;
				if(out_struct.y < PD_LCD_Height)
				{
					for(i = 0;i < PD_Resized_Width;i++)
					{
						if((PD_Pixel_Map_Hor[i] - hor_offset) % hor_inc == 0)
						{
							out_struct.x = i + PD_Left_Offset;
							if(out_struct.x >= PD_LCD_Width)
								break;
							temp = (PD_Pixel_Map_Hor[i] - hor_offset) / hor_inc;
							temp = (PD_Up_Scanline[temp / ppb] & PD_Data_Mask[temp & idx_mask]) >> PD_Data_Shift[temp & idx_mask];
							out_struct.Comp_1 = PD_Plte[temp].R;
							out_struct.Comp_2 = PD_Plte[temp].G;
							out_struct.Comp_3 = PD_Plte[temp].B;
							if(PD_Alpha_Use == 1)
								out_struct.Comp_4 = PD_Plte[temp].Alpha;
						#if defined(PNGDEC_MOD_RGB_COMP4_RESET)
							else
								out_struct.Comp_4 = 0;
						#endif
							
							out_struct.Offset = out_struct.y * PD_LCD_Width + out_struct.x;
							(PD_Out_Struct.write_func)(out_struct);
						}
					}
				}
				if(num_row == 0)
				{
					PD_Row++;
					return PD_PROCESS_DONE;
				}
			}
		}
		while(PD_Resize_Ver_Idx != PD_ADAM7_Height)
		{
			if(PNG_Defiltering(PD_Scanline_Size, PD_Absolute_Skip) == PD_PROCESS_ERROR)
				return PD_PROCESS_ERROR;
			num_row--;
			PD_Resize_Ver_Idx++;
			if(num_row == 0)
			{
				PD_Row++;
				return PD_PROCESS_DONE;
			}
		}

	}
	return PD_PROCESS_DONE;
}



//////////////////////
//Header Parsing Related
//////////////////////
static int PNG_Check_PNG_Marker(void)
{
	uint32 marker_ret;

	READWORD(marker_ret);
	if(marker_ret == PD_MARKER_PNG1)
	{
		READWORD(marker_ret);
		
		if( PD_nPngDecErrorCode < 0 ) {
			return PD_PROCESS_ERROR;
		}
	
		if(marker_ret == PD_MARKER_PNG2)
			return PD_PROCESS_DONE;
	}
	return PD_PROCESS_ERROR;
}

static int PNG_Parse_IHDR_Chunk(int iOption)
{
	uint32 stream_out;
	READWORD(PD_Chunk_Size);
	if(PD_Chunk_Size != PD_IHDR_CHUNK_SIZE)
		return PD_PROCESS_ERROR;

#if defined(PNGDEC_CHECK_CHUNK)
	PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_IHDR );
#endif

	READWORD(stream_out);
	if(stream_out != PD_MARKER_IHDR)
		return PD_PROCESS_ERROR;

	READWORD(PD_Global_Width);
	READWORD(PD_Global_Height);

	READBYTE(PD_Bit_Depth);		// 1,2,4,8,16
	#if defined(PNGDEC_STABILITY_BIT_DEPTH)
	switch( PD_Bit_Depth )
	{
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
		break;
	default:
		return PD_PROCESS_ERROR;
	}	
	#endif
	READBYTE(PD_Color_Type);

	READBYTE(PD_Compression_Method);
	if(PD_Compression_Method != 0)
		return PD_PROCESS_ERROR;
	
	READBYTE(stream_out);//Filter Method
	if(stream_out != 0)
		return PD_PROCESS_ERROR;
	
	READBYTE(PD_Interlace_Method);
	if(PD_Interlace_Method > PD_INTERLACE_ADAM)
		return PD_PROCESS_ERROR;

	if( PD_nPngDecErrorCode < 0 ) {
		return PD_PROCESS_ERROR;
	}

#ifdef PNGDEC_SUPPORT_ISDBT_PNG
	if( iOption& (1<<4) )
	{
		if( 
			( PD_Bit_Depth == 8 )
			&& ( PD_Color_Type == 3 )
			&& ( PD_Interlace_Method == 0 )
		  )
		{
			PD_Bit_Depth = 8;
			PD_Color_Type = PD_COLOR_GREY;
		}
	}
#endif

	if((PD_LCD_Width >= PD_Global_Width) && (PD_LCD_Height >= PD_Global_Height))
		PD_Image_Smaller_LCD = PD_TRUE;
	else
		PD_Image_Smaller_LCD = PD_FALSE;

	switch(PD_Color_Type)
	{
	case PD_COLOR_GREY:
		if(!(PD_Bit_Depth == 1 || PD_Bit_Depth == 2 || PD_Bit_Depth == 4 || PD_Bit_Depth == 8 || PD_Bit_Depth == 16))
			return PD_PROCESS_ERROR;
			
		if(PD_Image_Smaller_LCD == PD_TRUE)
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Origin_Grey_ADAM7;
			else
				PNG_Decode_Image = Image_Origin_Grey;
		}
		else
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Resize_Grey_ADAM7;
			else
				PNG_Decode_Image = Image_Resize_Grey;
		}
		PD_Compo_Num = 1;
		break;
	case PD_COLOR_TRUE:
		if(!(PD_Bit_Depth == 8 || PD_Bit_Depth == 16))
			return PD_PROCESS_ERROR;

		if(PD_Image_Smaller_LCD == PD_TRUE)
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Origin_True_ADAM7;
			else
				PNG_Decode_Image = Image_Origin_True;
		}
		else
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Resize_True_ADAM7;
			else
				PNG_Decode_Image = Image_Resize_True;
		}
		PD_Compo_Num = 3;
		break;
	case PD_COLOR_INDEX:
		if(!(PD_Bit_Depth == 1 || PD_Bit_Depth == 2 || PD_Bit_Depth == 4 || PD_Bit_Depth == 8))
			return PD_PROCESS_ERROR;

		if(PD_Image_Smaller_LCD == PD_TRUE)
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Origin_Indexed_ADAM7;
			else
				PNG_Decode_Image = Image_Origin_Indexed;
		}
		else
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Resize_Indexed_ADAM7;
			else
				PNG_Decode_Image = Image_Resize_Indexed;
		}
		PD_Compo_Num = 1;
		break;
	case PD_COLOR_GREY_ALPHA:
		if(!(PD_Bit_Depth == 8 || PD_Bit_Depth == 16))
			return PD_PROCESS_ERROR;

		if(PD_Image_Smaller_LCD == PD_TRUE)
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Origin_Grey_ADAM7;
			else
				PNG_Decode_Image = Image_Origin_Grey;
		}
		else
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Resize_Grey_ADAM7;
			else
				PNG_Decode_Image = Image_Resize_Grey;
		}
		PD_Alpha_Available = PD_ALPHA_AVAILABLE;
		PD_Compo_Num = 2;
		break;
	case PD_COLOR_TRUE_ALPHA:
		if(!(PD_Bit_Depth == 8 || PD_Bit_Depth == 16))
			return PD_PROCESS_ERROR;

		if(PD_Image_Smaller_LCD == PD_TRUE)
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Origin_True_ADAM7;
			else
				PNG_Decode_Image = Image_Origin_True;
		}
		else
		{
			if(PD_Interlace_Method == PD_INTERLACE_ADAM)
				PNG_Decode_Image = Image_Resize_True_ADAM7;
			else
				PNG_Decode_Image = Image_Resize_True;
		}
		PD_Alpha_Available = PD_ALPHA_AVAILABLE;
		PD_Compo_Num = 4;
		break;
	default :
		return PD_PROCESS_ERROR;
	}

	switch(PD_Bit_Depth)
	{
	case 1:
		PD_Data_Mask = PDRO_Depth_Mask;
		PD_Data_Shift = PDRO_Depth_Shift;
		break;
	case 2:
		PD_Data_Mask = &(PDRO_Depth_Mask[8]);
		PD_Data_Shift = &(PDRO_Depth_Shift[8]);
		break;
	case 4:
		PD_Data_Mask = &(PDRO_Depth_Mask[12]);
		PD_Data_Shift = &(PDRO_Depth_Shift[12]);
		break;
	case 8:
		PD_Data_Mask = &(PDRO_Depth_Mask[14]);
		PD_Data_Shift = &(PDRO_Depth_Shift[14]);
		break;
	default:
		break;
	}

	PD_Scaler = 255 / (((uint32)1 << PD_Bit_Depth) - 1);

	return PNG_Check_CRC();
}

static int PNG_Search_PLTE_Chunk(void)
{
	int iteration = 50;
	uint32 stream_out;

	while(iteration--)
	{
		READWORD(PD_Chunk_Size);
		READWORD(stream_out);

		if( PD_nPngDecErrorCode < 0 ) {
			return PD_PROCESS_ERROR;
		}
		
		switch(stream_out)
		{
		case PD_MARKER_tIME:
		case PD_MARKER_zTXt:
		case PD_MARKER_tEXt:
		case PD_MARKER_iTXt:
		case PD_MARKER_pHYs:
		case PD_MARKER_sPLT:
		case PD_MARKER_iCCP:
		case PD_MARKER_sRGB:
		case PD_MARKER_sBIT:
		case PD_MARKER_gAMA:
		case PD_MARKER_cHRM:	
	#if defined(PNGDEC_ADD_ANCILLARY_CHUNK_EXT120130)
		case PD_MARKER_fRAc:
		case PD_MARKER_gIFg:
		case PD_MARKER_gIFt:
		case PD_MARKER_gIFx:
		case PD_MARKER_oFFs:
		case PD_MARKER_pCAL:
		case PD_MARKER_sCAL:
		case PD_MARKER_sTER:			
	#endif
	#if defined(PNGDEC_SKIP_TRNS_BEFORE_PLTE)
		case PD_MARKER_tRNS:
	#endif
			PNG_Skip_Current_Chunk();
			break;
		case PD_MARKER_IHDR:
		case PD_MARKER_IDAT:
		case PD_MARKER_IEND:
			return PD_PROCESS_ERROR;

		case PD_MARKER_bKGD:
		case PD_MARKER_hIST:
	#if !defined(PNGDEC_SKIP_TRNS_BEFORE_PLTE)
		case PD_MARKER_tRNS:
			return PD_PROCESS_ERROR;
	#endif

		case PD_MARKER_PLTE:
		#if defined(PNGDEC_CHECK_CHUNK)
			PD_nPngDecCheck_Chunk |= ( 1 << PNGDEC_CHUNK_PLTE );
		#endif
			return PD_PROCESS_DONE;
		default:
		#if !defined(PNGDEC_STABILITY_READ_UNKNOWN_ANC_CHUNK_SKIP)
			return PD_PROCESS_ERROR;
		#else
			PNG_Skip_Current_Chunk();
			break;
		#endif
		}
	}
	return PD_PROCESS_ERROR;
}

static int PNG_Parse_PLTE_Chunk(void)
{
	int i;
	PD_Plte_Entry_Num = PD_Chunk_Size / 3;
	
	if(PD_Chunk_Size % 3 != 0)
		return PD_PROCESS_ERROR;

	for(i = 0;i < PD_Plte_Entry_Num;i++)
	{
		READBYTE(PD_Plte[i].R);
		READBYTE(PD_Plte[i].G);
		READBYTE(PD_Plte[i].B);
	}

	return PNG_Check_CRC();
}



//////////////////////
//Decoding Related
//////////////////////

static int PNG_Decode_ZLIB(void)
{
	uint8 stream_out_1;
	uint8 stream_out_2;
	uint16 fcheck;
	uint8 temp;

	NEEDBITS_IDAT(16);
	READBITS(8, stream_out_1);
	
	SHOWBITS(8, stream_out_2);
	DROPBITS(5);

	fcheck = ((stream_out_1 << 8) + stream_out_2) % 31;
	if(fcheck != 0)
		return PD_PROCESS_ERROR;

	if((stream_out_1 & 0x0F) != 8)
		return PD_PROCESS_ERROR;

	PD_Window_Size = 256 << (stream_out_1 >> 4);
	if(PD_Window_Size > 32768)
		return PD_PROCESS_ERROR;

	READBITS(1, temp);
	if(temp)
		return PD_PROCESS_ERROR;
	
	READBITS(2, PD_ZLIB_Flevel);

	if( PD_nPngDecErrorCode < 0 ) {
		return PD_PROCESS_ERROR;
	}

	return PD_PROCESS_DONE;
}

static int PNG_Decode_Block_Header(void)
{
	NEEDBITS_IDAT(3);
	READBITS(1, PD_Last_Block);
	READBITS(2, PD_Deflate_Type);
	if(PD_Deflate_Type == 3)
		return PD_PROCESS_ERROR;

	if( PD_nPngDecErrorCode < 0 )
		return PD_PROCESS_ERROR;
	
	return PD_PROCESS_DONE;
}

static int PNG_Copy_Block(void)
{
	int i;
	int msg_ret;
	int copy_length;
	uint16 temp;
	
	if(PD_Still_Decoding == PD_DONE_ALREADY)
	{
		int temp = PD_Valid_Bit & 7;

		NEEDBITS_IDAT(temp);
		DROPBITS(temp);

		NEEDBITS_IDAT(16);
		READBITS(16, PD_Len2Copy);
		
		NEEDBITS_IDAT(16);
		READBITS(16, temp);
		if(PD_Len2Copy != (temp ^ 0xffff))
			return PD_PROCESS_ERROR;
	}

	QUEUE_PUSH_CHECK(copy_length);

	if(copy_length <= 0)
	{
		msg_ret = PD_PROCESS_DONE;
		PD_Still_Decoding = PD_DONE_YET;
	}
	else
	{
		if(PD_Len2Copy > copy_length)
		{
			PD_Len2Copy -= copy_length;
			msg_ret = PD_PROCESS_DONE;
			PD_Still_Decoding = PD_DONE_YET;
		}
		else
		{
			copy_length = PD_Len2Copy;
			msg_ret = PD_PROCESS_CONTINUE;
			PD_Still_Decoding = PD_DONE_ALREADY;
		}
		while(copy_length >= 512)
		{
			for(i = 0;i < (512 >> 1);i++)
			{
				NEEDBITS_IDAT(16);
				READBITS(16, temp);
				QUEUE_PUSH(temp & 0xFF);
				QUEUE_PUSH(temp >> 8);
			}
			copy_length -= 512;
		}
		for(i = 0;i < copy_length;i++)
		{
			NEEDBITS_IDAT(8);
			READBITS(8, temp);
			QUEUE_PUSH(temp);
		}
	}
	
	return msg_ret;
}

static int PNG_Decode_Block(void)
{
	int i;
	int valid_length;
	uint16 ext;
	uint8 result;
	uint16 temp;
	
	huft * table;

	if(PD_Still_Decoding == PD_DONE_YET)
	{
		QUEUE_PUSH_CHECK(valid_length);
		if(valid_length < PD_Len2Copy)
			return PD_PROCESS_ERROR;
		for(i = 0;i < PD_Len2Copy;i++)
		{
			QUEUE_VISIT(result, PD_Dist2Copy);
			QUEUE_PUSH(result);
		}
	}
	PD_Still_Decoding = PD_DONE_YET;
	while(1)
	{
		#if defined(PNGDEC_CHECK_EOF)
			if( PD_nPngDecErrorCode < 0 ) {
			#if defined(PNGDEC_CHECK_EOF_2)
				if( PD_nPngDecErrorCode == TC_PNGDEC_ERR_STREAM_READING)
					return PD_nPngDecErrorCode;
			#endif
				return PD_PROCESS_ERROR;
			}
		#endif

		QUEUE_PUSH_CHECK(valid_length);
		if(valid_length <= 0)
		{
			PD_Still_Decoding = PD_DONE_ALREADY;
			return PD_PROCESS_DONE;
		}
			
		NEEDBITS_IDAT(PD_Lookup_Bit_Literal);
		SHOWBITS(PD_Lookup_Bit_Literal, temp);
		table = PD_Huff_Liter + temp;
		ext = table->e;
		if(ext > 16)
		{
			do
			{
				if(ext == 99)
					return PD_PROCESS_ERROR;
				DROPBITS(table->b);
				ext -= 16;
				NEEDBITS_IDAT(ext);
				SHOWBITS(ext, temp);
			#if !defined(PNGDEC_MOD_TYPE_STRUCT_HUFT)
				table = table->v.t + temp;
			#else
				table = (huft *)table->v.t + temp;
			#endif
				ext = table ->e;
			}while(ext > 16);
		}
		DROPBITS(table->b);
		
		if(ext == 16)
		{
			QUEUE_PUSH((uint8)(table->v.n));
		}
		else if(ext == 15)
		{
			PD_Still_Decoding = PD_DONE_ALREADY;
			return PD_PROCESS_CONTINUE;
		}
		else
		{
			NEEDBITS_IDAT(ext);
			READBITS(ext, temp);
			PD_Len2Copy = table->v.n + temp;

			NEEDBITS_IDAT(PD_Lookup_Bit_Distance);
			SHOWBITS(PD_Lookup_Bit_Distance, temp);
			table = PD_Huff_Dist + temp;
			ext = table->e;
			if(ext > 16)
			{
				do
				{
					if(ext == 99)
						return PD_PROCESS_ERROR;
					DROPBITS(table->b);
					ext -= 16;
					NEEDBITS_IDAT(ext);
					SHOWBITS(ext, temp);
				#if !defined(PNGDEC_MOD_TYPE_STRUCT_HUFT)
					table = table->v.t + temp;
				#else
					table = (huft *)table->v.t + temp;
				#endif
					ext = table->e;
				}while(ext > 16);
			}
			DROPBITS(table->b);
			
			NEEDBITS_IDAT(ext);
			READBITS(ext, temp);
			PD_Dist2Copy = table->v.n + temp;

			if(PD_Len2Copy > valid_length)
			{
				return PD_PROCESS_DONE;
			}
			else
			{
				for(i = 0;i < PD_Len2Copy;i++)
				{
					QUEUE_VISIT(result, PD_Dist2Copy);
					QUEUE_PUSH(result);
				}
			}
		}
	}
	return PD_PROCESS_DONE;
}


//////////////////////
//Initialization Function
//////////////////////

int TCCXXX_PNGDEC_Init(
				PD_INIT		*pInitInstanceMem,
				PD_CALLBACKS	*callbacks
				)
{
	int msg_ret = 0;	

	PD_LCD_Width = pInitInstanceMem->lcd_width;
	PD_LCD_Height = pInitInstanceMem->lcd_height;
	PD_Datasource = pInitInstanceMem->datasource;

#if defined(PNGDEC_REPORT_BITDEPTH)
	pInitInstanceMem->pixel_depth = 0;
#endif

#if defined(PNGDEC_CHECK_EOF_2)
	PD_TotFileSize = pInitInstanceMem->iTotFileSize;
	if( PD_TotFileSize == 0 )
		return PD_RETURN_INIT_FAIL;

	PD_ReadFileBytes = 0;
	PD_Read_Point_Max = 0;
#endif

	PD_callbacks.read_func = callbacks->read_func;

#if defined(PNGDEC_OPTI_INSTANCE_MEM)
	msg_ret = PNG_Init_instanceMem((char *)pInitInstanceMem->pInstanceBuf, PD_INSTANCE_MEM_SIZE);
	if( msg_ret )
		return PD_RETURN_INIT_FAIL;
#endif
	
	PNG_Init_Variable();

	PNG_Init_IO();
	
	msg_ret = PNG_Check_PNG_Marker();
	if(msg_ret == PD_PROCESS_ERROR)
		return PD_RETURN_INIT_FAIL;
	
	msg_ret = PNG_Parse_IHDR_Chunk(pInitInstanceMem->iOption);
	if(msg_ret == PD_PROCESS_ERROR)
		return PD_RETURN_INIT_FAIL;

	if(PD_Color_Type == PD_COLOR_INDEX)
	{
		msg_ret = PNG_Search_PLTE_Chunk();
		if(msg_ret == PD_PROCESS_ERROR)
		{
			return PD_RETURN_INIT_FAIL;
		}
		else
		{
			msg_ret = PNG_Parse_PLTE_Chunk();
			if(msg_ret == PD_PROCESS_ERROR)
				return PD_RETURN_INIT_FAIL;
		}
	}

	msg_ret = PNG_Search_IDAT_Chunk(0);
	if(msg_ret != PD_PROCESS_DONE)
		return PD_RETURN_INIT_FAIL;

	if(PD_Alpha_Available == PD_ALPHA_AVAILABLE)
		pInitInstanceMem->alpha_available = PD_ALPHA_AVAILABLE;
	else 
		pInitInstanceMem->alpha_available = PD_ALPHA_DISABLE;

	PD_Bpp = ((PD_Bit_Depth * PD_Compo_Num - 1) >> 3) + 1;	// 1,2,3,4,6,8
	PD_Scanline_Size = ((PD_Global_Width * PD_Bit_Depth * PD_Compo_Num - 1) >> 3) + 1;
	PD_Global_Scanline_Size = PD_Scanline_Size + PD_Bpp;

	pInitInstanceMem->image_width = PD_Global_Width;
	pInitInstanceMem->image_height = PD_Global_Height;

	if(PD_Image_Smaller_LCD == PD_TRUE)
	{
		PD_Resized_Width = PD_Global_Width;
		PD_Resized_Height = PD_Global_Height;
	#if !defined(PNGDEC_MOD_MEM_ALIGN)
		pInitInstanceMem->heap_size = (PD_Scanline_Size + PD_Bpp);
	#else 
		pInitInstanceMem->heap_size = (((PD_Scanline_Size + PD_Bpp + 63)>>2)<<2);
	#endif
	}
	else
	{
		int TX,TY;
		TX=(PD_Global_Width << 16) / PD_LCD_Width;
		TY=(PD_Global_Height << 16) / PD_LCD_Height;
		if(TX > TY)	//Resize based on horizontal direction
		{
			PD_Resized_Width = PD_LCD_Width;		
			PD_Resized_Height = (PD_Global_Height << 16) / TX;

		#if defined(PNGDEC_MOD_DIV0)
			PD_Resized_Height = (PD_Global_Height << 16) / TX;
			if( PD_Resized_Height == 0 ) 
				PD_Resized_Height = 1;
		#endif
		}
		else		//Resize based on vertical direction
		{
			PD_Resized_Height = PD_LCD_Height;
			PD_Resized_Width = (PD_Global_Width << 16) / TY;

		#if defined(PNGDEC_MOD_DIV0)
			if( PD_Resized_Width == 0 )
				PD_Resized_Width = 1;
		#endif
		}
	#if !defined(PNGDEC_MOD_MEM_ALIGN)
		pInitInstanceMem->heap_size = (PD_Scanline_Size + PD_Bpp * 2)
									 + PD_Resized_Width * 2 + PD_Resized_Height * 2;
	#else
		pInitInstanceMem->heap_size = (( (PD_Scanline_Size + PD_Bpp * 2)
									    + PD_Resized_Width * 2 + PD_Resized_Height * 2 + 63 
									  )>>2)<<2;
	#endif
	}

	PD_Cur_Job = PD_JOB_DECODE_INIT;
	PD_Out_Struct.RESOURCE_OCCUPATION = 1;

#if defined(PNGDEC_REPORT_BITDEPTH)
	{
		int tpxlDepth = 0;

		switch( PD_Color_Type )
		{
		case 0:	//Greyscale
			tpxlDepth = PD_Bit_Depth;
			break;
		case 2:	//Truecolor
			tpxlDepth = PD_Bit_Depth * 3;
			break;
		case 3:	//Indexed-color
			tpxlDepth = 24;
			break;
		case 4:	//Greyscale with alpah
			tpxlDepth = PD_Bit_Depth * 2;
			break;			
		case 6:	//Truecolor with alpha
			tpxlDepth = PD_Bit_Depth * 4;
			break;
		}
		pInitInstanceMem->pixel_depth = tpxlDepth;
	}
#endif

	return PD_RETURN_INIT_DONE;
}


//////////////////////
//Decoding Function
//////////////////////
int TCCXXX_PNGDEC_Decode(PD_CUSTOM_DECODE *out_info)
{
	int msg_ret;
	int routine_count = PD_Out_Struct.RESOURCE_OCCUPATION;

#if defined(PNGDEC_STABILITY_ERROR_HANDLE)
	#if !defined(PNGDEC_CHECK_EOF_2)
	PD_nPngDecErrorCode = 0; //init.
	#endif
#endif

	while(routine_count)
	{
		switch(PD_Cur_Job)
		{
		case PD_JOB_DECODE_INIT:
		#if defined(PNGDEC_MOD_API20081013)
			PD_Out_Struct = *out_info;
		#else
			PD_Out_Struct = *out_info;
		#endif
			switch(PD_Out_Struct.RESOURCE_OCCUPATION)
			{
			case PD_RESOURCE_LEVEL_NONE:
				PD_Out_Struct.RESOURCE_OCCUPATION = 2;
				break;
			case PD_RESOURCE_LEVEL_LOW:
				PD_Out_Struct.RESOURCE_OCCUPATION = 5;
				break;
			case PD_RESOURCE_LEVEL_MID:
				PD_Out_Struct.RESOURCE_OCCUPATION = 10;
				break;
			case PD_RESOURCE_LEVEL_HIGH:
				PD_Out_Struct.RESOURCE_OCCUPATION = 15;
				break;
			case PD_RESOURCE_LEVEL_ALL:
				PD_Out_Struct.RESOURCE_OCCUPATION = 100;
				break;
			default: // == PD_RESOURCE_LEVEL_ALL
				PD_Out_Struct.RESOURCE_OCCUPATION = 100;
				break;
			}

			routine_count = PD_Out_Struct.RESOURCE_OCCUPATION;
			PNG_Init_Heap();

			if(((PD_Color_Type == PD_COLOR_GREY_ALPHA) ||
				(PD_Color_Type == PD_COLOR_TRUE_ALPHA) ||
				(PD_Alpha_Available == PD_ALPHA_AVAILABLE)) && 
			#if defined(PNGDEC_MOD_API20081013)
				(out_info->USE_ALPHA_DATA == PD_ALPHA_AVAILABLE))
			#else
				(out_info->USE_ALPHA_DATA == PD_ALPHA_AVAILABLE))
			#endif
				PD_Alpha_Use = 1;
			else
				PD_Alpha_Use = 0;
			
			PD_Cur_Job = PD_JOB_DECODE_HEADER;
			break;

		////////////////////////////////////////
		//(1)Search for IDAT Chunk
		////////////////////////////////////////
		case PD_JOB_SEARCH_IDAT:
			msg_ret = PNG_Search_IDAT_Chunk(1);
			if(msg_ret == PD_PROCESS_EOF)
			{
				PD_Last_IDAT = PD_DONE_ALREADY;
				PD_Cur_Job = PD_JOB_DECODE_IMAGE;
			}
			else if(msg_ret == PD_PROCESS_ERROR)
				return PD_RETURN_DECODE_FAIL;
			else
				PD_Cur_Job = PD_JOB_DECODE_HEADER;
			break;

		////////////////////////////////////////
		//(2)Decode ZLIB Header & 3-bit Block Header
		////////////////////////////////////////
		case PD_JOB_DECODE_HEADER:
			msg_ret = PNG_Decode_ZLIB();
			if(msg_ret != PD_PROCESS_DONE)
				return PD_RETURN_DECODE_FAIL;
			PD_Cur_Job = PD_JOB_DECODE_BLOCK_HEADER;
			break;

		case PD_JOB_DECODE_BLOCK_HEADER:
			msg_ret = PNG_Decode_Block_Header();
			if(msg_ret != PD_PROCESS_DONE)
				return PD_RETURN_DECODE_FAIL;
			
			switch(PD_Deflate_Type)
			{
			case PD_DEFLATE_NOCOMP:
				PD_Cur_Job = PD_JOB_DECODE_COPY;
				break;
			case PD_DEFLATE_FIXHUFF:
				if(PD_FixHuff_Done == PD_DONE_YET)
					PD_Cur_Job = PD_JOB_BUILD_FIXHUFF;
				else
					PD_Cur_Job = PD_JOB_DECODE_BLOCK;
				break;
			case PD_DEFLATE_VARHUFF:
				PD_Cur_Job = PD_JOB_BUILD_VARHUFF;
				break;
			default:
				return PD_RETURN_DECODE_FAIL;
			}
			
			break;

		////////////////////////////////////////
		//(3)Build Up Huffman Table
		////////////////////////////////////////
		case PD_JOB_BUILD_FIXHUFF:
			msg_ret = PNG_Generate_FixHuff_Table();
			if(msg_ret != PD_PROCESS_DONE)
				return PD_RETURN_DECODE_FAIL;
			
			PD_Cur_Job = PD_JOB_DECODE_BLOCK;
			break;
		case PD_JOB_BUILD_VARHUFF:
			msg_ret = PNG_Generate_VarHuff_Table();
			if(msg_ret != PD_PROCESS_DONE)
				return PD_RETURN_DECODE_FAIL;
			
			PD_Cur_Job = PD_JOB_DECODE_BLOCK;
			break;

		////////////////////////////////////////
		//(4)Decode Block acorrding to Compression Options
		////////////////////////////////////////
		case PD_JOB_DECODE_COPY:
			msg_ret = PNG_Copy_Block();

			PD_Prev_Job = PD_JOB_DECODE_COPY;
			if( PD_nPngDecErrorCode < 0 ) {
				return PD_PROCESS_ERROR;
			}
		
			if(msg_ret == PD_PROCESS_ERROR)
				return PD_RETURN_DECODE_FAIL;
			else if(msg_ret == PD_PROCESS_CONTINUE)
			{
				if(PD_Last_Block)
				{
					QUEUE_PUSH(PD_FILT_UP);
					PNG_Check_Adler32();
					PNG_Check_CRC();
					PD_Cur_Job = PD_JOB_SEARCH_IDAT;
				}
				else
				{
					PD_Cur_Job = PD_JOB_DECODE_BLOCK_HEADER;
				}
			}
			else
				PD_Cur_Job = PD_JOB_DECODE_IMAGE;
			break;
		case PD_JOB_DECODE_BLOCK:
			msg_ret = PNG_Decode_Block();

			PD_Prev_Job = PD_JOB_DECODE_BLOCK;
			if( PD_nPngDecErrorCode < 0 ) {
			#if defined(PNGDEC_CHECK_EOF_2)
				if( PD_nPngDecErrorCode == TC_PNGDEC_ERR_STREAM_READING )
				{
					PD_Cur_Job = PD_JOB_DECODE_IMAGE;
					break;
				}
			#endif
				return PD_PROCESS_ERROR;
			}
		
			if(msg_ret == PD_PROCESS_ERROR)
				return PD_RETURN_DECODE_FAIL;
			else if(msg_ret == PD_PROCESS_CONTINUE)
			{
				if(PD_Last_Block)
				{
					QUEUE_PUSH(PD_FILT_UP);
					PNG_Check_Adler32();
					PNG_Check_CRC();
					PD_Cur_Job = PD_JOB_SEARCH_IDAT;
				}
				else
				{
					PD_Cur_Job = PD_JOB_DECODE_BLOCK_HEADER;
				}
			}
			else
				PD_Cur_Job = PD_JOB_DECODE_IMAGE;
			break;

		////////////////////////////////////////
		//(4)Decode Image by using Data in Ring-Queue
		////////////////////////////////////////
		case PD_JOB_DECODE_IMAGE:
			msg_ret = PNG_Decode_Image();

			if(msg_ret != PD_PROCESS_DONE)
				return PD_RETURN_DECODE_FAIL;

			if( PD_nPngDecErrorCode < 0 ) {
			#if defined(PNGDEC_CHECK_EOF_2)
				if( PD_nPngDecErrorCode == TC_PNGDEC_ERR_STREAM_READING)
				{
					if(msg_ret != PD_PROCESS_DONE)
					{
						return PD_PROCESS_ERROR;
					}else
					{
						PD_Last_IDAT = PD_DONE_ALREADY;
					}
				}else
			#endif
				{
					return PD_PROCESS_ERROR;
				}
			}

			if(PD_Last_IDAT == PD_DONE_ALREADY)
				return PD_RETURN_DECODE_DONE;

			PD_Cur_Job = PD_Prev_Job;
			break;		
		default:
			return PD_RETURN_DECODE_FAIL;
		}
		
		routine_count--;
	}
	return PD_RETURN_DECODE_PROCESSING;
}



/****************************************************************
	API
****************************************************************/

//////////////////////
//Init. or Decoding Function
//////////////////////
int TCCXXX_PNG_Decode(int iOp, void * pParam1, void * pParam2, int iOption)
{
	int msg_ret = PD_RETURN_DECODE_FAIL;

	switch( iOp )
	{
	case PD_DEC_INIT:
		msg_ret = TCCXXX_PNGDEC_Init( (PD_INIT*)pParam1, (PD_CALLBACKS*)pParam2 );
		break;
	case PD_DEC_DECODE:
		msg_ret = TCCXXX_PNGDEC_Decode( (PD_CUSTOM_DECODE*)pParam1 );
		break;
	default:
		break;
	}

	return msg_ret;
}
