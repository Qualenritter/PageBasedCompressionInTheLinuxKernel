/*
 * compress_test_memcpy.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#include "../memcpy_compress.h"

static const generic_compressor* const compressor_linear			= &generic_memcpy_linear;
static const generic_compressor* const compressor_page				= &generic_memcpy_page;
static const int					   too_small_bounds_substractor = 1;

#include "../interfaces/generic_compress_test.h"
