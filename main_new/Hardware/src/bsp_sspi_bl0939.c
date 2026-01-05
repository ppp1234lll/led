/********************************************************************************
* @File name  : 电能计量驱动
* @Description: 模拟SPI通信
* @Author     : ZHLE
*  Version Date        Modification Description
	7、BL0942单相计量芯片: 软件SPI
	   引脚分配为： RCD_SCLK：  PB10
                  RCD_MISO：  PB14		
                  RCD_MOSI：  PB15
********************************************************************************/

#include "bsp_sspi_bl0939.h"
#include "bsp.h"

#define SOFT_SPI_SCLK_GPIO_CLK()		__HAL_RCC_GPIOB_CLK_ENABLE()  
#define SOFT_SPI_SCLK_GPIO 					GPIOB
#define SOFT_SPI_SCLK_PIN  					GPIO_PIN_10
                                  
#define SOFT_SPI_MOSI_GPIO_CLK()		__HAL_RCC_GPIOB_CLK_ENABLE()
#define SOFT_SPI_MOSI_GPIO 				 GPIOB
#define SOFT_SPI_MOSI_PIN 				 GPIO_PIN_15
                                  
#define SOFT_SPI_MISO_GPIO_CLK()		__HAL_RCC_GPIOB_CLK_ENABLE()
#define SOFT_SPI_MISO_GPIO 				 GPIOB
#define SOFT_SPI_MISO_PIN 				 GPIO_PIN_14
 	    					
#define SOFT_SPI_SCLK_H		HAL_GPIO_WritePin(SOFT_SPI_SCLK_GPIO, SOFT_SPI_SCLK_PIN, GPIO_PIN_SET)
#define SOFT_SPI_SCLK_L		HAL_GPIO_WritePin(SOFT_SPI_SCLK_GPIO, SOFT_SPI_SCLK_PIN, GPIO_PIN_RESET)
#define SOFT_SPI_MOSI_H		HAL_GPIO_WritePin(SOFT_SPI_MOSI_GPIO, SOFT_SPI_MOSI_PIN, GPIO_PIN_SET) 
#define SOFT_SPI_MOSI_L		HAL_GPIO_WritePin(SOFT_SPI_MOSI_GPIO, SOFT_SPI_MOSI_PIN, GPIO_PIN_RESET)

#define SOFT_SPI_MISO     HAL_GPIO_ReadPin(SOFT_SPI_MISO_GPIO,SOFT_SPI_MISO_PIN)
/*
*********************************************************************************************************
*	函 数 名: bsp_InitSSPI
*	功能说明: 配置模拟SPI GPIO。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitSSPI(void)
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
*	函 数 名: sspi_delay
*	功能说明: 软件SPI延时
*	形    参: time 时间
*	返 回 值: 无
*********************************************************************************************************
*/
void sspi_delay(uint16_t time)	
{
	do
	{
	} while (--time);

}

/*
*********************************************************************************************************
*	函 数 名: SSPI_ReadWriteByte
*	功能说明: 读写字节函数 
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值: 读取到的字节
*********************************************************************************************************
*/
uint8_t SSPI_ReadWriteByte(uint8_t TxData)
{
	uint8_t RecevieData=0;
	uint8_t i = 0;

	for(i=0; i<8; i++)
	{
		SOFT_SPI_SCLK_L;
		sspi_delay(100);
		if(TxData&0x80) SOFT_SPI_MOSI_H;
		else SOFT_SPI_MOSI_L;
		TxData<<=1;
		sspi_delay(100);
		SOFT_SPI_SCLK_H;  // 上升沿采样
		sspi_delay(100);
		RecevieData<<=1;
		if(SOFT_SPI_MISO) RecevieData |= 0x01;
		else RecevieData &= ~0x01;   // 下降沿接收数据
		sspi_delay(100);
	}
	SOFT_SPI_SCLK_L;  // idle情况下SCK为电平
	sspi_delay(100);
	return RecevieData;
}

/*
*********************************************************************************************************
*	函 数 名: SSPI_WriteByte
*	功能说明: 模拟 SPI 写一个字节
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值:  
*********************************************************************************************************
*/
void SSPI_WriteByte(uint8_t TxData)  
{
	uint8_t i = 0;  
	for(i=0; i<8; i++) 
	{
		SOFT_SPI_SCLK_L; //CPOL=0        //拉低时钟，即空闲时钟为低电平， CPOL=0；
		if(TxData&0x80) SOFT_SPI_MOSI_H;
		else SOFT_SPI_MOSI_L;
		TxData<<=1;
		sspi_delay(100); 
		SOFT_SPI_SCLK_H;                   // 上升沿采样 //CPHA=0  
		sspi_delay(100); 
	}
	SOFT_SPI_SCLK_L;                   // 最后SPI发送完后，拉低时钟，进入空闲状态；
}
/*
*********************************************************************************************************
*	函 数 名: SSPI_Write_Multi_Byte
*	功能说明: 模拟 SPI 写多个字节
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值:  
*********************************************************************************************************
*/
void SSPI_Write_Multi_Byte(uint8_t *buff, uint16_t len)
{
	while(len--) {
		SSPI_WriteByte(buff[0]);
		buff++;
	}
}

/*
*********************************************************************************************************
*	函 数 名: SSPI_ReadByte
*	功能说明: 模拟 SPI 读一个字节
*	形    参: 
*	返 回 值: 读取到的字节
*********************************************************************************************************
*/
uint8_t SSPI_ReadByte(void)
{
	uint8_t i = 0;
	uint8_t RecevieData=0;
	for(i=0; i<8; i++) 
	{
		SOFT_SPI_SCLK_H;           //拉低时钟，即空闲时钟为低电平；  
		sspi_delay(100); 
		SOFT_SPI_SCLK_L;   
		RecevieData<<=1;
		if(SOFT_SPI_MISO) RecevieData |= 0x01;
		else RecevieData &= ~0x01;   // 下降沿接收数据
		sspi_delay(100); 
	}
	SOFT_SPI_SCLK_L;  // idle情况下SCK为电平
	return RecevieData;
}  


/*
*********************************************************************************************************
*	函 数 名: SSPI_test
*	功能说明: SPI测试
*	形    参: 
*	返 回 值:  
*********************************************************************************************************
*/
void SSPI_test(void)
{
	while(1)
	{
		SSPI_ReadByte();
	}
}






