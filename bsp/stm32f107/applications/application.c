/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2013, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <board.h>
#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#include <dfs_init.h>
#include <dfs_elm.h>
#endif

#ifdef RT_USING_LWIP
#include <stm32_eth.h>
#include <netif/ethernetif.h>
extern int lwip_system_init(void);
#endif

#ifdef RT_USING_FINSH
#include <shell.h>
#include <finsh.h>
#endif

#include "led.h"
#include "key.h"

void led_thread_entry(void* parameter)
{
	extern struct rt_device_led* led;
	unsigned int count=0;

	rt_hw_led_init();
	
	while (1)
	{
	    /* led1 on */
#ifndef RT_USING_FINSH
	    rt_kprintf("led on, count : %d\r\n",count);
#endif
	    count++;
		led->ops->led_on(&(led->parent));
	    rt_thread_delay( RT_TICK_PER_SECOND/2 ); /* sleep 0.5 second and switch to other thread */

	    /* led1 off */
#ifndef RT_USING_FINSH
	    rt_kprintf("led off\r\n");
#endif
		led->ops->led_off(&(led->parent));
	    rt_thread_delay( RT_TICK_PER_SECOND/2 );
	}
	
}

void key_thread_entry(void* parameter)
{
	extern struct rt_device_key* key;

	rt_hw_key_init();

	while (1)
	{
		rt_thread_delay(10);
	}
}

void rt_init_thread_entry(void* parameter)
{
    {
        extern void rt_platform_init(void);
       rt_platform_init();
    }

    /* Filesystem Initialization */
#if defined(RT_USING_DFS) && defined(RT_USING_DFS_ELMFAT)
	/* initialize the device file system */
	dfs_init();

	/* initialize the elm chan FatFS file system*/
	elm_init();
    
    /* mount sd card fat partition 1 as root directory */
    if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
    {
        rt_kprintf("File System initialized!\n");
    }
    else
    {
        rt_kprintf("File System initialzation failed!\n");
    }
#endif /* RT_USING_DFS && RT_USING_DFS_ELMFAT */

#ifdef RT_USING_LWIP
	/* initialize lwip stack */
	/* register ethernetif device */
	eth_system_device_init();

	/* initialize lwip system */
	lwip_system_init();
	rt_kprintf("TCP/IP initialized!\n");

//	telnet_srv();
//	tcpserv();
//	tcpclient("172.24.120.222", 8000);
//	udpserv();
//	udpclient("172.24.120.228", 8080, 10);
//	websrv();
//	mqtt_client();
#endif

#ifdef RT_USING_FINSH
	/* initialize finsh */
	finsh_system_init();
	finsh_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#ifdef RT_USING_PIN
	stm32_hw_pin_init();
#endif
}

int rt_application_init(void)
{
	rt_thread_t tid;
/*	tid = rt_thread_create("led", 
		led_thread_entry, RT_NULL, 
		1024, 20, 10);
	if (tid != RT_NULL)
	{
		rt_thread_startup(tid);
	}

	tid = rt_thread_create("key", 
		key_thread_entry, RT_NULL, 
		1024, 24, 20);
	if (tid != RT_NULL)
	{
		rt_thread_startup(tid);
	}
*/
	tid = rt_thread_create("init",
	    rt_init_thread_entry, RT_NULL,
	    2048, RT_THREAD_PRIORITY_MAX/3, 20);
	if (tid != RT_NULL) rt_thread_startup(tid);

	return 0;
}

/*@}*/
