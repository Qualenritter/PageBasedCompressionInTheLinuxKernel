obj-m += bewalgo_linear_compressor_U32.o
obj-m += bewalgo_page_compressor_U32.o
obj-m += bewalgo_linear_compressor_U64.o
obj-m += bewalgo_page_compressor_U64.o
ccflags-y := -save-temps -O3 -march=native
all:
	$(MAKE) clean
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	depmod -a

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -rf .tmp* .d* .b* .l* *.o *.ver *.ko *.cmd *.mod.c modules.order Module.symvers .tmp_versions/

install:
	insmod bewalgo_linear_compressor_U32.ko
	insmod bewalgo_page_compressor_U32.ko
	insmod bewalgo_linear_compressor_U64.ko
	insmod bewalgo_page_compressor_U64.ko

uninstall:
	-rmmod bewalgo_linear_compressor_U32.ko
	-rmmod bewalgo_page_compressor_U32.ko
	-rmmod bewalgo_linear_compressor_U64.ko
	-rmmod bewalgo_page_compressor_U64.ko

reload: uninstall clean all install

format:
	find . -maxdepth 10 -name "*.c" | xargs clang-format -i -sort-includes
	find . -maxdepth 10 -name "*.h" | xargs clang-format -i -sort-includes

