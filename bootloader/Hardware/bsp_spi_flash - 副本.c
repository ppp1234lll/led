#include "bsp_spi_flash.h"
#include "main.h"

/*
	10、W25Q128存储芯片：(硬件SPI方式)，引脚分配为：
		MOSI:   PB5
		MISO:   PB4
		CLK:    PB3
		CS:     PD7
*/
SPI_HandleTypeDef hspi5;                          /* SPI句柄 */

/************************************************************
*
* Function name	: bsp_InitSPI_Flash
* Description	: spi初始化函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void bsp_InitSPI_Flash(void)
{
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 0x0;
  hspi5.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi5.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi5.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi5.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi5.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi5.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi5.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi5.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi5.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi5.Init.IOSwap = SPI_IO_SWAP_DISABLE;
	HAL_SPI_Init(&hspi5);                                           /* 初始化 */
	
	__HAL_SPI_ENABLE(&hspi5);                                       /* 使能SPI2 */

	SPI_ReadWriteByte(0xff);                                        /* 启动传输 */ 
}

/**
 * @brief       SPI2底层驱动，时钟使能，引脚配置
 * @note        此函数会被HAL_SPI_Init()调用
 * @param       hspi:SPI句柄
 * @retval      无
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    
	__HAL_RCC_SPI5_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI5;
	PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
	
	/**SPI5 GPIO Configuration
	PF7     ------> SPI5_SCK
	PF8     ------> SPI5_MISO
	PF9     ------> SPI5_MOSI
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

/************************************************************
*
* Function name	: SPI_ReadWriteByte
* Description	: 读写字节函数
* Parameter		: 
*	@TxData		: 写入字节
* Return		: 读取到的字节
*	
************************************************************/
uint8_t SPI_ReadWriteByte(uint8_t txdata)
{
	uint8_t rxdata;
	HAL_SPI_TransmitReceive(&hspi5, &txdata, &rxdata, 1, 1000);
	return rxdata;                                    /* 返回收到的数据 */
}



