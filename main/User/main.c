/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-06
 * @brief       lwIP+FreeRTOS操作系统移植实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/LCD/ltdc.h"
#include "./BSP/PCF8574/pcf8574.h"
#include "lwip_comm.h"
#include "lwipopts.h"
#include "freertos_demo.h"


int main(void)
{
  sys_nvic_set_vector_table(FLASH_BASE, 0x20000);	/* 设置中断向量偏移 */  
	Write_Through();
	mpu_memory_protection();                 /* 保护相关存储区域 */
	sys_cache_enable();                      /* 打开L1-Cache */
	HAL_Init();                              /* 初始化HAL库 */
	sys_stm32_clock_init(192, 5, 2, 4);      /* 设置时钟, 480Mhz */
	delay_init(480);                         /* 延时初始化 */
	usart_init(115200);                      /* 串口初始化 */

	while(1)
	{
		printf("usrt1\n");
		delay_ms(1000);
	}
	
	

    
    freertos_demo();
}
