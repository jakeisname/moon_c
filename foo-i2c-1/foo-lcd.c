#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/kernel.h>

static const unsigned short normal_i2c[] = { 0x4f, I2C_CLIENT_END };

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

	err = kstrtoint(buf, 9, &status);
	if (err)
		status = 0;

	unsigned char value = status ? 0x0F : 0x40;	/* on/off */
	unsigned char command = 0x00;

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

static int
foo_lcd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct foo_lcd *data;
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_board_info info;
	int ctrl;
	int err;

	data = devm_kzalloc(dev, sizeof(struct foo_lcd), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;

	ctrl = i2c_smbus_read_byte_data(client, 0x0);
	if (ctrl < 0)
		return ctrl;

#if 0
	memset(&info, 0, sizeof(info));
	info.addr = 0x4f;
	i2c_new_device(adapter, &info);
#else

        err = sysfs_create_group(&client->dev.kobj, &foo_lcd_group);
        if (err)
                return -ENODEV;
#endif

	dev_info(dev, "backlight '%s'\n", client->name);

	return 0;
}

static const struct i2c_device_id foo_lcd_ids[] = {
	{ "foo_lcd", 0 },
	{ /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, foo_lcd_ids);

static int foo_lcd_detect(struct i2c_client *new_client,
			struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = new_client->adapter;
	int ctrl;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	ctrl = i2c_smbus_read_byte_data(new_client, 0x0);
	if (ctrl < 0)
		return -ENODEV;

	strlcpy(info->type, "foo_lcd", I2C_NAME_SIZE);

	return 0;
}

static struct i2c_driver foo_lcd_driver = {
	.class		= I2C_CLASS_DEPRECATED,
	.driver = {
		.name	= "foo_lcd",
	},
	.probe		= foo_lcd_probe,
	.id_table	= foo_lcd_ids,
	.detect		= foo_lcd_detect,
	.address_list	= normal_i2c,
};

module_i2c_driver(foo_lcd_driver);

MODULE_AUTHOR("Jake <jake9999@dreamwiz.com>");
MODULE_DESCRIPTION("foo_lcd driver");
MODULE_LICENSE("GPL");
