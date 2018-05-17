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
#include <linux/module.h>	/* for module programming */
#include <linux/init.h>
#include <linux/gpio/driver.h>
#include <linux/version.h>	/* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/printk.h>	/* KERN_INFO */
#include <linux/pci.h>

MODULE_LICENSE("GPL");

#ifndef DEBUG
#define DEBUG
#endif

#define VIRTUAL_GPIO_DEV_NAME "foo-gpio"
#define VIRTUAL_GPIO_NR_GPIOS 10

#define VIRTUAL_GPIO_DATA       0x00
#define VIRTUAL_GPIO_OUT_EN     0x04
#define VIRTUAL_GPIO_INT_EN     0x08
#define VIRTUAL_GPIO_INT_ST     0x0c
#define VIRTUAL_GPIO_INT_EOI    0x10
#define VIRTUAL_GPIO_RISING     0x14
#define VIRTUAL_GPIO_FALLING    0x18

#ifndef BITS_TO_BYTES
#define BITS_TO_BYTES(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE)
#endif

enum {
	/* KVM Inter-VM shared memory device register offsets */
	IntrMask        = 0x00,    /* Interrupt Mask */
	IntrStatus      = 0x04,    /* Interrupt Status */
	IVPosition      = 0x08,    /* VM ID */
	Doorbell        = 0x0c,    /* Doorbell */
};

struct foo_gpio {
	struct device *dev;
	struct pci_dev *pci_dev;

	/* mmio control registers */
	void __iomem    *regs_base_addr;
	resource_size_t regs_start;
	resource_size_t regs_len;

	/* data mmio region */
	void __iomem    *data_base_addr;
	resource_size_t data_mmio_start;
	resource_size_t data_mmio_len;

	/* void __iomem *base; */
	/* void __iomem *io_ctrl; */

	raw_spinlock_t lock;

	struct gpio_chip gc;

	/* irq related */
	struct irq_chip_generic *icg;
	struct irq_domain *irq_domain;
	unsigned num_banks;
	unsigned int irq;

	char (*msix_names)[256];
	struct msix_entry *msix_entries;
	int nvectors;

	bool pinmux_is_supported;

	struct pinctrl_dev *pctl;
	struct pinctrl_desc *pctldesc;
};

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
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;

	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);
	if (!foo->pinmux_is_supported)
		return 0;

	return pinctrl_request_gpio(gpio);
}

static void foo_gpio_free(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;

	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);
	if (!foo->pinmux_is_supported)
		return;

	pinctrl_free_gpio(gpio);
}

static int foo_gpio_direction_output(struct gpio_chip *gc,
		unsigned offset, int val)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;

	raw_spin_lock_irqsave(&foo->lock, flags);
	printk(KERN_INFO "%s offset=%u, gpio=%u, val=%d\n",
			__func__, offset, gpio, val);
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	return 0;
}

static int foo_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;

	raw_spin_lock_irqsave(&foo->lock, flags);
	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	return 0;
}

static int foo_val[512] = { 0,  };

static int foo_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	int val = foo_val[offset];
	unsigned gpio = gc->base + offset;
	printk(KERN_INFO "%s offset=%u, gpio=%u, val=%d\n",
			__func__, offset, gpio, val);
	return val;
}

static void foo_gpio_set(struct gpio_chip *gc, unsigned offset, int val)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;

	foo_val[offset] = val;
	raw_spin_lock_irqsave(&foo->lock, flags);
	printk(KERN_INFO "%s offset=%u, gpio=%u, val=%d\n",
			__func__, offset, gpio, val);
	raw_spin_unlock_irqrestore(&foo->lock, flags);
}


/*****************************************************************
 * irq_chip
 *****************************************************************/

static irqreturn_t foo_my_irq_handler(int irq, void * private)
{
	struct foo_gpio * foo = private;
	u32 status;

	if (unlikely(foo == NULL))
		return IRQ_NONE;

	status = readl(foo->regs_base_addr + IntrStatus);
	if (!status || (status == 0xFFFFFFFF))
		return IRQ_NONE;

	printk(KERN_INFO "%s status=0x%04x\n", __func__, status);

	return IRQ_HANDLED;
}

static void foo_gpio_irq_handler(struct irq_desc *desc)
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

static int foo_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct foo_gpio *foo = gpiochip_get_data(gc);
	unsigned long flags;
	int retval = 0;
	u32 mask;

	printk(KERN_INFO "%s irq=%u, hwirq=%lu, type=%u\n",
			__func__, d->irq, d->hwirq, type);
	raw_spin_lock_irqsave(&foo->lock, flags);

	switch (type) {
		case IRQ_TYPE_EDGE_RISING:
			mask = gc->read_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_RISING);
			gc->write_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_RISING, mask | d->mask);
			mask = gc->read_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING);
			gc->write_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING, mask & ~d->mask);
			break;
		case IRQ_TYPE_EDGE_FALLING:
			mask = gc->read_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING);
			gc->write_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING, mask | d->mask);

			mask = gc->read_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_RISING);
			gc->write_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_RISING, mask & ~d->mask);
			break;
		default:
			retval = -EINVAL;
			goto end;
	}

	/* enable interrupt */
	mask = gc->read_reg(foo->data_base_addr + VIRTUAL_GPIO_INT_EN);
	gc->write_reg(foo->data_base_addr + VIRTUAL_GPIO_INT_EN,
			mask | d->mask);
end:
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	return 0;
}

static struct irq_chip foo_gpio_irq_chip = {
	.name = "foo,foo-gpio-irq",
	.irq_ack = irq_gc_ack_set_bit,
	.irq_mask = irq_gc_mask_clr_bit,
	.irq_unmask = irq_gc_mask_set_bit,
	.irq_set_type = foo_gpio_irq_set_type,
};

static int foo_gpio_to_irq(struct gpio_chip *gc, unsigned pin)
{
	return 0;
}

/*****************************************************************
 * probe
 *****************************************************************/

static struct pci_device_id foo_gpio_id_table[] = {
	{ 0x1af4, 0x1110, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 32 },
	{ 0 },
};
MODULE_DEVICE_TABLE (pci, foo_gpio_id_table);


static int request_msix_vectors(struct foo_gpio *foo, int nvectors)
{
	int i, err;
	const char *name = "ivshmem";

	printk(KERN_INFO "devname is %s\n", name);
	foo->nvectors = nvectors;


	foo->msix_entries = kmalloc(nvectors * sizeof *foo->msix_entries,
			GFP_KERNEL);
	foo->msix_names = kmalloc(nvectors * sizeof *foo->msix_names,
			GFP_KERNEL);

	for (i = 0; i < nvectors; ++i)
		foo->msix_entries[i].entry = i;

	err = pci_enable_msix_exact(foo->pci_dev, foo->msix_entries,
			foo->nvectors);
	if (err > 0) {
		printk(KERN_INFO "no MSI. Back to INTx.\n");
		return -ENOSPC;
	}

	if (err) {
		printk(KERN_INFO "some error below zero %d\n", err);
		return err;
	}

	for (i = 0; i < nvectors; i++) {

		snprintf(foo->msix_names[i], sizeof *foo->msix_names,
				"%s-config", name);

		err = request_irq(foo->msix_entries[i].vector,
				foo_my_irq_handler, 0,
				foo->msix_names[i], foo);

		if (err) {
			printk(KERN_INFO "couldn't allocate irq for msi-x " \
					"entry %d with vector %d\n", 
					i, foo->msix_entries[i].vector);
			return -ENOSPC;
		}
	}

	return 0;
}

static int foo_gpio_probe(struct pci_dev *pdev,
		const struct pci_device_id *pdev_id)
{
	struct device *dev = &pdev->dev;
	struct foo_gpio *foo;
	struct gpio_chip *gc;
	struct irq_chip_generic *icg;
	struct irq_chip_type *ct;
	u32 ngpios;
	int irq, ret;
	void __iomem *data;
	void __iomem *dir;

	printk(KERN_INFO "%s\n", __func__);

	if (!of_device_is_compatible(dev->of_node, "foo,foo-gpio"))
		return -ENOMEM;

	foo = devm_kzalloc(dev, sizeof(*foo), GFP_KERNEL);
	if (!foo)
		return -ENOMEM;

	foo->dev = dev;
	pci_set_drvdata(pdev, foo);
	foo->pci_dev = pdev;

	if (of_property_read_u32(dev->of_node, "ngpios", &ngpios)) {
		dev_err(&pdev->dev, "missing ngpios DT property\n");
		return -ENODEV;
	}

	raw_spin_lock_init(&foo->lock);

	/* pci related */
	if((ret = pci_enable_device(pdev))){
		dev_err(dev, "pci_enable_device probe error %d for device %s\n",
				ret, pci_name(pdev));
		return ret;
	}

	if((ret = pci_request_regions(pdev, "foo-gpio-pci")) < 0){
		dev_err(dev, "pci_request_regions error %d\n", ret);
		goto pci_disable;
	}

	/* bar2: data mmio region */
	foo->data_mmio_start = pci_resource_start(pdev, 2);
	foo->data_mmio_len   = pci_resource_len(pdev, 2);
	foo->data_base_addr = ioremap_nocache(foo->data_mmio_start,
			foo->data_mmio_len);

	if (!foo->data_base_addr) {
		dev_err(dev, "cannot iomap region of size %lu\n",
				(unsigned long) foo->data_mmio_len);
		goto pci_release;
	}

	dev_dbg(dev, "bar2) data_mmio iomap base = 0x%lx \n",
			(unsigned long) foo->data_base_addr);

	dev_dbg(dev, "bar2) data_mmio_start = 0x%lx data_mmio_len = %lu\n",
			(unsigned long) foo->data_mmio_start,
			(unsigned long) foo->data_mmio_len);


	/* bar0: control registers */
	foo->regs_start =  pci_resource_start(pdev, 0);
	foo->regs_len = pci_resource_len(pdev, 0);
	foo->regs_base_addr = pci_iomap(pdev, 0, 0x100);
	if (!foo->regs_base_addr) {
		dev_err(dev, "cannot ioremap registers of size %lu\n",
				(unsigned long) foo->regs_len);
		goto reg_release;
	}

	dev_dbg(dev, "bar0) regs base = 0x%lx \n",
			(unsigned long) foo->regs_base_addr);

	dev_dbg(dev, "bar0) regs_start = 0x%lx regs_len = %lu\n",
			(unsigned long) foo->regs_start,
			(unsigned long) foo->regs_len);

	/* interrupts: set all masks */
	iowrite32(0xffff, foo->regs_base_addr + IntrMask);

	gc = &foo->gc;
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
	gc->to_irq = foo_gpio_to_irq;

	//foo->regs.ack = VIRTUAL_GPIO_INT_EOI; /* 0x10 */
	//     foo->regs.mask = VIRTUAL_GPIO_INT_EN; /* 0x08 */
	foo->num_banks = 1;
	foo->pinmux_is_supported = of_property_read_bool(dev->of_node,
			"gpio-ranges");

	data = foo->data_base_addr + VIRTUAL_GPIO_DATA;
	dir = foo->data_base_addr + VIRTUAL_GPIO_OUT_EN;
	ret = bgpio_init(gc, dev, BITS_TO_BYTES(ngpios),
			data, NULL, NULL, dir, NULL, 0);
	if (ret) {
		dev_err(dev, "Failed to register GPIOs: bgpio_init\n");
		goto reg_release;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0))
	ret = gpiochip_add_data(gc, foo);
#else
	ret = gpiochip_add(gc);
#endif
	if (ret < 0) {
		dev_err(dev, "unable to add GPIO chip\n");
		return ret;
	}

#if 0
	/* pinconf */

	if (!no_pinconf) {
		ret = foo_gpio_register_pinconf(foo);
		if (ret) {
			dev_err(dev, "unable to register pinconf\n");
			goto err_rm_gpiochip;
		}

		if (pinconf_disable_mask) {
			ret = foo_pinconf_disable_map_create(foo,
					pinconf_disable_mask);
			if (ret) {
				dev_err(dev,
						"unable to create pinconf disable map\n");
				goto err_rm_gpiochip;
			}
		}
	}
#endif

	/* chained gpio irq */

	foo->gc.parent = dev;

#if 0
	irq_base = irq_alloc_descs(-1, 0, ngpios, 0);
	if (irq_base < 0) {
		err = irq_base;
		goto desc_release;
	}

	foo->irq_domain = irq_domain_add_linear(0, ngpios,
			&irq_domain_simple_ops, foo);
	if (!foo->irq_domain) {
		err = -ENXIO;
		dev_err(&pdev->dev, "cannot initialize irq domain\n");
		goto desc_release;
	}

	irq_domain_associate_many(foo->irq_domain, irq_base, 0, ngpios);

	icg = irq_alloc_generic_chip(VIRTUAL_GPIO_DEV_NAME, 1, irq_base,
			foo->data_base_addr, handle_edge_irq);
	if(!icg) {
		dev_err(dev, "irq_alloc_generic_chip failed!\n");
		goto chip_release;
	}

	ct = icg->chip_types;
	icg->private = foo;

	foo->icg = icg;

	ct->type = IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING;

	ct->chip.name = VIRTUAL_GPIO_DEV_NAME;
	ct->chip.irq_ack = irq_gc_ack_set_bit;
	ct->chip.irq_mask = irq_gc_mask_clr_bit;
	ct->chip.irq_unmask = irq_gc_mask_set_bit;
	ct->chip.irq_set_type = virtual_gpio_irq_type;

	ct->regs.ack = VIRTUAL_GPIO_INT_EOI;
	ct->regs.mask = VIRTUAL_GPIO_INT_EN;

	irq_setup_generic_chip(gc, IRQ_MSK(VIRTUAL_GPIO_NR_GPIOS), 0, 0, 0);

	foo->gc.irqdomain = foo->irq_domain;

	gpiochip_set_chained_irqchip(gc, ct->chip, irq,
			foo_gpio_irq_handler);
#endif

	if (request_msix_vectors(foo, 1) != 0) {
		irq = pdev->irq;
		printk(KERN_INFO "KVM_IVSHMEM: irq=%u\n" , irq);
	} else {
		printk(KERN_INFO "MSI-X enabled\n");
	}

	printk(KERN_INFO "%s successed. foo_chip=%p\n", __func__, foo);

	return 0;

chip_release:
	free_irq(pdev->irq, foo);
	irq_remove_generic_chip(icg, 0, 0, 0);  
	kfree(gc);
desc_release:
	irq_free_descs(icg->irq_base, ngpios);
	pci_iounmap(pdev, foo->regs_base_addr);
	//err_rm_gpiochip:
	//     gpiochip_remove(gc);
reg_release:
	pci_iounmap(pdev, foo->data_base_addr);
	pci_set_drvdata(pdev, NULL);
pci_release:
	pci_release_regions(pdev);
pci_disable:
	pci_disable_device(pdev);

	return ret;
}

static void foo_gpio_remove(struct pci_dev *pdev)
{
	struct foo_gpio *foo = pci_get_drvdata(pdev);

	printk(KERN_INFO "%s foo_chip=%p\n", __func__, foo);
	gpiochip_remove(&foo->gc);
	pci_iounmap(pdev, foo->regs_base_addr);
	pci_iounmap(pdev, foo->data_base_addr);
	pci_set_drvdata(pdev, NULL);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}

/*--------------------------------------------------------*/
/* Module part                                            */
/*--------------------------------------------------------*/

static const struct of_device_id foo_gpio_of_match[] = {
	{ .compatible = "foo,foo-gpio" },
	{ /* sentinel */ }
};

static struct pci_driver foo_gpio_driver = {
	.driver = {
		.name = "foo-gpio",
		.owner = THIS_MODULE,
		.of_match_table = foo_gpio_of_match,
	},
	.probe = foo_gpio_probe,
	.remove= foo_gpio_remove,
};

/* arch_initcall_sync(foo_gpio_init); */
module_pci_driver(foo_gpio_driver);

MODULE_AUTHOR("moon_c");     /* Who wrote this module? */
MODULE_DESCRIPTION("moon_c");     /* What does this module do */
MODULE_SUPPORTED_DEVICE("testdevice");
