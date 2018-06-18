#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/platform_device.h>                                              
#include <linux/export.h>                                              
#include <linux/property.h>                                                     

struct foo_device {
	int test1;
	struct device dev;
};

/**************************************************************************
 * a1 device attribute
 **************************************************************************/
int a1 = 0;

static ssize_t a1_show(struct device *dev, struct device_attribute *attr, 
		char *buf)
{
	struct foo_device *foo1 = container_of(dev, struct foo_device, dev);
	struct foo_device *foo2 = (struct foo_device *) dev_get_platdata(dev);

	printk(KERN_INFO "test1=%d, test1=%d\n", foo1->test1, foo2->test1);

	return scnprintf(buf, PAGE_SIZE, "%d\n", a1);
}

static ssize_t a1_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &a1);
	return sizeof(int);
}

/*----------------------------------------------------------------
 *  static struct device_attribute dev_attr_a1 = {
 *	.attr = { .name = "a1", .mode = 0664 },
 *	.show = a1_show,
 *	.store = a2_store,
 *  };
 * 
 *  same as below)
 *
 *  static struct device_attribute dev_attr_a1 = {
 *       __ATTR_RW(a1),
 *  };
 *---------------------------------------------------------------*/
static DEVICE_ATTR_RW(a1);

static struct attribute *foo_device_attrs[] = {
	&dev_attr_a1.attr,
	NULL
};

/*----------------------------------------------------------------
 *  static const struct attribute_group *foo_device_groups[] = {
 *        &foo_device_group,
 *        NULL,
 *  };
 *
 *  static const struct attribute_group foo_device_group = {
 *        .attrs = foo_device_attrs, 
 *  }; 
 *---------------------------------------------------------------*/
ATTRIBUTE_GROUPS(foo_device);


/**************************************************************************
 * foo device
 **************************************************************************/

static void foo_release(struct device *dev)                             
{ 
        printk("%s\n", __func__);                                               
} 

static struct foo_device foo = {
	.test1 = 1,
	.dev = {
		.init_name = "foo",
		.groups = foo_device_groups,
		.platform_data = &foo,
		.release = foo_release,
	},
};                                                                              

static int __init foo_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = device_register(&foo.dev);
	if (ret < 0) {
		printk("%s: device_register() failed. ret=%d\n",
				__func__, ret);
	}

	return ret;	/* 0=success */
}

static void __exit foo_exit(void)
{
	device_unregister(&foo.dev);

	printk("%s\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");

