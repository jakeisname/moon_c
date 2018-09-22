obj-m := foo-pci-1.o

#KDIR := 4.14.37-g744dfee
#KDIR := /lib/modules/$(shell uname -r)/build
#KDIR := /lib/modules/4.14.37-g744dfee/build
#KDIR  := /lib/modules/4.14.35-g8abfa78-dirty/build
#KDIR := /home/jake/workspace/lsk-4.14
#KDIR := /usr/src/lsk-4.14
KDIR := /home/jake/workspace/5306/kernel/linux

all:
	make -C ${KDIR} M=$(PWD) modules

clean:
	make -C ${KDIR} M=$(PWD) clean

