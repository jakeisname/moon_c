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
	struct cdev cdev;
	dev_t devt;
	int count;
	int major;
	int minor;
};                                                                              

static struct foo_device  _foo_device = {
	.major = MAJOR_NUM,
	.minor = MINOR_NUM,
	.count = 1,
};

static struct foo_data _foo_data = {
	.a = 1,
	.b = 2,
};


/****************************
 * cdev operation 
 */

static long foo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)  
{                                                                               
	struct foo_device *foo = filp->private_data;                          
	void __user *ip = (void __user *) arg;

	printk(KERN_INFO "%s: cmd=0x%08x\n", __func__, cmd);
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
			printk(KERN_INFO "%s: undefined ioctl. cmd=%d\n", 
					__func__, cmd);
			break;
	}

	return 0;
}

static int foo_open(struct inode *inode, struct file *filp)
{
	struct foo_device *foo = container_of(inode->i_cdev,                  
			struct foo_device, cdev);      

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
	struct foo_device *foo = &_foo_device;
	int ret;

	printk(KERN_INFO "%s: Try to load module.\n", __func__);

	if (foo->major) {
		foo->devt = MKDEV(foo->major, foo->minor);
		ret  = register_chrdev_region(foo->devt, foo->count, "foo");
	} else {
		ret = alloc_chrdev_region(&foo->devt, foo->minor, 
				foo->count, "foo");
		foo->major = MAJOR(foo->devt); 
	}
	if (ret < 0) {
		printk(KERN_ERR "%s: alloc chardev region failed. ret=%d\n", 
				__func__, ret);
		return -ENODEV;
	}

	cdev_init(&foo->cdev, &foo_ops);
	foo->cdev.owner = THIS_MODULE;
	foo->cdev.ops = &foo_ops;

	ret = cdev_add(&foo->cdev, foo->devt, foo->count);
	if (ret < 0) {
		printk(KERN_ERR "%s: cdev_add() failed. ret=%d\n", 
				__func__, ret);
		ret = -ENODEV;
		goto err;
	}

	printk(KERN_INFO "%s: Load module successed. major=%d, minor=%d\n", 
			__func__, foo->major, foo->minor);

	return 0;
err:
	unregister_chrdev_region(foo->devt, foo->count);

	return ret;
}

static void __exit foo_exit(void)
{
	struct foo_device *foo = &_foo_device;

	cdev_del(&foo->cdev);
	unregister_chrdev_region(foo->devt, foo->count);

	printk("%s: module unloaded\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");
