/*
 * generic_compressor.h
 *
 *  Created on: July 14, 2017
 *      Author: Benjamin Warnke
 */

#ifndef GENERIC_COMPRESSOR_H_
#define GENERIC_COMPRESSOR_H_

/*
 * enum generic_compressor_memory
 * the implemented algorithms use either linear or page based buffers for input and output
 */
typedef enum { GENERIC_COMPRESSOR_LINEAR = 0, GENERIC_COMPRESSOR_PAGE } generic_compressor_memory;

/*
 * enum generic_compressor_concatenable
 * some algorithms may not support the concatenation of the binary compressed data
 */
typedef enum { GENERIC_COMPRESSOR_CONCATABLE = 0, GENERIC_COMPRESSOR_NOT_CONCATABLE } generic_compressor_concatenable;

/*
 * struct generic_compressor
 * interface for all compression algorithms for easy exchange in the test and benchmark functions
 */
typedef struct {
	int (*const compress_default) (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length);
	int (*const compress_fast) (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration);
	int (*const decompress_save) (void* const source, void* const dest, const int source_length, const int dest_length);
	int (*const decompress_fast) (void* const source, void* const dest, const int source_length, const int dest_length);
	const generic_compressor_memory generic_compressor_memory;			   /* buffers point to data or to page arrays */
	int (*const compress_bound) (int isize);							   /* worst case output size */
	const char* const					  compressor_name;				   /* string representation of the name for this compressor */
	const int							  compress_wrkmem;				   /* required memory for compression in BYTES */
	const generic_compressor_concatenable generic_compressor_concatenable; /* concatenation of of binary compressed data supported? */
} generic_compressor;

#endif /* GENERIC_COMPRESSOR_H_ */
