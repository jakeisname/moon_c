#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>

static struct kobject *foo_kobj;

struct foo_attr{
	struct kobj_attribute attr;
	int value;
};

static struct foo_attr foo_value;
static struct foo_attr foo_notify;

static struct attribute *foo_attrs[] = {
	&foo_value.attr.attr,
	&foo_notify.attr.attr,
	NULL
};

static struct attribute_group foo_group = {
	.attrs = foo_attrs,
};

static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr, 
		char *buf)
{
	struct foo_attr *foo = container_of(attr, struct foo_attr, attr);
	return scnprintf(buf, PAGE_SIZE, "%d\n", foo->value);
}

static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr, 
		const char *buf, size_t len)
{
	struct foo_attr *foo = container_of(attr, struct foo_attr, attr);

	sscanf(buf, "%d", &foo->value);
	sysfs_notify(foo_kobj, NULL, "foo_notify");
	return sizeof(int);
}

static struct foo_attr foo_value = {
	.attr = __ATTR(foo_value, 0644,	foo_show, foo_store),
	.value = 0,
};

static struct foo_attr foo_notify = {
	.attr = __ATTR(foo_notify, 0644, foo_show, foo_store),
	.value = 0,
};

static int __init foo_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	foo_kobj = kobject_create_and_add("foo", NULL);
	if (!foo_kobj) {
		printk("%s: kobject_create_and_add() failed\n", __func__);
		return -1;
	}

	ret = sysfs_create_group(foo_kobj, &foo_group);
	if (ret) 
		printk("%s: sysfs_create_group() failed. ret=%d\n", __func__, ret);

	return ret;	/* 0=success */
}

static void __exit foo_exit(void)
{
	if (foo_kobj) {
		kobject_put(foo_kobj);
	}

	printk("%s\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");
