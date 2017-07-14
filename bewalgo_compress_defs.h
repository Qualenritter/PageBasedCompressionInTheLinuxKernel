/*
 * bewalgo_compress_defs.h
 *
 *  Created on: Mar 31, 2017
 *      Author: benjamin
 */

#ifndef BEWALGO_COMPRESS_DEFS_H_
#define BEWALGO_COMPRESS_DEFS_H_
#define bewalgo_compress_always_inline __always_inline
#include "bewalgo_compressor.h"
#include "config.h"
#include "interfaces/file_helper.h"
#include <asm/unaligned.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h> /* memset, memcpy */
#include <linux/types.h>

#define BEWALGO_SKIPTRIGGER 6

#define BEWALGO_LENGTH_BITS 8
#define BEWALGO_LENGTH_MAX ((1 << BEWALGO_LENGTH_BITS) - 1)
#define BEWALGO_OFFSET_BITS 16
#define BEWALGO_OFFSET_MAX ((1 << BEWALGO_OFFSET_BITS) - 1)

#define BEWALGO_COMPRESS_BOUND(isize) (isize + (isize / (((1 << BEWALGO_LENGTH_BITS) - 1) * sizeof (BEWALGO_COMPRESS_DATA_TYPE))) * 4 + 8)

#define ROUND_UP_TO_NEXT_DIVIDEABLE(value, divisor_shift) ((value & (~((1 << divisor_shift) - 1))) + (((value & ((1 << divisor_shift) - 1)) > 0) << divisor_shift))

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define BEWALGO_HASHLOG (BEWALGO_MEMORY_USAGE - 2)
#define BEWALGO_HASH_SIZE_U32 (1 << BEWALGO_HASHLOG)
typedef struct { U32 table[BEWALGO_HASH_SIZE_U32]; } bewalgo_compress_internal;

#define BEWALGO_MEM_COMPRESS sizeof (bewalgo_compress_internal)

typedef enum { BEWALGO_UNSAFE = 0, BEWALGO_SAFE = 1 } bewalgo_safety_mode;
typedef enum { BEWALGO_NO_OFFSET_CHECK = 0, BEWALGO_OFFSET_CHECK = 1 } bewalgo_offset_check;

/********************************************************************************/

#ifdef LOG_STATISTICS
/*
 * log compressor statistics to file:
 * - which code is executed how ofthen?
 * - are the unit tests covering the whole code?
 */
#else
#ifdef APPEND_LOG
#undef APPEND_LOG
#endif
#define APPEND_LOG(...) \
	do {                \
	} while (0)
#endif

#define BEWALGO_COMPRESS_STRINGIFY_HELPER(x) #x
#define BEWALGO_COMPRESS_STRINGIFY(x) BEWALGO_COMPRESS_STRINGIFY_HELPER (x)
#define BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO_HELPER2(name, datatype) name##_##datatype
#define BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO_HELPER(name, datatype) BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO_HELPER2 (name, datatype)
#define BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO(name) BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO_HELPER (name, BEWALGO_COMPRESS_DATA_TYPE)

/* internal helper functions -->> */
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
#define bewalgo_copy_helper_src(dest, src, target) \
	do {                                           \
		while ((src) < (target) -3) {              \
			(dest)[0] = (src)[0];                  \
			(dest)[1] = (src)[1];                  \
			(dest)[2] = (src)[2];                  \
			(dest)[3] = (src)[3];                  \
			(dest) += 4;                           \
			(src) += 4;                            \
		}                                          \
		if ((src) < (target) -1) {                 \
			(dest)[0] = (src)[0];                  \
			(dest)[1] = (src)[1];                  \
			(dest) += 2;                           \
			(src) += 2;                            \
		}                                          \
		if ((src) < (target)) {                    \
			*(dest)++ = *(src)++;                  \
		}                                          \
	} while (0)
#define bewalgo_copy_helper_dest(dest, src, target) \
	do {                                            \
		while ((dest) < (target) -3) {              \
			(dest)[0] = (src)[0];                   \
			(dest)[1] = (src)[1];                   \
			(dest)[2] = (src)[2];                   \
			(dest)[3] = (src)[3];                   \
			(dest) += 4;                            \
			(src) += 4;                             \
		}                                           \
		if ((dest) < (target) -1) {                 \
			(dest)[0] = (src)[0];                   \
			(dest)[1] = (src)[1];                   \
			(dest) += 2;                            \
			(src) += 2;                             \
		}                                           \
		if ((dest) < (target)) {                    \
			*(dest)++ = *(src)++;                   \
		}                                           \
	} while (0)
#else /* BEWALGO_COMPRESS_DATA_TYPE_SHIFT !=3 */
#define bewalgo_copy_helper_src(dest, src, target) \
	do {                                           \
		while ((src) < (target) -3) {              \
			((U64*) (dest))[0] = ((U64*) src)[0];  \
			((U64*) (dest))[1] = ((U64*) src)[1];  \
			(dest) += 4;                           \
			(src) += 4;                            \
		}                                          \
		if ((src) < (target) -1) {                 \
			((U64*) (dest))[0] = ((U64*) src)[0];  \
			(dest) += 2;                           \
			(src) += 2;                            \
		}                                          \
		if ((src) < (target)) {                    \
			*(dest)++ = *(src)++;                  \
		}                                          \
	} while (0)
#define bewalgo_copy_helper_dest(dest, src, target) \
	do {                                            \
		while ((dest) < (target) -3) {              \
			((U64*) (dest))[0] = ((U64*) src)[0];   \
			((U64*) (dest))[1] = ((U64*) src)[1];   \
			(dest) += 4;                            \
			(src) += 4;                             \
		}                                           \
		if ((dest) < (target) -1) {                 \
			((U64*) (dest))[0] = ((U64*) src)[0];   \
			(dest) += 2;                            \
			(src) += 2;                             \
		}                                           \
		if ((dest) < (target)) {                    \
			*(dest)++ = *(src)++;                   \
		}                                           \
	} while (0)
#endif /* BEWALGO_COMPRESS_DATA_TYPE_SHIFT !=3 */
/* <<-- internal helper functions */

#endif /* BEWALGO_COMPRESS_H_ */
