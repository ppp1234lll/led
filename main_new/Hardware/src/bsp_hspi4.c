/********************************************************************************
* @File name  : 电能计量驱动
* @Description: 硬件SPI通信
* @Author     : ZHLE
*  Version Date        Modification Description
	6、BL0910单相计量芯片: 硬件SPI3
	   引脚分配为：   BL_SCLK： PE12
                    BL_MISO： PE13 
                    BL_MOSI： PE14

********************************************************************************/

#include "bsp_hspi4.h"
#include "bsp.h"

SPI_HandleTypeDef hspi4;
/*
*********************************************************************************************************
*	函 数 名: bsp_InitHSPI4
*	功能说明: 配置SPI GPIO
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitHSPI4(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
	
  /** Initializes the peripherals clock
  */
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI4;
	PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
	/* SPI4 clock enable */
	__HAL_RCC_SPI4_CLK_ENABLE();

	__HAL_RCC_GPIOE_CLK_ENABLE();
	/**SPI4 GPIO Configuration
	PE12     ------> SPI4_SCK
	PE13     ------> SPI4_MISO
	PE14     ------> SPI4_MOSI
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);	
	
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 0x0;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }	
	
}

/*
*********************************************************************************************************
*	函 数 名: HSPI4_Send_Data
*	功能说明: 模拟 SPI 写数据
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值: 无
*********************************************************************************************************
*/
void HSPI4_Send_Data(uint8_t *buff, uint16_t len)
{
	HAL_SPI_Transmit(&hspi4, buff, len, 1000);
}
/*
*********************************************************************************************************
*	函 数 名: HSPI_Read_Data
*	功能说明: 读数据
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值: 无
*********************************************************************************************************
*/
void HSPI4_Read_Data(uint8_t *txdata,uint8_t *rxdata, uint16_t len)
{
	HAL_SPI_TransmitReceive(&hspi4, txdata, rxdata,len, 1000);
}



