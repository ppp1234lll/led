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

#define U2_RX_SIZE  (4*32)
#define U2_TX_SIZE  (4*32)

__ALIGNED(32) uint8_t g_U2RxBuffer[U2_RX_SIZE];
__ALIGNED(32) uint8_t g_U2TxBuffer[U2_TX_SIZE];

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
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	HAL_UART_Init(&huart2);			/* HAL_UART_Init()会使能USART2 */

	SET_BIT(USART2->ICR, USART_ICR_TCCF);	/* 清除TC发送完成标志 */
	SET_BIT(USART2->RQR, USART_RQR_RXFRQ);  /* 清除RXNE接收标志 */
	
	HAL_NVIC_EnableIRQ(USART2_IRQn);                    /* 使能USART1中断 */
	HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);            /* 抢占优先级3，子优先级3 */
	/* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
	HAL_UART_Receive_IT(&huart2, (uint8_t *)g_U2RxBuffer, U2_RX_SIZE);

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
  if(__HAL_UART_GET_FLAG(&huart2,UART_FLAG_RXNE)!=RESET) //接收中断
	{
		/* 开启了cache 由于DMA更新了内存 cache不能同步，因此需要无效化从新加载 */
		SCB_InvalidateDCache_by_Addr((uint32_t *)g_U2RxBuffer, U2_RX_SIZE);
		printf("%s\r\n",g_U2RxBuffer);

		/* DMA发送时 cache的内容需要更新到SRAM中 */
		SCB_CleanDCache_by_Addr((uint32_t *)g_U2TxBuffer, U2_TX_SIZE); 
		HAL_UART_Receive_IT(&huart2, (uint8_t *)g_U2RxBuffer, U2_RX_SIZE);

	}
   else if(__HAL_UART_GET_FLAG(&huart2,UART_FLAG_ORE)!=RESET)
	{
	  	__HAL_UART_CLEAR_IT(&huart2,UART_FLAG_ORE);
	}
	else
	{
	   HAL_UART_IRQHandler(&huart2);
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



