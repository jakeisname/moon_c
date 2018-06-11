#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>                                              

int sub_dev;
int sub_drv;

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

extern struct bus_type foo_bus;
extern struct platform_device foo_device;

static struct device sub_device = { 
	.init_name = "sub",
	.groups = sub_dev_groups,
	.bus = &foo_bus,
	.parent = &foo_device.dev,
};                                                                              



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

extern struct bus_type foo_bus;

static int __init foo3_init(void)
{
	int ret = 0;

	printk("%s: bus=%p\n", __func__, &foo_bus);

	ret = device_register(&sub_device);
	if (ret < 0) {
		printk("%s: device_register() failed. ret=%d\n",
				__func__, ret);
		return -1;
	}

	ret = driver_register(&sub_driver);
	if (ret < 0) {
		printk("%s: driver_register() failed. ret=%d\n",
				__func__, ret);
		device_unregister(&sub_device);
		ret = -1;
	}

	return ret;	/* 0=success */
}

static void __exit foo3_exit(void)
{
	driver_unregister(&sub_driver);
	device_unregister(&sub_device);

	printk("%s\n", __func__);
}

module_init(foo3_init);
module_exit(foo3_exit);
MODULE_LICENSE("GPL");
