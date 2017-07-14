#!/bin/bash

compressorname="Version-3"
targettime=0

filenames=("/src/data/raw_text" "/src/data/raw_tbnt" "/src/data/raw_rand" "/dev/zero" )
shortfilenames=("text" "tbnt" "rand" "zero" )

cat <<EOT > ../config.h
#define BEWALGO_ACCELERATION_DEFAULT 1
/*#undef LOG_STATISTICS*/
/*#undef LOG_FILE_STATISTIC(DATA_TYPE)*/
EOT

make

rm -rf /tmp/data
mkdir /tmp/data

if [ "$?" -eq "0" ]; then
	for fileindex in "${!filenames[@]}";
	do
		for sourcefilelength in  8 16 24 336 5168 21768 37448 39288 131072 262152 524288 526328 528368 2088976 104857600
		do
			for var_accelleration in 1 2 128 1024
			do
				for var_datatype in "U32" "U64"
				do
					echo "var_accelleration=${var_accelleration} var_datatype=${var_datatype} sourcefilelength=${sourcefilelength} filename=${filenames[$fileindex]} "
					insmod "bewalgo_compress_test_${var_datatype}.ko" input_file_length=$sourcefilelength input_file_name="${filenames[$fileindex]}" input_file_name_short="${shortfilenames[$fileindex]}-$sourcefilelength" acceleration=$var_accelleration
					if [ "$?" -ne "0" ]; then
						rmmod "bewalgo_compress_test_${var_datatype}"
						make cleanall
						exit -1
					fi
					rmmod "bewalgo_compress_test_${var_datatype}"
					for timemode in 1 2
					do
						destfilelength=`du --bytes "/tmp/data/data.compressed"  | cut -f1`
						insmod "bewalgo_compress_test_time_${var_datatype}.ko" time_mode=$timemode compress_mode=1 input_file_length=$sourcefilelength output_file_length=$destfilelength input_file_name="${filenames[$fileindex]}" time_multiplicator=$targettime acceleration=$var_accelleration
						if [ "$?" -ne "0" ]; then
							rmmod "bewalgo_compress_test_time_${var_datatype}"
							make cleanall
							exit -1
						fi
						rmmod "bewalgo_compress_test_time_${var_datatype}"
						insmod "bewalgo_compress_test_time_${var_datatype}.ko" time_mode=$timemode compress_mode=2 input_file_length=$destfilelength output_file_length=$sourcefilelength input_file_name="${shortfilenames[$fileindex]}-$sourcefilelength" time_multiplicator=$targettime
						if [ "$?" -ne "0" ]; then
							rmmod "bewalgo_compress_test_time_${var_datatype}"
							make cleanall
							exit -1
						fi
						rmmod "bewalgo_compress_test_time_${var_datatype}"
					done
				done
			done
		done
	done
fi

make clean
