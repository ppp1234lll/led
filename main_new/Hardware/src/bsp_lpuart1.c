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
#include "bsp_lpuart1.h"
#include "./TASK/inc/single.h"

#define LPUART1_RX_NE     0    // 使用串口中断
#define LPUART1_RX_DMA    1    // 使用串口DMA

#define LU1_RX_SIZE  (2048)
/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_lpuart1_rx_sta = 0;

__attribute__((section (".RAM_SRAM4"))) uint8_t g_LU1RxBuffer[LU1_RX_SIZE];

UART_HandleTypeDef hlpuart1;        /* UART句柄 */

#if LPUART1_RX_DMA
DMA_HandleTypeDef hdma_lpuart1_rx;

#endif
/*
*********************************************************************************************************
*	函 数 名: bsp_InitLpuart1
*	功能说明: 调试串口初始化
*	形    参: baudrate : 波特率
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitLpuart1(uint32_t baudrate)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
	PeriphClkInitStruct.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_D3PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* LPUART1 clock enable */
	__HAL_RCC_LPUART1_CLK_ENABLE();

	__HAL_RCC_GPIOB_CLK_ENABLE();
#if LPUART1_RX_DMA
	__HAL_RCC_BDMA_CLK_ENABLE();
#endif
	/**LPUART1 GPIO Configuration
	PB6     ------> LPUART1_TX
	PB7     ------> LPUART1_RX
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF8_LPUART;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

#if LPUART1_RX_DMA
	/*##-3- 配置DMA ##################################################*/
	/* 配置DMA发送 */
	hdma_lpuart1_rx.Instance = BDMA_Channel0;
	hdma_lpuart1_rx.Init.Request = BDMA_REQUEST_LPUART1_RX;
	hdma_lpuart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_lpuart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_lpuart1_rx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_lpuart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_lpuart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_lpuart1_rx.Init.Mode = DMA_NORMAL;
	hdma_lpuart1_rx.Init.Priority = DMA_PRIORITY_HIGH;
	if (HAL_DMA_Init(&hdma_lpuart1_rx) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
	/* 记录DMA句柄hdma_tx到huart的成员hdmatx里 */
	__HAL_LINKDMA(&hlpuart1,hdmarx,hdma_lpuart1_rx);

#endif

  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = baudrate;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
  hlpuart1.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
	
#if LPUART1_RX_NE
	/* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
	HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)g_LU1RxBuffer, LU1_RX_SIZE);
#endif
	
#if LPUART1_RX_DMA
  // 启动 DMA 接收
	HAL_UARTEx_ReceiveToIdle_DMA(&hlpuart1, g_LU1RxBuffer, LU1_RX_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_lpuart1_rx, DMA_IT_HT);  // 禁用半满中断
	__HAL_UART_CLEAR_IDLEFLAG(&hlpuart1); //串口初始化完成后空闲中断标志位是1 需要清除  //很有必要 可以自己仿真看一下初始化后标志位是置一的

	/*##-4- 配置中断 #########################################*/
//	HAL_NVIC_SetPriority(BDMA_Channel0_IRQn, 0, 0);
//	HAL_NVIC_EnableIRQ(BDMA_Channel0_IRQn);
	
#endif

	// 3. 关键：初始化后先清除所有串口标志位，避免残留标志触发中断
	__HAL_UART_CLEAR_FLAG(&hlpuart1, UART_FLAG_IDLE);  // 清除空闲标志
	__HAL_UART_CLEAR_FLAG(&hlpuart1, UART_FLAG_RXNE);  // 清除接收非空标志
	__HAL_UART_CLEAR_FLAG(&hlpuart1, UART_FLAG_TC);     // 清除发送完成标志

	/* LPUART1 interrupt Init */
	HAL_NVIC_SetPriority(LPUART1_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(LPUART1_IRQn);

}

/*
*********************************************************************************************************
*	函 数 名: Lpuart1_SendString
*	功能说明: 发送字符串
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Lpuart1_SendString(uint8_t *str)
{
	unsigned int k=0;
  do 
  {
      HAL_UART_Transmit( &hlpuart1,(uint8_t *)(str + k) ,1,1000);
      k++;
  } while(*(str + k)!='\0');
}

/*
*********************************************************************************************************
*	函 数 名: Lpuart1_Send_Data
*	功能说明: 发送数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Lpuart1_Send_Data(uint8_t *buf, uint16_t len)
{
//#if LPUART1_RX_DMA
//		/* DMA发送时 cache的内容需要更新到SRAM中 */
//		SCB_CleanDCache_by_Addr((uint32_t *)buf, len);
//		HAL_UART_Transmit_DMA(&hlpuart1, buf, len);	
//#else
	HAL_UART_Transmit(&hlpuart1,(uint8_t *)buf ,len,1000);
//#endif  
}

/*
*********************************************************************************************************
*	函 数 名: LPUART1_IRQHandler
*	功能说明: 串口中断函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LPUART1_IRQHandler(void)
{ 
#if LPUART1_RX_NE
	uint8_t rxdata;
	if (LPUART1->ISR & (1 << 5))   /* 接收到数据 */
	{
		rxdata = LPUART1->RDR;
    if ((g_lpuart1_rx_sta & 0x8000) == 0)     /* 接收未完成? */
    {
			if (g_lpuart1_rx_sta & 0x4000)        /* 接收到了0x0d? */
			{
				if (rxdata != 0x0a)             /* 接收到了0x0a? (必须先接收到到0x0d,才检查0x0a) */
					g_lpuart1_rx_sta = 0;         /* 接收错误, 重新开始 */
				else
					g_lpuart1_rx_sta |= 0x8000;   /* 收到了0x0a,标记接收完成了 */
			}
			else      /* 还没收到0x0d */
			{
				if (rxdata == 0x0d)
					g_lpuart1_rx_sta |= 0x4000;   /* 标记接收到了 0x0d */
				else
				{
					g_LU1RxBuffer[g_lpuart1_rx_sta & 0X3FFF] = rxdata;   /* 存储数据到 g_usart_rx_buf */
					g_lpuart1_rx_sta++;

					if (g_lpuart1_rx_sta > (LU1_RX_SIZE - 1))
						g_lpuart1_rx_sta = 0;/* 接收数据溢出, 重新开始接收 */
				}
			}
		}
  }
#endif

#if LPUART1_RX_DMA
	uint32_t isrflags = LPUART1->ISR;
	uint32_t cr1its   = LPUART1->CR1;

	if ((isrflags & USART_ISR_IDLE) != 0 && (cr1its & USART_CR1_IDLEIE) != 0)
	{
		__HAL_UART_CLEAR_IDLEFLAG(&hlpuart1);
		
		HAL_UART_DMAStopRx(&hlpuart1);
		uint32_t total_len = LU1_RX_SIZE - __HAL_DMA_GET_COUNTER(hlpuart1.hdmarx);
		
		/* 开启了cache 由于DMA更新了内存 cache不能同步，因此需要无效化从新加载 */
		SCB_InvalidateDCache_by_Addr((uint32_t *)g_LU1RxBuffer, LU1_RX_SIZE);		
//		Lpuart1_SendString("\r\n hlpuart1 dma_recv:\r\n");
//		HAL_UART_Transmit(&hlpuart1, (uint8_t *)g_LU1RxBuffer, total_len, 1000);   /* 发送接收到的数据 */

    single_recv_board_data(BOARD_3,g_LU1RxBuffer,total_len);
		HAL_UARTEx_ReceiveToIdle_DMA(&hlpuart1, g_LU1RxBuffer, LU1_RX_SIZE);
	}
#endif

	/* 清除中断标志 */
	SET_BIT(LPUART1->ICR, UART_CLEAR_PEF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_FEF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_NEF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_OREF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_IDLEF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_TCF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_LBDF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_CTSF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_CMF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_WUF);
	SET_BIT(LPUART1->ICR, UART_CLEAR_TXFECF);
}

#if LPUART1_RX_DMA
/*
*********************************************************************************************************
*	函 数 名: BDMA_Channel0_IRQHandler
*	功能说明: 串口中断的接收回调函数。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void BDMA_Channel0_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_lpuart1_rx);
}

#endif

/*
*********************************************************************************************************
*	函 数 名: lpuart1_test
*	功能说明: 串口测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void lpuart1_test(void)
{
	uint8_t len;
	uint16_t times = 0;
	uint8_t testdata[10]={"123456789"};
	while(1)
	{
        if (g_lpuart1_rx_sta & 0x8000)                                                    /* 接收到了数据? */
        {
            len = g_lpuart1_rx_sta & 0x3fff;                                              /* 得到此次接收到的数据长度 */
            Lpuart1_SendString("\r\n您发送的消息为:\r\n");

            HAL_UART_Transmit(&hlpuart1, (uint8_t *)g_LU1RxBuffer, len, 1000);   /* 发送接收到的数据 */
            while(__HAL_UART_GET_FLAG(&hlpuart1, UART_FLAG_TC) != SET);           /* 等待发送结束 */
            Lpuart1_SendString("\r\n\r\n");                                                         /* 插入换行 */
            g_lpuart1_rx_sta = 0;
        }
        else
        {
            times++;

            if (times % 5000 == 0)
            {
                Lpuart1_SendString("\r\n正点原子 STM32开发板 串口实验\r\n");
                Lpuart1_SendString("正点原子@ALIENTEK\r\n\r\n\r\n");
            }

            if (times % 200 == 0)
            {
                Lpuart1_SendString("请输入数据,以回车键结束\r\n");
							Lpuart1_Send_Data(testdata,10);
            }

            if (times % 30  == 0) 
            {
//                Lpuart1_SendString("times\r\n");
            }

            delay_ms(10);
        }
    }	
}

