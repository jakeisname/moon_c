#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>                                              

#include "foo.h"                                                                                

extern struct bus_type foo_bus;
extern struct platform_device foo_platform_device;

/****************************
 * sub device attribute
 ****************************/
static int sub_dev;

static ssize_t sub_dev_show(struct device *dev, struct device_attribute *attr, 
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", sub_dev);
}

static ssize_t sub_dev_store(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &sub_dev);
	// sysfs_notify(&dev->kobj, NULL, "sub_dev");
	return sizeof(int);
}

static DEVICE_ATTR_RW(sub_dev);
static struct attribute *sub_dev_attrs[] = {
	&dev_attr_sub_dev.attr,
	NULL
};
ATTRIBUTE_GROUPS(sub_dev);


/****************************
 * sub driver attribute
 ****************************/
static int sub_drv;

static ssize_t sub_drv_show(struct device_driver *driver, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", sub_drv);
}

static ssize_t sub_drv_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &sub_drv);
	// sysfs_notify(&dev->kobj, NULL, "sub_drv");
	return sizeof(int);
}

static DRIVER_ATTR_RW(sub_drv);

static struct attribute *sub_drv_attrs[] = {
	&driver_attr_sub_drv.attr,
	NULL
};

ATTRIBUTE_GROUPS(sub_drv);

/****************************
 * define device
 ****************************/
static struct device sub_device = { 
	.init_name = "sub",
	.groups = sub_dev_groups,
	.bus = &foo_bus,
	.parent = &foo_platform_device.dev,
};                                                                              

/****************************
 * define driver 
 ****************************/
static int sub_probe(struct device *dev)
{
	int ret = 0;

	printk("%s ret=%d\n", __func__, ret);

	return 0;
}

static struct device_driver sub_driver = {
	.name = "sub-driver",
	.probe  = sub_probe,
	.groups = sub_drv_groups,
	.bus = &foo_bus,
};

/****************************
 * module
 ****************************/
static int __init foo_init(void)
{
	int ret = 0;

	printk("%s: bus=%p\n", __func__, &foo_bus);

	ret = device_register(&sub_device);
	if (ret < 0) {
		printk("%s: device_register() failed. ret=%d\n",
				__func__, ret);
		goto err2;
	}

	ret = driver_register(&sub_driver);
	if (ret < 0) {
		printk("%s: driver_register() failed. ret=%d\n",
				__func__, ret);
		goto err1;
	}

	return 0;	/* 0=success */

err1:
		device_unregister(&sub_device);
err2:
	return ret;
}

static void __exit foo_exit(void)
{
	driver_unregister(&sub_driver);
	device_unregister(&sub_device);

	printk("%s\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");
