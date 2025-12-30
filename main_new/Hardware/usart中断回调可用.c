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

#include "./SYSTEM/sys/sys.h"
#include "bsp.h"

/*
*********************************************************************************************************
*	                             选择DMA，中断或者查询方式
*********************************************************************************************************
*/
#define UART8_RX_NE     0    // 使用串口中断
#define UART8_RX_DMA    1    // 使用串口DMA

/*
*********************************************************************************************************
*	                            时钟，引脚，DMA，中断等宏定义
*********************************************************************************************************
*/

#define U8_RX_SIZE     2048
#define U8_TX_SIZE     2048
#define RXBUFFERSIZE   1    /* 缓存大小 */

enum {
	UART8_TRANSFER_NO,
	UART8_TRANSFER_WAIT,
	UART8_TRANSFER_COMPLETE,
	UART8_TRANSFER_ERROR
};

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
__attribute__((section (".RAM_SRAMD2"))) uint8_t g_U8TxBuffer[U8_TX_SIZE];
#else
uint8_t g_U8RxBuffer[U8_RX_SIZE];
uint8_t g_U8TxBuffer[U8_TX_SIZE];
#endif

__IO uint32_t g_Uart8_TransferState = UART8_TRANSFER_COMPLETE;

uint8_t g_rx_buffer[RXBUFFERSIZE];          /* HAL库使用的串口接收缓冲 */
UART_RxCpltCallbackFunc usart8_rxcplt_callback = NULL;// 定义USART2回调函数指针
/*
*********************************************************************************************************
*	                                           函数声明
*********************************************************************************************************
*/
static void UART8_RxCpltCallback(UART_HandleTypeDef *huart);

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
	
	/* UART8_TX Init */
	hdma_uart8_tx.Instance = DMA2_Stream1;
	hdma_uart8_tx.Init.Request = DMA_REQUEST_UART8_TX;
	hdma_uart8_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma_uart8_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_uart8_tx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_uart8_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_uart8_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_uart8_tx.Init.Mode = DMA_NORMAL;
	hdma_uart8_tx.Init.Priority = DMA_PRIORITY_LOW;
	hdma_uart8_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	if (HAL_DMA_Init(&hdma_uart8_tx) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
	__HAL_LINKDMA(&huart8,hdmatx,hdma_uart8_tx);
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
	
	usart8_rxcplt_callback = UART8_RxCpltCallback; // 绑定专属回调
	
#if UART8_RX_NE
	HAL_NVIC_SetPriority(UART8_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(UART8_IRQn);
	    
	/* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
	HAL_UART_Receive_IT(&huart8, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
#endif
	
#if UART8_RX_DMA

	__HAL_DMA_DISABLE_IT(&hdma_uart8_rx, DMA_IT_HT);  // 禁用半满中断
	__HAL_UART_CLEAR_IDLEFLAG(&huart8); //串口初始化完成后空闲中断标志位是1 需要清除  //很有必要 可以自己仿真看一下初始化后标志位是置一的

	/*##-4- 配置中断 #########################################*/
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
	/* UART8 interrupt Init */
	HAL_NVIC_SetPriority(UART8_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(UART8_IRQn);
	
  // 启动 DMA 接收
	HAL_UARTEx_ReceiveToIdle_DMA(&huart8, g_U8RxBuffer, U8_RX_SIZE);
#endif	
}

/**
 * @brief       串口1中断服务函数
 * @param       无
 * @retval      无
 */
void UART8_IRQHandler(void)
{ 
	// 检查是否为空闲中断
	if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) && 
			__HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_IDLE)) {

		// 必须先清标志：先读SR，再读DR
		__HAL_UART_CLEAR_IDLEFLAG(&huart1);

		// 暂停DMA传输，以便安全读取已接收字节数
		HAL_DMA_Abort(&hdma_usart1_rx);

		// 计算实际接收长度
		g_uart1_received_len = RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);

		// 手动调用接收完成回调（模拟HAL内部行为）
		HAL_UART_RxCpltCallback(&huart8);
	}
  HAL_UART_IRQHandler(&huart8); /* 调用HAL库中断处理公用函数 */
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == UART8)
    {
      
        __HAL_UART_DISABLE_IT(&huart8, UART_IT_IDLE);
    }
}


/**
 * @brief       Rx传输回调函数
 * @param       huart: UART句柄类型指针
 * @retval      无
 */
void UART8_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART8)                             /* 如果是串口1 */
    {
        if ((g_uart8_rx_sta & 0x8000) == 0)                    /* 接收未完成 */
        {
            if (g_uart8_rx_sta & 0x4000)                       /* 接收到了0x0d */
            {
                if (g_rx_buffer[0] != 0x0a) 
                {
                    g_uart8_rx_sta = 0;                       /* 接收错误,重新开始 */
                }
                else 
                {
                    g_uart8_rx_sta |= 0x8000;                 /* 接收完成了 */
                }
            }
            else                                              /* 还没收到0X0D */
            {
                if(g_rx_buffer[0] == 0x0d)
                {
                    g_uart8_rx_sta |= 0x4000;
                }
                else
                {
                    g_U8RxBuffer[g_uart8_rx_sta & 0X3FFF] = g_rx_buffer[0] ;
                    g_uart8_rx_sta++;
                    if(g_uart8_rx_sta > (U8_RX_SIZE - 1))
                    {
                        g_uart8_rx_sta = 0;                   /* 接收数据错误,重新开始接收 */
                    }
                }
            }
        }
        HAL_UART_Receive_IT(&huart8, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
    }
}



 /*
*********************************************************************************************************
*	函 数 名: uart8_test
*	功能说明: 串口测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void uart_test(void)
{
	uint8_t len;
	uint16_t times = 0;
	while(1)
	{
        if (g_uart8_rx_sta & 0x8000)                                                    /* 接收到了数据? */
        {
            len = g_uart8_rx_sta & 0x3fff;                                              /* 得到此次接收到的数据长度 */
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
            }

            if (times % 30  == 0) 
            {
//                LED0_TOGGLE();                   /* 闪烁LED,提示系统正在运行 */
            }

            delay_ms(10);
        }
    }	
}

/******************************************************************************************/
/* 加入以下代码, 支持printf函数, 而不需要选择use MicroLIB */

#if 1
/* 重定义fputc函数, printf函数最终会通过调用fputc输出字符串到串口 */
int fputc(int ch, FILE *f)
{
    while ((UART8->ISR & 0X40) == 0);    /* 等待上一个字符发送完成 */

    UART8->TDR = (uint8_t)ch;            /* 将要发送的字符 ch 写入到DR寄存器 */
    return ch;
}
#endif
 



 

 





