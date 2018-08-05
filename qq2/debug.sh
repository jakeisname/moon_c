#!/bin/sh
#gdb-multiarch -ex "target remote localhost:1234" \
#	~/workspace/lsk-4.14/vmlinux
aarch64-linux-gnu-gdb -ex "target remote localhost:1234" \
	~/workspace/lsk-4.14/vmlinux

