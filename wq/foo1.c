#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>

/*****************************************
 *                                       *
 * simple work on custom wq or global wq *
 *                                       *
 *****************************************/
#define USE_CUSTOM_WQ	1

#if defined(USE_CUSTOM_WQ)
static struct workqueue_struct *my_wq;
#endif

static void my_work_func_1(struct work_struct *work)
{
	printk(KERN_INFO "my_work_func_1\n");
}

/* prepare work on compile time */
DECLARE_WORK(my_work, my_work_func_1);

/******************************************/

int foo_init(void)
{
	printk(KERN_INFO "init.");

#if defined(USE_CUSTOM_WQ)
	/* create custom wq */
	my_wq = create_singlethread_workqueue("my_wq");

	/* send work to custom wq */
	queue_work(my_wq, &my_work);
#else

	/* send work to global wq */
	schedule_work(&my_work);
#endif

	return 0;
}

void foo_exit(void)
{
	cancel_work_sync(&my_work);

#if defined(USE_CUSTOM_WQ)
	destroy_workqueue(my_wq);
#endif

	printk(KERN_INFO "exit.\n");
}

module_init(foo_init);
module_exit(foo_exit);

MODULE_DESCRIPTION("foo1");
MODULE_LICENSE("GPL");
