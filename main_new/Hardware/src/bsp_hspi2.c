/********************************************************************************
* @File name  : 电能计量驱动
* @Description: 硬件SPI2通信
* @Author     : ZHLE
*  Version Date        Modification Description
	7、BL0942单相计量芯片: 软件SPI
	   引脚分配为：    RCD_SCLK： PB10
                    RCD_MISO： PB14		
                    RCD_MOSI： PB15

********************************************************************************/

#include "bsp_hspi2.h"

SPI_HandleTypeDef hspi2;
/*
*********************************************************************************************************
*	函 数 名: bsp_InitSPI2
*	功能说明: 硬件初始化。 
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitSPI2(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
	
	/* SPI2 clock enable */
	__HAL_RCC_SPI2_CLK_ENABLE();

	__HAL_RCC_GPIOB_CLK_ENABLE();
	/**SPI2 GPIO Configuration
	PB10     ------> SPI2_SCK
	PB14     ------> SPI2_MISO
	PB15     ------> SPI2_MOSI
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_14|GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x0;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
}


/*
*********************************************************************************************************
*	函 数 名: HSPI_Send_Data
*	功能说明: 模拟 SPI 写数据
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值: 无
*********************************************************************************************************
*/
void HSPI2_Send_Data(uint8_t *buff, uint16_t len)
{
	HAL_SPI_Transmit(&hspi2, buff, len, 1000);
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
void HSPI2_Read_Data(uint8_t *txdata,uint8_t *rxdata, uint16_t len)
{
	HAL_SPI_TransmitReceive(&hspi2, txdata, rxdata,len, 1000);
}



