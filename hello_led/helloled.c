/* hello.c */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#define PIO_LED	40
#define PIO_IRQ	81

#define sysfs_dir "helloled"

static void blink_led(struct work_struct *w);
DECLARE_DELAYED_WORK(helloled_task, blink_led);

static ssize_t set_value(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count) {

	unsigned int value = 0;
	int ret = 0;

	ret = kstrtouint(buf, 10, &value);

	switch (value) {
		case 0:
		case 1:
			gpio_set_value(PIO_LED, value);
			break;

		default:
			return -EINVAL;
	}

	return count;
}

static ssize_t show_value(struct device *dev,
				struct device_attribute *attr,
				char *buf) {
	unsigned int value = 0;

	value = gpio_get_value(PIO_LED);

	return sprintf(buf, "%d\n", value);
}

static DEVICE_ATTR(ledcontrol, S_IWUSR | S_IRUGO,
			show_value, set_value);

static struct attribute *hello_attributes[] = {
	&dev_attr_ledcontrol.attr,
	NULL
};

static const struct attribute_group hello_attr_group = {
	.attrs = hello_attributes,
};

static void blink_led(struct work_struct *w) {
	unsigned int value = 0;

	value = gpio_get_value(PIO_LED);

	gpio_set_value(PIO_LED, value == 0 ? 1 : 0);

	if (run) schedule_delayed_work(&helloled_task, HZ);
}

static struct kobject *hello_obj = NULL;

static int __init hello_init(void) {
	int ret = 0;
	int irq_num = 0;

	ret = gpio_request(PIO_LED,"LED");
	if (ret != 0) {
		pr_alert ("helloled: request gpio failed\n");
		return ret;
	}

	ret = gpio_direction_output(PIO_LED, 1);
	if (ret != 0) {
		pr_alert("helloled: setting direction failed\n");
		return ret;
	}

	hello_obj = kobject_create_and_add(sysfs_dir, kernel_kobj);
	if (hello_obj == NULL)
	{
		pr_alert("helloled: kobject_create_and_add failed\n");
		return -ENOMEM;
	}

	ret = sysfs_create_group(hello_obj, &hello_attr_group);
	if (ret != 0)
	{
		/* creating files failed, thus we must remove the created directory! */
		pr_alert ("helloled: sysfs_create_group failed with result %d\n", ret);
		kobject_put(hello_obj);
		return -ENOMEM;
	}

	schedule_delayed_work(&helloled_task, HZ);

	pr_alert("helloled: Very useful LED driver up and running\n");

	return 0;
}

static void __exit hello_exit(void)
{
	flush_delayed_work(&helloled_task);
	cancel_delayed_work_sync(&helloled_task);
	gpio_free(PIO_LED);
	kobject_put(hello_obj);
	pr_alert("helloled: Very useful LED driver down\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hello led");
MODULE_AUTHOR("Angelo Compagnucci <a.compagnucci@idea-on-line.it>");
