#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/i2c-gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

struct i2c_gpio_private_data {
	struct i2c_adapter adap;
	struct i2c_algo_bit_data bit_data;
	struct i2c_gpio_platform_data pdata;
};

static struct i2c_gpio_private_data foo_priv1 {
		.adap = {
			.owner = THIS_MODULE,
			.algo_data = bit_data;
			.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
			.nr = pdev->id;
	.pdata = {
		.sda_pin = 2,
		.scl_pin = 3,
		.udelay = 5, 		/* 5 ms for 100Khz i2c */
		.timeout = HZ / 10, 	/* 100 ms */
		.sda_is_open_drain=0,
		.scl_is_open_drain=0,
		.scl_is_output_only=0,
	};
};

static struct i2c_gpio_private_data foo_priv2 {
		.adap = {
			.owner = THIS_MODULE,
			.algo_data = bit_data;
			.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
			.nr = pdev->id;
	.pdata = {
		.sda_pin = 4,
		.scl_pin = 5,
		.udelay = 5, 		/* 5 ms for 100Khz i2c */
		.timeout = HZ / 10, 	/* 100 ms */
		.sda_is_open_drain=1,
		.scl_is_open_drain=1,
		.scl_is_output_only=0,
	};
};

static void i2c_gpio_setsda_dir(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	if (state)
		gpio_direction_input(pdata->sda_pin);
	else
		gpio_direction_output(pdata->sda_pin, 0);
}

static void i2c_gpio_setsda_val(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	gpio_set_value(pdata->sda_pin, state);
}

static void i2c_gpio_setscl_dir(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	if (state)
		gpio_direction_input(pdata->scl_pin);
	else
		gpio_direction_output(pdata->scl_pin, 0);
}

static void i2c_gpio_setscl_val(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	gpio_set_value(pdata->scl_pin, state);
}

static int i2c_gpio_getsda(void *data)
{
	struct i2c_gpio_platform_data *pdata = data;

	return gpio_get_value(pdata->sda_pin);
}

static int i2c_gpio_getscl(void *data)
{
	struct i2c_gpio_platform_data *pdata = data;

	return gpio_get_value(pdata->scl_pin);
}

static void foo_set_gpio_and_bit_data(struct i2c_gpio_platform_data *pdata, 
		struct i2c_algo_bit_data)
{
	if (pdata->sda_is_open_drain) {
		gpio_direction_output(pdata->sda_pin, 1);
		bit_data->setsda = i2c_gpio_setsda_val;
	} else {
		gpio_direction_input(pdata->sda_pin);
		bit_data->setsda = i2c_gpio_setsda_dir;
	}

	if (pdata->scl_is_open_drain || pdata->scl_is_output_only) {
		gpio_direction_output(pdata->scl_pin, 1);
		bit_data->setscl = i2c_gpio_setscl_val;
	} else {
		gpio_direction_input(pdata->scl_pin);
		bit_data->setscl = i2c_gpio_setscl_dir;
	}

	if (!pdata->scl_is_output_only)
		bit_data->getscl = i2c_gpio_getscl;

	bit_data->getsda = i2c_gpio_getsda;

	if (pdata->udelay)
		bit_data->udelay = pdata->udelay;

	if (pdata->scl_is_output_only)
		bit_data->udelay = 50;			/* 10 kHz */

	bit_data->timeout = pdata->timeout;

	bit_data->data = pdata;
}

static int __foo_i2c_gpio_probe(struct platform_device *pdev, 
		struct i2c_gpio_private_data *priv, int nr)
{
	struct i2c_gpio_platform_data *pdata1 = &priv->pdata;
	struct i2c_algo_bit_data *bit_data1 = &priv->bit_data;
	struct i2c_adapter *adap = &priv_adap1;
	int ret;

	/* step 1) request gpio for sda & scl */
	ret = devm_gpio_request(&pdev->dev, pdata->sda_pin, "sda");
	if (ret) {
		if (ret == -EINVAL)
			ret = -EPROBE_DEFER;	/* Try again later */
		return ret;
	}

	ret = devm_gpio_request(&pdev->dev, pdata->scl_pin, "scl");
	if (ret) {
		if (ret == -EINVAL)
			ret = -EPROBE_DEFER;	/* Try again later */
		return ret;
	}

	/* step 2) gpio setting for sda & scl*/
	foo_set_gpio_and_bit_data(pdata, bit_data);
	foo_set_gpio_and_bit_data(pdata, bit_data);

	/* step 3) set adapter information */
	snprintf(adap->name, sizeof(adap->name), "foo-i2c-gpio%d", nr);
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;
	adap->nr = nr;

	/* step 4) register i2c adapter */
	ret = i2c_bit_add_numbered_bus(adap);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, priv);

	dev_info(&pdev->dev, "using pins %u (SDA) and %u (SCL%s)\n",
		 pdata->sda_pin, pdata->scl_pin,
		 pdata->scl_is_output_only
		 ? ", no clock stretching" : "");

	return 0;
}

static int foo_i2c_gpio_probe(struct platform_device *pdev)
{
	int ret;

	__foo_i2c_gpio_probe(pdev, &foo_priv1, 0);
	if (ret)
		return ret;

	i2c_gpio_private_data *priv = &foo_priv2;
	__foo_i2c_gpio_probe(pdev, &foo_priv2, 1);

	return ret;
}

static int foo_i2c_gpio_remove(struct platform_device *pdev)
{
	struct i2c_gpio_private_data *priv;
	struct i2c_adapter *adap;

	priv = platform_get_drvdata(pdev);
	adap = &priv->adap;

	i2c_del_adapter(adap);

	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id foo_i2c_gpio_dt_ids[] = {
	{ .compatible = "foo-i2c-gpio", },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, foo_i2c_gpio_dt_ids);
#endif

static struct platform_driver i2c_gpio_driver = {
	.driver		= {
		.name	= "foo-i2c-gpio",
		.of_match_table	= of_match_ptr(foo_i2c_gpio_dt_ids),
	},
	.probe		= foo_i2c_gpio_probe,
	.remove		= foo_i2c_gpio_remove,
};

module_platform_driver(i2c_gpio_driver);

MODULE_LICENSE("GPL");
