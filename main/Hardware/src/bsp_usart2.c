/**
 ****************************************************************************************************
 * @file        rs485.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-06
 * @brief       RS485 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#include "bsp_usart2.h"
#include "bsp.h"

UART_HandleTypeDef huart2;

/*
*********************************************************************************************************
*	函 数 名: bsp_InitUsart2
*	功能说明: 调试串口初始化
*	形    参: baudrate : 波特率
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitUsart2(uint32_t bound)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};	
	
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART2;
	PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
     Error_Handler(__FILE__, __LINE__);
	}	
	
	/* USART2 clock enable */
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	/**USART2 GPIO Configuration
	PA2     ------> USART2_TX
	PA3     ------> USART2_RX
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3; 
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;        /* 复用推挽 */
	GPIO_InitStruct.Pull = GPIO_PULLUP;            /* 上拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;       /* 高速 */
	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;   /* 复用为USART2 */
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);        /* 初始化PA2,3 */


	/* USART 初始化设置 */
	huart2.Instance = USART2;                         /* USART2 */
	huart2.Init.BaudRate = bound;                       /* 波特率 */
	huart2.Init.WordLength = UART_WORDLENGTH_8B;        /* 字长为8位数据格式 */
	huart2.Init.StopBits = UART_STOPBITS_1;             /* 一个停止位 */
	huart2.Init.Parity = UART_PARITY_NONE;              /* 无奇偶校验位 */
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;        /* 无硬件流控 */
	huart2.Init.Mode = UART_MODE_TX_RX;                 /* 收发模式 */
	HAL_UART_Init(&huart2);                             /* HAL_UART_Init()会使能USART2 */

	__HAL_UART_DISABLE_IT(&huart2, UART_IT_TC);

	__HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);        /* 开启接收中断 */
	HAL_NVIC_EnableIRQ(USART2_IRQn);                          /* 使能USART1中断 */
	HAL_NVIC_SetPriority(USART2_IRQn, 3, 3);                  /* 抢占优先级3，子优先级3 */

	
}

/**
 * @brief       RS485发送len个字节
 * @param       buf:发送区首地址
 * @param       len:发送的字节数(为了和本代码的接收匹配,这里建议不要超过64个字节)
 * @retval      无
 */
void usart2_send_data(uint8_t *buf, uint8_t len)
{                                       	
  HAL_UART_Transmit(&huart2, buf, len, 1000);         /* 串口2发送数据 */	
}

/*
*********************************************************************************************************
*	函 数 名: USART_DEBUG_IRQHandler
*	功能说明: 串口中断函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void USART2_IRQHandler(void)
{
	if ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) != RESET))  /* 接收中断 */
	{
	} 
}

/*
*********************************************************************************************************
*	函 数 名: usart_debug_test
*	功能说明: 串口测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void usart2_test(void)
{
	while(1)
	{
		usart2_send_data((uint8_t*)"12345678ABD\n",11);
		delay_ms(1000);		
	}
}



