#!/bin/bash

compressorname="LZ4"
targettime=0

filenames=("/src/data/raw_text" "/src/data/raw_tbnt" "/src/data/raw_rand" "/dev/zero" )
shortfilenames=("text" "tbnt" "rand" "zero" )

rm -rf /tmp/data
mkdir /tmp/data

if [ "$?" -eq "0" ]; then
	for fileindex in "${!filenames[@]}";
	do
		for sourcefilelength in 524288 104857600
		do
			for var_accelleration in 1
			do
cat <<EOT > ../config.h
#define LZ4_ACCELERATION_DEFAULT 1
#define LOG_STATISTICS
#define LOG_FILE_STATISTIC(DATA_TYPE) "/src/bCompress-Generic-Statistic-${compressorname}-${var_datatype}-${shortfilenames[$fileindex]}-${sourcefilelength}.csv"
#include "interfaces/file_helper.h"
EOT
make
				echo "var_accelleration=${var_accelleration} sourcefilelength=${sourcefilelength} filename=${filenames[$fileindex]} "
				insmod "lz4_compress_test.ko" input_file_length=$sourcefilelength input_file_name="${filenames[$fileindex]}" input_file_name_short="${shortfilenames[$fileindex]}-$sourcefilelength" acceleration=$var_accelleration
				if [ "$?" -ne "0" ]; then
					rmmod "lz4_compress_test"
					make cleanall
					exit -1
				fi
				rmmod "lz4_compress_test"
			done
		done
	done
fi

make clean

cat <<EOT > ../config.h
#define LZ4_ACCELERATION_DEFAULT 1
/*#undef LOG_STATISTICS*/
/*#undef LOG_FILE_STATISTIC(DATA_TYPE)*/
EOT


