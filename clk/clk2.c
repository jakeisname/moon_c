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
	struct clk *clk5;
	struct clk *clk6;
	struct clk *clk7;
} foo_data_t;

foo_data_t *_foo;

/**************************************************************************
 * d1~d5 driver attribute
 **************************************************************************/
static int d1 = 0;
static int d2 = 0;
static int d3 = 0;
static int d4 = 0;
static int d5 = 0;

static void foo_open(struct device *dev)
{
	int rc;

	if (!_foo)
		return;

	_foo->clk1 = devm_clk_get(dev, "clk_f1");
	if (IS_ERR(_foo->clk1)) {
		dev_err(dev, "devm_clk_get() clk_f1 error \n");
		_foo->clk1 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk_f1 \n");

	_foo->clk2 = devm_clk_get(dev, "clk_f2");
	if (IS_ERR(_foo->clk2)) {
		dev_err(dev, "devm_clk_get() clk_f2 error \n");
		_foo->clk2 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk_f2\n");

	_foo->clk3 = devm_clk_get(dev, "clk_d1");
	if (IS_ERR(_foo->clk3)) {
		dev_err(dev, "devm_clk_get() clk_d1 error \n");
		_foo->clk3 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk_d1\n");

	_foo->clk4 = devm_clk_get(dev, "clk_d2");
	if (IS_ERR(_foo->clk4)) {
		dev_err(dev, "devm_clk_get() clk_d2 error \n");
		_foo->clk4 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk_d2\n");

	_foo->clk5 = devm_clk_get(dev, "clk_d3");
	if (IS_ERR(_foo->clk5)) {
		dev_err(dev, "devm_clk_get() clk_d3 error \n");
		_foo->clk5 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk_d3\n");

	_foo->clk6 = devm_clk_get(dev, "clk_d4");
	if (IS_ERR(_foo->clk6)) {
		dev_err(dev, "devm_clk_get() clk_d4 error \n");
		_foo->clk6 = NULL;
		return;
	}
	dev_info(dev, "devm_clk_get() clk_d4\n");

	_foo->clk7 = devm_clk_get(dev, "clk_d5");
	if (IS_ERR(_foo->clk7)) {
		dev_err(dev, "devm_clk_get() clk_d5 error \n");
		_foo->clk7 = NULL;
		return;
	}

	dev_info(dev, "devm_clk_get() clk_d3\n");
	rc = clk_prepare(_foo->clk1);
	dev_info(dev, "clk_prepare() clk_f1 rc=%d\n", rc);
	rc = clk_prepare(_foo->clk2);
	dev_info(dev, "clk_prepare() clk_f2 rc=%d\n", rc);
	rc = clk_prepare(_foo->clk3);
	dev_info(dev, "clk_prepare() clk_d1 rc=%d\n", rc);
	rc = clk_prepare(_foo->clk4);
	dev_info(dev, "clk_prepare() clk_d2 rc=%d\n", rc);
	rc = clk_prepare(_foo->clk5);
	dev_info(dev, "clk_prepare() clk_d3 rc=%d\n", rc);
	rc = clk_prepare(_foo->clk6);
	dev_info(dev, "clk_prepare() clk_d4 rc=%d\n", rc);
	rc = clk_prepare(_foo->clk7);
	dev_info(dev, "clk_prepare() clk_d5 rc=%d\n", rc);

	rc = clk_enable(_foo->clk1);
	dev_info(dev, "clk_enable() clk_f1 rc=%d\n", rc);
	rc = clk_enable(_foo->clk2);
	dev_info(dev, "clk_enable() clk_f2 rc=%d\n", rc);
	rc = clk_enable(_foo->clk3);
	dev_info(dev, "clk_enable() clk_d1 rc=%d\n", rc);
	rc = clk_enable(_foo->clk4);
	dev_info(dev, "clk_enable() clk_d2 rc=%d\n", rc);
	rc = clk_enable(_foo->clk5);
	dev_info(dev, "clk_enable() clk_d3 rc=%d\n", rc);
	rc = clk_enable(_foo->clk6);
	dev_info(dev, "clk_enable() clk_d4 rc=%d\n", rc);
	rc = clk_enable(_foo->clk7);
	dev_info(dev, "clk_enable() clk_d7 rc=%d\n", rc);
}

static void foo_close(struct device *dev)
{
	if (!_foo)
		return;

#if 0
	clk_disable(_foo->clk5);
	dev_info(dev, "clk_disable() clk5\n");
	clk_disable(_foo->clk4);
	dev_info(dev, "clk_disable() clk4\n");
#endif
	clk_disable(_foo->clk3);
	dev_info(dev, "clk_disable() clk_d1\n");
	clk_disable(_foo->clk2);
	dev_info(dev, "clk_disable() clk_f2\n");
	clk_disable(_foo->clk1);
	dev_info(dev, "clk_disable() clk_f1\n");

#if 0
	clk_unprepare(_foo->clk5);
	dev_info(dev, "clk_unprepare() clk5 \n");
	clk_unprepare(_foo->clk4);
	dev_info(dev, "clk_unprepare() clk4 \n");
#endif
	clk_unprepare(_foo->clk3);
	dev_info(dev, "clk_unprepare() clk_d1 \n");
	clk_unprepare(_foo->clk2);
	dev_info(dev, "clk_unprepare() clk_f2 \n");
	clk_unprepare(_foo->clk1);
	dev_info(dev, "clk_unprepare() clk_f1 \n");

#if 0
	devm_clk_put(dev, _foo->clk5);
	dev_info(dev, "devm_clk_put() clk5 \n");
	devm_clk_put(dev, _foo->clk4);
	dev_info(dev, "devm_clk_put() clk4 \n");
#endif
	devm_clk_put(dev, _foo->clk3);
	dev_info(dev, "devm_clk_put() clk_d1 \n");
	devm_clk_put(dev, _foo->clk2);
	dev_info(dev, "devm_clk_put() clk_f2 \n");
	devm_clk_put(dev, _foo->clk1);
	dev_info(dev, "devm_clk_put() clk_f1 \n");

}

void set_rate_d1(struct device *dev, int val)
{
	int rc;

	if (!_foo)
		return;

	rc = clk_set_rate(_foo->clk3, val);
	dev_info(dev, "clk_set_rate() clk_d1 val=%d rc=%d\n", val, rc);
}

void set_rate_d2(struct device *dev, int val)
{
	int rc;

	if (!_foo)
		return;

	rc = clk_set_rate(_foo->clk4, val);
	dev_info(dev, "clk_set_rate() clk_d2 val=%d rc=%d\n", val, rc);
}

void set_rate_d3(struct device *dev, int val)
{
	int rc;

	if (!_foo)
		return;

	rc = clk_set_rate(_foo->clk5, val);
	dev_info(dev, "clk_set_rate() clk_d3 val=%d rc=%d\n", val, rc);
}

void set_rate_d4(struct device *dev, int val)
{
	int rc;

	if (!_foo)
		return;

	rc = clk_set_rate(_foo->clk6, val);
	dev_info(dev, "clk_set_rate() clk_d4 val=%d rc=%d\n", val, rc);
}

void set_rate_d5(struct device *dev, int val)
{
	int rc;

	if (!_foo)
		return;

	rc = clk_set_rate(_foo->clk7, val);
	dev_info(dev, "clk_set_rate() clk_d5 val=%d rc=%d\n", val, rc);
}

static ssize_t d1_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", d1);
}

static ssize_t d1_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &d1);
	if (d1 > 9)
		set_rate_d1(&_foo->pdev->dev, d1);
	return sizeof(int);
}

static ssize_t d2_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", d2);
}

static ssize_t d2_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &d2);
	if (d2 > 9)
		set_rate_d2(&_foo->pdev->dev, d2);
	return sizeof(int);
}

static ssize_t d3_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", d3);
}

static ssize_t d3_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &d3);
	if (d3 > 9)
		set_rate_d3(&_foo->pdev->dev, d3);
	return sizeof(int);
}

static ssize_t d4_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", d4);
}

static ssize_t d4_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &d4);
	if (d4 > 9)
		set_rate_d4(&_foo->pdev->dev, d4);
	return sizeof(int);
}

static ssize_t d5_show(struct device_driver *driver,  
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", d5);
}

static ssize_t d5_store(struct device_driver *driver,
		const char *buf, size_t len)
{
	sscanf(buf, "%d", &d5);
	if (d5 > 9)
		set_rate_d5(&_foo->pdev->dev, d5);
	return sizeof(int);
}

static DRIVER_ATTR_RW(d1);
static DRIVER_ATTR_RW(d2);
static DRIVER_ATTR_RW(d3);
static DRIVER_ATTR_RW(d4);
static DRIVER_ATTR_RW(d5);

static struct attribute *foo22_driver_attrs[] = {
	&driver_attr_d1.attr,
	&driver_attr_d2.attr,
	&driver_attr_d3.attr,
	&driver_attr_d4.attr,
	&driver_attr_d5.attr,
	NULL
};
ATTRIBUTE_GROUPS(foo22_driver);

static int foo22_probe(struct platform_device *pdev)
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

static int foo22_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

        dev_info(dev, "%s\n", __func__);

	foo_close(dev);

	if (_foo)
		devm_kfree(dev, _foo);

	_foo = NULL;

        return 0;
}


static const struct of_device_id foo22_of_match_table[] = {
    { .compatible = "foo,foo22" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, foo22_of_match_table);

static struct platform_driver foo22 = {
        .probe = foo22_probe,
	.remove = foo22_remove,
        .driver = {
                .name = "foo22",
                .groups = foo22_driver_groups,
                .of_match_table = foo22_of_match_table,
        },
};

module_platform_driver(foo22);

MODULE_AUTHOR("Jake, Moon, https://jake.dothome.co.kr");
MODULE_DESCRIPTION("foo22 driver");
MODULE_LICENSE("GPL");

