obj-m := clk.o 

KER := $(shell uname -r)
#KER_DIR := /usr/src/linux-headers-5.0.0+
KER_DIR := /lib/modules/$(KER)/build

all:
	make -C ${KER_DIR} M=$(PWD) modules

clean:
	make -C ${KER_DIR} M=$(PWD) clean
