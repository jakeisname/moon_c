obj-m := mcast.o
KER := $(shell uname -r)
KER_DIR := /lib/modules/$(KER)/build

all:
	echo $(KER_DIR)
	make -C ${KER_DIR} M=$(PWD) modules
	gcc mcast-user.c -o mcast-user

clean:
	make -C ${KER_DIR} M=$(PWD) clean
	rm -rf mcast-user.o
	rm -rf mcast-user
