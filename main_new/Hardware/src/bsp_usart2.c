/**
 ****************************************************************************************************
 * @file        usart.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2023-03-02
 * @brief       串口初始化代码(一般是串口1)，支持printf
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#include "bsp.h"
#include "bsp_usart2.h"

#define UART2_RX_NE     1    // 使用串口中断
#define UART2_RX_DMA    0    // 使用串口DMA

#define U2_RX_SIZE  (2048)
/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_usart2_rx_sta = 0;

__attribute__((section (".RAM_SRAMD2"))) uint8_t g_U2RxBuffer[U2_RX_SIZE];

UART_HandleTypeDef huart2;        /* UART句柄 */

#if UART2_RX_DMA
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;
#endif
/*
*********************************************************************************************************
*	函 数 名: bsp_InitUsart2
*	功能说明: 调试串口初始化
*	形    参: baudrate : 波特率
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitUsart2(uint32_t baudrate)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
	PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* USART2 clock enable */
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
#if UART2_RX_DMA
	__HAL_RCC_DMA1_CLK_ENABLE();
#endif
	/**USART2 GPIO Configuration
	PD8     ------> USART3_TX
	PD9     ------> USART3_RX
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

#if UART2_RX_DMA
	/*##-3- 配置DMA ##################################################*/
	/* 配置DMA发送 */
	hdma_usart2_rx.Instance = DMA1_Stream4;
	hdma_usart2_rx.Init.Request = DMA_REQUEST_USART3_RX;
	hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_usart2_rx.Init.Mode = DMA_NORMAL;
	hdma_usart2_rx.Init.Priority = DMA_PRIORITY_HIGH;
	hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
	/* 记录DMA句柄hdma_tx到huart的成员hdmatx里 */
	__HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

	/* USART3_TX Init */
	hdma_usart2_tx.Instance = DMA1_Stream5;
	hdma_usart2_tx.Init.Request = DMA_REQUEST_USART3_TX;
	hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_usart2_tx.Init.Mode = DMA_NORMAL;
	hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
	hdma_usart2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
	__HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);
#endif

  huart2.Instance = USART2;
  huart2.Init.BaudRate = baudrate;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
	
#if UART2_RX_NE
	/* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
	HAL_UART_Receive_IT(&huart2, (uint8_t *)g_U2RxBuffer, U2_RX_SIZE);
#endif
	
#if UART2_RX_DMA
  // 启动 DMA 接收
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_U2RxBuffer, U2_RX_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);  // 禁用半满中断
	__HAL_UART_CLEAR_IDLEFLAG(&huart2); //串口初始化完成后空闲中断标志位是1 需要清除  //很有必要 可以自己仿真看一下初始化后标志位是置一的

	/*##-4- 配置中断 #########################################*/
	HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
	
#endif

//	// 3. 关键：初始化后先清除所有串口标志位，避免残留标志触发中断
//	__HAL_UART_CLEAR_FLAG(&huart2, UART_FLAG_IDLE);  // 清除空闲标志
//	__HAL_UART_CLEAR_FLAG(&huart2, UART_FLAG_RXNE);  // 清除接收非空标志
//	__HAL_UART_CLEAR_FLAG(&huart2, UART_FLAG_TC);     // 清除发送完成标志

	/* USART2 interrupt Init */
	HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(USART2_IRQn);

}

/*
*********************************************************************************************************
*	函 数 名: Usart2_SendString
*	功能说明: 发送字符串
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Usart2_SendString(uint8_t *str)
{
	unsigned int k=0;
  do 
  {
      HAL_UART_Transmit( &huart2,(uint8_t *)(str + k) ,1,1000);
      k++;
  } while(*(str + k)!='\0');
}

/*
*********************************************************************************************************
*	函 数 名: Usart2_Send_Data
*	功能说明: 发送数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Usart2_Send_Data(uint8_t *buf, uint16_t len)
{
//#if UART2_RX_DMA
//		/* DMA发送时 cache的内容需要更新到SRAM中 */
//		SCB_CleanDCache_by_Addr((uint32_t *)buf, len);
//		HAL_UART_Transmit_DMA(&huart2, buf, len);	
//#else
	HAL_UART_Transmit(&huart2,(uint8_t *)buf ,len,1000);
//#endif  
}

/*
*********************************************************************************************************
*	函 数 名: USART2_IRQHandler
*	功能说明: 串口中断函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void USART2_IRQHandler(void)
{ 
#if UART2_RX_NE
	uint8_t rxdata;
	if (USART2->ISR & (1 << 5))   /* 接收到数据 */
	{
		rxdata = USART2->RDR;
    if ((g_usart2_rx_sta & 0x8000) == 0)     /* 接收未完成? */
    {
			if (g_usart2_rx_sta & 0x4000)        /* 接收到了0x0d? */
			{
				if (rxdata != 0x0a)             /* 接收到了0x0a? (必须先接收到到0x0d,才检查0x0a) */
					g_usart2_rx_sta = 0;         /* 接收错误, 重新开始 */
				else
					g_usart2_rx_sta |= 0x8000;   /* 收到了0x0a,标记接收完成了 */
			}
			else      /* 还没收到0x0d */
			{
				if (rxdata == 0x0d)
					g_usart2_rx_sta |= 0x4000;   /* 标记接收到了 0x0d */
				else
				{
					g_U2RxBuffer[g_usart2_rx_sta & 0X3FFF] = rxdata;   /* 存储数据到 g_usart_rx_buf */
					g_usart2_rx_sta++;

					if (g_usart2_rx_sta > (U2_RX_SIZE - 1))
						g_usart2_rx_sta = 0;/* 接收数据溢出, 重新开始接收 */
				}
			}
		}
  }
#endif

#if UART2_RX_DMA
	uint32_t isrflags = USART2->ISR;
	uint32_t cr1its   = USART2->CR1;

	if ((isrflags & USART_ISR_IDLE) != 0 && (cr1its & USART_CR1_IDLEIE) != 0)
	{
		__HAL_UART_CLEAR_IDLEFLAG(&huart2);
		
		HAL_UART_DMAStopRx(&huart2);
		uint32_t total_len = U2_RX_SIZE - __HAL_DMA_GET_COUNTER(huart2.hdmarx);
		
		/* 开启了cache 由于DMA更新了内存 cache不能同步，因此需要无效化从新加载 */
		SCB_InvalidateDCache_by_Addr((uint32_t *)g_U2RxBuffer, U2_RX_SIZE);		
		Usart2_SendString("\r\ndma_recv:\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t *)g_U2RxBuffer, total_len, 1000);   /* 发送接收到的数据 */

//		Usart1_Send_Data("123456789000\n",12);
		
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, g_U2RxBuffer, U2_RX_SIZE);
	}
#endif

	/* 清除中断标志 */
	SET_BIT(USART2->ICR, UART_CLEAR_PEF);
	SET_BIT(USART2->ICR, UART_CLEAR_FEF);
	SET_BIT(USART2->ICR, UART_CLEAR_NEF);
	SET_BIT(USART2->ICR, UART_CLEAR_OREF);
	SET_BIT(USART2->ICR, UART_CLEAR_IDLEF);
	SET_BIT(USART2->ICR, UART_CLEAR_TCF);
	SET_BIT(USART2->ICR, UART_CLEAR_LBDF);
	SET_BIT(USART2->ICR, UART_CLEAR_CTSF);
	SET_BIT(USART2->ICR, UART_CLEAR_CMF);
	SET_BIT(USART2->ICR, UART_CLEAR_WUF);
	SET_BIT(USART2->ICR, UART_CLEAR_TXFECF);
}

#if UART2_RX_DMA
/*
*********************************************************************************************************
*	函 数 名: DMA1_Stream2_IRQn
*	功能说明: 串口中断的接收回调函数。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void DMA1_Stream2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart2_rx);
}

/*
*********************************************************************************************************
*	函 数 名: DMA1_Stream3_IRQn
*	功能说明: 串口发送DMA中断服务程序。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void DMA1_Stream3_IRQHandler(void)
{
	// 1. 检测DMA传输完成标志（TCIF：Transfer Complete Interrupt Flag）
	if (__HAL_DMA_GET_FLAG(&hdma_usart2_tx, DMA_FLAG_TCIF3_7) != RESET) {
		// 清除传输完成标志（必须：否则会反复触发中断）
		__HAL_DMA_CLEAR_FLAG(&hdma_usart2_tx, DMA_FLAG_TCIF3_7);
	}

	// 2. 检测DMA传输错误标志（可选：处理发送异常）
	if (__HAL_DMA_GET_FLAG(&hdma_usart2_tx, DMA_FLAG_TEIF3_7) != RESET) {
		// 清除错误标志
		__HAL_DMA_CLEAR_FLAG(&hdma_usart2_tx, DMA_FLAG_TEIF3_7);

		// 错误处理：停止DMA，重置状态
		HAL_DMA_Abort(&hdma_usart2_tx);
	}
}
#endif

/*
*********************************************************************************************************
*	函 数 名: usart2_test
*	功能说明: 串口测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void usart2_test(void)
{
    uint8_t len;
    uint16_t times = 0;
	while(1)
	{
        if (g_usart2_rx_sta & 0x8000)                                                    /* 接收到了数据? */
        {
            len = g_usart2_rx_sta & 0x3fff;                                              /* 得到此次接收到的数据长度 */
            Usart2_SendString("\r\n您发送的消息为:\r\n");

            HAL_UART_Transmit(&huart2, (uint8_t *)g_U2RxBuffer, len, 1000);   /* 发送接收到的数据 */
            while(__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) != SET);           /* 等待发送结束 */
            Usart2_SendString("\r\n\r\n");                                                         /* 插入换行 */
            g_usart2_rx_sta = 0;
        }
        else
        {
            times++;

            if (times % 5000 == 0)
            {
                Usart2_SendString("\r\n正点原子 STM32开发板 串口实验\r\n");
                Usart2_SendString("正点原子@ALIENTEK\r\n\r\n\r\n");
            }

            if (times % 200 == 0)
            {
                Usart2_SendString("请输入数据,以回车键结束\r\n");
            }

            if (times % 30  == 0) 
            {
//                Usart1_SendString("times\r\n");
            }

            delay_ms(10);
        }
    }	
}


 

