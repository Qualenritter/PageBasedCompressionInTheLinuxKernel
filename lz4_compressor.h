/*
 * lz4_compressor.h
 *
 *  Created on: Mar 31, 2017
 *      Author: benjamin
 */

#ifndef LZ4_COMPRESSOR_H_
#define LZ4_COMPRESSOR_H_

#include "interfaces/generic_compressor.h"

/* generic interface to use the LZ4 compressor and decompressor with linear buffers*/
const extern generic_compressor lz4_generic_compressor_linear;
/* generic interface to use the LZ4 compressor and decompressor with page buffers*/
const extern generic_compressor lz4_generic_compressor_page;

#endif /* LZ4_COMPRESSOR_H_ */
