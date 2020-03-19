#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>                                              
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>

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
static void foo_client1_release(struct device *dev)
{
	printk("%s\n", __func__);
}

static struct device sub_device = { 
	.init_name = "foo-client1-device",
	.groups = sub_dev_groups,
	.bus = &foo_bus,
	.parent = &foo_platform_device.dev,
	.release = foo_client1_release,
};                                                                              


dma_addr_t phys1;
dma_addr_t phys2;
dma_addr_t phys3;
dma_addr_t phys4;
struct page *p1;
void *p2;
void *p3;
void *p4;

/****************************
 * define driver 
 ****************************/
static int foo_client1_probe(struct device *dev)
{
	int ret = 0;
	int pages = 32;	/* 4K * 32 = 128K */

	printk("%s ret=%d\n", __func__, ret);
	
//	p1 = dma_alloc_contiguous(dev, pages, GFP_KERNEL);
//	dev_info(dev, "p1=%llx\n", (long long) (void *) p1);

	p2 = dmam_alloc_coherent(dev, pages, &phys2, GFP_KERNEL);
	dev_info(dev, "p2=%llx\n", (long long) p2);
	
	p3 = dmam_alloc_attrs(dev, pages, &phys3, GFP_KERNEL, DMA_ATTR_WRITE_COMBINE);
	dev_info(dev, "p3=%llx\n", (long long) p3);

	p4 = dmam_alloc_attrs(dev, pages, &phys4, GFP_KERNEL, DMA_ATTR_NO_KERNEL_MAPPING);
	dev_info(dev, "p4=%llx\n", (long long) p4);

	return 0;
}

static struct device_driver sub_driver = {
	.name = "foo-client1-driver",
	.owner = THIS_MODULE,
	.probe  = foo_client1_probe,
	.groups = sub_drv_groups,
	.bus = &foo_bus,
};

/****************************
 * module
 ****************************/
static int __init foo_client1_init(void)
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

static void __exit foo_client1_exit(void)
{
	driver_unregister(&sub_driver);
	device_unregister(&sub_device);

//	p2 = dma_free_coherent(dev, pages, &phys, GFP_KERNEL);
	
//	p3 = dma_free_attrs(dev, pages, p3, phys, GFP_KERNEL, DMA_ATTR_WRITE_COMBINE);

//	p4 = dma_free_attrs(dev, pages, p4, phys, GFP_KERNEL, DMA_ATTR_NO_KERNEL_MAPPING);
}

module_init(foo_client1_init);
module_exit(foo_client1_exit);
MODULE_LICENSE("GPL");
