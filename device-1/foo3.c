#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>                                              

int sub_dev1;
int sub_drv1;

static ssize_t sub_dev1_show(struct device *dev, struct device_attribute *attr, 
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", sub_dev1);
}

static ssize_t sub_dev1_store(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &sub_dev1);
	// sysfs_notify(&dev->kobj, NULL, "a1");
	return sizeof(int);
}

static DEVICE_ATTR_RW(sub_dev1);

static struct attribute *foo_sub_attrs[] = {
	&dev_attr_sub_dev1.attr,
	NULL
};

ATTRIBUTE_GROUPS(foo_sub);


struct foo_device {
	char * name;
	struct device dev;
};

static struct foo_device sub_device = { 
	.name = "sub",
	.dev.groups = foo_sub_groups,
};                                                                              

static int sub_probe(struct device *dev)
{
	int ret = 0;

	printk("%s ret=%d\n", __func__, ret);

	return 0;
}

struct foo_driver {
	char *name;
	struct device_driver driver;
};

static struct foo_driver sub_driver = {
	.name = "sub-driver",
	.driver = {
		.name = "sub-driver",
		.groups = foo_sub_groups,
		.probe  = sub_probe,
	}
};

static int __init foo3_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = device_register(&sub_device.dev);
	if (ret < 0) {
		printk("%s: device_register() failed. ret=%d\n",
				__func__, ret);
		return -1;
	}

	ret = driver_register(&sub_driver.driver);
	if (ret < 0) {
		printk("%s: driver_register() failed. ret=%d\n",
				__func__, ret);
		device_unregister(&sub_device.dev);
		ret = -1;
	}

	return ret;	/* 0=success */
}

static void __exit foo3_exit(void)
{
	driver_unregister(&sub_driver.driver);
	device_unregister(&sub_device.dev);

	printk("%s\n", __func__);
}

module_init(foo3_init);
module_exit(foo3_exit);
MODULE_LICENSE("GPL");
