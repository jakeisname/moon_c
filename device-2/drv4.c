#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/platform_device.h>                                              
#include <linux/export.h>                                              
#include <linux/property.h>                                                     
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/err.h>

/**************************************************************************
 * d4 device attribute
 **************************************************************************/
int d4 = 0;

static ssize_t d4_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", d4);
}

static ssize_t d4_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &d4);
	return sizeof(int);
}

static DRIVER_ATTR_RW(d4);
static struct attribute *drv4_driver_attrs[] = {
	&driver_attr_d4.attr,
	NULL
};
ATTRIBUTE_GROUPS(drv4_driver);


/**************************************************************************
 * drv4 driver
 **************************************************************************/

struct foo_data {
	struct platform_device *dev;
	int irq;
	void __iomem *base;
	u32 foo_val32;
	const char *foo_string;
};

static irqreturn_t foo_irq_handler(int irq, void *data)
{
        printk("%s\n", __func__);

	return IRQ_HANDLED;
}

static int drv4_probe(struct platform_device *pdev)
{ 
	int ret = 0;
	struct foo_data *foo;
	struct resource *res;

        printk("%s\n", __func__);

	foo = devm_kzalloc(&pdev->dev, sizeof(*foo), GFP_KERNEL);
	if (!foo)                                                         
		return -ENOMEM; 

	foo->dev = pdev;

	/* get platform resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "platform mem resources not found.\n");
		return -ENOENT;
	}

        dev_info(&pdev->dev, "iomem resource. start=0x%lx, size=0x%lx\n", 
			(long unsigned int) res->start, 
			(long unsigned int) (res->end - res->start + 1));

	/* ioremap */
	foo->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(foo->base)) {
		dev_err(&pdev->dev, "devm_ioremap_resource() failed.\n");
		return -ENOMEM;
	}
	
        dev_info(&pdev->dev, "iomem resource. base=0x%p\n", foo->base);

	/* get irq */
	foo->irq = platform_get_irq(pdev, 0);
	if (foo->irq < 0) {
		dev_err(&pdev->dev, "platform irq resources not found.\n");
		return foo->irq;
	}
	dev_info(&pdev->dev, "irq=%d\n", foo->irq);

	/* request irq */
	ret = devm_request_threaded_irq(&pdev->dev, foo->irq,
			NULL, foo_irq_handler,
			IRQF_SHARED | IRQF_ONESHOT,                             
			"drv4", foo);
	if (ret) {
		dev_err(&pdev->dev, 
				"devm_request_threaded_irq() failed. err=%d\n",
				ret);
		return ret;
	}
	
	dev_info(&pdev->dev, "devm_request_threaded_irq() irq=%d, ret=%d\n",
			foo->irq, ret); 

	/* get foo-prop-val32 */
	ret = of_property_read_u32(foo->dev->dev.of_node, 
			"foo-prop-val32", &foo->foo_val32); 
        dev_info(&pdev->dev, "foo_val32=%u, ret=%d\n", foo->foo_val32, ret);

	/* get foo-prop-string */
	ret = of_property_read_string(foo->dev->dev.of_node, 
			"foo-prop-string", &foo->foo_string);
        dev_info(&pdev->dev, "foo_string=%s, ret=%d\n", foo->foo_string, ret);

	return 0;
} 

static const struct platform_device_id drv4_id_table[] = {                      
	{ "foo4", 0 },
	{ },
};                                                                              

static const struct of_device_id drv4_of_match_table[] = { 
    { 
        .compatible = "foo,foo4", 
    }, 
    { /* sentinel */ }, 
}; 
MODULE_DEVICE_TABLE(of, drv4_of_match_table);

static struct platform_driver drv4 = {
	.probe = drv4_probe,
	.id_table = drv4_id_table,
	.driver = {
		.name = "drv4",
		.groups = drv4_driver_groups,
		.of_match_table = drv4_of_match_table,
	},
};                                                                              

module_platform_driver(drv4);
MODULE_LICENSE("GPL");

