/********************************************************************************
* @File name  : 光照度模块
* @Description: 模拟IIC通信
* @Author     : ZHLE
*  Version Date        Modification Description
	17、光照度模块： 模拟IIC，引脚分配为： 
	      BH1750_SCK：    PE2
        BH1750_SDA：    PE4

********************************************************************************/

#include "BH1750.h"
#include "bsp.h"


#define BH1750_SCL_GPIO_CLK        RCC_AHB1Periph_GPIOE
#define BH1750_SCL_GPIO            GPIOE
#define BH1750_SCL_PIN             GPIO_Pin_2

#define BH1750_SDA_GPIO_CLK        RCC_AHB1Periph_GPIOE
#define BH1750_SDA_GPIO            GPIOE
#define BH1750_SDA_PIN             GPIO_Pin_4

#define BH1750_SCL(x) (x ? GPIO_SetBits(BH1750_SCL_GPIO,BH1750_SCL_PIN) : GPIO_ResetBits(BH1750_SCL_GPIO,BH1750_SCL_PIN))

#define BH1750_SDA(x) (x ? GPIO_SetBits(BH1750_SDA_GPIO,BH1750_SDA_PIN) : GPIO_ResetBits(BH1750_SDA_GPIO,BH1750_SDA_PIN))

#define RD_BH1750_SDA  GPIO_ReadInputDataBit(BH1750_SDA_GPIO,BH1750_SDA_PIN)


/*
*********************************************************************************************************
*	函 数 名: bh1750_sda_in
*	功能说明: 设置SDA为输入
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void bh1750_sda_in(void)
{
//	GPIOD->MODER&=~(3<<(4*2));
//	GPIOD->MODER|= 0<<4*2;	 
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin   = BH1750_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;			 	 
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;    
	GPIO_Init(BH1750_SDA_GPIO,&GPIO_InitStructure); 

}
/*
*********************************************************************************************************
*	函 数 名: bh1750_sda_out
*	功能说明: 设置SDA为输出
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void bh1750_sda_out(void)
{
//	GPIOD->MODER&=~(3<<(4*2));
//	GPIOD->MODER|= 1<<4*2;  
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin   = BH1750_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;			// 输出
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  		// 推挽输出
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;   	// 上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; 	// 高速GPIO
	GPIO_Init(BH1750_SDA_GPIO,&GPIO_InitStructure); 
}

/*
*********************************************************************************************************
*	函 数 名: BH1750_IIC_GPIO_init
*	功能说明: IIC GPIO 初始化
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void BH1750_IIC_GPIO_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(BH1750_SCL_GPIO_CLK|BH1750_SDA_GPIO_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Pin   = BH1750_SCL_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;			// 输出
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  		// 推挽输出
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;   	// 上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; 	// 高速GPIO
	GPIO_Init(BH1750_SCL_GPIO,&GPIO_InitStructure); 

	GPIO_InitStructure.GPIO_Pin   = BH1750_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;			// 输出
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  		// 推挽输出
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;   	// 上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; 	// 高速GPIO
	GPIO_Init(BH1750_SDA_GPIO,&GPIO_InitStructure); 
	BH1750_SCL(1);
	BH1750_SDA(1);
}

/*
*********************************************************************************************************
*	函 数 名: bh1750_i2c_start
*	功能说明: 开始信号
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bh1750_i2c_start(void)
{
	bh1750_sda_out();
	BH1750_SDA(1);
	BH1750_SCL(1);
	delay_us(4);
 	BH1750_SDA(0);
	delay_us(4);
	BH1750_SCL(0);
}

/*
*********************************************************************************************************
*	函 数 名: bh1750_i2c_stop
*	功能说明: 结束信号
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bh1750_i2c_stop(void)
{
	bh1750_sda_out();
	BH1750_SCL(0);
	BH1750_SDA(0);
 	delay_us(4);
	BH1750_SCL(1);
	delay_us(4);
	BH1750_SDA(1);
}

/*
*********************************************************************************************************
*	函 数 名: bh1750_i2c_wait_ack
*	功能说明: 等待应答
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t bh1750_i2c_wait_ack(void)
{
	uint8_t ucErrTime=0;
    
	bh1750_sda_in();
	BH1750_SDA(1);
	delay_us(1);	   
	BH1750_SCL(1);
	delay_us(1);
	
	while(RD_BH1750_SDA)
	{
		ucErrTime++;
		if(ucErrTime > 250)
		{
			bh1750_i2c_stop();
			return 1;
		}
	}
	BH1750_SCL(0);//时钟输出0
	return 0;
} 

/*
*********************************************************************************************************
*	函 数 名: bh1750_i2c_ack
*	功能说明: 应答信号
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bh1750_i2c_ack(void)
{
	BH1750_SCL(0);
	bh1750_sda_out();
	BH1750_SDA(0);
	delay_us(2);
	BH1750_SCL(1);
	delay_us(2);
	BH1750_SCL(0);
}

/*
*********************************************************************************************************
*	函 数 名: bh1750_i2c_nack
*	功能说明: 无应答信号
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bh1750_i2c_nack(void)
{
	BH1750_SCL(0);
	bh1750_sda_out();
	BH1750_SDA(1);
	delay_us(2);
	BH1750_SCL(1);
	delay_us(2);
	BH1750_SCL(0);
}					 				     

/*
*********************************************************************************************************
*	函 数 名: bh1750_drive_write_byte
*	功能说明: 写字节
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bh1750_drive_write_byte(uint8_t dat)
{                        
	uint8_t t;   

	bh1750_sda_out();
	BH1750_SCL(0);//拉低时钟开始数据传输
	
	for(t=0;t<8;t++)
	{              
		if(dat&0x80)
		{
			BH1750_SDA(1);
		}
		else
		{
			BH1750_SDA(0);
		}
		dat<<=1; 	  
		delay_us(2); 
		BH1750_SCL(1);
		delay_us(2);
		BH1750_SCL(0);
		delay_us(2); 
	}
} 

/*
*********************************************************************************************************
*	函 数 名: bh1750_drive_read_byte
*	功能说明: 读字节
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t bh1750_drive_read_byte(uint8_t ack)
{
	uint8_t i = 0;
	uint8_t receive = 0;
	
	bh1750_sda_in();

	for(i=0;i<8;i++ )
	{
		BH1750_SCL(0);
		delay_us(2);
		BH1750_SCL(1);
		receive <<= 1;
		if(RD_BH1750_SDA)
		{
			receive++;
		}
		delay_us(1);
  }

	if(ack == 0)
		bh1750_i2c_nack();//发送nACK
	else if(ack == 1)
		bh1750_i2c_ack(); //发送ACK

	return receive;
}

/*
*********************************************************************************************************
*	函 数 名: BH1750_WriteI2C_Byte
*	功能说明: 向IIC设备写入一个字节数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void BH1750_WriteI2C_Byte(uint8_t Slave_Address,uint8_t REG_Address)
{
	uint8_t retry=0;	
	bh1750_i2c_start();        //起始信号         

	bh1750_drive_write_byte(Slave_Address+0);   //发送设备地址+写信号
	while(bh1750_i2c_wait_ack())  
	{
		retry++;
		if(retry>200) break;
	}	
	
	bh1750_drive_write_byte(REG_Address);    //内部寄存器地址，
	retry=0;
	while(bh1750_i2c_wait_ack())  
	{
		retry++;
		if(retry>200) break;
	}	
	
	bh1750_i2c_stop();   //发送停止信号
}

/*
*********************************************************************************************************
*	函 数 名: BH1750_ReadI2C_Data
*	功能说明: 读取BH1750内部数据
*	形    参: 无
*	返 回 值: 光照度 单位:lx
*********************************************************************************************************
*/

uint16_t BH1750_ReadI2C_Data(uint8_t Slave_Address)
{
	uint8_t retry=0;
  uint8_t  Data[2] = {0};
	uint16_t result = 0;
	bh1750_i2c_start();  //起始信号    
	 
	bh1750_drive_write_byte(Slave_Address+1);    //发送设备地址+写信号
	while(bh1750_i2c_wait_ack())  
	{
		retry++;
		if(retry>200) break;
	}	
	
	Data[0]=bh1750_drive_read_byte(1);  //发送ACK
	Data[1]=bh1750_drive_read_byte(0);  //发送NACK
	
	bh1750_i2c_stop();                          //停止信号
//	delay_ms(5);
	
	result=Data[0];
	result=(result<<8)+Data[1];  //合成数据，即光照数据
	
	result=(uint16_t)(result/1.2);
	
	return result;
}


/*
*********************************************************************************************************
*	函 数 名: BH1750_Init
*	功能说明: BH1750初始化
*	形    参: 无
*	返 回 值: 光照度 单位:lx
*********************************************************************************************************
*/
void BH1750_Init(void)
{
	BH1750_IIC_GPIO_init();
	
	BH1750_WriteI2C_Byte(BH1750_Addr,Power_Down);	                    //power on
	BH1750_WriteI2C_Byte(BH1750_Addr,Reset);	                        //clear
	BH1750_WriteI2C_Byte(BH1750_Addr,Continuously_HResolution_Mode);  //连续H分辨率模式，至少180ms，之后读取数据  
	delay_ms(180);  //测量一般需要120ms
}

/*
*********************************************************************************************************
*	函 数 名: bh1750_test
*	功能说明: bh1750测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bh1750_test(void)
{
	uint16_t temp;
	printf("start\r\n");
	while(1)
	{         
			temp = BH1750_ReadI2C_Data(BH1750_Addr);      //读出数据
			printf("光照强度 = %d lx\r\n",temp);
			delay_ms(500);
	}	
}

