/*
 * bewalgo_linear_compress.c
 *
 *  Created on: 30.03.2017
 *      Author: benjamin
 */

#define counters_size 1500
#include "bewalgo_compress_defs.h"
#include "bewalgo_page_helper.h"

MODULE_LICENSE ("Dual BSD/GPL");
MODULE_AUTHOR ("Benjamin Warnke");
MODULE_DESCRIPTION ("BeWalgo compression module");
MODULE_VERSION ("1.0.0");

static bewalgo_compress_always_inline U32 bewalgo_compress_hash (BEWALGO_COMPRESS_DATA_TYPE sequence) {
	return (U32) (((sequence >> 24) * 11400714785074694791ULL) >> (64 - BEWALGO_HASHLOG));
}

static bewalgo_compress_always_inline int bewalgo_linear_compress_generic (bewalgo_compress_internal*			   wrkmem,
																		   const BEWALGO_COMPRESS_DATA_TYPE* const source,
																		   BEWALGO_COMPRESS_DATA_TYPE* const	   dest,
																		   const int							   source_length,
																		   const int							   dest_length,
																		   int									   acceleration,
																		   const bewalgo_safety_mode			   safe_mode,
																		   const bewalgo_offset_check			   offset_check) {
	const int								acceleration_start = (acceleration < 4 ? 32 >> acceleration : 4);
	const U32								source_end		   = source_length >> BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
	const U32								dest_end		   = dest_length >> BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
	const BEWALGO_COMPRESS_DATA_TYPE* const source_end_ptr	 = source + source_end;
	const BEWALGO_COMPRESS_DATA_TYPE* const source_end_ptr_1   = source_end_ptr - 1;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 2
	const BEWALGO_COMPRESS_DATA_TYPE* const source_end_ptr_2 = source_end_ptr - 2;
#endif
	const BEWALGO_COMPRESS_DATA_TYPE* const dest_end_ptr = dest + dest_end;
	const BEWALGO_COMPRESS_DATA_TYPE*		match		 = source;
	const BEWALGO_COMPRESS_DATA_TYPE*		anchor		 = source; /* 'anchor' is pointing to the first not encoded input */
	const BEWALGO_COMPRESS_DATA_TYPE*		ip			 = source;
	const BEWALGO_COMPRESS_DATA_TYPE*		target;
	BEWALGO_COMPRESS_DATA_TYPE*				op					 = dest;
	BYTE*									op_control			 = NULL;
	U32										op_control_available = 0;
	int length; /* must be signed - contains different lengths at differend code positions. This helps to generate better code due to variable reusing. */
	U16 offset;
	U32 h;
	int j; /* must be signed */
	if (dest_length == 0)
		return -(source_length != 0); /* invalid input */
#if (BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3) || (BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 2)
#ifdef LOG_STATISTICS
	memset (counters, 0, sizeof (int) * counters_size);
	/* which loop is executed how ofthen? - initialize all with zero */
	if (open_log_file (LOG_FILE_STATISTIC (BEWALGO_COMPRESS_DATA_TYPE) "linear.csv"))
		goto _error;
#endif
	h				 = bewalgo_compress_hash (*ip);
	wrkmem->table[h] = 0;
	do { /* while there is data*/
		 /* search for next match */
		INC_COUNTER_COMPRESSOR;
		j = acceleration_start;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		length = source_end_ptr_1 - ip;
#else
		length = source_end_ptr_2 - ip;
#endif
		j = j < length ? j : length;
		for (length = 1; length <= j; length++) {
			/* search for next match stepsize = 1*/
			INC_COUNTER_COMPRESSOR;
			ip++;
			h				 = bewalgo_compress_hash (*ip);
			match			 = source + wrkmem->table[h];
			wrkmem->table[h] = ip - source;
			if ((*(U64*) match == *(U64*) ip) & ((offset_check == BEWALGO_NO_OFFSET_CHECK) || (ip - match <= BEWALGO_OFFSET_MAX))) {
				INC_COUNTER_COMPRESSOR;
				goto _find_match_left;
			}
		}
		length = acceleration_start + (acceleration << BEWALGO_SKIPTRIGGER);
		ip++;
		do {
			/* search for next match stepsize > 1*/
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			if (unlikely (ip >= source_end_ptr)) {
				INC_COUNTER_COMPRESSOR;
				goto _encode_last_literal;
			}
#else
			if (unlikely (ip >= source_end_ptr_1)) {
				INC_COUNTER_COMPRESSOR;
				ip = source_end_ptr;
				goto _encode_last_literal;
			}
#endif
			h				 = bewalgo_compress_hash (*ip);
			match			 = source + wrkmem->table[h];
			wrkmem->table[h] = ip - source;
			if ((*(U64*) match == *(U64*) ip) & ((offset_check == BEWALGO_NO_OFFSET_CHECK) || (ip - match <= BEWALGO_OFFSET_MAX))) {
				INC_COUNTER_COMPRESSOR;
				goto _find_match_left;
			}
			ip += (length++ >> BEWALGO_SKIPTRIGGER);
		} while (1);
	_find_match_left:
		/* search for the left end of the match or the last already encoded data */
		INC_COUNTER_COMPRESSOR;
		while ((match != source) && (match[-1] == ip[-1])) {
			INC_COUNTER_COMPRESSOR;
			ip--;
			match--;
			if (ip == anchor) {
				INC_COUNTER_COMPRESSOR;
				goto _find_match_right;
			}
		}
	_encode_literal:
		/* save the found literal to output buffer */
		INC_COUNTER_COMPRESSOR;
		/* 'ip' and 'match' pointing to the begin of the match */
		length = ip - anchor; /* length currently contains the length of the literal*/
		/* APPEND_LOG ("literal,%zu\n", length); */
		if (safe_mode) {
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			int tmp_literal_length = length - (op_control_available ? BEWALGO_LENGTH_MAX : 0);
			if (unlikely (op + (tmp_literal_length / (BEWALGO_LENGTH_MAX * 2)) + ((tmp_literal_length % (BEWALGO_LENGTH_MAX * 2)) > 0) + length > dest_end_ptr)) {
				INC_COUNTER_COMPRESSOR;
				goto _error;
			}
#else
			if (unlikely (op + (length / BEWALGO_LENGTH_MAX) + ((length % BEWALGO_LENGTH_MAX) > 0) + length > dest_end_ptr)) {
				INC_COUNTER_COMPRESSOR;
				goto _error;
			}
#endif
		}
		while (length > BEWALGO_LENGTH_MAX) {
			/* encode a long literal */
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			if (op_control_available == 0) {
				INC_COUNTER_COMPRESSOR;
				op_control = (BYTE*) op;
				*op++	  = 0;
			}
			op_control_available = !op_control_available;
			*op_control			 = BEWALGO_LENGTH_MAX;
			op_control += 4;
#else
			*op++		   = 0;
			op_control	 = (BYTE*) op;
			op_control[-4] = BEWALGO_LENGTH_MAX;
#endif
			target = anchor + BEWALGO_LENGTH_MAX;
			bewalgo_copy_helper_src (op, anchor, target);
			length -= BEWALGO_LENGTH_MAX;
		}
		if (likely (length > 0)) {
			/* encode the short (end of) a literal */
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			if (op_control_available == 0) {
				INC_COUNTER_COMPRESSOR;
				op_control = (BYTE*) op;
				*op++	  = 0;
			}
			op_control_available = !op_control_available;
			*op_control			 = length;
			op_control += 4;
#else
			*op++		   = 0;
			op_control	 = (BYTE*) op;
			op_control[-4] = length;
#endif
			bewalgo_copy_helper_src (op, anchor, ip);
		}
	_find_match_right:
		/* find the right end of a match */
		INC_COUNTER_COMPRESSOR;
		do {
			INC_COUNTER_COMPRESSOR;
			ip++;
			match++;
		} while ((ip < source_end_ptr) && (*match == *ip));
		/*_encode_match:*/
		/* 'ip' and 'match' pointing after the end of the match */
		length = ip - anchor; /* length currently contains the length of the match*/
		offset = ip - match;
		anchor = ip;
		/* APPEND_LOG ("match,%zu\n", length); */
		/* APPEND_LOG ("offset,%d\n", offset); */
		if (length > BEWALGO_LENGTH_MAX) {
			/* encode a long match */
			INC_COUNTER_COMPRESSOR;
			U32	control_match_value  = (BEWALGO_LENGTH_MAX << 8) | (offset << 16);
			size_t match_length_div_255 = length / 255;
			size_t match_length_mod_255 = length % 255;
			int	match_zero			= match_length_mod_255 == 0;
			int	match_nzero			= !match_zero;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			if (safe_mode == BEWALGO_SAFE) {
				INC_COUNTER_COMPRESSOR;
				int control_blocks_needed = match_length_div_255 + match_nzero - op_control_available;
				if (safe_mode & unlikely ((op + ((control_blocks_needed >> 1) + (control_blocks_needed & 1))) > dest_end_ptr)) {
					INC_COUNTER_COMPRESSOR;
					goto _error;
				}
			}
			op_control			 = op_control_available > 0 ? op_control : (BYTE*) op;
			*((U32*) op_control) = control_match_value;
			match_length_div_255 -= op_control_available > 0;
			{
				int match_nodd		 = !(match_length_div_255 & 1);
				int match_nzero_nodd = match_nzero && match_nodd;
				if (match_length_div_255 > 0) {
					INC_COUNTER_COMPRESSOR;
					U64 control_match_value_long = control_match_value | (((U64) control_match_value) << 32);
					target						 = op + (match_length_div_255 >> 1) + (match_length_div_255 & 1);
					do {
						INC_COUNTER_COMPRESSOR;
						*op++ = control_match_value_long;
					} while (op < target);
				}
				op_control										   = ((BYTE*) op) - 4;
				*(U32*) (op_control + (match_nzero_nodd << 3))	 = 0;
				*(U32*) (op_control + (match_nzero_nodd << 2) + 0) = 0;
				*(op_control + (match_nzero_nodd << 2) + 1)		   = (match_zero & match_nodd) ? BEWALGO_LENGTH_MAX : match_length_mod_255;
				*(U16*) (op_control + (match_nzero_nodd << 2) + 2) = offset;
				op_control += match_nzero_nodd << 3;
				op += match_nzero_nodd;
				op_control_available = (match_length_div_255 & 1) == match_zero;
			}
#else
			if (safe_mode & unlikely ((op + match_length_div_255 + match_nzero) > dest_end_ptr)) {
				INC_COUNTER_COMPRESSOR;
				goto _error;
			}
			target = op + match_length_div_255;
			while (op < target) {
				INC_COUNTER_COMPRESSOR;
				*op++ = control_match_value;
			}
			if (match_length_mod_255 > 0) {
				INC_COUNTER_COMPRESSOR;
				((BYTE*) op)[0] = 0;
				((BYTE*) op)[1] = match_length_mod_255;
				((U16*) op)[1]  = offset;
				op++;
			}
			op_control	 = ((BYTE*) op);
#endif
		} else {
			/* encode a short match */
			INC_COUNTER_COMPRESSOR;
			if (safe_mode & unlikely ((op_control_available == 0) & (op >= dest_end_ptr) & (op_control[-3] != 0))) {
				INC_COUNTER_COMPRESSOR;
				goto _error;
			}
			if (op_control[-3] != 0) {
				INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
				if (op_control_available == 0) {
					INC_COUNTER_COMPRESSOR;
					op_control = (BYTE*) op;
					*op++	  = 0;
				}
				op_control_available = !op_control_available;
				op_control += 4;
#else
				*op++	  = 0;
				op_control = (BYTE*) op;
#endif
			}
			op_control[-3]			= length;
			((U16*) op_control)[-1] = offset;
		}
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		if (unlikely (ip == source_end_ptr)) {
			INC_COUNTER_COMPRESSOR;
			goto _finish;
		}
#else
		if (unlikely (ip >= source_end_ptr_1)) {
			/* terminate? */
			INC_COUNTER_COMPRESSOR;
			if (unlikely (ip == source_end_ptr)) {
				INC_COUNTER_COMPRESSOR;
				goto _finish;
			}
			ip = source_end_ptr;
			goto _encode_last_literal;
		}
#endif
		h				 = bewalgo_compress_hash (*ip);
		match			 = source + wrkmem->table[h];
		wrkmem->table[h] = ip - source;
		if ((*(U64*) match == *(U64*) ip) & ((offset_check == BEWALGO_NO_OFFSET_CHECK) || (ip - match <= BEWALGO_OFFSET_MAX))) {
			INC_COUNTER_COMPRESSOR;
			/* consecutive match */
			goto _find_match_right;
		}
	} while (1);
_encode_last_literal:
	INC_COUNTER_COMPRESSOR;
	/* 'ip' and 'match' pointing to the begin of the match */
	length = source_end_ptr - anchor;
	/* APPEND_LOG ("literal,%zu\n", length); */
	if (safe_mode) {
		INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		int tmp_literal_length = length - (op_control_available ? BEWALGO_LENGTH_MAX : 0);
		if (op + (tmp_literal_length / (BEWALGO_LENGTH_MAX * 2)) + ((tmp_literal_length % (BEWALGO_LENGTH_MAX * 2)) > 0) + length > dest_end_ptr) {
			INC_COUNTER_COMPRESSOR;
			goto _error;
		}
#else
		if (op + (length / BEWALGO_LENGTH_MAX) + ((length % BEWALGO_LENGTH_MAX) > 0) + length > dest_end_ptr) {
			INC_COUNTER_COMPRESSOR;
			goto _error;
		}
#endif
	}
	while (length > BEWALGO_LENGTH_MAX) {
		/* encode a long literal*/
		INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		if (op_control_available == 0) {
			INC_COUNTER_COMPRESSOR;
			op_control = (BYTE*) op;
			*op++	  = 0;
		}
		op_control_available = !op_control_available;
		*op_control			 = BEWALGO_LENGTH_MAX;
		op_control += 4;
#else
		*op++		   = 0;
		op_control	 = (BYTE*) op;
		op_control[-4] = BEWALGO_LENGTH_MAX;
#endif
		target = anchor + BEWALGO_LENGTH_MAX;
		bewalgo_copy_helper_src (op, anchor, target);
		length -= BEWALGO_LENGTH_MAX;
	}
	if (length > 0) {
		/* encode a short (end of) a literal*/
		INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		if (op_control_available == 0) {
			INC_COUNTER_COMPRESSOR;
			op_control = (BYTE*) op;
			*op++	  = 0;
		}
		op_control_available = !op_control_available;
		*op_control			 = length;
		op_control += 4;
#else
		*op++		   = 0;
		op_control	 = (BYTE*) op;
		op_control[-4] = length;
#endif
		bewalgo_copy_helper_src (op, anchor, source_end_ptr);
	}
_finish:
	INC_COUNTER_COMPRESSOR;
#ifdef LOG_STATISTICS
	for (h = 0; h < counters_size; h++) {
		if (counters[h] > 0) {
			APPEND_LOG ("%llu,%d\n", counters[h], h);
		}
	}
	close_log_file ();
#endif
	return (op - dest) << BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
_error:
#ifdef LOG_STATISTICS
	for (h = 0; h < counters_size; h++) {
		if (counters[h] > 0) {
			APPEND_LOG ("%llu,%d\n", counters[h], h);
		}
	}
	close_log_file ();
#endif
#endif
	return -1;
}

__attribute__ ((no_instrument_function)) static int
	BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_compress_fast) (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration) {
	const int final_source_length = ROUND_UP_TO_NEXT_DIVIDEABLE (source_length, BEWALGO_COMPRESS_DATA_TYPE_SHIFT);
	const int final_dest_length   = ROUND_UP_TO_NEXT_DIVIDEABLE (dest_length, BEWALGO_COMPRESS_DATA_TYPE_SHIFT);
	if (dest_length < BEWALGO_COMPRESS_BOUND (source_length)) {
		if (source_length <= (BEWALGO_OFFSET_MAX << BEWALGO_COMPRESS_DATA_TYPE_SHIFT)) {
			return bewalgo_linear_compress_generic (wrkmem, source, dest, final_source_length, final_dest_length, acceleration, BEWALGO_SAFE, BEWALGO_NO_OFFSET_CHECK);
		} else {
			return bewalgo_linear_compress_generic (wrkmem, source, dest, final_source_length, final_dest_length, acceleration, BEWALGO_SAFE, BEWALGO_OFFSET_CHECK);
		}
	} else {
		if (source_length <= (BEWALGO_OFFSET_MAX << BEWALGO_COMPRESS_DATA_TYPE_SHIFT)) {
			return bewalgo_linear_compress_generic (wrkmem, source, dest, final_source_length, final_dest_length, acceleration, BEWALGO_UNSAFE, BEWALGO_NO_OFFSET_CHECK);
		} else {
			return bewalgo_linear_compress_generic (wrkmem, source, dest, final_source_length, final_dest_length, acceleration, BEWALGO_UNSAFE, BEWALGO_OFFSET_CHECK);
		}
	}
}

__attribute__ ((no_instrument_function)) static int
	BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_compress_default) (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length) {
	return BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_compress_fast) (wrkmem, source, dest, source_length, dest_length, BEWALGO_ACCELERATION_DEFAULT);
}
