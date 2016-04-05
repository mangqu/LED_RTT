#include <rtthread.h>
#include <stm32f10x.h>
#include "key.h"

// key define
#define key1_rcc                    RCC_APB2Periph_GPIOA
#define key1_gpio                   GPIOA
#define key1_pin                    (GPIO_Pin_5)
#define key1_exti_line				EXTI_Line5
#define key1_portsource			GPIO_PortSourceGPIOA
#define key1_pinsource				GPIO_PinSource5

#define key2_rcc                    RCC_APB2Periph_GPIOA
#define key2_gpio                   GPIOA
#define key2_pin                    (GPIO_Pin_6)
#define key2_exti_line				EXTI_Line6
#define key2_portsource			GPIO_PortSourceGPIOA
#define key2_pinsource				GPIO_PinSource6

struct rt_device_key* key;

static void EXTI_Enable(unsigned char key, unsigned char isenable)
{
	EXTI_InitTypeDef EXTI_InitStructure;

	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;

	if (key == 1)
	{
		EXTI_InitStructure.EXTI_Line = key1_exti_line;
	}
	else if (key == 2)
	{
		EXTI_InitStructure.EXTI_Line = key2_exti_line;
	}
	else
	{
		return;
	}
	
	if (isenable == 1)
	{
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	}
	else if (isenable == 0)
	{
		EXTI_InitStructure.EXTI_LineCmd = DISABLE;
	}
	else
	{
		return;
	}

	EXTI_Init(&EXTI_InitStructure);
}

static void EXTI_Configuration()
{
	/* key1, key2 */
	{
		GPIO_InitTypeDef GPIO_InitStructure;

		RCC_APB2PeriphClockCmd(key1_rcc, ENABLE);
		RCC_APB2PeriphClockCmd(key2_rcc, ENABLE);

		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

		GPIO_InitStructure.GPIO_Pin   = key1_pin;
		GPIO_Init(key1_gpio, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin   = key2_pin;
		GPIO_Init(key2_gpio, &GPIO_InitStructure);
	}

	GPIO_EXTILineConfig(key1_portsource, key1_pinsource);
	GPIO_EXTILineConfig(key2_portsource, key2_pinsource);

	/* Configure  EXTI  */
    EXTI_Enable(1, 1);
	EXTI_Enable(2, 1);
}

static void NVIC_Configuration()
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
}

void EXTI9_5_IRQHandler(void)
{
	if ((EXTI_GetITStatus(key1_exti_line) == SET) ||
		(EXTI_GetITStatus(key2_exti_line) == SET))
	{
		/* disable interrupt */
		EXTI_Enable(1, 0);
		EXTI_Enable(2, 0);

		/* start timer */
		rt_timer_start(key->poll_timer);

		EXTI_ClearITPendingBit(key1_exti_line | key2_exti_line);
	}
}

unsigned char key_scan()
{
	unsigned char key_value = 0;
	if (GPIO_ReadInputDataBit(key1_gpio, key1_pin) == RESET)
	{
		//TODO
		key_value = 1;
	}
	else if (GPIO_ReadInputDataBit(key2_gpio, key2_pin) == RESET)
	{
		//TODO
		key_value = 2;
	}

	EXTI_Enable(1, 1);
	EXTI_Enable(2, 1);

	return key_value;
}

void key_timeout(void* parameter)
{
	key->ops->key_scan();
}

const static struct rt_key_ops _stm32_key_ops =
{
	key_scan,
};

void rt_hw_key_init(void)
{
	rt_err_t result;
	/* config gpio */
	{
		EXTI_Configuration();
		NVIC_Configuration();
	}

	key = (struct rt_device_key *)rt_malloc(sizeof(struct rt_device_key));
	if (key == RT_NULL)
		return;	//no memory yet

	/* clear device structure */
	rt_memset(&(key->parent), 0, sizeof(struct rt_device));

	/* init device structure */
	key->parent.type = RT_Device_Class_Unknown;
	key->parent.init = RT_NULL;
	key->parent.control = RT_NULL;
	key->parent.open = RT_NULL;
	key->parent.close = RT_NULL;
	key->parent.write = RT_NULL;
	key->parent.read = RT_NULL;

	key->poll_timer = rt_timer_create("key scan", key_timeout, RT_NULL, 
		RT_TICK_PER_SECOND/20, RT_TIMER_FLAG_PERIODIC);
	key->ops = &_stm32_key_ops;
	
	/* register led to device */
	result = rt_device_register(&(key->parent), "key", RT_DEVICE_FLAG_WRONLY);
	if (result != RT_EOK)
	{
		rt_kprintf("key device register error!\n");
	}
}

