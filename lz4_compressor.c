/*
 * b_linear_compressor.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#include "lz4_compressor.h"
#include "lz4.h"

static int _lz4_compress_bound (int isize) {
	return LZ4_COMPRESSBOUND (isize);
}
static int _lz4_compress_linear_default (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length) {
	return LZ4_compress_default (source, dest, source_length, dest_length, wrkmem);
}
static int _lz4_compress_linear_fast (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration) {
	return LZ4_compress_fast (source, dest, source_length, dest_length, acceleration, wrkmem);
}
static int _lz4_decompress_linear_save (void* const source, void* const dest, const int source_length, const int dest_length) {
	return LZ4_decompress_safe (source, dest, source_length, dest_length);
}
static int _lz4_decompress_linear_fast (void* const source, void* const dest, const int source_length, const int dest_length) {
	int tmp = LZ4_decompress_fast (source, dest, dest_length);
	return tmp == source_length ? dest_length : -1;
}
static int _lz4_compress_page_default (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length) {
	return LZ4_pages_compress_default (source, dest, source_length, dest_length, wrkmem);
}
static int _lz4_compress_page_fast (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration) {
	return LZ4_pages_compress_fast (source, dest, source_length, dest_length, acceleration, wrkmem);
}
static int _lz4_decompress_page_save (void* const source, void* const dest, const int source_length, const int dest_length) {
	return LZ4_pages_decompress_safe (source, dest, source_length, dest_length);
}
static int _lz4_decompress_page_fast (void* const source, void* const dest, const int source_length, const int dest_length) {
	int tmp = LZ4_pages_decompress_fast (source, dest, source_length, dest_length);
	return tmp == source_length ? dest_length : -1;
}

const generic_compressor lz4_generic_compressor_linear = {.compress_default				   = &_lz4_compress_linear_default,
														  .compress_fast				   = &_lz4_compress_linear_fast,
														  .decompress_save				   = &_lz4_decompress_linear_save,
														  .decompress_fast				   = &_lz4_decompress_linear_fast,
														  .generic_compressor_memory	   = GENERIC_COMPRESSOR_LINEAR,
														  .compress_bound				   = &_lz4_compress_bound,
														  .compressor_name				   = "LZ4-linear-search-match-only",
														  .compress_wrkmem				   = LZ4_MEM_COMPRESS,
														  .generic_compressor_concatenable = GENERIC_COMPRESSOR_NOT_CONCATABLE };
EXPORT_SYMBOL (lz4_generic_compressor_linear);
const generic_compressor lz4_generic_compressor_page = {.compress_default				 = &_lz4_compress_page_default,
														.compress_fast					 = &_lz4_compress_page_fast,
														.decompress_save				 = &_lz4_decompress_page_save,
														.decompress_fast				 = &_lz4_decompress_page_fast,
														.generic_compressor_memory		 = GENERIC_COMPRESSOR_PAGE,
														.compress_bound					 = &_lz4_compress_bound,
														.compressor_name				 = "LZ4-page-search-match-only",
														.compress_wrkmem				 = LZ4_MEM_COMPRESS,
														.generic_compressor_concatenable = GENERIC_COMPRESSOR_NOT_CONCATABLE };
EXPORT_SYMBOL (lz4_generic_compressor_page);
