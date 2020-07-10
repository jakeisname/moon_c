#include <linux/kernel.h>
#include <linux/module.h>

int param = 0;
module_param(param, int, 0);

int foo_init(void)
{
	void *p = foo_init;

	printk(KERN_INFO "kptr_restrict=%d, x=0x%llx, p=%p, pK=%pK, px=%px\n", 
			param, (long long unsigned int) p, p, p, p);
	return 0;
}

void foo_exit(void)
{
}

module_init(foo_init);
module_exit(foo_exit);

MODULE_DESCRIPTION("foo");
MODULE_LICENSE("GPL");
