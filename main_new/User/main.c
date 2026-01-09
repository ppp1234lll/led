/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-06
 * @brief       lwIP HTTPS 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */
#include "appconfig.h"

#include "freertos_demo.h"

static void system_setup(void);
 

/*
*********************************************************************************************************
*	函 数 名: main
*	功能说明: 主函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
int main(void)
{
	system_setup();
	printf("run 2...\n");
	start_task_create();
}

/*
*********************************************************************************************************
*	函 数 名: system_setup
*	功能说明: 初始化
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void system_setup(void)
{
	sys_nvic_set_vector_table(FLASH_BASE, 0x20000);	/* 设置中断向量偏移 */ 
	Write_Through();
	mpu_memory_protection();                 /* 保护相关存储区域 */
	sys_cache_enable();                      /* 打开L1-Cache */
	HAL_Init();                              /* 初始化HAL库 */
	sys_stm32_clock_init(192, 5, 2, 4);      /* 设置时钟, 480Mhz */
	delay_init(480);                         /* 延时初始化 */
	bsp_InitUsart1(115200);
	/* 
	   Event Recorder：
	   - 可用于代码执行时间测量，MDK5.25及其以上版本才支持，IAR不支持。
	   - 默认不开启，如果要使能此选项，务必看V5开发板用户手册第8章
	*/	
	/* 初始化EventRecorder并开启 */
#ifdef Enable_EventRecorder
	EventRecorderInitialize(EventRecordAll, 1U);
	EventRecorderStart();
#endif 
	
	/* 配置通道0，上行配置*/
	SEGGER_RTT_ConfigUpBuffer(0, "RTTUP", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	/* 配置通道0，下行配置*/	
	SEGGER_RTT_ConfigDownBuffer(0, "RTTDOWN", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);

	cm_backtrace_init("CmBacktrace", "V1.0.0", "V1.0.0");
	
}





