# use: ./make-image.sh   <tar.gz file>
#                        linaro-stretch-developer-20180416-89.tar.gz

dd if=/dev/zero of=rootfs.ext4 bs=1M count=4000
mkfs.ext4 -F rootfs.ext4
sudo mount rootfs.ext4 mnt -o loop
cd mnt
sudo rm -rf lost+found
sudo tar xf ../$1 

if [ -d binary ]; then
	sudo mv binary/* .
	sudo rm -rf binary
fi 

cd ..
sudo umount mnt

