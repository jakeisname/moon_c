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

#define DRV_NAME "foo_gpio"
#define NR_GPIOS 32

/* pci bar0 for ivshmem */
enum {
	/* KVM Inter-VM shared memory device register offsets */
	IntrMask        = 0x00,    /* Interrupt Mask */
	IntrStatus      = 0x04,    /* Interrupt Status */
	IVPosition      = 0x08,    /* VM ID */
	Doorbell        = 0x0c,    /* Doorbell */
};

/* bar1 for ivshmem (used by msi-x) */
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


struct foo_gpio;
struct foo_line {
	struct foo_gpio    *foo;
	unsigned int       line;
	unsigned int       fil_bits;
};

struct foo_gpio {
	struct device *dev;
	struct pci_dev *pdev;
	raw_spinlock_t lock;
	struct gpio_chip gc;

	/* mmio control registers */
	void __iomem    *regs_base_addr;
	resource_size_t regs_start;
	resource_size_t regs_len;

	/* data mmio region */
	void __iomem    *data_base_addr;
	resource_size_t data_mmio_start;
	resource_size_t data_mmio_len;

	/* msix related */
	char (*msix_names)[256];
	struct msix_entry *msix_entries;
	struct foo_line *line_entries;
	struct irq_domain *irqdomain;
	int nvectors;
	int base_msi;
};

static u32 foo_rw(struct gpio_chip *gc, void __iomem *addr, 
	u32 set, u32 clear)
{
	u32 mask, mask2;

	mask = (u32) gc->read_reg(addr);
	mask2 = mask | set;
	mask2 &= ~clear;
	gc->write_reg( addr, mask2 );

	return mask2;
}

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

static int foo_gpio_to_irq(struct gpio_chip *gc, unsigned pin)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
        int to_irq = pci_irq_vector(foo->pdev, pin);

	// int to_irq = irq_create_mapping(gc->irqdomain, pin);

	printk(KERN_INFO "%s pin=%d, to_irq=%d\n",
			__func__, pin, to_irq);

	return to_irq;
}

static unsigned long foo_read_reg(void __iomem *reg)
{
    return readl(reg);
}

static void foo_write_reg(void __iomem *reg, unsigned long data)
{
    writel(data, reg);
}

static int foo_gpio_request(struct gpio_chip *gc, unsigned offset)
{
	unsigned gpio = gc->base + offset;

	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);

	return 0;
}

static void foo_gpio_free(struct gpio_chip *gc, unsigned offset)
{
	unsigned gpio = gc->base + offset;

	printk(KERN_INFO "%s offset=%u, gpio=%u\n", __func__, offset, gpio);
}

static int foo_gpio_direction_output(struct gpio_chip *gc,
		unsigned offset, int val)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;
	u32 mask = 1 << offset;

	gc->set(gc, gpio, val);	/* set value */

	raw_spin_lock_irqsave(&foo->lock, flags);
	foo_rw(gc, foo->data_base_addr + VIRTUAL_GPIO_OUT_EN, mask, 0);
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	printk(KERN_INFO "%s offset=%u, gpio=%u, init_val=%d\n",
			__func__, offset, gpio, val);

	return 0;
}

static int foo_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct foo_gpio *foo = foo_gpiochip_get_data(gc);
	unsigned gpio = gc->base + offset;
	unsigned long flags;
	u32 mask = 1 << offset;

	raw_spin_lock_irqsave(&foo->lock, flags);
	foo_rw(gc, foo->data_base_addr + VIRTUAL_GPIO_OUT_EN, 0, mask);
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	printk(KERN_INFO "%s offset=%u, gpio=%u\n", 
		__func__, offset, gpio);

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
	u32 mask = 1 << offset;

	raw_spin_lock_irqsave(&foo->lock, flags);
	foo_rw(gc, foo->data_base_addr + VIRTUAL_GPIO_DATA, mask, 0);
	raw_spin_unlock_irqrestore(&foo->lock, flags);

	printk(KERN_INFO "%s offset=%u, gpio=%u, val=%d\n",
			__func__, offset, gpio, val);
}



/*****************************************************************
 * 2) irq_chip part
 *****************************************************************/

int foo_count = 0;

static int foo_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct foo_gpio *foo = gpiochip_get_data(gc);
	unsigned long flags;
	int ret = 0;
	u32 mask = (1 << d->hwirq);

	printk(KERN_INFO "%s irq=%u, hwirq=%lu, type=%u, mask=0x%x\n",
			__func__, d->irq, d->hwirq, type, mask);
	raw_spin_lock_irqsave(&foo->lock, flags);

	switch (type) {
		case IRQ_TYPE_EDGE_RISING:
			/* enable rising edge irq */
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_RISING, mask, 0);
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING, 0, mask);
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN, mask, 0);
			break;

		case IRQ_TYPE_EDGE_FALLING:
			/* enable falling edge irq */
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_RISING, 0, mask);
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING, mask, 0);
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN, mask, 0);
			break;

		case IRQ_TYPE_EDGE_BOTH:
			/* enable rising & falling edge irq */
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_RISING, mask, 0);
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING, mask, 0);
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN, mask, 0);
			break;

		case IRQ_TYPE_NONE:
			/* disable irq */
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_RISING, 0, mask);
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_FALLING, 0, mask);
			foo_rw(gc, foo->data_base_addr + 
					VIRTUAL_GPIO_INT_EN, 0, mask);
			break;
		default:
			ret = -EINVAL;
	}

	raw_spin_unlock_irqrestore(&foo->lock, flags);

	return ret;
}

static irqreturn_t foo_common_irq_handler(int irq, void * private)
{
	struct foo_gpio * foo = private;
	struct gpio_chip *gc = &foo->gc;
	unsigned long status;
	unsigned long pending;
	//unsigned int sub_irq, hwirq;

	foo_count++;
	if (unlikely(foo == NULL))
		return IRQ_NONE;

	status = gc->read_reg(foo->regs_base_addr + IntrStatus);
	if (!status || (status == 0xFFFFFFFFL))
		return IRQ_NONE;

	pending = gc->read_reg(foo->data_base_addr + VIRTUAL_GPIO_INT_ST);

	printk(KERN_INFO "%s irq=%d, status=0x%lx, pending=0x%lx\n", 
		__func__, irq, status, pending);

#if 0
	/* check if irq is really raised */
	if (pending)
	{
		/* hwirq range is 0 ~ 31 */
		for_each_set_bit(hwirq, &pending, gc->ngpio) {
			sub_irq = irq_find_mapping(gc->irqdomain, hwirq);
			printk(KERN_INFO "%s sub_irq=%d, hwirq=%d\n", 
				__func__, sub_irq, hwirq);

			generic_handle_irq(sub_irq);

			/* eoi */
			foo_rw(gc, foo->data_base_addr + VIRTUAL_GPIO_INT_EOI, 
				1 << hwirq, 0);
		}
	}
#endif

	return IRQ_HANDLED;
}


/* 
 * impossible to use sleep api 
*/
static void foo_chained_irq_handler(struct irq_desc *d)
{
	struct gpio_chip *gc = irq_desc_get_handler_data(d);
        struct foo_gpio *foo = gpiochip_get_data(gc);
	struct irq_chip *chip = irq_desc_get_chip(d);

	printk(KERN_INFO "%s irq=%d, hwirq=%lu\n", 
		__func__, d->irq_data.irq, d->irq_data.hwirq);

	chained_irq_enter(chip, d);
	foo_common_irq_handler(d->irq_data.irq, foo);
	chained_irq_exit(chip, d);
}

static void foo_irq_ack(struct irq_data *d)
{
        struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
        struct foo_gpio *foo = gpiochip_get_data(gc);
        unsigned long flags;
	u32 mask = 1 << d->hwirq;

        raw_spin_lock_irqsave(&foo->lock, flags);
	foo_rw(gc, foo->data_base_addr + VIRTUAL_GPIO_INT_EOI, 0, mask);
        raw_spin_unlock_irqrestore(&foo->lock, flags);

        printk(KERN_INFO "%s irq=%u, hwirq=%lu\n",
                        __func__, d->irq, d->hwirq);
}

static void foo_irq_mask(struct irq_data *d)
{
        struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
        struct foo_gpio *foo = gpiochip_get_data(gc);
        unsigned long flags;
	u32 mask = 1 << d->hwirq;
	u32 mask2;

        raw_spin_lock_irqsave(&foo->lock, flags);
	mask2=foo_rw(gc, foo->data_base_addr + VIRTUAL_GPIO_INT_EN, 0, mask);
        raw_spin_unlock_irqrestore(&foo->lock, flags);

        printk(KERN_INFO "%s irq=%u, hwirq=%lu, mask2=0x%x\n",
                        __func__, d->irq, d->hwirq, mask2);
}

static void foo_irq_unmask(struct irq_data *d)
{
        struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
        struct foo_gpio *foo = gpiochip_get_data(gc);
        unsigned long flags;
	u32 mask = 1 << d->hwirq;
	u32 mask2;

        raw_spin_lock_irqsave(&foo->lock, flags);
	mask2=foo_rw(gc, foo->data_base_addr + VIRTUAL_GPIO_INT_EN, mask, 0);
        raw_spin_unlock_irqrestore(&foo->lock, flags);

        printk(KERN_INFO "%s irq=%u, hwirq=%lu, mask2=0x%x\n",
                        __func__, d->irq, d->hwirq, mask2);
}

static void foo_irq_enable(struct irq_data *d)
{
        irq_chip_enable_parent(d);
        foo_irq_unmask(d);

        printk(KERN_INFO "%s irq=%u, hwirq=%lu\n",
                        __func__, d->irq, d->hwirq);
}

static void foo_irq_disable(struct irq_data *d)
{
        foo_irq_mask(d);
        irq_chip_disable_parent(d);

        printk(KERN_INFO "%s irq=%u, hwirq=%lu\n",
                        __func__, d->irq, d->hwirq);
}

static struct irq_chip foo_gpio_irq_chip = {
	.name = "foo,foo-gpio-irq",
	.irq_ack =	foo_irq_ack,
	.irq_mask =	foo_irq_mask,
	.irq_unmask =	foo_irq_unmask,
        .irq_enable =	foo_irq_enable,
        .irq_disable =	foo_irq_disable,
        .irq_eoi =		irq_chip_eoi_parent,
        .irq_set_affinity =	irq_chip_set_affinity_parent,
	.irq_set_type = foo_irq_set_type,
        .flags =	IRQCHIP_SET_TYPE_MASKED
};


/*****************************************************************
 * 3) irq_domain part
 *****************************************************************/

static int foo_irqd_map(struct irq_domain *d, unsigned int irq,
		irq_hw_number_t hwirq)
{
	printk(KERN_INFO "%s irq=%u, hwirq=%lu\n",
			__func__, irq, hwirq);

	if (hwirq >= NR_GPIOS)
		return -EINVAL;

	return 0;
}

static int foo_irqd_alloc(struct irq_domain *d, unsigned int virq,
		unsigned int nr_irqs, void *arg)
{
	struct foo_line *line = arg;

	printk(KERN_INFO "%s irq=%u, nr_irqs=%u\n",
			__func__, virq, nr_irqs);

	return irq_domain_set_hwirq_and_chip(d, virq, line->line,
			&foo_gpio_irq_chip, line);
}

static int foo_irqd_translate(struct irq_domain *d,
		struct irq_fwspec *fwspec,
		irq_hw_number_t *hwirq,
		unsigned int *type)
{
        // struct foo_gpio *foo = d->host_data;

        if (WARN_ON(fwspec->param_count < 2))
                return -EINVAL;

        if (fwspec->param[0] >= NR_GPIOS)
                return -EINVAL;

        *hwirq = fwspec->param[0];
        *type = fwspec->param[1] & IRQ_TYPE_SENSE_MASK;
        return 0;
}

static const struct irq_domain_ops foo_irqd_ops = {
        .map            = foo_irqd_map,
        .alloc          = foo_irqd_alloc,
        .translate      = foo_irqd_translate
};


/*****************************************************************
 * 4) pci driver probe part
 *****************************************************************/


static int request_msix_entries(struct foo_gpio *foo, int ngpios)
{
	struct device *dev = foo->dev;
	struct pci_dev *pdev = foo->pdev;
	int i;
	int err;

	foo->msix_entries = devm_kzalloc(dev,
			sizeof(struct msix_entry) * ngpios,
			GFP_KERNEL);
        if (!foo->msix_entries) {
                err = -ENOMEM;
                goto out;
        }

	foo->line_entries = devm_kzalloc(dev,
			sizeof(struct foo_line) * ngpios,
			GFP_KERNEL);
        if (!foo->line_entries) {
                err = -ENOMEM;
                goto out;
        }

        for (i = 0; i < ngpios; i++) {
                // u64 bit_cfg = readq(txgpio->register_base + bit_cfg_reg(i));

                foo->msix_entries[i].entry = i;
                foo->msix_entries[i].entry = foo->base_msi + i;
                foo->line_entries[i].line = i;
                foo->line_entries[i].foo = foo;
                foo->line_entries[i].fil_bits = 0;
        }


        err = pci_enable_msix_range(pdev, foo->msix_entries, ngpios, ngpios);
        if (err < 0)
	{
		dev_err(dev, "pci_enable_msix_range failed. err=%d\n", err);
                goto out;
	}

	foo->irqdomain = irq_domain_create_hierarchy(
			irq_get_irq_data(foo->msix_entries[0].vector)->domain,
			0, 0, of_node_to_fwnode(dev->of_node),
			&foo_irqd_ops, foo);
        if (!foo->irqdomain) {
		dev_err(dev, "irq_domain_create_hierachy failed.\n");
                err = -ENOMEM;
                goto out;
        }

        for (i = 0; i < ngpios; i++) {
                err = irq_domain_push_irq(foo->irqdomain,
                                          foo->msix_entries[i].vector,
                                          &foo->line_entries[i]);
                if (err < 0)
                        dev_err(dev, "irq_domain_push_irq: %d\n", err);
        }

out:
	return err;
}



static int request_msix_vectors(struct foo_gpio *foo, int nvectors)
{
	int i, ret;
	int sub_irq;
	int nvec = nvectors;
	struct pci_dev *pdev = foo->pdev;
	struct device *dev = foo->dev;

	nvec = pci_alloc_irq_vectors(pdev, 1, nvectors, PCI_IRQ_MSIX);
	dev_info(dev, "pci_alloc_irq_vectors nvec=%d\n", nvec);
	if (nvec < 0) {
		dev_err(dev, "pci_alloc_irq_vectors failed. nvec=%d\n", nvec);
		return -ENOSPC;
	}

        /* register interrupts */
        for (i = 0; i < nvec; i++) {
                sub_irq = pci_irq_vector(pdev, i);

                ret = devm_request_irq(dev, sub_irq,
                                       foo_common_irq_handler,
                                       0, DRV_NAME, foo);

		dev_info(dev, "devm_request_irq sub_irq=%d, err=%d\n", 
			sub_irq, ret);
                if (ret)
                        goto err;
        }

	return 0;

err:
	pci_free_irq_vectors(pdev);

	return -ENOSPC;
}

static int foo_gpio_probe(struct pci_dev *pdev,
		const struct pci_device_id *pdev_id)
{
	struct device *dev = &pdev->dev;
	struct foo_gpio *foo;
	struct gpio_chip *gc;
	u32 ngpios = NR_GPIOS;
	int parent_irq;
	int ret;
	struct irq_desc *desc;
#ifdef USE_BGPIO
	int irq_base;
	void __iomem *data;
	void __iomem *dir;
	struct irq_chip_generic *icg;
	struct irq_chip_type *ct;
#endif

	printk(KERN_INFO "%s\n", __func__);

	foo = devm_kzalloc(dev, sizeof(*foo), GFP_KERNEL);
	if (!foo)
		return -ENOMEM;

	gc = &foo->gc;
	foo->dev = dev;
	foo->pdev = pdev;
	foo->base_msi = 48;

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
	printk(KERN_INFO "%s parent_irq=%d\n", __func__, parent_irq);

	/* interrupts: set all masks */
	iowrite32(0xffffffff, foo->regs_base_addr + IntrMask);

	desc = irq_to_desc(parent_irq);
	if (desc)
		dev_info(dev, "desc=%p, irq=%d, hwirq=%lu, "
				"status_use_accessors=0x%x\n", 
				desc, desc->irq_data.irq, 
				desc->irq_data.hwirq,
				desc->status_use_accessors);

#if 0
	ret = request_msix_vectors(foo, 1);
	if (ret < 0)
		goto pci_release;
#endif
	ret = request_msix_entries(foo, 1);
	if (ret < 0)
		goto pci_release;

	gc->ngpio = ngpios;
	gc->label = dev_name(dev);
	gc->owner = THIS_MODULE;
	gc->base = -1;
	gc->request = foo_gpio_request;
	gc->free = foo_gpio_free;
	gc->direction_input = foo_gpio_direction_input;
	gc->direction_output = foo_gpio_direction_output;
	gc->set = foo_gpio_set;
	gc->get = foo_gpio_get;
	gc->read_reg = foo_read_reg;
	gc->write_reg = foo_write_reg;
	gc->to_irq = foo_gpio_to_irq;
	gc->parent = dev;

	ret = devm_gpiochip_add_data(dev, gc, foo);
	if (ret) {
		dev_err(dev, "unable to add GPIO chip\n");
		goto pci_release;
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
 * 5) Module part                                         
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
