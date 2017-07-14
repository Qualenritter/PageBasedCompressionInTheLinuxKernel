/*
 * bewalgo_linear_compress_U64.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#include "../bewalgo_parallel_compressor.h"

static generic_compressor* compressor_linear			= &generic_compressor_parallel_linear_memcpy;
static generic_compressor* compressor_page				= &generic_compressor_parallel_page_memcpy;
static int				   too_small_bounds_substractor = sizeof (U64);

#define BEWALGO_SIMPLE_TEST

#include "../interfaces/generic_compress_test.h"
