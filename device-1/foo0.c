#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/platform_device.h>                                              

static const struct attribute_group *foo_class_groups[];

int export = -1;
int bus_bus = 0;
int bus_dev = 0;
int bus_drv = 0;
static struct device *new_dev = NULL;
struct device *foo0_dev = NULL;

/* for foo1.ko */
EXPORT_SYMBOL(foo0_dev);

/**************************************************************************
 * class
 **************************************************************************/

void export_device(struct device *dev, int export);

static ssize_t export_show(struct class *class, struct class_attribute *attr, 
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", export);
}

static ssize_t export_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct device *dev = foo0_dev;

	if (export != -1) {
		printk(KERN_ERR "class device is created already");
		return sizeof(int);
	}
		
	sscanf(buf, "%d", &export);

	export_device(dev, export);

	// sysfs_notify(&dev->kobj, NULL, "c1");
	return sizeof(int);
}

static ssize_t unexport_show(struct class *class, struct class_attribute *attr, 
		char *buf)
{

	return scnprintf(buf, PAGE_SIZE, "%d\n", export);
}

static ssize_t unexport_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t len)
{
	if (export == -1) {
		printk(KERN_ERR "class device was not created");
		return sizeof(int);
	}
	
	export = -1;

	// device_destroy(class, 0);
	put_device(new_dev);
	device_unregister(new_dev);

	// sysfs_notify(&dev->kobj, NULL, "c1");
	return sizeof(int);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0))
static CLASS_ATTR_RW(export);
static CLASS_ATTR_RW(unexport);

static struct attribute *foo_class_attrs[] = {
	&class_attr_export.attr,
	&class_attr_unexport.attr,
	NULL
};

ATTRIBUTE_GROUPS(foo_class);
#else 
static struct class_attribute foo_class_attrs[] = {
	__ATTR_RW(export),
	__ATTR_RW(unexport),
	__ATTR_NULL,
};
#endif

static void foo_classdev_release(struct device *dev)
{
	printk("%s\n", __func__);
}



struct class foo_class = {                                   
	.name = "foo_class",
	.owner = THIS_MODULE,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0))
	.class_groups = foo_class_groups,
#else
	.class_attrs = foo_class_attrs,
#endif
	.dev_groups = NULL,
	.dev_release = foo_classdev_release,
};                                                                              

EXPORT_SYMBOL(foo_class);

void export_device(struct device *dev, int export)
{
	new_dev = device_create_with_groups(&foo_class, dev,
			MKDEV(0, 0), NULL, NULL,
			"foo%d", export);
}

/**************************************************************************
 * bus_type
 **************************************************************************/

static ssize_t bus_bus_show(struct bus_type *bus, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", bus_bus);
}

static ssize_t bus_bus_store(struct bus_type *bus, 
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &bus_bus);

	return sizeof(int);
}
static BUS_ATTR_RW(bus_bus);

static struct attribute *bus_bus_attrs[] = {
	&bus_attr_bus_bus.attr,
	NULL
};
ATTRIBUTE_GROUPS(bus_bus);

static ssize_t bus_dev_show(struct device *dev, struct device_attribute *attr, 
	char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", bus_dev);
}

static ssize_t bus_dev_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &bus_dev);

	return sizeof(int);
}
static DEVICE_ATTR_RW(bus_dev);

static struct attribute *bus_dev_attrs[] = {
	&dev_attr_bus_dev.attr,
	NULL
};
ATTRIBUTE_GROUPS(bus_dev);

static ssize_t bus_drv_show(struct device_driver *drv, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", bus_drv);
}

static ssize_t bus_drv_store(struct device_driver *drv, 
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &bus_drv);

	return sizeof(int);
}
static DRIVER_ATTR_RW(bus_drv);

static struct attribute *bus_drv_attrs[] = {
	&driver_attr_bus_drv.attr,
	NULL
};
ATTRIBUTE_GROUPS(bus_drv);

struct bus_type foo_bus = {                                   
	.name = "foo_bus",
	.bus_groups = bus_bus_groups,
	.dev_groups = bus_dev_groups,
	.drv_groups = bus_drv_groups,
};                                                                              

EXPORT_SYMBOL(foo_bus);

static int __init foo0_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = class_register(&foo_class);
	if (ret < 0) {
		printk("%s: class_register() failed. ret=%d\n",
				__func__, ret);
		ret = -1;
	}

	ret = bus_register(&foo_bus);
	if (ret < 0) {
		printk("%s: bus_register() failed. ret=%d\n",
				__func__, ret);
		ret = -1;
	}

	return ret;	/* 0=success */
}

static void __exit foo0_exit(void)
{
	bus_unregister(&foo_bus);
	class_unregister(&foo_class);

	printk("%s\n", __func__);
}

module_init(foo0_init);
module_exit(foo0_exit);
MODULE_LICENSE("GPL");
