obj-m := foo.o
foo-proc-objs := foo.o
cflags := -DLINUX_VERSION

all:
	        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

