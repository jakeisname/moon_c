#!/bin/sh
aarch64-linux-gnu-gdb -ex "target remote localhost:1234" \
	~/workspace/linux-4.14/vmlinux

