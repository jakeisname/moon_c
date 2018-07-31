WORK_DIR=${HOME}"/workspace"
KERNEL_DIR=${WORK_DIR}"/lsk-4.14"
KERNEL_IMG=${KERNEL_DIR}"/arch/arm64/boot/Image"
#ROOTFS="vexpress64-openembedded_lamp-armv8-gcc-4.9_20150725-725.img"
ROOTFS="rootfs.ext4"
DTB_FILE="virt.dtb"

if [ "$1" == "dumpdtb" ]; then
DUMPDTB=",dumpdtb=virt_original.dtb"
fi

qemu-system-aarch64 -smp 2 -m 1024 -cpu cortex-a57 -nographic \
	-machine virt${DUMPDTB} \
	-kernel ${KERNEL_IMG} \
	-dtb ${DTB_FILE} \
	-nographic \
	-device virtio-net-device,netdev=mynet1 \
	-netdev tap,id=mynet1,ifname=tap0,br=virbr0,script=no,downscript=no \
	-device virtio-blk-device,drive=disk \
	-drive if=sd,id=disk,format=raw,file=${ROOTFS} \
	-append 'root=/dev/vda rw rootwait mem=1024M console=ttyAMA0,38400n8'

if [ "$1" == "dumpdtb" ]; then 
dtc -I dtb -O dts virt_original.dtb -o virt_original.dts
reset
exit
