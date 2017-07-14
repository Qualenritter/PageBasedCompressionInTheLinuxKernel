obj-m += lz4_compress_test.o
obj-m += lz4_compress_test_time.o

all:
	$(MAKE) clean
	cd .. && $(MAKE) reload && cd test
	cp ../Module.symvers ./Module.symvers
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules
	depmod -a

kernelLZ4:
	make CPPFLAGS="-DKERNELLZ4" -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules

clean:
	cd .. && $(MAKE) clean && cd test
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) clean
	rm -rf .tmp* .d* .b* .l* *.o *.ver *.ko *.cmd *.mod.c modules.order Module.symvers .tmp_versions/
