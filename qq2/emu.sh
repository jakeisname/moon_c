WORK_DIR=${HOME}"/workspace"
KERNEL_DIR=${WORK_DIR}"/lsk-4.14"
KERNEL_IMG=${KERNEL_DIR}"/arch/arm64/boot/Image"
ROOTFS="rootfs.ext4"
DTB_FILE="virt.dtb"
DTB="-dtb ${DTB_FILE}"
BR="virbr0"

echo "target remote localhost:1234"
qemu-system-aarch64 -s -S -smp 2 -m 1024 -cpu cortex-a57 -nographic \
	-machine virt \
	-kernel ${KERNEL_IMG} ${DTB} \
	-nographic \
	-device virtio-net-device,netdev=mynet1 \
	-netdev tap,id=mynet1,ifname=tap0,script=no,downscript=no \
	-device virtio-blk-device,drive=disk \
	-drive if=sd,id=disk,format=raw,file=${ROOTFS} \
	-append 'root=/dev/vda rw rootwait mem=1024M console=ttyAMA0,38400n8'
reset
exit
