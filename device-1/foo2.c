#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>                                              

static struct class foo_class;
static const struct attribute_group *foo_class_groups[];

int export = -1;
static struct device *new_dev = NULL;

static ssize_t export_show(struct class *class, struct class_attribute *attr, 
		char *buf)
{

	return scnprintf(buf, PAGE_SIZE, "%d\n", export);
}

static int match(struct device *dev, const void *data)
{
	return 1; /* always success */
}

static ssize_t export_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct device *dev;

	if (export != -1) {
		printk(KERN_ERR "class device is created already");
		return sizeof(int);
	}
		
	sscanf(buf, "%d", &export);

	/* 1st device */
	dev = class_find_device(class, NULL, NULL, &match);

	new_dev = device_create_with_groups(&foo_class, dev,
			MKDEV(0, 0), NULL, NULL,
			"foo%d", export);

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

static CLASS_ATTR_RW(export);
static CLASS_ATTR_RW(unexport);

static struct attribute *foo_class_attrs[] = {
	&class_attr_export.attr,
	&class_attr_unexport.attr,
	NULL
};

ATTRIBUTE_GROUPS(foo_class);

static void foo_classdev_release(struct device *dev)
{
	printk("%s\n", __func__);
}

static struct class foo_class = {                                   
	.name = "foo_class",
	.owner = THIS_MODULE,
	.class_groups = foo_class_groups,
	.dev_groups = NULL,
	.dev_release = foo_classdev_release,
};                                                                              

static int __init foo_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = class_register(&foo_class);
	if (ret < 0) {
		printk("%s: class_register() failed. ret=%d\n",
				__func__, ret);
		ret = -1;
	}

	return ret;	/* 0=success */
}

static void __exit foo_exit(void)
{
	class_unregister(&foo_class);

	printk("%s\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");
