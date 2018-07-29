#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include "foo.h"

/*
 * please mknod /dev/foo c <major> <minor> before use this module 
 */
#define MAJOR_NUM	0
#define MINOR_NUM	0

struct foo_device { 
	struct cdev chardev;
};                                                                              

static struct foo_data _foo_data;


/****************************
 * cdev operation 
 */

static long foo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)  
{                                                                               
	struct foo_device *foo = filp->private_data;                          
	void __user *ip = (void __user *) arg;

	if (!foo)                                                              
		return -ENODEV;                                                 

	switch (cmd) {
		case IOCTL_FOO_GET_A :
			printk(KERN_INFO "%s: invoked=IOCTL_FOO_GET_A\n", 
					__func__);
			if (copy_to_user(ip, &_foo_data, sizeof(_foo_data)))
				return -EFAULT;
			break;
		case IOCTL_FOO_SET_A :
			printk(KERN_INFO "%s: invoked=IOCTL_FOO_SET_A\n", 
					__func__);
			if (copy_from_user(&_foo_data, ip, sizeof(_foo_data)))
				return -EFAULT;
			break;
		default:
			printk("%s: undefined ioctl. cmd=%d\n", __func__, cmd);
			break;
	}

	return 0;
}

static int foo_open(struct inode *inode, struct file *filp)
{
	struct foo_device *foo = container_of(inode->i_cdev,                  
			struct foo_device, chardev);      

	printk("%s\n", __func__);

	filp->private_data = foo;                                              

	return nonseekable_open(inode, filp); 
}

static int foo_release(struct inode *inode, struct file *filp)
{
	printk("%s\n", __func__);

	return 0;
}

static const struct file_operations foo_ops = {                            
	.release = foo_release,                                         
	.open = foo_open,
	.owner = THIS_MODULE,                                                   
	.llseek = no_llseek,                                                    
	.unlocked_ioctl = foo_ioctl,                                           
}; 


/****************************
 * module
 */

static int __init foo_init(void)
{
	struct foo_device *foo;
	dev_t devt;
	int count = 1;
	int major = MAJOR_NUM;
	int minor = MINOR_NUM;
	int ret;

	printk(KERN_INFO "%s: Try to load module.\n", __func__);

       	foo = kzalloc(sizeof(struct foo_device), GFP_KERNEL);
	if (!foo) {
		printk(KERN_ERR "%s: allocation failed.\n", __func__);
		return -ENOMEM;
	}

	if (major) {
		devt = MKDEV(MAJOR_NUM, MINOR_NUM);
		ret  = register_chrdev_region(devt, count, "foo");
	} else {
		ret = alloc_chrdev_region(&devt, MINOR_NUM, count, "foo");
		major = MAJOR(devt); 
	}
	if (ret < 0) {
		printk(KERN_ERR "%s: alloc chardev region failed. ret=%d\n", 
				__func__, ret);
		return -ENODEV;
	}

	cdev_init(&foo->chardev, &foo_ops);
	foo->chardev.owner = THIS_MODULE;
	foo->chardev.ops = &foo_ops;

	ret = cdev_add(&foo->chardev, devt, count);
	if (ret < 0) {
		printk(KERN_ERR "%s: cdev_add() failed. ret=%d\n", 
				__func__, ret);
		return -ENODEV;
	}

	printk(KERN_INFO "%s: Load module successed. major=%d, minor=%d\n", 
			__func__, major, minor);

	return 0;
}

static void __exit foo_exit(void)
{
	// cdev_device_del(&foo->chrdev, &gdev->dev);

	printk("%s: module unloaded\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");
