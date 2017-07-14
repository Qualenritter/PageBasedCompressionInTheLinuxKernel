obj-m += bewalgo_parallel_compress_test_U32.o
obj-m += bewalgo_parallel_compress_test_U64.o
obj-m += bewalgo_parallel_compress_test_time_U32.o
obj-m += bewalgo_parallel_compress_test_time_U64.o
obj-m += bewalgo_parallel_compress_test_LZ4.o
obj-m += bewalgo_parallel_compress_test_memcpy.o
obj-m += bewalgo_parallel_compress_test_time_memcpy.o

all:
	$(MAKE) clean
	cd .. && $(MAKE) reload && cd test
	cp ../Module.symvers ./Module.symvers
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules
	depmod -a

kernelLZ4:
	make CPPFLAGS="-DKERNELLZ4" -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules

clean:	
	-rmmod bewalgo_parallel_compress_test_U32
	-rmmod bewalgo_parallel_compress_test_U64
	-rmmod bewalgo_parallel_compress_test_time_U32
	-rmmod bewalgo_parallel_compress_test_time_U64
	-rmmod bewalgo_parallel_compress_test_LZ4
	-rmmod bewalgo_parallel_compress_test_memcpy
	-rmmod bewalgo_parallel_compress_test_time_memcpy
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) clean
	cd .. && $(MAKE) clean && cd test
	rm -rf .tmp* .d* .b* .l* *.o *.ver *.ko *.cmd *.mod.c modules.order Module.symvers .tmp_versions/

testhc:
	insmod lz4_compress_test.ko useHC=1 cLevel=9
	rmmod lz4_compress_test

testfast:
	insmod lz4_compress_test.ko useHC=0 acceleration=1
	rmmod lz4_compress_test
