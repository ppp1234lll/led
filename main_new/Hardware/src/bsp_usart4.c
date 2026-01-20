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
#include "bsp_usart4.h"
#include "./TASK/inc/single.h"

#define UART4_RX_NE     0    // 使用串口中断
#define UART4_RX_DMA    1    // 使用串口DMA

#define U4_RX_SIZE  (2048)
/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_usart4_rx_sta = 0;

__attribute__((section (".RAM_SRAMD2")))  uint8_t g_U4RxBuffer[U4_RX_SIZE];

UART_HandleTypeDef huart4;          /* UART句柄 */

#if UART4_RX_DMA
	DMA_HandleTypeDef hdma_uart4_rx;
	DMA_HandleTypeDef hdma_uart4_tx;
#endif
/*
*********************************************************************************************************
*	函 数 名: bsp_InitUsart4
*	功能说明: 调试串口初始化
*	形    参: baudrate : 波特率
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitUsart4(uint32_t baudrate)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART4;
	PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* UART4 clock enable */
	__HAL_RCC_UART4_CLK_ENABLE();

	__HAL_RCC_GPIOC_CLK_ENABLE();
#if UART4_RX_DMA
	__HAL_RCC_DMA1_CLK_ENABLE();
#endif
	/**UART4 GPIO Configuration
	PC10     ------> UART4_TX
	PC11     ------> UART4_RX
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

#if UART4_RX_DMA
		/*##-3- 配置DMA ##################################################*/
		/* 配置DMA发送 */
    hdma_uart4_rx.Instance = DMA1_Stream3;
    hdma_uart4_rx.Init.Request = DMA_REQUEST_UART4_RX;
    hdma_uart4_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_uart4_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart4_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart4_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart4_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart4_rx.Init.Mode = DMA_NORMAL;
    hdma_uart4_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_uart4_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart4_rx) != HAL_OK)
    {
      Error_Handler(__FILE__, __LINE__);
    }
    /* 记录DMA句柄hdma_tx到huart的成员hdmatx里 */
    __HAL_LINKDMA(&huart4,hdmarx,hdma_uart4_rx);

#endif

  huart4.Instance = UART4;
  huart4.Init.BaudRate = baudrate;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
  huart4.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
		
#if UART4_RX_NE
	/* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
	HAL_UART_Receive_IT(&huart4, (uint8_t *)g_U4RxBuffer, U4_RX_SIZE);
#endif
	
		
#if UART4_RX_DMA
  // 启动 DMA 接收
	HAL_UARTEx_ReceiveToIdle_DMA(&huart4, g_U4RxBuffer, U4_RX_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_uart4_rx, DMA_IT_HT);  // 禁用半满中断
	__HAL_UART_CLEAR_IDLEFLAG(&huart4); //串口初始化完成后空闲中断标志位是1 需要清除  //很有必要 可以自己仿真看一下初始化后标志位是置一的

	/*##-4- 配置中断 #########################################*/
//	HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
//	HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
	
#endif

	// 3. 关键：初始化后先清除所有串口标志位，避免残留标志触发中断
	__HAL_UART_CLEAR_FLAG(&huart4, UART_FLAG_IDLE);  // 清除空闲标志
	__HAL_UART_CLEAR_FLAG(&huart4, UART_FLAG_RXNE);  // 清除接收非空标志
	__HAL_UART_CLEAR_FLAG(&huart4, UART_FLAG_TC);     // 清除发送完成标志

	/* UART4 interrupt Init */
	HAL_NVIC_SetPriority(UART4_IRQn, 4, 0);
	HAL_NVIC_EnableIRQ(UART4_IRQn);

}

/*
*********************************************************************************************************
*	函 数 名: Usart4_SendString
*	功能说明: 发送字符串
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Usart4_SendString(uint8_t *str)
{
	unsigned int k=0;
  do 
  {
      HAL_UART_Transmit( &huart4,(uint8_t *)(str + k) ,1,1000);
      k++;
  } while(*(str + k)!='\0');
}

/*
*********************************************************************************************************
*	函 数 名: Usart4_Send_Data
*	功能说明: 发送数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Usart4_Send_Data(uint8_t *buf, uint16_t len)
{
//#if UART4_RX_DMA
//		/* DMA发送时 cache的内容需要更新到SRAM中 */
//		SCB_CleanDCache_by_Addr((uint32_t *)buf, len);
//		HAL_UART_Transmit_DMA(&huart4, buf, len);	
//#else
	HAL_UART_Transmit(&huart4,(uint8_t *)buf ,len,1000);
//#endif  
}

/*
*********************************************************************************************************
*	函 数 名: UART4_IRQHandler
*	功能说明: 串口中断函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void UART4_IRQHandler(void)
{ 
#if UART4_RX_NE
	uint8_t rxdata;
	if (UART4->ISR & USART_FLAG_RXNE)   /* 接收到数据 */
	{
		rxdata = UART4->RDR;
    if ((g_usart4_rx_sta & 0x8000) == 0)     /* 接收未完成? */
    {
			if (g_usart4_rx_sta & 0x4000)        /* 接收到了0x0d? */
			{
				if (rxdata != 0x0a)             /* 接收到了0x0a? (必须先接收到到0x0d,才检查0x0a) */
					g_usart4_rx_sta = 0;         /* 接收错误, 重新开始 */
				else
					g_usart4_rx_sta |= 0x8000;   /* 收到了0x0a,标记接收完成了 */
			}
			else      /* 还没收到0x0d */
			{
				if (rxdata == 0x0d)
					g_usart4_rx_sta |= 0x4000;   /* 标记接收到了 0x0d */
				else
				{
					g_U4RxBuffer[g_usart4_rx_sta & 0X3FFF] = rxdata;   /* 存储数据到 g_usart_rx_buf */
					g_usart4_rx_sta++;

					if (g_usart4_rx_sta > (U4_RX_SIZE - 1))
						g_usart4_rx_sta = 0;/* 接收数据溢出, 重新开始接收 */
				}
			}
		}
  }
#endif

#if UART4_RX_DMA
	uint32_t isrflags = UART4->ISR;
	uint32_t cr1its   = UART4->CR1;

	if ((isrflags & USART_ISR_IDLE) != 0 && (cr1its & USART_CR1_IDLEIE) != 0)
	{
		__HAL_UART_CLEAR_IDLEFLAG(&huart4);
		
		HAL_UART_DMAStopRx(&huart4);
		uint32_t total_len = U4_RX_SIZE - __HAL_DMA_GET_COUNTER(huart4.hdmarx);
		
		/* 开启了cache 由于DMA更新了内存 cache不能同步，因此需要无效化从新加载 */
		SCB_InvalidateDCache_by_Addr((uint32_t *)g_U4RxBuffer, U4_RX_SIZE);		
//		Usart4_SendString("\r\n uart4 dma_recv:\r\n");
//		HAL_UART_Transmit(&huart4, (uint8_t *)g_U4RxBuffer, total_len, 1000);   /* 发送接收到的数据 */
//		Usart4_Send_Data("123456789000\n",12);
		
		single_recv_board_data(BOARD_2,g_U4RxBuffer,total_len);
		HAL_UARTEx_ReceiveToIdle_DMA(&huart4, g_U4RxBuffer, U4_RX_SIZE);
	}
#endif

	/* 清除中断标志 */
	SET_BIT(UART4->ICR, UART_CLEAR_PEF);
	SET_BIT(UART4->ICR, UART_CLEAR_FEF);
	SET_BIT(UART4->ICR, UART_CLEAR_NEF);
	SET_BIT(UART4->ICR, UART_CLEAR_OREF);
	SET_BIT(UART4->ICR, UART_CLEAR_IDLEF);
	SET_BIT(UART4->ICR, UART_CLEAR_TCF);
	SET_BIT(UART4->ICR, UART_CLEAR_LBDF);
	SET_BIT(UART4->ICR, UART_CLEAR_CTSF);
	SET_BIT(UART4->ICR, UART_CLEAR_CMF);
	SET_BIT(UART4->ICR, UART_CLEAR_WUF);
	SET_BIT(UART4->ICR, UART_CLEAR_TXFECF);
}

#if UART4_RX_DMA
/*
*********************************************************************************************************
*	函 数 名: USARTx_DMA_RX_IRQHandler
*	功能说明: 串口中断的接收回调函数。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void DMA1_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_uart4_rx);
}

#endif
/*
*********************************************************************************************************
*	函 数 名: uart4_test
*	功能说明: 串口测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void uart4_test(void)
{
	uint8_t len;
	uint16_t times = 0;
	while(1)
	{
        if (g_usart4_rx_sta & 0x8000)                                                    /* 接收到了数据? */
        {
            len = g_usart4_rx_sta & 0x3fff;                                              /* 得到此次接收到的数据长度 */
            Usart4_SendString("\r\n您发送的消息为:\r\n");

            HAL_UART_Transmit(&huart4, (uint8_t *)g_U4RxBuffer, len, 1000);   /* 发送接收到的数据 */
            while(__HAL_UART_GET_FLAG(&huart4, UART_FLAG_TC) != SET);           /* 等待发送结束 */
            Usart4_SendString("\r\n\r\n");                                                         /* 插入换行 */
            g_usart4_rx_sta = 0;
        }
        else
        {
            times++;

            if (times % 5000 == 0)
            {
                Usart4_SendString("\r\n正点原子 STM32开发板 串口实验\r\n");
                Usart4_SendString("正点原子@ALIENTEK\r\n\r\n\r\n");
            }

            if (times % 200 == 0)
            {
                Usart4_SendString("请输入数据,以回车键结束\r\n");
            }

            if (times % 30  == 0) 
            {
//                Usart4_SendString("times\r\n");
            }

            delay_ms(10);
        }
    }	
}

 



