/*
 * memcpy_compress.h
 *
 *  Created on: Mar 31, 2017
 *      Author: benjamin
 */

#ifndef MEMCPY_COMPRESS_H_
#define MEMCPY_COMPRESS_H_

#include "interfaces/generic_compressor.h"
#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/mm.h>

typedef uint8_t  BYTE;
typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t  S32;
typedef uint64_t U64;

const extern generic_compressor generic_memcpy_linear;
const extern generic_compressor generic_memcpy_page;

/* implementation as it is used by the (de)-compressors -->> */

#define b_copy_helper_dest(dest, src, target) \
	do {                                      \
		while ((dest) < (target) -3) {        \
			(dest)[0] = (src)[0];             \
			(dest)[1] = (src)[1];             \
			(dest)[2] = (src)[2];             \
			(dest)[3] = (src)[3];             \
			(dest) += 4;                      \
			(src) += 4;                       \
		}                                     \
		if ((dest) < (target) -1) {           \
			(dest)[0] = (src)[0];             \
			(dest)[1] = (src)[1];             \
			(dest) += 2;                      \
			(src) += 2;                       \
		}                                     \
		if ((dest) < (target)) {              \
			*(dest)++ = *(src)++;             \
		}                                     \
	} while (0)

#define copy_funtion(dest, source, length)         \
	do {                                           \
		U64* target = (dest) + (length >> 3);      \
		b_copy_helper_dest (dest, source, target); \
	} while (0)

#define b_copy_helper_dest_page(dest, src, target) \
	do {                                           \
		while ((dest) < (target) -1) {             \
			(dest)[0] = (src)[0];                  \
			(dest)[1] = (src)[1];                  \
			(dest) += 2;                           \
			(src) += 2;                            \
		}                                          \
		if ((dest) < (target)) {                   \
			*(dest)++ = *(src)++;                  \
		}                                          \
	} while (0)

#define copy_funtion_page(dest, source, length)         \
	do {                                                \
		U64* target = (dest) + (length >> 3);           \
		b_copy_helper_dest_page (dest, source, target); \
	} while (0)

/* theoretical maximum speed -->> */

/*#define copy_funtion(dest,source,length) memcpy(dest,source,length)*/
/*#define copy_funtion_page(dest,source,length) memcpy(dest,source,length)*/

#endif /* MEMCPY_COMPRESS_H_ */
