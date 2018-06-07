obj-m := foo1.o foo2.o foo3.o
KER := $(shell uname -r)
KER_DIR := ~/workspace/lsk-4.14
KER_DIR := /lib/modules/$(KER)/build

all:
	make -C ${KER_DIR} M=$(PWD) modules
	gcc test.c -o test
	gcc test3.c -o test3

clean:
	make -C ${KER_DIR} M=$(PWD) clean
	rm -f test.o
	rm -f test
	rm -f test3.o
	rm -f test3
