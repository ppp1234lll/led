/*
*********************************************************************************************************
*
*	模块名称 : TIM基本定时中断和PWM驱动模块
*	文件名称 : bsp_tim_pwm.c
*	版    本 : V1.6
*	说    明 : 利用STM32H7内部TIM输出PWM信号， 并实现基本的定时中断
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2013-08-16 armfly  正式发布
*		V1.1	2014-06-15 armfly  完善 bsp_SetTIMOutPWM，当占空比=0和100%时，关闭定时器，GPIO配置为输出
*		V1.2	2015-05-08 armfly  解决TIM8不能输出PWM的问题。
*		V1.3	2015-07-23 armfly  初始化定时器，必须设置 TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0x0000;		
*								   TIM1 和 TIM8 必须设置。否则蜂鸣器的控制不正常。
*		V1.4	2015-07-30 armfly  增加反相引脚输出PWM函数 bsp_SetTIMOutPWM_N();
*		V1.5	2016-02-01 armfly  去掉 TIM_OC1PreloadConfig(TIMx, TIM_OCPreload_Enable);
*		V1.6	2016-02-27 armfly  解决TIM14无法中断的BUG, TIM8_TRG_COM_TIM14_IRQn
*
*	Copyright (C), 2018-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"
#include "bsp_timers.h"

#define  TIMER_DEBUG

#ifdef TIMER_DEBUG
__attribute__((section (".RAM_SRAM4"))) uint16_t g_timer_test[16];
#endif

/*
   注意，STM32H7有TIM1 – TIM17（没有TIM9，TIM10和TIM11）共计14个定时器。
bsp.c 文件中 void SystemClock_Config(void) 函数对时钟的配置如下: 

	System Clock source       = PLL (HSE)
	SYSCLK(Hz)                = 480000000 (CPU Clock)
	HCLK(Hz)                  = 240000000 (AXI and AHBs Clock)
	AHB Prescaler             = 2
	D1 APB3 Prescaler         = 2 (APB3 Clock  120MHz)
	D2 APB1 Prescaler         = 2 (APB1 Clock  120MHz)
	D2 APB2 Prescaler         = 2 (APB2 Clock  120MHz)
	D3 APB4 Prescaler         = 2 (APB4 Clock  120MHz)

	因为APB1 prescaler != 1, 所以 APB1上的TIMxCLK = APB1 x 2 = 240MHz;
	因为APB2 prescaler != 1, 所以 APB2上的TIMxCLK = APB2 x 2 = 240MHz;
	APB4上面的TIMxCLK没有分频，所以就是120MHz;

	APB1 定时器有 TIM2, TIM3 ,TIM4, TIM5, TIM6, TIM7, TIM12, TIM13, TIM14，LPTIM1
	APB2 定时器有 TIM1, TIM8 , TIM15, TIM16，TIM17

	APB4 定时器有 LPTIM2，LPTIM3，LPTIM4，LPTIM5
*/

/*
*********************************************************************************************************
*	函 数 名: bsp_SetTIMforInt
*	功能说明: 配置TIM和NVIC，用于简单的定时中断，开启定时中断。另外注意中断服务程序需要由用户应用程序实现。
*	形    参: TIMx : 定时器
*			  _ulFreq : 定时频率 （Hz）。 0 表示关闭。
*			  _PreemptionPriority : 抢占优先级
*			  _SubPriority : 子优先级
*	返 回 值: 无
*********************************************************************************************************
*/

void bsp_InitTimers(TIM_TypeDef* TIMx, uint32_t _ulFreq, uint8_t _PreemptionPriority, uint8_t _SubPriority)
{
	TIM_HandleTypeDef   TimHandle = {0};
	uint16_t usPeriod;
	uint16_t usPrescaler;
	uint32_t uiTIMxCLK;
	
	/* 使能TIM时钟 */
	bsp_RCC_TIM_Enable(TIMx);
	
	if ((TIMx == TIM1) || (TIMx == TIM8) || (TIMx == TIM15) || (TIMx == TIM16) || (TIMx == TIM17))
	{
		/* APB2 定时器时钟 = 240M */
		uiTIMxCLK = SystemCoreClock / 2;
	}
	else	
	{
		/* APB1 定时器 = 240M */
		uiTIMxCLK = SystemCoreClock / 2;
	}

	if (_ulFreq < 100)
	{
		usPrescaler = 10000 - 1;					/* 分频比 = 10000 */
		usPeriod =  (uiTIMxCLK / 10000) / _ulFreq  - 1;		/* 自动重装的值 */
	}
	else if (_ulFreq < 3000)
	{
		usPrescaler = 100 - 1;					/* 分频比 = 100 */
		usPeriod =  (uiTIMxCLK / 100) / _ulFreq  - 1;		/* 自动重装的值 */
	}
	else	/* 大于4K的频率，无需分频 */
	{
		usPrescaler = 0;					/* 分频比 = 1 */
		usPeriod = uiTIMxCLK / _ulFreq - 1;	/* 自动重装的值 */
	}

	/* 
       定时器中断更新周期 = TIMxCLK / usPrescaler + 1）/usPeriod + 1）
	*/
	TimHandle.Instance = TIMx;
	TimHandle.Init.Prescaler         = usPrescaler;
	TimHandle.Init.Period            = usPeriod;	
	TimHandle.Init.ClockDivision     = 0;
	TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
	TimHandle.Init.RepetitionCounter = 0;
	TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* 使能定时器中断  */
	__HAL_TIM_ENABLE_IT(&TimHandle, TIM_IT_UPDATE);
	
	/* 配置TIM定时更新中断 (Update) */
	{
		uint8_t irq = 0;	/* 中断号, 定义在 stm32h7xx.h */

		if (TIMx == TIM1) irq = TIM1_UP_IRQn;
		else if (TIMx == TIM2) irq = TIM2_IRQn;
		else if (TIMx == TIM3) irq = TIM3_IRQn;
		else if (TIMx == TIM4) irq = TIM4_IRQn;
		else if (TIMx == TIM5) irq = TIM5_IRQn;
		else if (TIMx == TIM6) irq = TIM6_DAC_IRQn;
		else if (TIMx == TIM7) irq = TIM7_IRQn;
		else if (TIMx == TIM8) irq = TIM8_UP_TIM13_IRQn;
		else if (TIMx == TIM12) irq = TIM8_BRK_TIM12_IRQn;
		else if (TIMx == TIM13) irq = TIM8_UP_TIM13_IRQn;
		else if (TIMx == TIM14) irq = TIM8_TRG_COM_TIM14_IRQn;
		else if (TIMx == TIM15) irq = TIM15_IRQn;
		else if (TIMx == TIM16) irq = TIM16_IRQn;
		else if (TIMx == TIM17) irq = TIM17_IRQn;
		else
		{
				Error_Handler(__FILE__, __LINE__);
		}	
		HAL_NVIC_SetPriority((IRQn_Type)irq, _PreemptionPriority, _SubPriority);
		HAL_NVIC_EnableIRQ((IRQn_Type)irq);		
	}
	
	HAL_TIM_Base_Start(&TimHandle);
}

/*
*********************************************************************************************************
*	函 数 名: bsp_RCC_TIM_Enable
*	功能说明: 使能TIM RCC 时钟
*	形    参: TIMx TIM1 - TIM17
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_RCC_TIM_Enable(TIM_TypeDef* TIMx)
{
	if (TIMx == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
	else if (TIMx == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
	else if (TIMx == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
	else if (TIMx == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
	else if (TIMx == TIM5) __HAL_RCC_TIM5_CLK_ENABLE();
	else if (TIMx == TIM6) __HAL_RCC_TIM6_CLK_ENABLE();
	else if (TIMx == TIM7) __HAL_RCC_TIM7_CLK_ENABLE();
	else if (TIMx == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();
//	else if (TIMx == TIM9) __HAL_RCC_TIM9_CLK_ENABLE();
//	else if (TIMx == TIM10) __HAL_RCC_TIM10_CLK_ENABLE();
//	else if (TIMx == TIM11) __HAL_RCC_TIM11_CLK_ENABLE();
	else if (TIMx == TIM12) __HAL_RCC_TIM12_CLK_ENABLE();
	else if (TIMx == TIM13) __HAL_RCC_TIM13_CLK_ENABLE();
	else if (TIMx == TIM14) __HAL_RCC_TIM14_CLK_ENABLE();
	else if (TIMx == TIM15) __HAL_RCC_TIM15_CLK_ENABLE();
	else if (TIMx == TIM16) __HAL_RCC_TIM16_CLK_ENABLE();
	else if (TIMx == TIM17) __HAL_RCC_TIM17_CLK_ENABLE();	
	else
	{
		Error_Handler(__FILE__, __LINE__);
	}	
}

/*
*********************************************************************************************************
*	函 数 名: bsp_RCC_TIM_Disable
*	功能说明: 关闭TIM RCC 时钟
*	形    参: TIMx TIM1 - TIM17
*	返 回 值: TIM外设时钟名
*********************************************************************************************************
*/
void bsp_RCC_TIM_Disable(TIM_TypeDef* TIMx)
{
	/*
        APB1 定时器有 TIM2, TIM3 ,TIM4, TIM5, TIM6, TIM7, TIM12, TIM13, TIM14
        APB2 定时器有 TIM1, TIM8 , TIM15, TIM16，TIM17
	*/
	if (TIMx == TIM1) __HAL_RCC_TIM3_CLK_DISABLE();
	else if (TIMx == TIM2) __HAL_RCC_TIM2_CLK_DISABLE();
	else if (TIMx == TIM3) __HAL_RCC_TIM3_CLK_DISABLE();
	else if (TIMx == TIM4) __HAL_RCC_TIM4_CLK_DISABLE();
	else if (TIMx == TIM5) __HAL_RCC_TIM5_CLK_DISABLE();
	else if (TIMx == TIM6) __HAL_RCC_TIM6_CLK_DISABLE();
	else if (TIMx == TIM7) __HAL_RCC_TIM7_CLK_DISABLE();
	else if (TIMx == TIM8) __HAL_RCC_TIM8_CLK_DISABLE();
//	else if (TIMx == TIM9) __HAL_RCC_TIM9_CLK_DISABLE();
//	else if (TIMx == TIM10) __HAL_RCC_TIM10_CLK_DISABLE();
//	else if (TIMx == TIM11) __HAL_RCC_TIM11_CLK_DISABLE();
	else if (TIMx == TIM12) __HAL_RCC_TIM12_CLK_DISABLE();
	else if (TIMx == TIM13) __HAL_RCC_TIM13_CLK_DISABLE();
	else if (TIMx == TIM14) __HAL_RCC_TIM14_CLK_DISABLE();
	else if (TIMx == TIM15) __HAL_RCC_TIM15_CLK_DISABLE();
	else if (TIMx == TIM16) __HAL_RCC_TIM16_CLK_DISABLE();
	else if (TIMx == TIM17) __HAL_RCC_TIM17_CLK_DISABLE();
	else
	{
		Error_Handler(__FILE__, __LINE__);
	}
}
/*
*********************************************************************************************************
*	函 数 名: TIM2_IRQHandler
*	功能说明: 定时器中断函数
*	形    参: 无
*	返 回 值: 无
*	TIM定时中断服务程序范例，必须清中断标志
*********************************************************************************************************
*/
void TIM2_IRQHandler(void)
{
	if((TIM2->SR & TIM_FLAG_UPDATE) != RESET)
	{
		TIM2->SR = ~ TIM_FLAG_UPDATE;
		
#ifdef TIMER_DEBUG
		g_timer_test[0]++;
		if(g_timer_test[0] >= 1000)
		{
			g_timer_test[0] = 0;
			printf("TIM2 test\n");
		}
#endif		
	}
}

/*
*********************************************************************************************************
*	函 数 名: TIM3_IRQHandler
*	功能说明: 定时器中断函数
*	形    参: 无
*	返 回 值: 无
*	TIM定时中断服务程序范例，必须清中断标志
*********************************************************************************************************
*/
void TIM3_IRQHandler(void)
{
	if((TIM3->SR & TIM_FLAG_UPDATE) != RESET)
	{
		TIM3->SR = ~ TIM_FLAG_UPDATE;
		
#ifdef TIMER_DEBUG
		g_timer_test[1]++;
		if(g_timer_test[1] >= 1000)
		{
			g_timer_test[1] = 0;
			printf("TIM3 test\n");
		}
#endif		
	}
}

/*
*********************************************************************************************************
*	函 数 名: TIM4_IRQHandler
*	功能说明: 定时器中断函数
*	形    参: 无
*	返 回 值: 无
*	TIM定时中断服务程序范例，必须清中断标志
*********************************************************************************************************
*/
void TIM4_IRQHandler(void)
{
	if((TIM4->SR & TIM_FLAG_UPDATE) != RESET)
	{
		TIM4->SR = ~ TIM_FLAG_UPDATE;
		
#ifdef TIMER_DEBUG
		g_timer_test[2]++;
		if(g_timer_test[2] >= 1000)
		{
			g_timer_test[2] = 0;
			printf("TIM4 test\n");
		}
#endif		
	}
}

/*
*********************************************************************************************************
*	函 数 名: TIM5_IRQHandler
*	功能说明: 定时器中断函数
*	形    参: 无
*	返 回 值: 无
*	TIM定时中断服务程序范例，必须清中断标志
*********************************************************************************************************
*/
void TIM5_IRQHandler(void)
{
	if((TIM5->SR & TIM_FLAG_UPDATE) != RESET)
	{
		TIM5->SR = ~ TIM_FLAG_UPDATE;
		
#ifdef TIMER_DEBUG
		g_timer_test[3]++;
		if(g_timer_test[3] >= 1000)
		{
			g_timer_test[3] = 0;
			printf("TIM5 test\n");
		}
#endif		
	}
}

/*
*********************************************************************************************************
*	函 数 名: TIM6_DAC_IRQHandler
*	功能说明: 定时器中断函数
*	形    参: 无
*	返 回 值: 无
*	TIM定时中断服务程序范例，必须清中断标志
*********************************************************************************************************
*/
void TIM6_DAC_IRQHandler(void)
{
	if((TIM6->SR & TIM_FLAG_UPDATE) != RESET)
	{
		TIM6->SR = ~ TIM_FLAG_UPDATE;
		
#ifdef TIMER_DEBUG
		g_timer_test[4]++;
		if(g_timer_test[4] >= 1000)
		{
			g_timer_test[4] = 0;
			printf("TIM6 test\n");
		}
#endif		
	}
}

/*
*********************************************************************************************************
*	函 数 名: TIM7_IRQHandler
*	功能说明: 定时器中断函数
*	形    参: 无
*	返 回 值: 无
*	TIM定时中断服务程序范例，必须清中断标志
*********************************************************************************************************
*/
void TIM7_IRQHandler(void)
{
	if((TIM7->SR & TIM_FLAG_UPDATE) != RESET)
	{
		TIM7->SR = ~ TIM_FLAG_UPDATE;
		
#ifdef TIMER_DEBUG
		g_timer_test[5]++;
		if(g_timer_test[5] >= 1000)
		{
			g_timer_test[5] = 0;
			printf("TIM7 test\n");
		}
#endif		
	}
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
