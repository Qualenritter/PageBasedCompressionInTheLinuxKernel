/*
 * bewalgo_compressor.h
 *
 *  Created on: Mar 31, 2017
 *      Author: benjamin
 */

#ifndef BEWALGO_COMPRESS_H_
#define BEWALGO_COMPRESS_H_

#include "config.h"
#include "interfaces/generic_compressor.h"
#include <linux/mm.h>

typedef uint8_t  BYTE;
typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t  S32;
typedef uint64_t U64;

#define BEWALGO_MEMORY_USAGE 14

/*
 * generic interfaces to use the BeWalgo compressor and decompressor
 * BeWalgo can be either used wih 32 bit or 64 bit datasegments
 * BeWalgo can be either used wih linear or page based buffers
 */
const extern generic_compressor bewalgo_generic_compressor_linear_U32;
const extern generic_compressor bewalgo_generic_compressor_linear_U64;
const extern generic_compressor bewalgo_generic_compressor_page_U32;
const extern generic_compressor bewalgo_generic_compressor_page_U64;

#endif /* BEWALGO_COMPRESS_H_ */
