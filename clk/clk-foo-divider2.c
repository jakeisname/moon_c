
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/platform_device.h>

struct foo_divider {
	struct clk *clk;
        struct clk_hw *hw;
        void __iomem *reg;
};

static DEFINE_SPINLOCK(clklock);

static struct foo_divider *_of_foo_divider_clk2_setup(struct device *dev,
		struct device_node *node)
{
	struct foo_divider *foo;
	const char *parent_name;
	u32 width32 = 0;
	u8 width = 0;
	u8 shift = 0;
	u8 div_flags = 0;
	unsigned long flags = 0;
	const char *clk_name = node->name;
	
	parent_name = of_clk_get_parent_name(node, 0);

	if (of_property_read_bool(node, "foo,index-power-of-two"))
		div_flags |= CLK_DIVIDER_POWER_OF_TWO;

	if (of_property_read_bool(node, "foo,set-rate-parent"))
		flags |= CLK_SET_RATE_PARENT;

	if (of_property_read_u32(node, "foo,width", &width32)) {
		pr_err("no width for %pOFn!\n", node);
		return ERR_PTR(-EIO);
	}

	width = (u8) width32;

	foo = devm_kzalloc(dev, sizeof(struct foo_divider), GFP_KERNEL);
        if (foo == NULL) {
		pr_err("kmalloc failed.\n");
		return ERR_PTR(-ENOMEM);
        }

	foo->reg = devm_kzalloc(dev, sizeof(void *), GFP_KERNEL);
        if (foo->reg == NULL) {
		pr_err("kmalloc failed.\n");
		return ERR_PTR(-ENOMEM);
        }

        foo->hw = clk_hw_register_divider(NULL, clk_name, parent_name,
                                   flags, foo->reg, shift, width,
                                   div_flags, &clklock);
        if (IS_ERR(foo->hw)) {
		pr_err("clk_hw_register_divider() failed.\n");
		return ERR_PTR(-EIO);
        }

        of_clk_add_hw_provider(node, of_clk_hw_simple_get, foo->hw);

	return foo;
}

static int of_foo_divider_clk2_remove(struct platform_device *pdev)
{
	struct foo_divider *foo = platform_get_drvdata(pdev);

	if (!IS_ERR(foo->hw))
		clk_hw_unregister(foo->hw);

	return 0;
}

static int of_foo_divider_clk2_probe(struct platform_device *pdev)
{
	struct foo_divider *foo;

	/*
	 * This function is not executed when of_fixed_clk_setup
	 * succeeded.
	 */
	foo = _of_foo_divider_clk2_setup(&pdev->dev, pdev->dev.of_node);
	if (IS_ERR(foo))
		return -EIO;

	platform_set_drvdata(pdev, foo);

	return 0;
}

static const struct of_device_id of_foo_divider_clk2_ids[] = {
	{ .compatible = "foo,divider-clock2" },
	{ }
};
MODULE_DEVICE_TABLE(of, of_foo_divider_clk2_ids);

static struct platform_driver of_foo_divider_clk2_driver = {
	.driver = {
		.name = "of_foo_divider_clk2",
		.of_match_table = of_foo_divider_clk2_ids,
	},
	.probe = of_foo_divider_clk2_probe,
	.remove = of_foo_divider_clk2_remove,
};
module_platform_driver(of_foo_divider_clk2_driver);

MODULE_AUTHOR("Jake, Moon, https://jake.dothome.co.kr");
MODULE_DESCRIPTION("foo divider clock 2 driver");
MODULE_LICENSE("GPL");
