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
#include <linux/irqdesc.h>                                              
#include <linux/interrupt.h>
#include <linux/of_device.h>


static struct foo_ctrl {
	struct platform_device *pdev;
	struct irq_data *foo_irqd;
} foo;


/**************************************************************************
 * driver 
 **************************************************************************/

static int check_irq_resend2(struct irq_data *irqd)
{
	if (irqd->chip->irq_retrigger)
		return irqd->chip->irq_retrigger(irqd);

	return 0;
}

static ssize_t trigger_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	if (foo.foo_irqd) {
		dev_info(&foo.pdev->dev, "try to trigger irq=%d\n", foo.foo_irqd->irq);

		check_irq_resend2(foo.foo_irqd);
	}

	return strlen(buf);
}

static DRIVER_ATTR_WO(trigger);
static struct attribute *foo_driver_attrs[] = {
	&driver_attr_trigger.attr,
	NULL
};
ATTRIBUTE_GROUPS(foo_driver);

static irqreturn_t foo_irq_handler(int irq, void *irq_data)
{
	dev_info(&foo.pdev->dev, "triggered irq=%d\n", irq);

	return IRQ_HANDLED;
}


static int get_and_map_resource(struct platform_device *pdev)
{
	int irq;
	int ret;

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

		printk(KERN_ERR "%s: devm_request_threaded_irq() failed. ret=%d\n", 
				__func__, ret);
		return ret;
	}

	foo.foo_irqd = irq_get_irq_data(irq);
	foo.pdev = pdev;

	return 0;
}

static int foo_probe(struct platform_device *pdev)
{
	int ret = 0;

	platform_set_drvdata(pdev, NULL);

	ret = get_and_map_resource(pdev);

	printk("%s ret=%d\n", __func__, ret);

	return 0;
}

static const struct of_device_id of_foo_match[] = {
	        { .compatible = "foo,foo-irq", },
		        {},
};
MODULE_DEVICE_TABLE(of, of_foo_match);

#if 0
static const struct platform_device_id foo2_ids[] = {
	{ "foo", },
	{ }
};
#endif

static struct platform_driver foo_irq_driver = {
	.driver = {
		.name = "foo-irq-driver",
		.groups = foo_driver_groups,
		.of_match_table = of_foo_match,
	},
	.probe          = foo_probe,
#if 0
	.id_table       = foo2_ids,
#endif
};

module_platform_driver(foo_irq_driver);
MODULE_LICENSE("GPL");


