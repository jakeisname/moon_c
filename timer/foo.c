#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/timer.h>
#include <linux/slab.h>

/**************************************************************************
 * foo module
 **************************************************************************/

static void t1_expire(struct timer_list *t)
{
	printk("%s. cpu=%d\n", __func__, smp_processor_id());
	mod_timer(t, jiffies+1000);	
}

static struct timer_list *t;

static int __init foo_init(void)
{
	printk("%s: nr_cpu_ids=%d\n", __func__, nr_cpu_ids);
	
	t = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
	timer_setup(t, t1_expire, 0);
	mod_timer(t, jiffies+1000);	
	return 0;
}

static void __exit foo_exit(void)
{
	printk("%s:\n", __func__);
	del_timer(t);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");

