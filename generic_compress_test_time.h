/*
 * generic_compress_test.h
 *
 *  Created on: July 14, 2017
 *      Author: Benjamin Warnke
 */

#ifndef GENERIC_COMPRESS_TEST_TIME_H_
#define GENERIC_COMPRESS_TEST_TIME_H_

#include "file_helper.h"
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/vmalloc.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

/* there are different setups to benchmark */
#define TIME_MODE_LINEAR (1)
#define TIME_MODE_PAGES (2)
#define TIME_MODE_LINEAR_BUFFER (16)
#define TIME_MODE_PAGES_BUFFER (32)
#define TIME_MODE_ALL (TIME_MODE_LINEAR | TIME_MODE_PAGES)

#define MODE_COMPRESS (1)
#define MODE_DECOMPRESS (2)

/*
 * the file name for the data to compress
 */
static char* input_file_name = NULL;
module_param (input_file_name, charp, 0000);
MODULE_PARM_DESC (input_file_name, "the file name for the data to compress");

/*
 * amount of data to read from the input file
 */
static int input_file_length = 0;
module_param (input_file_length, int, 0);
MODULE_PARM_DESC (input_file_length, "amount of data to read from the input file");

/*
 * the output size limit for compression
 */
static int output_file_length = 0;
module_param (output_file_length, int, 0);
MODULE_PARM_DESC (output_file_length, "the output size limit for compression");

/*
 * target runtmie cor benchmarks
 */
static int time_multiplicator = 10000;
module_param (time_multiplicator, int, 0);
MODULE_PARM_DESC (time_multiplicator, "target runtmie cor benchmarks");

/*
 * buffer type linear or page
 */
static int time_mode = TIME_MODE_LINEAR;
module_param (time_mode, int, 0);
MODULE_PARM_DESC (time_mode, "buffer type linear or page");

/*
 * compress or decompress
 */
static int compress_mode = MODE_COMPRESS;
module_param (compress_mode, int, 0);
MODULE_PARM_DESC (compress_mode, "compress or decompress");

/*
 * acceleration to use in the compressor
 */
static int acceleration = 1;
module_param (acceleration, int, 0);
MODULE_PARM_DESC (acceleration, "acceleration to use in the compressor");

/*
 * helper macros for sarting and stopping time measuerements
 */
#define START_TIME_measurement(ts_start) getnstimeofday (&ts_start);
#define STOP_TIME_measurement(ts_start, ts_end, msg, time_measurement_data_to_store)                                                                                \
	do {                                                                                                                                                            \
		getnstimeofday (&ts_end);                                                                                                                                   \
		if (ts_end.tv_nsec - ts_start.tv_nsec < 0) {                                                                                                                \
			time_measurement_data_to_store.tv_sec  = ts_end.tv_sec - ts_start.tv_sec - 1;                                                                           \
			time_measurement_data_to_store.tv_nsec = ts_end.tv_nsec - ts_start.tv_nsec + 1000000000;                                                                \
		} else {                                                                                                                                                    \
			time_measurement_data_to_store.tv_sec  = ts_end.tv_sec - ts_start.tv_sec;                                                                               \
			time_measurement_data_to_store.tv_nsec = ts_end.tv_nsec - ts_start.tv_nsec;                                                                             \
		}                                                                                                                                                           \
		printk ("%s time = %lld:%09lld\n", msg, (long long int) (time_measurement_data_to_store.tv_sec), (long long int) (time_measurement_data_to_store.tv_nsec)); \
		getnstimeofday (&ts_start);                                                                                                                                 \
	} while (0)

/**
 * compress_benchmark_init() - the main benchmark execution function
 */
static int __init compress_benchmark_init (void) {
	/* variables for linear buffer compression */
	int   max_output_size = 0;
	char* wrkmem		  = NULL;
	char* src			  = NULL;
	char* dest			  = NULL;
	/* variables for page buffer compression */
	struct page** src_pages		  = NULL;
	struct page** dest_pages	  = NULL;
	int			  src_page_count  = 0;
	int			  dest_page_count = 0;
	/* variables for time measurements */
	struct timespec		  ts_start;
	struct timespec		  ts_end;
	struct timespec		  ts_target;
	time_measurement_data time_measurement_data;
	/* other variables */
	int						  i = 0, j = 0;
	int						  rc			  = 0;
	int						  gfp_flags		  = (GFP_KERNEL | __GFP_ZERO);
	int						  src_is_vmalloc  = 0;
	int						  dest_is_vmalloc = 0;
	const generic_compressor* compressor	  = compressor_linear;
	printk (KERN_INFO "Module "__FILE__
					  " loaded");
	/* init benchmark data */
	memset (&time_measurement_data, 0, sizeof (time_measurement_data));
	time_measurement_data.input_file_name	= input_file_name;
	time_measurement_data.korrect_result	 = 0;
	time_measurement_data.time_multiplicator = time_multiplicator;
	time_measurement_data.mode				 = compress_mode;
	time_measurement_data.timemode			 = time_mode;
	time_measurement_data.acceleration		 = acceleration;
	/* process and verify input parameters */
	if ((time_mode & ~TIME_MODE_ALL)) {
		printk (KERN_INFO "[file_helper]: 'time_mode' invalid\n");
		return -ENOEXEC;
	}
	if (time_mode & TIME_MODE_PAGES) {
		time_mode  = time_mode | TIME_MODE_PAGES_BUFFER;
		compressor = compressor_page;
	}
	if (time_mode & TIME_MODE_LINEAR) {
		time_mode  = time_mode | TIME_MODE_LINEAR_BUFFER;
		compressor = compressor_linear;
	}
	time_measurement_data.compressor_name = compressor->compressor_name;
	if (((compress_mode | MODE_COMPRESS | MODE_DECOMPRESS) != (MODE_COMPRESS | MODE_DECOMPRESS)) ||
		((((compress_mode & MODE_COMPRESS) == MODE_COMPRESS) + ((compress_mode & MODE_DECOMPRESS) == MODE_DECOMPRESS)) != 1)) {
		printk (KERN_INFO "[file_helper]: 'compress_mode' invalid\n");
		return -ENOEXEC;
	}
	if (input_file_name == NULL) {
		printk (KERN_INFO "[file_helper]: 'input_file_name' must not be NULL\n");
		return -ENOEXEC;
	}
	if (input_file_length < 1) {
		printk (KERN_INFO "[file_helper]: 'input_file_length' must be greater than 0\n");
		return -ENOEXEC;
	}
	if ((compress_mode & MODE_DECOMPRESS) == MODE_DECOMPRESS) {
		input_file_name = "/tmp/data/data.compressed";
	}
	/* alloc memory */
	wrkmem = kmalloc (compressor->compress_wrkmem, gfp_flags);
	if (wrkmem == NULL) {
		printk (KERN_ERR "[file_helper]: Could not allocate working memory\n");
		rc = -1;
		goto free0;
	}
	if ((compress_mode & MODE_COMPRESS) == MODE_COMPRESS) {
		max_output_size = compressor->compress_bound (input_file_length);
		if (output_file_length > max_output_size)
			max_output_size = output_file_length;
	} else {
		max_output_size = output_file_length;
	}
	if (time_mode & TIME_MODE_LINEAR_BUFFER) {
		src = kmalloc (input_file_length, gfp_flags);
		if (src == NULL) {
			src = vmalloc (input_file_length);
			if (src == NULL) {
				printk (KERN_ERR "[file_helper]: Could not allocate linear source buffer\n");
				rc = -1;
				goto free1;
			} else {
				src_is_vmalloc = true;
			}
		}
		dest = kmalloc (max_output_size, gfp_flags);
		if (dest == NULL) {
			dest = vmalloc (max_output_size);
			if (dest == NULL) {
				printk (KERN_ERR "Could not allocate linear destination buffer\n");
				rc = -1;
				goto free2;
			} else {
				dest_is_vmalloc = true;
			}
		}
		/* load data to compress */
		if (read_file (input_file_name, src, input_file_length) == -1) {
			goto _finish;
		}
	}
	if (time_mode & TIME_MODE_PAGES_BUFFER) {
		src_page_count = (input_file_length >> PAGE_SHIFT) + ((input_file_length & (PAGE_SIZE - 1)) > 0 ? 1 : 0);
		src_pages	  = kmalloc (sizeof (*src_pages) * src_page_count, gfp_flags);
		if (src_pages == NULL) {
			printk (KERN_ERR "[file_helper]: Could not allocate source pages buffer\n");
			rc = -1;
			goto free3;
		}
		for (i = 0; i < src_page_count; i++) {
			src_pages[i] = alloc_page (gfp_flags);
			if (src_pages[i] == NULL) {
				rc = -1;
				goto free4;
			}
		}
		dest_page_count = (max_output_size >> PAGE_SHIFT) + ((max_output_size & (PAGE_SIZE - 1)) > 0 ? 1 : 0);
		dest_pages		= kmalloc (sizeof (*dest_pages) * dest_page_count, gfp_flags);
		if (dest_pages == NULL) {
			printk (KERN_ERR "[file_helper]: Could not allocate destination pages buffer\n");
			rc = -1;
			goto free4;
		}
		for (i = 0; i < dest_page_count; i++) {
			dest_pages[i] = alloc_page (gfp_flags);
			if (dest_pages[i] == NULL) {
				rc = -1;
				goto free5;
			}
		}
		/* load data to compress */
		if (read_pages (input_file_name, src_pages, input_file_length) == -1) {
			goto _finish;
		}
	}
	/* target date time when tests whould be finished */
	getnstimeofday (&ts_target);
	ts_target.tv_sec += time_multiplicator;
	/* execute the tests */
	START_TIME_measurement (ts_start);
	if (compress_mode & MODE_COMPRESS) {
		if (time_mode & TIME_MODE_PAGES) {
			do {
				time_measurement_data.time_multiplicator++;
				memset (wrkmem, 0, compressor->compress_wrkmem);
				compressor->compress_fast (wrkmem, src_pages, dest_pages, input_file_length, max_output_size, acceleration);
				getnstimeofday (&ts_end);
			} while (ts_target.tv_sec > ts_end.tv_sec);
		} else if (time_mode & TIME_MODE_LINEAR) {
			do {
				time_measurement_data.time_multiplicator++;
				memset (wrkmem, 0, compressor->compress_wrkmem);
				compressor->compress_fast (wrkmem, src, dest, input_file_length, max_output_size, acceleration);
				getnstimeofday (&ts_end);
			} while (ts_target.tv_sec > ts_end.tv_sec);
		}
	} else if (compress_mode & MODE_DECOMPRESS) {
		if (time_mode & TIME_MODE_PAGES) {
			do {
				time_measurement_data.time_multiplicator++;
				compressor->decompress_fast (src_pages, dest_pages, input_file_length, max_output_size);
				getnstimeofday (&ts_end);
			} while (ts_target.tv_sec > ts_end.tv_sec);
		} else if (time_mode & TIME_MODE_LINEAR) {
			do {
				time_measurement_data.time_multiplicator++;
				compressor->decompress_fast (src, dest, input_file_length, max_output_size);
				getnstimeofday (&ts_end);
			} while (ts_target.tv_sec > ts_end.tv_sec);
		}
	}
	if (compress_mode & MODE_COMPRESS)
		STOP_TIME_measurement (ts_start, ts_end, "[file_helper]: compress b      xxxxx", time_measurement_data.compress_time);
	else
		STOP_TIME_measurement (ts_start, ts_end, "[file_helper]: decompress b    xxxxx", time_measurement_data.compress_time);
/* free memory */
_finish:
	i = dest_page_count;
free5:
	if (time_mode & TIME_MODE_PAGES_BUFFER) {
		for (i = i - 1; i >= 0; i--) {
			__free_page (dest_pages[i]);
		}
		kfree (dest_pages);
	}
	i = src_page_count;
free4:
	if (time_mode & TIME_MODE_PAGES_BUFFER) {
		for (i = i - 1; i >= 0; i--) {
			__free_page (src_pages[i]);
		}
		kfree (src_pages);
	}
free3:
	if (time_mode & TIME_MODE_LINEAR) {
		if (dest_is_vmalloc)
			vfree (dest);
		else
			kfree (dest);
	}
free2:
	if (time_mode & TIME_MODE_LINEAR) {
		if (src_is_vmalloc)
			vfree (src);
		else
			kfree (src);
	}
free1:
	kfree (wrkmem);
free0:
	if (rc == -1)
		return -ENOMEM;
	/* append benchmark results to file */
	time_measurement_data.compressedlength   = compress_mode == MODE_COMPRESS ? output_file_length : input_file_length;
	time_measurement_data.decompressedlength = compress_mode == MODE_COMPRESS ? input_file_length : output_file_length;
	if (time_multiplicator > 0)
		append_time_measurement_to_file ("/src/time_statistics.csv", &time_measurement_data);
	return 0;
}

/**
 * compress_benchmark_exit() - does nothing, just for completeness
 */
static void __exit compress_benchmark_exit (void) {
	printk (KERN_INFO "[file_helper]: Module "__FILE__
					  " removed\n");
}

MODULE_LICENSE ("Dual BSD/GPL");
MODULE_AUTHOR ("Benjamin Warnke");
MODULE_DESCRIPTION ("Test Module for the correctness of compression algorithms");
MODULE_VERSION ("1.0.0");

module_init (compress_benchmark_init);
module_exit (compress_benchmark_exit);

#endif /*GENERIC_COMPRESS_TEST_TIME_H_*/
