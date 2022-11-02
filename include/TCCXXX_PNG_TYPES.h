// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) Telechips Inc.
 */

#ifndef TYPES_DONE
#define TYPES_DONE

#ifdef _WIN32_WCE
#include <windows.h>
#define int64		signed __int64
#define uint64		unsigned __int64
#else
#define int64		signed long int
#define uint64		unsigned long int

#define BYTE		unsigned char
#endif

#define int8		signed char
#define uint8		unsigned char
#define int16		signed short
#define uint16		unsigned short
#define int32		signed int
#define uint32		unsigned int

#ifndef NULL
#define NULL 0
#endif

#endif
