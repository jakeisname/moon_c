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
struct page *p1;
void *p2;
void *p3;
void *p4;

static int foo_probe(struct platform_device *pdev)
{ 
	struct device *dev = &pdev->dev;
	int pages = 32;	/* 4K * 32 = 128K */

        dev_info(dev, "%s\n", __func__);

//	p1 = dma_alloc_contiguous(dev, pages, GFP_KERNEL);
//	dev_info(dev, "p1=%llx\n", (long long) (void *) p1);

	p2 = dmam_alloc_coherent(dev, pages, &phys2, GFP_KERNEL);
	dev_info(dev, "p2=%llx\n", (long long) p2);
	
	p3 = dmam_alloc_attrs(dev, pages, &phys3, GFP_KERNEL, DMA_ATTR_WRITE_COMBINE);
	dev_info(dev, "p3=%llx\n", (long long) p3);

	p4 = dmam_alloc_attrs(dev, pages, &phys4, GFP_KERNEL, DMA_ATTR_NO_KERNEL_MAPPING);
	dev_info(dev, "p4=%llx\n", (long long) p4);

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
