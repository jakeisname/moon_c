#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bitops.h>

int dual = 0;
module_param(dual, int, 0);
MODULE_PARM_DESC(dual, "An integer");

static unsigned long addr1[] = { 0x1020304050607080, 0x1122334455667788 };
static unsigned long addr2[] = { 0xffffffffffffffff, 0x000000000000ffff };

static void test1(void)
{
        unsigned long l, offset;
        unsigned long size = 96;

        l = find_next_bit(addr1, size, -1UL);
        printk("offset=%lu, l=%lu\n", -1UL, l);

        for (offset = 0; offset <= size; offset++) {
                l = find_next_bit(addr1, size, offset);
                printk("offset=%lu, l=%lu\n", offset, l);
        }
}

static void test2(void)
{
        unsigned long l, offset;
        unsigned long size = 96;

        l = find_next_zero_bit(addr1, size, -1UL);
        printk("offset=%lu, l=%lu\n", -1UL, l);

        for (offset = 0; offset <= size; offset++) {
                l = find_next_zero_bit(addr1, size, offset);
                printk("offset=%lu, l=%lu\n", offset, l);
        }
}

static void test3(void)
{
        unsigned long l, offset;
        unsigned long size = 96;

        l = find_next_and_bit(addr1, addr2, size, -1UL);
        printk("offset=%lu, l=%lu\n", -1UL, l);

        for (offset = 0; offset <= size; offset++) {
                l = find_next_and_bit(addr1, addr2, size, offset);
                printk("offset=%lu, l=%lu\n", offset, l);
        }
}

static void test4(void)
{
        unsigned long l;
        unsigned long size = 96;

        l = find_first_bit(addr1, size);
        printk("l=%lu\n", l);

        l = find_first_zero_bit(addr1, size);
        printk("l=%lu\n", l);

        l = find_last_bit(addr1, size);
        printk("l=%lu\n", l);
}

static int __init foo_init(void)
{
        printk("%s: invoked. dual=%d\n", __func__, dual);

        test1();
        test2();
        test3();
	test4();

        return 0;     /* 0=success */
}

static void __exit foo_exit(void)
{
        printk("%s: invoked\n", __func__);
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");

