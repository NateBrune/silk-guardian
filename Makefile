obj-m += silk.o

KERNELVER	?= $(shell uname -r)
KERNELDIR	?= /lib/modules/$(KERNELVER)/build
PWD		:= $(shell pwd)

all:
	make -C $(KERNELDIR) M=$(PWD)

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

install:
	make -C $(KERNELDIR) M=$(PWD) modules_install
	/sbin/depmod -a $(KERNELVER)
