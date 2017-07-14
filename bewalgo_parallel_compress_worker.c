/*
 * bewalgo_parallel_compress_worker.c
 *
 *  Created on: May 19, 2017
 *      Author: benjamin
 */

#include "bewalgo_parallel_compress_worker.h"
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/slab.h>

#define BEWALGO_COMPRESS_DATA_TYPE BYTE
#define BEWALGO_COMPRESS_DATA_TYPE_SHIFT 0
#include "compression-bewalgo/bewalgo_page_helper.h"
#undef BEWALGO_COMPRESS_DATA_TYPE_SHIFT
#undef BEWALGO_COMPRESS_DATA_TYPE

MODULE_LICENSE ("Dual BSD/GPL");
MODULE_AUTHOR ("Benjamin Warnke");
MODULE_DESCRIPTION ("decompression module");
MODULE_VERSION ("1.0.0");

#define length_to_pages(length) ((((length) >> PAGE_SHIFT) + (((length) &PAGE_MASK) > 0)) * sizeof (struct page*))

static struct workqueue_struct* bewalgo_parallel_compress_worker_queue;
static void*					bewalgo_parallel_compress_worker_memory_internal[BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT]; /*allocated on first use, increases size if necessary*/
static int						bewalgo_parallel_compress_worker_memory_internal_size[BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT];
static int						bewalgo_parallel_compress_worker_memory_is_use[BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT];
static spinlock_t				bewalgo_parallel_compress_worker_memory_is_use_lock;

int bewalgo_parallel_compress_fast (const generic_compressor* generic_compressor, void* const source, void* const dest, const int source_length, const int dest_length, int acceleration) {
	int										 chunkcount_full	 = source_length >> BEWALGO_PARALLEL_COMPRESS_CHUNK_SIZE_SHIFT;
	int										 chunkcount_partial  = (source_length & BEWALGO_PARALLEL_COMPRESS_CHUNK_SIZE_MASK) > 0;
	int										 chunkcount_total	= chunkcount_full + chunkcount_partial;
	bewalgo_parallel_compress_worker_struct* jobewalgo_memory	= kmalloc (chunkcount_total * sizeof (*jobewalgo_memory), GFP_KERNEL);
	int										 source_chunk_size   = BEWALGO_PARALLEL_COMPRESS_CHUNK_SIZE_DEFAULT;
	int										 dest_chunk_size_raw = (dest_length / chunkcount_total);
	int										 dest_chunk_size	 = (dest_chunk_size_raw & PAGE_MASK) + (((dest_chunk_size_raw & ~PAGE_MASK) > 0) << PAGE_SHIFT);
	int										 res;
	int										 source_offset;
	int										 dest_offset;
	int										 i;
	int										 compressed_size;
	bewalgo_page_helper						 ip;
	int										 dest_position;
	if (jobewalgo_memory == NULL)
		return -ENOMEM;
	memset (jobewalgo_memory, 0, chunkcount_total * sizeof (*jobewalgo_memory));
	if (generic_compressor->generic_compressor_memory == GENERIC_COMPRESSOR_PAGE) {
		source_offset = (source_chunk_size >> PAGE_SHIFT) * sizeof (struct page*);
		dest_offset   = (dest_chunk_size >> PAGE_SHIFT) * sizeof (struct page*);
	} else {
		source_offset = source_chunk_size;
		dest_offset   = dest_chunk_size;
	}
	for (i = 0; i < chunkcount_full; i++) {
		bewalgo_parallel_compress_worker_add_work (
			generic_compressor, BEWALGO_PARALLEL_COMPRESS, ((char*) source) + i * source_offset, ((char*) dest) + i * dest_offset, source_chunk_size, dest_chunk_size, acceleration, jobewalgo_memory + i);
	}
	if (chunkcount_partial) {
		bewalgo_parallel_compress_worker_add_work (generic_compressor,
												   BEWALGO_PARALLEL_COMPRESS,
												   ((char*) source) + chunkcount_full * source_offset,
												   ((char*) dest) + chunkcount_full * dest_offset,
												   source_length - i * source_chunk_size,
												   dest_length - i * dest_chunk_size,
												   acceleration,
												   jobewalgo_memory + chunkcount_full);
	}
	compressed_size = bewalgo_parallel_compress_worker_wait_for_work (jobewalgo_memory);
	if (unlikely (compressed_size < 0))
		return compressed_size;
	if (generic_compressor->generic_compressor_memory == GENERIC_COMPRESSOR_PAGE) {
		bewalgo_page_helper op;
		res = bewalgo_page_helper_init (&ip, dest, dest_length);
		if (res < 0)
			return res;
		res = compressed_size;
		bewalgo_page_helper_clone (&op, &ip);
		bewalgo_page_helper_set_position (&op, compressed_size);
		for (i = 1; i < chunkcount_total; i++) {
			compressed_size = bewalgo_parallel_compress_worker_wait_for_work (jobewalgo_memory + i);
			if (unlikely (compressed_size < 0)) {
				bewalgo_page_helper_free (&ip);
				bewalgo_page_helper_free (&op);
				return compressed_size;
			}
			if (i * dest_offset == res) {
				bewalgo_page_helper_set_position (&op, res + compressed_size);
			} else {
				bewalgo_page_helper_set_position (&ip, i * dest_chunk_size);
				bewalgo_page_helper_copy_long (&op, &ip, compressed_size);
			}
			res += compressed_size;
		}
		bewalgo_page_helper_free (&ip);
		bewalgo_page_helper_free (&op);
	} else {
		char* op;
		op  = dest + compressed_size;
		res = compressed_size;
		for (i = 1; i < chunkcount_total; i++) {
			compressed_size = bewalgo_parallel_compress_worker_wait_for_work (jobewalgo_memory + i);
			if (unlikely (compressed_size < 0))
				return compressed_size;
			if (likely (op != ((char*) dest) + i * dest_offset))
				memmove (op, ((char*) dest) + i * dest_offset, compressed_size);
			op += compressed_size;
			res += compressed_size;
		}
	}
	kfree (jobewalgo_memory);
	return res;
}
EXPORT_SYMBOL (bewalgo_parallel_compress_fast);

void bewalgo_parallel_compress_worker_function (struct work_struct* work) {
	bewalgo_parallel_compress_worker_struct* data;
	data = container_of (work, bewalgo_parallel_compress_worker_struct, work);
	if (data->parallel_compress_worker_mode == BEWALGO_PARALLEL_COMPRESS) {
		int i;
		{
			spin_lock (&bewalgo_parallel_compress_worker_memory_is_use_lock);
			for (i = 0; i < BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT; i++) {
				if (bewalgo_parallel_compress_worker_memory_is_use[i] == 0) {
					bewalgo_parallel_compress_worker_memory_is_use[i] = 1;
					break;
				}
			}
			spin_unlock (&bewalgo_parallel_compress_worker_memory_is_use_lock);
			// i is the index of the wrkmem to use. i is always vaild since the thread count is the same as the number of work memory pointers
		}
		if (unlikely (bewalgo_parallel_compress_worker_memory_internal_size[i] < data->generic_compressor->compress_wrkmem)) {
			if (bewalgo_parallel_compress_worker_memory_internal_size[i] != 0) {
				kfree (bewalgo_parallel_compress_worker_memory_internal[i]);
			}
			bewalgo_parallel_compress_worker_memory_internal_size[i] = data->generic_compressor->compress_wrkmem;
			bewalgo_parallel_compress_worker_memory_internal[i]		 = kmalloc (bewalgo_parallel_compress_worker_memory_internal_size[i], GFP_KERNEL);
			if (unlikely (bewalgo_parallel_compress_worker_memory_internal[i] == NULL)) {
				bewalgo_parallel_compress_worker_memory_internal_size[i] = 0;
				data->result											 = -1;
				data->done												 = 1;
				return;
			}
		}
		memset (bewalgo_parallel_compress_worker_memory_internal[i], 0, data->generic_compressor->compress_wrkmem);
		data->result =
			data->generic_compressor->compress_fast (bewalgo_parallel_compress_worker_memory_internal[i], data->source, data->dest, data->source_length, data->dest_length, data->acceleration);
		{
			spin_lock (&bewalgo_parallel_compress_worker_memory_is_use_lock);
			bewalgo_parallel_compress_worker_memory_is_use[i] = 0;
			spin_unlock (&bewalgo_parallel_compress_worker_memory_is_use_lock);
		}
	} else {
		data->result = data->generic_compressor->decompress_fast (data->source, data->dest, data->source_length, data->dest_length);
	}
	data->done = 1;
}

static int __init bewalgo_parallel_compress_worker_init (void) {
	int i;
	for (i = 0; i < BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT; i++) {
		bewalgo_parallel_compress_worker_memory_is_use[i]		 = 0;
		bewalgo_parallel_compress_worker_memory_internal_size[i] = 0;
	}
	printk ("BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT %d\n", BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT);
	bewalgo_parallel_compress_worker_queue = alloc_workqueue ("BEWALGO_PARALLEL_COMPRESS_WORKER_QUEUE", WQ_UNBOUND, BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT);
	spin_lock_init (&bewalgo_parallel_compress_worker_memory_is_use_lock);
	return 0;
}

static void __exit bewalgo_parallel_compress_worker_exit (void) {
	int i;
	flush_workqueue (bewalgo_parallel_compress_worker_queue);
	destroy_workqueue (bewalgo_parallel_compress_worker_queue);
	for (i = 0; i < BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT; i++) {
		kfree (bewalgo_parallel_compress_worker_memory_internal[i]);
	}
}

bewalgo_parallel_compress_worker_struct* bewalgo_parallel_compress_worker_add_work (const generic_compressor*				 generic_compressor,
																					bewalgo_parallel_compress_worker_mode	parallel_compress_worker_mode,
																					void*									 source,
																					void*									 dest,
																					int										 source_length,
																					int										 dest_length,
																					int										 acceleration,
																					bewalgo_parallel_compress_worker_struct* jobewalgo_memory) {
	if (jobewalgo_memory == NULL) {
		jobewalgo_memory = kmalloc (sizeof (bewalgo_parallel_compress_worker_struct), GFP_KERNEL);
		memset (jobewalgo_memory, 0, sizeof (bewalgo_parallel_compress_worker_struct));
		// printk (__FILE__ " : %d -> %s %d %p %p %d %d %d\n", __LINE__, generic_compressor->compressor_name, parallel_compress_worker_mode, source, dest, source_length,
		// dest_length, acceleration);msleep (100);
		if (jobewalgo_memory == NULL)
			return NULL;
	}
	INIT_WORK ((&jobewalgo_memory->work), bewalgo_parallel_compress_worker_function);
	jobewalgo_memory->generic_compressor			= generic_compressor;
	jobewalgo_memory->parallel_compress_worker_mode = parallel_compress_worker_mode;
	jobewalgo_memory->source						= source;
	jobewalgo_memory->dest							= dest;
	jobewalgo_memory->source_length					= source_length;
	jobewalgo_memory->dest_length					= dest_length;
	jobewalgo_memory->acceleration					= acceleration;
	jobewalgo_memory->done							= 0;
	queue_work (bewalgo_parallel_compress_worker_queue, &jobewalgo_memory->work);
	return jobewalgo_memory;
}
EXPORT_SYMBOL (bewalgo_parallel_compress_worker_add_work);

int bewalgo_parallel_compress_worker_wait_for_work (bewalgo_parallel_compress_worker_struct* jobewalgo_memory) {
	if (jobewalgo_memory == NULL)
		return -ENODATA;
	while (!jobewalgo_memory->done) {
		schedule_timeout (1000);
	}
	return jobewalgo_memory->result;
}
EXPORT_SYMBOL (bewalgo_parallel_compress_worker_wait_for_work);

module_init (bewalgo_parallel_compress_worker_init);
module_exit (bewalgo_parallel_compress_worker_exit);
