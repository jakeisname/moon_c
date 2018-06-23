#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/platform_device.h>                                              
#include <linux/export.h>                                              
#include <linux/property.h>                                                     


/**************************************************************************
 * a3 device attribute
 **************************************************************************/
int a3 = 0;

static ssize_t a3_show(struct device *dev, struct device_attribute *attr, 
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", a3);
}

static ssize_t a3_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &a3);
	return sizeof(int);
}

static DEVICE_ATTR_RW(a3);
static struct attribute *foo3_device_attrs[] = {
	&dev_attr_a3.attr,
	NULL
};
ATTRIBUTE_GROUPS(foo3_device);


/**************************************************************************
 * foo device
 **************************************************************************/

static void foo3_release(struct device *dev)
{ 
        printk("%s\n", __func__);                                               
} 

static struct resource foo_resource[] = {
	{	
	 .start = 0xf000e000,                                                    
	 .end = 0xf000e000 + 0x100,
	 .flags = IORESOURCE_MEM,                                                
	},
	{	
	 .start = 20,
	 .end = 20,
	 .flags = IORESOURCE_IRQ,                                                
	},
}; 

static struct platform_device foo3 = {
	.name = "foo3",
	.id = -1,	/* id=0  -> device name is foo3.0 */
			/* id=-1 -> device name is foo3   */
	.resource = foo_resource,
	.num_resources = ARRAY_SIZE(foo_resource),
	.dev = {
		.groups = foo3_device_groups,
		.platform_data = &foo3,
		.release = foo3_release,
	},
};                                                                              


static int __init foo3_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = platform_device_register(&foo3);
	if (ret < 0) {
		printk("%s: device_register() failed. ret=%d\n",
				__func__, ret);
	}

	return ret;	/* 0=success */
}

static void __exit foo3_exit(void)
{
	platform_device_unregister(&foo3);

	printk("%s\n", __func__);
}

module_init(foo3_init);
module_exit(foo3_exit);
MODULE_LICENSE("GPL");

