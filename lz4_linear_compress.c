/*
 * LZ4 - Fast LZ compression algorithm
 * Copyright (C) 2011 - 2016, Yann Collet.
 * BSD 2 - Clause License (http://www.opensource.org/licenses/bsd - license.php)
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

/*-************************************
 *	Dependencies
 **************************************/
#include "lz4.h"
#include "lz4_linear_defs.h"
#include <asm/unaligned.h>
#include <linux/kernel.h>
#include <linux/module.h>

/*-******************************
 *	Compression functions
 ********************************/

/*
 * LZ4_compress_generic() :
 * inlined, to ensure branches are decided at compilation time
 */
static FORCE_INLINE int LZ4_compress_generic (LZ4_stream_t_internal* const dictPtr,
											  const char* const			   source,
											  char* const				   dest,
											  const int					   inputSize,
											  const int					   maxOutputSize,
											  const LZ4_limitedOutput_t	outputLimited,
											  const LZ4_tableType_t		   tableType,
											  const LZ4_dict_t			   dict,
											  const LZ4_dictIssue_t		   dictIssue,
											  const U32					   acceleration) {
	const BYTE*		  ip = (const BYTE*) source;
	const BYTE*		  base;
	const BYTE*		  lowLimit;
	const BYTE* const lowRefLimit = ip - dictPtr->dictSize;
	const BYTE* const dictionary  = dictPtr->dictionary;
	const BYTE* const dictEnd	 = dictionary + dictPtr->dictSize;
	const size_t	  dictDelta   = dictEnd - (const BYTE*) source;
	const BYTE*		  anchor	  = (const BYTE*) source;
	const BYTE* const iend		  = ip + inputSize;
	const BYTE* const mflimit	 = iend - MFLIMIT;
	const BYTE* const matchlimit  = iend - LASTLITERALS;

	BYTE*		op	 = (BYTE*) dest;
	BYTE* const olimit = op + maxOutputSize;

	U32	forwardH;
	size_t refDelta = 0;

	/* Init conditions */
	if ((U32) inputSize > (U32) LZ4_MAX_INPUT_SIZE) {
		/* Unsupported inputSize, too large (or negative) */
		return 0;
	}

	if (dict == LZ4_withPrefix64k) {
		base	 = (const BYTE*) source - dictPtr->currentOffset;
		lowLimit = (const BYTE*) source - dictPtr->dictSize;
	} else if (dict == LZ4_usingExtDict) {
		base	 = (const BYTE*) source - dictPtr->currentOffset;
		lowLimit = (const BYTE*) source;
	} else {
		base	 = (const BYTE*) source;
		lowLimit = (const BYTE*) source;
	}

	if ((tableType == LZ4_byU16) && (inputSize >= LZ4_64Klimit)) {
		/* Size too large (not within 64K limit) */
		return 0;
	}

	if (inputSize < LZ4_minLength) {
		/* Input too small, no compression (all literals) */
		goto _last_literals;
	}

	/* First Byte */
	LZ4_putPosition (ip, dictPtr->hashTable, tableType, base);
	ip++;
	forwardH = LZ4_hashPosition (ip, tableType);

	/* Main Loop */
	for (;;) {
		const BYTE* match;
		BYTE*		token;

		/* Find a match */
		{
			const BYTE*  forwardIp	 = ip;
			unsigned int step		   = 1;
			unsigned int searchMatchNb = acceleration << LZ4_SKIPTRIGGER;

			do {
				U32 const h = forwardH;

				ip = forwardIp;
				forwardIp += step;
				step = (searchMatchNb++ >> LZ4_SKIPTRIGGER);

				if (unlikely (forwardIp > mflimit))
					goto _last_literals;

				match = LZ4_getPositionOnHash (h, dictPtr->hashTable, tableType, base);

				if (dict == LZ4_usingExtDict) {
					if (match < (const BYTE*) source) {
						refDelta = dictDelta;
						lowLimit = dictionary;
					} else {
						refDelta = 0;
						lowLimit = (const BYTE*) source;
					}
				}

				forwardH = LZ4_hashPosition (forwardIp, tableType);

				LZ4_putPositionOnHash (ip, h, dictPtr->hashTable, tableType, base);
			} while (((dictIssue == LZ4_dictSmall) ? (match < lowRefLimit) : 0) || ((tableType == LZ4_byU16) ? 0 : (match + MAX_DISTANCE < ip)) ||
					 (LZ4_read32 (match + refDelta) != LZ4_read32 (ip)));
		}

		/* Catch up */
		while (((ip > anchor) & (match + refDelta > lowLimit)) && (unlikely (ip[-1] == match[refDelta - 1]))) {
			ip--;
			match--;
		}

		/* Encode Literals */
		{ /* removed code */ }

	_next_match:
		/* Encode Offset */
		/* removed code */

		/* Encode MatchLength */
		{
			unsigned int matchCode;

			if ((dict == LZ4_usingExtDict) && (lowLimit == dictionary)) {
				const BYTE* limit;

				match += refDelta;
				limit = ip + (dictEnd - match);

				if (limit > matchlimit)
					limit = matchlimit;

				matchCode = LZ4_count (ip + MINMATCH, match + MINMATCH, limit);

				ip += MINMATCH + matchCode;

				if (ip == limit) {
					unsigned const int more = LZ4_count (ip, (const BYTE*) source, matchlimit);

					matchCode += more;
					ip += more;
				}
			} else {
				matchCode = LZ4_count (ip + MINMATCH, match + MINMATCH, matchlimit);
				ip += MINMATCH + matchCode;
			}

			/* removed code */
		}

		anchor = ip;

		/* Test end of chunk */
		if (ip > mflimit)
			break;

		/* Fill table */
		LZ4_putPosition (ip - 2, dictPtr->hashTable, tableType, base);

		/* Test next position */
		match = LZ4_getPosition (ip, dictPtr->hashTable, tableType, base);

		if (dict == LZ4_usingExtDict) {
			if (match < (const BYTE*) source) {
				refDelta = dictDelta;
				lowLimit = dictionary;
			} else {
				refDelta = 0;
				lowLimit = (const BYTE*) source;
			}
		}

		LZ4_putPosition (ip, dictPtr->hashTable, tableType, base);

		if (((dictIssue == LZ4_dictSmall) ? (match >= lowRefLimit) : 1) && (match + MAX_DISTANCE >= ip) && (LZ4_read32 (match + refDelta) == LZ4_read32 (ip))) {
			/* removed code */
			goto _next_match;
		}

		/* Prepare next loop */
		forwardH = LZ4_hashPosition (++ip, tableType);
	}

_last_literals:
	/* Encode Last Literals */
	{ /* removed code */ }

	/* End */
	return 0;
}

static int LZ4_compress_fast_extState (void* state, const char* source, char* dest, int inputSize, int maxOutputSize, int acceleration) {
	LZ4_stream_t_internal* ctx = &((LZ4_stream_t*) state)->internal_donotuse;
#if LZ4_ARCH64
	const LZ4_tableType_t tableType = LZ4_byU32;
#else
	const LZ4_tableType_t tableType = LZ4_byPtr;
#endif

	LZ4_resetStream ((LZ4_stream_t*) state);

	if (acceleration < 1)
		acceleration = LZ4_ACCELERATION_DEFAULT;

	if (maxOutputSize >= LZ4_COMPRESSBOUND (inputSize)) {
		if (inputSize < LZ4_64Klimit)
			return LZ4_compress_generic (ctx, source, dest, inputSize, 0, LZ4_noLimit, LZ4_byU16, LZ4_noDict, LZ4_noDictIssue, acceleration);
		else
			return LZ4_compress_generic (ctx, source, dest, inputSize, 0, LZ4_noLimit, tableType, LZ4_noDict, LZ4_noDictIssue, acceleration);
	} else {
		if (inputSize < LZ4_64Klimit)
			return LZ4_compress_generic (ctx, source, dest, inputSize, maxOutputSize, LZ4_limitedOutput, LZ4_byU16, LZ4_noDict, LZ4_noDictIssue, acceleration);
		else
			return LZ4_compress_generic (ctx, source, dest, inputSize, maxOutputSize, LZ4_limitedOutput, tableType, LZ4_noDict, LZ4_noDictIssue, acceleration);
	}
}

int LZ4_compress_fast (const char* source, char* dest, int inputSize, int maxOutputSize, int acceleration, void* wrkmem) {
	return LZ4_compress_fast_extState (wrkmem, source, dest, inputSize, maxOutputSize, acceleration);
}
EXPORT_SYMBOL (LZ4_compress_fast);

int benjamin_debug_asm_compress_fast_linear_LZ4 (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration) {
	LZ4_stream_t_internal* ctx		 = &((LZ4_stream_t*) wrkmem)->internal_donotuse;
	const LZ4_tableType_t  tableType = LZ4_byU32;

	LZ4_resetStream ((LZ4_stream_t*) wrkmem);

	if (acceleration < 1)
		acceleration = LZ4_ACCELERATION_DEFAULT;

	return LZ4_compress_generic (ctx, source, dest, source_length, 0, LZ4_noLimit, tableType, LZ4_noDict, LZ4_noDictIssue, acceleration);
}
EXPORT_SYMBOL (benjamin_debug_asm_compress_fast_linear_LZ4);

int LZ4_compress_default (const char* source, char* dest, int inputSize, int maxOutputSize, void* wrkmem) {
	return LZ4_compress_fast (source, dest, inputSize, maxOutputSize, LZ4_ACCELERATION_DEFAULT, wrkmem);
}
EXPORT_SYMBOL (LZ4_compress_default);

/*-******************************
 *	*_destSize() variant
 ********************************/
static int LZ4_compress_destSize_generic (LZ4_stream_t_internal* const ctx, const char* const src, char* const dst, int* const srcSizePtr, const int targetDstSize, const LZ4_tableType_t tableType) {
	const BYTE*		  ip		 = (const BYTE*) src;
	const BYTE*		  base		 = (const BYTE*) src;
	const BYTE*		  lowLimit   = (const BYTE*) src;
	const BYTE*		  anchor	 = ip;
	const BYTE* const iend		 = ip + *srcSizePtr;
	const BYTE* const mflimit	= iend - MFLIMIT;
	const BYTE* const matchlimit = iend - LASTLITERALS;

	BYTE*		op		= (BYTE*) dst;
	BYTE* const oend	= op + targetDstSize;
	BYTE* const oMaxLit = op + targetDstSize - 2 /* offset */
						  - 8 /* because 8 + MINMATCH == MFLIMIT */ - 1 /* token */;
	BYTE* const oMaxMatch = op + targetDstSize - (LASTLITERALS + 1 /* token */);
	BYTE* const oMaxSeq   = oMaxLit - 1 /* token */;

	U32 forwardH;

	/* Init conditions */
	/* Impossible to store anything */
	if (targetDstSize < 1)
		return 0;
	/* Unsupported input size, too large (or negative) */
	if ((U32) *srcSizePtr > (U32) LZ4_MAX_INPUT_SIZE)
		return 0;
	/* Size too large (not within 64K limit) */
	if ((tableType == LZ4_byU16) && (*srcSizePtr >= LZ4_64Klimit))
		return 0;
	/* Input too small, no compression (all literals) */
	if (*srcSizePtr < LZ4_minLength)
		goto _last_literals;

	/* First Byte */
	*srcSizePtr = 0;
	LZ4_putPosition (ip, ctx->hashTable, tableType, base);
	ip++;
	forwardH = LZ4_hashPosition (ip, tableType);

	/* Main Loop */
	for (;;) {
		const BYTE* match;
		BYTE*		token;

		/* Find a match */
		{
			const BYTE*  forwardIp	 = ip;
			unsigned int step		   = 1;
			unsigned int searchMatchNb = 1 << LZ4_SKIPTRIGGER;

			do {
				U32 h = forwardH;

				ip = forwardIp;
				forwardIp += step;
				step = (searchMatchNb++ >> LZ4_SKIPTRIGGER);

				if (unlikely (forwardIp > mflimit))
					goto _last_literals;

				match	= LZ4_getPositionOnHash (h, ctx->hashTable, tableType, base);
				forwardH = LZ4_hashPosition (forwardIp, tableType);
				LZ4_putPositionOnHash (ip, h, ctx->hashTable, tableType, base);

			} while (((tableType == LZ4_byU16) ? 0 : (match + MAX_DISTANCE < ip)) || (LZ4_read32 (match) != LZ4_read32 (ip)));
		}

		/* Catch up */
		while ((ip > anchor) && (match > lowLimit) && (unlikely (ip[-1] == match[-1]))) {
			ip--;
			match--;
		}

		/* Encode Literal length */
		{
			unsigned int litLength = (unsigned int) (ip - anchor);

			token = op++;
			if (op + ((litLength + 240) / 255) + litLength > oMaxLit) {
				/* Not enough space for a last match */
				op--;
				goto _last_literals;
			}
			if (litLength >= RUN_MASK) {
				unsigned int len = litLength - RUN_MASK;
				*token			 = (RUN_MASK << ML_BITS);
				for (; len >= 255; len -= 255)
					*op++ = 255;
				*op++	 = (BYTE) len;
			} else
				*token = (BYTE) (litLength << ML_BITS);

			/* Copy Literals */
			LZ4_wildCopy (op, anchor, op + litLength);
			op += litLength;
		}

	_next_match:
		/* Encode Offset */
		LZ4_writeLE16 (op, (U16) (ip - match));
		op += 2;

		/* Encode MatchLength */
		{
			size_t matchLength = LZ4_count (ip + MINMATCH, match + MINMATCH, matchlimit);

			if (op + ((matchLength + 240) / 255) > oMaxMatch) {
				/* Match description too long : reduce it */
				matchLength = (15 - 1) + (oMaxMatch - op) * 255;
			}
			ip += MINMATCH + matchLength;

			if (matchLength >= ML_MASK) {
				*token += ML_MASK;
				matchLength -= ML_MASK;
				while (matchLength >= 255) {
					matchLength -= 255;
					*op++ = 255;
				}
				*op++ = (BYTE) matchLength;
			} else
				*token += (BYTE) (matchLength);
		}

		anchor = ip;

		/* Test end of block */
		if (ip > mflimit)
			break;
		if (op > oMaxSeq)
			break;

		/* Fill table */
		LZ4_putPosition (ip - 2, ctx->hashTable, tableType, base);

		/* Test next position */
		match = LZ4_getPosition (ip, ctx->hashTable, tableType, base);
		LZ4_putPosition (ip, ctx->hashTable, tableType, base);

		if ((match + MAX_DISTANCE >= ip) && (LZ4_read32 (match) == LZ4_read32 (ip))) {
			token  = op++;
			*token = 0;
			goto _next_match;
		}

		/* Prepare next loop */
		forwardH = LZ4_hashPosition (++ip, tableType);
	}

_last_literals:
	/* Encode Last Literals */
	{
		size_t lastRunSize = (size_t) (iend - anchor);

		if (op + 1 /* token */
				+
				((lastRunSize + 240) / 255) /* litLength */
				+
				lastRunSize /* literals */
			> oend) {
			/* adapt lastRunSize to fill 'dst' */
			lastRunSize = (oend - op) - 1;
			lastRunSize -= (lastRunSize + 240) / 255;
		}
		ip = anchor + lastRunSize;

		if (lastRunSize >= RUN_MASK) {
			size_t accumulator = lastRunSize - RUN_MASK;

			*op++ = RUN_MASK << ML_BITS;
			for (; accumulator >= 255; accumulator -= 255)
				*op++ = 255;
			*op++	 = (BYTE) accumulator;
		} else {
			*op++ = (BYTE) (lastRunSize << ML_BITS);
		}
		memcpy (op, anchor, lastRunSize);
		op += lastRunSize;
	}

	/* End */
	*srcSizePtr = (int) (((const char*) ip) - src);
	return (int) (((char*) op) - dst);
}

static int LZ4_compress_destSize_extState (LZ4_stream_t* state, const char* src, char* dst, int* srcSizePtr, int targetDstSize) {
#if LZ4_ARCH64
	const LZ4_tableType_t tableType = LZ4_byU32;
#else
	const LZ4_tableType_t tableType = LZ4_byPtr;
#endif

	LZ4_resetStream (state);

	if (targetDstSize >= LZ4_COMPRESSBOUND (*srcSizePtr)) {
		/* compression success is guaranteed */
		return LZ4_compress_fast_extState (state, src, dst, *srcSizePtr, targetDstSize, 1);
	} else {
		if (*srcSizePtr < LZ4_64Klimit)
			return LZ4_compress_destSize_generic (&state->internal_donotuse, src, dst, srcSizePtr, targetDstSize, LZ4_byU16);
		else
			return LZ4_compress_destSize_generic (&state->internal_donotuse, src, dst, srcSizePtr, targetDstSize, tableType);
	}
}

int LZ4_compress_destSize (const char* src, char* dst, int* srcSizePtr, int targetDstSize, void* wrkmem) {
	return LZ4_compress_destSize_extState (wrkmem, src, dst, srcSizePtr, targetDstSize);
}
EXPORT_SYMBOL (LZ4_compress_destSize);

/*-******************************
 *	Streaming functions
 ********************************/

#define HASH_UNIT sizeof (size_t)
int LZ4_loadDict (LZ4_stream_t* LZ4_dict, const char* dictionary, int dictSize) {
	LZ4_stream_t_internal* dict	= &LZ4_dict->internal_donotuse;
	const BYTE*			   p	   = (const BYTE*) dictionary;
	const BYTE* const	  dictEnd = p + dictSize;
	const BYTE*			   base;

	if ((dict->initCheck) || (dict->currentOffset > 1 * GB)) {
		/* Uninitialized structure, or reuse overflow */
		LZ4_resetStream (LZ4_dict);
	}

	if (dictSize < (int) HASH_UNIT) {
		dict->dictionary = NULL;
		dict->dictSize   = 0;
		return 0;
	}

	if ((dictEnd - p) > 64 * KB)
		p = dictEnd - 64 * KB;
	dict->currentOffset += 64 * KB;
	base			 = p - dict->currentOffset;
	dict->dictionary = p;
	dict->dictSize   = (U32) (dictEnd - p);
	dict->currentOffset += dict->dictSize;

	while (p <= dictEnd - HASH_UNIT) {
		LZ4_putPosition (p, dict->hashTable, LZ4_byU32, base);
		p += 3;
	}

	return dict->dictSize;
}
EXPORT_SYMBOL (LZ4_loadDict);

static void LZ4_renormDictT (LZ4_stream_t_internal* LZ4_dict, const BYTE* src) {
	if ((LZ4_dict->currentOffset > 0x80000000) || ((uptrval) LZ4_dict->currentOffset > (uptrval) src)) {
		/* address space overflow */
		/* rescale hash table */
		U32 const   delta   = LZ4_dict->currentOffset - 64 * KB;
		const BYTE* dictEnd = LZ4_dict->dictionary + LZ4_dict->dictSize;
		int			i;

		for (i = 0; i < LZ4_HASH_SIZE_U32; i++) {
			if (LZ4_dict->hashTable[i] < delta)
				LZ4_dict->hashTable[i] = 0;
			else
				LZ4_dict->hashTable[i] -= delta;
		}
		LZ4_dict->currentOffset = 64 * KB;
		if (LZ4_dict->dictSize > 64 * KB)
			LZ4_dict->dictSize = 64 * KB;
		LZ4_dict->dictionary   = dictEnd - LZ4_dict->dictSize;
	}
}

int LZ4_saveDict (LZ4_stream_t* LZ4_dict, char* safeBuffer, int dictSize) {
	LZ4_stream_t_internal* const dict			 = &LZ4_dict->internal_donotuse;
	const BYTE* const			 previousDictEnd = dict->dictionary + dict->dictSize;

	if ((U32) dictSize > 64 * KB) {
		/* useless to define a dictionary > 64 * KB */
		dictSize = 64 * KB;
	}
	if ((U32) dictSize > dict->dictSize)
		dictSize = dict->dictSize;

	memmove (safeBuffer, previousDictEnd - dictSize, dictSize);

	dict->dictionary = (const BYTE*) safeBuffer;
	dict->dictSize   = (U32) dictSize;

	return dictSize;
}
EXPORT_SYMBOL (LZ4_saveDict);

int LZ4_compress_fast_continue (LZ4_stream_t* LZ4_stream, const char* source, char* dest, int inputSize, int maxOutputSize, int acceleration) {
	LZ4_stream_t_internal* streamPtr = &LZ4_stream->internal_donotuse;
	const BYTE* const	  dictEnd   = streamPtr->dictionary + streamPtr->dictSize;

	const BYTE* smallest = (const BYTE*) source;

	if (streamPtr->initCheck) {
		/* Uninitialized structure detected */
		return 0;
	}

	if ((streamPtr->dictSize > 0) && (smallest > dictEnd))
		smallest = dictEnd;

	LZ4_renormDictT (streamPtr, smallest);

	if (acceleration < 1)
		acceleration = LZ4_ACCELERATION_DEFAULT;

	/* Check overlapping input/dictionary space */
	{
		const BYTE* sourceEnd = (const BYTE*) source + inputSize;

		if ((sourceEnd > streamPtr->dictionary) && (sourceEnd < dictEnd)) {
			streamPtr->dictSize = (U32) (dictEnd - sourceEnd);
			if (streamPtr->dictSize > 64 * KB)
				streamPtr->dictSize = 64 * KB;
			if (streamPtr->dictSize < 4)
				streamPtr->dictSize = 0;
			streamPtr->dictionary   = dictEnd - streamPtr->dictSize;
		}
	}

	/* prefix mode : source data follows dictionary */
	if (dictEnd == (const BYTE*) source) {
		int result;

		if ((streamPtr->dictSize < 64 * KB) && (streamPtr->dictSize < streamPtr->currentOffset)) {
			result = LZ4_compress_generic (streamPtr, source, dest, inputSize, maxOutputSize, LZ4_limitedOutput, LZ4_byU32, LZ4_withPrefix64k, LZ4_dictSmall, acceleration);
		} else {
			result = LZ4_compress_generic (streamPtr, source, dest, inputSize, maxOutputSize, LZ4_limitedOutput, LZ4_byU32, LZ4_withPrefix64k, LZ4_noDictIssue, acceleration);
		}
		streamPtr->dictSize += (U32) inputSize;
		streamPtr->currentOffset += (U32) inputSize;
		return result;
	}

	/* external dictionary mode */
	{
		int result;

		if ((streamPtr->dictSize < 64 * KB) && (streamPtr->dictSize < streamPtr->currentOffset)) {
			result = LZ4_compress_generic (streamPtr, source, dest, inputSize, maxOutputSize, LZ4_limitedOutput, LZ4_byU32, LZ4_usingExtDict, LZ4_dictSmall, acceleration);
		} else {
			result = LZ4_compress_generic (streamPtr, source, dest, inputSize, maxOutputSize, LZ4_limitedOutput, LZ4_byU32, LZ4_usingExtDict, LZ4_noDictIssue, acceleration);
		}
		streamPtr->dictionary = (const BYTE*) source;
		streamPtr->dictSize   = (U32) inputSize;
		streamPtr->currentOffset += (U32) inputSize;
		return result;
	}
}
EXPORT_SYMBOL (LZ4_compress_fast_continue);

MODULE_LICENSE ("Dual BSD/GPL");
MODULE_DESCRIPTION ("LZ4 compressor");
