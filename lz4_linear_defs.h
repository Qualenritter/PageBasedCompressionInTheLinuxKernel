#ifndef __LZ4_LINEAR_DEFS_H__
#define __LZ4_LINEAR_DEFS_H__

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

#include "lz4defs.h"
/*-************************************
 *	Reading and writing into memory
 **************************************/
static FORCE_INLINE U16 LZ4_read16 (const void* ptr) {
	return get_unaligned ((const U16*) ptr);
}

static FORCE_INLINE U32 LZ4_read32 (const void* ptr) {
	return get_unaligned ((const U32*) ptr);
}

static FORCE_INLINE size_t LZ4_read_ARCH (const void* ptr) {
	return get_unaligned ((const size_t*) ptr);
}

static FORCE_INLINE void LZ4_write16 (void* memPtr, U16 value) {
	put_unaligned (value, (U16*) memPtr);
}

static FORCE_INLINE void LZ4_write32 (void* memPtr, U32 value) {
	put_unaligned (value, (U32*) memPtr);
}

static FORCE_INLINE U16 LZ4_readLE16 (const void* memPtr) {
	return get_unaligned_le16 (memPtr);
}

static FORCE_INLINE void LZ4_writeLE16 (void* memPtr, U16 value) {
	return put_unaligned_le16 (value, memPtr);
}

static FORCE_INLINE void LZ4_copy8 (void* dst, const void* src) {
#if LZ4_ARCH64
	U64 a = get_unaligned ((const U64*) src);

	put_unaligned (a, (U64*) dst);
#else
	U32 a = get_unaligned ((const U32*) src);
	U32 b = get_unaligned ((const U32*) src + 1);

	put_unaligned (a, (U32*) dst);
	put_unaligned (b, (U32*) dst + 1);
#endif
}

/*
 * customized variant of memcpy,
 * which can overwrite up to 7 bytes beyond dstEnd
 */
static FORCE_INLINE void LZ4_wildCopy (void* dstPtr, const void* srcPtr, void* dstEnd) {
	BYTE*		d = (BYTE*) dstPtr;
	const BYTE* s = (const BYTE*) srcPtr;
	BYTE* const e = (BYTE*) dstEnd;

	do {
		LZ4_copy8 (d, s);
		d += 8;
		s += 8;
	} while (d < e);
}

static FORCE_INLINE unsigned int LZ4_count (const BYTE* pIn, const BYTE* pMatch, const BYTE* pInLimit) {
	const BYTE* const pStart = pIn;

	while (likely (pIn < pInLimit - (STEPSIZE - 1))) {
		size_t const diff = LZ4_read_ARCH (pMatch) ^ LZ4_read_ARCH (pIn);

		if (!diff) {
			pIn += STEPSIZE;
			pMatch += STEPSIZE;
			continue;
		}

		pIn += LZ4_NbCommonBytes (diff);

		return (unsigned int) (pIn - pStart);
	}

#if LZ4_ARCH64
	if ((pIn < (pInLimit - 3)) && (LZ4_read32 (pMatch) == LZ4_read32 (pIn))) {
		pIn += 4;
		pMatch += 4;
	}
#endif

	if ((pIn < (pInLimit - 1)) && (LZ4_read16 (pMatch) == LZ4_read16 (pIn))) {
		pIn += 2;
		pMatch += 2;
	}

	if ((pIn < pInLimit) && (*pMatch == *pIn))
		pIn++;

	return (unsigned int) (pIn - pStart);
}

static FORCE_INLINE U32 LZ4_hashPosition (const void* p, LZ4_tableType_t const tableType) {
#if LZ4_ARCH64
	if (tableType == LZ4_byU32)
		return LZ4_hash5 (LZ4_read_ARCH (p), tableType);
#endif

	return LZ4_hash4 (LZ4_read32 (p), tableType);
}

static void LZ4_putPositionOnHash (const BYTE* p, U32 h, void* tableBase, LZ4_tableType_t const tableType, const BYTE* srcBase) {
	switch (tableType) {
		case LZ4_byPtr: {
			const BYTE** hashTable = (const BYTE**) tableBase;

			hashTable[h] = p;
			return;
		}
		case LZ4_byU32: {
			U32* hashTable = (U32*) tableBase;

			hashTable[h] = (U32) (p - srcBase);
			return;
		}
		case LZ4_byU16: {
			U16* hashTable = (U16*) tableBase;

			hashTable[h] = (U16) (p - srcBase);
			return;
		}
	}
}

static FORCE_INLINE void LZ4_putPosition (const BYTE* p, void* tableBase, LZ4_tableType_t tableType, const BYTE* srcBase) {
	U32 const h = LZ4_hashPosition (p, tableType);

	LZ4_putPositionOnHash (p, h, tableBase, tableType, srcBase);
}

static const BYTE* LZ4_getPositionOnHash (U32 h, void* tableBase, LZ4_tableType_t tableType, const BYTE* srcBase) {
	if (tableType == LZ4_byPtr) {
		const BYTE** hashTable = (const BYTE**) tableBase;

		return hashTable[h];
	}

	if (tableType == LZ4_byU32) {
		const U32* const hashTable = (U32*) tableBase;

		return hashTable[h] + srcBase;
	}

	{
		/* default, to ensure a return */
		const U16* const hashTable = (U16*) tableBase;

		return hashTable[h] + srcBase;
	}
}

static FORCE_INLINE const BYTE* LZ4_getPosition (const BYTE* p, void* tableBase, LZ4_tableType_t tableType, const BYTE* srcBase) {
	U32 const h = LZ4_hashPosition (p, tableType);

	return LZ4_getPositionOnHash (h, tableBase, tableType, srcBase);
}

#endif
