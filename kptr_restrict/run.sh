echo 0 > /proc/sys/kernel/kptr_restrict
insmod foo.ko param=0
rmmod foo

echo 1 > /proc/sys/kernel/kptr_restrict
insmod foo.ko param=1
rmmod foo

echo 2 > /proc/sys/kernel/kptr_restrict
insmod foo.ko param=2
rmmod foo

echo 0 > /proc/sys/kernel/kptr_restrict
