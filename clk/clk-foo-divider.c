
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define CLKF_INDEX_STARTS_AT_ONE	1
#define CLKF_INDEX_POWER_OF_TWO		2

struct foo_divider {
	struct device		*dev;
	struct clk		*clk;
        struct clk_hw           hw;
        u32			reg;
        u8                      shift;
        u8                      width;
        u8                      flags;
        s8                      latch;
        const struct clk_div_table      *table;
        u32             context;
};
#define to_foo_divider(_hw) container_of(_hw, struct foo_divider, hw)

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

#define div_mask(d)	((1 << ((d)->width)) - 1)

static unsigned int _get_table_maxdiv(const struct clk_div_table *table)
{
	unsigned int maxdiv = 0;
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div > maxdiv)
			maxdiv = clkt->div;
	return maxdiv;
}

static unsigned int _get_maxdiv(struct foo_divider *divider)
{
	if (divider->flags & CLK_DIVIDER_ONE_BASED)
		return div_mask(divider);
	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << div_mask(divider);
	if (divider->table)
		return _get_table_maxdiv(divider->table);
	return div_mask(divider) + 1;
}

static unsigned int _get_table_div(const struct clk_div_table *table,
				   unsigned int val)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->val == val)
			return clkt->div;
	return 0;
}

static unsigned int _get_div(struct foo_divider *divider, unsigned int val)
{
	if (divider->flags & CLK_DIVIDER_ONE_BASED)
		return val;
	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << val;
	if (divider->table)
		return _get_table_div(divider->table, val);
	return val + 1;
}

static unsigned int _get_table_val(const struct clk_div_table *table,
				   unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return clkt->val;
	return 0;
}

static unsigned int _get_val(struct foo_divider *divider, u8 div)
{
	if (divider->flags & CLK_DIVIDER_ONE_BASED)
		return div;
	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		return __ffs(div);
	if (divider->table)
		return  _get_table_val(divider->table, div);
	return div - 1;
}

static unsigned long foo_clk_divider_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	struct foo_divider *divider = to_foo_divider(hw);
	unsigned int div, val;
	unsigned long round;

	printk("%s: parent_rate=%lu\n", __func__, parent_rate);

	val = foo_readl(&divider->reg) >> divider->shift;
	val &= div_mask(divider);

	div = _get_div(divider, val);
	if (!div) {
		WARN(!(divider->flags & CLK_DIVIDER_ALLOW_ZERO),
		     "%s: Zero divisor and CLK_DIVIDER_ALLOW_ZERO not set\n",
		     clk_hw_get_name(hw));
		return parent_rate;
	}

	round = DIV_ROUND_UP(parent_rate, div);
	printk("%s: parent_rate=%lu, round=%lu\n", __func__, parent_rate, round);
	return round;
}

#define MULT_ROUND_UP(r, m) ((r) * (m) + (m) - 1)

static bool _is_valid_table_div(const struct clk_div_table *table,
				unsigned int div)
{
	const struct clk_div_table *clkt;

	for (clkt = table; clkt->div; clkt++)
		if (clkt->div == div)
			return true;
	return false;
}

static bool _is_valid_div(struct foo_divider *divider, unsigned int div)
{
	if (divider->flags & CLK_DIVIDER_POWER_OF_TWO)
		return is_power_of_2(div);
	if (divider->table)
		return _is_valid_table_div(divider->table, div);
	return true;
}

static int _div_round_up(const struct clk_div_table *table,
			 unsigned long parent_rate, unsigned long rate)
{
	const struct clk_div_table *clkt;
	int up = INT_MAX;
	int div = DIV_ROUND_UP_ULL((u64)parent_rate, rate);

	for (clkt = table; clkt->div; clkt++) {
		if (clkt->div == div)
			return clkt->div;
		else if (clkt->div < div)
			continue;

		if ((clkt->div - div) < (up - div))
			up = clkt->div;
	}

	return up;
}

static int _div_round(const struct clk_div_table *table,
		      unsigned long parent_rate, unsigned long rate)
{
	if (!table)
		return DIV_ROUND_UP(parent_rate, rate);

	return _div_round_up(table, parent_rate, rate);
}

static int foo_clk_divider_bestdiv(struct clk_hw *hw, unsigned long rate,
				  unsigned long *best_parent_rate)
{
	struct foo_divider *divider = to_foo_divider(hw);
	int i, bestdiv = 0;
	unsigned long parent_rate, best = 0, now, maxdiv;
	unsigned long parent_rate_saved = *best_parent_rate;

	if (!rate)
		rate = 1;

	maxdiv = _get_maxdiv(divider);
	printk("%s: maxdiv=%lu\n", __func__, maxdiv);

	if (!(clk_hw_get_flags(hw) & CLK_SET_RATE_PARENT)) {
		parent_rate = *best_parent_rate;
		bestdiv = _div_round(divider->table, parent_rate, rate);
		bestdiv = bestdiv == 0 ? 1 : bestdiv;
		bestdiv = bestdiv > maxdiv ? maxdiv : bestdiv;
		printk("%s: bestdiv=%d\n", __func__, bestdiv);
		return bestdiv;
	}

	/*
	 * The maximum divider we can use without overflowing
	 * unsigned long in rate * i below
	 */
	maxdiv = min(ULONG_MAX / rate, maxdiv);
	printk("%s: maxdiv2=%lu\n", __func__, maxdiv);

	for (i = 1; i <= maxdiv; i++) {
		if (!_is_valid_div(divider, i))
			continue;
		if (rate * i == parent_rate_saved) {
			/*
			 * It's the most ideal case if the requested rate can be
			 * divided from parent clock without needing to change
			 * parent rate, so return the divider immediately.
			 */
			*best_parent_rate = parent_rate_saved;
			return i;
		}
		parent_rate = clk_hw_round_rate(clk_hw_get_parent(hw),
				MULT_ROUND_UP(rate, i));
		now = DIV_ROUND_UP(parent_rate, i);
		if (now <= rate && now > best) {
			bestdiv = i;
			best = now;
			*best_parent_rate = parent_rate;
		}
	}

	if (!bestdiv) {
		bestdiv = _get_maxdiv(divider);
		*best_parent_rate =
			clk_hw_round_rate(clk_hw_get_parent(hw), 1);
	}
	printk("%s: bestdiv2=%d, best_parent_rate=%lu\n", __func__, bestdiv, *best_parent_rate);

	return bestdiv;
}

static long foo_clk_divider_round_rate(struct clk_hw *hw, unsigned long rate,
				      unsigned long *prate)
{
	int div;
	long round;

	div = foo_clk_divider_bestdiv(hw, rate, prate);
	round = DIV_ROUND_UP(*prate, div);

	printk("%s: rate=%lu, prate=%lu, round=%ld\n", __func__, rate, *prate, round);

	return round;
}

static int foo_clk_divider_set_rate(struct clk_hw *hw, unsigned long rate,
				   unsigned long parent_rate)
{
	struct foo_divider *divider;
	unsigned int div, value;
	u32 val;

	if (!hw || !rate)
		return -EINVAL;

	printk("%s: rate=%lu, parent_rate=%lu\n", __func__, rate, parent_rate);

	divider = to_foo_divider(hw);

	div = DIV_ROUND_UP(parent_rate, rate);
	value = _get_val(divider, div);

	if (value > div_mask(divider))
		value = div_mask(divider);

	if (divider->flags & CLK_DIVIDER_HIWORD_MASK) {
		val = div_mask(divider) << (divider->shift + 16);
	} else {
		val = foo_readl(&divider->reg);
		val &= ~(div_mask(divider) << divider->shift);
	}
	val |= value << divider->shift;
	foo_writel(val, &divider->reg);

	// ti_clk_latch(&divider->reg, divider->latch);

	return 0;
}

/**
 * clk_divider_save_context - Save the divider value
 * @hw: pointer  struct clk_hw
 *
 * Save the divider value
 */
static int clk_divider_save_context(struct clk_hw *hw)
{
	struct foo_divider *divider = to_foo_divider(hw);
	u32 val;

	printk("%s: \n", __func__);

	val = foo_readl(&divider->reg) >> divider->shift;
	divider->context = val & div_mask(divider);

	return 0;
}

/**
 * clk_divider_restore_context - restore the saved the divider value
 * @hw: pointer  struct clk_hw
 *
 * Restore the saved the divider value
 */
static void clk_divider_restore_context(struct clk_hw *hw)
{
	struct foo_divider *divider = to_foo_divider(hw);
	u32 val;

	printk("%s: \n", __func__);

	val = foo_readl(&divider->reg);
	val &= ~(div_mask(divider) << divider->shift);
	val |= divider->context << divider->shift;
	foo_writel(val, &divider->reg);
}

const struct clk_ops foo_clk_divider_ops = {
	.recalc_rate = foo_clk_divider_recalc_rate,
	.round_rate = foo_clk_divider_round_rate,
	.set_rate = foo_clk_divider_set_rate,
	.save_context = clk_divider_save_context,
	.restore_context = clk_divider_restore_context,
};

static struct foo_divider *_register_divider(struct device *dev, const char *name,
				     const char *parent_name,
				     unsigned long flags,
				     u8 shift, u8 width, s8 latch,
				     u8 clk_divider_flags,
				     const struct clk_div_table *table)
{
	struct foo_divider *div;
	struct clk_init_data init;

	if (clk_divider_flags & CLK_DIVIDER_HIWORD_MASK) {
		if (width + shift > 16) {
			pr_warn("divider value exceeds LOWORD field\n");
			return ERR_PTR(-EINVAL);
		}
	}

	/* allocate the divider */
	div = devm_kzalloc(dev, sizeof(*div), GFP_KERNEL);
	if (!div)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.ops = &foo_clk_divider_ops;
	init.flags = flags;
	init.parent_names = (parent_name ? &parent_name : NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct clk_divider assignments */
	// memcpy(&div->reg, reg, sizeof(*reg));
	div->shift = shift;
	div->width = width;
	div->latch = latch;
	div->flags = clk_divider_flags;
	div->hw.init = &init;
	div->table = table;

	/* register the clock */
	div->clk = clk_register(dev, &div->hw);
	if (IS_ERR(div->clk)) 
		div = ERR_PTR(-EIO);

	return div;
}


static struct clk_div_table *foo_clk_get_div_table(struct device *dev, 
		struct device_node *node)
{
	struct clk_div_table *table;
	const __be32 *divspec;
	u32 val;
	u32 num_div;
	u32 valid_div;
	int i;

	divspec = of_get_property(node, "foo,dividers", &num_div);

	if (!divspec)
		return NULL;

	num_div /= 4;

	valid_div = 0;

	/* Determine required size for divider table */
	for (i = 0; i < num_div; i++) {
		of_property_read_u32_index(node, "foo,dividers", i, &val);
		if (val)
			valid_div++;
	}

	if (!valid_div) {
		pr_err("no valid dividers for %pOFn table\n", node);
		return ERR_PTR(-EINVAL);
	}

	table = devm_kcalloc(dev, valid_div + 1, sizeof(*table), GFP_KERNEL);

	if (!table)
		return ERR_PTR(-ENOMEM);

	valid_div = 0;

	for (i = 0; i < num_div; i++) {
		of_property_read_u32_index(node, "foo,dividers", i, &val);
		if (val) {
			table[valid_div].div = val;
			table[valid_div].val = i;
			valid_div++;
		}
	}

	return table;
}

static int _get_divider_width(struct device_node *node,
			      const struct clk_div_table *table,
			      u8 flags)
{
	u32 min_div;
	u32 max_div;
	u32 val = 0;
	u32 div;

	if (!table) {
		/* Clk divider table not provided, determine min/max divs */
		if (of_property_read_u32(node, "foo,min-div", &min_div))
			min_div = 1;

		if (of_property_read_u32(node, "foo,max-div", &max_div)) {
			pr_err("no max-div for %pOFn!\n", node);
			return -EINVAL;
		}

		/* Determine bit width for the field */
		if (flags & CLK_DIVIDER_ONE_BASED)
			val = 1;

		div = min_div;

		while (div < max_div) {
			if (flags & CLK_DIVIDER_POWER_OF_TWO)
				div <<= 1;
			else
				div++;
			val++;
		}
	} else {
		div = 0;

		while (table[div].div) {
			val = table[div].val;
			div++;
		}
	}

	return fls(val);
}

static int foo_clk_divider_populate(struct device *dev, struct device_node *node,
	const struct clk_div_table **table,
	u32 *flags, u8 *div_flags, u8 *width, u8 *shift, s8 *latch)
{
	u32 val;

#if 0
	/* reg = < >; */
	ret = ti_clk_get_reg_addr(node, 0, reg);
	if (ret)
		return ret;
#endif

	if (!of_property_read_u32(node, "foo,bit-shift", &val))
		*shift = val;
	else
		*shift = 0;

	if (latch) {
		if (!of_property_read_u32(node, "foo,latch-bit", &val))
			*latch = val;
		else
			*latch = -EINVAL;
	}

	*flags = 0;
	*div_flags = 0;

	if (of_property_read_bool(node, "foo,index-starts-at-one"))
		*div_flags |= CLK_DIVIDER_ONE_BASED;

	if (of_property_read_bool(node, "foo,index-power-of-two"))
		*div_flags |= CLK_DIVIDER_POWER_OF_TWO;

	if (of_property_read_bool(node, "foo,set-rate-parent"))
		*flags |= CLK_SET_RATE_PARENT;

	*table = foo_clk_get_div_table(dev, node);
	if (IS_ERR(*table))
		return -EIO;

	*width = _get_divider_width(node, *table, *div_flags);

	printk("%s: width=%u\n", __func__, *width);

	return 0;
}

static struct foo_divider *_of_foo_divider_clk_setup(struct device *dev,
			  struct device_node *node)
{
	const char *parent_name;
	u8 clk_divider_flags = 0;
	u8 width = 0;
	u8 shift = 0;
	s8 latch = -EINVAL;
	u32 flags = 0;
	int ret;
	struct foo_divider *foo;
        const struct clk_div_table *table;

	parent_name = of_clk_get_parent_name(node, 0);

	if (foo_clk_divider_populate(dev, node, &table, &flags,
				    &clk_divider_flags, &width, &shift, &latch)) {
		return ERR_PTR(-EIO);
	}

	foo = _register_divider(dev, node->name, parent_name, flags, 
				shift, width, latch, clk_divider_flags, table);
	if (IS_ERR(foo)) 
		return foo;

	ret = of_clk_add_provider(node, of_clk_src_simple_get, foo->clk);
	if (!ret) {
		return foo;	/* success */
	}

	clk_unregister(foo->clk);

	return ERR_PTR(-EIO);
}

static int of_foo_divider_clk_remove(struct platform_device *pdev)
{
	struct foo_divider *foo = platform_get_drvdata(pdev);

	if (!IS_ERR(foo->clk)) 
		clk_unregister(foo->clk);

	return 0;
}

static int of_foo_divider_clk_probe(struct platform_device *pdev)
{
	struct foo_divider *foo;

	/*
	 * This function is not executed when of_fixed_clk_setup
	 * succeeded.
	 */
	foo = _of_foo_divider_clk_setup(&pdev->dev, pdev->dev.of_node);
	if (IS_ERR(foo))
		return -EIO;

	platform_set_drvdata(pdev, foo);

	return 0;
}

static const struct of_device_id of_foo_divider_clk_ids[] = {
	{ .compatible = "foo,divider-clock" },
	{ }
};
MODULE_DEVICE_TABLE(of, of_foo_divider_clk_ids);

static struct platform_driver of_foo_divider_clk_driver = {
	.driver = {
		.name = "of_foo_divider_clk",
		.of_match_table = of_foo_divider_clk_ids,
	},
	.probe = of_foo_divider_clk_probe,
	.remove = of_foo_divider_clk_remove,
};
module_platform_driver(of_foo_divider_clk_driver);

MODULE_AUTHOR("Jake, Moon, https://jake.dothome.co.kr");
MODULE_DESCRIPTION("foo divider clock driver");
MODULE_LICENSE("GPL");
