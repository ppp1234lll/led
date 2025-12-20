#include "bsp_led.h"
#include "main.h"

/*
	2、指示灯（原理图）：
		外接电源指示灯  POWER_OUT  : PE8
		外接网口指示灯  LAN_OUT    : PE9
		
		系统状态指示灯  STATE      : PB8
		网口指示灯      LAN        : PB9
		4G指示灯        GPRS       : PE0
*/
/******************************************************************************************/
/* 引脚 定义 */

#define LED_STATE_GPIO_PORT				GPIOB
#define LED_STATE_GPIO_PIN				GPIO_PIN_8

#define LED_LAN_GPIO_PORT				  GPIOB
#define LED_LAN_GPIO_PIN				  GPIO_PIN_9

#define LED_GPRS_GPIO_PORT				GPIOB
#define LED_GPRS_GPIO_PIN					GPIO_PIN_0

/******************************************************************************************/

#define LED_STATE(x)  	x ? \
												HAL_GPIO_WritePin(LED_STATE_GPIO_PORT, LED_STATE_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(LED_STATE_GPIO_PORT, LED_STATE_GPIO_PIN, GPIO_PIN_RESET);  
#define LED_LAN(x)    	x ? \
												HAL_GPIO_WritePin(LED_LAN_GPIO_PORT, LED_LAN_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(LED_LAN_GPIO_PORT, LED_LAN_GPIO_PIN, GPIO_PIN_RESET);  
#define LED_GPRS(x)   	x ? \
												HAL_GPIO_WritePin(LED_GPRS_GPIO_PORT, LED_GPRS_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(LED_GPRS_GPIO_PORT, LED_GPRS_GPIO_PIN, GPIO_PIN_RESET);  

/*
*********************************************************************************************************
*	函 数 名: bsp_InitLed
*	功能说明: 初始化指示灯控制io:默认不开启
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitLed(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);

  /*Configure GPIO pins : PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}


/*
*********************************************************************************************************
*	函 数 名: led_all_on
*	功能说明: LED全亮
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void led_all_on(void)
{
	LED_STATE(0);
	LED_LAN(0);
	LED_GPRS(0);
}

/*
*********************************************************************************************************
*	函 数 名: led_all_off
*	功能说明: LED全灭
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void led_all_off(void)
{
	LED_STATE(1);
	LED_LAN(1);
	LED_GPRS(1);
}

/*
*********************************************************************************************************
*	函 数 名: led_test
*	功能说明: led测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void led_test(void)
{
	while(1)
	{
		led_all_on();
		delay_ms(1000);
		led_all_off();
		delay_ms(1000);	
	}
}
/******************************************  (END OF FILE) **********************************************/


