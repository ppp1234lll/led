/**
 ****************************************************************************************************
 * @file        usart.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2023-03-02
 * @brief       串口初始化代码(一般是串口1)，支持printf
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20220420
 * 第一次发布
 * V1.1 20230607
 * 修改SYS_SUPPORT_OS部分代码, 包含头文件改成:"os.h"
 * 删除USART_UX_IRQHandler()函数的超时处理和修改HAL_UART_RxCpltCallback()
 *
 ****************************************************************************************************
 */

#include "bsp.h"
#include "bsp_usart1.h"

#define UART1_RX_NE     0    // 使用串口中断
#define UART1_RX_DMA    1    // 使用串口DMA

#define U1_RX_SIZE  (32*4)
/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_usart1_rx_sta = 0;
__ALIGNED(32) uint8_t g_U1RxBuffer[U1_RX_SIZE];

UART_HandleTypeDef huart1;          /* UART句柄 */

#if UART1_RX_DMA
	DMA_HandleTypeDef hdma_usart1_rx;
	DMA_HandleTypeDef hdma_usart1_tx;
#endif
/*
*********************************************************************************************************
*	函 数 名: bsp_InitUsart1
*	功能说明: 调试串口初始化
*	形    参: baudrate : 波特率
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitUsart1(uint32_t baudrate)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
	PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* USART1 clock enable */
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
#if UART1_RX_DMA
	__HAL_RCC_DMA1_CLK_ENABLE();
#endif
	/**USART1 GPIO Configuration
	PA9     ------> USART1_TX
	PA10     ------> USART1_RX
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* USART1 interrupt Init */
	HAL_NVIC_SetPriority(USART1_IRQn, 8, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = baudrate;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }



	
#if UART1_RX_NE
	/* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
	HAL_UART_Receive_IT(&huart1, (uint8_t *)g_U1RxBuffer, U1_RX_SIZE);
#endif
	
}

/*
*********************************************************************************************************
*	函 数 名: Usart1_SendString
*	功能说明: 发送字符串
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Usart1_SendString(uint8_t *str)
{
	unsigned int k=0;
  do 
  {
      HAL_UART_Transmit( &huart1,(uint8_t *)(str + k) ,1,1000);
      k++;
  } while(*(str + k)!='\0');
  
}


/*
*********************************************************************************************************
*	函 数 名: USART1_IRQHandler
*	功能说明: 串口中断函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void USART1_IRQHandler(void)
{ 
#if UART1_RX_NE
	uint8_t rxdata;
	if (USART1->ISR & USART_FLAG_RXNE)   /* 接收到数据 */
	{
		rxdata = USART1->RDR;
    if ((g_usart1_rx_sta & 0x8000) == 0)     /* 接收未完成? */
    {
			if (g_usart1_rx_sta & 0x4000)        /* 接收到了0x0d? */
			{
				if (rxdata != 0x0a)             /* 接收到了0x0a? (必须先接收到到0x0d,才检查0x0a) */
					g_usart1_rx_sta = 0;         /* 接收错误, 重新开始 */
				else
					g_usart1_rx_sta |= 0x8000;   /* 收到了0x0a,标记接收完成了 */
			}
			else      /* 还没收到0x0d */
			{
				if (rxdata == 0x0d)
					g_usart1_rx_sta |= 0x4000;   /* 标记接收到了 0x0d */
				else
				{
					g_U1RxBuffer[g_usart1_rx_sta & 0X3FFF] = rxdata;   /* 存储数据到 g_usart_rx_buf */
					g_usart1_rx_sta++;

					if (g_usart1_rx_sta > (U1_RX_SIZE - 1))g_usart1_rx_sta = 0;/* 接收数据溢出, 重新开始接收 */
				}
			}
		}
  }
#endif
	/* 清除中断标志 */
	SET_BIT(USART1->ICR, UART_CLEAR_PEF);
	SET_BIT(USART1->ICR, UART_CLEAR_FEF);
	SET_BIT(USART1->ICR, UART_CLEAR_NEF);
	SET_BIT(USART1->ICR, UART_CLEAR_OREF);
	SET_BIT(USART1->ICR, UART_CLEAR_IDLEF);
	SET_BIT(USART1->ICR, UART_CLEAR_TCF);
	SET_BIT(USART1->ICR, UART_CLEAR_LBDF);
	SET_BIT(USART1->ICR, UART_CLEAR_CTSF);
	SET_BIT(USART1->ICR, UART_CLEAR_CMF);
	SET_BIT(USART1->ICR, UART_CLEAR_WUF);
	SET_BIT(USART1->ICR, UART_CLEAR_TXFECF);
}


/*
*********************************************************************************************************
*	函 数 名: usart_debug_test
*	功能说明: 串口测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void usart1_test(void)
{
    uint8_t len;
    uint16_t times = 0;
	while(1)
	{
        if (g_usart1_rx_sta & 0x8000)                                                    /* 接收到了数据? */
        {
            len = g_usart1_rx_sta & 0x3fff;                                              /* 得到此次接收到的数据长度 */
            Usart1_SendString("\r\n您发送的消息为:\r\n");

            HAL_UART_Transmit(&huart1, (uint8_t *)g_U1RxBuffer, len, 1000);   /* 发送接收到的数据 */
            while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) != SET);           /* 等待发送结束 */
            Usart1_SendString("\r\n\r\n");                                                         /* 插入换行 */
            g_usart1_rx_sta = 0;
        }
        else
        {
            times++;

            if (times % 5000 == 0)
            {
                Usart1_SendString("\r\n正点原子 STM32开发板 串口实验\r\n");
                Usart1_SendString("正点原子@ALIENTEK\r\n\r\n\r\n");
            }

            if (times % 200 == 0)
            {
                Usart1_SendString("请输入数据,以回车键结束\r\n");
            }

            if (times % 30  == 0) 
            {
                Usart1_SendString("times\r\n");
            }

            delay_ms(10);
        }
    }	
}

 



