#!/bin/bash

targettime=0

make

rm -rf /tmp/data
mkdir /tmp/data


if [ "$?" -eq "0" ]; then
			for sourcefilelength in 131072
			do
				echo "sourcefilelength=${sourcefilelength}"
				insmod "b_compress_test_memcpy.ko" input_file_length=$sourcefilelength input_file_name="/dev/zero" input_file_name_short="zero" acceleration=1
				if [ "$?" -ne "0" ]; then
					rmmod "b_compress_test_memcpy"
					make clean
					exit -1
				fi
				rmmod "b_compress_test_memcpy"
				insmod "b_compress_test_time_memcpy.ko" time_mode=1 compress_mode=1 input_file_length=$sourcefilelength output_file_length=$sourcefilelength input_file_name="/dev/zero" time_multiplicator=$targettime acceleration=1
				rmmod "b_compress_test_time_memcpy"
		done
fi

make clean
