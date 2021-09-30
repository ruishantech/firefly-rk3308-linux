/*
 * Copyright (C) 2017 Sean Young <sean@mess.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pwm.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <media/rc-core.h>
#include "rc-core-priv.h"

#define DRIVER_NAME	"pwm-ir-tx"
#define DEVICE_NAME	"PWM IR Transmitter"

#define to_rc_device(obj) container_of(obj, struct rc_dev, dev)

int ir_tx_debug;
module_param_named(debug, ir_tx_debug, int, 0644);
MODULE_PARM_DESC(debug, "debug level (0-2)");

#define dprintk(lvl, fmt, arg...)                                  \
	do {                                                       \
		if ((lvl) <= ir_tx_debug)                          \
			pr_info("%s: " fmt, __func__, ## arg);     \
	} while (0)

struct pwm_ir {
	struct pwm_device *pwm;
	unsigned int carrier;
	unsigned int duty_cycle;
};

static const struct of_device_id pwm_ir_of_match[] = {
	{ .compatible = "pwm-ir-tx", },
	{ },
};
MODULE_DEVICE_TABLE(of, pwm_ir_of_match);

static int pwm_ir_set_duty_cycle(struct rc_dev *dev, u32 duty_cycle)
{
	struct pwm_ir *pwm_ir = dev->priv;

	pwm_ir->duty_cycle = duty_cycle;

	return 0;
}

static int pwm_ir_set_carrier(struct rc_dev *dev, u32 carrier)
{
	struct pwm_ir *pwm_ir = dev->priv;

	if (!carrier)
		return -EINVAL;

	pwm_ir->carrier = carrier;

	return 0;
}

static void ir_delay(long time)
{
	while (time > 0) {
		if (time > 2000) {
			time = time - 2000;
			mdelay(2);
		} else {
			udelay(time);
			break;
		}
	}
}

static int pwm_ir_tx(struct rc_dev *dev,
		     unsigned int *txbuf,
		     unsigned int count)
{
	struct pwm_ir *pwm_ir = dev->priv;
	struct pwm_device *pwm = pwm_ir->pwm;
	unsigned int i;
	int duty, period;
	ktime_t edge, tmp;
	long delta;
	unsigned long flags;

	period = DIV_ROUND_CLOSEST(NSEC_PER_SEC, pwm_ir->carrier);
	duty = DIV_ROUND_CLOSEST(pwm_ir->duty_cycle * period, 100);

	pwm_config(pwm, duty, period);

	tmp = ktime_get();
	local_irq_save(flags);
	edge = ktime_get();
	for (i = 0; i < count; i++) {
		if (i % 2) // space
			pwm_disable(pwm);
		else
			pwm_enable(pwm);

		edge = ktime_add_us(edge, txbuf[i]);
		delta = ktime_us_delta(edge, ktime_get());
		dprintk(2, "delta %ld|%d\n", delta, txbuf[i]);

		if (delta > 0)
			ir_delay(delta);
			/* usleep_range(delta, delta + 10); */
	}
	pwm_disable(pwm);
	local_irq_restore(flags);
	pr_info("%s: spend time =  %llu\n",
		__func__,
		ktime_to_ms(ktime_sub(ktime_get(), tmp)));
	return count;
}

static ssize_t duty_ratio_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int ret = 0;
	unsigned long duty_ratio = 0;
	struct rc_dev *rcd = to_rc_device(dev);

	ret = kstrtoul(buf, 0, &duty_ratio);
	if (ret) {
		dev_err(dev, "duty_ratio param error!\n");
		return ret;
	}

	if (duty_ratio > 0)
		pwm_ir_set_duty_cycle(rcd, duty_ratio);

	return count;
}

static ssize_t duty_ratio_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct rc_dev *rcd = to_rc_device(dev);
	struct pwm_ir *pwm_ir = rcd->priv;

	return sprintf(buf, "duty_ratio: %u\n", pwm_ir->duty_cycle);
}

static ssize_t frequency_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t count)
{
	int ret = 0;
	unsigned long frequency = 0;
	struct rc_dev *rcd = to_rc_device(dev);

	ret = kstrtoul(buf, 0, &frequency);
	if (ret) {
		dev_err(dev, "frequency param error!\n");
		return ret;
	}

	if (frequency > 0)
		pwm_ir_set_carrier(rcd, frequency);

	return count;
}

static ssize_t frequency_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct rc_dev *rcd = to_rc_device(dev);
	struct pwm_ir *pwm_ir = rcd->priv;

	return sprintf(buf, "frequency: %u\n", pwm_ir->carrier);
}

static ssize_t transmit_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf,
			      size_t count)
{
	int ret = 0, index = 0;
	char delim[] = ",";
	char *tmp, *copy_buf = NULL, *token = NULL;

	unsigned int *patterns = NULL, times = 0;
	struct rc_dev *rcd = to_rc_device(dev);

	copy_buf = kstrdup(buf, GFP_KERNEL);
	if (!copy_buf)
		return -1;
	dprintk(1, "%s\n", copy_buf);

	patterns = kzalloc(sizeof(unsigned int) * MAX_IR_EVENT_SIZE,
			   GFP_KERNEL);
	if (!patterns)
		goto exit;

	tmp = copy_buf;
	while ((token = strsep(&tmp, delim)) != NULL &&
	       index < MAX_IR_EVENT_SIZE) {
		ret = kstrtouint(token, 0, patterns + (index++));
		if (ret) {
			dev_err(dev,
				"%s: kstrtouint error\n",
				__func__);
			goto exit1;
		}

		times += *(patterns + index - 1);
		if (times >= 1000 * 1000)
			break;
	}
	pr_info("\n%s: total times = %u cnt = %d\n",
		__func__,
		times / 1000,
		index);

	pwm_ir_tx(rcd, patterns, index);

	if (!ret)
		ret = count;

exit1:
	kzfree(patterns);
exit:
	kfree(copy_buf);

	return ret;
}

static ssize_t transmit_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	return 0;
}

static const struct device_attribute ir_attrs[] = {
	__ATTR_RW(duty_ratio),
	__ATTR_RW(frequency),
	__ATTR_RW(transmit),
};

static int pwm_ir_probe(struct platform_device *pdev)
{
	struct pwm_ir *pwm_ir;
	struct rc_dev *rcdev;
	unsigned int i;
	int rc;

	pwm_ir = devm_kmalloc(&pdev->dev, sizeof(*pwm_ir), GFP_KERNEL);
	if (!pwm_ir)
		return -ENOMEM;

	pwm_ir->pwm = devm_pwm_get(&pdev->dev, NULL);
	if (IS_ERR(pwm_ir->pwm))
		return PTR_ERR(pwm_ir->pwm);

	pwm_ir->carrier = 38000;
	pwm_ir->duty_cycle = 50;

	rcdev = devm_rc_allocate_device(&pdev->dev, RC_DRIVER_IR_RAW_TX);
	if (!rcdev)
		return -ENOMEM;

	rcdev->priv = pwm_ir;
	rcdev->driver_name = DRIVER_NAME;
	rcdev->device_name = DEVICE_NAME;
	rcdev->tx_ir = pwm_ir_tx;
	rcdev->s_tx_duty_cycle = pwm_ir_set_duty_cycle;
	rcdev->s_tx_carrier = pwm_ir_set_carrier;

	rc = devm_rc_register_device(&pdev->dev, rcdev);
	if (rc < 0)
		dev_err(&pdev->dev, "failed to register rc device\n");

	rc = pwm_adjust_config(pwm_ir->pwm);
	if (rc) {
		dev_err(&pdev->dev,
			"pwm adjust config failed! err = %d\n",
			rc);
		return rc;
	}

	for (i = 0; i < ARRAY_SIZE(ir_attrs); i++) {
		if (device_create_file(&rcdev->dev, &ir_attrs[i])) {
			dev_err(&rcdev->dev,
				"Create %s attr failed\n",
				rcdev->driver_name);
			return -ENOMEM;
		}
	}

	return rc;
}

static struct platform_driver pwm_ir_driver = {
	.probe = pwm_ir_probe,
	.driver = {
		.name	= DRIVER_NAME,
		.of_match_table = of_match_ptr(pwm_ir_of_match),
	},
};
module_platform_driver(pwm_ir_driver);

MODULE_DESCRIPTION("PWM IR Transmitter");
MODULE_AUTHOR("Sean Young <sean@mess.org>");
MODULE_LICENSE("GPL");
