obj-m += lz4_linear_hc_compress.o
obj-m += lz4_linear_compress.o
obj-m += lz4_linear_decompress.o
obj-m += lz4_pages_compress.o
obj-m += lz4_pages_decompress.o
obj-m += lz4_compressor.o
ccflags-y := -save-temps -O3
all:
	$(MAKE) clean
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	depmod -a
	
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -rf .tmp* .d* .b* .l* *.o *.ver *.ko *.cmd *.mod.c modules.order Module.symvers .tmp_versions/

install:
	insmod lz4_linear_hc_compress.ko
	insmod lz4_linear_compress.ko
	insmod lz4_linear_decompress.ko
	insmod lz4_pages_compress.ko
	insmod lz4_pages_decompress.ko
	insmod lz4_compressor.ko

uninstall:
	-rmmod lz4_compressor.ko
	-rmmod lz4_linear_hc_compress.ko
	-rmmod lz4_linear_compress.ko
	-rmmod lz4_linear_decompress.ko
	-rmmod lz4_pages_compress.ko
	-rmmod lz4_pages_decompress.ko

reload: uninstall clean all install


format:
	find . -maxdepth 10 -name "*.c" | xargs clang-format -i -sort-includes
	find . -maxdepth 10 -name "*.h" | xargs clang-format -i -sort-includes

