
/* ref: https://lkw.readthedocs.io/en/latest/doc/05_proc_interface.html 
 *
 * tested on kernel v5.15 
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/types.h>
#include <linux/version.h>


#define BUF_SIZE	100
#define FOO1_ENTRY	"foo1"	/* string */
#define FOO2_ENTRY	"foo2"	/* value */

static char *foo1_str = NULL;
static int foo1_len;

static ssize_t proc_str_write(struct file *filp, 
		const char __user *ubuf, size_t count, loff_t *pos,
		char *dest, int *str_len)
{
	char *data = PDE_DATA(file_inode(filp));

	pr_info("%s(%d): count=%lu, pos=%lld, old str_len=%d\n", 
			__func__, __LINE__, count, *pos, *str_len);

	if (count > BUF_SIZE) {
		return -EFAULT;
	}

	if (copy_from_user(data, ubuf, count)) {
		return -EFAULT;
	}

	data[count] = '\0';

	pr_info("set new val=%s", data);

	*str_len = count - 1;
	*pos = (int) count;

	pr_info("%s(%d): count=%lu, pos=%lld, new str_len=%d\n", 
			__func__, __LINE__, count, *pos, *str_len);
	return count;
}

static ssize_t proc_str_read(struct file *filp, char __user *ubuf, 
		size_t count, loff_t *offp,
		char *dest, int str_len)
{
	int err;
	char *data = PDE_DATA(file_inode(filp));
	int len;

	pr_info("%s(%d): count=%lu, offp=%lld, str_len=%d\n", 
			__func__, __LINE__, count, *offp, str_len);

	if ((int) (*offp) > str_len) {
		return 0;
	}

	if (!(data)) {
		return 0;
	}

	if (count == 0) {
		return count;	/* read 0 size, nothing to do */
	} 

	len = str_len + 1;	/* +1 to read the \0 */
	err = copy_to_user(ubuf, data, len);	/* +1 for \0 */
	if (err) {
		pr_info("Error in copying data.");
	}

	*offp = len;

	return len;
}

static ssize_t foo1_write(struct file *filp, 
		const char __user *ubuf, size_t count, loff_t *pos)
{
	return proc_str_write(filp, ubuf, count, pos, foo1_str, &foo1_len);
}

static ssize_t foo1_read(struct file *filp, char __user *buf, 
		size_t count, loff_t *offp)
{
	return proc_str_read(filp, buf, count, offp, foo1_str, foo1_len);
}


static char *foo2_str = NULL;
static int foo2_int = 0;
static int foo2_len;

static ssize_t proc_int_write(struct file *filp, 
		const char __user *ubuf, size_t count, loff_t *pos,
		int *dest, int *val_len)
{
	int num;
	char *data = PDE_DATA(file_inode(filp));
	int len;

	pr_info("%s(%d): count=%lu, pos=%lld, old val_len=%d\n", 
			__func__, __LINE__, count, *pos, *val_len);

	if (count > BUF_SIZE) {
		return -EFAULT;
	}

	if (copy_from_user(data, ubuf, count)) {
		return -EFAULT;
	}

	data[count-1] = '\0';

	num = sscanf(data, "%d", dest);
	if (num != 1)
		return -EFAULT;

	len = sprintf(data, "%d\n", *dest);
	data[len] = 0;

	pr_info("set new val=%d\n", *dest);

	*val_len = len - 1;

	*pos = (loff_t) len;

	pr_info("%s(%d): count=%lu, pos=%lld, new val_len=%d\n", 
			__func__, __LINE__, count, *pos, *val_len);

	return len;
}

static ssize_t foo2_write(struct file *filp, 
		const char __user *ubuf, size_t count, loff_t *pos)
{
	return proc_int_write(filp, ubuf, count, pos, &foo2_int, &foo2_len);
}

static ssize_t proc_int_read(struct file *filp, char __user *ubuf, 
		size_t count, loff_t *offp, int val, int val_len)
{
	char *data = PDE_DATA(file_inode(filp));
	ssize_t size;
	int n;

	pr_info("%s(%d): count=%lu, offp=%lld, val_len=%d\n", 
			__func__, __LINE__, count, *offp, val_len);

	if ((int) (*offp) > val_len) {
		return 0;
	}

	if (data == NULL) {
		return 0;
	}

	if (count == 0) {
		return count;	/* read 0 size, nothing to do */
	} 

	size = sprintf(data, "%d\n", val);
	size++;

	n = copy_to_user(ubuf, data, size);	/* +1 for \0 */
	if (n) {
		pr_err("copy_to_use() failed\n");
		return -EFAULT;
	}

	*offp = size;

	return size;
}

static ssize_t foo2_read(struct file *filp, char __user *buf, 
		size_t count, loff_t *offp)
{
	return proc_int_read(filp, buf, count, offp, foo2_int, foo2_len);
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


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static int proc_create_str_and_init(char *name, umode_t mode,
		struct proc_dir_entry *dir, const struct proc_ops *ops,
		char *init_value, int *val_len, char *data_str)
#else
static int proc_create_str_and_init(char *name, umode_t mode,
		struct proc_dir_entry *dir, const struct file_operations *ops,
		char *init_value, int *val_len, char *data_str)
#endif
{
	struct proc_dir_entry *f1;

	pr_err("%s(%d): Jake\n", __func__, __LINE__);
	data_str = kmalloc((size_t) BUF_SIZE, GFP_KERNEL);
	if (data_str == NULL) {
		goto err2;
	}

	if (init_value) {
		*val_len = strlen(init_value);
		strncpy(data_str, init_value, *val_len + 1);
	} else {
		*val_len = 0;
		data_str[0] = 0;
	}

	f1 = proc_create_data(name, mode, dir, ops, data_str);
	if (!f1) {
		pr_err("Failed to create %s\n", name);
		goto err;
	}

	return 0;

err:
	kfree(data_str);

err2:
	return -1;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static int proc_create_int_and_init(char *name, umode_t mode,
		struct proc_dir_entry *dir, const struct proc_ops *ops,
		int init_value, int *val_len, char *data_str)
#else
static int proc_create_int_and_init(char *name, umode_t mode,
		struct proc_dir_entry *dir, const struct file_operations *ops,
		int init_value, int *val_len, char *data_str)
#endif
{
	struct proc_dir_entry *f1;

	pr_err("%s(%d): Jake\n", __func__, __LINE__);
	data_str = kmalloc((size_t) BUF_SIZE, GFP_KERNEL);
	if (data_str == NULL) {
		goto err2;
	}

	*val_len = sprintf(data_str, "%d", init_value);
	data_str[*val_len] = 0;

	f1 = proc_create_data(name, mode, dir, ops, data_str);
	if (!f1) {
		pr_err("Failed to create %s\n", name);
		goto err;
	}

	return 0;

err:
	kfree(data_str);

err2:
	return -1;
}


int create_new_proc_entry(void) 
{
	int ret;

	ret = proc_create_str_and_init(FOO1_ENTRY, 0666, NULL, &foo1_proc_fops,
			foo1_str, &foo1_len, foo1_str);
	if (ret) {
		pr_err("Failed to create %s\n", FOO1_ENTRY);
		goto err2;
	}

	ret = proc_create_int_and_init(FOO2_ENTRY, 0666, NULL, &foo2_proc_fops,
			foo2_int, &foo2_len, foo2_str);
	if (ret) {
		pr_err("Failed to create %s\n", FOO2_ENTRY);
		goto err1;
	}

	return 0;

err1:	
	remove_proc_entry(FOO1_ENTRY, NULL);
	kfree(foo1_str);
err2:
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
	remove_proc_entry(FOO1_ENTRY, NULL);
	remove_proc_entry(FOO2_ENTRY, NULL);

	kfree(foo1_str);
	kfree(foo2_str);
}

MODULE_LICENSE("GPL");
module_init(foo_init);
module_exit(foo_exit);

