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
void SystemClock_Config(void);
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
//	sys_nvic_set_vector_table(FLASH_BASE, 0x20000);	/* 设置中断向量偏移 */ 
	Write_Through();
	mpu_memory_protection();                 /* 保护相关存储区域 */
	sys_cache_enable();                      /* 打开L1-Cache */
	HAL_Init();                              /* 初始化HAL库 */
	sys_stm32_clock_init(192, 5, 2, 4);      /* 设置时钟, 480Mhz */
//	SystemClock_Config();
//	delay_init(400); 
	delay_init(480);                         /* 延时初始化 */
	
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

}

void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	HAL_StatusTypeDef ret = HAL_OK;

	/* 锁住SCU(Supply configuration update) */
	MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);

	/* 
      1、芯片内部的LDO稳压器输出的电压范围，可选VOS1，VOS2和VOS3，不同范围对应不同的Flash读速度，
         详情看参考手册的Table 12的表格。
      2、这里选择使用VOS1，电压范围1.15V - 1.26V。
    */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

	/* 使能HSE，并选择HSE作为PLL时钟源 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
		
	RCC_OscInitStruct.PLL.PLLM = 5;
	RCC_OscInitStruct.PLL.PLLN = 160;
	RCC_OscInitStruct.PLL.PLLP = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLQ = 4;		
		
	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;	
	ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if(ret != HAL_OK)
	{
        Error_Handler(__FILE__, __LINE__);
	}

	/* 
       选择PLL的输出作为系统时钟
       配置RCC_CLOCKTYPE_SYSCLK系统时钟
       配置RCC_CLOCKTYPE_HCLK 时钟，对应AHB1，AHB2，AHB3和AHB4总线
       配置RCC_CLOCKTYPE_PCLK1时钟，对应APB1总线
       配置RCC_CLOCKTYPE_PCLK2时钟，对应APB2总线
       配置RCC_CLOCKTYPE_D1PCLK1时钟，对应APB3总线
       配置RCC_CLOCKTYPE_D3PCLK1时钟，对应APB4总线     
  */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
								 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

	RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
	RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
	RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
	
	/* 此函数会更新SystemCoreClock，并重新配置HAL_InitTick */
	ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
	if(ret != HAL_OK)
	{
        Error_Handler(__FILE__, __LINE__);
	}

	/*
		使用IO的高速模式，要使能IO补偿，即调用下面三个函数 
		（1）使能CSI clock
		（2）使能SYSCFG clock
		（3）使能I/O补偿单元， 设置SYSCFG_CCCSR寄存器的bit0
	*/
	__HAL_RCC_CSI_ENABLE() ;
	__HAL_RCC_SYSCFG_CLK_ENABLE() ;

	HAL_EnableCompensationCell();

   /* AXI SRAM的时钟是上电自动使能的，而D2域的SRAM1，SRAM2和SRAM3要单独使能 */	
#if 1
	__HAL_RCC_D2SRAM1_CLK_ENABLE();
	__HAL_RCC_D2SRAM2_CLK_ENABLE();
	__HAL_RCC_D2SRAM3_CLK_ENABLE();
#endif
}




