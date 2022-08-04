#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/clk/ti.h>
#include <linux/platform_device.h>
#include <linux/module.h>

struct foo_mux {
	struct clk		*clk;
        struct clk_hw           hw;
        u32			reg;
        u32                     *table;
        u32                     mask;
        u8                      shift;
        s8                      latch;
        u8                      flags;
        u8                      saved_parent;
};

#define to_foo_mux(_hw) container_of(_hw, struct foo_mux, hw)

static u32 foo_readl(u32 *reg)
{
	printk("%s: val=%u\n", __func__, *reg);
	return *reg;
}

static void foo_writel(u32 val, u32 *reg)
{
	*reg = val;
	printk("%s: val=%u\n", __func__, val);
	return;
}

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__

static u8 foo_clk_mux_get_parent(struct clk_hw *hw)
{
	struct foo_mux *foo = to_foo_mux(hw);
	int num_parents = clk_hw_get_num_parents(hw);
	u32 val;

	val = foo_readl(&foo->reg) >> foo->shift;
	val &= foo->mask;

	if (foo->table) {
		int i;

		for (i = 0; i < num_parents; i++)
			if (foo->table[i] == val)
				return i;
		return -EINVAL;
	}

	if (val && (foo->flags & CLK_MUX_INDEX_BIT))
		val = ffs(val) - 1;

	if (val && (foo->flags & CLK_MUX_INDEX_ONE))
		val--;

	if (val >= num_parents)
		return -EINVAL;

       	printk("%s: val=%u\n", __func__, val);

	return val;
}

static int foo_clk_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct foo_mux *foo = to_foo_mux(hw);
	u32 val;

       	printk("%s: index=%u\n", __func__, index);

	if (foo->table) {
		index = foo->table[index];
	} else {
		if (foo->flags & CLK_MUX_INDEX_BIT)
			index = (1 << ffs(index));

		if (foo->flags & CLK_MUX_INDEX_ONE)
			index++;
	}

	if (foo->flags & CLK_MUX_HIWORD_MASK) {
		val = foo->mask << (foo->shift + 16);
	} else {
		val = foo_readl(&foo->reg);
		val &= ~(foo->mask << foo->shift);
	}
	val |= index << foo->shift;
	foo_writel(val, &foo->reg);

       	printk("%s: index2=%u, val=%u\n", __func__, index, val);

	return 0;
}

static int clk_mux_save_context(struct clk_hw *hw)
{
	struct foo_mux *foo = to_foo_mux(hw);

       	printk("%s: \n", __func__);

	foo->saved_parent = foo_clk_mux_get_parent(hw);

	return 0;
}

static void clk_mux_restore_context(struct clk_hw *hw)
{
	struct foo_mux *foo = to_foo_mux(hw);

       	printk("%s: \n", __func__);

	foo_clk_mux_set_parent(hw, foo->saved_parent);
}

const struct clk_ops foo_clk_mux_ops = {
	.get_parent = foo_clk_mux_get_parent,
	.set_parent = foo_clk_mux_set_parent,
	.determine_rate = __clk_mux_determine_rate,
	.save_context = clk_mux_save_context,
	.restore_context = clk_mux_restore_context,
};

static struct foo_mux *_register_mux(struct device *dev, const char *name,
				 const char * const *parent_names,
				 u8 num_parents, unsigned long flags,
				 u8 shift, u32 mask,
				 s8 latch, u8 clk_mux_flags, u32 *table)
{
	struct foo_mux *foo;
	struct clk_init_data init;

	/* allocate the foo_mux */
	foo = devm_kzalloc(dev, sizeof(*foo), GFP_KERNEL);
	if (!foo)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.ops = &foo_clk_mux_ops;
	init.flags = flags;
	init.parent_names = parent_names;
	init.num_parents = num_parents;

	/* struct clk_mux assignments */
	foo->shift = shift;
	foo->mask = mask;
	foo->latch = latch;
	foo->flags = clk_mux_flags;
	foo->table = table;
	foo->hw.init = &init;

	foo->clk = clk_register(dev, &foo->hw);
	if (IS_ERR(foo->clk)) 
		return ERR_PTR(-EIO);

	/* success */
	return foo;
}

static struct foo_mux *_of_foo_mux_clk_setup(struct device *dev, 
		struct device_node *node)
{
	struct foo_mux *foo;
	unsigned int num_parents;
	const char **parent_names;
	u8 clk_mux_flags = 0;
	u32 mask = 0;
	u32 shift = 0;
	s32 latch = -EINVAL;
	u32 flags = CLK_SET_RATE_NO_REPARENT;

	num_parents = of_clk_get_parent_count(node);
	if (num_parents < 2) {
		pr_err("mux-clock %pOFn must have parents\n", node);
		return ERR_PTR(-ENOMEM);
	}
	parent_names = devm_kzalloc(dev, (sizeof(char *) * num_parents), GFP_KERNEL);
	if (!parent_names)
		return ERR_PTR(-EIO);

	of_clk_parent_fill(node, parent_names, num_parents);

	of_property_read_u32(node, "foo,bit-shift", &shift);

	of_property_read_u32(node, "foo,latch-bit", &latch);

	if (of_property_read_bool(node, "foo,index-starts-at-one"))
		clk_mux_flags |= CLK_MUX_INDEX_ONE;

	if (of_property_read_bool(node, "foo,set-rate-parent"))
		flags |= CLK_SET_RATE_PARENT;

	/* Generate bit-mask based on parent info */
	mask = num_parents;
	if (!(clk_mux_flags & CLK_MUX_INDEX_ONE))
		mask--;

	mask = (1 << fls(mask)) - 1;

	foo = _register_mux(dev, node->name, parent_names, num_parents,
			    flags, shift, mask, latch, clk_mux_flags,
			    NULL);
	if (!IS_ERR(foo))
		of_clk_add_provider(node, of_clk_src_simple_get, foo->clk);

	return foo;
}

static int of_foo_mux_clk_remove(struct platform_device *pdev)
{
	struct foo_mux *foo = platform_get_drvdata(pdev);

	if (!IS_ERR(foo->clk))
		clk_unregister(foo->clk);

	return 0;
}

static int of_foo_mux_clk_probe(struct platform_device *pdev)
{
	struct foo_mux *foo;

	/*
	 * This function is not executed when of_fixed_clk_setup
	 * succeeded.
	 */
	foo = _of_foo_mux_clk_setup(&pdev->dev, pdev->dev.of_node);
	if (IS_ERR(foo))
		return -EIO;

	platform_set_drvdata(pdev, foo);

	return 0;
}

static const struct of_device_id of_foo_mux_clk_ids[] = {
	{ .compatible = "foo,mux-clock" },
	{ }
};
MODULE_DEVICE_TABLE(of, of_foo_mux_clk_ids);

static struct platform_driver of_foo_mux_clk_driver = {
	.driver = {
		.name = "of_foo_mux_clk",
		.of_match_table = of_foo_mux_clk_ids,
	},
	.probe = of_foo_mux_clk_probe,
	.remove = of_foo_mux_clk_remove,
};
module_platform_driver(of_foo_mux_clk_driver);

MODULE_AUTHOR("Jake, Moon, https://jake.dothome.co.kr");
MODULE_DESCRIPTION("foo mux clock driver");
MODULE_LICENSE("GPL");
