#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/gfp.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/spinlock.h>

#include <linux/ctype.h>
#include <linux/file.h>
#include <linux/fs.h>

#define DRV_NAME	"foo_pci_1"

#define PCIE_FOO_VENDOR_ID 0x14E4 /* PCIE_CFG_TYPE0_EP_SUBSYSTEM_ID_VENDOR_ID */

#define FOR_MAPLE
#ifdef FOR_MAPLE
/* for Maple */
#define PCIE_FOO_DEVICE_ID 0x6862
#define BAR0_REG_OFFSET 0x64000U
#else
/* for Quamran AX */
#define PCIE_FOO_DEVICE_ID 0x8470
#define BAR0_REG_OFFSET 0x0U
#endif

/* remap address i
 * 1=n/a
 * 0x00000000 ~ 0xffffffff 
 */
ulong remap = 1;
module_param(remap, ulong, 0644);

/* pci domain number 
 * -1: all
 * 0~7: selected domain only
 */
int domain = -1;
module_param(domain, int, 0644);

static DEFINE_PCI_DEVICE_TABLE(foo_id_table) =
{ /* vendor_id,                  device_id,       any,       any */
    {PCI_DEVICE(PCIE_FOO_VENDOR_ID, PCIE_FOO_DEVICE_ID)},
    {0}
};
MODULE_DEVICE_TABLE (pci, foo_id_table);

typedef struct {
    struct pci_dev  *pdev;
    void __iomem    *base[6];
    phys_addr_t	    base_phys[6];
    size_t	    size[6];
    unsigned long   flags[6];
} foo_device_t;

static inline uint32_t foo_pci_read32(const volatile uint32_t *address)
{
        return readl(address);
}

static void do_remap(struct pci_dev *pdev)
{
    resource_size_t start;
    resource_size_t end;
    unsigned long size;

    if (remap != 1) {
	start = remap; /* 0x31000000; */
	end = remap | 0x7FFFFF;
	size = end - start + 1;

	dev_info(&pdev->dev,
		"pdev BAR0: force_remap: 0x%lx~0x%lx(old) -> 0x%lx~0x%lx(new), size=0x%lx\n", 
		(unsigned long) pdev->resource[0].start, 
		(unsigned long) pdev->resource[0].end, 
		(unsigned long) start, (unsigned long) end, (unsigned long) size);
	pdev->resource[0].start = start;
	pdev->resource[0].end = end;
    }
}

static void do_ioremap(struct pci_dev *pdev, foo_device_t *foo)
{
    int i;

    /* ioremap & display all BAR from pci_dev */
    for (i = 0; i < 6; i++) {
	foo->base_phys[i] = pci_resource_start(pdev, i);
	foo->size[i] = pci_resource_len(pdev, i);
	foo->flags[i] = pci_resource_flags(pdev, i);

	if (foo->size[i])
	    foo->base[i] = pci_ioremap_bar(pdev, i);

	dev_info(&pdev->dev,
		"pdev BAR%d: base_phys=0x%lx, base=0x%lx, size=0x%lx, flags=0x%lx\n",
		i, (unsigned long) foo->base_phys[i], 
		(unsigned long) foo->base[i], foo->size[i],
		(unsigned long) foo->flags[i]);
    }
}

static void show_info(struct pci_dev *pdev, foo_device_t *foo)
{
    int i, j;
    int val;
    uint32_t *addr;

    /* read BAR from config space */
    for (i = 0; i < 6; i++) {
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + i * 4, &val);
	printk("Read pci BAR%d=0x%x\n", i, val);
    }

    /* display hex for BAR0 */
    for (i = 0; i < 1; i++) {
	if (!foo->base[i])
	    continue;

	for (j = BAR0_REG_OFFSET; j < (BAR0_REG_OFFSET + 0x100); j+=4) {
	    addr = (uint32_t *) (foo->base[i] + j);
	    val = foo_pci_read32(addr);
	    if (j % 32 == 0) 
		printk("%16lx: ", (unsigned long) addr);
	    printk("%8x%s", val, j % 32 == 28 ? "\n" : " ");
	}
    }
}


static int foo_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    int rc;
    foo_device_t *foo;

    if ((domain != -1) && domain != pci_domain_nr(pdev->bus)) {
	dev_info(&pdev->dev,
		"Skip probe: domain=%d\n\n\n", pci_domain_nr(pdev->bus));
	return -ENODEV;
    }

    dev_info(&pdev->dev,
        "\n\nTry Probe: Domain=%d, Bus=%d, Slot=%d Vendor:device=0x%04x:0x%04x revision=0x%02x\n",
	pci_domain_nr(pdev->bus), pdev->bus->number, pdev->devfn,
        pdev->vendor, pdev->device, pdev->revision);

    foo = devm_kzalloc(&pdev->dev, sizeof(*foo), GFP_KERNEL);
    if (!foo)
	return -ENOMEM;

    rc = pcim_enable_device(pdev);
    if (rc) {
	dev_info(&pdev->dev, "pci_enable_device_mem() failed. rc=%d\n", rc);
	return -ENODEV;
    }

#if 0
    rc = pcim_iomap_regions(pdev, 0x15, DRV_NAME);
    if (rc) {
	dev_info(&pdev->dev, "pcim_iomap_regions() failed. rc=%d\n", rc);
	return -ENODEV;
    }
#else
    pci_assign_resource(pdev, 0);
    pci_assign_resource(pdev, 2);

#ifdef FOR_MAPLE
    pci_assign_resource(pdev, 4);
#else
#endif

    do_remap(pdev);

    rc = pci_request_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM), DRV_NAME);
    if (rc) {
	dev_info(&pdev->dev, "pci_request_selected_regions() failed. rc=%d\n", rc);
	return -ENODEV;
    }

    do_ioremap(pdev, foo);
#endif

    pci_set_master(pdev);

    foo->pdev = pdev;
    pci_set_drvdata(pdev, foo);

    show_info(pdev, foo);

    return 0;
}

static void foo_remove(struct pci_dev *pdev)
{
    int i;
    foo_device_t *foo = pci_get_drvdata(pdev);

#if 1
    for (i = 0; i < 6; i++) 
	if (foo->base[i]) {
	    iounmap((void *) pdev->resource[i].start);
    }
#endif

    pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
}


static struct pci_driver foo_pci_driver = {
    .name       = DRV_NAME,
    .id_table   = foo_id_table,
    .probe      = foo_probe,
    .remove     = foo_remove,
};

module_pci_driver(foo_pci_driver);

MODULE_DESCRIPTION("foo pci client driver");
MODULE_LICENSE("GPL");
