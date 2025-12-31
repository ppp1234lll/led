#include "w25qxx.h"
#include "bsp_spi_flash.h"
#include "appconfig.h"

/*
	10、W25Q128存储芯片：(硬件SPI方式)，引脚分配为：
		MOSI:   PB5
		MISO:   PB4
		CLK:    PB3
		CS:     PD7
*/

uint8_t W25QXX_BUFFER[4096] = {0};
#define	W25QXX_CS 			PDout(7)  	// W25QXX的片选信号

/*
*********************************************************************************************************
*	函 数 名: bsp_InitCS
*	功能说明: CS控制脚初始化函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitCS(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD,ENABLE);

	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7;
 	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_OUT;//输出
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOD,&GPIO_InitStructure); 	
}

/************************************************************
*
* Function name	: W25QXX_Init
* Description	: 初始化函数-4Kbytes为一个Sector-16个扇区为1个Block
*				: -容量为16M字节,共有128个Block,4096个Sector
* Parameter		: 
* Return		: 
*	
************************************************************/


/*
*********************************************************************************************************
*	函 数 名: W25QXX_Init
*	功能说明: 初始化函数
*	形    参: 无
*	返 回 值: 无
*	-4Kbytes为一个Sector   -16个扇区为1个Block
* -容量为16M字节,共有128个Block,4096个Sector
*********************************************************************************************************
*/
void W25QXX_Init(void)
{
  uint16_t W25QXX_TYPE = 0;	//  
	bsp_InitCS();
	W25QXX_CS = 1;                  // SPI FLASH不选中
	bsp_InitSPIFlash();              // 初始化SPI
	W25QXX_TYPE = W25QXX_ReadID();  // 读取FLASH ID.
	
	printf("0x%04X\n",W25QXX_TYPE);
	W25QXX_CS = 1;                  // SPI FLASH不选中
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_ReadSR
*	功能说明: 读取SR寄存器状态
*	形    参: 无
*	返 回 值: 状态
*********************************************************************************************************
*/
static uint8_t W25QXX_ReadSR(void)
{
	uint8_t byte=0;

	W25QXX_CS=0;                            // 使能器件
	SPI_ReadWriteByte(W25X_ReadStatusReg);	// 发送读取状态寄存器命令
	byte=SPI_ReadWriteByte(0Xff);        	// 读取一个字节
	W25QXX_CS=1;                            // 取消片选

	return byte;
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_Write_Enable
*	功能说明: 写使能
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void W25QXX_Write_Enable(void)
{
    W25QXX_CS=0;                            // 使能器件
    SPI_ReadWriteByte(W25X_WriteEnable); 	// 发送写使能
    W25QXX_CS=1;                            // 取消片选
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_ReadID
*	功能说明: 读取器件ID
*	形    参: 无
*	返 回 值: 32bit的器件ID (最高8bit填0，有效ID位数为24bit）
*********************************************************************************************************
*/
uint16_t W25QXX_ReadID(void)
{
	uint16_t Temp = 0;

	W25QXX_CS=0;
	SPI_ReadWriteByte(0x90); // 发送读取ID命令
	SPI_ReadWriteByte(0x00);
	SPI_ReadWriteByte(0x00);
	SPI_ReadWriteByte(0x00);
	Temp|=SPI_ReadWriteByte(0xFF)<<8;
	Temp|=SPI_ReadWriteByte(0xFF);
	W25QXX_CS=1;

	return Temp;
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_Read
*	功能说明: 连续读取若干字节，字节个数不能超出芯片容量。
*	形    参:  	 
*	@pBuffer	   : 数据存储区
*	@ReadAddr	   : 开始读取的地址(24bit)
*	@NumByteToRead : 要读取的字节数(最大65535)
*	返 回 值: 无
*********************************************************************************************************
*/
void W25QXX_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead)
{
	uint16_t i;
	W25QXX_CS=0;                            		// 使能器件
	SPI_ReadWriteByte(W25X_ReadData);   			// 发送读取命令
	SPI_ReadWriteByte((uint8_t)((ReadAddr)>>16));	// 发送24bit地址
	SPI_ReadWriteByte((uint8_t)((ReadAddr)>>8));
	SPI_ReadWriteByte((uint8_t)ReadAddr);
	for(i=0; i<NumByteToRead; i++)
	{
			pBuffer[i]=SPI_ReadWriteByte(0XFF);   		// 循环读数
	}
	W25QXX_CS=1;
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_Wait_Busy
*	功能说明: 等待空闲
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void W25QXX_Wait_Busy(void)
{
    while((W25QXX_ReadSR()&0x01)==0x01);   // 等待BUSY位清空
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_Write_Page
*	功能说明: 写1个扇区。
*	形    参:  
*	@pBuffer		: 数据存储区
*	@WriteAddr		: 开始写入的地址(24bit)
*	@NumByteToWrite : 要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!
*	返 回 值: 0 : 错误， 1 ： 成功
*********************************************************************************************************
*/
void W25QXX_Write_Page(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
    uint16_t i;
    W25QXX_Write_Enable();                    // SET WEL
    W25QXX_CS=0;                              // 使能器件
    SPI_ReadWriteByte(W25X_PageProgram);      // 发送写页命令
    SPI_ReadWriteByte((uint8_t)((WriteAddr)>>16)); // 发送24bit地址
    SPI_ReadWriteByte((uint8_t)((WriteAddr)>>8));
    SPI_ReadWriteByte((uint8_t)WriteAddr);
    for(i=0; i<NumByteToWrite; i++)SPI_ReadWriteByte(pBuffer[i]); // 循环写数
    W25QXX_CS=1;                 			// 取消片选
    W25QXX_Wait_Busy();						// 等待写入结束
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_Write_NoCheck
*	功能说明: 写数据-无需校验
*	形    参:  
*	@pBuffer		: 数据存储区
*	@WriteAddr		: 开始写入的地址(24bit)
*	@NumByteToWrite : 要写入的字节数(最大65535)
*	返 回 值: 0 : 错误， 1 ： 成功
*********************************************************************************************************
*/
void W25QXX_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
	uint16_t pageremain;
	pageremain=256-WriteAddr%256; // 单页剩余的字节数
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite; // 不大于256个字节
	while(1)
	{
		W25QXX_Write_Page(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain) {
			break; 									// 写入结束了
		}
		else 
		{									// NumByteToWrite>pageremain
			pBuffer+=pageremain;
			WriteAddr+=pageremain;

			NumByteToWrite-=pageremain;			  	// 减去已经写入了的字节数
			if(NumByteToWrite>256)pageremain=256; 	// 一次可以写入256个字节
			else pageremain=NumByteToWrite; 	  	// 不够256个字节了
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_Write
*	功能说明: 写1个扇区并校验 
*	形    参: 
*	@pBuffer		: 数据存储区
*	@WriteAddr		: 开始写入的地址(24bit)
*	@NumByteToWrite : 要写入的字节数(最大65535)
*	返 回 值: 1 : 成功， 0 ： 失败
*********************************************************************************************************
*/
void W25QXX_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
    uint32_t secpos = 0;
    uint32_t secoff = 0;
    uint32_t secremain = 0;
    uint32_t i = 0;
    uint8_t * W25QXX_BUF = NULL;

    W25QXX_BUF=W25QXX_BUFFER;

    secpos=WriteAddr/4096; // 扇区地址
    secoff=WriteAddr%4096; // 在扇区内的偏移
    secremain=4096-secoff; // 扇区剩余空间大小
		W25QXX_ReadSR();
    if(NumByteToWrite<=secremain)secremain=NumByteToWrite; // 不大于4096个字节
    while(1)
    {
        W25QXX_Read(W25QXX_BUF,secpos*4096,4096); 	// 读出整个扇区的内容
        for(i=0; i<secremain; i++) 					// 校验数据
        {
            if(W25QXX_BUF[secoff+i]!=0XFF)break;	// 需要擦除
        }
        if(i<secremain)								// 需要擦除
        {
            W25QXX_Erase_Sector(secpos);			// 擦除这个扇区
            for(i=0; i<secremain; i++)	  			// 复制
            {
                W25QXX_BUF[i+secoff]=pBuffer[i];
            }
            W25QXX_Write_NoCheck(W25QXX_BUF,secpos*4096,4096);	// 写入整个扇区
        } else {
            W25QXX_Write_NoCheck(pBuffer,WriteAddr,secremain);  // 写已经擦除了的,直接写入扇区剩余区间.
        }
        if(NumByteToWrite==secremain) {
			break;												// 写入结束了
        } else {												// 写入未结束
            secpos++;											// 扇区地址增1
            secoff=0;											// 偏移位置为0

            pBuffer+=secremain;  								// 指针偏移
            WriteAddr+=secremain;								// 写地址偏移
            NumByteToWrite-=secremain;							// 字节数递减
            if(NumByteToWrite>4096) {
				secremain=4096;									// 下一个扇区还是写不完
            } else { 
				secremain = NumByteToWrite;						// 下一个扇区可以写完了
			}
    }
  }
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_Write_Update_Param
*	功能说明: 写升级参数
*	形    参:  
*	@pBuffer		: 数据存储区
*	@WriteAddr		: 开始写入的地址(24bit)
*	@NumByteToWrite : 要写入的字节数(最大65535)
*	返 回 值: 1 : 成功， 0 ： 失败
*********************************************************************************************************
*/
void W25QXX_Write_Update_Param(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
	uint32_t secpos = 0;

	secpos=WriteAddr/4096; // 扇区地址

	W25QXX_Erase_Sector(secpos);			// 擦除这个扇区
	W25QXX_Write_NoCheck(pBuffer,WriteAddr,NumByteToWrite);  // 写已经擦除了的,直接写入扇区剩余区间.
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_Erase_Chip
*	功能说明: 擦除片
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void W25QXX_Erase_Chip(void)
{
    W25QXX_Write_Enable();              // SET WEL
    W25QXX_Wait_Busy();
    W25QXX_CS=0;                        // 使能器件
    SPI_ReadWriteByte(W25X_ChipErase);  // 发送片擦除命令
    W25QXX_CS=1;                        // 取消片选
    W25QXX_Wait_Busy();   				   	  // 等待芯片擦除结束
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_Erase_Sector
*	功能说明: 擦除扇区
*	形    参: 
*	@Dst_Addr	: 地址
*	返 回 值: 无
*********************************************************************************************************
*/
void W25QXX_Erase_Sector(uint32_t Dst_Addr)
{
    Dst_Addr*=4096;
    W25QXX_Write_Enable();                  // SET WEL
    W25QXX_Wait_Busy();
    W25QXX_CS=0;                            // 使能器件
    SPI_ReadWriteByte(W25X_SectorErase);  	// 发送扇区擦除指令
    SPI_ReadWriteByte((uint8_t)((Dst_Addr)>>16));  // 发送24bit地址
    SPI_ReadWriteByte((uint8_t)((Dst_Addr)>>8));
    SPI_ReadWriteByte((uint8_t)Dst_Addr);
    W25QXX_CS=1;                       		// 取消片选
    W25QXX_Wait_Busy();   				   	// 等待擦除完成
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_PowerDown
*	功能说明: 进入掉电模式
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void W25QXX_PowerDown(void)
{
    W25QXX_CS=0;                            // 使能器件
    SPI_ReadWriteByte(W25X_PowerDown);      // 发送掉电命令
    W25QXX_CS=1;                            // 取消片选
    delay_us(3);                            // 等待TPD
}

/*
*********************************************************************************************************
*	函 数 名: W25QXX_WAKEUP
*	功能说明: 唤醒
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void W25QXX_WAKEUP(void)
{
    W25QXX_CS=0;                            	// 使能器件
    SPI_ReadWriteByte(W25X_ReleasePowerDown);	// send W25X_PowerDown command 0xAB
    W25QXX_CS=1;                            	// 取消片选
    delay_us(3);                               	// 等待TRES1
}

/*
*********************************************************************************************************
*	函 数 名: w25qxx_test
*	功能说明: w25qxx测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void w25qxx_test(void)
{
	uint16_t W25QXX_TYPE = 0;
	while(1)
	{
		W25QXX_TYPE = W25QXX_ReadID();  // 读取FLASH ID.
		printf("W25QXX ID: 0x%04X \n",W25QXX_TYPE);
			
		delay_ms(1000);		
	}
}

