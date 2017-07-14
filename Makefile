obj-m += bewalgo_parallel_compress_worker.o
obj-m += bewalgo_parallel_compressor.o
ccflags-y := -save-temps -O3
all:
	$(MAKE) clean
	cd compression-bewalgo && $(MAKE) reload && cd ..
	cp compression-bewalgo/Module.symvers ./Module.symvers
	cd compression-lz4 && $(MAKE) reload && cd ..
	cat compression-lz4/Module.symvers >> ./Module.symvers
	cd compression-memcpy && $(MAKE) reload && cd ..
	cat compression-memcpy/Module.symvers >> ./Module.symvers
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	depmod -a

clean:
	$(MAKE) uninstall
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	cd compression-bewalgo && $(MAKE) clean && cd ..
	cd compression-lz4 && $(MAKE) clean && cd ..
	rm -rf .tmp* .d* .b* .l* *.o *.ver *.ko *.cmd *.mod.c modules.order Module.symvers .tmp_versions/

install:
	insmod bewalgo_parallel_compress_worker.ko
	insmod bewalgo_parallel_compressor.ko

uninstall:
	-rmmod bewalgo_parallel_compressor.ko
	-rmmod bewalgo_parallel_compress_worker.ko

reload: uninstall clean all install

format:
	find . -maxdepth 10 -name "*.c" | xargs clang-format -i -sort-includes
	find . -maxdepth 10 -name "*.h" | xargs clang-format -i -sort-includes

