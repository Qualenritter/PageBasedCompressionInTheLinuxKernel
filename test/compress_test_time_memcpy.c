/*
 * compress_test_time_memcpy.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#include "../memcpy_compress.h"

static const generic_compressor* const compressor_linear = &generic_memcpy_linear;
static const generic_compressor* const compressor_page   = &generic_memcpy_page;

#include "../interfaces/generic_compress_test_time.h"
