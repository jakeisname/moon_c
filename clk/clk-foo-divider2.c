
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/module.h>

static DEFINE_SPINLOCK(clklock);

static void __init of_foo_divider_clk_setup(struct device_node *node)
{
	const char *parent_name;
	u32 width32 = 0;
	u8 width = 0;
	u8 shift = 0;
	u8 div_flags = 0;
	unsigned long flags = 0;
	const char *clk_name = node->name;
	void __iomem *reg;
	struct clk_hw *hw;

	parent_name = of_clk_get_parent_name(node, 0);

	if (of_property_read_bool(node, "foo,index-power-of-two"))
		div_flags |= CLK_DIVIDER_POWER_OF_TWO;

	if (of_property_read_bool(node, "foo,set-rate-parent"))
		flags |= CLK_SET_RATE_PARENT;

	if (of_property_read_u32(node, "foo,width", &width32)) {
		pr_err("no width for %pOFn!\n", node);
		return;
	}

	width = (u8) width32;

	reg = (void __iomem *) kmalloc(sizeof(void *), GFP_KERNEL);

        hw = clk_hw_register_divider(NULL, clk_name, parent_name,
                                   flags, reg, shift, width,
                                   div_flags, &clklock);
        if (IS_ERR(hw)) {
		pr_err("clk_hw_register_divider() failed.\n");
                return;
        }

        of_clk_add_hw_provider(node, of_clk_hw_simple_get, hw);
}
CLK_OF_DECLARE(divider_clk, "foo,divider-clock2", of_foo_divider_clk_setup);
