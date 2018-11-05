WORK_DIR=${HOME}"/workspace"
KERNEL_DIR=${WORK_DIR}"/lsk-4.14"
KERNEL_IMG=${KERNEL_DIR}"/arch/arm64/boot/Image"
ROOTFS="rootfs.ext4"
DTB_FILE="virt.dtb"

if [ "$1" == "dumpdtb" ]; then
DUMPDTB=",dumpdtb=virt_original.dtb"
echo dumpdtb to ${DUMPDTB}
else
echo use ${DTB_FILE}
DTB="-dtb ${DTB_FILE}"
fi

sudo taskset -c 0,1 qemu-system-aarch64 -smp 2 -m 1024 -cpu host -nographic -enable-kvm \
	-machine virt${DUMPDTB} \
	-kernel ${KERNEL_IMG} ${DTB} \
	-nographic \
	-device virtio-net-device,netdev=mynet1 \
	-netdev tap,id=mynet1,ifname=tap0,script=no,downscript=no \
	-device virtio-blk-device,drive=disk \
	-drive if=sd,id=disk,format=raw,file=${ROOTFS} \
	-device ivshmem-doorbell,chardev=myivshmem,vectors=32 \
	-chardev socket,path=/tmp/ivshmem_socket,id=myivshmem \
	-append 'root=/dev/vda rw rootwait mem=1024M console=ttyAMA0,38400n8'

if [ "$1" == "dumpdtb" ]; then 
dtc -I dtb -O dts virt_original.dtb -o virt_original.dts
fi
reset
exit

