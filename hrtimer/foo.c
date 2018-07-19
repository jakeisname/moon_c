#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/hrtimer.h>
#include <linux/delay.h>

/**************************************************************************
 * foo hrtimer
 **************************************************************************/

/* 1 sec */
#define FOO_INTERVAL 1000000000UL

static DEFINE_PER_CPU(struct hrtimer, foo_hrtimer);

static enum hrtimer_restart foo_hrtimer_handler(struct hrtimer *hrtimer)
{
	static int foo_cnt[NR_CPUS] = { 0, };
	int cpu = smp_processor_id();

	printk("cpu#%d, cpu2#%d: cnt=%d\n", cpu, cpu, ++foo_cnt[cpu]);
	hrtimer_forward_now(hrtimer, ns_to_ktime(FOO_INTERVAL));

	return HRTIMER_RESTART;
}

static void start_foo_timer(int cpu)
{
	struct hrtimer *hrtimer = per_cpu_ptr(&foo_hrtimer, cpu);

	printk("%s: cpu#%d start.\n", __func__, cpu);
	hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
	hrtimer->function = foo_hrtimer_handler;
	hrtimer_start(hrtimer, ns_to_ktime(FOO_INTERVAL), 
			HRTIMER_MODE_REL_PINNED);
}

static void stop_foo_timer(int cpu)
{
	struct hrtimer *hrtimer = per_cpu_ptr(&foo_hrtimer, cpu);

	printk("%s: cpu#%d stop.\n", __func__, cpu);
	hrtimer_cancel(hrtimer);
}


/**************************************************************************
 * foo workqueue
 **************************************************************************/

static void foo_work_func(struct work_struct *work)
{
	int cpu = smp_processor_id();

	printk("%s: current_cpu=%d\n", __func__, cpu);

	start_foo_timer(cpu);
}

static DECLARE_WORK(foo_work0, foo_work_func);
static DECLARE_WORK(foo_work1, foo_work_func);
static DECLARE_WORK(foo_work2, foo_work_func);
static DECLARE_WORK(foo_work3, foo_work_func);


/**************************************************************************
 * foo module
 **************************************************************************/

static int __init foo_init(void)
{
	printk("%s: nr_cpu_ids=%d\n", __func__, nr_cpu_ids);
	schedule_work_on(0, &foo_work0);
	schedule_work_on(1, &foo_work1);
	schedule_work_on(2, &foo_work2);
	schedule_work_on(3, &foo_work3);

	return 0;
}

static void __exit foo_exit(void)
{
	int cpu;

	printk("%s:\n", __func__);
	flush_scheduled_work();
	for (cpu = 0; cpu < nr_cpu_ids; cpu++) 
		stop_foo_timer(cpu);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");

