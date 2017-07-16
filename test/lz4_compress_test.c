/*
 * b_linear_compress_U32.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#include "../config.h"
#include "../lz4_compressor.h"

static const generic_compressor* const compressor_linear			= &lz4_generic_compressor_linear;
static const generic_compressor* const compressor_page				= &lz4_generic_compressor_page;
static int							   too_small_bounds_substractor = 1;

#define SIMPLE_TEST

#include "../interfaces/generic_compress_test.h"
