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
#include "main.h"

UART_HandleTypeDef huart2;
uint8_t g_U2RxBuffer[1];
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

  /** Initializes the peripherals clock
  */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART2;
	PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* USART2 clock enable */
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
 
	/**USART2 GPIO Configuration
	PD5     ------> USART2_TX
	PD6     ------> USART2_RX
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* USART 初始化设置 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = bound;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
  huart2.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
	HAL_UART_Init(&huart2);                             /* HAL_UART_Init()会使能USART2 */

 
	HAL_NVIC_EnableIRQ(USART2_IRQn);                          /* 使能USART1中断 */
	HAL_NVIC_SetPriority(USART2_IRQn, 3, 3);                  /* 抢占优先级3，子优先级3 */
	
	HAL_UART_Receive_IT(&huart2, (uint8_t *)g_U2RxBuffer, 1);
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


//重定向c库函数printf到串口USARTx，重定向后可使用printf函数
int fputc(int ch, FILE *f)
{
    /* 发送一个字节数据到串口USARTx */
	HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
	return (ch);
}

///重定向c库函数scanf到串口USARTx，重写向后可使用scanf、getchar等函数
int fgetc(FILE *f)
{	
	int ch;
	/* 等待串口输入数据 */
	while(__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET);
	__HAL_UART_CLEAR_OREFLAG(&huart2);
	HAL_UART_Receive(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
	return (ch);
}

