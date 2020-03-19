#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/platform_device.h>                                              
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/export.h>                                              
#include <linux/of.h>
#include <linux/property.h>  

dma_addr_t phys1;
dma_addr_t phys2;
dma_addr_t phys3;
dma_addr_t phys4;
void *virt1;
void *virt2;
void *virt3;
void *virt4;

static int foo_probe(struct platform_device *pdev)
{ 
	struct device *dev = &pdev->dev;
	int pages = 32;	/* 4K * 32 = 128K */

        dev_info(dev, "%s\n", __func__);

//	virt1 = dma_alloc_wc(dev, pages, &phys1, GFP_KERNEL);
//	dev_info(dev, "virt1=0x%llx, phys1=0x%llx\n", (long long) virt1, phys1);

	virt2 = dmam_alloc_coherent(dev, pages, &phys2, GFP_KERNEL);
	dev_info(dev, "virt2=0x%llx, phys2=0x%llx\n", (long long) virt2, phys2);
	
	virt3 = dmam_alloc_attrs(dev, pages, &phys3, GFP_KERNEL, DMA_ATTR_WRITE_COMBINE);
	dev_info(dev, "virt3=0x%llx, phys3=0x%llx\n", (long long) virt3, phys3);

	virt4 = dmam_alloc_attrs(dev, pages, &phys4, GFP_KERNEL, DMA_ATTR_NO_KERNEL_MAPPING);
	dev_info(dev, "virt4=0x%llx, phys4=0x%llx\n", (long long) virt4, phys4);

        return 0;     /* 0=success */
}

static int foo_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

        dev_info(dev, "%s\n", __func__);

        return 0;
}


static const struct of_device_id foo_of_match_table[] = {
    { .compatible = "foo,foo" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, foo_of_match_table);

static struct platform_driver foo = {
        .probe = foo_probe,
	.remove = foo_remove,
        .driver = {
                .name = "foo",
                .of_match_table = foo_of_match_table,
        },
};

module_platform_driver(foo);

MODULE_AUTHOR("Jake, Moon, https://jake.dothome.co.kr");
MODULE_DESCRIPTION("foo driver");
MODULE_LICENSE("GPL");
