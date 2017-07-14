#!/bin/bash
function removeModules {
	rmmod bewalgo_compress_test_U64
	rmmod bewalgo_compress_test_time_U64
	rmmod bewalgo_compress_test_U32
	rmmod bewalgo_compress_test_time_U32
	rmmod bewalgo_compress_test_LZ4
	rmmod bewalgo_compress_test_time_LZ4
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
}

cleanAll

