/*
 * generic_compress_test.h
 *
 *  Created on: July 14, 2017
 *      Author: Benjamin Warnke
 */

#ifndef GENERIC_COMPRESS_TEST_H_
#define GENERIC_COMPRESS_TEST_H_

#include "file_helper.h"
#include <asm/pgtable_types.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

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
 * the short file name for the data to use in the log file
 */
static char* input_file_name_short = NULL;
module_param (input_file_name_short, charp, 0000);
MODULE_PARM_DESC (input_file_name_short, "the short file name for the data to use in the log file");

/*
 * acceleration to use in the compressor
 */
static int acceleration = 1;
module_param (acceleration, int, 0);
MODULE_PARM_DESC (acceleration, "aacceleration to use in the compressor");

/**
 * verify_equal() - is the content of the page array and the linear buffer equal?
 * @pages: page buffer
 * @buffer: linear buffer
 * @length: the size of the buffers to compare
 */
static int verify_equal (struct page** pages, char* buffer, int length) {
	int i, j;
	int data_is_equal = 1;
	int page_count	= length >> PAGE_SHIFT;
	int counter		  = 0;
	if ((length & (PAGE_SIZE - 1)) > 0) {
		/* last page partially filled */
		page_count++;
		for (i = 0; i < page_count - 1; i++) {
			for (j = 0; j < PAGE_SIZE; j++) {
				if (i * PAGE_SIZE + j >= length)
					break;
				if (((((char*) page_address (pages[i]))[j]) & 0xFF) != ((buffer[i * PAGE_SIZE + j]) & 0xFF)) {
					printk (KERN_INFO "[file_helper]: data is different %ld: %u <- %u\n", i * PAGE_SIZE + j, ((((char*) page_address (pages[i]))[j]) & 0xFF), ((buffer[i * PAGE_SIZE + j]) & 0xFF));
					data_is_equal = 0;
					counter++;
					if (counter > 100) {
						/* print only first 100 errors */
						return 0;
					}
				}
			}
		}
		for (j = 0; j < (length & (PAGE_SIZE - 1)); j++) {
			if ((page_count - 1) * PAGE_SIZE + j >= length)
				break;
			if (((((char*) page_address (pages[i]))[j]) & 0xFF) != ((buffer[i * PAGE_SIZE + j]) & 0xFF)) {
				printk (KERN_INFO "[file_helper]: data is different %ld: %u <- %u\n",
						(page_count - 1) * PAGE_SIZE + j,
						((((char*) page_address (pages[(page_count - 1)]))[j]) & 0xFF),
						((buffer[(page_count - 1) * PAGE_SIZE + j]) & 0xFF));

				data_is_equal = 0;
				counter++;
				if (counter > 100) {
					/* print only first 100 errors */
					return 0;
				}
			}
		}
	} else {
		for (i = 0; i < page_count; i++) {
			for (j = 0; j < PAGE_SIZE; j++) {
				if (i * PAGE_SIZE + j >= length)
					break;
				if (((((char*) page_address (pages[i]))[j]) & 0xFF) != ((buffer[i * PAGE_SIZE + j]) & 0xFF)) {
					printk (KERN_INFO "[file_helper]: data is different %ld: %u <- %u\n", i * PAGE_SIZE + j, ((((char*) page_address (pages[i]))[j]) & 0xFF), ((buffer[i * PAGE_SIZE + j]) & 0xFF));
					data_is_equal = 0;
					counter++;
					if (counter > 100) {
						/* print only first 100 errors */
						return 0;
					}
				}
			}
		}
	}
	return data_is_equal;
}

/*
 * helpers for pretty unified debug output
 */
#define PRINT_UNIT_TEST_SUCCESS(func, what, success)                                         \
	do {                                                                                     \
		if (success) {                                                                       \
			printk (KERN_INFO "[file_helper]: %30s : %-50s %10s\n", #func, what, "success"); \
		} else {                                                                             \
			printk (KERN_ERR "[file_helper]: %30s : %-50s %10s\n", #func, what, "failed");   \
			res = 0;                                                                         \
		}                                                                                    \
	} while (0)
#define PRINT_UNIT_TEST_EQUAL_INT(var1, var2)                                                                                          \
	do {                                                                                                                               \
		if (var1 == var2) {                                                                                                            \
			printk (KERN_INFO "[file_helper]: %38s <-> %-38s %10s %d <-> %d\n", #var1, #var2, "equal", (int) (var1), (int) (var2));    \
		} else {                                                                                                                       \
			printk (KERN_ERR "[file_helper]: %38s <-> %-38s %10s %d <-> %d\n", #var1, #var2, "different", (int) (var1), (int) (var2)); \
			res = 0;                                                                                                                   \
		}                                                                                                                              \
	} while (0)
#define PRINT_UNIT_TEST_EQUAL_INT_GE_ZERO(var1, var2)                                                                                        \
	do {                                                                                                                                     \
		if ((var1 == var2) && (var1 >= 0)) {                                                                                                 \
			printk (KERN_INFO "[file_helper]: %38s <-> %-38s %10s %d <-> %d\n", #var1, #var2, "equal", (int) (var1), (int) (var2));          \
		} else {                                                                                                                             \
			printk (KERN_ERR "[file_helper]: %38s <-> %-38s %10s %d <-> %d\n", #var1, #var2, "different or -1", (int) (var1), (int) (var2)); \
			res = 0;                                                                                                                         \
		}                                                                                                                                    \
	} while (0)
#define PRINT_UNIT_TEST_EQUAL(var1, var2, equal)                                                 \
	do {                                                                                         \
		if (equal) {                                                                             \
			printk (KERN_INFO "[file_helper]: %38s <-> %-38s %10s\n", #var1, #var2, "equal");    \
		} else {                                                                                 \
			printk (KERN_ERR "[file_helper]: %38s <-> %-38s %10s\n", #var1, #var2, "different"); \
			res = 0;                                                                             \
		}                                                                                        \
	} while (0)
#define PRINT_UNIT_TEST_VARIABLE_INT(var)                            \
	do {                                                             \
		printk (KERN_INFO "[file_helper]: %-50s = %d\n", #var, var); \
	} while (0)
#define PRINT_UNIT_TEST_VARIABLE_STR(var)                            \
	do {                                                             \
		printk (KERN_INFO "[file_helper]: %-50s = %s\n", #var, var); \
	} while (0)
#define PRINT_UNIT_TEST_INFO_STRING(var)                  \
	do {                                                  \
		printk (KERN_INFO "[file_helper]: %-30s\n", var); \
	} while (0)
#define PRINT_UNIT_TEST_ERROR_STRING(var)                \
	do {                                                 \
		printk (KERN_ERR "[file_helper]: %-30s\n", var); \
	} while (0)
#define PRINT_UNIT_TEST_EQUAL_DATA(pages, linear, pages_length, linear_length, abort)    \
	do {                                                                                 \
		PRINT_UNIT_TEST_EQUAL_INT_GE_ZERO (pages_length, linear_length);                 \
		data_is_equal = verify_equal (pages, linear, MIN (pages_length, linear_length)); \
		res &= data_is_equal;                                                            \
		PRINT_UNIT_TEST_EQUAL (pages, linear, data_is_equal);                            \
		if ((abort) && (!data_is_equal))                                                 \
			goto finish;                                                                 \
	} while (0)

/**
 * compress_test_init() - the main test execution function
 */
static int __init compress_test_init (void) {
	/* variables for linear buffer compression */
	char* src							 = NULL;
	char* dest							 = NULL;
	int   compressed_size_linear		 = 0;
	int   compressed_size_linear_exact   = 0;
	int   compressed_size_linear_small   = 0;
	int   decompressed_size_linear		 = 0;
	int   decompressed_size_linear_small = 0;
	/* variables for page buffer compression */
	struct page** src_pages						= NULL;
	struct page** dest_pages					= NULL;
	int			  src_page_count				= 0;
	int			  dest_page_count				= 0;
	int			  compressed_size_pages			= 0;
	int			  compressed_size_pages_exact   = 0;
	int			  compressed_size_pages_small   = 0;
	int			  decompressed_size_pages		= 0;
	int			  decompressed_size_pages_small = 0;
	/* other variables */
	int   max_output_size = compressor_linear->compress_bound (input_file_length);
	char* wrkmem		  = NULL;
	int   gfp_flags		  = (GFP_KERNEL | __GFP_ZERO);
	int   src_is_vmalloc  = 0;
	int   dest_is_vmalloc = 0;
	int   res			  = 1;
	int   i				  = 0;
	int   rc			  = 0;
	int   data_is_equal   = 1;
	/* logging the test setup to track potential errors */
	PRINT_UNIT_TEST_INFO_STRING ("");
	PRINT_UNIT_TEST_INFO_STRING ("");
	PRINT_UNIT_TEST_INFO_STRING ("");
	PRINT_UNIT_TEST_INFO_STRING ("Module "__FILE__
								 " loaded");
	PRINT_UNIT_TEST_VARIABLE_STR (input_file_name);
	PRINT_UNIT_TEST_VARIABLE_INT (input_file_length);
	PRINT_UNIT_TEST_VARIABLE_STR (input_file_name_short);
	PRINT_UNIT_TEST_VARIABLE_INT (acceleration);
	PRINT_UNIT_TEST_VARIABLE_INT (max_output_size);
	PRINT_UNIT_TEST_VARIABLE_STR (compressor_linear->compressor_name);
	PRINT_UNIT_TEST_VARIABLE_STR (compressor_page->compressor_name);
	/* process and verify input parameters */
	if (input_file_name == NULL) {
		PRINT_UNIT_TEST_ERROR_STRING ("'input_file_name' must not be NULL or 0");
		return -ENOEXEC;
	}
	if (input_file_length < 1) {
		PRINT_UNIT_TEST_ERROR_STRING ("'input_file_length' must be greater than 0");
		return -ENOEXEC;
	}
	/* alloc memory */
	wrkmem = kmalloc (compressor_linear->compress_wrkmem, gfp_flags);
	if (wrkmem == NULL) {
		PRINT_UNIT_TEST_ERROR_STRING ("Could not allocate working memory");
		rc = -1;
		goto free0;
	}
	{
		src = kmalloc (input_file_length, gfp_flags);
		if (src == NULL) {
			src = vmalloc (input_file_length);
			if (src == NULL) {
				PRINT_UNIT_TEST_ERROR_STRING ("Could not allocate linear source buffer");
				rc = -1;
				goto free1;
			} else {
				src_is_vmalloc = true;
			}
		}
		max_output_size = compressor_linear->compress_bound (input_file_length);
		dest			= kmalloc (max_output_size, gfp_flags);
		if (dest == NULL) {
			dest = vmalloc (max_output_size);
			if (dest == NULL) {
				PRINT_UNIT_TEST_ERROR_STRING ("Could not allocate linear destination buffer");
				rc = -1;
				goto free2;
			} else {
				dest_is_vmalloc = true;
			}
		}
	}
	{
		src_page_count = (input_file_length >> PAGE_SHIFT) + ((input_file_length & (PAGE_SIZE - 1)) > 0 ? 1 : 0);
		src_pages	  = kmalloc (sizeof (*src_pages) * src_page_count, gfp_flags);
		if (src_pages == NULL) {
			PRINT_UNIT_TEST_ERROR_STRING ("Could not allocate source pages buffer");
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
			PRINT_UNIT_TEST_ERROR_STRING ("Could not allocate destination pages buffer");
			rc = -1;
			goto free3;
		}
		for (i = 0; i < dest_page_count; i++) {
			dest_pages[i] = alloc_page (gfp_flags);
			if (dest_pages[i] == NULL) {
				rc = -1;
				goto free5;
			}
		}
	}
	/* load data to compress */
	if (read_file (input_file_name, src, input_file_length) == -1) {
		res = 0;
		goto finish;
	}
	if (read_pages (input_file_name, src_pages, input_file_length) == -1) {
		res = 0;
		goto finish;
	}
	/* execute the tests */
	{
		int tmpres = res;
		PRINT_UNIT_TEST_INFO_STRING ("compression:");
		{
			/* linear compress */
			memset (wrkmem, 0, compressor_linear->compress_wrkmem);
			compressed_size_linear = compressor_linear->compress_fast (wrkmem, src, dest, input_file_length, max_output_size, acceleration);
		}
		{
			/* pages compress */
			memset (wrkmem, 0, compressor_page->compress_wrkmem);
			compressed_size_pages = compressor_page->compress_fast (wrkmem, src_pages, dest_pages, input_file_length, max_output_size, acceleration);
		}
		PRINT_UNIT_TEST_EQUAL_DATA (dest_pages, dest, compressed_size_pages, compressed_size_linear, 0);
		res = tmpres;
		/* compressed data can be different for page and linear based compression to improve speed
		the decompressor will show if the compressed data is ok */
	}
#ifndef SIMPLE_TEST
	/* some algorithms fail this tests by 'intention' to improve speed in other cases */
	{
		for (i = 0; i < 8; i++) {
			int size = ((compressed_size_pages < compressed_size_linear ? compressed_size_pages : compressed_size_linear) & PAGE_MASK) - (too_small_bounds_substractor << i);
			if (size > 0) {
				PRINT_UNIT_TEST_INFO_STRING ("compression too small bounds:");
				{
					/* linear compress into too small bounds */
					memset (wrkmem, 0, compressor_linear->compress_wrkmem);
					compressed_size_linear_small = compressor_linear->compress_fast (wrkmem, src, dest, input_file_length, size, acceleration);
				}
				{
					/* pages compress into too small bounds */
					memset (wrkmem, 0, compressor_page->compress_wrkmem);
					compressed_size_pages_small = compressor_page->compress_fast (wrkmem, src_pages, dest_pages, input_file_length, size, acceleration);
				}
				PRINT_UNIT_TEST_SUCCESS (compress_fast, "abort automatically", compressed_size_linear_small <= 0);
				PRINT_UNIT_TEST_SUCCESS (page_compress_fast, "abort automatically", compressed_size_pages_small <= 0);
			}
		}
	}
	{
		int tmpres = res;
		PRINT_UNIT_TEST_INFO_STRING ("compression exact bounds:");
		{
			/*  linear compress into exact bounds */
			memset (wrkmem, 0, compressor_linear->compress_wrkmem);
			compressed_size_linear_exact = compressor_linear->compress_fast (wrkmem, src, dest, input_file_length, compressed_size_linear, acceleration);
		}
		{
			/*  pages compress into exact bounds */
			memset (wrkmem, 0, compressor_page->compress_wrkmem);
			compressed_size_pages_exact = compressor_page->compress_fast (wrkmem, src_pages, dest_pages, input_file_length, compressed_size_pages, acceleration);
		}
		PRINT_UNIT_TEST_EQUAL_DATA (dest_pages, dest, compressed_size_pages_exact, compressed_size_linear_exact, 0);
		res = tmpres;
		/* compressed data can be different for page and linear based compression to improve speed
		the decompressor will show if the compressed data is ok */
	}
#endif
	{
		PRINT_UNIT_TEST_INFO_STRING ("decompression fast:");
		{
			/* linear decompress */
			decompressed_size_linear = compressor_linear->decompress_fast (dest, src, compressed_size_linear, input_file_length);
			PRINT_UNIT_TEST_EQUAL_DATA (src_pages, src, input_file_length, decompressed_size_linear, 1);
		}
		{
			/* pages decompress */
			decompressed_size_pages = compressor_page->decompress_fast (dest_pages, src_pages, compressed_size_pages, input_file_length);
			PRINT_UNIT_TEST_EQUAL_DATA (src_pages, src, decompressed_size_pages, input_file_length, 1);
		}
	}
	{
		PRINT_UNIT_TEST_INFO_STRING ("decompression save:");
		{
			/* linear decompress */
			decompressed_size_linear = compressor_linear->decompress_save (dest, src, compressed_size_linear, input_file_length);
			PRINT_UNIT_TEST_EQUAL_DATA (src_pages, src, input_file_length, decompressed_size_linear, 1);
		}
		{
			/* pages decompress */
			decompressed_size_pages = compressor_page->decompress_save (dest_pages, src_pages, compressed_size_pages, input_file_length);
			PRINT_UNIT_TEST_EQUAL_DATA (src_pages, src, decompressed_size_pages, input_file_length, 1);
		}
	}
#ifndef SIMPLE_TEST
	/*  XXX this test destroys the original data on success */
	{
		int tmpres = res;
		for (i = 0; i < 8; i++) {
			int size = (input_file_length & PAGE_MASK) - (too_small_bounds_substractor << i);
			if (size > 0) {
				PRINT_UNIT_TEST_INFO_STRING ("decompression partial:");
				{
					/* linear decompress */
					decompressed_size_linear_small = compressor_linear->decompress_save (dest, src, compressed_size_linear, size);
					PRINT_UNIT_TEST_EQUAL_DATA (src_pages, src, size, decompressed_size_linear_small, 0);
				}
				{
					/* pages decompress */
					decompressed_size_pages_small = compressor_page->decompress_save (dest_pages, src_pages, compressed_size_pages, size);
					PRINT_UNIT_TEST_EQUAL_DATA (src_pages, src, decompressed_size_pages_small, size, 0);
				}
				if ((decompressed_size_linear_small != (size)) && (decompressed_size_linear_small == decompressed_size_pages_small)) {
					/* if the decompressor (page and linear) doesn't support partial decoding, then skip this result
					 * e.g. lz4 can't handle partial decompression, if the partial size is not matching exactly the end of an encoded literal
					 * */
					res = tmpres;
				}
			}
		}
	}
	{
		for (i = 0; i < 8; i++) {
			int size = ((compressed_size_pages < compressed_size_linear ? compressed_size_pages : compressed_size_linear) & PAGE_MASK) - (too_small_bounds_substractor << i);
			if (size > 0) {
				PRINT_UNIT_TEST_INFO_STRING ("decompression too small source bounds:");
				{
					/* linear decompress */
					decompressed_size_linear_small = compressor_linear->decompress_save (dest, src, size, input_file_length);
				}
				{
					/* pages decompress */
					decompressed_size_pages_small = compressor_page->decompress_save (dest_pages, src_pages, size, input_file_length);
				}
				PRINT_UNIT_TEST_SUCCESS (decompress_save, "abort automatically", decompressed_size_linear_small <= 0);
				PRINT_UNIT_TEST_SUCCESS (page_decompress_save, "abort automatically", decompressed_size_pages_small <= 0);
			}
		}
	}
#endif
	{
		/* logging the test summary to track potential errors */
		int compress_bound = max_output_size;
		PRINT_UNIT_TEST_VARIABLE_INT (input_file_length);
		PRINT_UNIT_TEST_VARIABLE_INT (decompressed_size_linear);
		PRINT_UNIT_TEST_VARIABLE_INT (decompressed_size_pages);
		PRINT_UNIT_TEST_VARIABLE_INT (compress_bound);
		PRINT_UNIT_TEST_VARIABLE_INT (compressed_size_linear);
		PRINT_UNIT_TEST_VARIABLE_INT (compressed_size_pages);
	}
	{
		/* used by decompression benchmark tests */
		write_file ("/tmp/data/data.compressed", dest, compressed_size_linear);
	}
/* free memory */
finish:
	i = dest_page_count;
free5:
	for (i = i - 1; i >= 0; i--) {
		__free_page (dest_pages[i]);
	}
	i = src_page_count;
free4:
	for (i = i - 1; i >= 0; i--) {
		__free_page (src_pages[i]);
	}
	kfree (src_pages);
free3:
	if (dest_is_vmalloc)
		vfree (dest);
	else
		kfree (dest);
free2:
	if (src_is_vmalloc)
		vfree (src);
	else
		kfree (src);
free1:
	kfree (wrkmem);
free0:
	if (rc == -1)
		return -ENOMEM;
	return -(!res);
}

/**
 * compress_test_exit() - does nothing, just for completeness
 */
static void __exit compress_test_exit (void) {
	printk (KERN_INFO "[file_helper]: Module "__FILE__
					  " removed\n");
}

MODULE_LICENSE ("Dual BSD/GPL");
MODULE_AUTHOR ("Benjamin Warnke");
MODULE_DESCRIPTION ("Test Module for the correctness of compression algorithms");
MODULE_VERSION ("1.0.0");

module_init (compress_test_init);
module_exit (compress_test_exit);

#endif /* GENERIC_COMPRESS_TEST_H_ */
