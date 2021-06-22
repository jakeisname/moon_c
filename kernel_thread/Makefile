obj-m := test.o
KER := $(shell uname -r)
KER_DIR := /lib/modules/$(KER)/build
CFLAGS := -msign-return-address=none

all:
	make -C ${KER_DIR} M=$(PWD) modules

clean:
	make -C ${KER_DIR} M=$(PWD) clean
