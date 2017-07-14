/*
 * bewalgo_parallel_worker.h
 *
 *  Created on: May 19, 2017
 *      Author: root
 */

#ifndef BEWALGO_PARALLEL_COMPRESS_WORKER_H_
#define BEWALGO_PARALLEL_COMPRESS_WORKER_H_
#include "config.h"
#include "interfaces/generic_compressor.h"
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

#define BEWALGO_PARALLEL_COMPRESS_CHUNK_SIZE_SHIFT (5 + PAGE_SHIFT)
#define BEWALGO_PARALLEL_COMPRESS_CHUNK_SIZE_DEFAULT (1 << BEWALGO_PARALLEL_COMPRESS_CHUNK_SIZE_SHIFT)
#define BEWALGO_PARALLEL_COMPRESS_CHUNK_SIZE_MASK (BEWALGO_PARALLEL_COMPRESS_CHUNK_SIZE_DEFAULT - 1)

typedef enum { BEWALGO_PARALLEL_COMPRESS = 0, BEWALGO_PARALLEL_DECOMPRESS } bewalgo_parallel_compress_worker_mode;
#define ROUND_UP_TO_NEXT_DIVIDEABLE(value, divisor_shift) ((value & (~((1 << divisor_shift) - 1))) + (((value & ((1 << divisor_shift) - 1)) > 0) << divisor_shift))
#define BEWALGO_COMPRESS_BOUND_CHUNK(isize, chunksize, compressor) \
	((isize / chunksize + ((isize % chunksize) > 1)) * ROUND_UP_TO_NEXT_DIVIDEABLE (compressor->compress_bound (chunksize), PAGE_SHIFT))

typedef struct {
	struct work_struct					  work;
	const generic_compressor*			  generic_compressor;
	bewalgo_parallel_compress_worker_mode parallel_compress_worker_mode;
	void*								  source;
	void*								  dest;
	int									  source_length;
	int									  dest_length;
	int									  acceleration;
	int									  result;
	int									  done;
} bewalgo_parallel_compress_worker_struct;

int bewalgo_parallel_compress_worker_wait_for_work (bewalgo_parallel_compress_worker_struct* jobewalgo_memory);
bewalgo_parallel_compress_worker_struct* bewalgo_parallel_compress_worker_add_work (const generic_compressor*				 generic_compressor,
																					bewalgo_parallel_compress_worker_mode	parallel_compress_worker_mode,
																					void*									 source,
																					void*									 dest,
																					int										 source_length,
																					int										 dest_length,
																					int										 acceleration,
																					bewalgo_parallel_compress_worker_struct* jobewalgo_memory);

int bewalgo_parallel_compress_fast (const generic_compressor* generic_compressor, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration);

#define BEWALGO_DEFINE_PARALLEL_COMPRESSOR(name, compressor, name_string, memory)                                                                            \
	static int name##_bound (int isize) {                                                                                                                    \
		return BEWALGO_COMPRESS_BOUND_CHUNK (isize, BEWALGO_PARALLEL_COMPRESS_CHUNK_SIZE_DEFAULT, (&compressor));                                            \
	}                                                                                                                                                        \
	static int name##_compress_fast (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration) { \
		return bewalgo_parallel_compress_fast (&compressor, source, dest, source_length, dest_length, acceleration);                                         \
	}                                                                                                                                                        \
	static int name##_compress_default (void* wrkmem, void* const source, void* const dest, const int source_length, const int dest_length) {                \
		return bewalgo_parallel_compress_fast (&compressor, source, dest, source_length, dest_length, BEWALGO_ACCELERATION_DEFAULT);                         \
	}                                                                                                                                                        \
	static int name##_decompress_save (void* const source, void* const dest, const int source_length, const int dest_length) {                               \
		return compressor.decompress_save (source, dest, source_length, dest_length);                                                                        \
	}                                                                                                                                                        \
	static int name##_decompress_fast (void* const source, void* const dest, const int source_length, const int dest_length) {                               \
		return compressor.decompress_fast (source, dest, source_length, dest_length);                                                                        \
	}                                                                                                                                                        \
	const generic_compressor name = {.compress_default				  = &name##_compress_default,                                                            \
									 .compress_fast					  = &name##_compress_fast,                                                               \
									 .decompress_save				  = &name##_decompress_save,                                                             \
									 .decompress_fast				  = &name##_decompress_fast,                                                             \
									 .generic_compressor_memory		  = memory,                                                                              \
									 .compress_bound				  = &name##_bound,                                                                       \
									 .compressor_name				  = name_string,                                                                         \
									 .compress_wrkmem				  = 0,                                                                                   \
									 .generic_compressor_concatenable = GENERIC_COMPRESSOR_CONCATABLE };                                                     \
	EXPORT_SYMBOL (name);

#endif /* BEWALGO_PARALLEL_COMPRESS_WORKER_H_ */
