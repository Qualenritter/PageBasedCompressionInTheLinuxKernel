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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* LZ4_pages_decompress_generic() :
 * This generic decompression function cover all use cases.
 * It shall be instantiated several times, using different sets of directives
 * Note that it is important this generic function is really inlined,
 * in order to remove useless branches during compilation optimization.
 */
static FORCE_INLINE int LZ4_pages_decompress_generic (BYTE** source,
													  BYTE** dest,
													  int	inputSize,
													  /*
													   * If endOnInput == endOnInputSize,
													   * this value is the max size of Output Buffer.
													   */
													  int				 outputSize,
													  LZ4_safe_t		 safeDecode,
													  LZ4_endCondition_t endCondition,
													  LZ4_earlyEnd_t	 earlyEnd,
													  /* only used if partialDecoding == partial */
													  int targetOutputSize) {
	const int inputSize_2 = inputSize - 2;
	const int inputSize_3 = inputSize - 3;
	/* Local Variables */
	size_t			   ip = 0;
	size_t			   op = 0;
	size_t			   length;
	U16				   offset;
	BYTE			   token;
	BYTE			   s;
	int				   rc;
	page_access_helper in_helper;
	page_access_helper out_helper;
	page_access_helper out_helper_match;
	LZ4_pages_helper_init (&in_helper, source);
	LZ4_pages_helper_init (&out_helper, dest);
	LZ4_pages_helper_init (&out_helper_match, dest);
	if ((earlyEnd == LZ4_full) ? 1 : (targetOutputSize > outputSize))
		targetOutputSize = outputSize;
	/* Special cases */
	if (inputSize == 0) {
		rc = 0;
		goto _cleanup;
	}
	if (inputSize == 1) {
		if (LZ4_pages_read8_continue (&in_helper) == 0x00) {
			rc = 0;
			goto _cleanup;
		} else {
			goto _input_overflow;
		}
	}
	if (targetOutputSize == 0) {
		if (LZ4_pages_read8_continue (&in_helper) == 0x00) {
			rc = 0;
			goto _cleanup;
		} else {
			goto _output_overflow;
		}
	}
	/* Main Loop : decode sequences */
	while (1) {
		token = LZ4_pages_read8_continue (&in_helper);
		ip++;
		/* get literal length */
		length = token >> ML_BITS;
		if (likely (length > 0)) {
			if (length == RUN_MASK) {
				do {
					s = LZ4_pages_read8_continue (&in_helper);
					ip++;
					length += s;
					if (((safeDecode == LZ4_safe)) && unlikely (ip >= inputSize))
						goto _input_overflow;
				} while (s == 255);
			}
			if ((safeDecode == LZ4_safe) && unlikely (ip + length < ip))
				goto _offset_overflow;
			if ((safeDecode == LZ4_safe) && unlikely (op + length < op))
				goto _offset_overflow;
			ip += length;
			op += length;
			if ((safeDecode == LZ4_safe) && unlikely (ip > inputSize))
				goto _input_overflow;
			if ((safeDecode == LZ4_safe) && unlikely (op > targetOutputSize)) {
				if (earlyEnd == LZ4_partial) {
					ip -= length;
					op -= length;
					length = targetOutputSize - op;
					ip += length;
					op = targetOutputSize;
				} else {
					goto _output_overflow;
				}
			}
			LZ4_pages_memcpy_continue (&out_helper, &in_helper, length);
		}
		if ((endCondition == LZ4_endOnInputSize) && (ip >= inputSize)) {
			rc = op;
			goto _cleanup;
		}
		if ((endCondition == LZ4_endOnOutputSize) && (op >= targetOutputSize)) {
			rc = ip;
			goto _cleanup;
		}
		if ((safeDecode == LZ4_safe) && (ip >= inputSize_3))
			goto _input_overflow;
		offset = LZ4_pages_read16_continue (&in_helper);
		if ((safeDecode == LZ4_safe) && (unlikely (op < offset))) {
			goto _output_underflow;
		}
		LZ4_pages_helper_set_offset (&out_helper_match, op - offset);
		ip += 2;
		/* get matchlength */
		length = token & ML_MASK;
		if (length == ML_MASK) {
			do {
				s = LZ4_pages_read8_continue (&in_helper);
				ip++;
				if ((safeDecode == LZ4_safe) && (ip >= inputSize_2))
					goto _input_overflow;
				length += s;
			} while (s == 255);
		}
		length += MINMATCH;
		if ((safeDecode == LZ4_safe) && unlikely (op + length < op))
			goto _offset_overflow;
		op += length;
		if ((safeDecode == LZ4_safe) && unlikely (op > targetOutputSize))
			goto _output_overflow;
		LZ4_pages_memcpy_continue_overlap (&out_helper, &out_helper_match, length, offset);
	}
_output_underflow:
_output_overflow:
_input_overflow:
_offset_overflow:
	rc = -1;
_cleanup:
	LZ4_pages_helper_free (&in_helper);
	LZ4_pages_helper_free (&out_helper);
	LZ4_pages_helper_free (&out_helper_match);
	return rc;
}

int LZ4_pages_decompress_safe (struct page** source, struct page** dest, int source_length, int dest_length) {
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
			res				   = LZ4_pages_decompress_generic (source_mapped, dest_mapped, source_length, dest_length, LZ4_safe, LZ4_endOnInputSize, LZ4_full, 0);
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

int LZ4_pages_decompress_fast (struct page** source, struct page** dest, int source_length, int dest_length) {
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
			res				   = LZ4_pages_decompress_generic (source_mapped, dest_mapped, source_length, dest_length, LZ4_unsafe, LZ4_endOnOutputSize, LZ4_full, 0);
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

#ifndef STATIC
EXPORT_SYMBOL (LZ4_pages_decompress_fast);
EXPORT_SYMBOL (LZ4_pages_decompress_safe);

MODULE_LICENSE ("Dual BSD/GPL");
MODULE_DESCRIPTION ("LZ4 decompressor");
#endif
