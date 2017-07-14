/*
 * bewalgo_linear_compress_U32.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#include "../bewalgo_parallel_compressor.h"

static generic_compressor* compressor_linear = &generic_compressor_parallel_bewalgo_linear_U64;
static generic_compressor* compressor_page   = &generic_compressor_parallel_bewalgo_page_U64;

#include "../interfaces/generic_compress_test_time.h"
