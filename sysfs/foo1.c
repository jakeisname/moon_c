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
	struct attribute attr;
	int value;
};

static struct foo_attr foo_value = {
	.attr.name="foo_value",
	.attr.mode = 0644,
	.value = 0,
};

static struct foo_attr foo_notify = {
	.attr.name="foo_notify",
	.attr.mode = 0644,
	.value = 0,
};

static struct attribute * foo_attrs[] = {
	&foo_value.attr,
	&foo_notify.attr,
	NULL
};

static ssize_t foo_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct foo_attr *foo = container_of(attr, struct foo_attr, attr);
	return scnprintf(buf, PAGE_SIZE, "%d\n", foo->value);
}

static ssize_t foo_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t len)
{
	struct foo_attr *foo = container_of(attr, struct foo_attr, attr);

	sscanf(buf, "%d", &foo->value);
	foo_value.value = foo->value;
	sysfs_notify(foo_kobj, NULL, "foo_notify");
	return sizeof(int);
}

static struct sysfs_ops foo_ops = {
	.show = foo_show,
	.store = foo_store,
};

static struct kobj_type foo_type = {
	.sysfs_ops = &foo_ops,
	.default_attrs = foo_attrs,
};

static int __init foo_init(void)
{
	int ret = 0;

	foo_kobj = kzalloc(sizeof(*foo_kobj), GFP_KERNEL);
	if (!foo_kobj)
	{
		printk("%s: kzalloc() failed\n", __func__);
		return -1;
	}

	kobject_init(foo_kobj, &foo_type);
	ret = kobject_add(foo_kobj, NULL, "%s", "foo");
	if (ret) {
		printk("kobject_add() failed. ret=%d\n", ret);
		kobject_put(foo_kobj);
		foo_kobj = NULL;
	}

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
