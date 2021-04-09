#include <linux/slab.h>
#include <linux/module.h>       /* for module programming */
#include "foo.h"
#include "proc.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Youngil, Moon <jake9999@dreamwiz.com>");
MODULE_DESCRIPTION("A sample driver");
MODULE_LICENSE("Dual BSD/GPL");

/*--------------------------------------------------------*/
/* 1) Generate sample data                                */
/*--------------------------------------------------------*/

DEFINE_MUTEX(foo_lock);
LIST_HEAD(foo_list);

static int add_data(int a, int b)
{
	struct foo_info *info;

	printk(KERN_INFO "%s %d, %d\n", __func__, a, b);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	INIT_LIST_HEAD(&info->list);
	info->a = a;
	info->b = b;

	mutex_lock(&foo_lock);
	list_add(&info->list, &foo_list);
	mutex_unlock(&foo_lock);

	return 0;
}

static int add_sample_data(void)
{
	if (add_data(10, 20))
		return -ENOMEM;
	if (add_data(30, 40))
		return -ENOMEM;
	return 0;
}

static int remove_sample_data(void)
{
	struct foo_info *tmp;
	struct list_head *node, *q;

	list_for_each_safe(node, q, &foo_list){
		tmp = list_entry(node, struct foo_info, list);
		list_del(node);
		kfree(tmp);
	}

	return 0;
}
/*--------------------------------------------------------*/
/* 2) Module part                                         */
/*--------------------------------------------------------*/

static int __init foo_init(void)
{
	if (add_sample_data())
	{
		printk(KERN_INFO "add_sample_data() failed.\n");
		return -ENOMEM;
	}

	return foo_proc_init();
}

static void __exit foo_exit(void)
{
	remove_sample_data();
	foo_proc_exit();

	return;
}

module_init(foo_init);
module_exit(foo_exit);


