#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/platform_device.h>                                              
#include <linux/export.h>                                              
#include <linux/property.h>                                                     

/**************************************************************************
 * a1 device attribute
 **************************************************************************/
int a1 = 0;

static ssize_t a1_show(struct device *dev, struct device_attribute *attr, 
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", a1);
}

static ssize_t a1_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &a1);
	return sizeof(int);
}

/*
static struct device_attribute dev_attr_a1 = {
	.attr = { .name = "a1", .mode = 0664 },
	.show = a1_show,
	.store = a2_store,
};
*/
static DEVICE_ATTR_RW(a1);
static struct attribute *foo_device_attrs[] = {
	&dev_attr_a1.attr,
	NULL
};
ATTRIBUTE_GROUPS(foo_device);

/**************************************************************************
 * foo_device
 **************************************************************************/
struct foo_device {
	int test1;
	struct device dev;
};


struct foo_device foo = {
	.test1 = 1,
	.dev = {
		.init_name = "foo",
		.groups = foo_device_groups,
		.platform_data = &foo,
	},
};                                                                              

static int __init foo0_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = device_register(&foo.dev);
	if (ret < 0) {
		printk("%s: device_register() failed. ret=%d\n",
				__func__, ret);
		ret = -1;
	}

	return ret;	/* 0=success */
}

static void __exit foo0_exit(void)
{
	device_unregister(&foo.dev);

	printk("%s\n", __func__);
}

module_init(foo0_init);
module_exit(foo0_exit);
MODULE_LICENSE("GPL");

