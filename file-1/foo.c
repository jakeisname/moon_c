#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>

#define FOO_FILE        "/tmp/foo"

int foo_init(void)
{
        struct file *f;
        const char *buf = "1\n";

        printk(KERN_INFO "init.\n");

        f = filp_open(FOO_FILE, O_CREAT | O_WRONLY, 755);
        if (f == NULL) {
                printk(KERN_ALERT "filp_open %s failed.\n", FOO_FILE);
                return -1;
        }

	kernel_write(f, buf, 2, &f->f_pos);

        filp_close(f, NULL);

	vfs_fsync(f, 0);

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

