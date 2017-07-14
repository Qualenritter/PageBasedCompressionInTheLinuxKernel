/*
 * bewalgo_compress.h
 *
 *  Created on: Mar 31, 2017
 *      Author: benjamin
 */

#ifndef BEWALGO_PARALLEL_COMPRESSOR_H_
#define BEWALGO_PARALLEL_COMPRESSOR_H_

#include "interfaces/generic_compressor.h"
#include <linux/mm.h>

typedef uint8_t  BYTE;
typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t  S32;
typedef uint64_t U64;

extern generic_compressor generic_compressor_parallel_bewalgo_linear_U32;
extern generic_compressor generic_compressor_parallel_bewalgo_page_U32;
extern generic_compressor generic_compressor_parallel_bewalgo_linear_U64;
extern generic_compressor generic_compressor_parallel_bewalgo_page_U64;
extern generic_compressor generic_compressor_parallel_linear_lz4;
extern generic_compressor generic_compressor_parallel_page_lz4;
extern generic_compressor generic_compressor_parallel_linear_memcpy;
extern generic_compressor generic_compressor_parallel_page_memcpy;

#endif /* BEWALGO_PARALLEL_COMPRESSOR_H_ */
