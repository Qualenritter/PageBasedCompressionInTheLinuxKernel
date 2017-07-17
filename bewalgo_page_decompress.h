/*
 * bewalgo_page_decompress.c
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
	bewalgo_page_decompress_generic (bewalgo_page_helper* const ip, bewalgo_page_helper* const op, const int source_length, const int dest_length, const bewalgo_safety_mode safe_mode) {
	bewalgo_page_helper		   __match;
	bewalgo_page_helper*	   match	  = &__match;
	const U32				   source_end = source_length >> BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
	const U32				   dest_end   = dest_length >> BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
	BEWALGO_COMPRESS_DATA_TYPE controll_block;
	const BYTE* const		   controll_block_ptr = (BYTE*) &controll_block;
	U32						   len_src;
	U32						   len_dest;
	U32						   to_read;
	U32						   avail;
	bewalgo_page_helper_clone (match, op);
	do {
		if (!safe_mode && unlikely (dest_end <= op->position))
			goto _last_control_block; /* finish? */
		if (ip->page_pointer == ip->page_pointer_end) {
			INC_COUNTER_PAGE_HELPER;
			bewalgo_page_helper_forward (ip);
		}
		ip->position++;
		controll_block = *(ip->page_pointer)++;
		if (safe_mode) {
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			len_src  = controll_block_ptr[0] + controll_block_ptr[4];
			len_dest = len_src + controll_block_ptr[1] + controll_block_ptr[5];
#else
			len_src  = controll_block_ptr[0];
			len_dest = len_src + controll_block_ptr[1];
#endif
			if (unlikely (source_end < len_src + ip->position))
				goto _error;
			if (unlikely (dest_end <= len_dest + op->position))
				goto _last_control_block;
		}

		bewalgo_page_helper_copy (op, ip, controll_block_ptr[0]); /* decode literal */
		if (controll_block_ptr[1] > 0) {
			bewalgo_page_helper_set_position (match, op->position - ((U16*) controll_block_ptr)[1]);
			bewalgo_page_helper_copy (op, match, controll_block_ptr[1]); /* decode match */
		}
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		bewalgo_page_helper_copy (op, ip, controll_block_ptr[4]); /* decode literal */
		if (controll_block_ptr[5] > 0) {
			bewalgo_page_helper_set_position (match, op->position - ((U16*) controll_block_ptr)[3]);
			bewalgo_page_helper_copy (op, match, controll_block_ptr[5]); /* decode match */
		}
#endif
	} while (1);
_last_control_block:
	to_read = controll_block_ptr[0];
	avail   = dest_end - op->position;
	bewalgo_page_helper_copy (op, ip, MIN (to_read, avail)); /* decode literal */
	if (controll_block_ptr[1] > 0) {
		bewalgo_page_helper_set_position (match, op->position - ((U16*) controll_block_ptr)[1]);
		to_read = controll_block_ptr[1];
		avail   = dest_end - op->position;
		bewalgo_page_helper_copy (op, match, MIN (to_read, avail)); /* decode match */
	}
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
	to_read = controll_block_ptr[4];
	avail   = dest_end - op->position;
	bewalgo_page_helper_copy (op, ip, MIN (to_read, avail)); /* decode literal */
	if (controll_block_ptr[5] > 0) {
		bewalgo_page_helper_set_position (match, op->position - ((U16*) controll_block_ptr)[3]);
		to_read = controll_block_ptr[5];
		avail   = dest_end - op->position;
		bewalgo_page_helper_copy (op, match, MIN (to_read, avail)); /* decode match */
	}
#endif
	bewalgo_page_helper_free (match);
	return op->position << BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
_error:
	bewalgo_page_helper_free (match);
	return -1;
}
static bewalgo_compress_always_inline int bewalgo_page_decompress_helper (void* src, void* dst, const int source_length, const int dest_length, const bewalgo_safety_mode safe_mode) {
	struct page**		source				= (struct page**) src;
	struct page**		dest				= (struct page**) dst;
	const int			final_source_length = ROUND_UP_TO_NEXT_DIVIDEABLE (source_length, BEWALGO_COMPRESS_DATA_TYPE_SHIFT);
	const int			final_dest_length   = ROUND_UP_TO_NEXT_DIVIDEABLE (dest_length, BEWALGO_COMPRESS_DATA_TYPE_SHIFT);
	int					res;
	bewalgo_page_helper ip;
	bewalgo_page_helper op;
	res = bewalgo_page_helper_init (&ip, source, final_source_length);
	if (res < 0) {
		return res;
	}
	res = bewalgo_page_helper_init (&op, dest, final_dest_length);
	if (res < 0) {
		bewalgo_page_helper_free (&ip);
		return res;
	}
	res = bewalgo_page_decompress_generic (&ip, &op, final_source_length, final_dest_length, safe_mode);
	bewalgo_page_helper_free (&ip);
	bewalgo_page_helper_free (&op);
	return res;
}
__attribute__ ((no_instrument_function)) static int BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_page_decompress_fast) (void* source, void* dest, const int source_length, const int dest_length) {
	return bewalgo_page_decompress_helper (source, dest, source_length, dest_length, BEWALGO_UNSAFE);
}
__attribute__ ((no_instrument_function)) static int BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_page_decompress_save) (void* source, void* dest, const int source_length, const int dest_length) {
	return bewalgo_page_decompress_helper (source, dest, source_length, dest_length, BEWALGO_SAFE);
}
