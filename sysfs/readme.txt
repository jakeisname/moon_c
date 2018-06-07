
*. build 

  - make


---------------------------
1. using kobject API 
---------------------------

1) sudo insmod foo1.ko

2) ./test

3) open another terminal

4) sudo echo 1 > /sys/foo/foo_value

5) rmmod foo1


---------------------------
2. using sysfs API
---------------------------

1) sudo insmod foo2.ko

2) ./test

3) open another terminal

4) sudo echo 1 > /sys/foo/foo_value

5) rmmod foo2


---------------------------
3. using device API
---------------------------

1) sudo insmod foo3.ko

2) ./test3

3) open another terminal

4) sudo echo 1 > /sys/devices/platform/foo/foo_value

5) rmmod foo3


