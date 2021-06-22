#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm_types.h>
#include <linux/slab_def.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/printk.h>
#include <linux/vmalloc.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>

int dual = 0;
module_param(dual, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(dual, "An integer");

struct task_struct *th1 = NULL;
struct task_struct *th2= NULL;

static int foo_kthread1(void *arg)
{
	volatile long l = 0;

	printk("%s: invoked\n", __func__);

	while (!kthread_should_stop()) 
		l++;

	printk("%s: stopped. l=%ld\n", __func__, l);

	return 0;
} 

static int foo_kthread2(void *arg)
{
	volatile long l = 0;

	printk("%s: invoked\n", __func__);

	while (!kthread_should_stop()) 
		l++;

	printk("%s: stopped. l=%ld\n", __func__, l);

	return 0;
} 


static int __init foo_init(void)
{
        printk("%s\n", __func__);

#if defined(TEST_RR)
	/* RR prio=99 */
	th1 = (struct task_struct *) kthread_run(foo_kthread1, NULL, "foo_kthread1");
	if (IS_ERR(th1)) {
		printk(KERN_ALERT "Fail to create the thread\n");
		return -1;
	}

	th2 = (struct task_struct *) kthread_run(foo_kthread2, NULL, "foo_kthread2");
	if (IS_ERR(th2)) {
		printk(KERN_ALERT "Fail to create the thread\n");
		return -1;
	}
#else
	th1 = (struct task_struct *) kthread_create(foo_kthread1, NULL, "foo_kthread1");
	if (IS_ERR(th1)) {
		printk(KERN_ALERT "Fail to create the thread\n");
		return -1;
	}

	/* FIFO */
	sched_set_fifo_low(th1);
	wake_up_process(th1);

	if (!dual)
		return 0;

	th2 = (struct task_struct *) kthread_create(foo_kthread2, NULL, "foo_kthread2");
	if (IS_ERR(th2)) {
		printk(KERN_ALERT "Fail to create the thread\n");
		return -1;
	}

	sched_set_fifo(th2);
	wake_up_process(th2);
#endif

        return 0;     /* 0=success */
}

static void __exit foo_exit(void)
{

        printk("%s: invoked\n", __func__);

	if (dual && th2) {
		kthread_stop(th2);
		th2 = NULL;
	}

	if (th1) {
		kthread_stop(th1);
		th1 = NULL;
	}
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");

