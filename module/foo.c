#include <linux/kernel.h>
#include <linux/module.h>

int foo_init(void)
{
	printk(KERN_INFO "init.\n");

	return 0;
}

void foo_exit(void)
{
	printk(KERN_INFO "exit.\n");
}

module_init(foo_init);
module_exit(foo_exit);

MODULE_DESCRIPTION("foo");
MODULE_LICENSE("GPL");
