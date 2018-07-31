ps -ef | grep 'qemu-system-aarch64' -m 1 | awk '{print $2}' | xargs kill -9

