#include <rtthread.h>
#include <board.h>

#ifdef RT_USING_LWIP
#include "stm32_eth.h"
#endif /* RT_USING_LWIP */

#ifdef RT_USING_SPI
#include "rt_stm32f10x_spi.h"

#if defined(RT_USING_DFS) && defined(RT_USING_DFS_ELMFAT)
#include "msd.h"
#endif /* RT_USING_DFS */

#include "spi_flash_w25qxx.h"
/*
 * SPI1_MOSI: PB5
 * SPI1_MISO: PB4
 * SPI1_SCK : PB3
 *
 * 
*/
static void rt_hw_spi_init(void)
{
#ifdef RT_USING_SPI1
	/* register spi bus */
	{
		static struct stm32_spi_bus stm32_spi;
		GPIO_InitTypeDef GPIO_InitStructure;

		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);

		/* Enable GPIO clock */
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

		GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
		GPIO_Init(GPIOB, &GPIO_InitStructure);

		stm32_spi_register(SPI1, &stm32_spi, "spi1");
	}

	/* attach cs0 */
	{
		static struct rt_spi_device spi_device;
		static struct stm32_spi_cs  spi_cs;

		GPIO_InitTypeDef GPIO_InitStructure;

		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

		/* spi10: PA4 */
		spi_cs.GPIOx = GPIOA;
		spi_cs.GPIO_Pin = GPIO_Pin_4;
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

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
		w25qxx_init("flash0", "spi10");
	}

#endif /* RT_USING_DFS && RT_USING_DFS_ELMFAT */

#endif // RT_USING_SPI

#ifdef RT_USING_LWIP
	/* initialize eth interface */
	rt_hw_stm32_eth_init();
#endif /* RT_USING_LWIP */

}
