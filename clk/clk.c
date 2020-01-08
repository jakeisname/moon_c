#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm_types.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/platform_device.h>                                              
#include <linux/export.h>                                              
#include <linux/of.h>
#include <linux/property.h>  
#include <linux/printk.h>
#include <asm/fixmap.h>
#include <asm/pgtable.h>
#include <asm/memory.h>
#include <linux/device.h>
#include <linux/clk.h>

typedef struct foo_data {
	struct platform_device *pdev;
	struct clk *clk1;
	struct clk *clk2;
	struct clk *clk3;
	struct clk *clk4;
} foo_data_t;

foo_data_t *_foo;

/**************************************************************************
 * foo1 device attribute
 **************************************************************************/
int foo1 = 1000000;
int foo2 = 0;
int foo3 = 0;
int foo4 = 0;

void foo_open(struct device *dev)
{
	int rc;

	if (!_foo)
		return;

	_foo->clk1 = devm_clk_get(dev, "fooclk1");
	if (IS_ERR(_foo->clk1)) {
		dev_err(dev, "devm_clk_get() fooclk1 error \n");
		_foo->clk1 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk1\n");

	_foo->clk2 = devm_clk_get(dev, "fooclk2");
	if (IS_ERR(_foo->clk2)) {
		dev_err(dev, "devm_clk_get() fooclk3 error \n");
		_foo->clk2 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk2\n");

	_foo->clk3 = devm_clk_get(dev, "fooclk3");
	if (IS_ERR(_foo->clk3)) {
		dev_err(dev, "devm_clk_get() fooclk3 error \n");
		_foo->clk3 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk3\n");

	_foo->clk4 = devm_clk_get(dev, "fooclk4");
	if (IS_ERR(_foo->clk4)) {
		dev_err(dev, "devm_clk_get() fooclk4 error \n");
		_foo->clk4 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk4\n");

	rc = clk_prepare(_foo->clk2);
	dev_info(dev, "clk_prepare() clk2 rc=%d\n", rc);
	rc = clk_prepare(_foo->clk3);
	dev_info(dev, "clk_prepare() clk3 rc=%d\n", rc);
	rc = clk_prepare(_foo->clk4);
	dev_info(dev, "clk_prepare() clk4 rc=%d\n", rc);
}

void foo_close(struct device *dev)
{
	if (!_foo)
		return;

	clk_unprepare(_foo->clk4);
	dev_info(dev, "clk_unprepare() clk4 \n");
	clk_unprepare(_foo->clk3);
	dev_info(dev, "clk_unprepare() clk3 \n");
	clk_unprepare(_foo->clk2);
	dev_info(dev, "clk_unprepare() clk2 \n");

	devm_clk_put(dev, _foo->clk4);
	dev_info(dev, "devm_clk_put() clk4 \n");
	devm_clk_put(dev, _foo->clk3);
	dev_info(dev, "devm_clk_put() clk3 \n");
	devm_clk_put(dev, _foo->clk2);
	dev_info(dev, "devm_clk_put() clk2 \n");
	devm_clk_put(dev, _foo->clk1);
	dev_info(dev, "devm_clk_put() clk1 \n");

}

void set_rate_foo2(struct device *dev, int val)
{
	int rc;

	if (!_foo)
		return;

	rc = clk_set_rate(_foo->clk2, val);
	dev_info(dev, "clk_set_rate() clk2 val=%d rc=%d\n", val, rc);
}

void set_rate_foo3(struct device *dev, int val)
{
	int rc;

	if (!_foo)
		return;

	rc = clk_set_rate(_foo->clk3, val);
	dev_info(dev, "clk_set_rate() clk3 val=%d rc=%d\n", val, rc);
}

void set_rate_foo4(struct device *dev, int val)
{
	int rc;

	if (!_foo)
		return;

	rc = clk_set_rate(_foo->clk4, val);
	dev_info(dev, "clk_set_rate() clk4 val=%d rc=%d\n", val, rc);
}

static ssize_t foo1_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", foo1);
}

static ssize_t foo1_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	int __foo1;	

	sscanf(buf, "%d", &__foo1);
	return sizeof(int);
}

static ssize_t foo2_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", foo2);
}

static ssize_t foo2_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &foo2);
	if (foo2 > 9)
		set_rate_foo2(&_foo->pdev->dev, foo2);
	return sizeof(int);
}

static ssize_t foo3_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", foo3);
}

static ssize_t foo3_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &foo3);
	if (foo3 > 9)
		set_rate_foo3(&_foo->pdev->dev, foo3);
	return sizeof(int);
}

static ssize_t foo4_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", foo4);
}

static ssize_t foo4_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &foo4);
	if (foo4 > 9)
		set_rate_foo4(&_foo->pdev->dev, foo4);
	return sizeof(int);
}

static DRIVER_ATTR_RW(foo1);
static DRIVER_ATTR_RW(foo2);
static DRIVER_ATTR_RW(foo3);
static DRIVER_ATTR_RW(foo4);
static struct attribute *foo_driver_attrs[] = {
	&driver_attr_foo1.attr,
	&driver_attr_foo2.attr,
	&driver_attr_foo3.attr,
	&driver_attr_foo4.attr,
	NULL
};
ATTRIBUTE_GROUPS(foo_driver);

static int foo_probe(struct platform_device *pdev)
{ 
	struct device *dev = &pdev->dev;
	foo_data_t *foo;

        dev_info(dev, "%s\n", __func__);

	foo = devm_kzalloc(dev, sizeof(*foo), GFP_KERNEL);
	if (!foo) {
		dev_err(dev, "out of memory error.\n");
		return -ENOMEM;
	}

	foo->pdev = pdev;
	platform_set_drvdata(pdev, foo);
	_foo = foo;

	foo_open(dev);

        return 0;     /* 0=success */
}

static int foo_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

        dev_info(dev, "%s\n", __func__);

	foo_close(dev);

	if (_foo)
		devm_kfree(dev, _foo);

	_foo = NULL;

        return 0;
}


static const struct of_device_id foo_of_match_table[] = {
    { .compatible = "foo,foo" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, foo_of_match_table);

static struct platform_driver foo = {
        .probe = foo_probe,
	.remove = foo_remove,
        .driver = {
                .name = "foo",
                .groups = foo_driver_groups,
                .of_match_table = foo_of_match_table,
        },
};

module_platform_driver(foo);

MODULE_AUTHOR("Jake, Moon, https://jake.dothome.co.kr");
MODULE_DESCRIPTION("foo driver");
MODULE_LICENSE("GPL");

