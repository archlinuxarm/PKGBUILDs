XCE_DO_DEBUG ?= 0
SHELL     ?= /bin/sh
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

export XCE_DO_DEBUG

all: 
	@echo Building XCE module for kernel $(KERNELDIR)
	$(MAKE) -C $(KERNELDIR) SUBDIRS=`pwd` modules

clean::
	rm -rf *.ko *.o .*.cmd *.mod.c .*.d .tmp_versions Module.symvers 

EXTRA_CFLAGS += -DXCE_DO_DEBUG=$(XCE_DO_DEBUG)

obj-m    := xce.o
xce-objs := xce_mod.o xce_thread.o xce_dev.o xce_ecc_rs.o
