obj-m += linear_compressor_memcpy.o
obj-m += page_compressor_memcpy.o
ccflags-y := -save-temps -O3 -march=native
all:
	$(MAKE) clean
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	depmod -a

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -rf .tmp* .d* .b* .l* *.o *.ver *.ko *.cmd *.mod.c modules.order Module.symvers .tmp_versions/

install:
	insmod linear_compressor_memcpy.ko
	insmod page_compressor_memcpy.ko

uninstall:
	-rmmod linear_compressor_memcpy.ko
	-rmmod page_compressor_memcpy.ko

reload: uninstall clean all install

format:
	find . -maxdepth 10 -name "*.c" | xargs clang-format -i -sort-includes
	find . -maxdepth 10 -name "*.h" | xargs clang-format -i -sort-includes

