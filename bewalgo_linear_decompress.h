/*
 * bewalgo_linear_decompress.c
 *
 *  Created on: Mar 31, 2017
 *      Author: benjamin
 */

#include "bewalgo_compress_defs.h"
MODULE_LICENSE ("Dual BSD/GPL");
MODULE_AUTHOR ("Benjamin Warnke");
MODULE_DESCRIPTION ("decompression module");
MODULE_VERSION ("1.0.0");

static bewalgo_compress_always_inline int
	bewalgo_linear_decompress_generic (const BEWALGO_COMPRESS_DATA_TYPE* const source, BEWALGO_COMPRESS_DATA_TYPE* const dest, const int source_length, const int dest_length, const bewalgo_safety_mode safe_mode) {
	BEWALGO_COMPRESS_DATA_TYPE*				match			   = dest;
	const BEWALGO_COMPRESS_DATA_TYPE*		ip				   = source;
	BEWALGO_COMPRESS_DATA_TYPE*				op				   = dest;
	const U32								dest_end		   = dest_length >> BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
	const U32								source_end		   = source_length >> BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
	const BEWALGO_COMPRESS_DATA_TYPE* const dest_end_ptr	   = dest + dest_end;
	const BEWALGO_COMPRESS_DATA_TYPE* const source_end_ptr	 = source + source_end;
	const BYTE*								controll_block_ptr = (BYTE*) ip;
	const BEWALGO_COMPRESS_DATA_TYPE*		target;
	U32										len_src;
	U32										len_dest;
	U32										to_read;
	U32										avail;
	do {
		if ((safe_mode == BEWALGO_UNSAFE) & unlikely (dest_end_ptr <= op))
			goto _last_control_block; /* finish? */
		controll_block_ptr = (BYTE*) ip;
		ip++;
		if (safe_mode == BEWALGO_SAFE) {
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			len_src  = *(controll_block_ptr) + *(controll_block_ptr + 4);
			len_dest = len_src + *(controll_block_ptr + 1) + *(controll_block_ptr + 5);
#else
			len_src  = *(controll_block_ptr);
			len_dest = len_src + *(controll_block_ptr + 1);
#endif
			if (unlikely (source_end_ptr < len_src + ip))
				goto _error;
			if (unlikely (dest_end_ptr <= len_dest + op))
				goto _last_control_block;
		}
		target = ip + controll_block_ptr[0];
		bewalgo_copy_helper_src (op, ip, target); /* decode literal */
		match  = op - ((U16*) controll_block_ptr)[1];
		target = match + controll_block_ptr[1];
		bewalgo_copy_helper_src (op, match, target); /* decode match */
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		target = ip + controll_block_ptr[4];
		bewalgo_copy_helper_src (op, ip, target); /* decode literal */
		match  = op - ((U16*) controll_block_ptr)[3];
		target = match + controll_block_ptr[5];
		bewalgo_copy_helper_src (op, match, target); /* decode match */
#endif
	} while (1);
_last_control_block:
	to_read = controll_block_ptr[0];
	avail   = dest_end_ptr - op;
	target  = ip + MIN (to_read, avail);
	bewalgo_copy_helper_src (op, ip, target); /* decode literal */
	match   = op - ((U16*) controll_block_ptr)[1];
	to_read = controll_block_ptr[1];
	avail   = dest_end_ptr - op;
	target  = match + MIN (to_read, avail);
	bewalgo_copy_helper_src (op, match, target); /* decode match */
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
	to_read = controll_block_ptr[4];
	avail   = dest_end_ptr - op;
	target  = ip + MIN (to_read, avail);
	bewalgo_copy_helper_src (op, ip, target); /* decode literal */
	match   = op - ((U16*) controll_block_ptr)[3];
	to_read = controll_block_ptr[5];
	avail   = dest_end_ptr - op;
	target  = match + MIN (to_read, avail);
	bewalgo_copy_helper_src (op, match, target); /* decode match */
#endif
	return (op - dest) << BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
_error:
	return -1;
}

__attribute__ ((no_instrument_function)) static int
	BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_decompress_fast) (void* const source, void* const dest, const int source_length, const int dest_length) {
	const int final_source_length = ROUND_UP_TO_NEXT_DIVIDEABLE (source_length, BEWALGO_COMPRESS_DATA_TYPE_SHIFT);
	const int final_dest_length   = ROUND_UP_TO_NEXT_DIVIDEABLE (dest_length, BEWALGO_COMPRESS_DATA_TYPE_SHIFT);
	return bewalgo_linear_decompress_generic (source, dest, final_source_length, final_dest_length, BEWALGO_UNSAFE);
}
__attribute__ ((no_instrument_function)) static int
	BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_decompress_save) (void* const source, void* const dest, const int source_length, const int dest_length) {
	const int final_source_length = ROUND_UP_TO_NEXT_DIVIDEABLE (source_length, BEWALGO_COMPRESS_DATA_TYPE_SHIFT);
	const int final_dest_length   = ROUND_UP_TO_NEXT_DIVIDEABLE (dest_length, BEWALGO_COMPRESS_DATA_TYPE_SHIFT);
	return bewalgo_linear_decompress_generic (source, dest, final_source_length, final_dest_length, BEWALGO_SAFE);
}
