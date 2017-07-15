#!/bin/bash

compressorname="LZ4"
targettime=20

filenames=( "/src/data/raw_final_hdf5" "/src/data/raw_final_silesia_corpus" "/src/data/raw_final_text" )
shortfilenames=( "hdf5" "silesia" "text")

cat <<EOT > ../config.h
#define LZ4_ACCELERATION_DEFAULT 1
/*#undef LOG_STATISTICS*/
/*#undef LOG_FILE_STATISTIC(DATA_TYPE)*/
EOT

make

rm -rf /tmp/data
mkdir /tmp/data

if [ "$?" -eq "0" ]; then
	for fileindex in "${!filenames[@]}";
	do
		sourcefilelength=`du --bytes "${filenames[$fileindex]}" | cut -f1`
		sourcefilelength=$(( $sourcefilelength / 8 ))
		sourcefilelength=$(( $sourcefilelength * 8 ))
		for var_accelleration in 1
		do
			echo "var_accelleration=${var_accelleration} sourcefilelength=${sourcefilelength} filename=${filenames[$fileindex]} "
			for timemode in 1 2
			do
				insmod "lz4_compress_test_time.ko" time_mode=$timemode compress_mode=1 input_file_length=$sourcefilelength output_file_length=$sourcefilelength input_file_name="${filenames[$fileindex]}" time_multiplicator=$targettime acceleration=$var_accelleration
				if [ "$?" -ne "0" ]; then
					rmmod "lz4_compress_test_time"
					make clean
					exit -1
				fi
				rmmod "lz4_compress_test_time"
			done
		done
	done
fi

make clean
