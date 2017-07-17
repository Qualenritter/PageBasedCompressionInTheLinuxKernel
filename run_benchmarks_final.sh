#!/bin/bash
function removeModules {
	rmmod bewalgo_parallel_compressor
	rmmod bewalgo_parallel_compress_worker
	rmmod bewalgo_linear_compressor_U32
	rmmod bewalgo_page_compressor_U32
	rmmod bewalgo_linear_compressor_U64
	rmmod bewalgo_page_compressor_U64
	rmmod lz4_compressor
	rmmod lz4_linear_compress
	rmmod lz4_linear_decompress
	rmmod lz4_pages_compress
	rmmod lz4_pages_decompress
	rmmod lz4_linear_hc_compress
	rmmod linear_compressor_memcpy
	rmmod pages_compressor_memcpy
}

function cleanAll {
	removeModules
	cd parallel/test && make clean && cd ../..
	cd lz4/test && make clean && cd ../..
	cd memcpy/test && make clean && cd ../..
	cd bewalgo/test && make clean && cd ../..
	cd bewalgo-if-vs-terniary/test && make clean && cd ../..
	cd bewalgo-boolean-vs-binary-operator/test && make clean && cd ../..
	cd bewalgo-no-inline/test && make clean && cd ../..
	cd bewalgo-find-match-only/test && make clean && cd ../..
	cd lz4-find-match-only/test && make clean && cd ../..
	cd lz4-no-inline/test && make clean && cd ../..
}

cleanAll

currentpath=`pwd`

rm -f 1 2 3 4 5
echo "" > status.out
echo "" > nohup.out
chown benjamin:benjamin nohup.out
chown benjamin:benjamin status.out
for j in 1 2 3 4 5 6
do
	echo "major number : ${j}" >> "${currentpath}/status.out"
	for module in  "memcpy" "lz4" "bewalgo" "parallel" "lz4-no-inline" "lz4-find-match-only" "bewalgo-no-inline" "bewalgo-find-match-only" "bewalgo-if-vs-terniary" "bewalgo-boolean-vs-binary-operator"
	do
		for i in 1 2 3 4 5
		do
			removeModules
			echo "${module}-${i}-final" >> "${currentpath}/status.out"
			cd "${currentpath}/${module}/test" && ./run_benchmarks_final.sh
		done
exit 0
	done
	cleanAll
done
