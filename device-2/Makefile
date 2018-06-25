obj-m := foo.o foo2.o foo3.o drv3.o drv4.o

#KDIR := 4.14.37-g744dfee
KDIR := $(shell uname -r)

all:
	make -C /lib/modules/${KDIR}/build M=$(PWD) modules

clean:
	make -C /lib/modules/${KDIR}/build M=$(PWD) clean
