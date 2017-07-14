/*
 * page_compressor_memcpy.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */
#include "memcpy_compress.h"

__attribute__ ((no_instrument_function)) static int compress_bound (int isize) {
	return isize;
}
__attribute__ ((no_instrument_function)) static int compress_default (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length) {
	struct page** src = (struct page**) source;
	struct page** dst = (struct page**) dest;
	int			  i;
	U64*		  s;
	U64*		  d;
	U32			  length = source_length;
	if (dest_length < source_length)
		return -1;
	for (i = 0; i < (length >> PAGE_SHIFT); i++) {
		s = (U64*) kmap (src[i]);
		d = (U64*) kmap (dst[i]);
		copy_funtion_page (d, s, PAGE_SIZE);
		kunmap (src[i]);
		kunmap (dst[i]);
	}
	if (length & ~PAGE_MASK) {
		s = (U64*) kmap (src[i]);
		d = (U64*) kmap (dst[i]);
		copy_funtion_page (d, s, length & ~PAGE_MASK);
		kunmap (src[i]);
		kunmap (dst[i]);
	}
	return source_length;
}
__attribute__ ((no_instrument_function)) static int compress_fast (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration) {
	struct page** src = (struct page**) source;
	struct page** dst = (struct page**) dest;
	int			  i;
	U64*		  s;
	U64*		  d;
	U32			  length = source_length;
	if (dest_length < source_length)
		return -1;
	for (i = 0; i < (length >> PAGE_SHIFT); i++) {
		s = (U64*) kmap (src[i]);
		d = (U64*) kmap (dst[i]);
		copy_funtion_page (d, s, PAGE_SIZE);
		kunmap (src[i]);
		kunmap (dst[i]);
	}
	if (length & ~PAGE_MASK) {
		s = (U64*) kmap (src[i]);
		d = (U64*) kmap (dst[i]);
		copy_funtion_page (d, s, length & ~PAGE_MASK);
		kunmap (src[i]);
		kunmap (dst[i]);
	}
	return source_length;
}
__attribute__ ((no_instrument_function)) static int decompress_save (void* const source, void* const dest, const int source_length, const int dest_length) {
	struct page** src = (struct page**) source;
	struct page** dst = (struct page**) dest;
	int			  i;
	U64*		  s;
	U64*		  d;
	U32			  length = dest_length;
	if (source_length < dest_length)
		return -1;
	for (i = 0; i < (length >> PAGE_SHIFT); i++) {
		s = (U64*) kmap (src[i]);
		d = (U64*) kmap (dst[i]);
		copy_funtion_page (d, s, PAGE_SIZE);
		kunmap (src[i]);
		kunmap (dst[i]);
	}
	if (length & ~PAGE_MASK) {
		s = (U64*) kmap (src[i]);
		d = (U64*) kmap (dst[i]);
		copy_funtion_page (d, s, length & ~PAGE_MASK);
		kunmap (src[i]);
		kunmap (dst[i]);
	}
	return dest_length;
}
__attribute__ ((no_instrument_function)) static int decompress_fast (void* const source, void* const dest, const int source_length, const int dest_length) {
	struct page** src = (struct page**) source;
	struct page** dst = (struct page**) dest;
	int			  i;
	U64*		  s;
	U64*		  d;
	U32			  length = dest_length;
	if (source_length < dest_length)
		return -1;
	for (i = 0; i < (length >> PAGE_SHIFT); i++) {
		s = (U64*) kmap (src[i]);
		d = (U64*) kmap (dst[i]);
		copy_funtion_page (d, s, PAGE_SIZE);
		kunmap (src[i]);
		kunmap (dst[i]);
	}
	if (length & ~PAGE_MASK) {
		s = (U64*) kmap (src[i]);
		d = (U64*) kmap (dst[i]);
		copy_funtion_page (d, s, length & ~PAGE_MASK);
		kunmap (src[i]);
		kunmap (dst[i]);
	}
	return dest_length;
}

const generic_compressor generic_memcpy_page = {.compress_default				 = &compress_default,
												.compress_fast					 = &compress_fast,
												.decompress_save				 = &decompress_save,
												.decompress_fast				 = &decompress_fast,
												.generic_compressor_memory		 = GENERIC_COMPRESSOR_PAGE,
												.compress_bound					 = &compress_bound,
												.compressor_name				 = "memcpy_page",
												.compress_wrkmem				 = 1,
												.generic_compressor_concatenable = GENERIC_COMPRESSOR_CONCATABLE };
EXPORT_SYMBOL (generic_memcpy_page);
