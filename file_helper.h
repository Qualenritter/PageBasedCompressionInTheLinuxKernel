/*
 * file_helper.h
 *
 *  Created on: July 14, 2017
 *      Author: Benjamin Warnke
 */

#ifndef FILE_HELPER_H_
#define FILE_HELPER_H_

#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/types.h>

#ifndef __maybe_unused
/*help eclipse if the indexer fails*/
#define __maybe_unused
#endif /* __maybe_unused */

/**
 * write_file() - writes data of size size into file path
 * @path: file to write buffer into
 * @data: buffer containing the data to write
 * @size: size of data
 */
static void __maybe_unused write_file (char* path, char* data, size_t size) {
	struct file* fd;
	mm_segment_t oldFS;
	/*
	 * open file write only (O_WRONLY),
	 * create if not existing (O_CREAT),
	 * truncate file contents before opening (O_TRUNC)
	 */
	fd = filp_open (path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	if (!IS_ERR (fd)) {
		/*
		 * Backup current file system and set to KERNEL_DS
		 * ("Kernel data segment")
		 */
		oldFS = get_fs ();
		set_fs (KERNEL_DS);
		/* Write output to file */
		vfs_write (fd, data, size, &fd->f_pos);
		/* Restore old file system and close file */
		set_fs (oldFS);
		filp_close (fd, NULL);
		printk (KERN_INFO "[file_helper]: Wrote to %s\n", path);
	} else {
		printk (KERN_ERR "[file_helper]: Error opening file %s", path);
	}
}

/**
 * read_file() - reads data of size size from file path
 * @path: file to read buffer from
 * @data: buffer allocated for the data to read
 * @size: size of buffer
 */
static int __maybe_unused read_file (char* path, char* data, size_t size) {
	struct file* fd;
	mm_segment_t oldFS;
	/*
	 * open file read only (O_RDONLY)
	 */
	fd = filp_open (path, O_RDONLY, S_IRWXU);
	if (!IS_ERR (fd)) {
		/*
		 * Backup current file system and set to KERNEL_DS
		 * ("Kernel data segment")
		 */
		oldFS = get_fs ();
		set_fs (KERNEL_DS);
		/* Read input from file */
		vfs_read (fd, data, size, &fd->f_pos);
		/* Restore old file system and close file */
		set_fs (oldFS);
		filp_close (fd, NULL);
		printk (KERN_INFO "[file_helper]: Load from %s\n", path);
		return 0;
	} else {
		printk (KERN_ERR "[file_helper]: Error opening file %s", path);
		return -1;
	}
}

/**
 * write_pages() - writes data of size size into file path
 * @path: file to write buffer into
 * @data: page array containing the data to write
 * @size: size of data
 */
static void __maybe_unused write_pages (char* path, struct page** data, size_t size) {
	struct file* fd;
	mm_segment_t oldFS;
	int			 i;
	int			 src_page_count;
	/*
	 * open file write only (O_WRONLY),
	 * create if not existing (O_CREAT),
	 * truncate file contents before opening (O_TRUNC)
	 */
	fd = filp_open (path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	if (!IS_ERR (fd)) {
		/*
		 * Backup current file system and set to KERNEL_DS
		 * ("Kernel data segment")
		 */
		oldFS = get_fs ();
		set_fs (KERNEL_DS);
		/* Write output to file */
		src_page_count = (size >> PAGE_SHIFT) + ((size & (PAGE_SIZE - 1)) > 0 ? 1 : 0);
		if ((size & (PAGE_SIZE - 1)) > 0) {
			for (i = 0; i < src_page_count - 1; i++) {
				vfs_write (fd, page_address (data[i]), PAGE_SIZE, &fd->f_pos);
			}
			vfs_write (fd, page_address (data[src_page_count - 1]), size & (PAGE_SIZE - 1), &fd->f_pos);
		} else {
			for (i = 0; i < src_page_count; i++) {
				vfs_write (fd, page_address (data[i]), PAGE_SIZE, &fd->f_pos);
			}
		}
		/* Restore old file system and close file */
		set_fs (oldFS);
		filp_close (fd, NULL);

		printk (KERN_INFO "[file_helper]: Wrote to %s\n", path);
	} else {
		printk (KERN_ERR "[file_helper]: Error opening file %s", path);
	}
}

/**
 * read_file() - reads data of size size from file path
 * @path: file to read buffer from
 * @data: page array allocated for the data to read
 * @size: size of buffer
 */
static int __maybe_unused read_pages (char* path, struct page** data, size_t size) {
	struct file* fd;
	mm_segment_t oldFS;
	int			 i;
	int			 src_page_count;
	/*
	 * open file read only (O_RDONLY)
	 */
	fd = filp_open (path, O_RDONLY, S_IRWXU);
	if (!IS_ERR (fd)) {
		/*
		 * Backup current file system and set to KERNEL_DS
		 * ("Kernel data segment")
		 */
		oldFS = get_fs ();
		set_fs (KERNEL_DS);
		/* Read input from file */
		src_page_count = (size >> PAGE_SHIFT) + ((size & (PAGE_SIZE - 1)) > 0 ? 1 : 0);
		if ((size & (PAGE_SIZE - 1)) > 0) {
			for (i = 0; i < src_page_count - 1; i++) {
				vfs_read (fd, page_address (data[i]), PAGE_SIZE, &fd->f_pos);
			}
			vfs_read (fd, page_address (data[src_page_count - 1]), size & (PAGE_SIZE - 1), &fd->f_pos);
		} else {
			for (i = 0; i < src_page_count; i++) {
				vfs_read (fd, page_address (data[i]), PAGE_SIZE, &fd->f_pos);
			}
		}
		/* Restore old file system and close file */
		set_fs (oldFS);
		filp_close (fd, NULL);
		printk (KERN_INFO "[file_helper]: Load from %s\n", path);
		return 0;
	} else {
		printk (KERN_ERR "[file_helper]: Error opening file %s", path);
		return -1;
	}
}

/*
 * struct mytimespec
 * seconds and nanoseconds to describe one duration
 */
typedef struct {
	size_t tv_sec;
	size_t tv_nsec;
} mytimespec;

/*
 * struct time_measurement_data
 * contains all information for the benchmarks
 */
typedef struct {
	char*		input_file_name;
	mytimespec  compress_time;
	int			korrect_result;
	int			time_multiplicator;
	int			mode;
	int			timemode;
	int			compressedlength;
	int			decompressedlength;
	int			acceleration;
	const char* compressor_name;
} time_measurement_data;

/**
 * append_time_measurement_to_file() - appends an time measurement to a file
 * @path: file to append measurement
 * @data: the measurement to append
 */
static void __maybe_unused append_time_measurement_to_file (char* path, time_measurement_data* data) {
	struct file*   fd;
	mm_segment_t   oldFS;
	char		   buf[512]; /*buffer for the string representation of the measurement*/
	int			   len;		 /*used length of the buffer*/
	struct timeval now;
	struct tm	  tm_val;
	/*get current date and time*/
	do_gettimeofday (&now);
	/*convert it to a usable format*/
	time_to_tm (now.tv_sec, 0, &tm_val);
	/*
	 * open file write only (O_WRONLY),
	 * create if not existing (O_CREAT),
	 * append to existing data (O_APPEND)
	 */
	fd = filp_open (path, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
	if (!IS_ERR (fd)) {
		/*
		 * Backup current file system and set to KERNEL_DS
		 * ("Kernel data segment")
		 */
		oldFS = get_fs ();
		set_fs (KERNEL_DS);
		/* convert measurement to string */
		len = snprintf (buf,
						512,
						"%lld/%lld/%lld %02lld:%02lld:%02lld,%d,%d,%d,%s,%d,%d,%s,%lld.%09lld,%d\n",
						(long long int) 1900 + tm_val.tm_year,
						(long long int) tm_val.tm_mon + 1,
						(long long int) tm_val.tm_mday,
						(long long int) tm_val.tm_hour,
						(long long int) tm_val.tm_min,
						(long long int) tm_val.tm_sec,
						data->acceleration,
						data->compressedlength,
						data->decompressedlength,
						data->compressor_name,
						data->mode,
						data->timemode,
						data->input_file_name,
						(long long int) data->compress_time.tv_sec,
						(long long int) data->compress_time.tv_nsec,
						data->time_multiplicator);
		/* Write output to file */
		vfs_write (fd, buf, len, &fd->f_pos);
		/* Restore old file system and close file */
		set_fs (oldFS);
		filp_close (fd, NULL);
		printk (KERN_INFO "[file_helper]: Wrote to %s\n", path);
	} else {
		printk (KERN_ERR "[file_helper]: Error opening file %s", path);
	}
}

/* global log file for debugging purposes*/
static struct file* log_fd;
static mm_segment_t log_oldFS;

/**
 * open_log_file() - opens the global log file
 * @path: file to use
 */
static int __maybe_unused open_log_file (char* path) {
	/*
	 * open file write only (O_WRONLY),
	 * create if not existing (O_CREAT),
	 * append to existing data (O_APPEND)
	 */
	log_fd = filp_open (path, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
	if (!IS_ERR (log_fd)) {
		log_oldFS = get_fs ();
		set_fs (KERNEL_DS);
		return 0;
	} else {
		printk (KERN_ERR "[file_helper]: Error opening file %s", path);
		return 1;
	}
}

/**
 * open_log_file() - close the global log file
 */
static void __maybe_unused close_log_file (void) {
	set_fs (log_oldFS);
	filp_close (log_fd, NULL);
}

/* used for writing to the global log file. */
#ifndef APPEND_LOG
#define APPEND_LOG(fmt, ...)                           \
	do {                                               \
		char buf[128];                                 \
		int  len;                                      \
		len = snprintf (buf, 128, fmt, ##__VA_ARGS__); \
		vfs_write (log_fd, buf, len, &log_fd->f_pos);  \
	} while (0)
#endif /* APPEND_LOG */
#endif /* FILE_HELPER_H_ */
