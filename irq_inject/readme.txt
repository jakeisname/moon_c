

virt.dts

        ...

        foo_irq {
              compatible = "foo,foo-irq";
                interrupt-controller;
                interrupts = <0 100 1>; /* SPI, INTID=132, rising edge */
        };

	...




$ insmod foo.ko
foo-irq-driver foo_irq: resource-irq=48
foo_probe ret=0

$ echo 1 > /sys/devices/platform/foo_irq/driver/trigger
foo-irq-driver foo_irq: try to trigger irq=48
foo-irq-driver foo_irq: triggered irq=48

