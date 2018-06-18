#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/platform_device.h>                                              
#include <linux/export.h>                                              
#include <linux/property.h>                                                     


/**************************************************************************
 * d3 device attribute
 **************************************************************************/
int d3 = 0;

static ssize_t d3_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", d3);
}

static ssize_t d3_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &d3);
	return sizeof(int);
}

static DRIVER_ATTR_RW(d3);
static struct attribute *drv3_driver_attrs[] = {
	&driver_attr_d3.attr,
	NULL
};
ATTRIBUTE_GROUPS(drv3_driver);


/**************************************************************************
 * drv3 driver
 **************************************************************************/

static int drv3_probe(struct platform_device *pdev)
{ 
        printk("%s\n", __func__);                                               

	return 0;
} 

static const struct platform_device_id drv3_id_table[] = {                      
	{ "foo3",    (unsigned long) &foo3_id },
	{ },
};                                                                              

static const struct of_device_id drv3_of_match_table[] = { 
    { 
        .compatible = "foo3,foo3", 
    }, 
    { /* sentinel */ }, 
}; 
MODULE_DEVICE_TABLE(of, drv3_of_match_table);

static struct platform_driver foo_driver = {                               
	.probe      = foo_probe,                                               
	.driver     = {
		.name   = "foo",
		.of_match_table = of_match_ptr(foo_ids),
	},                                                                          
	.id_table   = foo_id_table,                                                
};

static struct platform_driver drv3 = {
	.probe = drv3_probe,
	.id_table = drv3_id_table,
	.driver = {
		.name = "drv3"
		.of_match_table = drv3_of_match_table,
	},
};                                                                              

module_platform_driver(drv3);
MODULE_LICENSE("GPL");

