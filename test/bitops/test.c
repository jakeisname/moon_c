#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/atomic.h>
#include <linux/bitops.h>

int dual = 0;
module_param(dual, int, 0);
MODULE_PARM_DESC(dual, "An integer");

static void test1(void)
{
	unsigned long bitmap[4] = { 0xffff8003, };
	unsigned long l, offset;

	l = find_next_bit(bitmap, 16, -1L);
	printk("offset=%ld, l=%ld\n", -1L, l);

	for (offset = 0; offset <= 16; offset++) {
		l = find_next_bit(bitmap, 16, offset);
		printk("offset=%ld, l=%ld\n", offset, l);
	}
}

static void test2(void)
{
	unsigned long bitmap[4] = { 0xffff800a, };
	unsigned long l, offset;

	l = find_next_zero_bit(bitmap, 16, -1L);
	printk("offset=%ld, l=%ld\n", -1L, l);

	for (offset = 0; offset <= 16; offset++) {
		l = find_next_zero_bit(bitmap, 16, offset);
		printk("offset=%ld, l=%ld\n", offset, l);
	}
}

static void test3(void)
{
	unsigned long bitmap1[4] = { 0xffff800a, };
	unsigned long bitmap2[4] = { 0xffff800f, };
	unsigned long l, offset;

	l = find_next_and_bit(bitmap1, bitmap2, 16, -1L);
	printk("offset=%ld, l=%ld\n", -1L, l);

	for (offset = 0; offset <= 16; offset++) {
		l = find_next_and_bit(bitmap1, bitmap2, 16, offset);
		printk("offset=%ld, l=%ld\n", offset, l);
	}
}

static void test4(void)
{
	unsigned long bitmap[4] = { 0xffff800a, };
	unsigned long l;

	l = find_first_zero_bit(bitmap, 16);
	printk("l=%ld\n", l);
}

static void test5(void)
{
	unsigned long bitmap[4] = { 0xffff800a, };
	unsigned long l;

	l = find_first_bit(bitmap, 16);
	printk("l=%ld\n", l);
}

static void test6(void)
{
	unsigned long bitmap[4] = { 0xffff800a, };
	unsigned long l;

	l = find_last_bit(bitmap, 16);
	printk("l=%ld\n", l);
}

static int __init foo_init(void)
{
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();

        return 0;     /* 0=success */
}

static void __exit foo_exit(void)
{
        printk("%s: invoked\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");

