/*
 * bewalgo_page_compressor_U64.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#define BEWALGO_COMPRESS_DATA_TYPE U64
#define BEWALGO_COMPRESS_DATA_TYPE_SHIFT 3
#include "bewalgo_page_compress.h"
#include "bewalgo_page_decompress.h"

__attribute__ ((no_instrument_function)) static int bewalgo_compress_bound (int isize) {
	return BEWALGO_COMPRESS_BOUND (isize);
}

const generic_compressor bewalgo_generic_compressor_page_U64 = {.compress_default				 = &bewalgo_page_compress_default_U64,
																.compress_fast					 = &bewalgo_page_compress_fast_U64,
																.decompress_save				 = &bewalgo_page_decompress_save_U64,
																.decompress_fast				 = &bewalgo_page_decompress_fast_U64,
																.generic_compressor_memory		 = GENERIC_COMPRESSOR_PAGE,
																.compress_bound					 = &bewalgo_compress_bound,
																.compressor_name				 = "BeWalgo64-page-if",
																.compress_wrkmem				 = sizeof (bewalgo_compress_internal),
																.generic_compressor_concatenable = GENERIC_COMPRESSOR_CONCATABLE };
EXPORT_SYMBOL (bewalgo_generic_compressor_page_U64);
