/*
 * bewalgo_page_compressor_U32.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#define BEWALGO_COMPRESS_DATA_TYPE U32
#define BEWALGO_COMPRESS_DATA_TYPE_SHIFT 2
#include "bewalgo_page_compress.h"
#include "bewalgo_page_decompress.h"

__attribute__ ((no_instrument_function)) static int bewalgo_compress_bound (int isize) {
	return BEWALGO_COMPRESS_BOUND (isize);
}

const generic_compressor bewalgo_generic_compressor_page_U32 = {.compress_default				 = &bewalgo_page_compress_default_U32,
																.compress_fast					 = &bewalgo_page_compress_fast_U32,
																.decompress_save				 = &bewalgo_page_decompress_save_U32,
																.decompress_fast				 = &bewalgo_page_decompress_fast_U32,
																.generic_compressor_memory		 = GENERIC_COMPRESSOR_PAGE,
																.compress_bound					 = &bewalgo_compress_bound,
																.compressor_name				 = "BeWalgo32-page-search-match-only",
																.compress_wrkmem				 = sizeof (bewalgo_compress_internal),
																.generic_compressor_concatenable = GENERIC_COMPRESSOR_CONCATABLE };
EXPORT_SYMBOL (bewalgo_generic_compressor_page_U32);
