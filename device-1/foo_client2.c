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
 * sub2 device attribute
 ****************************/
static int sub2_dev;

static ssize_t sub2_dev_show(struct device *dev, struct device_attribute *attr, 
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", sub2_dev);
}

static ssize_t sub2_dev_store(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &sub2_dev);
	return sizeof(int);
}

static DEVICE_ATTR_RW(sub2_dev);
static struct attribute *sub2_dev_attrs[] = {
	&dev_attr_sub2_dev.attr,
	NULL
};
ATTRIBUTE_GROUPS(sub2_dev);

/****************************
 * sub2 driver attribute
 ****************************/
static int sub2_drv;

static ssize_t sub2_drv_show(struct device_driver *driver, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", sub2_drv);
}

static ssize_t sub2_drv_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &sub2_drv);
	// sysfs_notify(&dev->kobj, NULL, "sub2_drv");
	return sizeof(int);
}

static DRIVER_ATTR_RW(sub2_drv);
static struct attribute *sub2_drv_attrs[] = {
	&driver_attr_sub2_drv.attr,
	NULL
};
ATTRIBUTE_GROUPS(sub2_drv);


/****************************
 * define foo_device
 ****************************/
static struct foo_device sub_device = { 
	.name = "sub2",
	.dev.parent = &foo_platform_device.dev,
	.dev.groups = sub2_dev_groups,
};                                                                              


static int sub2_probe(struct foo_device *dev)
{
	int ret = 0;

	printk("%s ret=%d\n", __func__, ret);

	return 0;
}

static int sub2_remove(struct foo_device *dev)
{
	int ret = 0;

	printk("%s ret=%d\n", __func__, ret);

	return 0;
}

/****************************
 * define foo_driver
 ****************************/
static struct foo_driver sub_driver = {                               
    .driver = {                                                                 
        .name = "sub2-driver",
		.groups = sub2_drv_groups,
    },                                                                          
    .probe = sub2_probe,
    .remove = sub2_remove,
}; 

/****************************
 * module
 ****************************/
static int __init foo_init(void)
{
	int ret = 0;

	printk("%s: bus=%p\n", __func__, &foo_bus);

	ret = foo_device_register(&sub_device);
	if (ret < 0) {
		printk("%s: foo_device_register() failed. ret=%d\n",
				__func__, ret);
		goto err2;
	}

	ret = foo_driver_register(&sub_driver, THIS_MODULE);
	if (ret < 0) {
		printk("%s: foo_driver_register() failed. ret=%d\n",
				__func__, ret);
		goto err1;
	}
	return 0;	/* 0=success */

err1:
		foo_device_unregister(&sub_device);
err2:
	return ret;
}

static void __exit foo_exit(void)
{
	foo_driver_unregister(&sub_driver);
	foo_device_unregister(&sub_device);

	printk("%s\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");
