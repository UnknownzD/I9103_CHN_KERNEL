/*
 * arch/arm/mach-tegra/panel-n1.c
 *
 * Copyright (c) 2010, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/resource.h>
#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>
#include <linux/earlysuspend.h>
#include <mach/nvhost.h>
#include <mach/nvmap.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/dc.h>
#include <mach/fb.h>

#include "devices.h"
#include "gpio-names.h"
#include "board.h"

#define n1_lvds_reset TEGRA_GPIO_PC1

#define LCD_ONOFF_TEST 1 //LCD on/off test
#define LCD_TYPE 1

#define n1_ld9040 0
#define USE_CHECK_HW_REVERSION 0 


// LCD_ONOFF_TEST , LCD_TYPE
extern struct class *sec_class;
struct device *tune_lcd_dev;

#if LCD_ONOFF_TEST
static int lcdonoff_test=0;
#define TEGRA_GPIO_PR3		139
#define GPIO_LCD_LDO_LED_EN TEGRA_GPIO_PR3
#endif

static struct spi_device *n1_disp1_spi;
static struct regulator *reg_lcd_1v8, *reg_lcd_3v0;
static struct early_suspend n1_panel_early_suspend;

static int n1_spi_write(u8 addr, u8 data)
{
	struct spi_message m;
	struct spi_transfer xfer;

	u8 w[2];
	int ret;

	spi_message_init(&m);

	memset(&xfer, 0, sizeof(xfer));

	w[0] = addr;
	w[1] = data;
	xfer.tx_buf = &w;

	xfer.bits_per_word = 9;
	xfer.len = 2;
	spi_message_add_tail(&xfer, &m);

	ret = spi_sync(n1_disp1_spi, &m);

	if (ret < 0)
		dev_warn(&n1_disp1_spi->dev, "failed to write to LCD reg (%d)\n", addr);

	return ret;
}

#if n1_ld9040
int n1_panel_enable(void)
{
	printk(KERN_INFO "%s: start\n", __func__);

	regulator_enable(reg_lcd_1v8);
	mdelay(1);
	printk(KERN_INFO "%s: regulator_enable(reg_lcd_1v8);\n", __func__);

	regulator_enable(reg_lcd_3v0);
	mdelay(1);
	printk(KERN_INFO "%s: regulator_enable(reg_lcd_3v0);\n", __func__);

	/* take panel out of reset */
	gpio_set_value(n1_lvds_reset, 1);

	mdelay(50);
	printk(KERN_INFO "%s: gpio_set_value(n1_lvds_reset, 1);\n", __func__);

	//SEQ_USER_SETTING
	n1_spi_write( 0, 0xF0 );
	n1_spi_write( 1, 0x5A );
	n1_spi_write( 1, 0x5A );

	//SEQ_DISPCTL
	n1_spi_write( 0, 0xF2 );
	n1_spi_write( 1, 0x02 );
	n1_spi_write( 1, 0x06 );
	n1_spi_write( 1, 0x0A );
	n1_spi_write( 1, 0x10 );
	n1_spi_write( 1, 0x10 );
	
	//SEQ_GTCON
	n1_spi_write( 0, 0xF7 );
	n1_spi_write( 1, 0x09 );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x00 );
	
	//SEQ_PANEL_CONDITION
	n1_spi_write( 0, 0xF8 );
	n1_spi_write( 1, 0x05 );
	n1_spi_write( 1, 0x5E );
	n1_spi_write( 1, 0x96 );
	n1_spi_write( 1, 0x6B );
	n1_spi_write( 1, 0x7D );
	n1_spi_write( 1, 0x0D );
	n1_spi_write( 1, 0x3F );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x32 );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x07 );
	n1_spi_write( 1, 0x07 );
	n1_spi_write( 1, 0x20 );
	n1_spi_write( 1, 0x20 );
	n1_spi_write( 1, 0x20 );	
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x00 );	
	//SEQ_SLPOUT
	n1_spi_write( 0, 0x11 );
	n1_spi_write( 1, 0x09 );
	
    mdelay(120);//Wait 120ms


	//SEQ_ELVSS_ON_SM2
	n1_spi_write( 0, 0xB1 );
	n1_spi_write( 1, 0x0F );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x16 );
	n1_spi_write( 0, 0xB2 );
	n1_spi_write( 1, 0x15 );
	n1_spi_write( 1, 0x15 );
	n1_spi_write( 1, 0x15 );
	
	//SEQ_PWR_CTRL_SM2
	n1_spi_write( 0, 0xF4 );
	n1_spi_write( 1, 0x0A );
	n1_spi_write( 1, 0x87 );
	n1_spi_write( 1, 0x25 );
	n1_spi_write( 1, 0x6A );
	n1_spi_write( 1, 0x44 );
	n1_spi_write( 1, 0x02 );
	
	//SEQ_GAMMA_SET1_SM2
	n1_spi_write( 0, 0xF9 );
	n1_spi_write( 1, 0x2E );
	n1_spi_write( 1, 0xB1 );
	n1_spi_write( 1, 0xB3 );
	n1_spi_write( 1, 0xAD );
	n1_spi_write( 1, 0xBF );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0x8E );
	n1_spi_write( 1, 0x36 );
	n1_spi_write( 1, 0xA3 );
	n1_spi_write( 1, 0xA9 );
	n1_spi_write( 1, 0xA6 );
	n1_spi_write( 1, 0xBB );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0xA3 );
	n1_spi_write( 1, 0x2E );
	n1_spi_write( 1, 0xAC );
	n1_spi_write( 1, 0xAD );
	n1_spi_write( 1, 0xA8 );
	n1_spi_write( 1, 0xBC );
	n1_spi_write( 1, 0x00 );
	n1_spi_write( 1, 0xB4 );
	
	//SEQ_GAMMA_CTRL
	n1_spi_write( 0, 0xFB );
	n1_spi_write( 1, 0x02 );
	n1_spi_write( 1, 0x5A );

	//SEQ_DISPON
	n1_spi_write( 0, 0x29 );

	return 0;
}
#else
int n1_panel_enable(void)
{
	regulator_enable(reg_lcd_1v8);
	regulator_enable(reg_lcd_3v0);
	mdelay(10);
	/* take panel out of reset */
	gpio_set_value(n1_lvds_reset, 1);
	//gpio_set_value(GPIO_LCD_LDO_LED_EN, 1);

#ifdef CONFIG_MACH_N1_CHN
	msleep(10);//10ms
	//Pushing DC data out 10 msec after from LCD reset.
	tegra_fb_dc_data_out(registered_fb[0]);
	msleep(40);//40ms
#else	
	msleep(50);//50ms
#endif 
	msleep(50);//50ms
	msleep(10);//50ms

	//Set address mode
	{
		n1_spi_write( 0, 0x36 );
#if USE_CHECK_HW_REVERSION		
		if(system_rev==0 || system_rev==-1)
		{
			n1_spi_write( 1, 0x44 ); 
		}
		else
		{
			n1_spi_write( 1, 0xD4 ); 
		}
#else
	n1_spi_write( 1, 0xD4 );
#endif		
	}

	msleep(25);//25ms

	//Sleep Out Command
	{
		n1_spi_write( 0, 0x11 );
	}

	msleep(150);//150ms

	//Display On Command
	{
		n1_spi_write( 0, 0x29 );
	}

	gpio_set_value(GPIO_LCD_LDO_LED_EN, 1);
	
	return 0;
}
#endif
EXPORT_SYMBOL(n1_panel_enable);

int n1_panel_disable(void)
{
	//Display off
	{
		n1_spi_write( 0, 0x28 );
	}

	msleep(25);//25ms

	//Sleep In
	{
		n1_spi_write( 0, 0x10 );
	}

	msleep(150);//150ms

	gpio_set_value(GPIO_LCD_LDO_LED_EN, 0);

	gpio_set_value(n1_lvds_reset, 0);

	usleep_range(5000, 6000);//5ms
	regulator_disable(reg_lcd_3v0);
	regulator_disable(reg_lcd_1v8);
	return 0;
}
EXPORT_SYMBOL(n1_panel_disable);

void n1_panel_config_pins(void)
{
    tegra_gpio_enable(TEGRA_GPIO_PN4);
    gpio_request(TEGRA_GPIO_PN4, "SFIO_LCD_NCS");
    gpio_direction_output(TEGRA_GPIO_PN4, 0); 
    tegra_gpio_enable(TEGRA_GPIO_PZ4);
    gpio_request(TEGRA_GPIO_PZ4, "SFIO_LCD_SCLK");
    gpio_direction_output(TEGRA_GPIO_PZ4, 0);
}
EXPORT_SYMBOL(n1_panel_config_pins);

void n1_panel_reconfig_pins(void)
{
    /* LCD_nCS */
    tegra_gpio_disable(TEGRA_GPIO_PN4);
    /* LCD_SCLK */
    tegra_gpio_disable(TEGRA_GPIO_PZ4);
}
EXPORT_SYMBOL(n1_panel_reconfig_pins);

static int panel_n1_spi_suspend(struct spi_device *spi, pm_message_t message)
{
	printk(KERN_INFO "\n ************ %s : %d \n", __func__, __LINE__);
	n1_panel_disable();
    n1_panel_config_pins();
    return 0;
}

static int panel_n1_spi_shutdown(struct spi_device *spi, pm_message_t message)
{
	printk(KERN_INFO "\n ************ %s : %d \n", __func__, __LINE__);
	n1_panel_disable();
	return 0;
}

static int panel_n1_spi_resume(struct spi_device *spi)
{
	printk(KERN_INFO "\n ************ %s : %d \n", __func__, __LINE__);
    n1_panel_reconfig_pins();
    n1_panel_enable();
	return 0;
}

#if LCD_ONOFF_TEST
void lcd_power_on(void)
{
	n1_panel_enable();

	//gpio_set_value(GPIO_LCD_LDO_LED_EN, 1);

	lcdonoff_test = 1;
}


void lcd_power_off(void)
{
	printk(KERN_INFO "-0- %s called -0-\n", __func__);

	lcdonoff_test = 0;

	n1_panel_disable();

	//gpio_set_value(GPIO_LCD_LDO_LED_EN, 0);

	msleep(100);
}


static ssize_t lcd_power_file_cmd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	printk("called %s \n", __func__);

	return sprintf(buf, "%u\n", lcdonoff_test);
}

static ssize_t lcd_power_file_cmd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	printk(KERN_INFO "[lcd_power] in lcd_power_file_cmd_store, input value = %d \n", value);

	if (value == 1) {
		lcd_power_on();
		printk(KERN_INFO "[lcd_power on] <= value : %d \n", value);
	} else if (value == 0) {
		lcd_power_off();
		printk(KERN_INFO "[lcd_power off] <= value : %d \n", value);
	} else
		printk(KERN_INFO "[lcd_power] lcd is already = %d \n", lcdonoff_test);

	return size;
}

static DEVICE_ATTR(lcd_onoff, 0666, lcd_power_file_cmd_show, lcd_power_file_cmd_store);

#endif //LCD_ONOFF_TEST

#if LCD_TYPE
static ssize_t lcdtype_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	char temp[25];
	sprintf(temp, "SONY_L4F00430T01\n");
	strcat(buf, temp);
	return strlen(buf);
}
static DEVICE_ATTR(lcdtype, 0666,lcdtype_show, NULL);
#endif // LCD_TYPE


static int panel_n1_spi_probe(struct spi_device *spi)
{
	int ret;

	spi->bits_per_word = 9;
	spi->mode = SPI_MODE_3;
	spi->max_speed_hz = 1000000;

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "spi_setup failed: %d\n", ret);
		return ret;
	}

	printk(KERN_INFO "%s: probe\n", __func__);

	ret = gpio_request(n1_lvds_reset, "lvds_reset");
	if (ret < 0)
		printk(KERN_INFO "%s: gpio_request failed with %d \n", __func__, ret);
	tegra_gpio_enable(n1_lvds_reset);

//LCD_ONOFF_TEST, LCD_TYPE
	tune_lcd_dev = device_create(sec_class, NULL, 0, NULL, "sec_tune_lcd");

	if (IS_ERR(tune_lcd_dev)) 
    	{
		printk("Failed to create device!");
		ret = -1;
	}
//LCD_ONOFF_TEST, LCD_TYPE

#if LCD_ONOFF_TEST
	if (device_create_file(tune_lcd_dev, &dev_attr_lcd_onoff) < 0) {
		printk("Failed to create device file!(%s)!\n", dev_attr_lcd_onoff.attr.name);
		ret = -1;
	}
#endif

#if LCD_TYPE
	if (device_create_file(tune_lcd_dev, &dev_attr_lcdtype) < 0) {
		printk("Failed to create device file!(%s)!\n", dev_attr_lcdtype.attr.name);
		ret = -1;
	}
#endif //LCD_TYPE
	printk(KERN_INFO "%s: device_create_file(tune_lcd_dev, &dev_attr_lcdtype) \n", __func__);

	reg_lcd_1v8 = regulator_get(NULL, "VLCD_1V8");
	if (IS_ERR(reg_lcd_1v8)) {
		printk(KERN_INFO "%s: VLCD_1V8 regulator not found\n", __func__);
		reg_lcd_1v8 = NULL;
	} else {
		regulator_set_voltage(reg_lcd_1v8, 1800*1000,1800*1000);
//		regulator_enable(reg_lcd_1v8);
	}

	reg_lcd_3v0 = regulator_get(NULL, "VLCD_3V0");
	if (IS_ERR(reg_lcd_3v0)) {
		printk(KERN_INFO "%s: VLCD_3V0 regulator not found\n", __func__);
		reg_lcd_3v0 = NULL;
	} else {
		regulator_set_voltage(reg_lcd_3v0, 3000*1000,3000*1000);
//		regulator_enable(reg_lcd_3v0);
	}

	n1_disp1_spi = spi;

#if 0 
	//n1_panel_early_suspend.level =  EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;/* EARLY_SUSPEND_LEVEL_DISABLE_FB; */
	n1_panel_early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING;
	n1_panel_early_suspend.suspend = panel_n1_spi_suspend;
	n1_panel_early_suspend.resume = panel_n1_spi_resume;
	register_early_suspend(&n1_panel_early_suspend);
#endif
	n1_panel_enable();

	return 0;
}

static int __devexit panel_n1_spi_remove(struct spi_device *spi)
{
	return 0;
}

static struct spi_driver panel_n1_spi_driver = {
	.driver = {
		.name	= "panel_n1_spi",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe = panel_n1_spi_probe,
	.remove = __devexit_p(panel_n1_spi_remove),
#if !(defined CONFIG_HAS_EARLYSUSPEND)	
	.suspend = panel_n1_spi_suspend,
	.resume = panel_n1_spi_resume,
#endif
	.shutdown = panel_n1_spi_shutdown,
};

static int __init panel_n1_init(void)
{
	//if (!machine_is_n1()) return 0;

	return spi_register_driver(&panel_n1_spi_driver);
}

static void __exit panel_n1_exit(void)
{
	//if (!machine_is_n1()) return;

	spi_unregister_driver(&panel_n1_spi_driver);
}
module_init(panel_n1_init);
module_exit(panel_n1_exit);

MODULE_DESCRIPTION("LCD Driver");
MODULE_LICENSE("GPL");
