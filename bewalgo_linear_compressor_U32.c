/*
 * bewalgo_linear_compressor_U32.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#define BEWALGO_COMPRESS_DATA_TYPE U32
#define BEWALGO_COMPRESS_DATA_TYPE_SHIFT 2
#include "bewalgo_linear_compress.h"
#include "bewalgo_linear_decompress.h"

__attribute__ ((no_instrument_function)) static int bewalgo_compress_bound (int isize) {
	return BEWALGO_COMPRESS_BOUND (isize);
}

__attribute__ ((no_instrument_function)) int
	benjamin_debug_asm_compress_fast_linear_U32 (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration) {
	return bewalgo_linear_compress_generic (wrkmem, source, dest, source_length, dest_length, acceleration, BEWALGO_UNSAFE, BEWALGO_NO_OFFSET_CHECK);
}
EXPORT_SYMBOL (benjamin_debug_asm_compress_fast_linear_U32);
__attribute__ ((no_instrument_function)) int benjamin_debug_asm_decompress_fast_linear_U32 (void* const source, void* const dest, const int source_length, const int dest_length) {
	return bewalgo_linear_decompress_generic (source, dest, source_length, dest_length, BEWALGO_UNSAFE);
}
EXPORT_SYMBOL (benjamin_debug_asm_decompress_fast_linear_U32);

const generic_compressor bewalgo_generic_compressor_linear_U32 = {.compress_default				   = &bewalgo_compress_default_U32,
																  .compress_fast				   = &bewalgo_compress_fast_U32,
																  .decompress_save				   = &bewalgo_decompress_save_U32,
																  .decompress_fast				   = &bewalgo_decompress_fast_U32,
																  .generic_compressor_memory	   = GENERIC_COMPRESSOR_LINEAR,
																  .compress_bound				   = &bewalgo_compress_bound,
																  .compressor_name				   = "BeWalgo32-linear-&&",
																  .compress_wrkmem				   = sizeof (bewalgo_compress_internal),
																  .generic_compressor_concatenable = GENERIC_COMPRESSOR_CONCATABLE };
EXPORT_SYMBOL (bewalgo_generic_compressor_linear_U32);
