
/* ref: https://lkw.readthedocs.io/en/latest/doc/05_proc_interface.html 
 *
 * tested on kernel v5.15 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/types.h>
#include <linux/version.h>


#define DATA_SIZE 10
#define MY_PROC_ENTRY1 "foo1"	/* string */
#define MY_PROC_ENTRY2 "foo2"	/* value */

static int foo1_len;
static char *foo1_str = NULL;

static ssize_t foo1_write(struct file *filp, 
		const char __user * buffer, size_t count, loff_t *pos)
{
	char *data = PDE_DATA(file_inode(filp));

	printk(KERN_INFO "%s(%d): count=%lu, pos=%lld, old foo_len=%d\n", 
			__func__, __LINE__, count, *pos, foo1_len);

	if (count > DATA_SIZE) {
		return -EFAULT;
	}

	if (copy_from_user(data, buffer, count)) {
		return -EFAULT;
	}

	data[count-1] = '\0';

	printk(KERN_INFO "set new foo=%s", foo1_str);

	*pos = (int) count;
	foo1_len = count - 1;

	printk(KERN_INFO "%s(%d): count=%lu, pos=%lld, new foo_len=%d\n", 
			__func__, __LINE__, count, *pos, foo1_len);
	return count;
}

static ssize_t foo1_read(struct file *filp, char __user *buf, 
		size_t count, loff_t *offp)
{
	int err;
	char *data = PDE_DATA(file_inode(filp));

	printk(KERN_INFO "%s(%d): count=%lu, offp=%lld, foo_len=%d\n", 
			__func__, __LINE__, count, *offp, foo1_len);

	if ((int) (*offp) > foo1_len) {
		return 0;
	}

	if (!(data)) {
		return 0;
	}

	if (count == 0) {
		return count;	/* read 0 size, nothing to do */
	} 

	count = foo1_len + 1;	/* +1 to read the \0 */
	err = copy_to_user(buf, data, count);	/* +1 for \0 */
	printk(KERN_INFO "get foo=%s\n", buf);
	*offp = count;

	if (err) {
		printk(KERN_INFO "Error in copying data.");
	}

	printk(KERN_INFO "%s(%d): count=%lu, offp=%lld, foo_len=%d\n", 
			__func__, __LINE__, count, *offp, foo1_len);

	return count;
}

static int foo2 = 0;
static int foo2_len;
static char *foo2_str = NULL;

static ssize_t foo2_write(struct file *filp, 
		const char __user * buffer, size_t count, loff_t *pos)
{
	char *data = PDE_DATA(file_inode(filp));
	char tmp[100];

	printk(KERN_INFO "%s(%d): count=%lu, pos=%lld, old foo2_len=%d\n", 
			__func__, __LINE__, count, *pos, foo2_len);

	if (count > DATA_SIZE) {
		return -EFAULT;
	}

	if (copy_from_user(data, buffer, count)) {
		return -EFAULT;
	}

	data[count-1] = '\0';

	sscanf(data, "%d", &foo2);
	foo2_len = sprintf(tmp, "%d\n", foo2);
	foo2_len--;

	printk(KERN_INFO "set new foo2=%d", foo2);

	*pos = (int) count;

	printk(KERN_INFO "%s(%d): count=%lu, pos=%lld, new foo2_len=%d\n", 
			__func__, __LINE__, count, *pos, foo2_len);
	return count;
}

static ssize_t foo2_read(struct file *filp, char __user *buf, 
		size_t count, loff_t *offp)
{
	int err;
	char *data = PDE_DATA(file_inode(filp));
	int temp_len;

	printk(KERN_INFO "%s(%d): count=%lu, offp=%lld, foo2_len=%d\n", 
			__func__, __LINE__, count, *offp, foo2_len);

	if ((int) (*offp) > foo2_len) {
		return 0;
	}

	if (!(data)) {
		return 0;
	}

	if (count == 0) {
		return count;	/* read 0 size, nothing to do */
	} 

	temp_len = sprintf(data, "%d\n", foo2);

	count = temp_len + 1;	/* +1 to read the \0 */
	err = copy_to_user(buf, data, count);	/* +1 for \0 */

	printk(KERN_INFO "get foo2=%d\n", foo2);
	*offp = count;

	if (err) {
		printk(KERN_INFO "Error in copying data.");
	}

	printk(KERN_INFO "%s(%d): count=%lu, offp=%lld, foo2_len=%d\n", 
			__func__, __LINE__, count, *offp, foo2_len);

	return count;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static struct proc_ops foo1_proc_fops = {
	.proc_read = foo1_read,
	.proc_write = foo1_write,
};
static struct proc_ops foo2_proc_fops = {
	.proc_read = foo2_read,
	.proc_write = foo2_write,
};
#else
static struct file_operations foo1_proc_fops = {
	.read = foo1_read,
	.write = foo1_write,
};
static struct file_operations foo2_proc_fops = {
	.read = foo2_read,
	.write = foo2_write,
};
#endif


static int create_new_proc_entry(void) 
{
	struct proc_dir_entry *proc1;
	struct proc_dir_entry *proc2;
	char *DATA1 = "foo";
	char *DATA2 = "0";

	foo1_len = strlen(DATA1);
	foo1_str = kmalloc((size_t) DATA_SIZE, GFP_KERNEL);
	if (foo1_str == NULL) {
		goto err4;
	}

	strncpy(foo1_str, DATA1, foo1_len + 1);

	proc1 = proc_create_data(MY_PROC_ENTRY1, 0666, NULL, 
			&foo1_proc_fops, foo1_str);
	if (proc1 == NULL) {
		goto err3;
	}

	foo2_len = strlen(DATA2);
	foo2_str = kmalloc((size_t) DATA_SIZE, GFP_KERNEL);
	if (foo2_str == NULL) {
		goto err2;
	}

	strncpy(foo2_str, DATA2, foo2_len + 1);

	proc2 = proc_create_data(MY_PROC_ENTRY2, 0666, NULL, 
			&foo2_proc_fops, foo2_str);
	if (proc2 == NULL) {
		goto err1;
	}

	return 0;

err1:	
	kfree(foo2_str);
err2:
	remove_proc_entry(MY_PROC_ENTRY1, NULL);
err3:
	kfree(foo1_str);
err4:
	return -1;
}


int foo_init (void) 
{
	if (create_new_proc_entry()) {
		return -1;
	}

	return 0;
}

void foo_exit(void) 
{
	remove_proc_entry(MY_PROC_ENTRY1, NULL);
	remove_proc_entry(MY_PROC_ENTRY2, NULL);
}

MODULE_LICENSE("GPL");
module_init(foo_init);
module_exit(foo_exit);

