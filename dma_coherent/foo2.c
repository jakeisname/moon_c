#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/platform_device.h>                                              
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <linux/dma-direct.h>
#include <linux/of_reserved_mem.h>

struct foo {
	struct platform_device *pdev;
	void *virt_addr;
	phys_addr_t phys_addr;
	dma_addr_t dma_addr;
};

typedef struct foo foo_t;

static int foo_probe(struct platform_device *pdev)
{
	foo_t *foo;
	

	foo = devm_kmalloc(&pdev->dev, sizeof(foo_t), GFP_KERNEL);
	if (foo == NULL) 
		return -1;

	/* set private data */
	foo->pdev = pdev;
	platform_set_drvdata(pdev, foo);

	/* select foo-dev's memory */
	of_reserved_mem_device_init(&pdev->dev);

	/* test for dma buffer allocation */
	foo->virt_addr = dma_alloc_coherent(&pdev->dev, 100, &foo->dma_addr, GFP_KERNEL);
	if (foo->virt_addr == NULL)
		return -2;

	foo->phys_addr = dma_to_phys(&pdev->dev, foo->dma_addr);

	printk("%s: dma=%llx, phys=%llx, virt=%llx\n", __func__, 
			(uint64_t) foo->dma_addr, (uint64_t) foo->phys_addr, 
			(uint64_t) foo->virt_addr);

	memset(foo->virt_addr, 0xa0, 10);

	printk("%s: writed\n", __func__);

	return 0;
}

static int foo_remove(struct platform_device *pdev)
{
	foo_t *foo = (foo_t *)platform_get_drvdata(pdev);

	dma_free_coherent(&pdev->dev, 100, foo->virt_addr, foo->dma_addr);

	printk("%s\n", __func__);

	return 0;
}

static const struct of_device_id of_foo_match[] = {
	        { .compatible = "foo,foo-dev2", },
		{},
};
MODULE_DEVICE_TABLE(of, of_foo_match);

static struct platform_driver foo_driver2 = {
	.driver = {
		.name = "foo_driver2",
		.of_match_table = of_match_ptr(of_foo_match),
	},
	.probe          = foo_probe,
	.remove		= foo_remove,
};

module_platform_driver(foo_driver2);
MODULE_LICENSE("GPL");

