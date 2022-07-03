
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
#define MY_PROC_ENTRY "foo"
#define PROC_FULL_PATH "/proc/foo"

struct proc_dir_entry *proc;
int foo_len;
char *msg = NULL;

static ssize_t foo_write(struct file *filp, 
		const char __user * buffer, size_t count, loff_t *pos)
{
    char *data = PDE_DATA(file_inode(filp));

    printk(KERN_INFO "%s(%d): count=%lu, pos=%lld, old foo_len=%d\n", 
		    __func__, __LINE__, count, *pos, foo_len);

    if (count > DATA_SIZE) {
        return -EFAULT;
    }

    if (copy_from_user(data, buffer, count)) {
        return -EFAULT;
    }

    data[count-1] = '\0';

    printk(KERN_INFO "set new foo=%s", msg);

    *pos = (int) count;
    foo_len = count - 1;

    printk(KERN_INFO "%s(%d): count=%lu, pos=%lld, new foo_len=%d\n", 
		    __func__, __LINE__, count, *pos, foo_len);
    return count;
}

ssize_t foo_read(struct file *filp, char __user *buf, 
		size_t count, loff_t *offp)
{
    int err;
    char *data = PDE_DATA(file_inode(filp));

    printk(KERN_INFO "%s(%d): count=%lu, offp=%lld, foo_len=%d\n", 
		    __func__, __LINE__, count, *offp, foo_len);

    if ((int) (*offp) > foo_len) {
        return 0;
    }

    if (!(data)) {
        return 0;
    }

    if (count == 0) {
        return count;	/* read 0 size, nothing to do */
    } 

    count = foo_len + 1;	/* +1 to read the \0 */
    err = copy_to_user(buf, data, count);	/* +1 for \0 */
    printk(KERN_INFO "get foo=%s\n", buf);
    *offp = count;

    if (err) {
        printk(KERN_INFO "Error in copying data.");
    }

    printk(KERN_INFO "%s(%d): count=%lu, offp=%lld, foo_len=%d\n", 
		    __func__, __LINE__, count, *offp, foo_len);

    return count;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
struct proc_ops proc_fops = {
    .proc_read = foo_read,
    .proc_write = foo_write,
};
#else
struct file_operations proc_fops = {
    .read = foo_read,
    .write = foo_write,
};
#endif


int create_new_proc_entry(void) 
{
    char *DATA = "foo";
    foo_len = strlen(DATA);
    msg = kmalloc((size_t) DATA_SIZE, GFP_KERNEL); /* +1 for \0 */

    if (msg == NULL) {
        return -1;
    }

    strncpy(msg, DATA, foo_len + 1);

    proc = proc_create_data(MY_PROC_ENTRY, 0666, NULL, &proc_fops, msg);
    if (proc) {
        return 0;
    }

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
    remove_proc_entry(MY_PROC_ENTRY, NULL);
}

MODULE_LICENSE("GPL");
module_init(foo_init);
module_exit(foo_exit);

