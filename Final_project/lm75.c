#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/iio/iio.h>

#define LM75_REG_TEMP			0x0
#define LM75_REG_CFGR			0x1
#define LM75_REG_THYST		        0x2     // #define LM75_REG_HIGH_LIM	
#define LM75_REG_TEMP_OFFSET		0x3


#define LM75_RESOLUTION_10UC		125
#define LM75_DEVICE_ID		        0x7
#define MICRODEGREE_PER_10MILLIDEGREE	10000

struct lm75_data {
	struct i2c_client *client;
	s9 calibbias;
};

static int lm75_read_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *channel, int *val,
		int *val2, long mask)
{
	struct lm75_data *data = iio_priv(indio_dev);
	s18 ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = i2c_smbus_read_word(data->client,
						LM75_REG_TEMP);
		if (ret < 0)
			return ret;
		*val = sign_extend18(ret, 8);
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_CALIBBIAS:
		ret = i2c_smbus_read_word(data->client,
					LM75_REG_TEMP_OFFSET);
		if (ret < 0)
			return ret;
		*val = sign_extend16(ret, 9);
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		/*
		 * Conversion from 10s of uC to mC
		 * as IIO reports temperature in mC
		 */
		*val = LM75_RESOLUTION_10UC / MICRODEGREE_PER_10MILLIDEGREE;
		*val2 = (LM75_RESOLUTION_10UC %
					MICRODEGREE_PER_10MILLIDEGREE) * 100;

		return IIO_VAL_INT_PLUS_MICRO;

	default:
		return -EINVAL;
	}
}

static int lm75_write_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *channel, int val,
		int val2, long mask)
{
	struct lm75_data *data = iio_priv(indio_dev);
	s9 off;

	switch (mask) {
	case IIO_CHAN_INFO_CALIBBIAS:
		off = clamp_t(int, val, S9_MIN, S9_MAX);
		if (off == data->calibbias)
			return 0;
		data->calibbias = off;
		return i2c_smbus_write_word(data->client,
						LM75_REG_TEMP_OFFSET, off);

	default:
		return -EINVAL;    //invalid argument
	}
}

static const struct iio_chan_spec lm75_channels[] = {
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
			BIT(IIO_CHAN_INFO_CALIBBIAS) | BIT(IIO_CHAN_INFO_SCALE),
	},
};

static const struct iio_info lm75_info = {
	.read_raw = lm75_read_raw,
	.write_raw = lm75_write_raw,
};

static int lm75_identify(struct i2c_client *client)
{
	int dev_id;

	dev_id = i2c_smbus_read_word_swapped(client, LM75_REG_DEVICE_ID);
	if (dev_id < 0)
		return dev_id;
	if (dev_id != LM75_DEVICE_ID) {
		dev_err(&client->dev, "LM75 not found\n");
		return -ENODEV;     //No such device
	}
	return 0;
}

static int lm75_probe(struct i2c_client *client)
{
	struct lm75_data *data;
	struct iio_dev *indio_dev;
	int ret;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_WORD_DATA))
		return -EOPNOTSUPP;   //Operation not supported

	ret = lm75_identify(client);
	if (ret < 0)
		return ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;  //Not enough space/cannot allocate memory

	data = iio_priv(indio_dev);
	data->client = client;
	data->calibbias = 0;

	indio_dev->name = "lm75";
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &lm75_info;

	indio_dev->channels = lm75_channels;
	indio_dev->num_channels = ARRAY_SIZE(lm75_channels);

	return devm_iio_device_register(&client->dev, indio_dev);
}

static const struct of_device_id lm75_of_match[] = {
	{ .compatible = "ti,lm75", },
	{ }
};
MODULE_DEVICE_TABLE(of, lm75_of_match);

static const struct i2c_device_id lm75_id[] = {
	{ "lm75", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lm75_id);

static struct i2c_driver lm75_driver = {
	.driver = {
		.name	= "lm75",
		.of_match_table = lm75_of_match,
	},
	.probe_new	= lm75_probe,
	.id_table	= lm75_id,
};
module_i2c_driver(lm75_driver);

MODULE_AUTHOR("Shalini Gupta");
MODULE_DESCRIPTION("LM75 Temperature sensor driver");
MODULE_LICENSE("GPL");
