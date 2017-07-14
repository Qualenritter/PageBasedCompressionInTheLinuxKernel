#!/bin/bash

compressorname="Version-3"
targettime=0

filenames=("/src/data/raw_rand" "/dev/zero" "/src/data/raw_text" "/src/data/raw_tbnt" )
shortfilenames=("rand" "zero" "text" "tbnt" )

rm -rf /tmp/data
mkdir /tmp/data

if [ "$?" -eq "0" ]; then

	for processorcount in 1 2 8
	do
cat <<EOT > ../config.h
#define BEWALGO_PARALLEL_COMPRESS_WORKER_COUNT ${processorcount}
EOT

make

		for fileindex in "${!filenames[@]}";
		do
			for sourcefilelength in 131072 524288 104857600
			do
				for var_accelleration in 1
				do
					for var_datatype in "U32" "U64"
					do
						echo "processorcount=${processorcount} var_accelleration=${var_accelleration} var_datatype=${var_datatype} sourcefilelength=${sourcefilelength} filename=${filenames[$fileindex]} "
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
						done
					done
				done
			done
		done
	done
fi

make clean
