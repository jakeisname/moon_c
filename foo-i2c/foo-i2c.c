#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define FOO_TX_RX_FIFO_SIZE 256

struct foo_i2c_dev {
	struct device *device;
	struct i2c_adapter adapter;
	struct i2c_msg *msg;
};

static int foo_i2c_xfer_single_msg(struct foo_i2c_dev *foo_i2c,
					 struct i2c_msg *msg)
{
	int i;
	u8 addr;
	u32 val = 0;
	unsigned int tx_bytes;

	foo_i2c->msg = msg;
	addr = i2c_8bit_addr_from_msg(msg);
	dev_info(foo_i2c->device, "addr=0x%02x\n", addr >> 1);
	tx_bytes = msg->len;
	if (!(msg->flags & I2C_M_RD)) {
		for (i = 0; i < tx_bytes; i++) {
			val = msg->buf[i];

			/* send to fifo */
			dev_info(foo_i2c->device, "write=0x%02x\n", val);
		}
	}

	if (msg->flags & I2C_M_RD) {
		for (i = 0; i < msg->len; i++) {
			msg->buf[i] = (addr & 0xff) >> 1;

			/* receive from fifo */
			dev_info(foo_i2c->device, "read=0x%02x\n", msg->buf[i]);
		}
	}

	return 0;
}

static int foo_i2c_xfer(struct i2c_adapter *adapter,
			      struct i2c_msg msgs[], int num)
{
	struct foo_i2c_dev *foo_i2c = i2c_get_adapdata(adapter);
	int ret, i;

	for (i = 0; i < num; i++) {
		ret = foo_i2c_xfer_single_msg(foo_i2c, &msgs[i]);
		if (ret) {
			dev_info(foo_i2c->device, "xfer failed\n");
			return ret;
		}
	}

	return num;
}

static uint32_t foo_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm foo_algo = {
	.master_xfer = foo_i2c_xfer,
	.functionality = foo_i2c_functionality,
};

static const struct i2c_adapter_quirks foo_i2c_quirks = {
	.max_read_len = FOO_TX_RX_FIFO_SIZE - 1,
};

static int foo_i2c_probe(struct platform_device *pdev)
{
	struct foo_i2c_dev *foo_i2c;
	struct i2c_adapter *adap;
	int ret;

	foo_i2c = devm_kzalloc(&pdev->dev, sizeof(*foo_i2c),
				 GFP_KERNEL);
	if (!foo_i2c)
		return -ENOMEM;

	platform_set_drvdata(pdev, foo_i2c);
	foo_i2c->device = &pdev->dev;

	adap = &foo_i2c->adapter;
	i2c_set_adapdata(adap, foo_i2c);
	strlcpy(adap->name, "foo I2C adapter", sizeof(adap->name));
	adap->algo = &foo_algo;
	adap->quirks = &foo_i2c_quirks;
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;

	ret = i2c_add_adapter(adap);

	if (!ret)
		dev_info(foo_i2c->device, "foo i2c driver probed success.\n");

	return ret;
}

static int foo_i2c_remove(struct platform_device *pdev)
{
	struct foo_i2c_dev *foo_i2c = platform_get_drvdata(pdev);

	i2c_del_adapter(&foo_i2c->adapter);

	dev_info(foo_i2c->device, "foo i2c driver unloaded.\n");

	return 0;
}

static const struct of_device_id foo_i2c_of_match[] = {
	{ .compatible = "foo,foo-i2c" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, foo_i2c_of_match);

static struct platform_driver foo_i2c_driver = {
	.driver = {
		.name = "foo-i2c",
		.of_match_table = foo_i2c_of_match,
	},
	.probe = foo_i2c_probe,
	.remove = foo_i2c_remove,
};
module_platform_driver(foo_i2c_driver);

MODULE_AUTHOR("Jake <jake@dreamwiz.com>");
MODULE_DESCRIPTION("foo I2C Driver");
MODULE_LICENSE("GPL v2");
