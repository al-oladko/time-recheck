TARGET=ipt_tm

obj-m += $(TARGET).o
ipt_tm-y := ipt_tm_mod.o

KERNELDIR = /lib/modules/$(shell uname -r)/build

modules:
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

.PHONY: modules clean
