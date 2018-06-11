#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>                                              

int a1;

static ssize_t a1_show(struct device *dev, struct device_attribute *attr, 
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", a1);
}

static ssize_t a1_store(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &a1);
	// sysfs_notify(&dev->kobj, NULL, "a1");
	return sizeof(int);
}

static DEVICE_ATTR_RW(a1);

static struct attribute *foo_attrs[] = {
	&dev_attr_a1.attr,
	NULL
};

ATTRIBUTE_GROUPS(foo);

static struct resource foo_resource[] = {
		DEFINE_RES_MEM_NAMED(0x400000000, 0x100, "foo_mem"),
		DEFINE_RES_IO_NAMED(0xe000, 0x100, "foo_io"),
		DEFINE_RES_IRQ_NAMED(10, "foo_irq"),
};

static void foo_device_release(struct device *dev)
{
}


extern struct device *foo0_dev;
extern struct class foo_class;
extern struct bus_type foo_bus;

struct platform_device foo_device = {                                   
	.name = "foo",
	.id = -1,
	.dev = { 
		.groups = foo_groups,
		.release = foo_device_release,
	},
	.num_resources = ARRAY_SIZE(foo_resource),
	.resource = foo_resource,
};                                                                              
EXPORT_SYMBOL(foo_device);

static int __init foo1_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = platform_device_register(&foo_device);
	if (ret < 0) {
		printk("%s: platform_device_register() failed. ret=%d\n",
				__func__, ret);
		ret = -1;
	}

	foo0_dev = &foo_device.dev;

	return ret;	/* 0=success */
}

static void __exit foo1_exit(void)
{
	platform_device_unregister(&foo_device);

	printk("%s\n", __func__);
}

module_init(foo1_init);
module_exit(foo1_exit);
MODULE_LICENSE("GPL");
