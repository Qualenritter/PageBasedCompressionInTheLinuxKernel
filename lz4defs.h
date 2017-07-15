#ifndef __LZ4DEFS_H__
#define __LZ4DEFS_H__

/*
 * lz4defs.h -- common and architecture specific defines for the kernel usage

 * LZ4 - Fast LZ compression algorithm
 * Copyright (C) 2011-2016, Yann Collet.
 * BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *	* Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 *	* Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * You can contact the author at :
 *	- LZ4 homepage : http://www.lz4.org
 *	- LZ4 source repository : https://github.com/lz4/lz4
 *
 *	Changed for kernel usage by:
 *	Sven Schmidt <4sschmid@informatik.uni-hamburg.de>
 */

#include <asm/unaligned.h>
#include <linux/string.h> /* memset, memcpy */

//#define FORCE_INLINE __always_inline
#define FORCE_INLINE

/*-************************************
 *	Basic Types
 **************************************/
#include <linux/types.h>

/*-************************************
 *	Architecture specifics
 **************************************/
#if defined(CONFIG_64BIT)
#define LZ4_ARCH64 1
#else
#define LZ4_ARCH64 0
#endif

#if defined(__LITTLE_ENDIAN)
#define LZ4_LITTLE_ENDIAN 1
#else
#define LZ4_LITTLE_ENDIAN 0
#endif

/*-************************************
 *	Constants
 **************************************/
#define MINMATCH 4

#define WILDCOPYLENGTH sizeof (size_t)
#define LASTLITERALS 5
#define MFLIMIT (WILDCOPYLENGTH + MINMATCH)

/* Increase this value ==> compression run slower on incompressible data */
#define LZ4_SKIPTRIGGER 6
#define LZ4_SKIPTRIGGER_MASK (1 << (LZ4_SKIPTRIGGER + 1))

#define KB (1 << 10)
#define MB (1 << 20)
#define GB (1U << 30)

#define MAXD_LOG 16
#define MAX_DISTANCE ((1 << MAXD_LOG) - 1)
#define STEPSIZE sizeof (size_t)

#define ML_BITS 4
#define ML_MASK ((1U << ML_BITS) - 1)
#define RUN_BITS (8 - ML_BITS)
#define RUN_MASK ((1U << RUN_BITS) - 1)

#define LZ4_minLength (MFLIMIT + 1)
#define LZ4_64Klimit ((64 * KB) + (MFLIMIT - 1))

typedef enum { LZ4_noLimit = 0, LZ4_limitedOutput = 1 } LZ4_limitedOutput_t;
typedef enum { LZ4_byPtr, LZ4_byU32, LZ4_byU16 } LZ4_tableType_t;

typedef enum { LZ4_noDict = 0, LZ4_withPrefix64k, LZ4_usingExtDict } LZ4_dict_t;
typedef enum { LZ4_noDictIssue = 0, LZ4_dictSmall } LZ4_dictIssue_t;

typedef enum { LZ4_endOnOutputSize = 0, LZ4_endOnInputSize = 1 } LZ4_endCondition_t;
typedef enum { LZ4_full = 0, LZ4_partial = 1 } LZ4_earlyEnd_t;
typedef enum { LZ4_unsafe = 0, LZ4_safe = 1 } LZ4_safe_t;

static FORCE_INLINE unsigned int LZ4_NbCommonBytes (register size_t val) {
#if LZ4_LITTLE_ENDIAN
	return __ffs (val) >> 3;
#else
	return (BITS_PER_LONG - 1 - __fls (val)) >> 3;
#endif
}

static FORCE_INLINE U32 LZ4_hash4 (U32 sequence, LZ4_tableType_t const tableType) {
	if (tableType == LZ4_byU16)
		return ((sequence * 2654435761U) >> ((MINMATCH * 8) - (LZ4_HASHLOG + 1)));
	else
		return ((sequence * 2654435761U) >> ((MINMATCH * 8) - LZ4_HASHLOG));
}

static FORCE_INLINE __maybe_unused U32 LZ4_hash5 (U64 sequence, LZ4_tableType_t const tableType) {
	const U32 hashLog = (tableType == LZ4_byU16) ? LZ4_HASHLOG + 1 : LZ4_HASHLOG;

#if LZ4_LITTLE_ENDIAN
	static const U64 prime5bytes = 889523592379ULL;

	return (U32) (((sequence << 24) * prime5bytes) >> (64 - hashLog));
#else
	static const U64 prime8bytes = 11400714785074694791ULL;

	return (U32) (((sequence >> 24) * prime8bytes) >> (64 - hashLog));
#endif
}

void LZ4_resetStream (LZ4_stream_t* LZ4_stream) {
	memset (LZ4_stream, 0, sizeof (LZ4_stream_t));
}

#endif
