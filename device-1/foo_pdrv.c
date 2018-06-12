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
#include <linux/interrupt.h>                                              
#include <linux/irqnr.h>                                              

static int drv1 = 0;

/**************************************************************************
 * driver 
 **************************************************************************/

static ssize_t drv1_show(struct device_driver *driver, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", drv1);
}

static ssize_t drv1_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &drv1);

	// sysfs_notify(&dev->kobj, NULL, "drv1");
	return sizeof(int);
}

static DRIVER_ATTR_RW(drv1);
static struct attribute *foo_driver_attrs[] = {
	&driver_attr_drv1.attr,
	NULL
};
ATTRIBUTE_GROUPS(foo_driver);

static irqreturn_t foo_irq_handler(int irq, void *irq_data)
{
	printk("%s irq=%d\n", __func__, irq);

	return IRQ_HANDLED;
}


static int get_and_map_resource(struct platform_device *pdev)
{
	int irq;
	int ret;
	struct irq_desc *desc;
	struct resource *res;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		printk(KERN_ERR "%s: platform_get_irq() failed. ret=%d\n", 
				__func__, irq);

		return irq;
	}

	dev_info(&pdev->dev, "resource-irq=%d\n", irq);

	ret = devm_request_threaded_irq(&pdev->dev, irq,
			NULL, foo_irq_handler,
			IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
			"foo-irq", pdev);
	if (ret != 0) {
		for_each_irq_desc(irq, desc) {
			printk("irq=%d, desc=%p\n", irq, desc);
		}

		printk(KERN_ERR "%s: devm_request_threaded_irq() failed. ret=%d\n", 
				__func__, ret);
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Missing resource #%d\n", 0);
		return -ENOENT;
	}

	dev_info(&pdev->dev, "resource-mem=0x%lx-0x%lx\n",
			(unsigned long)res->start,
			(unsigned long)res->end);
#if 0
	if (!devm_request_region(&pdev->dev, res->start,
				resource_size(res), "foo-mem")) {
		dev_err(&pdev->dev,
				"Failed to request region 0x%lx-0x%lx\n",
				(unsigned long)res->start,
				(unsigned long)res->end);
		return -EBUSY;
	}
#endif

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!res) {
		dev_err(&pdev->dev, "Missing resource #%d\n", 0);
		return -ENOENT;
	}

	dev_info(&pdev->dev, "resource-io=0x%lx-0x%lx\n",
			(unsigned long)res->start,
			(unsigned long)res->end);
#if 0
	if (!devm_request_region(&pdev->dev, res->start,
				resource_size(res), "foo-io")) {
		dev_err(&pdev->dev,
				"Failed to request region 0x%lx-0x%lx\n",
				(unsigned long)res->start,
				(unsigned long)res->end);
		return -EBUSY;
	}
#endif
	return 0;
}

static int foo2_probe(struct platform_device *pdev)
{
	int ret = 0;

	platform_set_drvdata(pdev, NULL);

	ret = get_and_map_resource(pdev);

	printk("%s ret=%d\n", __func__, ret);

	return 0;
}

#if 0
static const struct of_device_id of_foo_match[] = {
	        { .compatible = "foo,foo", },
		        {},
};
#endif

static const struct platform_device_id foo2_ids[] = {
	{ "foo", },
	{ }
};

static struct platform_driver foo2_driver = {
	.driver = {
		.name = "foo2-driver",
		.groups = foo_driver_groups,
		 // .of_match_table = of_foo_match,
	},
	.probe          = foo2_probe,
	.id_table       = foo2_ids,
};

module_platform_driver(foo2_driver);
MODULE_LICENSE("GPL");
