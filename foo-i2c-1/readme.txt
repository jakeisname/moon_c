
Method 1) add i2c device manualy

1. insmod foo-i2c.ko
2. insmod foo-lcd.ko
3. echo foo_lcd1 0x3f > /sys/bus/i2c/devices/i2c-0/new_device
4. echo 1 > /sys/devices/platform/i2c/i2c-0/0-003f/backlight
5. echo 0x3f > /sys/bus/i2c/devices/i2c-0/delete_device
6. rmmod foo_lcd
7. rmmod foo_i2c



Method 2) add i2c device from device tree 

ex) virt.dts

i2c {
	compatible = "foo,foo-i2c";
	#address-cells = <1>;
	#size-cells = <0>;
		foo_lcd@0x3f {
		compatible = "foo,foo_lcd";
		reg = <0x3f>;
	};
};

1. insmod foo-i2c.ko
2. insmod foo-lcd.ko
3. echo 1 > /sys/devices/platform/i2c/i2c-0/0-003f/backlight
4. rmmod foo_lcd
5. rmmod foo_i2c



