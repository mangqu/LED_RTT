/*
 * File      : led.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */
#include <rtthread.h>
#include <stm32f10x.h>
#include "led.h"

// led define
#define led1_rcc                    RCC_APB2Periph_GPIOA
#define led1_gpio                   GPIOA
#define led1_pin                    (GPIO_Pin_5)

#define led2_rcc                    RCC_APB2Periph_GPIOA
#define led2_gpio                   GPIOA
#define led2_pin                    (GPIO_Pin_6)

struct rt_device_led* led;

void led_on(uint8_t num)
{
	switch (num)
	{
		case 1:
			GPIO_SetBits(led1_gpio, led1_pin);
			break;
		case 2:
			GPIO_SetBits(led2_gpio, led2_pin);
			break;
		default:
			break;
	}
}

void led_off(uint8_t num)
{
	switch (num)
	{
		case 1:
			GPIO_ResetBits(led1_gpio, led1_pin);
			break;
		case 2:
			GPIO_ResetBits(led2_gpio, led2_pin);
			break;
		default:
			break;
	}
}

const static struct rt_led_ops _stm32_led_ops =
{
	led_on,
	led_off,
};


void rt_hw_led_init(void)
{
	rt_err_t result;
	/* config gpio */
	{
		GPIO_InitTypeDef GPIO_InitStructure;

		RCC_APB2PeriphClockCmd(led1_rcc, ENABLE);
		RCC_APB2PeriphClockCmd(led2_rcc, ENABLE);

		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

		GPIO_InitStructure.GPIO_Pin   = led1_pin;
		GPIO_Init(led1_gpio, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin   = led2_pin;
		GPIO_Init(led2_gpio, &GPIO_InitStructure);
	}

	led = (struct rt_device_led *)rt_malloc(sizeof(struct rt_device_led));
	if (led == RT_NULL)
		return;	//no memory yet

	/* clear device structure */
	rt_memset(&(led->parent), 0, sizeof(struct rt_device));

	/* init device structure */
	led->parent.type = RT_Device_Class_Unknown;
	led->parent.init = RT_NULL;
	led->parent.control = RT_NULL;
	led->parent.open = RT_NULL;
	led->parent.close = RT_NULL;
	led->parent.write = RT_NULL;
	led->parent.read = RT_NULL;

	led->ops = &_stm32_led_ops;
	
	/* register led to device */
	result = rt_device_register(&(led->parent), "led", RT_DEVICE_FLAG_WRONLY);
	if (result != RT_EOK)
	{
		rt_kprintf("led device register error!\n");
	}
}

