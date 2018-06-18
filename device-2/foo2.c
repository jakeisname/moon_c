#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/platform_device.h>                                              
#include <linux/export.h>                                              
#include <linux/property.h>                                                     

struct foo2_device {
	int test1;
	struct platform_device dev;
};

/**************************************************************************
 * a2 device attribute
 **************************************************************************/
int a2 = 0;

static ssize_t a2_show(struct device *dev, struct device_attribute *attr, 
		char *buf)
{
	struct platform_device *dev2 = container_of(dev, 
		struct platform_device, dev);
	struct foo2_device *foo_a = container_of(dev2, struct foo2_device, dev);
	struct foo2_device *foo_b= (struct foo2_device *) dev_get_platdata(dev);

	printk(KERN_INFO "test1=%d, test1=%d\n", foo_a->test1, foo_b->test1);

	return scnprintf(buf, PAGE_SIZE, "%d\n", a2);
}

static ssize_t a2_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &a2);
	return sizeof(int);
}

static DEVICE_ATTR_RW(a2);
static struct attribute *foo2_device_attrs[] = {
	&dev_attr_a2.attr,
	NULL
};
ATTRIBUTE_GROUPS(foo2_device);


/**************************************************************************
 * foo device
 **************************************************************************/

static void foo2_release(struct device *dev)                             
{ 
        printk("%s\n", __func__);                                               
} 

static struct foo2_device foo2 = {
	.test1 = 2,
	.dev = {
		.name = "foo2",
		.dev = {
			.init_name = "foo2",
			.parent = &platform_bus,
			.bus = &platform_bus_type,
			.groups = foo2_device_groups,
			.platform_data = &foo2,
			.release = foo2_release,
		}
	},
};                                                                              

static int __init foo2_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = device_register(&foo2.dev.dev);
	if (ret < 0) {
		printk("%s: device_register() failed. ret=%d\n",
				__func__, ret);
	}

	return ret;	/* 0=success */
}

static void __exit foo2_exit(void)
{
	device_unregister(&foo2.dev.dev);

	printk("%s\n", __func__);
}

module_init(foo2_init);
module_exit(foo2_exit);
MODULE_LICENSE("GPL");

