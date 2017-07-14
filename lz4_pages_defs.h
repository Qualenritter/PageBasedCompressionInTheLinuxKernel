#ifndef __LZ4_PAGES_DEFS_H__
#define __LZ4_PAGES_DEFS_H__

/*
 * lz4_pages_defs.h -- common and architecture specific defines for the kernel usage

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
 *	Changed for page based buffers usage by:
 *	Benjamin Warnke <4bwarnke@informatik.uni-hamburg.de>
 */

#include "lz4defs.h"
#include <linux/highmem.h>
#include <linux/mm.h>

/********************************************************************************/

#ifdef LOG_STATISTICS
/*
 * log compressor statistics to file:
 * - which code is executed how ofthen?
 * - are the unit tests covering the whole code?
 * - how long are literals/matches/offsets?
 */
#else /* LOG_STATISTICS */
#ifdef APPEND_LOG
#undef APPEND_LOG
#endif
#define APPEND_LOG(...) \
	do {                \
	} while (0)
#endif

/********************************************************************************/

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#define PAGE_OFFSET_MASK (PAGE_SIZE - 1)
#define TOKEN_BYTE_COUNT 1
#define OFFSET_BYTE_COUNT 2

/*
 * struct page_access_helper
 * interface for efficiently read and write data to and from page arrays
 */
typedef struct {
	BYTE** pages;
	BYTE*  page_pointer;
	BYTE*  page_pointer_end_byte_t;
	BYTE*  page_pointer_end_size_t;
	int	page_index;
	size_t offset;
} page_access_helper;

/**
 * LZ4_pages_helper_available() - how many BYTEs are available on this page?
 * @page_helper: the structure to modify
 */
static FORCE_INLINE int LZ4_pages_helper_available (page_access_helper* page_helper) {
	return page_helper->page_pointer_end_byte_t - page_helper->page_pointer;
}

/**
 * LZ4_pages_helper_init() - initialize the helper structure
 * @page_helper: the structure to initialize
 * @pages: the mapped pages which are going to be accesed by the helper
 */
static FORCE_INLINE void LZ4_pages_helper_init (page_access_helper* page_helper, BYTE** pages) {
	page_helper->pages					 = pages;
	page_helper->page_index				 = 0;
	page_helper->page_pointer			 = page_helper->pages[page_helper->page_index];
	page_helper->page_pointer_end_byte_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE;
	page_helper->page_pointer_end_size_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE - sizeof (size_t);
	page_helper->offset					 = 0;
}

/**
 * LZ4_pages_helper_free() - free the helper structure
 * @page_helper: the structure to free
 */
static FORCE_INLINE void LZ4_pages_helper_free (page_access_helper* page_helper) {
	page_helper->pages					 = NULL;
	page_helper->page_index				 = 0;
	page_helper->page_pointer_end_byte_t = NULL;
	page_helper->page_pointer_end_size_t = NULL;
	page_helper->page_pointer			 = NULL;
}

/**
 * LZ4_pages_helper_clone() - clones the mapped pointers
 * @page_helper_dest: the structure to initialize
 * @page_helper_source: the structure to replicate
 */
static FORCE_INLINE void LZ4_pages_helper_clone (page_access_helper* page_helper_dest, page_access_helper* page_helper_source) {
	page_helper_dest->pages					  = page_helper_source->pages;
	page_helper_dest->page_index			  = page_helper_source->page_index;
	page_helper_dest->page_pointer			  = page_helper_source->page_pointer;
	page_helper_dest->page_pointer_end_byte_t = page_helper_source->page_pointer_end_byte_t;
	page_helper_dest->page_pointer_end_size_t = page_helper_source->page_pointer_end_size_t;
	page_helper_dest->offset				  = page_helper_source->offset;
}

/**
 * LZ4_pages_helper_page_forward() - moves the pointer to the first byte of the next page
 * @page_helper: the structure to manipulate
 */
static FORCE_INLINE void LZ4_pages_helper_page_forward (page_access_helper* page_helper) {
	page_helper->page_index				 = page_helper->page_index + 1;
	page_helper->page_pointer			 = page_helper->pages[page_helper->page_index];
	page_helper->page_pointer_end_byte_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE;
	page_helper->page_pointer_end_size_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE - sizeof (size_t);
}

/**
 * LZ4_pages_helper_page_backward() - moves the pointer behind the last byte of the previous page
 * @page_helper: the structure to manipulate
 */
static FORCE_INLINE void LZ4_pages_helper_page_backward (page_access_helper* page_helper) {
	page_helper->page_index				 = page_helper->page_index - 1;
	page_helper->page_pointer			 = page_helper->pages[page_helper->page_index];
	page_helper->page_pointer_end_byte_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE;
	page_helper->page_pointer_end_size_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE - sizeof (size_t);
}

/**
 * LZ4_pages_helper_set_offset() - moves the pointer to an absolute position in the page data
 * @page_helper: the structure to manipulate
 * @offset: the offset to point to
 */
static FORCE_INLINE void LZ4_pages_helper_set_offset (page_access_helper* page_helper, int offset) {
	page_helper->page_index				 = offset >> PAGE_SHIFT;
	page_helper->page_pointer			 = page_helper->pages[page_helper->page_index] + (offset & PAGE_OFFSET_MASK);
	page_helper->page_pointer_end_byte_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE;
	page_helper->page_pointer_end_size_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE - sizeof (size_t);
	page_helper->offset					 = offset;
}

/**
 * LZ4_pages_helper_seek() - moves the pointer some bytes further
 * @page_helper: the structure to manipulate
 * @seek: number of BYTEs to skip
 */
static FORCE_INLINE void LZ4_pages_helper_seek (page_access_helper* page_helper, int seek) {
	page_helper->offset					 = page_helper->offset + seek;
	page_helper->page_index				 = page_helper->offset >> PAGE_SHIFT;
	page_helper->page_pointer			 = page_helper->pages[page_helper->page_index] + (page_helper->offset & PAGE_OFFSET_MASK);
	page_helper->page_pointer_end_byte_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE;
	page_helper->page_pointer_end_size_t = page_helper->pages[page_helper->page_index] + PAGE_SIZE - sizeof (size_t);
}

/**
 * LZ4_pages_write8_continue() - write a BYTE to the buffer
 * @page_helper: the structure to manipulate
 * @value: data to write
 */
static FORCE_INLINE void LZ4_pages_write8_continue (page_access_helper* page_helper, BYTE value) {
	if (LZ4_pages_helper_available (page_helper) <= 0) {
		LZ4_pages_helper_page_forward (page_helper);
	}
	*(page_helper->page_pointer) = value;
	page_helper->page_pointer += 1;
	page_helper->offset += 1;
}

/**
 * LZ4_pages_write16_continue() - write two BYTEs to the buffer
 * @page_helper: the structure to manipulate
 * @value: data to write
 */
static FORCE_INLINE void LZ4_pages_write16_continue (page_access_helper* page_helper, U16 value) {
	switch (LZ4_pages_helper_available (page_helper)) {
		case 0:
			LZ4_pages_helper_page_forward (page_helper);
			*((U16*) page_helper->page_pointer) = value;
			page_helper->page_pointer += 2;
			break;
		case 1:
			*(page_helper->page_pointer) = value;
			page_helper->page_pointer += 1;
			LZ4_pages_helper_page_forward (page_helper);
			*(page_helper->page_pointer) = value >> 8;
			page_helper->page_pointer += 1;
			break;
		default:
			put_unaligned_le16 (value, page_helper->page_pointer);
			page_helper->page_pointer += 2;
			break;
	}
	page_helper->offset += 2;
}

/**
 * LZ4_pages_bundle_read8_continue() - read a BYTE from the buffer - dont check page boundaries
 * @page_helper: the structure to manipulate
 */
static FORCE_INLINE BYTE LZ4_pages_bundle_read8_continue (page_access_helper* page_helper) {
	BYTE res;
	res = *(page_helper->page_pointer);
	page_helper->page_pointer += 1;
	page_helper->offset += 1;
	return res;
}

/**
 * LZ4_pages_read8_continue() - read a BYTE from the buffer
 * @page_helper: the structure to manipulate
 */
static FORCE_INLINE BYTE LZ4_pages_read8_continue (page_access_helper* page_helper) {
	BYTE res;
	if (LZ4_pages_helper_available (page_helper) == 0) {
		LZ4_pages_helper_page_forward (page_helper);
	}
	res = *(page_helper->page_pointer);
	page_helper->page_pointer += 1;
	page_helper->offset += 1;
	return res;
}

/**
 * LZ4_pages_read8_reverse() - read a BYTE from the buffer and seek backwards in the buffer
 * @page_helper: the structure to manipulate
 */
static FORCE_INLINE BYTE LZ4_pages_read8_reverse (page_access_helper* page_helper) {
	BYTE res;
	switch (LZ4_pages_helper_available (page_helper)) {
		case 0:
			LZ4_pages_helper_page_forward (page_helper);
			res = *(page_helper->page_pointer);
			LZ4_pages_helper_page_backward (page_helper);
			page_helper->page_pointer += PAGE_SIZE;
			break;
		case PAGE_SIZE:
			res = *(page_helper->page_pointer);
			LZ4_pages_helper_page_backward (page_helper);
			page_helper->page_pointer += PAGE_SIZE - 1;
			break;
		default:
			res = *(page_helper->page_pointer);
			page_helper->page_pointer -= 1;
	}
	page_helper->offset -= 1;
	return res;
}

/**
 * LZ4_pages_read16_continue() - read two BYTEs from the buffer
 * @page_helper: the structure to manipulate
 */
static FORCE_INLINE U16 LZ4_pages_read16_continue (page_access_helper* page_helper) {
	U16 res;
	switch (LZ4_pages_helper_available (page_helper)) {
		case 0:
			LZ4_pages_helper_page_forward (page_helper);
			res = *((U16*) page_helper->page_pointer);
			page_helper->page_pointer += 2;
			break;
		case 1:
			res = *page_helper->page_pointer;
			page_helper->page_pointer += 1;
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U16) (*page_helper->page_pointer) << 8);
			page_helper->page_pointer += 1;
			break;
		default:
			res = get_unaligned_le16 (page_helper->page_pointer);
			page_helper->page_pointer += 2;
			break;
	}
	page_helper->offset += 2;
	return res;
}

/**
 * LZ4_pages_read32_continue() - read four BYTEs from the buffer
 * @page_helper: the structure to manipulate
 */
static FORCE_INLINE U32 LZ4_pages_read32_continue (page_access_helper* page_helper) {
	U32 res;
	switch (LZ4_pages_helper_available (page_helper)) {
		case 0:
			LZ4_pages_helper_page_forward (page_helper);
			res = *((U32*) page_helper->page_pointer);
			page_helper->page_pointer += 4;
			break;
		case 1:
			res = (U32) (*(page_helper->page_pointer));
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U32) (*((U16*) page_helper->page_pointer)) << 8) | ((U32) (*(page_helper->page_pointer + 2)) << 24);
			page_helper->page_pointer += 3;
			break;
		case 2:
			res = (U32) (*((U16*) page_helper->page_pointer));
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U32) (*((U16*) page_helper->page_pointer)) << 16);
			page_helper->page_pointer += 2;
			break;
		case 3:
			res = (U32) (*(page_helper->page_pointer)) | ((U32) (*((U16*) (page_helper->page_pointer + 1))) << 8);
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U32) (*(page_helper->page_pointer)) << 24);
			page_helper->page_pointer += 1;
			break;
		default:
			res = get_unaligned_le32 (page_helper->page_pointer);
			page_helper->page_pointer += 4;
			break;
	}
	page_helper->offset += 4;
	return res;
}

/**
 * LZ4_pages_read64_continue() - read eight BYTEs from the buffer
 * @page_helper: the structure to manipulate
 */
static FORCE_INLINE U64 LZ4_pages_read64_continue (page_access_helper* page_helper) {
	U64 res;
	switch (LZ4_pages_helper_available (page_helper)) {
		case 0:
			LZ4_pages_helper_page_forward (page_helper);
			res = *((U64*) page_helper->page_pointer);
			page_helper->page_pointer += 8;
			break;
		case 1:
			res = (U64) (*(page_helper->page_pointer));
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U64) (*((U32*) page_helper->page_pointer)) << 8) | ((U64) (*((U16*) (page_helper->page_pointer + 4))) << 40) | ((U64) (*(page_helper->page_pointer + 6)) << 56);
			page_helper->page_pointer += 7;
			break;
		case 2:
			res = (U64) (*((U16*) page_helper->page_pointer));
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U64) (*((U32*) page_helper->page_pointer)) << 16) | ((U64) (*((U16*) (page_helper->page_pointer + 4))) << 48);
			page_helper->page_pointer += 6;
			break;
		case 3:
			res = (U64) (*(page_helper->page_pointer)) | ((U64) (*((U16*) (page_helper->page_pointer + 1))) << 8);
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U64) (*((U32*) page_helper->page_pointer)) << 24) | ((U64) (*(page_helper->page_pointer + 4)) << 56);
			page_helper->page_pointer += 5;
			break;
		case 4:
			res = (U64) (*((U32*) page_helper->page_pointer));
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U64) (*((U32*) page_helper->page_pointer)) << 32);
			page_helper->page_pointer += 4;
			break;
		case 5:
			res = (U64) (*page_helper->page_pointer) | ((U64) (*((U32*) (page_helper->page_pointer + 1))) << 8);
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U64) (*((U16*) page_helper->page_pointer)) << 40) | ((U64) (*(page_helper->page_pointer + 2)) << 56);
			page_helper->page_pointer += 3;
			break;
		case 6:
			res = (U64) (*((U16*) page_helper->page_pointer)) | ((U64) (*((U32*) (page_helper->page_pointer + 2))) << 16);
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U64) (*((U16*) page_helper->page_pointer)) << 48);
			page_helper->page_pointer += 2;
			break;
		case 7:
			res = (U64) (*(page_helper->page_pointer)) | ((U64) (*((U16*) (page_helper->page_pointer + 1))) << 8) | ((U64) (*((U32*) (page_helper->page_pointer + 3))) << 24);
			LZ4_pages_helper_page_forward (page_helper);
			res |= ((U64) (*(page_helper->page_pointer)) << 56);
			page_helper->page_pointer += 1;
			break;
		default:
			res = get_unaligned_le64 (page_helper->page_pointer);
			page_helper->page_pointer += 8;
			break;
	}
	page_helper->offset += 8;
	return res;
}

/**
 * LZ4_pages_encode_number_continue() - encodes an number to the compressed stream
 * @page_helper: the structure to manipulate
 * @length: the number to encode
 * @length_mask: the mask that is applied by the token
 * @length_div_255: number divided by 255 - this is a byproduct in the caller code
 * @length_mod_255: number modulo 255 - this is a byproduct in the caller code
 */
static FORCE_INLINE void LZ4_pages_encode_number_continue (page_access_helper* page_helper, int length, int length_mask, int length_div_255, int length_mod_255) {
	if (length >= length_mask) {
		BYTE* dest = page_helper->page_pointer + length_div_255;
		do {
			while ((page_helper->page_pointer <= page_helper->page_pointer_end_size_t) && (page_helper->page_pointer <= dest)) {
				put_unaligned_le64 (~((size_t) 0), (size_t*) page_helper->page_pointer);
				page_helper->page_pointer += sizeof (size_t);
				page_helper->offset += sizeof (size_t);
				if (page_helper->page_pointer >= dest) {
					goto _finish;
				}
			}
			while ((page_helper->page_pointer < page_helper->page_pointer_end_byte_t)) {
				*(page_helper->page_pointer) = ~((BYTE) 0);
				page_helper->page_pointer += 1;
				page_helper->offset += 1;
			}
			length_div_255 = dest - page_helper->page_pointer;
			if (length_div_255 <= 0) {
				goto _finish;
			}
			LZ4_pages_helper_page_forward (page_helper);
			dest = page_helper->page_pointer + length_div_255;
		} while (1);
	_finish:
		page_helper->offset += dest - page_helper->page_pointer;
		page_helper->page_pointer = dest;
		LZ4_pages_write8_continue (page_helper, length_mod_255);
	}
}

/**
 * LZ4_pages_memcpy_continue() - copy data from one page helper to another
 * @page_helper_dest: the desination for the data
 * @page_helper_source: the source of the data
 * @count: number of BYTEs to copy
 */
static FORCE_INLINE void LZ4_pages_memcpy_continue (page_access_helper* page_helper_dest, page_access_helper* page_helper_source, int count) {
	size_t value_64;
	BYTE   value_8;
	int	i;
	do {
		while ((page_helper_dest->page_pointer <= page_helper_dest->page_pointer_end_size_t) && (page_helper_source->page_pointer <= page_helper_source->page_pointer_end_size_t)) {
			value_64 = get_unaligned ((size_t*) page_helper_source->page_pointer);
			put_unaligned (value_64, (size_t*) page_helper_dest->page_pointer);
			page_helper_source->page_pointer += sizeof (size_t);
			page_helper_dest->page_pointer += sizeof (size_t);
			page_helper_source->offset += sizeof (size_t);
			page_helper_dest->offset += sizeof (size_t);
			count -= sizeof (size_t);
			if (count <= 0)
				goto _finish;
		}
		for (i = 0; i < 8; i++) {
			value_8 = LZ4_pages_read8_continue (page_helper_source);
			LZ4_pages_write8_continue (page_helper_dest, value_8);
			count--;
			if (count <= 0)
				goto _finish;
		}
	} while (1);
_finish:
	page_helper_source->page_pointer += count;
	page_helper_dest->page_pointer += count;
	page_helper_source->offset += count;
	page_helper_dest->offset += count;
}

/**
 * LZ4_pages_memcpy_continue() - copy data from one page helper to another the two buffers may overlap
 * @page_helper_dest: the desination for the data
 * @page_helper_source: the source of the data
 * @count: number of BYTEs to copy
 * @max_block: initial distance between the two pointers
 */
static FORCE_INLINE void LZ4_pages_memcpy_continue_overlap (page_access_helper* page_helper_dest, page_access_helper* page_helper_source, size_t count, size_t max_block) {
	int	bytesToCopy = count;
	int	currentSize;
	size_t source_position;
	int	current_bytesToCopy;
	if (likely (count <= max_block)) {
		LZ4_pages_memcpy_continue (page_helper_dest, page_helper_source, bytesToCopy);
		return;
	}

	source_position = (page_helper_source->page_index << PAGE_SHIFT) | (PAGE_SIZE - LZ4_pages_helper_available (page_helper_source));
	do {
		current_bytesToCopy = MIN (bytesToCopy, max_block);
		bytesToCopy -= current_bytesToCopy;
		do {
			if (LZ4_pages_helper_available (page_helper_dest) < LZ4_pages_helper_available (page_helper_source)) {
				currentSize = MIN (LZ4_pages_helper_available (page_helper_dest), current_bytesToCopy);
				memcpy (page_helper_dest->page_pointer, page_helper_source->page_pointer, currentSize);
				current_bytesToCopy -= currentSize;
				if (LZ4_pages_helper_available (page_helper_dest) == currentSize) {
					LZ4_pages_helper_page_forward (page_helper_dest);
				} else {
					page_helper_dest->page_pointer += currentSize;
					page_helper_dest->offset += currentSize;
				}
				page_helper_source->page_pointer += currentSize;
				page_helper_source->offset += currentSize;
			} else {
				currentSize = MIN (LZ4_pages_helper_available (page_helper_source), current_bytesToCopy);
				memcpy (page_helper_dest->page_pointer, page_helper_source->page_pointer, currentSize);
				current_bytesToCopy -= currentSize;
				if (LZ4_pages_helper_available (page_helper_source) == currentSize) {
					LZ4_pages_helper_page_forward (page_helper_source);
				} else {
					page_helper_source->page_pointer += currentSize;
					page_helper_source->offset += currentSize;
				}
				page_helper_dest->page_pointer += currentSize;
				page_helper_dest->offset += currentSize;
			}
		} while (current_bytesToCopy > 0);
		max_block = max_block << 1;
		LZ4_pages_helper_set_offset (page_helper_source, source_position);
		if (unlikely (max_block >= PAGE_SIZE)) {
			if (bytesToCopy > 0)
				LZ4_pages_memcpy_continue (page_helper_dest, page_helper_source, bytesToCopy);
			return;
		}
	} while (bytesToCopy > 0);
}

/**
 * LZ4_pages_count_continue() - find the maximum match length - precondition is that there is a match at the current position between both buffers
 * @page_helper_in: input data buffer position
 * @page_helper_match: the position of the match in the input buffer
 * @maxCount: maximum match length before buffer overrun
 */
static FORCE_INLINE unsigned int LZ4_pages_count_continue (page_access_helper* page_helper_in, page_access_helper* page_helper_match, int maxCount) {
	int	count		= 0;
	int	myupperlimit = maxCount - (sizeof (size_t));
	size_t diff;
	size_t in_value;
	size_t match_value;
	int	i;
	do {
		while ((page_helper_in->page_pointer <= page_helper_in->page_pointer_end_size_t) && (page_helper_match->page_pointer <= page_helper_match->page_pointer_end_size_t)) {
			if (unlikely (count >= myupperlimit))
				goto _last_bytes;
			in_value	= get_unaligned ((size_t*) page_helper_in->page_pointer);
			match_value = get_unaligned ((size_t*) page_helper_match->page_pointer);
			diff		= in_value ^ match_value;
			if (diff) {
				i = LZ4_NbCommonBytes (diff);
				page_helper_in->page_pointer += i;
				page_helper_match->page_pointer += i;
				page_helper_in->offset += i;
				page_helper_match->offset += i;
				count += i;
				goto _finish;
			}
			page_helper_in->page_pointer += sizeof (size_t);
			page_helper_match->page_pointer += sizeof (size_t);
			page_helper_in->offset += sizeof (size_t);
			page_helper_match->offset += sizeof (size_t);
			count += sizeof (size_t);
		}
		i = 0;
		while ((page_helper_match->page_pointer < page_helper_match->page_pointer_end_byte_t) && (page_helper_in->page_pointer < page_helper_in->page_pointer_end_byte_t) && (count < maxCount)) {
			if (*(page_helper_match->page_pointer) != *(page_helper_in->page_pointer)) {
				goto _finish;
			}
			page_helper_match->page_pointer += 1;
			page_helper_match->offset += 1;
			page_helper_in->page_pointer += 1;
			page_helper_in->offset += 1;
			count++;
			if (unlikely (count >= myupperlimit))
				goto _last_bytes;
		}
		if (page_helper_match->page_pointer == page_helper_match->page_pointer_end_byte_t)
			LZ4_pages_helper_page_forward (page_helper_match);
		if (page_helper_in->page_pointer == page_helper_in->page_pointer_end_byte_t)
			LZ4_pages_helper_page_forward (page_helper_in);
	} while (1);
_last_bytes:
	while ((count < maxCount) && (LZ4_pages_read8_continue (page_helper_match) == LZ4_pages_read8_continue (page_helper_in)))
		count++;
	if (count < maxCount) {
		page_helper_in->page_pointer -= 1;
		page_helper_match->page_pointer -= 1;
		page_helper_in->offset -= 1;
		page_helper_match->offset -= 1;
		return count;
	}
_finish:
	i = maxCount - count;
	if (unlikely (i < 0)) {
		page_helper_in->page_pointer += i;
		page_helper_match->page_pointer += i;
		page_helper_in->offset += i;
		page_helper_match->offset += i;
		return maxCount;
	} else {
		return count;
	}
}
#endif /* __LZ4_PAGES_DEFS_H__ */
