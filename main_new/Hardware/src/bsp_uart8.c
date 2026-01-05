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
#include "bsp_uart8.h"
#include "./DRIVER/inc/GPRS.h"

/*
*********************************************************************************************************
*	                             选择DMA，中断或者查询方式
*********************************************************************************************************
*/
#define UART8_RX_NE     0    // 使用串口中断
#define UART8_RX_DMA    1    // 使用串口DMA

/*
*********************************************************************************************************
*	                               时钟，引脚，DMA，中断等宏定义
*********************************************************************************************************
*/

#define U8_RX_SIZE     2048  
#define RXBUFFERSIZE   1    /* 缓存大小 */

/*
*********************************************************************************************************
*	                                           变量
*********************************************************************************************************
*/
UART_HandleTypeDef huart8;          /* UART句柄 */
DMA_HandleTypeDef hdma_uart8_rx;
DMA_HandleTypeDef hdma_uart8_tx;


/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_uart8_rx_sta = 0;

#if UART8_RX_DMA
__attribute__((section (".RAM_SRAMD2"))) uint8_t g_U8RxBuffer[U8_RX_SIZE];
#else
uint8_t g_U8RxBuffer[U8_RX_SIZE];
#endif

uint8_t g_uart8_rx_data[RXBUFFERSIZE];  /* HAL库使用的串口接收缓冲 */

UART_TxCpltCallbackFunc   usart8_txcplt_callback = NULL;  
UART_RxCpltCallbackFunc   usart8_rxcplt_callback = NULL; 
UART_RxEventCallbackFunc  usart8_rxeventcplt_callback = NULL; 

/*
*********************************************************************************************************
*	                                           函数声明
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*	函 数 名: bsp_InitUart8
*	功能说明: 调试串口初始化
*	形    参: baudrate : 波特率
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitUart8(uint32_t baudrate)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART8;
	PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* UART8 clock enable */
	__HAL_RCC_UART8_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();

#if UART8_RX_DMA
	__HAL_RCC_DMA2_CLK_ENABLE();
#endif
	/**UART8 GPIO Configuration
	PE0     ------> UART8_RX
	PE1     ------> UART8_TX
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF8_UART8;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

#if UART8_RX_DMA
	/*##-3- 配置DMA ##################################################*/
	/* 配置DMA发送 */
	hdma_uart8_rx.Instance = DMA2_Stream0;
	hdma_uart8_rx.Init.Request = DMA_REQUEST_UART8_RX;
	hdma_uart8_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_uart8_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_uart8_rx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_uart8_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_uart8_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_uart8_rx.Init.Mode = DMA_NORMAL;
	hdma_uart8_rx.Init.Priority = DMA_PRIORITY_LOW;
	hdma_uart8_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	if (HAL_DMA_Init(&hdma_uart8_rx) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
	/* 记录DMA句柄hdma_tx到huart的成员hdmatx里 */
	__HAL_LINKDMA(&huart8,hdmarx,hdma_uart8_rx);
	
#endif

  huart8.Instance = UART8;
  huart8.Init.BaudRate = baudrate;
  huart8.Init.WordLength = UART_WORDLENGTH_8B;
  huart8.Init.StopBits = UART_STOPBITS_1;
  huart8.Init.Parity = UART_PARITY_NONE;
  huart8.Init.Mode = UART_MODE_TX_RX;
  huart8.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart8.Init.OverSampling = UART_OVERSAMPLING_16;
  huart8.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart8.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart8.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
  huart8.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  if (HAL_UART_Init(&huart8) != HAL_OK)
	{
    Error_Handler(__FILE__, __LINE__);
  }
	
#if UART8_RX_NE
	/* UART8 interrupt Init */
	HAL_NVIC_SetPriority(UART8_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(UART8_IRQn);
	
	/* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
	HAL_UART_Receive_IT(&huart8, (uint8_t *)g_uart8_rx_data, RXBUFFERSIZE);
#endif
	
#if UART8_RX_DMA

	__HAL_DMA_DISABLE_IT(&hdma_uart8_rx, DMA_IT_HT);  // 禁用半满中断
	__HAL_UART_CLEAR_IDLEFLAG(&huart8); //串口初始化完成后空闲中断标志位是1 需要清除  //很有必要 可以自己仿真看一下初始化后标志位是置一的

	/*##-4- 配置中断 #########################################*/
	/* UART8 interrupt Init */
	HAL_NVIC_SetPriority(UART8_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(UART8_IRQn);
	
  // 启动 DMA 接收
	HAL_UARTEx_ReceiveToIdle_DMA(&huart8, g_U8RxBuffer, U8_RX_SIZE);
#endif

}

/*
*********************************************************************************************************
*	函 数 名: Uart8_SendString
*	功能说明: 发送字符串
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Uart8_SendString(uint8_t *str)
{
	unsigned int k=0;
  do 
  {
		HAL_UART_Transmit( &huart8,(uint8_t *)(str + k) ,1,1000);
		k++;
  } while(*(str + k)!='\0');
}

/*
*********************************************************************************************************
*	函 数 名: Uart8_Send_Data
*	功能说明: 发送数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Uart8_Send_Data(uint8_t *buf, uint16_t len)
{
	HAL_UART_Transmit(&huart8,(uint8_t *)buf ,len,1000);
}

/*
*********************************************************************************************************
*	函 数 名: UART8_IRQHandler
*	功能说明: 串口中断函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void UART8_IRQHandler(void)
{ 
#if UART8_RX_NE
	uint8_t rxdata;
	if (UART8->ISR & USART_FLAG_RXNE)   /* 接收到数据 */
	{
		rxdata = UART8->RDR;
    if ((g_uart8_rx_sta & 0x8000) == 0)     /* 接收未完成? */
    {
			if (g_uart8_rx_sta & 0x4000)        /* 接收到了0x0d? */
			{
				if (rxdata != 0x0a)             /* 接收到了0x0a? (必须先接收到到0x0d,才检查0x0a) */
					g_uart8_rx_sta = 0;         /* 接收错误, 重新开始 */
				else
					g_uart8_rx_sta |= 0x8000;   /* 收到了0x0a,标记接收完成了 */
			}
			else      /* 还没收到0x0d */
			{
				if (rxdata == 0x0d)
					g_uart8_rx_sta |= 0x4000;   /* 标记接收到了 0x0d */
				else
				{
					g_U8RxBuffer[g_uart8_rx_sta & 0X3FFF] = rxdata;   /* 存储数据到 g_usart_rx_buf */
					g_uart8_rx_sta++;

					if (g_uart8_rx_sta > (U8_RX_SIZE - 1))
						g_uart8_rx_sta = 0;/* 接收数据溢出, 重新开始接收 */
				}
			}
		}
  }
#endif

#if UART8_RX_DMA
	uint32_t isrflags = UART8->ISR;
	uint32_t cr1its   = UART8->CR1;

	if ((isrflags & USART_ISR_IDLE) != 0 && (cr1its & USART_CR1_IDLEIE) != 0)
	{
		__HAL_UART_CLEAR_IDLEFLAG(&huart8);
		
		HAL_UART_DMAStopRx(&huart8);
		uint32_t total_len = U8_RX_SIZE - __HAL_DMA_GET_COUNTER(huart8.hdmarx);
		
		/* 开启了cache 由于DMA更新了内存 cache不能同步，因此需要无效化从新加载 */
		SCB_InvalidateDCache_by_Addr((uint32_t *)g_U8RxBuffer, U8_RX_SIZE);		

		gprs_get_receive_data_function(g_U8RxBuffer,total_len);
		HAL_UARTEx_ReceiveToIdle_DMA(&huart8, g_U8RxBuffer, U8_RX_SIZE);
	}
#endif

	/* 清除中断标志 */
	SET_BIT(UART8->ICR, UART_CLEAR_PEF);
	SET_BIT(UART8->ICR, UART_CLEAR_FEF);
	SET_BIT(UART8->ICR, UART_CLEAR_NEF);
	SET_BIT(UART8->ICR, UART_CLEAR_OREF);
	SET_BIT(UART8->ICR, UART_CLEAR_IDLEF);
	SET_BIT(UART8->ICR, UART_CLEAR_TCF);
	SET_BIT(UART8->ICR, UART_CLEAR_LBDF);
	SET_BIT(UART8->ICR, UART_CLEAR_CTSF);
	SET_BIT(UART8->ICR, UART_CLEAR_CMF);
	SET_BIT(UART8->ICR, UART_CLEAR_WUF);
	SET_BIT(UART8->ICR, UART_CLEAR_TXFECF);
}

/*
*********************************************************************************************************
*	函 数 名: uart8_test
*	功能说明: 串口测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void uart8_test(void)
{
	uint8_t len;
	uint16_t times = 0;
	while(1)
	{
		if (g_uart8_rx_sta & 0x8000)					/* 接收到了数据? */
		{
			len = g_uart8_rx_sta & 0x3fff;			/* 得到此次接收到的数据长度 */
			printf("\r\n您发送的消息为:\r\n");
			
			HAL_UART_Transmit(&huart8, (uint8_t *)g_U8RxBuffer, len, 1000);   /* 发送接收到的数据 */
			while(__HAL_UART_GET_FLAG(&huart8, UART_FLAG_TC) != SET);           /* 等待发送结束 */
			printf("\r\n\r\n");                                                         /* 插入换行 */
			g_uart8_rx_sta = 0;
		}
		else
		{
			times++;

			if (times % 5000 == 0)
			{
					printf("\r\n正点原子 STM32开发板 串口实验\r\n");
					printf("正点原子@ALIENTEK\r\n\r\n\r\n");
			}

			if (times % 200 == 0)
			{
					printf("请输入数据,以回车键结束\r\n");
					Uart8_Send_Data("123456789000\n",12);
			}

			if (times % 30  == 0) 
			{
//                Uart8_SendString("times\r\n");
			}

			delay_ms(10);
		}
	}	
}

 



