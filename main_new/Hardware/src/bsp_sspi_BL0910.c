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

#include "bsp_sspi_BL0910.h"
#include "bsp.h"

#define SOFT_SPI_SCLK_GPIO_CLK()		__HAL_RCC_GPIOE_CLK_ENABLE()
#define SOFT_SPI_SCLK_GPIO 					GPIOE
#define SOFT_SPI_SCLK_PIN  					GPIO_PIN_12

#define SOFT_SPI_MOSI_GPIO_CLK()		__HAL_RCC_GPIOE_CLK_ENABLE()
#define SOFT_SPI_MOSI_GPIO 				 GPIOE
#define SOFT_SPI_MOSI_PIN 				 GPIO_PIN_14

#define SOFT_SPI_MISO_GPIO_CLK()		__HAL_RCC_GPIOE_CLK_ENABLE()
#define SOFT_SPI_MISO_GPIO 				 GPIOE
#define SOFT_SPI_MISO_PIN 				 GPIO_PIN_13

#define SOFT_SPI_SCLK_H		HAL_GPIO_WritePin(SOFT_SPI_SCLK_GPIO, SOFT_SPI_SCLK_PIN, GPIO_PIN_SET)
#define SOFT_SPI_SCLK_L		HAL_GPIO_WritePin(SOFT_SPI_SCLK_GPIO, SOFT_SPI_SCLK_PIN, GPIO_PIN_RESET)
#define SOFT_SPI_MOSI_H		HAL_GPIO_WritePin(SOFT_SPI_MOSI_GPIO, SOFT_SPI_MOSI_PIN, GPIO_PIN_SET) 
#define SOFT_SPI_MOSI_L		HAL_GPIO_WritePin(SOFT_SPI_MOSI_GPIO, SOFT_SPI_MOSI_PIN, GPIO_PIN_RESET)

#define SOFT_SPI_MISO     HAL_GPIO_ReadPin(SOFT_SPI_MISO_GPIO,SOFT_SPI_MISO_PIN)
/*
*********************************************************************************************************
*	函 数 名: bsp_InitSSPI_BL0910
*	功能说明: 配置SPI GPIO
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitSSPI_BL0910(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	SOFT_SPI_SCLK_GPIO_CLK();
	SOFT_SPI_MOSI_GPIO_CLK();
  SOFT_SPI_MISO_GPIO_CLK();
	
	GPIO_InitStruct.Pin = SOFT_SPI_SCLK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SOFT_SPI_SCLK_GPIO, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = SOFT_SPI_MOSI_PIN;
  HAL_GPIO_Init(SOFT_SPI_MOSI_GPIO, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SOFT_SPI_MISO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SOFT_SPI_MISO_GPIO, &GPIO_InitStruct);	
}


/*
*********************************************************************************************************
*	函 数 名: hspi_delay
*	功能说明: spi延时
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void hspi_delay(uint16_t time)	
{
//	delay_us(time);
	do
	{
	}while(time--);
}

/*
*********************************************************************************************************
*	函 数 名: HSPI_WriteByte
*	功能说明: 模拟 SPI 写一个字节
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值: 无
*********************************************************************************************************
*/
void HSPI_WriteByte(uint8_t TxData)  
{
	uint8_t i = 0;  
	for(i=0; i<8; i++) 
	{
		SOFT_SPI_SCLK_L;                //拉低时钟，即空闲时钟为低电平， CPOL=0；
		if(TxData&0x80) 
			SOFT_SPI_MOSI_H;
		else 
			SOFT_SPI_MOSI_L;
		TxData<<=1;
		hspi_delay(100); 
		SOFT_SPI_SCLK_H;          // 上升沿采样 //CPHA=0   
		hspi_delay(100); 
	}
	SOFT_SPI_SCLK_L;         // 最后SPI发送完后，拉低时钟，进入空闲状态；
}

/*
*********************************************************************************************************
*	函 数 名: HSPI_Write_Multi_Byte
*	功能说明: 模拟 SPI 写数据
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值: 无
*********************************************************************************************************
*/
void HSPI_Write_Multi_Byte(uint8_t *buff, uint16_t len)
{
	while(len--) {
		HSPI_WriteByte(buff[0]);
		buff++;
	}
}

/*
*********************************************************************************************************
*	函 数 名: HSPI_ReadByte
*	功能说明: 模拟 SPI 读一个字节
*	形    参: 
*	返 回 值: 读到的数据
*********************************************************************************************************
*/
uint8_t HSPI_ReadByte(void)
{
	uint8_t i = 0;
	uint8_t RecevieData=0;
	for(i=0; i<8; i++) 
	{
		SOFT_SPI_SCLK_H;           //拉低时钟，即空闲时钟为低电平；  
		hspi_delay(100); 
		SOFT_SPI_SCLK_L;   
		RecevieData<<=1;
		if(SOFT_SPI_MISO) 
			RecevieData |= 0x01;
		else 
			RecevieData &= ~0x01;   // 下降沿接收数据
		hspi_delay(100); 
	}
	return RecevieData;
}


/*
*********************************************************************************************************
*	函 数 名: HSPI_test
*	功能说明: SPI测试
*	形    参: 
*	返 回 值:  
*********************************************************************************************************
*/
void HSPI_test(void)
{
	while(1)
	{
		HSPI_ReadByte();
	}
}


