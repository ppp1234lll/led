/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-06
 * @brief       内存管理 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 阿波罗 H743开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "main.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/SDRAM/sdram.h"
#include "./USMART/usmart.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/LCD/ltdc.h"
#include "./MALLOC/malloc.h"

#define PWR_TST_READ  HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_1) 

/* 本.c文件调用的函数 */
static void DeviceRstReason(void);
static void read_boot_update_param(struct BOOT_UPDATE_PARAM *boot_update_param);
static void write_boot_update_param(struct BOOT_UPDATE_PARAM *boot_update_param);
static void update_check_function(void);
static void pwr_test_init(void);
static void led_show_control(uint8_t mode);

int main(void)
{
	sys_cache_enable();				/* 打开L1-Cache */
	HAL_Init();								/* 初始化HAL库 */
	SystemClock_Config();			/* 设置时钟, 480Mhz */
	delay_init(480);					/* 延时初始化 */
	mpu_memory_protection();	/* 保护相关存储区域 */
	bsp_InitLed();         		/* LED初始化 */ 
	pwr_test_init();
	bsp_InitUsart2(115200);
	usart_init(115200);				/* 串口初始化 */
	DeviceRstReason();
	iwdg_init(IWDG_PRESCALER_64, 1000); /* 预分频数为 64,重载值为 1000,溢出时间为 2s */
  update_check_function();
	while(1)
	{
		printf("usart1\n");
		delay_ms(1000);
	}
}


/*
*********************************************************************************************************
*	函 数 名: update_check_function
*	功能说明: 更新检测
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void update_check_function(void)
{
	struct BOOT_UPDATE_PARAM boot_update_param = {0};
	unsigned int ii;
	unsigned int read_addr = 0, write_addr = 0;
	unsigned char *app_buff = NULL;
	uint8_t count = 30;
	////

	while(count--)
	{
    iwdg_feed();
		delay_ms(100);
		if(PWR_TST_READ == 0) 
			break; 
	}
  count = 200;
  while(count--)
	{
     delay_ms(10);
     iwdg_feed();
  }
	
	my_mem_init(SRAMIN);	// 内存初始化

	// 读取升级参数
	read_boot_update_param(&boot_update_param);

	// 判断升级标志,直接跳转
	if( boot_update_param.is_update != 1 )
	{
		printf("\n无需升级，直接执行 main 模块 ...\n");
		iwdg_feed();
		iap_load_app(MAIN_APP_ADDR); // 执行FLASH APP代码
		return;
	}

	// 执行更新
	printf("\n执行升级程序 ..... \n");
	boot_update_param.is_update = false; // 关闭标志
	write_boot_update_param(&boot_update_param); // 保存升级参数

	app_buff = (unsigned char *)mymalloc(SRAMIN, (boot_update_param.section_size + 64));

	printf("\n执行升级参数,section_count: %u, section_size: %u\n", boot_update_param.section_count, boot_update_param.section_size);
	
	// 写入BIN文件
	for(ii = 0; ii < boot_update_param.section_count; ii++)
	{
		iwdg_feed();
		led_show_control(ii % 256); // led灯效果

		// 读一块
		read_addr = UPDATA_SPIFLASH_ADDR + (ii * boot_update_param.section_size);
//		W25QXX_Read(app_buff, read_addr, boot_update_param.section_size);

		// 写入一块
		write_addr = MAIN_APP_ADDR + (ii * boot_update_param.section_size);
		iap_write_appbin(write_addr, app_buff, boot_update_param.section_size);
	}  
	
	myfree(SRAMIN, (void *)app_buff);

	iwdg_feed();

	printf("\n升级完毕！跳转 ...\n");
	iap_load_app(MAIN_APP_ADDR); // 执行FLASH APP代码
}

/*
*********************************************************************************************************
*	函 数 名: DeviceRstReason
*	功能说明: 判断硬件重启原因
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void DeviceRstReason(void)
{
	uint8_t ret=0;
	
	if( __HAL_RCC_GET_FLAG(RCC_FLAG_CPURST)== 1U)
	{
		printf("CPU reset\n");
		ret = 1;
	}
	if( __HAL_RCC_GET_FLAG(RCC_FLAG_D1RST)== 1U)
	{
		printf("D1 domain power switch reset\n");
		ret = 2;
	}
	if( __HAL_RCC_GET_FLAG(RCC_FLAG_D2RST)== 1U)
	{
		printf("D2 domain power switch reset\n");
		ret = 3;
	}
	if( __HAL_RCC_GET_FLAG(RCC_FLAG_BORRST)== 1U)
	{
		printf("BOR reset\n");
		ret = 5;
	}
	if( __HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)== 1U)
	{
		ret = 6;
		printf("Pin reset\n");
	}
	if( __HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)== 1U)
	{
		ret = 7;
		printf("POR/PDR  reset\n");
	}	
	if( __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)== 1U)
	{
		ret = 8;
		printf("System reset from CPU reset\n");
	}	
	if( __HAL_RCC_GET_FLAG(RCC_FLAG_BORRST)== 1U)
	{
		ret = 9;
		printf("D2 domain power switch reset\n");
	}	
	if( __HAL_RCC_GET_FLAG(RCC_FLAG_IWDG1RST)== 1U)
	{
		ret = 10;
		printf("CPU Independent Watchdog reset\n");
	}		

	// 清除所有复位标志位（必须操作，否则下次复位标志不会更新）
	__HAL_RCC_CLEAR_RESET_FLAGS();
}

/*
*********************************************************************************************************
*	函 数 名: read_boot_update_param
*	功能说明: 读取更新信息
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void read_boot_update_param(struct BOOT_UPDATE_PARAM *boot_update_param)
{
//	W25QXX_Read((uint8_t*)boot_update_param, UPDATA_PARAM_ADDR, sizeof(struct BOOT_UPDATE_PARAM));
}

/*
*********************************************************************************************************
*	函 数 名: write_boot_update_param
*	功能说明: 保存升级参数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void write_boot_update_param(struct BOOT_UPDATE_PARAM *boot_update_param)
{
//	W25QXX_Write((uint8_t *)boot_update_param, UPDATA_PARAM_ADDR, sizeof(struct BOOT_UPDATE_PARAM));
}


/*
*********************************************************************************************************
*	函 数 名: pwr_test_init
*	功能说明: 输入检测
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void pwr_test_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOF_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}
/*
*********************************************************************************************************
*	函 数 名: led_show_control
*	功能说明: LED灯光控制，显示更新进度
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void led_show_control(uint8_t mode)
{
	static uint8_t flag = 0;
	uint8_t num = mode % 100;
	if(num<50 && flag == 0) 
	{
		flag = 1;
		led_all_on();
	} 
	else if(num >=50 && flag == 1)
	{
		flag = 0;
		led_all_off();
	}
}



/****************************************** END OF FILE **********************************************/
