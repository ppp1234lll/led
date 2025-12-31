/********************************************************************************
* @File name  : RTC模块
* @Description: 模拟IIC通信
* @Author     : ZHLE
*  Version Date        Modification Description
		
	14、RTC时钟芯片： 硬件IIC3，引脚分配为： 
	      RTC_SCL：    PA8
        RTC_SDA：    PC9	

********************************************************************************/


#include "pcf8563.h"
#include "bsp.h" 

//脉冲频率：32.768KHZ=0x80,1024HZ=0x81,32HZ=0x82,1HZ=0x83,0x00为关闭脉冲
uint8_t clkout_frequency[5] = {0x80,0x81,0x82,0x83,0x00};

/**
  * @brief   写数据到PCF8563寄存器
  * @param   
  * @retval  
  */
void PCF8563_WriteReg(uint8_t reg_add,uint8_t reg_dat)
{
	iic_start();
	iic_send_byte(PCF8563_SLAVE_ADDRESS);
	iic_wait_ack();
	iic_send_byte(reg_add);
	iic_wait_ack();
	iic_send_byte(reg_dat);
	iic_wait_ack();
	iic_stop();
}

/**
  * @brief   从PCF8563寄存器读取数据
  * @param   
  * @retval  
  */
void PCF8563_ReadData(uint8_t reg_add,unsigned char *Read,uint8_t num)
{
	unsigned char i;
	
	iic_start();
	iic_send_byte(PCF8563_SLAVE_ADDRESS);    
	iic_wait_ack();
    
	iic_send_byte(reg_add);	
  iic_wait_ack();
	
	iic_start();
	iic_send_byte(PCF8563_SLAVE_ADDRESS+1);	
  iic_wait_ack();
	
	for(i=0;i<(num-1);i++){
		*Read=iic_read_byte(1);
		Read++;
	}
	*Read=iic_read_byte(0);
	iic_stop();
}


/**
  * @brief   初始化PCF8563芯片
  * @param   无
  * @retval  无
  */
void PCF8563_Init(void)
{
    uint8_t buf[4];
    iic_init();
	
    PCF8563_ReadData(0x05,buf,4);
    //年月日星期均为0时，执行初始化，初始化后不重复初始化
    if(buf[0] == 0 && buf[1] == 0 &&buf[2] == 0 &&buf[3] == 0 )
    {
        PCF8563_WriteReg(PCF8563_CONTROL_STATUS_1_REG,0x00);//控制状态寄存器0
        PCF8563_WriteReg(PCF8563_CONTROL_STATUS_2_REG,0x00);//控制状态寄存器1
        PCF8563_WriteReg(PCF8563_VL_SECONDS_REG,SECONDS);//秒
        PCF8563_WriteReg(PCF8563_MINUTES_REG,MINUTES);//分
        PCF8563_WriteReg(PCF8563_HOURS_REG,HOURS);//小时
        PCF8563_WriteReg(PCF8563_DAYS_REG,DAYS);//日
        PCF8563_WriteReg(PCF8563_WEEK_REG,WEEK);//星期
        PCF8563_WriteReg(PCF8563_MONTHS_CENTURY_REG,MONTHS_CENTURY);//月/世纪
        PCF8563_WriteReg(PCF8563_YEARS_REG,YEARS);//年          
    }
    
    PCF8563_WriteReg(PCF8563_CLKOUT_FREQUENCY_REG,0x00);//关闭脉冲
    PCF8563_WriteReg(PCF8563_TIMER_CONTROL_REG,0x03);//倒计数定时器无效
}

/**
  * @brief   设置倒计时的数值
  * @param   倒计时数值，单位秒
  * @retval  无
  */
void PCF8563_Countdown(uint8_t value)
{

    PCF8563_WriteReg(PCF8563_CONTROL_STATUS_2_REG,0x01);//倒计数定时器中断使能
    PCF8563_WriteReg(PCF8563_TIMER_CONTROL_REG,0x82);//倒计数定时器有效，并设置为1HZ时钟频率
    PCF8563_WriteReg(PCF8563_TIMER_COUNTDOWN_REG,value);//设置倒计数数值
    
}

/**
  * @brief   倒计数定时器标志清除并中断失能
  * @param   无
  * @retval  无
  */
void PCF8563_Clearflag(void)
{

    PCF8563_WriteReg(PCF8563_CONTROL_STATUS_2_REG,0x00);//倒计数定时器标志清除并中断失能
    PCF8563_WriteReg(PCF8563_TIMER_CONTROL_REG,0x03);//倒计数定时器无效
    
}
/**
  * @brief   十进制转BCD码
  * @param   十进制数值
  * @retval  BCD码转换结果
  */
uint8_t PCF8563_dec2bcd(uint8_t value)
{
    return (((value / 10) << 4) | (value % 10));
}

/**
  * @brief   BCD码转十进制
  * @param   BCD码数值
  * @retval  十进制转换结果
  */
uint8_t PCF8563_bcd2dec(uint8_t value)
{
    return (((value >> 4) * 10) + (value & 0x0f));
}

/*
*********************************************************************************************************
*	函 数 名: PCF8563_Test
*	功能说明: RTC时间读取测试函数（循环打印当前时间）
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void PCF8563_Test(void)
{
	uint8_t buf[7];//数据缓存数组
	while(1)
	{
		PCF8563_ReadData(PCF8563_VL_SECONDS_REG,buf,7);
		// 打印格式：日期-月份-年份，星期，时间（时:分:秒）
		printf("%x年%x月%x日 %x小时%x分%x秒 星期%x\r\n",buf[6],buf[5],buf[3],buf[2],buf[1],buf[0],buf[4]);
		delay_ms(1000);  // 每秒刷新一次
	}
}





