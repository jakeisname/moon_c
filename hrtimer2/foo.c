#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/hrtimer.h>

/**************************************************************************
 * foo module
 **************************************************************************/

#define FOO_INTERVAL 3000000000
static DEFINE_PER_CPU(struct hrtimer, foo_hrtimer);

static enum hrtimer_restart foo_hrtimer_handler(struct hrtimer *hrtimer)
{
	static int foo_cnt[NR_CPUS] = { 0, };
	int cpu = smp_processor_id();

	printk("cpu#%d: cnt=%d\n", cpu, ++foo_cnt[cpu]);
	hrtimer_forward_now(hrtimer, ns_to_ktime(FOO_INTERVAL));

	return HRTIMER_RESTART;
}

static void start_foo_timer(int cpu)
{
	struct hrtimer *hrtimer = per_cpu_ptr(&foo_hrtimer, cpu);

	printk("%s: cpu#%d start.\n", __func__, cpu);
	hrtimer_init(hrtimer, CLOCK_MONOTONIC, 0);
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

static int __init foo_init(void)
{
	int cpu = 1;

	printk("%s: nr_cpu_ids=%d\n", __func__, nr_cpu_ids);
	
	start_foo_timer(cpu);
	return 0;
}

static void __exit foo_exit(void)
{
	int cpu = 1;

	printk("%s:\n", __func__);
	stop_foo_timer(cpu);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");

