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
 *	Changed for page based buffers usage by:
 *	Benjamin Warnke <4bwarnke@informatik.uni-hamburg.de>
 */

#include "lz4.h"
#include "lz4_pages_defs.h"
#include <asm/unaligned.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE ("Dual BSD/GPL");
MODULE_DESCRIPTION ("LZ4 page based compressor");
MODULE_AUTHOR ("Benjamin Warnke");
MODULE_VERSION ("1.0.0");

/**
 * LZ4_pages_hash4() - calculate the hash for a given data sequence
 * @sequence: 32 bit data to hash
 * @tableType: modifier for the hash calculation
 */
static FORCE_INLINE U32 LZ4_pages_hash4 (U32 sequence, LZ4_tableType_t const tableType) {
	if (tableType == LZ4_byU16)
		return ((sequence * 2654435761U) >> ((MINMATCH * 8) - (LZ4_HASHLOG + 1)));
	else
		return ((sequence * 2654435761U) >> ((MINMATCH * 8) - LZ4_HASHLOG));
}

/**
 * LZ4_pages_hash5() - calculate the hash for a given data sequence
 * @sequence: 64 bit data to hash
 * @tableType: modifier for the hash calculation
 */
static FORCE_INLINE __maybe_unused U32 LZ4_pages_hash5 (U64 sequence) {
#if LZ4_LITTLE_ENDIAN
	return (U32) (((sequence << 24) * 889523592379ULL) >> (64 - LZ4_HASHLOG));
#else
	return (U32) (((sequence >> 24) * 11400714785074694791ULL) >> (64 - LZ4_HASHLOG));
#endif
}

/**
 * LZ4_pages_putPosition() - put data in the hash table
 */
static FORCE_INLINE void LZ4_pages_putPosition (const int p_offset, U32 h, void* tableBase, LZ4_tableType_t tableType) {
	if (tableType == LZ4_byU32) {
		U32* const hashTable = (U32*) tableBase;
		hashTable[h]		 = (U32) (p_offset);
		return;
	}
	if (tableType == LZ4_byU16) {
		U16* const hashTable = (U16*) tableBase;
		hashTable[h]		 = (U16) (p_offset);
		return;
	}
	printk ("LZ4_pages_putPositionOnHash tableType %d not supported", tableType);
	return;
}

/**
 * LZ4_pages_updatePositionOnHash() - get and update data in the hash table
 */
static FORCE_INLINE int LZ4_pages_updatePositionOnHash (const int p_offset, U32 h, void* tableBase, LZ4_tableType_t const tableType) {
	int res = 0;
	if (tableType == LZ4_byU32) {
		U32* const hashTable = (U32*) tableBase;
		res					 = hashTable[h];
		hashTable[h]		 = (U32) (p_offset);
		return res;
	}
	if (tableType == LZ4_byU16) {
		U16* const hashTable = (U16*) tableBase;
		res					 = hashTable[h];
		hashTable[h]		 = (U16) (p_offset);
		return res;
	}
	printk ("LZ4_pages_updatePositionOnHash tableType %d not supported", tableType);
	return res;
}

static FORCE_INLINE int LZ4_pages_compress_generic (LZ4_stream_t_internal* const dictPtr,
													BYTE**						 source,
													BYTE**						 dest,
													const int					 inputSize,
													const int					 maxOutputSize,
													const LZ4_limitedOutput_t	outputLimited,
													const LZ4_tableType_t		 tableType,
													const U32					 acceleration) {
	page_access_helper in_helper_ip;
	page_access_helper in_helper_match;
	page_access_helper in_helper_anchor;
	size_t			   forwardIp;
	unsigned int	   step;
	unsigned int	   searchMatchNb;
	unsigned int	   searchMatchNbLimit;
	page_access_helper out_helper;
	size_t			   match_length;
	size_t			   match_length_tmp;
	BYTE			   match_length_masked;
	size_t			   match_length_div_255;
	size_t			   match_length_mod_255;
	size_t			   literal_length;
	size_t			   literal_length_tmp;
	BYTE			   literal_length_masked;
	size_t			   literal_length_div_255;
	size_t			   literal_length_mod_255;
	U32				   forwardH;
	U32				   h;
	BYTE			   data8	   = 0;
	U32				   data32	  = 0;
	U32				   data32cache = 0;
#if LZ4_ARCH64
	U64			 data64			 = 0;
	const size_t hash_byte_count = (tableType == LZ4_byU32) ? 8 : 4;
#else
	const size_t hash_byte_count = 4;
#endif
	const size_t mflimit				 = inputSize - MFLIMIT;
	const size_t matchlimit				 = inputSize - LASTLITERALS;
	const size_t mflimit_hash_byte_count = mflimit + hash_byte_count;

	LZ4_pages_helper_init (&out_helper, dest);
	LZ4_pages_helper_init (&in_helper_ip, source);
	LZ4_pages_helper_init (&in_helper_match, source);
	LZ4_pages_helper_init (&in_helper_anchor, source);
#ifdef LOG_STATISTICS
	if (open_log_file (LOG_FILE_STATISTIC (B_COMPRESS_DATA_TYPE) "pages.csv"))
		goto _error;
#endif
	/* Init conditions */
	if ((U32) inputSize > (U32) LZ4_MAX_INPUT_SIZE) {
		/* Unsupported inputSize, too large (or negative) */
		goto _error;
	}
	if ((tableType == LZ4_byU16) && (inputSize >= LZ4_64Klimit)) {
		/* Size too large (not within 64K limit) */
		goto _error;
	}
	if (tableType == LZ4_byPtr) {
		/*LZ4 tableType (byPtr) unsupported for page based compression */
		goto _error;
	}
	if (inputSize < LZ4_minLength) {
		/* Input too small, no compression (all literals) */
		goto _last_literals;
	}
/* First Byte */
#if LZ4_ARCH64
	if (tableType == LZ4_byU32) {
		data64 = LZ4_pages_read64_continue (&in_helper_ip);
		LZ4_pages_helper_seek (&in_helper_ip, -7);
		forwardH = LZ4_pages_hash5 (data64);
	} else
#endif
	{
		data32 = LZ4_pages_read32_continue (&in_helper_ip);
		LZ4_pages_helper_seek (&in_helper_ip, -3);
		forwardH = LZ4_pages_hash4 (data32, tableType);
	}
	LZ4_pages_putPosition (0, forwardH, dictPtr->hashTable, tableType);
	/* Main Loop */
	for (;;) {
		/* Find a match */
		{
			searchMatchNb = acceleration << LZ4_SKIPTRIGGER;
#if LZ4_ARCH64
			if (tableType == LZ4_byU32) {
				data64		= LZ4_pages_read64_continue (&in_helper_ip);
				data32cache = data64;
			} else
#endif
			{
				data32		= LZ4_pages_read32_continue (&in_helper_ip);
				data32cache = data32;
			}
			if (acceleration == 1) {
				while (searchMatchNb <= LZ4_SKIPTRIGGER_MASK) {
					searchMatchNbLimit = LZ4_pages_helper_available (&in_helper_ip) + searchMatchNb - 1;
					searchMatchNbLimit = LZ4_SKIPTRIGGER_MASK < searchMatchNbLimit ? LZ4_SKIPTRIGGER_MASK : searchMatchNbLimit;
					while (searchMatchNb <= searchMatchNbLimit) {
						if (unlikely (in_helper_ip.offset >= mflimit_hash_byte_count))
							goto _last_literals_seek;
						h = LZ4_pages_updatePositionOnHash (in_helper_ip.offset - hash_byte_count, forwardH, dictPtr->hashTable, tableType);
						LZ4_pages_helper_set_offset (&in_helper_match, h);
						if (((tableType == LZ4_byU16) ? 1 : (in_helper_match.offset + MAX_DISTANCE + hash_byte_count >= in_helper_ip.offset)) &&
							(LZ4_pages_read32_continue (&in_helper_match) == data32cache))
							goto _catch_up;
						data8 = LZ4_pages_bundle_read8_continue (&in_helper_ip);
#if LZ4_ARCH64
						if (tableType == LZ4_byU32) {
							data64		= (data64 >> 8) | (((U64) data8) << 56);
							data32cache = data64;
							forwardH	= LZ4_pages_hash5 (data64);
						} else
#endif
						{
							data32		= (data32 >> 8) | (((U32) data8) << 24);
							data32cache = data32;
							forwardH	= LZ4_pages_hash4 (data32, tableType);
						}
						searchMatchNb++;
					}
					if (in_helper_ip.page_pointer_end_byte_t == in_helper_ip.page_pointer) {
						LZ4_pages_helper_page_forward (&in_helper_ip);
					}
				}
			}
			forwardIp = in_helper_ip.offset - hash_byte_count;
			step	  = (searchMatchNb >> LZ4_SKIPTRIGGER);
			for (;;) {
				forwardIp += step;
				if (unlikely (forwardIp > mflimit))
					goto _last_literals_seek;
				data32 = LZ4_pages_updatePositionOnHash (in_helper_ip.offset - hash_byte_count, forwardH, dictPtr->hashTable, tableType);
				LZ4_pages_helper_set_offset (&in_helper_match, data32);
				if (((tableType == LZ4_byU16) ? 1 : (in_helper_match.offset + MAX_DISTANCE + hash_byte_count >= in_helper_ip.offset)) &&
					(LZ4_pages_read32_continue (&in_helper_match) == data32cache))
					goto _catch_up;
				LZ4_pages_helper_set_offset (&in_helper_ip, forwardIp);
#if LZ4_ARCH64
				if (tableType == LZ4_byU32) {
					data64		= LZ4_pages_read64_continue (&in_helper_ip);
					data32cache = data64;
					forwardH	= LZ4_pages_hash5 (data64);
				} else
#endif
				{
					data32		= LZ4_pages_read32_continue (&in_helper_ip);
					data32cache = data32;
					forwardH	= LZ4_pages_hash4 (data32, tableType);
				}
				step = (searchMatchNb++ >> LZ4_SKIPTRIGGER);
			}
		}
	_catch_up:
		LZ4_pages_helper_seek (&in_helper_ip, -hash_byte_count - 1);
		LZ4_pages_helper_seek (&in_helper_match, -5);
		while (((in_helper_ip.offset + 1) > in_helper_anchor.offset) && ((in_helper_match.offset + 1) > 0)) {
			if (unlikely (LZ4_pages_read8_reverse (&in_helper_ip) != LZ4_pages_read8_reverse (&in_helper_match))) {
				LZ4_pages_helper_seek (&in_helper_ip, 1);
				LZ4_pages_helper_seek (&in_helper_match, 1);
				break;
			}
		}
		LZ4_pages_helper_seek (&in_helper_ip, 1 + MINMATCH);
		LZ4_pages_helper_seek (&in_helper_match, 1 + MINMATCH);
		literal_length		   = in_helper_ip.offset - MINMATCH - in_helper_anchor.offset;
		match_length		   = LZ4_pages_count_continue (&in_helper_ip, &in_helper_match, matchlimit - in_helper_ip.offset);
		literal_length_masked  = (literal_length >= RUN_MASK) ? RUN_MASK : literal_length;
		match_length_masked	= (match_length >= ML_MASK) ? ML_MASK : match_length;
		literal_length_tmp	 = literal_length - literal_length_masked;
		match_length_tmp	   = match_length - match_length_masked;
		literal_length_mod_255 = literal_length_tmp % 255;
		literal_length_div_255 = literal_length_tmp / 255;
		match_length_mod_255   = match_length_tmp % 255;
		match_length_div_255   = match_length_tmp / 255;
		if ((outputLimited) && (unlikely (out_helper.offset + TOKEN_BYTE_COUNT + (literal_length_div_255 + 1 + literal_length) + (OFFSET_BYTE_COUNT + match_length_div_255 + 1) > maxOutputSize)))
			goto _error;
		LZ4_pages_write8_continue (&out_helper, (literal_length_masked << ML_BITS) + match_length_masked);
		LZ4_pages_encode_number_continue (&out_helper, literal_length, RUN_MASK, literal_length_div_255, literal_length_mod_255);
		APPEND_LOG ("literal,%d\n", literal_length);
		LZ4_pages_memcpy_continue (&out_helper, &in_helper_anchor, literal_length);
		LZ4_pages_helper_clone (&in_helper_anchor, &in_helper_ip);
		LZ4_pages_write16_continue (&out_helper, in_helper_ip.offset - in_helper_match.offset);
		APPEND_LOG ("offset,%d\n", (in_helper_ip.offset - in_helper_match.offset));
		LZ4_pages_encode_number_continue (&out_helper, match_length, ML_MASK, match_length_div_255, match_length_mod_255);
		APPEND_LOG ("match,%d\n", match_length);
	_next_match:
		/* Test end of chunk */
		if (in_helper_ip.offset > mflimit)
			goto _last_literals;
		LZ4_pages_helper_seek (&in_helper_ip, -2);
#if LZ4_ARCH64
		if (tableType == LZ4_byU32) {
			/* Fill table */
			data64 = LZ4_pages_read64_continue (&in_helper_ip);
			h	  = LZ4_pages_hash5 (data64);
			LZ4_pages_putPosition (in_helper_ip.offset - 8, h, dictPtr->hashTable, tableType);
			/* Test next position */
			data64		= (data64 >> 16) | ((U64) LZ4_pages_read16_continue (&in_helper_ip) << 48);
			data32cache = data64;
			h			= LZ4_pages_hash5 (data64);
			data32		= LZ4_pages_updatePositionOnHash (in_helper_ip.offset - 8, h, dictPtr->hashTable, tableType);
			LZ4_pages_helper_set_offset (&in_helper_match, data32);
			/* prepare next loop */
			data64   = (data64 >> 8) | ((U64) LZ4_pages_read8_continue (&in_helper_ip) << 56);
			forwardH = LZ4_pages_hash5 (data64);
			LZ4_pages_helper_seek (&in_helper_ip, -5);
		} else
#endif
		{
			/* Fill table */
			data32 = LZ4_pages_read32_continue (&in_helper_ip);
			h	  = LZ4_pages_hash4 (data32, tableType);
			LZ4_pages_putPosition (in_helper_ip.offset - 4, h, dictPtr->hashTable, tableType);
			/* Test next position */
			data32		= (data32 >> 16) | ((U32) LZ4_pages_read16_continue (&in_helper_ip) << 16);
			data32cache = data32;
			h			= LZ4_pages_hash4 (data32, tableType);
			data32		= LZ4_pages_updatePositionOnHash (in_helper_ip.offset - 4, h, dictPtr->hashTable, tableType);
			LZ4_pages_helper_set_offset (&in_helper_match, data32);
			/* prepare next loop */
			data32   = (data32 >> 8) | ((U32) LZ4_pages_read8_continue (&in_helper_ip) << 8);
			forwardH = LZ4_pages_hash4 (data32, tableType);
			LZ4_pages_helper_seek (&in_helper_ip, -1);
		}
		if ((in_helper_match.offset + MAX_DISTANCE + MINMATCH >= in_helper_ip.offset) && (LZ4_pages_read32_continue (&in_helper_match) == data32cache)) {
			match_length		= LZ4_pages_count_continue (&in_helper_ip, &in_helper_match, matchlimit - in_helper_ip.offset);
			match_length_masked = (match_length >= ML_MASK) ? ML_MASK : match_length;
			LZ4_pages_helper_clone (&in_helper_anchor, &in_helper_ip);
			match_length_tmp	 = match_length - match_length_masked;
			match_length_mod_255 = match_length_tmp % 255;
			match_length_div_255 = match_length_tmp / 255;
			if (outputLimited && (unlikely (out_helper.offset + TOKEN_BYTE_COUNT + OFFSET_BYTE_COUNT + match_length_div_255 + 1 > maxOutputSize)))
				goto _error;
			LZ4_pages_write8_continue (&out_helper, match_length_masked);
			APPEND_LOG ("literal,%d\n", 0);
			LZ4_pages_write16_continue (&out_helper, in_helper_ip.offset - in_helper_match.offset);
			APPEND_LOG ("offset,%d\n", (in_helper_ip.offset - in_helper_match.offset));
			LZ4_pages_encode_number_continue (&out_helper, match_length, ML_MASK, match_length_div_255, match_length_mod_255);
			APPEND_LOG ("match,%d\n", match_length);
			goto _next_match;
		}
		LZ4_pages_helper_seek (&in_helper_ip, -3);
	}

_last_literals_seek:
	LZ4_pages_helper_seek (&in_helper_ip, -hash_byte_count);
_last_literals:
	literal_length		   = (inputSize - in_helper_anchor.offset);
	literal_length_masked  = (literal_length >= RUN_MASK) ? RUN_MASK : literal_length;
	literal_length_tmp	 = literal_length - literal_length_masked;
	literal_length_mod_255 = literal_length_tmp % 255;
	literal_length_div_255 = literal_length_tmp / 255;
	if ((outputLimited) && (out_helper.offset + 1 + literal_length_div_255 + (literal_length >= RUN_MASK) + literal_length > maxOutputSize))
		goto _error;
	LZ4_pages_write8_continue (&out_helper, literal_length_masked << ML_BITS);
	LZ4_pages_encode_number_continue (&out_helper, literal_length, RUN_MASK, literal_length_div_255, literal_length_mod_255);
	APPEND_LOG ("literal,%d\n", literal_length);
	LZ4_pages_memcpy_continue (&out_helper, &in_helper_anchor, literal_length);

	LZ4_pages_helper_free (&out_helper);
	LZ4_pages_helper_free (&in_helper_ip);
	LZ4_pages_helper_free (&in_helper_match);
	LZ4_pages_helper_free (&in_helper_anchor);
#ifdef LOG_STATISTICS
	close_log_file ();
#endif
	return out_helper.offset;
_error:
	LZ4_pages_helper_free (&out_helper);
	LZ4_pages_helper_free (&in_helper_ip);
	LZ4_pages_helper_free (&in_helper_match);
	LZ4_pages_helper_free (&in_helper_anchor);
	return 0;
}

static FORCE_INLINE int LZ4_pages_compress_fast_extState (void* state, BYTE** source, BYTE** dest, int inputSize, int maxOutputSize, int acceleration) {
	LZ4_stream_t_internal* ctx		 = &((LZ4_stream_t*) state)->internal_donotuse;
	const LZ4_tableType_t  tableType = LZ4_byU32;
	LZ4_resetStream ((LZ4_stream_t*) state);
	if (acceleration < 1)
		acceleration = LZ4_ACCELERATION_DEFAULT;
	if (maxOutputSize >= LZ4_COMPRESSBOUND (inputSize)) {
		if (inputSize < LZ4_64Klimit)
			return LZ4_pages_compress_generic (ctx, source, dest, inputSize, 0, LZ4_noLimit, LZ4_byU16, acceleration);
		else
			return LZ4_pages_compress_generic (ctx, source, dest, inputSize, 0, LZ4_noLimit, tableType, acceleration);
	} else {
		if (inputSize < LZ4_64Klimit)
			return LZ4_pages_compress_generic (ctx, source, dest, inputSize, maxOutputSize, LZ4_limitedOutput, LZ4_byU16, acceleration);
		else
			return LZ4_pages_compress_generic (ctx, source, dest, inputSize, maxOutputSize, LZ4_limitedOutput, tableType, acceleration);
	}
}

int LZ4_pages_compress_fast (struct page** source, struct page** dest, int source_length, int dest_length, int acceleration, void* wrkmem) {
	int		  res = -ENOMEM;
	BYTE**	source_mapped;
	BYTE**	dest_mapped;
	const int source_page_count = (source_length + PAGE_SIZE - 1) >> PAGE_SHIFT;
	const int dest_page_count   = (dest_length + PAGE_SIZE - 1) >> PAGE_SHIFT;
	const int gfp_flags			= (__GFP_IO | __GFP_NOWARN | __GFP_ZERO | __GFP_NORETRY | __GFP_NOMEMALLOC | __GFP_NOTRACK);
	int		  i;
	source_mapped = kmalloc (source_page_count * sizeof (*source_mapped), gfp_flags);
	if (source_mapped != NULL) {
		dest_mapped = kmalloc (dest_page_count * sizeof (*dest_mapped), gfp_flags);
		if (dest_mapped != NULL) {
			for (i				 = 0; i < source_page_count; i++)
				source_mapped[i] = kmap (source[i]);
			for (i			   = 0; i < dest_page_count; i++)
				dest_mapped[i] = kmap (dest[i]);
			res				   = LZ4_pages_compress_fast_extState (wrkmem, source_mapped, dest_mapped, source_length, dest_length, acceleration);
			for (i = 0; i < source_page_count; i++)
				kunmap (source[i]);
			for (i = 0; i < dest_page_count; i++)
				kunmap (dest[i]);
			kfree (dest_mapped);
		}
		kfree (source_mapped);
	}
	return res;
}
EXPORT_SYMBOL (LZ4_pages_compress_fast);

int LZ4_pages_compress_default (struct page** source, struct page** dest, int inputSize, int maxOutputSize, void* wrkmem) {
	return LZ4_pages_compress_fast (source, dest, inputSize, maxOutputSize, LZ4_ACCELERATION_DEFAULT, wrkmem);
}
EXPORT_SYMBOL (LZ4_pages_compress_default);
