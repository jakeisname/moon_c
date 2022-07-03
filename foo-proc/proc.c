#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include "foo.h"
#include "proc.h"

// #define USE_SINGLE_OPEN

/*--------------------------------------------------------*/
/* 1) seq_file operations part                            */
/*--------------------------------------------------------*/

#ifdef USE_SINGLE_OPEN
#else
static void *foo_seq_start(struct seq_file *s, loff_t *pos)
{
	printk(KERN_INFO "%s", __func__);
	mutex_lock(&foo_lock);
	s->private = "";

	return seq_list_start(&foo_list, *pos);
}

static void *foo_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	printk(KERN_INFO "%s", __func__);
	s->private = "\n";

	return seq_list_next(v, &foo_list, pos);
}

static void foo_seq_stop(struct seq_file *s, void *v)
{
	mutex_unlock(&foo_lock);
	printk(KERN_INFO "%s", __func__);
}

static int foo_seq_show(struct seq_file *m, void *v)
{
	struct foo_info *info = list_entry(v, struct foo_info, list);

	printk(KERN_INFO "%s", __func__);
	seq_printf(m, "%d + %d = %d\n", info->a, info->b, info->a + info->b);

	return 0;
}

static const struct seq_operations foo_seq_ops = {
	.start  = foo_seq_start,
	.next   = foo_seq_next,
	.stop   = foo_seq_stop,
	.show   = foo_seq_show
};
#endif

/*--------------------------------------------------------*/
/* 2) proc operations part                                */
/*--------------------------------------------------------*/

#ifdef USE_SINGLE_OPEN
static int foo_simple_show(struct seq_file *s, void *unused)
{
	struct foo_info *info;

	list_for_each_entry(info, &foo_list, list)
		seq_printf(s, "%d + %d = %d\n", info->a, info->b, info->a + info->b);

	return 0;
}
#endif

static int foo_proc_open(struct inode *inode, struct file *file)
{
#ifdef USE_SINGLE_OPEN
	return single_open(file, foo_simple_show, NULL);
#else
	return seq_open(file, &foo_seq_ops);
#endif
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
struct proc_ops foo_proc_ops = {
	.proc_open	= foo_proc_open,
	.proc_read	= seq_read,
	.proc_lseek     = seq_lseek,
	.proc_release   = seq_release,
};
#else
static const struct file_operations foo_proc_ops = {
	.owner          = THIS_MODULE,
	.open           = foo_proc_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = seq_release,
};
#endif

/*--------------------------------------------------------*/
/* 3) proc interface part  (/proc/foo-dir/foo)            */
/*--------------------------------------------------------*/

#define FOO_DIR "foo-dir"
#define FOO_FILE "foo"

static struct proc_dir_entry *foo_proc_dir = NULL;
static struct proc_dir_entry *foo_proc_file = NULL;

int foo_proc_init(void)
{
	foo_proc_dir = proc_mkdir(FOO_DIR, NULL);
	if (foo_proc_dir == NULL)
	{
		printk("Unable to create /proc/%s\n", FOO_DIR);
		return -1;
	}

	foo_proc_file = proc_create(FOO_FILE, 0, foo_proc_dir, &foo_proc_ops); /* S_IRUGO */
	if (foo_proc_file == NULL)
	{
		printk("Unable to create /proc/%s/%s\n", FOO_DIR, FOO_FILE);
		remove_proc_entry(FOO_DIR, NULL);
		return -1;
	}

	printk(KERN_INFO "Created /proc/%s/%s\n", FOO_DIR, FOO_FILE);
	return 0;
}

void foo_proc_exit(void)
{
	/* remove directory and file from procfs */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
	remove_proc_entry(FOO_FILE, foo_proc_dir);
	remove_proc_entry(FOO_DIR, NULL);
#else
	remove_proc_subtree(FOO_DIR, NULL);
#endif

	/* remove proc_dir_entry instance */
	//proc_remove(foo_proc_file);
	//proc_remove(foo_proc_dir);

	printk(KERN_INFO "Removed /proc/%s/%s\n", FOO_DIR, FOO_FILE);
}



