obj-m := foo-proc.o
foo-proc-objs := foo.o proc.o
cflags := -DLINUX_VERSION

all:
	        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	        make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

