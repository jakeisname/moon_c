if [ ! -d mnt ]; then
echo mn
fi

if [ ! -d deb ]; then
echo de
fi

sudo mount rootfs.ext4 mnt -o loop
cd mnt
sudo cp ../../linux-4.14.59_4.14.59-4_arm64.changes .
sudo cp ../../linux-4.14.59_4.14.59-4.debian.tar.gz .
sudo cp ../../linux-4.14.59_4.14.59-4.dsc .
sudo cp ../../linux-4.14.59_4.14.59.orig.tar.gz .
sudo cp ../../linux-headers-4.14.59_4.14.59-4_arm64.deb .
sudo cp ../../linux-image-4.14.59_4.14.59-4_arm64.deb .
sudo cp ../../linux-image-4.14.59-dbg_4.14.59-4_arm64.deb .
sudo cp ../../linux-libc-dev_4.14.59-4_arm64.deb .
cd ..
sudo umount mnt
cd deb
mv ../../linux-4.14.59_4.14.59-4_arm64.changes .
mv ../../linux-4.14.59_4.14.59-4.debian.tar.gz .
mv ../../linux-4.14.59_4.14.59-4.dsc .
mv ../../linux-4.14.59_4.14.59.orig.tar.gz .
mv ../../linux-headers-4.14.59_4.14.59-4_arm64.deb .
mv ../../linux-image-4.14.59_4.14.59-4_arm64.deb .
mv ../../linux-image-4.14.59-dbg_4.14.59-4_arm64.deb .
mv ../../linux-libc-dev_4.14.59-4_arm64.deb .
cd ..

