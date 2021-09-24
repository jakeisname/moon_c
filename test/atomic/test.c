#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/atomic.h>

int dual = 0;
module_param(dual, int, 0);
MODULE_PARM_DESC(dual, "An integer");

static int __init foo_init(void)
{
	atomic_t v;

	atomic_set(&v, 100);
	atomic_add(1, &v);

	printk("v=%d\n", v.counter);

	printk("%s: invoked. dual=%d\n", __func__, dual);

	

        return 0;     /* 0=success */
}

static void __exit foo_exit(void)
{
        printk("%s: invoked\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");

