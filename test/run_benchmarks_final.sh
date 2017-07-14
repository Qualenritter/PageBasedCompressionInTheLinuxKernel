#!/bin/bash

compressorname="Version-3"
targettime=20

filenames=( "/src/data/raw_final_hdf5" "/src/data/raw_final_silesia_corpus" "/src/data/raw_final_text" )
shortfilenames=( "hdf5" "silesia" "text")
var_accelleration=1

rm -rf /tmp/data
mkdir /tmp/data

if [ "$?" -eq "0" ]; then
	for processorcount in 1 2 3 4 5 6 7 8
	do
cat <<EOT > ../config.h
#define BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT ${processorcount}
EOT
		make
		for fileindex in "${!filenames[@]}";
		do
			sourcefilelength=`du --bytes "${filenames[$fileindex]}" | cut -f1`
			sourcefilelength=$(( $sourcefilelength / 8 ))
			sourcefilelength=$(( $sourcefilelength * 8 ))
			for var_datatype in "U64" "U32"
			do
				echo "processorcount=${processorcount} var_datatype=${var_datatype} sourcefilelength=${sourcefilelength} filename=${filenames[$fileindex]} "
				insmod "bewalgo_parallel_compress_test_${var_datatype}.ko" input_file_length=$sourcefilelength input_file_name="${filenames[$fileindex]}" input_file_name_short="${shortfilenames[$fileindex]}-$sourcefilelength" acceleration=$var_accelleration
				if [ "$?" -ne "0" ]; then
					rmmod "bewalgo_parallel_compress_test_${var_datatype}"
					make clean
					exit -1
				fi
				rmmod "bewalgo_parallel_compress_test_${var_datatype}"
				for timemode in 1 2
				do
					destfilelength=`du --bytes "/tmp/data/data.compressed"  | cut -f1`
					insmod "bewalgo_parallel_compress_test_time_${var_datatype}.ko" time_mode=$timemode compress_mode=1 input_file_length=$sourcefilelength output_file_length=$destfilelength input_file_name="${filenames[$fileindex]}" time_multiplicator=$targettime acceleration=$var_accelleration
					if [ "$?" -ne "0" ]; then
						rmmod "bewalgo_parallel_compress_test_time_${var_datatype}"
							make clean
						exit -1
					fi
					rmmod "bewalgo_parallel_compress_test_time_${var_datatype}"
					insmod "bewalgo_parallel_compress_test_time_${var_datatype}.ko" time_mode=$timemode compress_mode=2 input_file_length=$destfilelength output_file_length=$sourcefilelength input_file_name="${shortfilenames[$fileindex]}-$sourcefilelength" time_multiplicator=$targettime
					if [ "$?" -ne "0" ]; then
						rmmod "bewalgo_parallel_compress_test_time_${var_datatype}"
						make clean
						exit -1
					fi
					rmmod "bewalgo_parallel_compress_test_time_${var_datatype}"
				done
			done
		done
		echo "processorcount=${processorcount} memcpy "
		for timemode in 1 2
		do
			insmod "bewalgo_parallel_compress_test_time_memcpy.ko" time_mode=$timemode compress_mode=1 input_file_length=468318452 output_file_length=468318452 input_file_name="/dev/zero" time_multiplicator=$targettime acceleration=$var_accelleration
			if [ "$?" -ne "0" ]; then
				rmmod "bewalgo_parallel_compress_test_time_memcpy"
				make clean
				exit -1
			fi
			rmmod "bewalgo_parallel_compress_test_time_memcpy"
		done
	done
fi

make clean
