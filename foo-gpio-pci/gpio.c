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

#define USE_BGPIO

#define DRV_NAME "pci"
#define NR_GPIOS 32

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
	struct pci_dev *pdev;

	/* mmio control registers */
	void __iomem    *regs_base_addr;
	resource_size_t regs_start;
	resource_size_t regs_len;

	/* data mmio region */
	void __iomem    *data_base_addr;
	resource_size_t data_mmio_start;
	resource_size_t data_mmio_len;

	/* void __iomem *base; */

	raw_spinlock_t lock;

	struct gpio_chip gc;

	/* irq related */
#ifdef USE_BGPIO
	struct irq_chip_generic *icg;
	struct irq_domain *irq_domain;
	unsigned int irq;
#endif

	char (*msix_names)[256];
	struct msix_entry *msix_entries;
	int nvectors;
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

#ifdef USE_BGPIO
static int foo_gpio_to_irq(struct gpio_chip *gc, unsigned pin)
{
	struct foo_gpio *foo = gpiochip_get_data(gc);
	return irq_create_mapping(foo->irq_domain, pin);
}
#else
static unsigned long foo_read32(void __iomem *reg)
{
    return readl(reg);
}

static void foo_write32(void __iomem *reg, unsigned long data)
{
    writel(data, reg);
}

static int foo_gpio_request(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;

	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);

	return 0;
}

static void foo_gpio_free(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;

	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);
}

static int foo_gpio_direction_output(struct gpio_chip *gc,
		unsigned offset, int val)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;
	unsigned long mask, mask2;

	gc->set(gc, gpio, val);	/* set value */

	raw_spin_lock_irqsave(&foo->lock, flags);
	mask = gc->read_reg(foo->data_base_addr + VIRTUAL_GPIO_OUT_EN);
	mask2 = mask | (1 << offset);
	gc->write_reg(foo->data_base_addr + VIRTUAL_GPIO_OUT_EN, mask2);
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	printk(KERN_INFO "%s offset=%u, gpio=%u, mask2=0x%lx, init_val=%d\n",
			__func__, offset, gpio, mask2, val);

	return 0;
}

static int foo_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;
	unsigned long mask, mask2;

	raw_spin_lock_irqsave(&foo->lock, flags);
	mask = gc->read_reg(foo->data_base_addr + VIRTUAL_GPIO_OUT_EN);
	mask2 = mask & ~(1 << offset);
	gc->write_reg(foo->data_base_addr + VIRTUAL_GPIO_OUT_EN, mask2);
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	printk(KERN_INFO "%s offset=%u, gpio=%u, mask2=0x%lx\n", 
		__func__, offset, gpio, mask2);

	return 0;
}

static int foo_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;
	u32 mask;
	int val;

	raw_spin_lock_irqsave(&foo->lock, flags);
	mask = gc->read_reg(foo->data_base_addr + VIRTUAL_GPIO_DATA);
	val = !!(mask & (1 << offset));
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	printk(KERN_INFO "%s offset=%u, gpio=%u, val=%d\n",
			__func__, offset, gpio, val);

	return val;
}

static void foo_gpio_set(struct gpio_chip *gc, unsigned offset, int val)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;
	u32 mask;

	raw_spin_lock_irqsave(&foo->lock, flags);
	mask = gc->read_reg(foo->data_base_addr + VIRTUAL_GPIO_DATA);
	mask &= ~(1 << offset);
	gc->write_reg( foo->data_base_addr + VIRTUAL_GPIO_DATA, 
		mask | (!!(val) << offset));
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	printk(KERN_INFO "%s offset=%u, gpio=%u, val=%d\n",
			__func__, offset, gpio, val);
}

static int foo_gpio_to_irq(struct gpio_chip *gc, unsigned pin)
{
	return 0;
}
#endif



/*****************************************************************
 * 2) irq_chip part
 *****************************************************************/

int foo_count = 0;

static int foo_irq_set_type(struct irq_data *d, unsigned int type)
{
#ifdef USE_BGPIO
	struct irq_chip_generic *icg = irq_data_get_irq_chip_data(d);
	struct foo_gpio *foo = icg->private;
	struct gpio_chip *gc = &foo->gc;
#else
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct foo_gpio *foo = gpiochip_get_data(gc);
#endif
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

			/* enable interrupt */
			mask = gc->read_reg(foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN);
			gc->write_reg(foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN, mask | d->mask);
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

			/* enable interrupt */
			mask = gc->read_reg(foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN);
			gc->write_reg(foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN, mask | d->mask);
			break;

#if 0
		case IRQ_TYPE_EDGE_BOTH:
			mask = gc->read_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING);
			gc->write_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING, mask | d->mask);

			mask = gc->read_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_RISING);
			gc->write_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_RISING, mask | d->mask);

			/* enable interrupt */
			mask = gc->read_reg(foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN);
			gc->write_reg(foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN, mask | d->mask);
			break;

		case IRQ_TYPE_NONE:
			mask = gc->read_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING);
			gc->write_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING, mask & ~d->mask);

			mask = gc->read_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_RISING);
			gc->write_reg( foo->data_base_addr + 
					VIRTUAL_GPIO_RISING, mask & ~d->mask);

			/* disable interrupt */
			mask = gc->read_reg(foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN);
			gc->write_reg(foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN, mask & ~d->mask);
#endif
		default:
			retval = -EINVAL;
			goto end;
	}

	/* enable interrupt */
	mask = gc->read_reg(foo->data_base_addr + 
			VIRTUAL_GPIO_INT_EN);
	gc->write_reg(foo->data_base_addr + 
			VIRTUAL_GPIO_INT_EN, mask | d->mask);
end:
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	return 0;
}

#ifdef USE_BGPIO
static irqreturn_t foo_parent_irq_handler(int irq, void * private)
{
	struct foo_gpio * foo = private;
	struct gpio_chip *gc = &foo->gc;
	unsigned long status;
	unsigned long pending;
	unsigned int sub_irq, hwirq;

	foo_count++;
	if (unlikely(foo == NULL))
		return IRQ_NONE;

	status = gc->read_reg(foo->regs_base_addr + IntrStatus);
	if (!status || (status == 0xFFFFFFFFL))
		return IRQ_NONE;

	pending = gc->read_reg(foo->data_base_addr + VIRTUAL_GPIO_INT_ST);

	printk(KERN_INFO "%s irq=%d, status=0x%lx, pending=0x%lx\n", 
		__func__, irq, status, pending);

	/* check if irq is really raised */
	if (pending)
	{
		/* hwirq range is 0 ~ 31 */
		for_each_set_bit(hwirq, &pending, gc->ngpio) {
			sub_irq = irq_find_mapping(gc->irqdomain, hwirq);
			generic_handle_irq(sub_irq);
		}
	}

	/* temporary */
	gc->write_reg(foo->data_base_addr + VIRTUAL_GPIO_INT_EOI, 
			0xffffffff);

	printk(KERN_INFO "%s foo_count=%d, irq=%d, status=%lu\n", 
		__func__, foo_count, irq, status);

	return IRQ_HANDLED;
}

#else

static void foo_parent_gpio_irq_handler(struct irq_desc *desc)
{
	struct gpio_chip *gc = irq_desc_get_handler_data(desc);
	struct irq_chip *irq_chip = irq_desc_get_chip(desc);
	unsigned pin = 0;
	int child_irq = 0;

	foo_count++;
	chained_irq_enter(irq_chip, desc);

	child_irq = irq_find_mapping(gc->irqdomain, pin);

	generic_handle_irq(child_irq);

	chained_irq_exit(irq_chip, desc);
}

static void foo_irq_ack(struct irq_data *d)
{
        struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
        struct foo_gpio *foo = gpiochip_get_data(gc);
        unsigned long flags;
	u32 mask;
	u32 clear = 1 << d->hwirq;

        raw_spin_lock_irqsave(&foo->lock, flags);

	mask = gc->read_reg( foo->data_base_addr + VIRTUAL_GPIO_INT_EOI);
	gc->write_reg( foo->data_base_addr + VIRTUAL_GPIO_RISING, 
		mask & ~clear);

        printk(KERN_INFO "%s irq=%u, hwirq=%lu\n",
                        __func__, d->irq, d->hwirq);
        raw_spin_unlock_irqrestore(&foo->lock, flags);
}

static void foo_irq_mask(struct irq_data *d)
{
        struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
        struct foo_gpio *foo = gpiochip_get_data(gc);
        unsigned long flags;

        raw_spin_lock_irqsave(&foo->lock, flags);
        printk(KERN_INFO "%s irq=%u, hwirq=%lu\n",
                        __func__, d->irq, d->hwirq);
        raw_spin_unlock_irqrestore(&foo->lock, flags);
}

static void foo_irq_unmask(struct irq_data *d)
{
        struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
        struct foo_gpio *foo = gpiochip_get_data(gc);
        unsigned long flags;

        raw_spin_lock_irqsave(&foo->lock, flags);
        printk(KERN_INFO "%s irq=%u, hwirq=%lu\n",
                        __func__, d->irq, d->hwirq);
        raw_spin_unlock_irqrestore(&foo->lock, flags);
}

static struct irq_chip foo_gpio_irq_chip = {
	.name = "foo,foo-gpio-irq",
	.irq_ack = foo_irq_ack,
	.irq_mask = foo_irq_mask,
	.irq_unmask = foo_irq_unmask,
	.irq_set_type = foo_irq_set_type,
};

#endif

/*****************************************************************
 * 3) pci driver probe part
 *****************************************************************/

static int request_msix_vectors(struct foo_gpio *foo, int nvectors)
{
	int i, err;
	const char *name = DRV_NAME;

	foo->nvectors = nvectors;

	foo->msix_entries = kmalloc(nvectors * sizeof *foo->msix_entries,
			GFP_KERNEL);
	foo->msix_names = kmalloc(nvectors * sizeof *foo->msix_names,
			GFP_KERNEL);

	for (i = 0; i < nvectors; ++i)
		foo->msix_entries[i].entry = i;

	err = pci_enable_msix_exact(foo->pdev, foo->msix_entries,
			foo->nvectors);
	if (err > 0) {
		printk(KERN_INFO "no MSI. Back to INTx.\n");
		return -ENOSPC;
	}
	else if (err) {
		printk(KERN_INFO "some error below zero %d\n", err);
		return err;
	}

	for (i = 0; i < nvectors; i++) {

		snprintf(foo->msix_names[i], sizeof *foo->msix_names,
				"%s-config", name);

		err = request_irq(foo->msix_entries[i].vector,
				foo_parent_irq_handler, 0,
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
	u32 ngpios = NR_GPIOS;
	int parent_irq;
	int ret;
	int msix;
	void __iomem *data;
	void __iomem *dir;
	int irq_base;

	printk(KERN_INFO "%s\n", __func__);

	foo = devm_kzalloc(dev, sizeof(*foo), GFP_KERNEL);
	if (!foo)
		return -ENOMEM;

	gc = &foo->gc;
	foo->dev = dev;
	foo->pdev = pdev;

	raw_spin_lock_init(&foo->lock);

	/* pci related */
	if((ret = pcim_enable_device(pdev))){
		dev_err(dev, "pci_enable_device probe error %d for device %s\n",
				ret, pci_name(pdev));
		return ret;
	}

	if((ret = pci_request_regions(pdev, DRV_NAME)) < 0){
		dev_err(dev, "pci_request_regions error %d\n", ret);
		goto pci_disable;
	}

	/* bar2: data mmio region */
	foo->data_mmio_start = pci_resource_start(pdev, 2);
	foo->data_mmio_len   = pci_resource_len(pdev, 2);
	foo->data_base_addr = pcim_iomap(pdev, 2, 0);

	if (!foo->data_base_addr) {
		dev_err(dev, "cannot iomap region of size %lu\n",
				(unsigned long) foo->data_mmio_len);
		goto pci_release;
	}

	dev_info(dev, "bar2) data base=0x%lx\n",
			(unsigned long) foo->data_base_addr);

	dev_info(dev, "bar2) data start=0x%lx, len=0x%lx\n",
			(unsigned long) foo->data_mmio_start,
			(unsigned long) foo->data_mmio_len);


	/* bar0: control registers */
	foo->regs_start =  pci_resource_start(pdev, 0);
	foo->regs_len = pci_resource_len(pdev, 0);
	foo->regs_base_addr = pcim_iomap(pdev, 0, 0x100);

	if (!foo->regs_base_addr) {
		dev_err(dev, "cannot ioremap registers of size %lu\n",
				(unsigned long) foo->regs_len);
		goto pci_release;
	}

	dev_info(dev, "bar0) registers base=0x%lx \n",
			(unsigned long) foo->regs_base_addr);

	dev_info(dev, "bar0) registers start=0x%lx, len=0x%lx\n",
			(unsigned long) foo->regs_start,
			(unsigned long) foo->regs_len);

	parent_irq = pdev->irq;
	msix = parent_irq < 1;
	printk(KERN_INFO "%s parent_irq=%d\n", __func__, parent_irq);

	/* interrupts: set all masks */
	iowrite32(0xffff, foo->regs_base_addr + IntrMask);

	{
		struct irq_desc *desc = irq_to_desc(parent_irq);
		if (desc)
			dev_info(dev, "desc=%p, irq=%d, hwirq=%lu, "
					"status_use_accessors=0x%x\n", 
					desc, desc->irq_data.irq, 
					desc->irq_data.hwirq,
					desc->status_use_accessors);
	}

	if (!msix)
	{
		ret = devm_request_irq(dev, parent_irq,
				foo_parent_irq_handler, 
				IRQF_SHARED, DRV_NAME, foo);
		dev_info(dev, "request_irq() irq=%d, ret=%d\n", 
				parent_irq, ret);
	}

	/* virtual_gpio_setup() */
	gc->ngpio = ngpios;
	gc->label = dev_name(dev);
	gc->owner = THIS_MODULE;
	gc->to_irq = foo_gpio_to_irq;

#ifdef USE_BGPIO
	data = foo->data_base_addr + VIRTUAL_GPIO_DATA;
	dir = foo->data_base_addr + VIRTUAL_GPIO_OUT_EN;
	ret = bgpio_init(gc, dev, BITS_TO_BYTES(ngpios),
			data, NULL, NULL, dir, NULL, 0);
	if (ret) {
		dev_err(dev, "Failed to register GPIOs: bgpio_init\n");
		goto pci_release;
	}
#else
	gc->request = foo_gpio_request;
	gc->free = foo_gpio_free;
	gc->direction_input = foo_gpio_direction_input;
	gc->direction_output = foo_gpio_direction_output;
	gc->set = foo_gpio_set;
	gc->get = foo_gpio_get;
	gc->read_reg = foo_read32;
	gc->write_reg = foo_write32;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0))
	ret = devm_gpiochip_add_data(dev, gc, foo);
#else
	ret = devm_gpiochip_add(dev, gc);
#endif
	if (ret) {
		dev_err(dev, "unable to add GPIO chip\n");
		goto pci_release;
	}

	gc->parent = dev;

#if 0
	if (!msix)
	{
		/* chained gpio irq */
		foo->gc.parent = dev;

		ret = gpiochip_irqchip_add(gc, &foo_gpio_irq_chip, 0,
				handle_edge_irq, IRQ_TYPE_NONE);
		dev_info(dev, "gpiochip_irqchip_add() ret=%d\n", ret);
		if (ret) {
			dev_err(dev, "no GPIO irqchip\n");
			goto pci_release;
		}

		gpiochip_set_chained_irqchip(gc, &foo_gpio_irq_chip, parent_irq,
				foo_parent_gpio_irq_handler);
	}
#endif

#ifdef USE_BGPIO
	irq_base = devm_irq_alloc_descs(dev, -1, 0, ngpios, 0);
	if (irq_base < 0) {
		ret = irq_base;
		goto pci_release;
	}

	foo->irq_domain = irq_domain_add_linear(0, ngpios,
			&irq_domain_simple_ops, foo);
	if (!foo->irq_domain) {
		ret = -ENXIO;
		dev_err(&pdev->dev, "cannot initialize irq domain\n");
		goto pci_release;
	}

	irq_domain_associate_many(foo->irq_domain, irq_base, 0, ngpios);

	/* allocation irq_chip_generic */
	icg = devm_irq_alloc_generic_chip(dev, DRV_NAME, 1, irq_base,
			foo->data_base_addr, handle_edge_irq);
	if(!icg) {
		ret = -ENXIO;
		dev_err(dev, "irq_alloc_generic_chip failed!\n");
		goto pci_release;
	}

	ct = icg->chip_types;
	icg->private = foo;
	foo->icg = icg;

	ct->type = IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING;
	ct->chip.name = DRV_NAME;
	ct->chip.irq_ack = irq_gc_ack_set_bit;
	ct->chip.irq_mask = irq_gc_mask_clr_bit;
	ct->chip.irq_unmask = irq_gc_mask_set_bit;
	ct->chip.irq_set_type = foo_irq_set_type;

	ct->regs.ack = VIRTUAL_GPIO_INT_EOI;
	ct->regs.mask = VIRTUAL_GPIO_INT_EN;

	/* Setup a range of interrupts with a generic chip */
	devm_irq_setup_generic_chip(dev, icg, 
		IRQ_MSK(NR_GPIOS), 0, 0, 0);

	foo->gc.irqdomain = foo->irq_domain;

	/* not handler coz irq is nested (irq is not cascaded) */
	gpiochip_set_chained_irqchip(gc, &ct->chip, parent_irq, NULL);
#endif

	if (msix)
	{
		if (request_msix_vectors(foo, NR_GPIOS) != 0) {
			printk(KERN_INFO "MSI-X disabled\n");
		} else {
			printk(KERN_INFO "MSI-X enabled\n");
		}
	}

	pci_set_drvdata(pdev, foo);

	return 0;

pci_release:
	pci_release_regions(pdev);

pci_disable:
	return ret;
}

static void foo_gpio_remove(struct pci_dev *pdev)
{
	struct foo_gpio *foo = pci_get_drvdata(pdev);

	printk(KERN_INFO "%s foo_chip=%p, foo_count=%d\n", 
		__func__, foo, foo_count);

	pci_set_drvdata(pdev, NULL);
	pci_release_regions(pdev);
}


/*****************************************************************
 * 4) Module part                                         
 *****************************************************************/

static struct pci_device_id foo_gpio_id_table[] = {
	{ 0x1af4, 0x1110, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 32 },
	{ 0 },
};
MODULE_DEVICE_TABLE (pci, foo_gpio_id_table);

static struct pci_driver foo_gpio_driver = {
        .name      = DRV_NAME,
        .id_table  = foo_gpio_id_table,
	.probe = foo_gpio_probe,
	.remove= foo_gpio_remove,
};

/* arch_initcall_sync(foo_gpio_init); */
module_pci_driver(foo_gpio_driver);

MODULE_AUTHOR("moon_c");     /* Who wrote this module? */
MODULE_DESCRIPTION("moon_c");     /* What does this module do */
MODULE_SUPPORTED_DEVICE("testdevice");
