rm -f /tmp/ivshmem_socket
ivshmem-server -p /tmp/ivshmem_pid -S /tmp/ivshmem_socket -l 512k -n 32

