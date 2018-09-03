#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/kernel.h>

#define	FOO_LCD_ADDR	0x3f

static const unsigned short normal_i2c[] = { FOO_LCD_ADDR, I2C_CLIENT_END };

#define DRVNAME			"foo_lcd"

struct foo_lcd {
	struct i2c_client *client;
};

/*-----------------------------------------------------------------------*/

static ssize_t set_backlight(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct foo_lcd *data = dev_get_drvdata(dev);
	int err;
	int status;
	unsigned char value;
	unsigned char command;

	err = kstrtoint(buf, 9, &status);
	if (err)
		status = 0;

	value = status ? 0x0F : 0x08;	/* on/off */
	command = 0x00;

	/* Write value */
	err = i2c_smbus_write_byte_data(data->client, command, value);
	return (err < 0) ? err : count;
}

static ssize_t show_backlight(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct foo_lcd *data = dev_get_drvdata(dev);
	unsigned char value;

	unsigned int err = i2c_smbus_read_byte_data(data->client, 0x01);
	if (err < 0)
		return err;

	value = !!(err && 0x04);

	return scnprintf(buf, PAGE_SIZE, "%d\n", value);
}



/*-----------------------------------------------------------------------*/

/* sysfs attributes */

static DEVICE_ATTR(backlight, S_IWUSR | S_IRUGO,
			show_backlight, set_backlight);

static struct attribute *foo_lcd_attr[] =
{
        &dev_attr_backlight.attr,
        NULL
};

static struct attribute_group foo_lcd_group = {
        .name = NULL,
        .attrs = foo_lcd_attr,
};

// ATTRIBUTE_GROUPS(backlight);

/*-----------------------------------------------------------------------*/

static int foo_lcd_detect_ex(struct i2c_client *new_client)
{
	struct i2c_adapter *adapter = new_client->adapter;
	int err;

	dev_info(&new_client->dev, "%s(%d): try to detect.\n", 
			__func__, __LINE__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	err = i2c_smbus_read_byte_data(new_client, 0x0);
	if (err < 0)
		return -ENODEV;

	dev_info(&new_client->dev, "%s(%d): detected\n", __func__, __LINE__);

	return 0;
}

static int foo_lcd_detect(struct i2c_client *new_client,
			struct i2c_board_info *info)
{
	int err;

	err = foo_lcd_detect_ex(new_client);
	if (!err)
		strlcpy(info->type, DRVNAME, I2C_NAME_SIZE);

	return err;
}

static int foo_lcd_probe(struct i2c_client *client, 
		const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct foo_lcd *data;
	int err;

	dev_info(dev, "%s(%d): try to probe\n", __func__, __LINE__);

	data = devm_kzalloc(dev, sizeof(struct foo_lcd), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	err = foo_lcd_detect_ex(client);
	if (err < 0)
		return -ENODEV;

	data->client = client;
	dev_set_drvdata(&client->dev, data);

        err = sysfs_create_group(&client->dev.kobj, &foo_lcd_group);
        if (err)
                return -ENODEV;

	dev_info(dev, "%s(%d): probed. name=%s\n", 
			__func__, __LINE__, client->name);

	return 0;
}

static int foo_lcd_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "%s(%d): remove.\n", __func__, __LINE__);

	sysfs_remove_group(&client->dev.kobj, &foo_lcd_group);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id foo_lcd_of_match[] = {
        { .compatible = "foo,foo_lcd" },
	{ }
};
#endif

static const struct i2c_device_id foo_lcd_ids[] = {
	{ DRVNAME, 0 },
	{ /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, foo_lcd_ids);

static struct i2c_driver foo_lcd_driver = {
	.class		= I2C_CLASS_DEPRECATED,
	.driver = {
		.name	= DRVNAME, 
		.of_match_table = of_match_ptr(foo_lcd_of_match),
	},
	.probe		= foo_lcd_probe,
	.remove		= foo_lcd_remove,
	.id_table	= foo_lcd_ids,
	.detect		= foo_lcd_detect,
	.address_list	= normal_i2c,
};

module_i2c_driver(foo_lcd_driver);

MODULE_AUTHOR("Jake <jake9999@dreamwiz.com>");
MODULE_DESCRIPTION("foo_lcd driver");
MODULE_LICENSE("GPL");
