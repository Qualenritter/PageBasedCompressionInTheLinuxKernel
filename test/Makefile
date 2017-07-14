obj-m += bewalgo_compress_test_U32.o
obj-m += bewalgo_compress_test_U64.o
obj-m += bewalgo_compress_test_time_U32.o
obj-m += bewalgo_compress_test_time_U64.o

all:
	$(MAKE) clean
	cd .. && $(MAKE) reload && cd test
	cp ../Module.symvers ./Module.symvers
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules
	depmod -a

kernelLZ4:
	make CPPFLAGS="-DKERNELLZ4" -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) clean
	cd .. && $(MAKE) clean && cd test
	rm -rf .tmp* .d* .b* .l* *.o *.ver *.ko *.cmd *.mod.c modules.order Module.symvers .tmp_versions/
