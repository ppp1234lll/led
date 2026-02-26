/**
 ****************************************************************************************************
 * @file        wdg.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-09-06
 * @brief       看门狗 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 ****************************************************************************************************
 */

#include "bsp_iwdg.h"


IWDG_HandleTypeDef g_iwdg_handle;                       /* 独立看门狗句柄 */

#define SOFT_IWDG_ENABLE  0  // 软件看门狗
#define HARD_IWDG_ENABLE  0  // 硬件看门狗

/**
 * @brief       初始化独立看门狗 
 * @param       prer  : IWDG_PRESCALER_4~IWDG_PRESCALER_256,对应4~256分频
 *   @arg       分频因子 = 4 * 2^prer. 但最大值只能是256!
 * @param       rlr   : 自动重装载值,0~0XFFF. 
 * @note        时间计算(大概):Tout=((4 * 2^prer) * rlr) / 32 (ms) 
 * @retval      无
 */
void iwdg_init(uint32_t prer, uint16_t rlr)
{
#if SOFT_IWDG_ENABLE > 0U
    g_iwdg_handle.Instance = IWDG1;
    g_iwdg_handle.Init.Prescaler = prer;                /* 设置IWDG分频系数 */
    g_iwdg_handle.Init.Reload = rlr;                    /* 从加载寄存器 IWDG->RLR 重装载值 */
    g_iwdg_handle.Init.Window = IWDG_WINDOW_DISABLE;    /* 关闭窗口功能 */
    HAL_IWDG_Init(&g_iwdg_handle);                      /* 初始化IWDG并使能 */
#endif
	
#if HARD_IWDG_ENABLE > 0U
	
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);
	
#endif	
	
}

/**
 * @brief       喂独立看门狗
 * @param       无
 * @retval      无
 */
void iwdg_feed(void)
{
	#if SOFT_IWDG_ENABLE > 0U
		HAL_IWDG_Refresh(&g_iwdg_handle);                   /* 喂狗 */
	#endif	

#if HARD_IWDG_ENABLE > 0U	
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
#endif
}
