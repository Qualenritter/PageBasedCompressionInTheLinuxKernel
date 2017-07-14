/*
 * bewalgo_linear_compress_U32.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#include "../bewalgo_compressor.h"

static const generic_compressor* const compressor_linear = &bewalgo_generic_compressor_linear_U64;
static const generic_compressor* const compressor_page   = &bewalgo_generic_compressor_page_U64;

#include "../interfaces/generic_compress_test_time.h"
