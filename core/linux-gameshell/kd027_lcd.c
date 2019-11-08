/*
 * kd027_lcd.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 *
 */

#include <linux/gpio.h> /* Only for legacy support */
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_data/gpio_backlight.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/clk.h>

struct kd027_lcd {
	struct gpio_desc* rst_pin;
	struct gpio_desc* cs_pin;
	struct gpio_desc* sck_pin;
	struct gpio_desc* sda_pin;
	unsigned char init_seq[100];
	unsigned char suspend_seq[20];
	unsigned char resume_seq[20];
	int init_seq_len;
	int suspend_seq_len;
	int resume_seq_len;
};

struct kd027_lcd * lcd_data;

static void kd027_write_lcd(unsigned char data)
{
	unsigned char i;

	for(i = 0; i < 8; i++) {
		if (data & 0x80)
			gpiod_set_value(lcd_data->sda_pin, 1);
		else
			gpiod_set_value(lcd_data->sda_pin, 0);
		gpiod_set_value(lcd_data->sck_pin, 0);
		gpiod_set_value(lcd_data->sck_pin, 1);
		data <<= 1;
	}
}

static void kd027_write_cmd_data(unsigned char c, unsigned char d)
{
	gpiod_set_value(lcd_data->cs_pin, 0);
	kd027_write_lcd(c);
	kd027_write_lcd(d);
	gpiod_set_value(lcd_data->cs_pin, 1);
}

static void kd027_init(void)
{
	int i;
	for(i = 0; i < lcd_data->init_seq_len/2; i++) {
		kd027_write_cmd_data(lcd_data->init_seq[i * 2], lcd_data->init_seq[i * 2 + 1]);
	}
}

#ifdef CONFIG_PROC_FS
static char global_buffer[64];

static int kd027_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "kd027\n");
	return 0;
}

static int kd027_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, kd027_proc_show, NULL);
}

static ssize_t kd027_proc_read(struct file * file, char __user * buf, size_t size, loff_t * loff)
{
	return 0;
}

static ssize_t kd027_proc_write(struct file * file, const char __user * buf, size_t size, loff_t * loff)
{
	int cmd, data;
	char* tmp;

	if(copy_from_user(global_buffer, buf, size))
		return -EFAULT;

	global_buffer[size] = 0;
	cmd = simple_strtol(global_buffer, 0, 16);
	tmp = strchr(global_buffer, ' ');
	if(tmp) {
		data = simple_strtol(tmp+1, 0, 16);
		kd027_write_cmd_data(cmd, data);
	}

	return size;
}

static const struct file_operations kd027_proc_fops = {
	.open		= kd027_proc_open,
	.read		= kd027_proc_read,
	.write		= kd027_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init kd027_proc_init(void)
{
	struct proc_dir_entry *r;

	r = proc_create("driver/lcd", S_IRWXUGO, NULL, &kd027_proc_fops);
	if (!r)
		return -ENOMEM;
	return 0;
}
#else
static inline int kd027_proc_init(void) { return 0; }
#endif /* CONFIG_PROC_FS */

static int kd027_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;

	if ( !np) {
		dev_err(&pdev->dev,
			"failed to find platform data or device tree node.\n");
		return -ENODEV;
	}

	lcd_data = devm_kzalloc(&pdev->dev, sizeof(*lcd_data), GFP_KERNEL);
	if (lcd_data == NULL)
		return -ENOMEM;

	lcd_data->init_seq_len =  of_property_read_variable_u8_array(np, "init-seq", lcd_data->init_seq, 1, 100);
	lcd_data->suspend_seq_len =  of_property_read_variable_u8_array(np, "suspend-seq", lcd_data->suspend_seq, 1, 20);
	lcd_data->resume_seq_len =  of_property_read_variable_u8_array(np, "resume-seq", lcd_data->resume_seq, 1, 20);

	lcd_data->rst_pin = devm_gpiod_get(dev, "lcd-rst", GPIOD_OUT_HIGH);
	if (IS_ERR(lcd_data->rst_pin)) {
		ret = PTR_ERR(lcd_data->rst_pin);

		if (ret != -EPROBE_DEFER) {
			dev_err(dev,
				"Error: The gpios parameter is missing or invalid.\n");
		}
		return ret;
	}

	lcd_data->cs_pin = devm_gpiod_get(dev, "lcd-cs", GPIOD_OUT_HIGH);
	if (IS_ERR(lcd_data->cs_pin)) {
		ret = PTR_ERR(lcd_data->cs_pin);

		if (ret != -EPROBE_DEFER) {
			dev_err(dev,
				"Error: The gpios parameter is missing or invalid.\n");
		}
		return ret;
	}

	lcd_data->sck_pin = devm_gpiod_get(dev, "lcd-sck", GPIOD_OUT_HIGH);
	if (IS_ERR(lcd_data->sck_pin)) {
		ret = PTR_ERR(lcd_data->sck_pin);

		if (ret != -EPROBE_DEFER) {
			dev_err(dev,
				"Error: The gpios parameter is missing or invalid.\n");
		}
		return ret;
	}

	lcd_data->sda_pin = devm_gpiod_get(dev, "lcd-sda", GPIOD_OUT_HIGH);
	if (IS_ERR(lcd_data->sda_pin)) {
		ret = PTR_ERR(lcd_data->sda_pin);

		if (ret != -EPROBE_DEFER) {
			dev_err(dev,
				"Error: The gpios parameter is missing or invalid.\n");
		}
		return ret;
	}

	kd027_init();
	kd027_proc_init();

	return 0;
}

static int kd027_suspend(struct platform_device * pdev, pm_message_t state)
{
	int i;
	for(i = 0; i < lcd_data->suspend_seq_len/2; i++) {
		kd027_write_cmd_data(lcd_data->suspend_seq[i * 2], lcd_data->suspend_seq[i * 2 + 1]);
	}
	return 0;
}

static int kd027_resume(struct platform_device * pdev)
{
	int i;
	for(i = 0; i < lcd_data->resume_seq_len/2; i++) {
		kd027_write_cmd_data(lcd_data->resume_seq[i * 2], lcd_data->resume_seq[i * 2 + 1]);
	}
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id kd027_of_match[] = {
	{ .compatible = "kd027-lcd" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, kd027_of_match);
#endif

static struct platform_driver kd027_device_driver = {
	.probe		= kd027_probe,
	.suspend 		= kd027_suspend,
	.resume 		= kd027_resume,
	.driver		= {
		.name		= "kd027-lcd",
		.of_match_table = of_match_ptr(kd027_of_match),
	},
};

module_platform_driver(kd027_device_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KD027 Driver");

