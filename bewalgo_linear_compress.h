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
			if ((*(U64*) match == *(U64*) ip) && (offset_check || (ip - match <= BEWALGO_OFFSET_MAX))) {
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
			if ((*(U64*) match == *(U64*) ip) && (offset_check || (ip - match <= BEWALGO_OFFSET_MAX))) {
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
		/* write code omited */
		anchor = ip;
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
/* write code omited */
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
		if ((*(U64*) match == *(U64*) ip) && (offset_check || (ip - match <= BEWALGO_OFFSET_MAX))) {
			INC_COUNTER_COMPRESSOR;
			/* consecutive match */
			goto _find_match_right;
		}
	} while (1);
_encode_last_literal:
	INC_COUNTER_COMPRESSOR;
/* write code omited */
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
	return 1;
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
