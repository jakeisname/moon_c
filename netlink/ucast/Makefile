obj-m := ucast.o
KER := $(shell uname -r)
KER_DIR := /lib/modules/$(KER)/build

all:
	echo $(KER_DIR)
	make -C ${KER_DIR} M=$(PWD) modules
	gcc ucast-user.c -o ucast-user

clean:
	make -C ${KER_DIR} M=$(PWD) clean
	rm -rf ucast-user.o
	rm -rf ucast-user
