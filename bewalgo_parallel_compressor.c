/*
 * bewalgo_pages_compressor_U32.c
 *
 *  Created on: May 21, 2017
 *      Author: benjamin
 */

#include "bewalgo_parallel_compress_worker.h"
#include "compression-bewalgo/bewalgo_compressor.h"
#include "compression-lz4/lz4_compressor.h"
#include "compression-memcpy/memcpy_compress.h"

BEWALGO_DEFINE_PARALLEL_COMPRESSOR (generic_compressor_parallel_bewalgo_page_U32, bewalgo_generic_compressor_page_U32, "BeWalgo32-page-parallel", GENERIC_COMPRESSOR_PAGE)
BEWALGO_DEFINE_PARALLEL_COMPRESSOR (generic_compressor_parallel_bewalgo_linear_U32, bewalgo_generic_compressor_linear_U32, "BeWalgo32-linear-parallel", GENERIC_COMPRESSOR_LINEAR)
BEWALGO_DEFINE_PARALLEL_COMPRESSOR (generic_compressor_parallel_bewalgo_page_U64, bewalgo_generic_compressor_page_U64, "BeWalgo64-page-parallel", GENERIC_COMPRESSOR_PAGE)
BEWALGO_DEFINE_PARALLEL_COMPRESSOR (generic_compressor_parallel_bewalgo_linear_U64, bewalgo_generic_compressor_linear_U64, "BeWalgo64-linear-parallel", GENERIC_COMPRESSOR_LINEAR)
BEWALGO_DEFINE_PARALLEL_COMPRESSOR (generic_compressor_parallel_page_lz4, lz4_generic_compressor_page, "lz4-page-parallel", GENERIC_COMPRESSOR_PAGE)
BEWALGO_DEFINE_PARALLEL_COMPRESSOR (generic_compressor_parallel_linear_lz4, lz4_generic_compressor_linear, "lz4-linear-parallel", GENERIC_COMPRESSOR_LINEAR)
BEWALGO_DEFINE_PARALLEL_COMPRESSOR (generic_compressor_parallel_page_memcpy, generic_memcpy_page, "memcpy-page-parallel", GENERIC_COMPRESSOR_PAGE)
BEWALGO_DEFINE_PARALLEL_COMPRESSOR (generic_compressor_parallel_linear_memcpy, generic_memcpy_linear, "memcpy-linear-parallel", GENERIC_COMPRESSOR_LINEAR)
