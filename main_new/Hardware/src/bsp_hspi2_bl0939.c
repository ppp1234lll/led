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

#include "bsp_hspi2_bl0939.h"

#define SPI2_SCLK_GPIO_CLK		RCC_AHB1Periph_GPIOB   
#define SPI2_SCLK_GPIO 				GPIOB
#define SPI2_SCLK_PIN  				GPIO_Pin_10
					 
#define SPI2_MOSI_GPIO_CLK		RCC_AHB1Periph_GPIOB
#define SPI2_MOSI_GPIO 				GPIOB
#define SPI2_MOSI_PIN 				GPIO_Pin_15
					 
#define SPI2_MISO_GPIO_CLK		RCC_AHB1Periph_GPIOB
#define SPI2_MISO_GPIO 				GPIOB
#define SPI2_MISO_PIN 				GPIO_Pin_14
					 
#define SPI2_SCLK		PBout(10)
#define SPI2_MOSI		PBout(15)
#define SPI2_MISO		PBin(14)

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
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB,ENABLE);
	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI2,ENABLE );//SPI3时钟使能

  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;//复用功能
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;//上拉
  GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化

	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_SPI2); 
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource14,GPIO_AF_SPI2); 
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource15,GPIO_AF_SPI2); 
 
	//这里只针对SPI口初始化
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,ENABLE);//复位SPI1
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,DISABLE);//停止复位SPI1
	
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//设置SPI工作模式:设置为主SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//设置SPI的数据大小:SPI发送接收8位帧结构
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;		//串行同步时钟的空闲状态为高电平
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	//串行同步时钟的第二个跳变沿（上升或下降）数据被采样
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;		//定义波特率预分频的值:波特率预分频值为256
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRC值计算的多项式
	SPI_Init(SPI2, &SPI_InitStructure);  //根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器
 
	SPI_Cmd(SPI2, ENABLE); //使能SPI外设
	
	SPI2_ReadWriteByte(0xff);//启动传输		 
}

/*
*********************************************************************************************************
*	函 数 名: SPI2_ReadWriteByte
*	功能说明: 读写字节函数
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值: 读取到的字节
*********************************************************************************************************
*/
uint8_t SPI2_ReadWriteByte(uint8_t TxData)
{
	u8 retry=0;				 	
	while((SPI2->SR&SPI_I2S_FLAG_TXE)==RESET) //检查指定的SPI标志位设置与否:发送缓存空标志位
	{
		retry++;
		if(retry>200)return 0;
	}			  
	SPI2->DR=TxData;	 	//发送一个byte   //通过外设SPIx发送一个数据
	retry=0;

	while((SPI2->SR&SPI_I2S_FLAG_RXNE)==RESET) //检查指定的SPI标志位设置与否:接受缓存非空标志位
	{
		retry++;
		if(retry>200)return 0;
	}	  						    
	return SPI2->DR; //返回通过SPIx最近接收的数据
}


/*
*********************************************************************************************************
*	函 数 名: SPI2_Write_Multi_Byte
*	功能说明: 模拟 SPI 写数据
*	形    参: 
*	@TxData		: 写入字节
*	返 回 值: 无
*********************************************************************************************************
*/
void SPI2_Write_Multi_Byte(uint8_t *buff, uint16_t len)
{
	while(len--) {
		SPI2_ReadWriteByte(buff[0]);
		buff++;
	}
}


/*
*********************************************************************************************************
*	函 数 名: HSPI2_ReadByte
*	功能说明: 模拟 SPI 读一个字节
*	形    参: 
*	返 回 值: 读到的数据
*********************************************************************************************************
*/
uint8_t HSPI2_ReadByte(void)
{
	return SPI2_ReadWriteByte(0xff);
}


