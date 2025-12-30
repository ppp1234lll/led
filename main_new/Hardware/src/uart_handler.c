/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "bsp.h"

/*
*********************************************************************************************************
*	函 数 名: HAL_UART_RxCpltCallback
*	功能说明: UART接收回调函数
*	形    参: UART_HandleTypeDef 类型指针变量
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == UART8)   /* 如果是串口1 */
	{
		usart8_rxcplt_callback(huart);
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == UART8)   /* 如果是串口1 */
	{
		usart8_rxeventcplt_callback(huart,Size);
	}
}	

/*
*********************************************************************************************************
*	函 数 名: HAL_SPI_TxRxCpltCallback，HAL_SPI_ErrorCallback
*	功能说明: SPI数据传输完成回调和传输错误回调
*	形    参: SPI_HandleTypeDef 类型指针变量
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2) {
		uint32_t err = HAL_UART_GetError(huart);

		if (err & HAL_UART_ERROR_DMA) {
			// 尝试恢复
			HAL_UART_AbortTransmit(huart);  // 终止当前传输
			// 可记录日志、重启DMA、报警等
		}
	}
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
