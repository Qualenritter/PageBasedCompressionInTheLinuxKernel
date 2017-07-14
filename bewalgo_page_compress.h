/*
 * bewalgo_page_compress.c
 *
 *  Created on: 30.03.2017
 *      Author: benjamin
 */

#define counters_size 1500
#include "bewalgo_compress_defs.h"
#include "bewalgo_page_helper.h"
MODULE_LICENSE ("Dual BSD/GPL");
MODULE_AUTHOR ("Benjamin Warnke");
MODULE_DESCRIPTION ("compression module");
MODULE_VERSION ("1.0.0");

static bewalgo_compress_always_inline U32 bewalgo_compress_hash (BEWALGO_COMPRESS_DATA_TYPE sequence) {
	return (U32) (((sequence >> 24) * 11400714785074694791ULL) >> (64 - BEWALGO_HASHLOG));
}

static bewalgo_compress_always_inline int bewalgo_page_compress_generic (bewalgo_compress_internal* wrkmem,
																		 bewalgo_page_helper* const ip,
																		 bewalgo_page_helper* const op,
																		 const int					source_length,
																		 const int					dest_length,
																		 int						acceleration,
																		 const bewalgo_safety_mode  safe_mode,
																		 const bewalgo_offset_check offset_check) {
	bewalgo_page_helper		   __match;
	bewalgo_page_helper		   __anchor;
	bewalgo_page_helper* const match		= &__match;
	bewalgo_page_helper* const anchor		= &__anchor; /* 'anchor' is pointing to the first not encoded input */
	const int				   source_end   = source_length >> BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
	const int				   source_end_1 = source_end - 1;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 2
	const int source_end_2 = source_end - 2;
#endif
	const int dest_end			   = dest_length >> BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
	const U32 acceleration_start   = (acceleration < 4 ? 32 >> acceleration : 4);
	BYTE*	 op_control		   = NULL;
	U32		  op_control_available = 0;
	U32		  match_length_div_255 = 0;
	U32		  match_length_mod_255 = 0;
	U32		  match_zero		   = 0;
	U32		  match_nzero		   = 0;
	U32		  h;
	U32		  i;
	int		  j; /* must be signed */
	int		  k; /* must be signed */
	int		  searchTrigger;
	int		  length; /* must be signed - contains different lengths at differend code positions. This helps to generate better code due to variable reusing. */
	U32		  tmpu32;
	U16		  offset;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
	int tmp_literal_length;
#endif
	if (dest_length == 0)
		return -(source_length != 0);
	bewalgo_page_helper_clone (match, ip);
	bewalgo_page_helper_clone (anchor, ip);
#ifdef LOG_STATISTICS
	memset (counters, 0, sizeof (int) * counters_size);
	if (open_log_file (LOG_FILE_STATISTIC (BEWALGO_COMPRESS_DATA_TYPE) "page.csv")) {
		return 0;
	}
#endif
	ip->position++;
	h				 = bewalgo_compress_hash (*(ip->page_pointer)++);
	wrkmem->table[h] = 0;
	do {
		/* while there is data to compress */
		/* search for a match */
		INC_COUNTER_COMPRESSOR;
		if (ip->page_pointer == ip->page_pointer_end) {
			INC_COUNTER_COMPRESSOR;
			bewalgo_page_helper_forward (ip);
		}
		j = acceleration_start;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		k = source_end_1 - ip->position;
#else
		k = source_end_2 - ip->position;
#endif
		j = j < k ? j : k;
		k = ip->page_pointer_end - ip->page_pointer - 1;
		j = j < k ? j : k;
		for (searchTrigger = 1; searchTrigger <= j; searchTrigger++) {
			/* search for a match stepsize = 1 on the current page*/
			INC_COUNTER_COMPRESSOR;
			h						= bewalgo_compress_hash (*(ip->page_pointer));
			match->position			= wrkmem->table[h];
			match->page_index		= match->position >> BEWALGO_COMPRESS_PAGE_SHIFT;
			match->page_pointer		= match->page_mapped[match->page_index];
			match->page_pointer_end = match->page_mapped[match->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			match->page_pointer += match->position & BEWALGO_COMPRESS_PAGE_OFFSET_MASK;
			wrkmem->table[h] = ip->position;
			if (((offset_check == BEWALGO_NO_OFFSET_CHECK) || (ip->position - match->position <= BEWALGO_OFFSET_MAX)) && (*(ip->page_pointer) == *(match->page_pointer))) {
				INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 2
				if (match->page_pointer + 1 >= match->page_pointer_end) {
					INC_COUNTER_COMPRESSOR;
					if (ip->page_pointer[1] == (match->page_mapped[match->page_index + 1])[match->page_pointer == match->page_pointer_end]) {
						INC_COUNTER_COMPRESSOR;
						goto _find_match_left;
					}
				} else {
					INC_COUNTER_COMPRESSOR;
					if (ip->page_pointer[1] == match->page_pointer[1]) {
						INC_COUNTER_COMPRESSOR;
						goto _find_match_left;
					}
				}
#else
				goto _find_match_left;
#endif
			}
			ip->position++;
			ip->page_pointer++;
		}
		for (; searchTrigger <= acceleration_start; searchTrigger++) {
			/* search for a match stepsize = 1 on the next page*/
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			if (unlikely (ip->position >= source_end)) {
				INC_COUNTER_COMPRESSOR;
				goto _encode_last_literal;
			}
#else
			if (unlikely (ip->position >= source_end_1)) {
				INC_COUNTER_COMPRESSOR;
				goto _encode_last_literal;
			}
#endif
			h						= bewalgo_compress_hash (*(ip->page_pointer));
			match->position			= wrkmem->table[h];
			match->page_index		= match->position >> BEWALGO_COMPRESS_PAGE_SHIFT;
			match->page_pointer		= match->page_mapped[match->page_index];
			match->page_pointer_end = match->page_mapped[match->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			match->page_pointer += match->position & BEWALGO_COMPRESS_PAGE_OFFSET_MASK;
			wrkmem->table[h] = ip->position;
			if (((offset_check == BEWALGO_NO_OFFSET_CHECK) || (ip->position - match->position <= BEWALGO_OFFSET_MAX)) && (*(ip->page_pointer) == *(match->page_pointer))) {
				INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 2
				if (ip->page_pointer + 1 >= ip->page_pointer_end) {
					INC_COUNTER_COMPRESSOR;
					tmpu32 = (ip->page_mapped[ip->page_index + 1])[ip->page_pointer == ip->page_pointer_end];
				} else {
					INC_COUNTER_COMPRESSOR;
					tmpu32 = ip->page_pointer[1];
				}
				if (match->page_pointer + 1 >= match->page_pointer_end) {
					INC_COUNTER_COMPRESSOR;
					if (tmpu32 == (match->page_mapped[match->page_index + 1])[match->page_pointer == match->page_pointer_end]) {
						INC_COUNTER_COMPRESSOR;
						goto _find_match_left;
					}
				} else {
					INC_COUNTER_COMPRESSOR;
					if (tmpu32 == match->page_pointer[1]) {
						INC_COUNTER_COMPRESSOR;
						goto _find_match_left;
					}
				}
#else
				goto _find_match_left;
#endif
			}
			ip->position++;
			ip->page_pointer++;
			if (ip->page_pointer >= ip->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				ip->page_index		 = ip->position >> BEWALGO_COMPRESS_PAGE_SHIFT;
				ip->page_pointer	 = ip->page_mapped[ip->page_index];
				ip->page_pointer_end = ip->page_mapped[ip->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
				ip->page_pointer += ip->position & BEWALGO_COMPRESS_PAGE_OFFSET_MASK;
			}
		}
		searchTrigger = acceleration_start + (acceleration << BEWALGO_SKIPTRIGGER);
		do {
			/* search for a match stepsize > 1 */
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			if (unlikely (ip->position >= source_end)) {
				INC_COUNTER_COMPRESSOR;
				goto _encode_last_literal;
			}
#else
			if (unlikely (ip->position >= source_end_1)) {
				INC_COUNTER_COMPRESSOR;
				goto _encode_last_literal;
			}
#endif
			h						= bewalgo_compress_hash (*(ip->page_pointer));
			match->position			= wrkmem->table[h];
			match->page_index		= match->position >> BEWALGO_COMPRESS_PAGE_SHIFT;
			match->page_pointer		= match->page_mapped[match->page_index];
			match->page_pointer_end = match->page_mapped[match->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			match->page_pointer += match->position & BEWALGO_COMPRESS_PAGE_OFFSET_MASK;
			wrkmem->table[h] = ip->position;
			if (((offset_check == BEWALGO_NO_OFFSET_CHECK) || (ip->position - match->position <= BEWALGO_OFFSET_MAX)) && (*(ip->page_pointer) == *(match->page_pointer))) {
				INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 2
				if (ip->page_pointer + 1 >= ip->page_pointer_end) {
					INC_COUNTER_COMPRESSOR;
					tmpu32 = (ip->page_mapped[ip->page_index + 1])[ip->page_pointer == ip->page_pointer_end];
				} else {
					INC_COUNTER_COMPRESSOR;
					tmpu32 = ip->page_pointer[1];
				}
				if (match->page_pointer + 1 >= match->page_pointer_end) {
					INC_COUNTER_COMPRESSOR;
					if (tmpu32 == (match->page_mapped[match->page_index + 1])[match->page_pointer == match->page_pointer_end]) {
						INC_COUNTER_COMPRESSOR;
						goto _find_match_left;
					}
				} else {
					INC_COUNTER_COMPRESSOR;
					if (tmpu32 == match->page_pointer[1]) {
						INC_COUNTER_COMPRESSOR;
						goto _find_match_left;
					}
				}
#else
				goto _find_match_left;
#endif
			}
			tmpu32 = ip->position + (searchTrigger++ >> BEWALGO_SKIPTRIGGER);
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			tmpu32 = tmpu32 >= source_end ? source_end : tmpu32;
#else
			tmpu32 = tmpu32 >= source_end_1 ? source_end : tmpu32;
#endif
			ip->page_index		 = tmpu32 >> BEWALGO_COMPRESS_PAGE_SHIFT;
			ip->page_pointer	 = ip->page_mapped[ip->page_index];
			ip->page_pointer_end = ip->page_mapped[ip->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			ip->page_pointer += tmpu32 & BEWALGO_COMPRESS_PAGE_OFFSET_MASK;
			ip->position = tmpu32;
		} while (1);
	_find_match_left:
		/* search for the left end of a match or the last encoded data */
		INC_COUNTER_COMPRESSOR;
		do {
			INC_COUNTER_COMPRESSOR;
			if (ip->position == anchor->position) {
				INC_COUNTER_COMPRESSOR;
				goto _find_match_right;
			}
			if (unlikely (match->position == 0)) {
				INC_COUNTER_COMPRESSOR;
				goto _encode_literal;
			}
			if (ip->page_pointer + BEWALGO_COMPRESS_PAGE_SIZE <= ip->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				ip->page_index--;
				ip->page_pointer_end = ip->page_mapped[ip->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
				ip->page_pointer	 = ip->page_pointer_end;
			}
			if (match->page_pointer + BEWALGO_COMPRESS_PAGE_SIZE <= match->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				match->page_index--;
				match->page_pointer_end = match->page_mapped[match->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
				match->page_pointer		= match->page_pointer_end;
			}
			ip->position--;
			ip->page_pointer--;
			match->position--;
			match->page_pointer--;
		} while (*(ip->page_pointer) == *(match->page_pointer));
		match->position++;
		match->page_pointer++;
		ip->position++;
		ip->page_pointer++;
	_encode_literal:
		/* encode the literal */
		INC_COUNTER_COMPRESSOR;
		anchor->page_index		 = anchor->position >> BEWALGO_COMPRESS_PAGE_SHIFT;
		anchor->page_pointer	 = anchor->page_mapped[anchor->page_index];
		anchor->page_pointer_end = anchor->page_mapped[anchor->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
		anchor->page_pointer += anchor->position & BEWALGO_COMPRESS_PAGE_OFFSET_MASK;
		/* 'ip' and 'match' pointing to the begin of the match */
		length = ip->position - anchor->position; /* length of the literal */
		/*		APPEND_LOG ("literal,%d\n", length); */
		if (safe_mode) {
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			tmp_literal_length = length - (op_control_available ? BEWALGO_LENGTH_MAX : 0);
			if (unlikely (op->position + (tmp_literal_length / (BEWALGO_LENGTH_MAX * 2)) + ((tmp_literal_length % (BEWALGO_LENGTH_MAX * 2)) > 0) + length > dest_end)) {
				INC_COUNTER_COMPRESSOR;
				goto _error_dest_end;
			}
#else
			if (unlikely (op->position + (length / BEWALGO_LENGTH_MAX) + ((length % BEWALGO_LENGTH_MAX) > 0) + length > dest_end)) {
				INC_COUNTER_COMPRESSOR;
				goto _error_dest_end;
			}
#endif
		}
		while (length > BEWALGO_LENGTH_MAX) {
			/* encode a long match */
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			if (op_control_available == 0) {
				INC_COUNTER_COMPRESSOR;
				if (op->page_pointer == op->page_pointer_end) {
					INC_COUNTER_COMPRESSOR;
					op->page_index++;
					op->page_pointer	 = op->page_mapped[op->page_index];
					op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
				}
				*(op->page_pointer) = 0;
				op_control			= (BYTE*) (op->page_pointer);
				op->position++;
				op->page_pointer++;
			}
			op_control_available = !op_control_available;
			*op_control			 = BEWALGO_LENGTH_MAX;
			op_control += 4;
#else
			if (op->page_pointer == op->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				op->page_index++;
				op->page_pointer	 = op->page_mapped[op->page_index];
				op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			}
			*op->page_pointer			= 0;
			*((BYTE*) op->page_pointer) = BEWALGO_LENGTH_MAX;
			op->position++;
			op->page_pointer++;
			op_control = (BYTE*) op->page_pointer;
#endif
			bewalgo_page_helper_copy (op, anchor, BEWALGO_LENGTH_MAX);
			length -= BEWALGO_LENGTH_MAX;
		}
		if (likely (length > 0)) {
			/* encode the short (end of) a literal */
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			if (op_control_available == 0) {
				INC_COUNTER_COMPRESSOR;
				if (op->page_pointer == op->page_pointer_end) {
					INC_COUNTER_COMPRESSOR;
					op->page_index++;
					op->page_pointer	 = op->page_mapped[op->page_index];
					op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
				}
				*(op->page_pointer) = 0;
				op_control			= (BYTE*) (op->page_pointer);
				op->position++;
				op->page_pointer++;
			}
			op_control_available = !op_control_available;
			*op_control			 = length;
			op_control += 4;
#else
			if (op->page_pointer == op->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				op->page_index++;
				op->page_pointer	 = op->page_mapped[op->page_index];
				op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			}
			*op->page_pointer			= 0;
			*((BYTE*) op->page_pointer) = length;
			op->position++;
			op->page_pointer++;
			op_control = (BYTE*) op->page_pointer;
#endif
			bewalgo_page_helper_copy (op, anchor, length);
		}
	_find_match_right:
		/* find the right end of a match */
		INC_COUNTER_COMPRESSOR;
		ip->position++;
		match->position++;
		ip->page_pointer++;
		match->page_pointer++;
		do {
			/* high frequent loop */
			INC_COUNTER_COMPRESSOR;
			if (ip->page_pointer >= ip->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				ip->page_index++;
				ip->page_pointer	 = ip->page_mapped[ip->page_index] + (ip->page_pointer > ip->page_pointer_end);
				ip->page_pointer_end = ip->page_mapped[ip->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			}
			if (match->page_pointer >= match->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				match->page_index++;
				match->page_pointer		= match->page_mapped[match->page_index] + (match->page_pointer > match->page_pointer_end);
				match->page_pointer_end = match->page_mapped[match->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			}
			if (ip->position == source_end) {
				INC_COUNTER_COMPRESSOR;
				goto _encode_match;
			}
			tmpu32 = ip->page_pointer_end - ip->page_pointer;
			i	  = match->page_pointer_end - match->page_pointer;
			tmpu32 = i < tmpu32 ? i : tmpu32;
			i	  = source_end - ip->position;
			tmpu32 = i < tmpu32 ? i : tmpu32;
			for (i = 0; i < tmpu32; i++) {
				/* most high frequent loop in the code*/
				INC_COUNTER_COMPRESSOR;
				if (ip->page_pointer[i] != match->page_pointer[i]) {
					INC_COUNTER_COMPRESSOR;
					goto _encode_match_helper;
				}
			}
			ip->position += tmpu32;
			match->position += tmpu32;
			ip->page_pointer += tmpu32;
			match->page_pointer += tmpu32;
		} while (1);
	_encode_match_helper:
		INC_COUNTER_COMPRESSOR;
		ip->position += i;
		match->position += i;
		ip->page_pointer += i;
		match->page_pointer += i;
	_encode_match:
		/* encode the current match */
		INC_COUNTER_COMPRESSOR;
		/* 'ip' and 'match' pointing after the end of the match */
		length = ip->position - anchor->position; /* length of the match */
		offset = ip->position - match->position;
		/* 		APPEND_LOG ("match,%d\n", length); */
		/* 		APPEND_LOG ("offset,%d\n", offset); */
		if (length > BEWALGO_LENGTH_MAX) {
			/* encode a long match */
			INC_COUNTER_COMPRESSOR;
			match_length_div_255 = length / BEWALGO_LENGTH_MAX;
			match_length_mod_255 = length % BEWALGO_LENGTH_MAX;
			match_zero			 = match_length_mod_255 == 0;
			match_nzero			 = !match_zero;
			tmpu32				 = (BEWALGO_LENGTH_MAX << 8) + (((U32) offset) << 16);
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
			if (safe_mode) {
				int control_blocks_needed = match_length_div_255 + match_nzero - op_control_available;
				INC_COUNTER_COMPRESSOR;
				if (safe_mode && unlikely ((op->position + (((control_blocks_needed) >> 1) + (control_blocks_needed & 1))) > dest_end)) {
					INC_COUNTER_COMPRESSOR;
					goto _error_dest_end;
				}
			}
			if (op_control_available > 0) {
				INC_COUNTER_COMPRESSOR;
				op_control_available = 0;
				*((U32*) op_control) = tmpu32;
				op_control += 4;
				match_length_div_255--;
			}
			if (match_length_div_255 > 1) {
				INC_COUNTER_COMPRESSOR;
				bewalgo_page_helper_set (op, tmpu32, match_length_div_255);
				op_control = (BYTE*) (op->page_pointer);
			}
			if (match_length_div_255 & 1) {
				INC_COUNTER_COMPRESSOR;
				if (unlikely (op->page_pointer == op->page_pointer_end)) {
					INC_COUNTER_COMPRESSOR;
					op->page_index++;
					op->page_pointer	 = op->page_mapped[op->page_index];
					op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
				}
				*op->page_pointer		   = 0;
				*((U32*) op->page_pointer) = tmpu32;
				op_control				   = (BYTE*) op->page_pointer;
				op_control += 4;
				op->position++;
				op->page_pointer++;
				op_control_available = 1;
			}
			length = match_length_mod_255;
#else
			if (safe_mode && unlikely ((op->position + match_length_div_255 + match_nzero) > dest_end)) {
				INC_COUNTER_COMPRESSOR;
				goto _error_dest_end;
			}
			bewalgo_page_helper_set (op, tmpu32, match_length_div_255);
			length	 = match_length_mod_255;
			op_control = (BYTE*) op->page_pointer;
#endif
		}
		if (likely (length > 0)) {
			/* encode a short (end of) a match */
			INC_COUNTER_COMPRESSOR;
			if (safe_mode && unlikely ((op_control_available == 0) && (op->position >= dest_end) && (op_control[-3] != 0))) {
				INC_COUNTER_COMPRESSOR;
				goto _error_dest_end;
			}
			if ((*(op_control - 3) != 0)) { /*last control-block contained an match*/
				INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
				if (op_control_available == 0) {
					INC_COUNTER_COMPRESSOR;
					if (op->page_pointer == op->page_pointer_end) {
						INC_COUNTER_COMPRESSOR;
						op->page_index++;
						op->page_pointer	 = op->page_mapped[op->page_index];
						op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
					}
					*(op->page_pointer) = 0;
					op_control			= (BYTE*) (op->page_pointer);
					op->position++;
					op->page_pointer++;
				}
				op_control_available = !op_control_available;
				op_control += 4;
#else
				if (op->page_pointer == op->page_pointer_end) {
					INC_COUNTER_COMPRESSOR;
					op->page_index++;
					op->page_pointer	 = op->page_mapped[op->page_index];
					op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
				}
				*op->page_pointer = 0;
				op->position++;
				op->page_pointer++;
				op_control = (BYTE*) op->page_pointer;
#endif
			} /*last control-block was only a literal*/
			op_control[-3]			= length;
			((U16*) op_control)[-1] = offset;
		}
		anchor->position = ip->position;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		if (unlikely (ip->position == source_end)) {
			INC_COUNTER_COMPRESSOR;
			goto _finish;
		}
#else
		if (unlikely (ip->position >= source_end_1)) {
			INC_COUNTER_COMPRESSOR;
			if (unlikely (ip->position == source_end)) {
				INC_COUNTER_COMPRESSOR;
				goto _finish;
			}
			goto _encode_last_literal;
		}
#endif
		h						= bewalgo_compress_hash (*(ip->page_pointer));
		match->position			= wrkmem->table[h];
		match->page_index		= match->position >> BEWALGO_COMPRESS_PAGE_SHIFT;
		match->page_pointer		= match->page_mapped[match->page_index];
		match->page_pointer_end = match->page_mapped[match->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
		match->page_pointer += match->position & BEWALGO_COMPRESS_PAGE_OFFSET_MASK;
		wrkmem->table[h] = ip->position;
		if (((offset_check == BEWALGO_NO_OFFSET_CHECK) || (ip->position - match->position <= BEWALGO_OFFSET_MAX)) && (*(ip->page_pointer) == *(match->page_pointer))) {
			INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 2
			if (ip->page_pointer + 1 >= ip->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				tmpu32 = (ip->page_mapped[ip->page_index + 1])[ip->page_pointer == ip->page_pointer_end];
			} else {
				INC_COUNTER_COMPRESSOR;
				tmpu32 = ip->page_pointer[1];
			}
			if (match->page_pointer + 1 >= match->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				if (tmpu32 == (match->page_mapped[match->page_index + 1])[match->page_pointer == match->page_pointer_end]) {
					INC_COUNTER_COMPRESSOR;
					/* consecutive match */
					goto _find_match_left;
				}
			} else {
				INC_COUNTER_COMPRESSOR;
				if (tmpu32 == match->page_pointer[1]) {
					INC_COUNTER_COMPRESSOR;
					/* consecutive match */
					goto _find_match_left;
				}
			}
#else
			goto _find_match_right;
#endif
		}
		ip->position++;
		ip->page_pointer++;
	} while (1);
_encode_last_literal:
	INC_COUNTER_COMPRESSOR;
	ip->position			 = source_end;
	anchor->page_index		 = anchor->position >> BEWALGO_COMPRESS_PAGE_SHIFT;
	anchor->page_pointer	 = anchor->page_mapped[anchor->page_index];
	anchor->page_pointer_end = anchor->page_mapped[anchor->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
	anchor->page_pointer += anchor->position & BEWALGO_COMPRESS_PAGE_OFFSET_MASK;
	/* 'ip' and 'match' pointing to the begin of the match */
	length = ip->position - anchor->position;
	/* 		APPEND_LOG ("literal,%d\n", length); */
	if (safe_mode) {
		INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		tmp_literal_length = length - (op_control_available ? BEWALGO_LENGTH_MAX : 0);
		if (op->position + (tmp_literal_length / (BEWALGO_LENGTH_MAX * 2)) + ((tmp_literal_length % (BEWALGO_LENGTH_MAX * 2)) > 0) + length > dest_end) {
			INC_COUNTER_COMPRESSOR;
			goto _error_dest_end;
		}
#else
		if (op->position + (length / BEWALGO_LENGTH_MAX) + ((length % BEWALGO_LENGTH_MAX) > 0) + length > dest_end) {
			INC_COUNTER_COMPRESSOR;
			goto _error_dest_end;
		}
#endif
	}
	while (length > BEWALGO_LENGTH_MAX) {
		INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		if (op_control_available == 0) {
			INC_COUNTER_COMPRESSOR;
			if (op->page_pointer == op->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				op->page_index++;
				op->page_pointer	 = op->page_mapped[op->page_index];
				op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			}
			*(op->page_pointer) = 0;
			op_control			= (BYTE*) (op->page_pointer);
			op->position++;
			op->page_pointer++;
		}
		op_control_available = !op_control_available;
#else
		if (op->page_pointer == op->page_pointer_end) {
			INC_COUNTER_COMPRESSOR;
			op->page_index++;
			op->page_pointer	 = op->page_mapped[op->page_index];
			op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
		}
		*(op->page_pointer) = 0;
		op_control			= (BYTE*) (op->page_pointer);
		op->position++;
		op->page_pointer++;
#endif
		*op_control = BEWALGO_LENGTH_MAX;
		op_control += 4;
		bewalgo_page_helper_copy (op, anchor, BEWALGO_LENGTH_MAX);
		length -= BEWALGO_LENGTH_MAX;
	}
	if (likely (length > 0)) {
		INC_COUNTER_COMPRESSOR;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
		if (op_control_available == 0) {
			INC_COUNTER_COMPRESSOR;
			if (op->page_pointer == op->page_pointer_end) {
				INC_COUNTER_COMPRESSOR;
				op->page_index++;
				op->page_pointer	 = op->page_mapped[op->page_index];
				op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
			}
			*(op->page_pointer) = 0;
			op_control			= (BYTE*) (op->page_pointer);
			op->position++;
			op->page_pointer++;
		}
#else
		if (op->page_pointer == op->page_pointer_end) {
			INC_COUNTER_COMPRESSOR;
			op->page_index++;
			op->page_pointer	 = op->page_mapped[op->page_index];
			op->page_pointer_end = op->page_mapped[op->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
		}
		*(op->page_pointer) = 0;
		op_control			= (BYTE*) (op->page_pointer);
		op->position++;
		op->page_pointer++;
#endif
		*op_control = length;
		bewalgo_page_helper_copy (op, anchor, length);
	}
_finish:
#ifdef LOG_STATISTICS
	for (h = 0; h < counters_size; h++) {
		if (counters[h] > 0) {
			APPEND_LOG ("%llu,%d\n", counters[h], h);
		}
	}
	close_log_file ();
#endif
	bewalgo_page_helper_free (match);
	bewalgo_page_helper_free (anchor);
	return op->position << BEWALGO_COMPRESS_DATA_TYPE_SHIFT;
_error_dest_end:
#ifdef LOG_STATISTICS
	for (h = 0; h < counters_size; h++) {
		if (counters[h] > 0) {
			APPEND_LOG ("%llu,%d\n", counters[h], h);
		}
	}
	close_log_file ();
#endif
	bewalgo_page_helper_free (match);
	bewalgo_page_helper_free (anchor);
	return -1;
}

__attribute__ ((no_instrument_function)) static int bewalgo_compress_always_inline
	BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_page_compress_fast) (void* wrkmem, void* src, void* dst, const int source_length, const int dest_length, int acceleration) {
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
	/* choose optimal configuration for best performance */
	if (final_dest_length < BEWALGO_COMPRESS_BOUND (final_source_length)) {
		if (source_length <= (BEWALGO_OFFSET_MAX << BEWALGO_COMPRESS_DATA_TYPE_SHIFT)) {
			res = bewalgo_page_compress_generic (wrkmem, &ip, &op, final_source_length, final_dest_length, acceleration, BEWALGO_SAFE, BEWALGO_NO_OFFSET_CHECK);
		} else {
			res = bewalgo_page_compress_generic (wrkmem, &ip, &op, final_source_length, final_dest_length, acceleration, BEWALGO_SAFE, BEWALGO_OFFSET_CHECK);
		}
	} else {
		if (source_length <= (BEWALGO_OFFSET_MAX << BEWALGO_COMPRESS_DATA_TYPE_SHIFT)) {
			res = bewalgo_page_compress_generic (wrkmem, &ip, &op, final_source_length, final_dest_length, acceleration, BEWALGO_UNSAFE, BEWALGO_NO_OFFSET_CHECK);
		} else {
			res = bewalgo_page_compress_generic (wrkmem, &ip, &op, final_source_length, final_dest_length, acceleration, BEWALGO_UNSAFE, BEWALGO_OFFSET_CHECK);
		}
	}
	bewalgo_page_helper_free (&ip);
	bewalgo_page_helper_free (&op);
	return res;
}
__attribute__ ((no_instrument_function)) static int
	BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_page_compress_default) (void* wrkmem, void* source, void* dest, const int source_length, const int dest_length) {
	return BEWALGO_COMPRESS_FUNCTION_NAME_MAKRO (bewalgo_page_compress_fast) (wrkmem, source, dest, source_length, dest_length, BEWALGO_ACCELERATION_DEFAULT);
}
