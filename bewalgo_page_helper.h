/*
 * bewalgo_page_helper.h
 *
 *  Created on: July 14, 2017
 *      Author: Benjamin Warnke
 */

#ifndef BEWALGO_PAGE_HELPER_H_
#define BEWALGO_PAGE_HELPER_H_

#include <asm/unaligned.h>
#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h> /* memset, memcpy */
#include <linux/types.h>

typedef uint8_t  BYTE;
typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t  S32;
typedef uint64_t U64;

#define BEWALGO_CACHE_GFP_FLAGS (__GFP_IO | __GFP_NOWARN | __GFP_ZERO | __GFP_NORETRY | __GFP_NOMEMALLOC | __GFP_NOTRACK)
#define BEWALGO_COMPRESS_PAGE_SHIFT (PAGE_SHIFT - BEWALGO_COMPRESS_DATA_TYPE_SHIFT)
#define BEWALGO_COMPRESS_PAGE_SIZE (1 << BEWALGO_COMPRESS_PAGE_SHIFT)
#define BEWALGO_COMPRESS_PAGE_OFFSET_MASK (BEWALGO_COMPRESS_PAGE_SIZE - 1)

/*
 * struct bewalgo_page_helper
 * interface for efficiently read and write data to and from page arrays
 */
typedef struct {
	BEWALGO_COMPRESS_DATA_TYPE*  page_pointer;	 /* current position in the pages */
	BEWALGO_COMPRESS_DATA_TYPE*  page_pointer_end; /* points after the end of the current page */
	BEWALGO_COMPRESS_DATA_TYPE** page_mapped;	  /* array of pointers to each of the mapped pages */
	struct page**				 pages;			   /* array of pages which contains the data */
	int							 page_index;	   /* curent page index */
	size_t						 page_count;	   /* length of the page array */
	int							 position;		   /* absolute position in the data (multiple of size_of(BEWALGO_COMPRESS_DATA_TYPE)) */
} bewalgo_page_helper __attribute__ ((aligned (sizeof (BEWALGO_COMPRESS_DATA_TYPE*))));

#ifdef LOG_STATISTICS
/*
 * log compressor statistics to file:
 * - which code is executed how ofthen?
 * - are the unit tests covering the whole code?
 */
#ifndef counters_size
#define counters_size 0
#endif /* counters_size */
static U64 counters[counters_size];
#define INC_COUNTER_PAGE_HELPER counters[1000 + __LINE__]++
#define INC_COUNTER_COMPRESSOR counters[__LINE__]++

#else /* !LOG_STATISTICS */
/* define empty macros to do not breake the code */
#define INC_COUNTER_PAGE_HELPER ;
#define INC_COUNTER_COMPRESSOR
#endif /* !LOG_STATISTICS */

#ifndef bewalgo_compress_always_inline
#define bewalgo_compress_always_inline __always_inline
#endif /* bewalgo_compress_always_inline */

/* local helper functions -->> */
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
#define bewalgo_page_helper_copy_helper_src(dest, src, target) \
	do {                                                       \
		while ((src) < (target) -1) {                          \
			INC_COUNTER_PAGE_HELPER;                           \
			(dest)[0] = (src)[0];                              \
			(dest)[1] = (src)[1];                              \
			(dest) += 2;                                       \
			(src) += 2;                                        \
		}                                                      \
		if ((src) < (target)) {                                \
			INC_COUNTER_PAGE_HELPER;                           \
			*(dest)++ = *(src)++;                              \
		}                                                      \
	} while (0)
#define bewalgo_page_helper_copy_helper_dest(dest, src, target) \
	do {                                                        \
		while ((dest) < (target) -1) {                          \
			INC_COUNTER_PAGE_HELPER;                            \
			(dest)[0] = (src)[0];                               \
			(dest)[1] = (src)[1];                               \
			(dest) += 2;                                        \
			(src) += 2;                                         \
		}                                                       \
		if ((dest) < (target)) {                                \
			INC_COUNTER_PAGE_HELPER;                            \
			*(dest)++ = *(src)++;                               \
		}                                                       \
	} while (0)
#else /* BEWALGO_COMPRESS_DATA_TYPE_SHIFT != 3 */
#define bewalgo_page_helper_copy_helper_src(dest, src, target) \
	do {                                                       \
		while ((src) < (target) -1) {                          \
			INC_COUNTER_PAGE_HELPER;                           \
			((U64*) (dest))[0] = ((U64*) (src))[0];            \
			(dest) += 2;                                       \
			(src) += 2;                                        \
		}                                                      \
		if ((src) < (target)) {                                \
			INC_COUNTER_PAGE_HELPER;                           \
			*(dest)++ = *(src)++;                              \
		}                                                      \
	} while (0)
#define bewalgo_page_helper_copy_helper_dest(dest, src, target) \
	do {                                                        \
		while ((dest) < (target) -1) {                          \
			INC_COUNTER_PAGE_HELPER;                            \
			((U64*) (dest))[0] = ((U64*) (src))[0];             \
			(dest) += 2;                                        \
			(src) += 2;                                         \
		}                                                       \
		if ((dest) < (target)) {                                \
			INC_COUNTER_PAGE_HELPER;                            \
			*(dest)++ = *(src)++;                               \
		}                                                       \
	} while (0)
#endif /* BEWALGO_COMPRESS_DATA_TYPE_SHIFT != 3 */
#define bewalgo_page_helper_set_helper(ptr, target, value) \
	do {                                                   \
		while ((ptr) < (target) -1) {                      \
			INC_COUNTER_PAGE_HELPER;                       \
			*(ptr)++ = (value);                            \
			*(ptr)++ = (value);                            \
		}                                                  \
		if ((ptr) < (target)) {                            \
			INC_COUNTER_PAGE_HELPER;                       \
			*(ptr)++ = (value);                            \
		}                                                  \
	} while (0)
/* <<-- local helper functions */

/**
 * bewalgo_page_helper_init() - map all pages and initialize the helper structure
 * @page_helper: the structure to initialize
 * @pages: the raw pages which are going to be accesed by the helper
 * @length: the number of BYTES represented by the pages
 */
static bewalgo_compress_always_inline int bewalgo_page_helper_init (bewalgo_page_helper* page_helper, struct page** pages, size_t length) {
	int i;
	int consecutive_mapped = 1;
	INC_COUNTER_PAGE_HELPER;
	page_helper->position		  = 0;
	page_helper->page_count		  = (length >> PAGE_SHIFT) + ((length & (~PAGE_MASK)) > 0);
	page_helper->pages			  = pages;
	page_helper->page_index		  = 0;
	page_helper->page_mapped	  = kmalloc ((page_helper->page_count + 1) * sizeof (*page_helper->page_mapped), BEWALGO_CACHE_GFP_FLAGS);
	page_helper->page_pointer	 = NULL;
	page_helper->page_pointer_end = NULL;
	if (page_helper->page_mapped == NULL) {
		INC_COUNTER_PAGE_HELPER;
		return -ENOMEM;
	}
	{
		page_helper->page_mapped[0] = __builtin_assume_aligned (kmap (page_helper->pages[0]), sizeof (BEWALGO_COMPRESS_DATA_TYPE));
		if (page_helper->page_mapped[0] == NULL) {
			INC_COUNTER_PAGE_HELPER;
			return -ENOMEM;
		}
		for (i = 1; i < page_helper->page_count; i++) {
			INC_COUNTER_PAGE_HELPER;
			page_helper->page_mapped[i] = __builtin_assume_aligned (kmap (page_helper->pages[i]), sizeof (BEWALGO_COMPRESS_DATA_TYPE));
			if (page_helper->page_mapped[i] == NULL) {
				INC_COUNTER_PAGE_HELPER;
				for (i--; i >= 0; i--) {
					INC_COUNTER_PAGE_HELPER;
					kunmap (page_helper->pages[i]);
				}
				return -ENOMEM;
			}
			consecutive_mapped = consecutive_mapped & ((page_helper->page_mapped[i - 1] + BEWALGO_COMPRESS_PAGE_SIZE) == page_helper->page_mapped[i]);
		}
	}
	page_helper->page_mapped[page_helper->page_count] = page_helper->page_mapped[page_helper->page_count - 1]; // overrun protection
	page_helper->page_pointer						  = page_helper->page_mapped[0];
	page_helper->page_pointer_end					  = page_helper->page_mapped[0] + BEWALGO_COMPRESS_PAGE_SIZE;
	return consecutive_mapped;
}

/**
 * bewalgo_page_helper_free() - unmap all pages and free the helper structure
 * @page_helper: the structure to free
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_free (bewalgo_page_helper* page_helper) {
	int i;
	INC_COUNTER_PAGE_HELPER;
	if (page_helper->pages != NULL) {
		INC_COUNTER_PAGE_HELPER;
		/*don't double-free cloned pointers*/
		for (i = 0; i < page_helper->page_count; i++) {
			INC_COUNTER_PAGE_HELPER;
			kunmap (page_helper->pages[i]);
		}
		kfree (page_helper->page_mapped);
	}
	page_helper->position		  = 0;
	page_helper->page_mapped	  = NULL;
	page_helper->pages			  = NULL;
	page_helper->page_index		  = 0;
	page_helper->page_count		  = 0;
	page_helper->page_pointer	 = NULL;
	page_helper->page_pointer_end = NULL;
}

/**
 * bewalgo_page_helper_clone() - clones the mapped pointers
 * @page_helper_dest: the structure to initialize
 * @page_helper_source: the structure to replicate
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_clone (bewalgo_page_helper* page_helper_dest, bewalgo_page_helper* page_helper_source) {
	INC_COUNTER_PAGE_HELPER;
	page_helper_dest->page_pointer	 = page_helper_source->page_pointer;
	page_helper_dest->page_pointer_end = page_helper_source->page_pointer_end;
	page_helper_dest->page_mapped	  = page_helper_source->page_mapped;
	page_helper_dest->page_index	   = page_helper_source->page_index;
	page_helper_dest->page_count	   = page_helper_source->page_count;
	page_helper_dest->pages			   = NULL;
	page_helper_dest->position		   = page_helper_source->position;
}

/**
 * bewalgo_page_helper_forward() - moves the pointer to the first byte of the next page
 * @page_helper: the structure to manipulate
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_forward (bewalgo_page_helper* page_helper) {
	INC_COUNTER_PAGE_HELPER;
	page_helper->page_index++;
	page_helper->page_pointer	 = page_helper->page_mapped[page_helper->page_index];
	page_helper->page_pointer_end = page_helper->page_mapped[page_helper->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
}

/**
 * bewalgo_page_helper_backward() - moves the pointer behind the last byte of the previous page
 * @page_helper: the structure to manipulate
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_backward (bewalgo_page_helper* page_helper) {
	INC_COUNTER_PAGE_HELPER;
	page_helper->page_index--;
	page_helper->page_pointer	 = page_helper->page_mapped[page_helper->page_index];
	page_helper->page_pointer_end = page_helper->page_mapped[page_helper->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
}

/**
 * bewalgo_page_helper_position_safe() - moves the pointer to the next page if the pointer points behind a page
 * @page_helper: the structure to manipulate
 */
static bewalgo_compress_always_inline BEWALGO_COMPRESS_DATA_TYPE* bewalgo_page_helper_position_safe (bewalgo_page_helper* page_helper) {
	INC_COUNTER_PAGE_HELPER;
	if (page_helper->page_pointer == page_helper->page_pointer_end) {
		INC_COUNTER_PAGE_HELPER;
		bewalgo_page_helper_forward (page_helper);
	}
	return page_helper->page_pointer;
}

/**
 * bewalgo_page_helper_set_position() - moves the pointer to an absolute position in the page data
 * @page_helper: the structure to manipulate
 * @offset: the offset to point to
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_set_position (bewalgo_page_helper* page_helper, U32 offset) {
	INC_COUNTER_PAGE_HELPER;
	page_helper->page_index		  = offset >> BEWALGO_COMPRESS_PAGE_SHIFT;
	page_helper->page_pointer	 = page_helper->page_mapped[page_helper->page_index];
	page_helper->page_pointer_end = page_helper->page_mapped[page_helper->page_index] + BEWALGO_COMPRESS_PAGE_SIZE;
	page_helper->page_pointer += offset & BEWALGO_COMPRESS_PAGE_OFFSET_MASK;
	page_helper->position = offset;
}

/**
 * bewalgo_page_helper_read_safe() - read data from the buffer and increment the pointer. Moves the pointer to the next page if the pointer points behind a page
 * @page_helper: the structure to manipulate
 */
static bewalgo_compress_always_inline BEWALGO_COMPRESS_DATA_TYPE bewalgo_page_helper_read_safe (bewalgo_page_helper* page_helper) {
	bewalgo_page_helper_position_safe (page_helper);
	page_helper->position++;
	return *(page_helper->page_pointer)++;
}

/**
 * bewalgo_page_helper_write_safe() - write data to the buffer and increment the pointer. Moves the pointer to the next page if the pointer points behind a page
 * @page_helper: the structure to manipulate
 * @value: data to write
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_write_safe (bewalgo_page_helper* page_helper, BEWALGO_COMPRESS_DATA_TYPE value) {
	bewalgo_page_helper_position_safe (page_helper);
	page_helper->position++;
	*(page_helper->page_pointer)++ = value;
}

/**
 * bewalgo_page_helper_read_fast() - read data from the buffer and increment the pointer. Does not move the pointer to the next page if the pointer points behind a page
 * @page_helper: the structure to manipulate
 */
static bewalgo_compress_always_inline BEWALGO_COMPRESS_DATA_TYPE bewalgo_page_helper_read_fast (bewalgo_page_helper* page_helper) {
	page_helper->position++;
	return *(page_helper->page_pointer)++;
}

/**
 * bewalgo_page_helper_write_fast() - write data to the buffer and increment the pointer. Does not move the pointer to the next page if the pointer points behind a page
 * @page_helper: the structure to manipulate
 * @value: data to write
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_write_fast (bewalgo_page_helper* page_helper, BEWALGO_COMPRESS_DATA_TYPE value) {
	page_helper->position++;
	*(page_helper->page_pointer)++ = value;
}

/**
 * bewalgo_page_helper_copy() - copy data from one page helper to another
 * @page_helper_dest: the desination for the data
 * @page_helper_source: the source of the data
 * @length: number of objects to copy in the range [0 .. 255].
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_copy (bewalgo_page_helper* page_helper_dest, bewalgo_page_helper* page_helper_source, BYTE length) {
	const BEWALGO_COMPRESS_DATA_TYPE* target;
	int								  dest_available   = page_helper_dest->page_pointer_end - page_helper_dest->page_pointer;
	int								  source_available = page_helper_source->page_pointer_end - page_helper_source->page_pointer;
	INC_COUNTER_PAGE_HELPER;
	page_helper_dest->position += length;
	page_helper_source->position += length;
	if ((length <= dest_available) & (length <= source_available)) {
		INC_COUNTER_PAGE_HELPER;
		/* length can be 0 */
		/* both pages have enough data */
		target = page_helper_dest->page_pointer + length;
		bewalgo_page_helper_copy_helper_dest (page_helper_dest->page_pointer, page_helper_source->page_pointer, target);
	} else {
		INC_COUNTER_PAGE_HELPER;
		/* length > 0 otherwise both pages would have enough data*/
		/* at least 1 page has not enough data */
		if (length <= dest_available) {
			INC_COUNTER_PAGE_HELPER;
			target = page_helper_dest->page_pointer + length;
			bewalgo_page_helper_copy_helper_src (page_helper_dest->page_pointer, page_helper_source->page_pointer, page_helper_source->page_pointer_end);
			bewalgo_page_helper_forward (page_helper_source);
			bewalgo_page_helper_copy_helper_dest (page_helper_dest->page_pointer, page_helper_source->page_pointer, target);
		} else if (length <= source_available) {
			INC_COUNTER_PAGE_HELPER;
			target = page_helper_source->page_pointer + length;
			bewalgo_page_helper_copy_helper_dest (page_helper_dest->page_pointer, page_helper_source->page_pointer, page_helper_dest->page_pointer_end);
			bewalgo_page_helper_forward (page_helper_dest);
			bewalgo_page_helper_copy_helper_src (page_helper_dest->page_pointer, page_helper_source->page_pointer, target);
		} else {
			INC_COUNTER_PAGE_HELPER;
			//* both pages need to wrap to the next one */
			if (source_available <= dest_available) {
				INC_COUNTER_PAGE_HELPER;
				/* source needs to wrap before dest */
				bewalgo_page_helper_copy_helper_src (page_helper_dest->page_pointer, page_helper_source->page_pointer, page_helper_source->page_pointer_end);
				bewalgo_page_helper_forward (page_helper_source);
				bewalgo_page_helper_copy_helper_dest (page_helper_dest->page_pointer, page_helper_source->page_pointer, page_helper_dest->page_pointer_end);
				bewalgo_page_helper_forward (page_helper_dest);
				target = page_helper_dest->page_pointer + length - dest_available;
				bewalgo_page_helper_copy_helper_dest (page_helper_dest->page_pointer, page_helper_source->page_pointer, target);
			} else {
				INC_COUNTER_PAGE_HELPER;
				/* dest needs to wrap before source */
				bewalgo_page_helper_copy_helper_dest (page_helper_dest->page_pointer, page_helper_source->page_pointer, page_helper_dest->page_pointer_end);
				bewalgo_page_helper_forward (page_helper_dest);
				bewalgo_page_helper_copy_helper_src (page_helper_dest->page_pointer, page_helper_source->page_pointer, page_helper_source->page_pointer_end);
				bewalgo_page_helper_forward (page_helper_source);
				target = page_helper_dest->page_pointer + length - source_available;
				bewalgo_page_helper_copy_helper_dest (page_helper_dest->page_pointer, page_helper_source->page_pointer, target);
			}
		}
	}
}

/**
 * bewalgo_page_helper_set() - write a given value multiple times to the destination page helper
 * @page_helper_dest: the desination for the data
 * @val: the value to write repeatedly
 * @length: number of objects to copy in the range [0 .. 255].
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_set (bewalgo_page_helper* page_helper_dest, U32 val, U32 length) {
	BEWALGO_COMPRESS_DATA_TYPE		  value = val;
	const BEWALGO_COMPRESS_DATA_TYPE* target;
	int								  i;
	int								  dest_available = page_helper_dest->page_pointer_end - page_helper_dest->page_pointer;
#if BEWALGO_COMPRESS_DATA_TYPE_SHIFT == 3
	/*length tells this function how many 32bit blocks should be written.*/
	value  = value | (((U64) value) << 32);
	length = length >> 1;
#endif
	INC_COUNTER_PAGE_HELPER;
	page_helper_dest->position += length;
	if (length <= dest_available) {
		INC_COUNTER_PAGE_HELPER;
		/* length can be 0 */
		/* pages has enough data */
		target = page_helper_dest->page_pointer + length;
		bewalgo_page_helper_set_helper (page_helper_dest->page_pointer, target, value);
	} else {
		INC_COUNTER_PAGE_HELPER;
		bewalgo_page_helper_set_helper (page_helper_dest->page_pointer, page_helper_dest->page_pointer_end, value);
		length -= dest_available;
		bewalgo_page_helper_forward (page_helper_dest);
		while (length > BEWALGO_COMPRESS_PAGE_SIZE) {
			length -= BEWALGO_COMPRESS_PAGE_SIZE;
			for (i = 0; i < (BEWALGO_COMPRESS_PAGE_SIZE >> 3); i++) {
				INC_COUNTER_PAGE_HELPER;
				*(page_helper_dest->page_pointer)++ = value;
				*(page_helper_dest->page_pointer)++ = value;
				*(page_helper_dest->page_pointer)++ = value;
				*(page_helper_dest->page_pointer)++ = value;
				*(page_helper_dest->page_pointer)++ = value;
				*(page_helper_dest->page_pointer)++ = value;
				*(page_helper_dest->page_pointer)++ = value;
				*(page_helper_dest->page_pointer)++ = value;
			}
			bewalgo_page_helper_forward (page_helper_dest);
		}
		target = page_helper_dest->page_pointer + length;
		bewalgo_page_helper_set_helper (page_helper_dest->page_pointer, target, value);
	}
}

/**
 * bewalgo_page_helper_copy_long() - copy data from one page helper to another
 * @page_helper_dest: the desination for the data
 * @page_helper_source: the source of the data
 * @length: number of objects to copy. if length is known to be always smaller than 256 than the function 'bewalgo_page_helper_copy' should be used to improve performance
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_copy_long (bewalgo_page_helper* page_helper_dest, bewalgo_page_helper* page_helper_source, int length) {
	int dest_available   = (char*) page_helper_dest->page_pointer_end - (char*) page_helper_dest->page_pointer;
	int source_available = (char*) page_helper_source->page_pointer_end - (char*) page_helper_source->page_pointer;
	INC_COUNTER_PAGE_HELPER;
	page_helper_dest->position += length;
	page_helper_source->position += length;
	if ((length <= dest_available) & (length <= source_available)) {
		INC_COUNTER_PAGE_HELPER;
		goto _finish;
	}
	/* length > 0 otherwise both pages would have enough data*/
	if (source_available >= dest_available) {
		INC_COUNTER_PAGE_HELPER;
		/* dest needs to wrap before source */
		length -= dest_available;
		memmove (page_helper_dest->page_pointer, page_helper_source->page_pointer, dest_available);
		page_helper_dest->page_pointer += dest_available;
		page_helper_source->page_pointer += dest_available;
		bewalgo_page_helper_forward (page_helper_dest);
		source_available -= dest_available;
		dest_available = PAGE_SIZE;
	}
	if (length <= source_available) {
		INC_COUNTER_PAGE_HELPER;
		goto _finish;
	}
	length -= source_available;
	memmove (page_helper_dest->page_pointer, page_helper_source->page_pointer, source_available);
	page_helper_dest->page_pointer += source_available;
	page_helper_source->page_pointer += source_available;
	bewalgo_page_helper_forward (page_helper_source);
	dest_available -= source_available;
	source_available = PAGE_SIZE;
	{
		int step1 = dest_available;
		int step2 = PAGE_SIZE - step1;
		do {
			INC_COUNTER_PAGE_HELPER;
			if (length <= step1) {
				INC_COUNTER_PAGE_HELPER;
				goto _finish;
			}
			length -= step1;
			memmove (page_helper_dest->page_pointer, page_helper_source->page_pointer, step1);
			page_helper_dest->page_pointer += step1;
			page_helper_source->page_pointer += step1;
			bewalgo_page_helper_forward (page_helper_dest);
			if (length <= step2) {
				INC_COUNTER_PAGE_HELPER;
				goto _finish;
			}
			length -= step2;
			memmove (page_helper_dest->page_pointer, page_helper_source->page_pointer, step2);
			page_helper_dest->page_pointer += step2;
			page_helper_source->page_pointer += step2;
			bewalgo_page_helper_forward (page_helper_source);
		} while (1);
	}
_finish:
	INC_COUNTER_PAGE_HELPER;
	memmove (page_helper_dest->page_pointer, page_helper_source->page_pointer, length);
	page_helper_dest->page_pointer += length;
	page_helper_source->page_pointer += length;
}

/**
 * bewalgo_page_helper_position() - get the absolute position in the page data
 * @page_helper: the structure to manipulate
 */
static bewalgo_compress_always_inline U32 bewalgo_page_helper_position (bewalgo_page_helper* page_helper) {
	INC_COUNTER_PAGE_HELPER;
	return page_helper->position;
}

/**
 * bewalgo_page_helper_seek() - moves the pointer one dataobject further
 * @page_helper: the structure to manipulate
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_seek (bewalgo_page_helper* page_helper) {
	INC_COUNTER_PAGE_HELPER;
	page_helper->position++;
	if (page_helper->page_pointer < page_helper->page_pointer_end) {
		INC_COUNTER_PAGE_HELPER;
		page_helper->page_pointer += 1;
	} else {
		INC_COUNTER_PAGE_HELPER;
		bewalgo_page_helper_forward (page_helper);
		page_helper->page_pointer += 1;
	}
}

/**
 * bewalgo_page_helper_seek_back() - moves the pointer one dataobject back
 * @page_helper: the structure to manipulate
 */
static bewalgo_compress_always_inline void bewalgo_page_helper_seek_back (bewalgo_page_helper* page_helper) {
	INC_COUNTER_PAGE_HELPER;
	page_helper->position--;
	if (page_helper->page_pointer + BEWALGO_COMPRESS_PAGE_SIZE > page_helper->page_pointer_end) {
		INC_COUNTER_PAGE_HELPER;
		page_helper->page_pointer -= 1;
	} else {
		INC_COUNTER_PAGE_HELPER;
		bewalgo_page_helper_backward (page_helper);
		page_helper->page_pointer = page_helper->page_pointer_end - 1;
	}
}

/**
 * bewalgo_page_helper_compare_safe() - compares the data at the current position of both page helpers
 * @a: one page helper to data
 * @b: one page helper to another data
 */
static bewalgo_compress_always_inline int bewalgo_page_helper_compare_safe (bewalgo_page_helper* a, bewalgo_page_helper* b) {
	INC_COUNTER_PAGE_HELPER;
	if (a->page_pointer == a->page_pointer_end) {
		INC_COUNTER_PAGE_HELPER;
		bewalgo_page_helper_forward (a);
	}
	if (b->page_pointer == b->page_pointer_end) {
		INC_COUNTER_PAGE_HELPER;
		bewalgo_page_helper_forward (b);
	}
	return *(a->page_pointer) == *(b->page_pointer);
}

#endif /* BEWALGO_PAGE_HELPER_H_ */
