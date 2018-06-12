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
#include <linux/export.h>                                              


#include <linux/property.h>                                                     
#include <linux/pm_runtime.h>                                                   
#include <linux/pm_domain.h>                                                    
#include <linux/limits.h>                                                       

#include "foo.h"


static const struct attribute_group *foo_class_groups[];

static struct device *new_dev = NULL;
struct device *foo0_dev = NULL;

/* for foo1.ko */
EXPORT_SYMBOL(foo0_dev);

/**************************************************************************
 * export & unexport class attribute
 **************************************************************************/
int export = -1;

void export_device(struct device *dev, int export);

static ssize_t export_show(struct class *class, struct class_attribute *attr, 
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", export);
}

static ssize_t export_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct device *dev = foo0_dev;

	if (export != -1) {
		printk(KERN_ERR "class device is created already");
		return sizeof(int);
	}
		
	sscanf(buf, "%d", &export);

	export_device(dev, export);

	// sysfs_notify(&dev->kobj, NULL, "c1");
	return sizeof(int);
}

static ssize_t unexport_show(struct class *class, struct class_attribute *attr, 
		char *buf)
{

	return scnprintf(buf, PAGE_SIZE, "%d\n", export);
}

static ssize_t unexport_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t len)
{
	if (export == -1) {
		printk(KERN_ERR "class device was not created");
		return sizeof(int);
	}
	
	export = -1;

	put_device(new_dev);
	device_unregister(new_dev);

	// sysfs_notify(&dev->kobj, NULL, "c1");
	return sizeof(int);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0))
static CLASS_ATTR_RW(export);
static CLASS_ATTR_RW(unexport);

static struct attribute *foo_class_attrs[] = {
	&class_attr_export.attr,
	&class_attr_unexport.attr,
	NULL
};

ATTRIBUTE_GROUPS(foo_class);
#else 
static struct class_attribute foo_class_attrs[] = {
	__ATTR_RW(export),
	__ATTR_RW(unexport),
	__ATTR_NULL,
};
#endif

/**************************************************************************
 * class_dev device attribute
 **************************************************************************/
static int class_dev = 0;

static ssize_t class_dev_show(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", class_dev);
}

static ssize_t class_dev_store(struct device *dev, 
		struct device_attribute *attr, const char *buf, size_t len)
{
	sscanf(buf, "%d", &class_dev);
	return sizeof(int);
}

static DEVICE_ATTR_RW(class_dev);
static struct attribute *class_dev_attrs[] = {
	&dev_attr_class_dev.attr,
	NULL
};
ATTRIBUTE_GROUPS(class_dev);


/**************************************************************************
 * foo_class
 **************************************************************************/
static void foo_classdev_release(struct device *dev)
{
	printk("%s\n", __func__);
}

struct class foo_class = {                                   
	.name = "foo_class",
	.owner = THIS_MODULE,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0))
	.class_groups = foo_class_groups,
#else
	.class_attrs = foo_class_attrs,
#endif
	.dev_groups = class_dev_groups,
	.dev_release = foo_classdev_release,
};                                                                              
EXPORT_SYMBOL(foo_class);

void export_device(struct device *dev, int export)
{
	new_dev = device_create_with_groups(&foo_class, dev,
			MKDEV(0, 0), NULL, NULL,
			"foo%d", export);
}

/**************************************************************************
 * bus_bus bus attribute
 **************************************************************************/
static int bus_bus = 0;

static ssize_t bus_bus_show(struct bus_type *bus, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", bus_bus);
}

static ssize_t bus_bus_store(struct bus_type *bus, 
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &bus_bus);

	return sizeof(int);
}
static BUS_ATTR_RW(bus_bus);

static struct attribute *bus_bus_attrs[] = {
	&bus_attr_bus_bus.attr,
	NULL
};
ATTRIBUTE_GROUPS(bus_bus);

/**************************************************************************
 * bus_dev device attribute
 **************************************************************************/
static int bus_dev = 0;

static ssize_t bus_dev_show(struct device *dev, struct device_attribute *attr, 
	char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", bus_dev);
}

static ssize_t bus_dev_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &bus_dev);

	return sizeof(int);
}
static DEVICE_ATTR_RW(bus_dev);

static struct attribute *bus_dev_attrs[] = {
	&dev_attr_bus_dev.attr,
	NULL
};
ATTRIBUTE_GROUPS(bus_dev);

/**************************************************************************
 * bus_drv driver attribute
 **************************************************************************/
static int bus_drv = 0;

static ssize_t bus_drv_show(struct device_driver *drv, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", bus_drv);
}

static ssize_t bus_drv_store(struct device_driver *drv, 
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &bus_drv);

	return sizeof(int);
}
static DRIVER_ATTR_RW(bus_drv);

static struct attribute *bus_drv_attrs[] = {
	&driver_attr_bus_drv.attr,
	NULL
};
ATTRIBUTE_GROUPS(bus_drv);

/**************************************************************************
 * foo_bus
 **************************************************************************/
int foo_bus_match(struct device *dev, struct device_driver *drv)
{  
	struct foo_device *pdev;
	struct foo_driver *pdrv;
	int match;

	match = (!strcmp(dev->kobj.name, "foo-client1-device") && 
			!strcmp(drv->name, "foo-client1-driver"));
	printk("dev=%s, drv=%s name %s\n", dev->kobj.name, drv->name, 
			match ? "matched" : "skip");

	if (match)
		return 1;

	pdev = to_foo_device(dev);                     
	pdrv = to_foo_driver(drv);                     

	printk("pdev=%p, pdrv=%p\n", pdev, pdrv);
	match = (pdev->match_id == pdrv->match_id);
	printk("pdev->match_id=%d, pdrv->match_id=%d id %s\n", 
			pdev->match_id, pdrv->match_id, match ? "matched" : "skip");

	return match;
}

struct bus_type foo_bus = {                                   
	.name = "foo_bus",
	.bus_groups = bus_bus_groups,
	.dev_groups = bus_dev_groups,
	.drv_groups = bus_drv_groups,
	.match = foo_bus_match,
};                                                                              

EXPORT_SYMBOL(foo_bus);

static int __init foo0_init(void)
{
	int ret = 0;

	printk("%s\n", __func__);

	ret = class_register(&foo_class);
	if (ret < 0) {
		printk("%s: class_register() failed. ret=%d\n",
				__func__, ret);
		ret = -1;
	}

	ret = bus_register(&foo_bus);
	if (ret < 0) {
		printk("%s: bus_register() failed. ret=%d\n",
				__func__, ret);
		ret = -1;
	}

	return ret;	/* 0=success */
}

static void __exit foo0_exit(void)
{
	bus_unregister(&foo_bus);
	class_unregister(&foo_class);

	printk("%s\n", __func__);
}

module_init(foo0_init);
module_exit(foo0_exit);
MODULE_LICENSE("GPL");


/**************************************************************************     
 * support foo_device & foo_driver API
 **************************************************************************/    

/* platform_device_add */
int foo_device_add(struct foo_device *pdev)                           
{                                                                               
    int ret;                                                                 
                                                                                
    if (!pdev)                                                                  
        return -EINVAL;                                                         
                                                                                
    pdev->dev.bus = &foo_bus;   
	dev_set_name(&pdev->dev, "%s", pdev->name);

	ret = device_add(&pdev->dev);                                               

	return ret;
}
EXPORT_SYMBOL_GPL(foo_device_add); 

void foo_device_del(struct foo_device *pdev)                          
{                                                                               
    if (pdev) {                                                                 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0))
        device_remove_properties(&pdev->dev);                                   
#endif
        device_del(&pdev->dev);                                                 
    }                                                                           
}                                                                               
EXPORT_SYMBOL_GPL(foo_device_del); 

void foo_device_put(struct foo_device *pdev)                          
{                                                                               
    if (pdev)                                                                   
        put_device(&pdev->dev);                                                 
}                                                                               
EXPORT_SYMBOL_GPL(foo_device_put); 


/* platform_device_register */
int foo_device_register(struct foo_device *pdev)                      
{                                                                               
    device_initialize(&pdev->dev);                                              
    return foo_device_add(pdev);                                           
}                                                                               
EXPORT_SYMBOL_GPL(foo_device_register);                                    
                                                                                
void foo_device_unregister(struct foo_device *pdev)                   
{                                                                               
    foo_device_del(pdev);                                                  
    foo_device_put(pdev);                                                  
}                                                                               
EXPORT_SYMBOL_GPL(foo_device_unregister);
                                                                                
static int foo_drv_probe(struct device *_dev)                              
{                                                                               
    struct foo_driver *drv = to_foo_driver(_dev->driver);             
    struct foo_device *dev = to_foo_device(_dev);                     
    int ret;                                                                    

    ret = dev_pm_domain_attach(_dev, true);                                     
    if (ret != -EPROBE_DEFER) {                                                 
        if (drv->probe) {                                                       
            ret = drv->probe(dev);                                              
            if (ret)                                                            
                dev_pm_domain_detach(_dev, true);                               
        } else {                                                                
            /* don't fail if just dev_pm_domain_attach failed */                
            ret = 0;                                                            
        }                                                                       
    }                                                                           
                                                                                
    return ret;                                                                 
} 

static int foo_drv_remove(struct device *_dev)                             
{                                                                               
    struct foo_driver *drv = to_foo_driver(_dev->driver);             
    struct foo_device *dev = to_foo_device(_dev);                     
    int ret = 0;                                                                
                                                                                
    if (drv->remove)                                                            
        ret = drv->remove(dev);                                                 
    dev_pm_domain_detach(_dev, true);                                           
                                                                                
    return ret;                                                                 
}                                                                               
                                                                                
static void foo_drv_shutdown(struct device *_dev)                          
{                                                                               
    struct foo_driver *drv = to_foo_driver(_dev->driver);             
    struct foo_device *dev = to_foo_device(_dev);                     
                                                                                
    if (drv->shutdown)                                                          
        drv->shutdown(dev);                                                     
}   

int foo_driver_register(struct foo_driver *drv,                     
		struct module *owner)                                           
{                                                                               
	drv->driver.owner = owner;                                                  
	drv->driver.bus = &foo_bus;                                      
	drv->driver.probe = foo_drv_probe;                                     
	drv->driver.remove = foo_drv_remove;                                   
	drv->driver.shutdown = foo_drv_shutdown;                               

	return driver_register(&drv->driver);                                       
}                                                                               
EXPORT_SYMBOL_GPL(foo_driver_register);                                  

void foo_driver_unregister(struct foo_driver *drv)                    
{                                                                               
	driver_unregister(&drv->driver);                                            
}                                                                               
EXPORT_SYMBOL_GPL(foo_driver_unregister); 
