#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>

/******************************************
 *                                        *
 * delayed work on custom wq or global wq *
 *                                        *
 ******************************************/
#define USE_CUSTOM_WQ	0

#if defined(USE_CUSTOM_WQ)
static struct workqueue_struct *my_wq;
#endif

static void my_work_func_2(struct work_struct *work)
{
	printk(KERN_INFO "my_work_func_2\n");
}

/* prepare work on compile time */
DECLARE_DELAYED_WORK(my_delayed_work, my_work_func_2);

/*******************************************/

int foo_init(void)
{
	unsigned long delay = 250; /* 250 jiffies@250HZ = 1 sec */

	printk(KERN_INFO "init.");

#if defined(USE_CUSTOM_WQ)
	/* create custom wq */
	my_wq = create_singlethread_workqueue("my_wq");

	/* send work to custom wq after 1 sec */
	queue_delayed_work(my_wq, &my_delayed_work, delay);
#else
	/* send work to glabal wq after 1 sec */
	schedule_delayed_work(&my_delayed_work, delay);
#endif

	return 0;
}

void foo_exit(void)
{
	cancel_delayed_work(&my_delayed_work);

#if defined(USE_CUSTOM_WQ)
	destroy_workqueue(my_wq);
#endif

	printk(KERN_INFO "exit.\n");
}

module_init(foo_init);
module_exit(foo_exit);

MODULE_DESCRIPTION("foo2");
MODULE_LICENSE("GPL");
