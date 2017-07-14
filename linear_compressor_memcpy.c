/*
 * linear_compressor_memcpy.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */
#include "memcpy_compress.h"

__attribute__ ((no_instrument_function)) static int compress_bound (int isize) {
	return isize;
}
__attribute__ ((no_instrument_function)) static int compress_default (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length) {
	U64* src = (U64*) source;
	U64* dst = (U64*) dest;
	if (dest_length < source_length)
		return -1;
	copy_funtion (dst, src, source_length);
	return source_length;
}
__attribute__ ((no_instrument_function)) static int compress_fast (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration) {
	U64* src = (U64*) source;
	U64* dst = (U64*) dest;
	if (dest_length < source_length)
		return -1;
	copy_funtion (dst, src, source_length);
	return source_length;
}
__attribute__ ((no_instrument_function)) static int decompress_save (void* const source, void* const dest, const int source_length, const int dest_length) {
	U64* src = (U64*) source;
	U64* dst = (U64*) dest;
	if (source_length < dest_length)
		return -1;
	copy_funtion (dst, src, source_length);
	return dest_length;
}
__attribute__ ((no_instrument_function)) static int decompress_fast (void* const source, void* const dest, const int source_length, const int dest_length) {
	U64* src = (U64*) source;
	U64* dst = (U64*) dest;
	if (source_length < dest_length)
		return -1;
	copy_funtion (dst, src, source_length);
	return dest_length;
}

const generic_compressor generic_memcpy_linear = {.compress_default				   = &compress_default,
												  .compress_fast				   = &compress_fast,
												  .decompress_save				   = &decompress_save,
												  .decompress_fast				   = &decompress_fast,
												  .generic_compressor_memory	   = GENERIC_COMPRESSOR_LINEAR,
												  .compress_bound				   = &compress_bound,
												  .compressor_name				   = "memcpy_linear",
												  .compress_wrkmem				   = 1,
												  .generic_compressor_concatenable = GENERIC_COMPRESSOR_CONCATABLE };
EXPORT_SYMBOL (generic_memcpy_linear);
