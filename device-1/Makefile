obj-m := foo0.o foo1.o foo2.o foo3.o
KER := $(shell uname -r)
KER_DIR := ~/workspace/lsk-4.14
KER_DIR := /lib/modules/$(KER)/build

all:
	make -C ${KER_DIR} M=$(PWD) modules

clean:
	make -C ${KER_DIR} M=$(PWD) clean
