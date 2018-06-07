#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>                                              

struct foo_attr{
	struct device_attribute attr;
	int value;
};

static struct foo_attr foo_value;
static struct foo_attr foo_notify;

static struct attribute *foo_attrs[] = {
	&foo_value.attr.attr,
	&foo_notify.attr.attr,
	NULL
};

ATTRIBUTE_GROUPS(foo);

static ssize_t foo_show(struct device *dev, struct device_attribute *attr, 
		char *buf)
{
	struct foo_attr *foo = container_of(attr, struct foo_attr, attr);
	return scnprintf(buf, PAGE_SIZE, "%d\n", foo->value);
}

static ssize_t foo_store(struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t len)
{
	struct foo_attr *foo = container_of(attr, struct foo_attr, attr);

	sscanf(buf, "%d", &foo->value);
	sysfs_notify(&dev->kobj, NULL, "foo_notify");
	return sizeof(int);
}

static struct foo_attr foo_value = {
	.attr = __ATTR(foo_value, 0644, foo_show, foo_store),
	.value = 0,
};

static struct foo_attr foo_notify = {
	.attr = __ATTR(foo_notify, 0644, foo_show, foo_store),
	.value = 0,
};

static struct platform_device foo_device = {                                   
	.name = "foo",
	.id = -1,
	.dev.groups = foo_groups,
};                                                                              

static int __init foo_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = platform_device_register(&foo_device);
	if (ret < 0) {
		printk("%s: platform_device_register() failed. ret=%d\n",
				__func__, ret);
		ret = -1;
	}

	return ret;	/* 0=success */
}

static void __exit foo_exit(void)
{
	platform_device_unregister(&foo_device);

	printk("%s\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");
