#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/module.h>       /* for module programming */
#include <linux/init.h>
#include <linux/gpio/driver.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/printk.h>	/* KERN_INFO */

MODULE_LICENSE("GPL");

#define DRV_NAME "foo"

struct foo_regs {
	unsigned int reg_dat;	/* gpio value */
	unsigned int reg_set;	/* set bit when write gpio value=1 */
	unsigned int reg_clr;	/* set bit when write gpio value=0 */
	unsigned int reg_dir;	/* gpio direction 0=input, 1=output */
};

struct foo_gpio {
	struct device		*dev;
	raw_spinlock_t		lock;
	struct gpio_chip	gc;
	struct foo_regs		regs;

	bool pinmux_is_supported;
	struct pinctrl_dev *pctl;
	struct pinctrl_desc *pctldesc;
};


/*****************************************************************
 * 1) gpio_chip part
 *****************************************************************/

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0))
static inline struct foo_gpio *foo_gpiochip_get_data(struct gpio_chip *gc2)
{
	return gpiochip_get_data(gc2);
}
#else
static inline struct foo_gpio *foo_gpiochip_get_data(struct gpio_chip *gc2)
{
	return container_of(gc2, struct foo_gpio, gc);
}
#endif


static int foo_gpio_request(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *chip = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;

	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);
	if (!chip->pinmux_is_supported)
		return 0;

	return pinctrl_request_gpio(gpio);
}

static void foo_gpio_free(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *chip = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;

	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);
	if (!chip->pinmux_is_supported)
		return;

	pinctrl_free_gpio(gpio);
}

static void _set_gpio_data(struct foo_gpio *chip, int offset, int value)
{
	if (value) {
		chip->regs.reg_dat |= (1 << offset);
		chip->regs.reg_set |= (1 << offset);
		chip->regs.reg_clr &= ~(1 << offset);
	}
	else {
		chip->regs.reg_dat &= ~(1 << offset);
		chip->regs.reg_set &= ~(1 << offset);
		chip->regs.reg_clr |= (1 << offset);
	}
}

static int foo_gpio_direction_output(struct gpio_chip *gc, 
	unsigned offset, int val)
{
	struct foo_gpio *chip = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;

	raw_spin_lock_irqsave(&chip->lock, flags);
	printk(KERN_INFO "%s offset=%u, gpio=%u, val=%d\n", 
		__func__, offset, gpio, !!val);
	chip->regs.reg_dir |= (1 << offset);
	_set_gpio_data(chip, offset, val);
	raw_spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int foo_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *chip = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;

	raw_spin_lock_irqsave(&chip->lock, flags);
	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);
	chip->regs.reg_dir &= ~(1 << offset);
	raw_spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int foo_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *chip = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;
	int val;

	raw_spin_lock_irqsave(&chip->lock, flags);
	val = !!(chip->regs.reg_dat & (1 << offset));
	printk(KERN_INFO "%s offset=%u, gpio=%u, val=%d\n", 
			__func__, offset, gpio, val);
	raw_spin_unlock_irqrestore(&chip->lock, flags);
	return val;
}

static void foo_gpio_set(struct gpio_chip *gc, unsigned offset, int val)
{
	struct foo_gpio *chip = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;

	raw_spin_lock_irqsave(&chip->lock, flags);
	_set_gpio_data(chip, offset, val);
	printk(KERN_INFO "%s offset=%u, gpio=%u, val=%d\n", 
		__func__, offset, gpio, val);
	raw_spin_unlock_irqrestore(&chip->lock, flags);
}


/*****************************************************************
 * 2) irq_chip
 *****************************************************************/

static void foo_parent_gpio_irq_handler(struct irq_desc *desc)
{
	struct gpio_chip *gc = irq_desc_get_handler_data(desc);
	struct irq_chip *irq_chip = irq_desc_get_chip(desc);
	unsigned pin = 0;
	int child_irq = 0;

	chained_irq_enter(irq_chip, desc);

	child_irq = irq_find_mapping(gc->irqdomain, pin);

	generic_handle_irq(child_irq);

	chained_irq_exit(irq_chip, desc);
}

static void foo_irq_ack(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct foo_gpio *chip = gpiochip_get_data(gc);
	unsigned long flags;

	raw_spin_lock_irqsave(&chip->lock, flags);
	printk(KERN_INFO "%s irq=%u, hwirq=%lu\n", 
			__func__, d->irq, d->hwirq);
	raw_spin_unlock_irqrestore(&chip->lock, flags);
}

static void foo_irq_mask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct foo_gpio *chip = gpiochip_get_data(gc);
	unsigned long flags;

	raw_spin_lock_irqsave(&chip->lock, flags);
	printk(KERN_INFO "%s irq=%u, hwirq=%lu\n", 
			__func__, d->irq, d->hwirq);
	raw_spin_unlock_irqrestore(&chip->lock, flags);
}

static void foo_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct foo_gpio *chip = gpiochip_get_data(gc);
	unsigned long flags;

	raw_spin_lock_irqsave(&chip->lock, flags);
	printk(KERN_INFO "%s irq=%u, hwirq=%lu\n", 
			__func__, d->irq, d->hwirq);
	raw_spin_unlock_irqrestore(&chip->lock, flags);
}

static int foo_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct foo_gpio *chip = gpiochip_get_data(gc);
	unsigned long flags;

	raw_spin_lock_irqsave(&chip->lock, flags);
	printk(KERN_INFO "%s irq=%u, hwirq=%lu, type=%u\n", 
		__func__, d->irq, d->hwirq, type);
	raw_spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static struct irq_chip foo_gpio_irq_chip = {
	.name = "foo,foo-gpio-irq",
        .irq_ack = foo_irq_ack,
        .irq_mask = foo_irq_mask,
        .irq_unmask = foo_irq_unmask,
	.irq_set_type = foo_irq_set_type,
};


/*****************************************************************
 * 3) gpio driver probe
 *****************************************************************/

static int foo_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct foo_gpio *chip;
	struct gpio_chip *gc;
	u32 ngpios;
	int parent_irq;
	int ret;

	printk(KERN_INFO "%s\n", __func__);

	if (!of_device_is_compatible(dev->of_node, "foo,foo-gpio"))
		return -ENOMEM;

        chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
        if (!chip)
                return -ENOMEM;

	chip->dev = dev;
	platform_set_drvdata(pdev, chip);

	if (of_property_read_u32(dev->of_node, "ngpios", &ngpios)) {
		dev_err(&pdev->dev, "missing ngpios DT property\n");
		return -ENODEV;
	}

	parent_irq = platform_get_irq(pdev, 0);
	if (parent_irq < 0) {
		dev_err(&pdev->dev, "Failed to get irq\n");
		return -ENODEV;
	}

	printk(KERN_INFO "%s ngpios=%d, parent_hwirq=%d\n", 
			__func__, ngpios, parent_irq);

	raw_spin_lock_init(&chip->lock);

	gc = &chip->gc;
	gc->base = -1;
	gc->ngpio = ngpios;
	gc->label = dev_name(dev);
	gc->parent = dev;
	gc->of_node = dev->of_node;
	gc->request = foo_gpio_request;
	gc->free = foo_gpio_free;
	gc->direction_input = foo_gpio_direction_input;
	gc->direction_output = foo_gpio_direction_output;
	gc->set = foo_gpio_set;
	gc->get = foo_gpio_get;

	chip->pinmux_is_supported = of_property_read_bool(dev->of_node,
			"gpio-ranges");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0))
	ret = devm_gpiochip_add_data(dev, gc, chip);
#else
	ret = devm_gpiochip_add(dev, gc);
#endif
	if (ret < 0) {
		dev_err(dev, "unable to add GPIO chip\n");
		return ret;
	}

#if 0
	/* pinconf */

	if (!no_pinconf) {
		ret = foo_gpio_register_pinconf(chip);
		if (ret) {
			dev_err(dev, "unable to register pinconf\n");
			return ret;
		}

		if (pinconf_disable_mask) {
			ret = foo_pinconf_disable_map_create(chip,
					pinconf_disable_mask);
			if (ret) {
				dev_err(dev,
				    "unable to create pinconf disable map\n");
				return ret;
			}
		}
	}
#endif

	/* chained gpio irq */

	ret = gpiochip_irqchip_add(gc, &foo_gpio_irq_chip, 0,
			handle_simple_irq, IRQ_TYPE_NONE);
	if (ret) {
		dev_err(dev, "no GPIO irqchip\n");
		return ret;
	}

	gpiochip_set_chained_irqchip(gc, &foo_gpio_irq_chip, parent_irq,
			foo_parent_gpio_irq_handler);

	printk(KERN_INFO "%s successed. chip=%p\n", 
		__func__, chip);

	return 0;
}

static int foo_gpio_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);

	return 0;
}


/*****************************************************************
 * 4) module part
 *****************************************************************/

static const struct of_device_id foo_gpio_of_match[] = {
	{ .compatible = "foo,foo-gpio" },
	{ /* sentinel */ }
};

static struct platform_driver foo_gpio_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = foo_gpio_of_match,
	},
	.probe = foo_gpio_probe,
	.remove= foo_gpio_remove,
};

/* arch_initcall_sync(foo_gpio_init); */
module_platform_driver(foo_gpio_driver);

MODULE_AUTHOR("moon_c");     /* Who wrote this module? */
MODULE_DESCRIPTION("moon_c");     /* What does this module do */
MODULE_SUPPORTED_DEVICE(DRV_NAME);

