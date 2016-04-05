#include <rtthread.h>
#include <board.h>

#ifdef RT_USING_LWIP
#include "stm32_eth.h"
#endif /* RT_USING_LWIP */

#ifdef RT_USING_SPI
#include "rt_stm32f10x_spi.h"

#if defined(RT_USING_DFS) && defined(RT_USING_DFS_ELMFAT)
#include "msd.h"
#include "spi_flash_sst25vfxx.h"
#endif /* RT_USING_DFS */

/*
 * SPI1_MOSI: PA7
 * SPI1_MISO: PA6
 * SPI1_SCK : PA5
 *
 * 
*/
static void rt_hw_spi_init(void)
{
#ifdef RT_USING_SPI
    /* register spi bus */
    {
		static struct stm32_spi_bus stm32_spi;
		GPIO_InitTypeDef GPIO_InitStructure;

		/* Enable GPIO clock */
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

		GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		stm32_spi_register(SPI1, &stm32_spi, "spi1");
    }

    /* attach cs */
    {
		static struct rt_spi_device spi_device;
		static struct stm32_spi_cs  spi_cs;

		GPIO_InitTypeDef GPIO_InitStructure;

		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

		/* spi10: PC4 */
		spi_cs.GPIOx = GPIOC;
		spi_cs.GPIO_Pin = GPIO_Pin_4;
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

		GPIO_InitStructure.GPIO_Pin = spi_cs.GPIO_Pin;
		GPIO_SetBits(spi_cs.GPIOx, spi_cs.GPIO_Pin);
		GPIO_Init(spi_cs.GPIOx, &GPIO_InitStructure);

		rt_spi_bus_attach_device(&spi_device, "spi10", "spi1", (void*)&spi_cs);
    }
#endif /* RT_USING_SPI1 */
}
#endif /* RT_USING_SPI */


void rt_platform_init(void)
{
#ifdef RT_USING_SPI
	rt_hw_spi_init();

#if defined(RT_USING_DFS) && defined(RT_USING_DFS_ELMFAT)
	/* init sst25 driver */
	{
		sst25vfxx_init("flash0", "spi10");
	}

	/* init sd card */
	{
#ifdef RT_USING_SDIO

#else
		rt_hw_sdcard_init();
#endif /* RT_USING_SDIO*/
	}
#endif /* RT_USING_DFS && RT_USING_DFS_ELMFAT */

#endif // RT_USING_SPI

#ifdef RT_USING_LWIP
	/* initialize eth interface */
	rt_hw_stm32_eth_init();
#endif /* RT_USING_LWIP */

}
