
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>      /* LINUX_VERSION_CODE & KERNEL_VERSION() */
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/export.h>                                              
#include <linux/property.h>                                                     
#include <linux/interrupt.h>

//#include "/home/jake/workspace/lsk-4.14/drivers/pinctrl/core.h"
//#include "/home/jake/workspace/lsk-4.14/drivers/pinctrl/pinctrl-utils.h"
#include "/usr/src/lsk-4.14/drivers/pinctrl/core.h"
#include "/usr/src/lsk-4.14/drivers/pinctrl/pinctrl-utils.h"


struct foo_pinctrl {
	struct pinctrl_dev *pctrl_dev;
	struct device *dev;

	/* here.... pinmux & pinconf register base, ... */

	/* groups information */
	const struct foo_group *groups;
	unsigned int num_groups;

	/* function information */
	const struct foo_function *functions;
	unsigned int num_functions;
};


/****************************************************************************
 *
 * 1) Define groups & pinctrl operations
 *
 ****************************************************************************/

struct foo_group {
	const char *name;
	const unsigned int *pins;
	const unsigned int num_pins;
};

static const unsigned int gpio_0_3_pins[] = {0, 1, 2, 3};
static const unsigned int nand_0_3_pins[] = {0, 1, 2, 3};
static const unsigned int uart_0_3_pins[] = {0, 1, 2, 3};

static const unsigned int gpio_4_5_pins[] = {4, 5};
static const unsigned int nand_4_5_pins[] = {4, 5};
static const unsigned int uart_4_5_pins[] = {4, 5};

static const unsigned int gpio_6_7_pins[] = {6, 7};
static const unsigned int nand_6_7_pins[] = {6, 7};
static const unsigned int uart_6_7_pins[] = {6, 7};
static const unsigned int i2c_6_7_pins[] = {6, 7};

static const struct foo_group foo_groups[] = {
	{
		.name = "gpio_0_3_grp",
		.pins = gpio_0_3_pins,
		.num_pins = ARRAY_SIZE(gpio_0_3_pins),
	},
	{
		.name = "nand_0_3_grp",
		.pins = nand_0_3_pins,
		.num_pins = ARRAY_SIZE(nand_0_3_pins),
	},
	{
		.name = "uart_0_3_grp",
		.pins = uart_0_3_pins,
		.num_pins = ARRAY_SIZE(uart_0_3_pins),
	},
	{
		.name = "gpio_4_5_grp",
		.pins = gpio_4_5_pins,
		.num_pins = ARRAY_SIZE(gpio_4_5_pins),
	},
	{
		.name = "nand_4_5_grp",
		.pins = nand_4_5_pins,
		.num_pins = ARRAY_SIZE(nand_4_5_pins),
	},
	{
		.name = "uart_4_5_grp",
		.pins = uart_4_5_pins,
		.num_pins = ARRAY_SIZE(uart_4_5_pins),
	},
	{
		.name = "gpio_6_7_grp",
		.pins = gpio_6_7_pins,
		.num_pins = ARRAY_SIZE(gpio_6_7_pins),
	},
	{
		.name = "nand_6_7_grp",
		.pins = nand_6_7_pins,
		.num_pins = ARRAY_SIZE(nand_6_7_pins),
	},
	{
		.name = "uart_6_7_grp",
		.pins = uart_6_7_pins,
		.num_pins = ARRAY_SIZE(uart_6_7_pins),
	},
	{
		.name = "i2c_6_7_grp",
		.pins = i2c_6_7_pins,
		.num_pins = ARRAY_SIZE(i2c_6_7_pins),
	},
};

static int foo_get_groups_count(struct pinctrl_dev *pctrl_dev)
{
	struct foo_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	return pinctrl->num_groups;
}

static const char *foo_get_group_name(struct pinctrl_dev *pctrl_dev,
				      unsigned int selector)
{
	struct foo_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	return pinctrl->groups[selector].name;
}

static int foo_get_group_pins(struct pinctrl_dev *pctrl_dev,
			      unsigned int selector, const unsigned int **pins,
			      unsigned int *num_pins)
{
	struct foo_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	*pins = pinctrl->groups[selector].pins;
	*num_pins = pinctrl->groups[selector].num_pins;

	return 0;
}

static void foo_pin_dbg_show(struct pinctrl_dev *pctrl_dev,
			     struct seq_file *s, unsigned int offset)
{
	seq_printf(s, " %s", dev_name(pctrl_dev->dev));
}

static const struct pinctrl_ops foo_pinctrl_ops = {
	.get_groups_count = foo_get_groups_count,
	.get_group_name = foo_get_group_name,
	.get_group_pins = foo_get_group_pins,
	.pin_dbg_show = foo_pin_dbg_show,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_pin,
	.dt_free_map = pinctrl_utils_free_map,
};


/****************************************************************************
 *
 * 2) Define functions & pinmux operations
 *
 ****************************************************************************/

struct foo_function {
	const char *name;
	const char * const *groups;
	const unsigned int num_groups;
};

static const char * const gpio_grps[] = { "gpio_0_3_grp",
	"gpio_4_5_grp", "gpio_6_7_grp" };

static const char * const nand_grps[] = {
	"nand_0_3_grp", "nand_4_5_grp", "nand_6_7_grp" };

static const char * const uart_grps[] = {
	"uart_0_3_grp", "uart_4_5_grp", "uart_6_7_grp" };

static const char * const i2c_grps[] = {
	"i2c_6_7_grp" };

static const struct foo_function foo_functions[] = {
	{ 
		.name = "gpio", 
		.groups = gpio_grps, 
		.num_groups = ARRAY_SIZE(gpio_grps),
	},
	{ 
		.name = "nand", 
		.groups = nand_grps, 
		.num_groups = ARRAY_SIZE(nand_grps),
	},
	{ 
		.name = "uart", 
		.groups = uart_grps, 
		.num_groups = ARRAY_SIZE(uart_grps),
	},
	{ 
		.name = "i2c", 
		.groups = i2c_grps, 
		.num_groups = ARRAY_SIZE(i2c_grps),
	},
};

static int foo_get_functions_count(struct pinctrl_dev *pctrl_dev)
{
	struct foo_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	return pinctrl->num_functions;
}

static const char *foo_get_function_name(struct pinctrl_dev *pctrl_dev,
					 unsigned int selector)
{
	struct foo_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	return pinctrl->functions[selector].name;
}

static int foo_get_function_groups(struct pinctrl_dev *pctrl_dev,
				   unsigned int selector,
				   const char * const **groups,
				   unsigned int * const num_groups)
{
	struct foo_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);

	*groups = pinctrl->functions[selector].groups;
	*num_groups = pinctrl->functions[selector].num_groups;

	return 0;
}

static int foo_set_mux(struct pinctrl_dev *pctrl_dev,
			     unsigned int func_select, unsigned int grp_select)
{
	struct foo_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);
	const struct foo_function *func;
	const struct foo_group *grp;

	if (grp_select > pinctrl->num_groups ||
		func_select > pinctrl->num_functions)
		return -EINVAL;

	func = &pinctrl->functions[func_select];
	grp = &pinctrl->groups[grp_select];

	dev_info(pctrl_dev->dev, "func:%u name:%s grp:%u name:%s\n",
		func_select, func->name, grp_select, grp->name);

	return 0;
}

static const struct pinmux_ops foo_pinmux_ops = {
	.get_functions_count = foo_get_functions_count,
	.get_function_name = foo_get_function_name,
	.get_function_groups = foo_get_function_groups,
	.set_mux = foo_set_mux,
};


/****************************************************************************
 *
 * 3) Define pins & pinconf operations
 *
 ****************************************************************************/

static struct pinctrl_pin_desc foo_pins[] = {
	PINCTRL_PIN(0, "mfio_0"),
	PINCTRL_PIN(1, "mfio_1"),
	PINCTRL_PIN(2, "mfio_2"),
	PINCTRL_PIN(3, "mfio_3"),
	PINCTRL_PIN(4, "mfio_4"),
	PINCTRL_PIN(5, "mfio_5"),
	PINCTRL_PIN(6, "mfio_6"),
	PINCTRL_PIN(7, "mfio_7"),
};

struct foo_pin_drv_data {
	int bias;	/* 0=high-impedance, 1=pull-up, 2=pull-down */
	int mode;	/* 0=input, 1=output */
};

static int foo_pin_config_get(struct pinctrl_dev *pctrl_dev, unsigned int pin,
			      unsigned long *config)
{
	struct foo_pin_drv_data *drv_data = pctrl_dev->desc->pins[pin].drv_data;
	enum pin_config_param param = pinconf_to_config_param(*config);

	switch (param) {
		case PIN_CONFIG_INPUT_ENABLE:
			dev_info(pctrl_dev->dev, 
					"pin=%d, config=input-enable:%d\n",
					pin, drv_data->mode);
			break;

		case PIN_CONFIG_OUTPUT_ENABLE:
			dev_info(pctrl_dev->dev, 
					"pin=%d, config=output-enable:%d\n",
					pin, !drv_data->mode);
			break;

		case PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
			dev_info(pctrl_dev->dev, 
					"pin=%d, config=bias-high-impedance:%d\n",
					pin, drv_data->bias == 0);
			break;

		case PIN_CONFIG_BIAS_PULL_UP:
			dev_info(pctrl_dev->dev, 
					"pin=%d, config=bias-pull-up:%d\n",
					pin, drv_data->bias == 1);
			break;

		case PIN_CONFIG_BIAS_PULL_DOWN:
			dev_info(pctrl_dev->dev, 
					"pin=%d, config=bias-pull-down:%d\n",
					pin, drv_data->bias == 1);
			break;


		default:
			return -ENOTSUPP;
	}

	return 0; /* success */
}

static int foo_pin_config_set(struct pinctrl_dev *pctrl_dev, unsigned int pin,
			      unsigned long *configs, unsigned int num_configs)
{
	struct foo_pin_drv_data *drv_data = pctrl_dev->desc->pins[pin].drv_data;
	enum pin_config_param param;
	unsigned int i;
	u32 arg;

	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(configs[i]);
		arg = pinconf_to_config_argument(configs[i]);

		switch (param) {
			case PIN_CONFIG_INPUT_ENABLE:
				drv_data->mode = 0;
				dev_info(pctrl_dev->dev, 
						"pin:%u set input-enable\n", 
						pin);
				break;

			case PIN_CONFIG_OUTPUT_ENABLE:
				drv_data->mode = 1;
				dev_info(pctrl_dev->dev, 
						"pin:%u set output-enable\n", 
						pin);
				break;

			case PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
				drv_data->bias = 0;
				dev_info(pctrl_dev->dev, 
						"pin:%u set bias-high-impedance\n", 
						pin);
				break;

			case PIN_CONFIG_BIAS_PULL_UP:
				drv_data->bias = 1;
				dev_info(pctrl_dev->dev, 
						"pin:%u set bias-pull-up\n", 
						pin);
				break;

			case PIN_CONFIG_BIAS_PULL_DOWN:
				drv_data->bias = 2;
				dev_info(pctrl_dev->dev, 
						"pin:%u set bias-pull-down\n", 
						pin);
				break;

		default:
			dev_err(pctrl_dev->dev, "invalid configuration\n");
			return -ENOTSUPP;
		}
	}

	return 0; /* success */
}

static const struct pinconf_ops foo_pinconf_ops = {
	.is_generic = true,
	.pin_config_get = foo_pin_config_get,
	.pin_config_set = foo_pin_config_set,
};


/****************************************************************************
 *
 * foo Pinctrl Driver
 *
 ****************************************************************************/

static struct pinctrl_desc foo_pinctrl_desc = {
	.name = "foo-pinmux",
	.pctlops = &foo_pinctrl_ops,
	.pmxops = &foo_pinmux_ops,
	.confops = &foo_pinconf_ops,
};

static struct foo_pinctrl my_pinctrl = {
	.groups = foo_groups,
	.num_groups = ARRAY_SIZE(foo_groups),
	.functions = foo_functions,
	.num_functions = ARRAY_SIZE(foo_functions),
};

static int foo_pinctrl_probe(struct platform_device *pdev)
{
	struct foo_pinctrl *pinctrl = &my_pinctrl;
	struct foo_pin_drv_data *drv_data;
	unsigned int num_pins = ARRAY_SIZE(foo_pins);
	int i;

	/*
         * 1) allocate foo_pinctrl 
         */

#if 0
	pinctrl = devm_kzalloc(&pdev->dev, sizeof(*pinctrl), GFP_KERNEL);
	if (!pinctrl)
		return -ENOMEM;
#endif

	pinctrl->dev = &pdev->dev;
	platform_set_drvdata(pdev, pinctrl);

#if 0
	pinctrl->groups = foo_groups;
	pinctrl->num_groups = ARRAY_SIZE(foo_groups);
	pinctrl->functions = foo_functions;
	pinctrl->num_functions = ARRAY_SIZE(foo_functions);
#endif
	/* 
	 * 2) allocate array of pin drv_data 
	 */

	for (i = 0; i < num_pins; i++) {
		drv_data = devm_kzalloc(&pdev->dev, 
				sizeof(struct foo_pin_drv_data), GFP_KERNEL);
		if (!drv_data)
			return -ENOMEM;

		foo_pins[i].drv_data = (void *) drv_data;
	}

	/* 
	 * 3) register pinctrl_desc 
	 */

	foo_pinctrl_desc.pins = foo_pins;
	foo_pinctrl_desc.npins = num_pins;

	pinctrl->pctrl_dev = pinctrl_register(&foo_pinctrl_desc, &pdev->dev,
			pinctrl);
	if (IS_ERR(pinctrl->pctrl_dev)) {
		dev_err(&pdev->dev, "unable to register foo pinctrl\n");
		return PTR_ERR(pinctrl->pctrl_dev);
	}

	return 0;
}

static int foo_pinctrl_remove(struct platform_device *pdev)
{
	struct foo_pinctrl *pinctrl = platform_get_drvdata(pdev);

	pinctrl_unregister(pinctrl->pctrl_dev);

	return 0;
}


static const struct of_device_id foo_pinctrl_of_match[] = {
	{.compatible = "foo,foo-pinctrl"},
	{ }
};
MODULE_DEVICE_TABLE(of, foo_pinctrl_of_match);

static struct platform_driver foo_pinctrl_driver = {
	.probe = foo_pinctrl_probe,
	.driver = {
		.name = "foo-pinctrl",
		.of_match_table = foo_pinctrl_of_match,
	},
	.remove = foo_pinctrl_remove,
};
module_platform_driver(foo_pinctrl_driver);
MODULE_LICENSE("GPL");

