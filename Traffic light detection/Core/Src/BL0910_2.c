#include "BL0910_2.h"
//#include "delay.h"
//#include "det.h"
#include "spi.h"
#include <stdio.h>

/*
	8、BL0906单相计量芯片:(硬件SPI方式)，引脚分配为：
		MOSI:    PC12
		MISO:    PC11
		CLK:     PC10
		CS1:     PD0
		CS2:     PD1
		CS3:     PE2
		CS4:     PE3
		
									 
实际电压值(V) = [电压有效值寄存器值*Vref*(R25+R26+R35+R36+R37)]/[13162*Gain_V*R46*1000]

电压系数Kv  = [13162*Gain_V*R46*1000]/[Vref*(R25+R26+R35+R36+R37)]
(去除1000) = [13162*1*(51+51)*1000]/[1.097*(20+20+20+20+20)]
					 = 1342524 / 109.7
					 = 12238.14


实际电流值(A) = [电流有效值寄存器值*Vref] / [12875*Gain_I*(R1+R2)*1000/Rt]

电流系数 Ki = [12875*Gain_I*(R1+R2)*1000/Rt] / Vref
						= [12875*1*(51+51)*1000/1000] / 1.097
						= 1197128.53
						= 1197.128 (mA)

实际有功功率值(W) = [有功功率寄存器值*Vref*Vref*(R25+R26+R35+R36+R37)]/
										[40.4125*((R1+R2)*1000/Rt*Gain_I*R46*Gain_V*1000]
功率系数Kp = [40.4125*((R1+R2)*1000/Rt*Gain_I*R46*Gain_V*1000]/[Vref*Vref*(R25+R26+R35+R36+R37)]
           = [40.4125*((51+51)*1000/1000*1*(51+51)*1*1000]/[1.097*1.097*(20+20+20+20+20)]     
					 = 420451.65 / 120.3409
					 = 3493.84
					 
每个电能脉冲对应的电量 = [4194304*0.032768*16]/
												 [3600000*CFDIV*Kp]
功率系数Ke = [4194304*0.032768*16]/[3600000*16*Kp]
           = [4194304*0.032768*16]/[3600000*16*3493.84]  
					 = 0.00001093	 
*/


struct bl0910_2_data_t {
	uint8_t auto_cmd;		// 自动采集功能
	uint8_t checksum;		// 和校验 - 主要是读使用
	uint8_t repeat;			// 重复计数
	uint8_t overtime;		// 超时倒计时 在等于1的状态下，计数清零，标识超时，result使用2
	uint8_t result;			// 结果：0-等待 1-获取到 2-超时
	uint8_t send; 			// 1-有发送数据
	uint8_t reg;				// 本次操作类型-寄存器地址			
	uint8_t mode;				// 0-读 0x80-写
	uint8_t flag;    // 
};

static uint16_t sg_bl0910_2_rec_sta = 0;
static uint8_t  sg_bl0910_2_buff[64] = {0};
struct bl0910_2_data_t sg_bl0910_2_data_t = {0};

/* 接口与参数 */
#define BL0910_BAUDRATE (4800)
//#define BL0910_INIT() 							elec_hard_spi_init()
#define BL0910_2_SEND_STR(buff,len) 	hardSPI_2_Write_Multi_Byte(buff,len)

/* 宏定义数据 */
#define BL0910_2_DET_NUM   			10  		// 采集次数 
#define BL0910_2_TIME_OUT  			20 		// 超时时间 20ms
#define BL0910_2_AUTO_TIME   		200 	  // 2s (采集18次，每次100ms)
#define BL0910_SEND_TIME   		10 	    // 发送时间 10ms
/* 数据 */
#define BL0910_2_REC_STA  sg_bl0910_2_rec_sta
#define BL0910_2_REC_BUFF sg_bl0910_2_buff

//#define BL0906_4_CS	  PEout(3)
///************************************************************
//*
//* Function name	: bl0910_cs_init_function
//* Description	: CS控制脚初始化函数
//* Parameter		:
//* Return		:
//*
//************************************************************/
//void bl0910_cs_init_function(void)
//{
//	GPIO_InitTypeDef GPIO_InitStructure;

//	RCC_AHB1PeriphClockCmd(BL0906_1_GPIO_CLK|BL0906_2_GPIO_CLK,ENABLE);

//	GPIO_InitStructure.GPIO_Pin   = BL0906_1_PIN;
//	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(BL0906_1_GPIO,&GPIO_InitStructure); 	
//	
//	GPIO_InitStructure.GPIO_Pin   = BL0906_2_PIN;
//	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
//	GPIO_Init(BL0906_2_GPIO,&GPIO_InitStructure); 	
//	
////	GPIO_InitStructure.GPIO_Pin   = BL0906_3_PIN;
////	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
////	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
////	GPIO_Init(BL0906_3_GPIO,&GPIO_InitStructure); 	
////	
////	GPIO_InitStructure.GPIO_Pin   = BL0906_4_PIN;
////	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
////	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
////	GPIO_Init(BL0906_4_GPIO,&GPIO_InitStructure); 	
////	
//	BL0906_1_CS = 1;
//	BL0906_2_CS = 1;
//	BL0906_3_CS = 1;
////	BL0906_4_CS = 1;
//}

/************************************************************
*
* Function name	: bl0910_2_init_function
* Description	: 初始化                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_init_function(void)
{
//	bl0910_cs_init_function();
//	BL0910_INIT();
	
	/* 开启自动采集功能-电压、电流 */
	sg_bl0910_2_data_t.auto_cmd = 1;
	
		/* 写命令使能 */
	bl0910_2_write_enable_function(1);
	HAL_Delay(10);	
	bl0910_2_set_gain_function();  // 设置增益
	HAL_Delay(10);	
//	bl0910_2_set_ch_function();    // 设置通道
//	HAL_Delay(10);	
	bl0910_2_set_mode_function();
//	bl0910_2_set_eng_rst_function();
//	HAL_Delay(10);
	/* 写命令失能 */
	bl0910_2_write_enable_function(0);
	HAL_Delay(100);
	
		
//	/* 写命令使能 */
//	bl0910_2_write_enable_function(1);
//	delay_ms(10);
////	bl0910_2_set_gain_function();  // 设置增益
////	delay_ms(10);
////	bl0910_2_set_ch_function();
////	delay_ms(10);	
//	bl0910_2_set_mode_function();
////	bl0910_2_set_eng_rst_function();
//	delay_ms(10);
//	bl0910_2_reset_numreg_function();
//	/* 写命令失能 */
//	bl0910_2_write_enable_function(0);
//	delay_ms(100);
}

/************************************************************
*
* Function name	: bl0910_2_sending_data_function
* Description	: 发送数据函数
* Parameter		: 
*	@reg		: 寄存器值
*	@mode		: 发送模式
* Return		: 
*	
************************************************************/
void bl0910_2_sending_data_function(uint8_t reg, uint8_t mode)
{
	sg_bl0910_2_data_t.overtime 	= 1;  // 超时倒计时
	sg_bl0910_2_data_t.result  	= 0;   
	sg_bl0910_2_data_t.mode    	= mode;
	sg_bl0910_2_data_t.send    	= 1;
	sg_bl0910_2_data_t.reg     	= reg;
	sg_bl0910_2_data_t.repeat  	= 0;
}

/************************************************************
*
* Function name	: bl0910_2_send_over_function
* Description	: 数据发送操作完成
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_send_over_function(void)
{
	sg_bl0910_2_data_t.overtime = 0;
	sg_bl0910_2_data_t.result  = 0;
	sg_bl0910_2_data_t.send    = 0;
	sg_bl0910_2_data_t.repeat  = 0;
}

///************************************************************
//*
//* Function name	: Complement_2_Original
//* Description	: 补码转换为原码
//* Parameter		: 
//* Return		: 
//*	
//************************************************************/
//uint32_t Complement_2_Original(uint32_t data)
//{
//	uint32_t temp;
//	if((data&0x00800000) == 0x00800000)  // 判断最高位是否为0，Bit[23]为符号位，Bit[23]=0为正
//	{
//		data &= 0x007FFFFF;  // 清除符号位 	
//		temp =~data;         // 反码
//		data = temp & 0x007FFFFF;  // 清除左边多余位
//		data += 1;				
//	}
//	else  // 当前为负功
//	{
//		data &= 0x007FFFFF;  // 清除符号位
//	}
//	return data;
//}

/************************************************************
*
* Function name	: bl0910_2_deal_read_data_function
* Description	: 处理读取到的数据
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t bl0910_2_deal_read_data_function(void)
{
	uint32_t data  = 0;
	int8_t   ret   = 0;
	uint8_t  index = 0;
	
	for(index=0; index<3; index++) 
		sg_bl0910_2_data_t.checksum += BL0910_2_REC_BUFF[index];

	if(BL0910_2_REC_BUFF[3] != (0xff - sg_bl0910_2_data_t.checksum)) 
	{
		ret = -1;
	}
	data = BL0910_2_REC_BUFF[0];
	data = (data<<8) | BL0910_2_REC_BUFF[1];
	data = (data<<8) | BL0910_2_REC_BUFF[2];
	
	//printf ("%d\r\n",data);
	
	switch(sg_bl0910_2_data_t.reg) 
	{
		case I10_RMS:  det_set_total_energy(11,data); break;
		case I9_RMS:   det_set_total_energy(12,data); break;
		case I8_RMS:   det_set_total_energy(13,data); break;
		case I7_RMS:   det_set_total_energy(14,data); break;
		case I6_RMS:   det_set_total_energy(15,data); break;
		case I5_RMS:   det_set_total_energy(16,data); break;
		case I4_RMS:   det_set_total_energy(17,data); break;
		case I3_RMS:   det_set_total_energy(18,data); break;
		case I2_RMS:   det_set_total_energy(19,data); break;
		case I1_RMS:   det_set_total_energy(20,data); break;
		default:			break;
	}
	return ret;
}

/************************************************************
*
* Function name	: bl0910_2_repeat_function
* Description	: 重复操作函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_repeat_function(void)
{
	/* 数据等待超时 */
	if( (++sg_bl0910_2_data_t.repeat) <= 5) 
	{
		sg_bl0910_2_data_t.overtime = 1;	/* 重复获取或写入 */
		bl0910_2_read_reg_function(sg_bl0910_2_data_t.reg,0);
	} 
	else 
		bl0910_2_send_over_function();		/* 结束任务 */
}

/************************************************************
*
* Function name	: bl0910_2_analysis_data_function
* Description	: 数据读取函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_analysis_data_function(void)
{
	int8_t   ret = 0;
	/* 等待回传数据 */
	if(BL0910_2_REC_STA&0x8000) 
	{
		if(sg_bl0910_2_data_t.mode == 0) 		/* 数据处理 */
		{
			ret = bl0910_2_deal_read_data_function();		/* 读取数据 */
			
//			printf("%d\r\n",ret);
			if(ret != 0) 
				bl0910_2_repeat_function();
			else 
				bl0910_2_send_over_function();
		}
		BL0910_2_REC_STA = 0;
	}
	
	/* 检测本次操作是否超时 */
	if(sg_bl0910_2_data_t.result == 2) 
	{
		sg_bl0910_2_data_t.result = 0;
		bl0910_2_repeat_function();
	}
}

/************************************************************
*
* Function name	: bl0910_2_write_reg_function
* Description	: 写寄存器
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_write_reg_function(uint8_t reg,uint8_t *data, uint8_t len,uint8_t mode)
{
	uint8_t buff[64] = {0};
	uint8_t index = 0;		
	
	buff[0] = CMD_WRITE;
	buff[1] = reg;
	
	for(index=0; index<len; index++) 
		buff[2+index] = data[index];
	
	/* 计数和校验 */
	sg_bl0910_2_data_t.checksum = 0;
	for(index=0; index<(2+len); index++)
		sg_bl0910_2_data_t.checksum+=buff[index];
	
	/* 填充和校验 */
	buff[2+len] = 0xff - sg_bl0910_2_data_t.checksum;
	
	if( mode == 0) 	/* 更新标志 */
		bl0910_2_sending_data_function(reg,0x80);
	
	/* 数据发送 */
	BL0910_2_SEND_STR(buff,3+len);
}
/************************************************************
*
* Function name	: bl0910_2_read_reg_function
* Description	: 寄存器读取命令
* Parameter		: 
*	@reg		: 寄存器值
*	@mode		: 0-更新标志 other-不更新
* Return		: 
*	
************************************************************/
void bl0910_2_read_reg_function(uint8_t reg, uint8_t mode)
{
	uint8_t buff[2] = {0};
	uint16_t index = 0;
	buff[0] = CMD_READ;	/* 数据 */
	buff[1] = reg;	/* 数据 */

	sg_bl0910_2_data_t.checksum = buff[0]+buff[1];   /* 计数和校验 */
	
	if( mode == 0) 	/* 更新标志 */
		bl0910_2_sending_data_function(reg,0);
	
	BL0910_2_SEND_STR(buff,sizeof(buff));	/* 数据发送 */
	//hardSPI_ReadWriteByte(0x00);
	for(index=0; index < 4; index++) {
		BL0910_2_REC_BUFF[index] = hardSPI_2_ReadByte();
	}
	BL0910_2_REC_STA = 0x8000+4;
}

/************************************************************
*
* Function name	: bl0910_2_send_data_function
* Description	: 数据发送函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_send_data_function(void)
{
	/* 允许进行发送操作 */
	if(sg_bl0910_2_data_t.send == 0)
	{
		if(sg_bl0910_2_data_t.flag > 0)
		{
			switch(sg_bl0910_2_data_t.flag)
			{
				case 1: 	bl0910_2_read_reg_function(I10_RMS,0); break; // 电流 
				case 2: 	bl0910_2_read_reg_function(I9_RMS,0); break; // 电流 
				case 3: 	bl0910_2_read_reg_function(I8_RMS,0); break; // 电流 
				case 4: 	bl0910_2_read_reg_function(I7_RMS,0); break; // 电流 
				case 5: 	bl0910_2_read_reg_function(I6_RMS,0); break; // 电流			
				case 6: 	bl0910_2_read_reg_function(I5_RMS,0); break; // 电流 
				case 7: 	bl0910_2_read_reg_function(I4_RMS,0); break; // 电流 
				case 8: 	bl0910_2_read_reg_function(I3_RMS,0); break; // 电流 
				case 9: 	bl0910_2_read_reg_function(I2_RMS,0); break; // 电流 
        case 10: 	bl0910_2_read_reg_function(I1_RMS,0); break; // 电流 				
				default: break;			
			}
			sg_bl0910_2_data_t.flag--;
		}
	}
}

/************************************************************
*
* Function name	: bl0910_2_run_timer_function
* Description	: 运行计时相关函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_run_timer_function(void)
{
	static uint16_t  times = 0;
	static uint16_t  auto_time = 0;
	
	if(sg_bl0910_2_data_t.overtime != 0) 	/* 计数更新 */
	{
		if(sg_bl0910_2_data_t.overtime == 1) 
		{
			sg_bl0910_2_data_t.overtime = 2;
			times = 0;
		}
		/* 计数值 */
		if((++times) >= BL0910_2_TIME_OUT)  // 超时时间
		{
			times = 0;
			sg_bl0910_2_data_t.overtime = 0;
			sg_bl0910_2_data_t.result = 2;
		}
	} 
	else 
		times = 0;
	
	/* 自动采集 */
	if(sg_bl0910_2_data_t.auto_cmd == 1) 
	{
		if((++auto_time) >= BL0910_2_AUTO_TIME) 
		{
			auto_time = 0;
			sg_bl0910_2_data_t.flag = BL0910_2_DET_NUM;
			sg_bl0910_2_data_t.send = 0;
		}
	} 
	else 
		auto_time = 0;;
}

/************************************************************
*
* Function name	: bl0910_2_work_process_function
* Description	: 工作进程函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_work_process_function(void)
{
	bl0910_2_analysis_data_function();
	bl0910_2_send_data_function();
}

/************************************************************
*
* Function name	: bl0910_2_get_rec_data_function
* Description	: 获取通信数据
* Parameter		: 
*	@
* Return: 
*	
************************************************************/
void bl0910_2_get_rec_data_function(uint8_t *buff, uint16_t len)
{
	uint16_t index = 0;

	if(buff == NULL || len == 0) {
		return;
	}
	
	for(index=0; index<len; index++) {
		BL0910_2_REC_BUFF[index] = buff[index];
	}
	
	sg_bl0910_2_rec_sta = 0x8000|len;
}

/************************************************************
*
* Function name	: bl0910_2_write_enable_function
* Description	: 写使能控制函数
* Parameter		: 
*	@cmd		: 0-失能 1-使能
* Return		: 
*	
************************************************************/
void bl0910_2_write_enable_function(uint8_t cmd)
{
	uint8_t buff[3] = {0};
	
	if(cmd == 1) 
	{
		buff[0] = 0x00;
		buff[1] = 0x55;
		buff[2] = 0x55;
		bl0910_2_write_reg_function(US_WRPROT_REG,buff,3,0);
	} 
	else 
	{
		buff[0] = 0x00;
		buff[1] = 0x00;
		buff[2] = 0x00;
		bl0910_2_write_reg_function(US_WRPROT_REG,buff,3,0);
	}
}

/************************************************************
*
* Function name	: bl0910_2_set_gain_function
* Description	: 设置增益寄存器
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_set_gain_function(void)
{
	uint8_t gain1_buff[3] = {0};
	uint8_t gain2_buff[3] = {0};
	
	gain1_buff[0] = GAIN_1<<4 | GAIN_1;  // 通道1 电压通道
	gain1_buff[1] = GAIN_1<<4 | GAIN_1;  // 通道3 通道2
	gain1_buff[2] = GAIN_1<<4 | GAIN_1;  // 通道5 通道4

	gain2_buff[0] = GAIN_1<<4 | GAIN_1;  // 通道7 通道6
	gain2_buff[1] = GAIN_1<<4 | GAIN_1;  // 通道9 通道8
	gain2_buff[2] = GAIN_1;  						 // 通道10
	
	bl0910_2_write_reg_function(GAIN1_REG,gain1_buff,3,0);
	HAL_Delay(50);
	bl0910_2_write_reg_function(GAIN2_REG,gain2_buff,3,0);

}

/************************************************************
*
* Function name	: bl0910_2_set_ch_function
* Description	: 设置通道寄存器
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_set_ch_function(void)
{
	uint8_t ch_buff[3] = {0};
	
	ch_buff[0] = 0x00;  // 全部使用
	ch_buff[1] = 0x04;  // 关闭通道10
	ch_buff[2] = 0x00;  // 关闭通道10	
	bl0910_2_write_reg_function(ADC_PD_CTRL,ch_buff,3,0);
}


/************************************************************
*
* Function name	: bl0910_2_set_mode_function
* Description	: 设置模式
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_set_mode_function(void)
{
	uint8_t mode_buff[3] = {0};
	
	mode_buff[0] = 0x00;  // 
	mode_buff[1] = 0x02;  // 打开cf
	mode_buff[2] = 0x00;  // 	
	bl0910_2_write_reg_function(MODE3_REG,mode_buff,3,0);
}


/************************************************************
*
* Function name	: bl0910_2_set_eng_rst_function
* Description	: 能量读后清零
* Parameter		: 
* Return		: 
*	
************************************************************/
void bl0910_2_set_eng_rst_function(void)
{
	uint8_t ch_buff[3] = {0};
	
	ch_buff[0] = 0x00;  // 全部使用
	ch_buff[1] = 0x00;  
	ch_buff[2] = 0x00; 
	bl0910_2_write_reg_function(RST_ENG,ch_buff,3,0);
}
/************************************************************
*
* Function name	: bl0910_2_reset_numreg_function
* Description	: 复位数字部分的状态机和寄存器
* Parameter		: 
* Return		: 
*	 5A5A5A
************************************************************/
void bl0910_2_reset_numreg_function(void)
{
	uint8_t ch_buff[3] = {0};
	
	ch_buff[0] = 0x5A;  // 全部使用
	ch_buff[1] = 0x5A;  
	ch_buff[2] = 0x5A; 
	bl0910_2_write_reg_function(SOFT_RESET_REG,ch_buff,3,0);
}

/************************************************************
*
* Function name	: bl0910_test
* Description	: 电压、电流测试
* Parameter		:
* Return		:
*
************************************************************/
void bl0910_2_test(void)
{
//	bl0910_2_read_reg_function(TPS_CTRL,0);  // 默认值是0x07FF
//	delay_ms(200);	
//	bl0910_2_read_reg_function(SAGLVL_LINECYC,0);  // 默认值是0x100009
//	delay_ms(200);	
//	bl0910_2_read_reg_function(ADC_PD_CTRL,0);  // 默认值是0x100009
//	delay_ms(200);
//	bl0910_2_read_reg_function(GAIN1_REG,0);  // 默认值是0x100009
//	delay_ms(200);	
//	bl0910_2_read_reg_function(GAIN2_REG,0);  // 默认值是0x100009
//	delay_ms(200);	
//	bl0910_2_read_reg_function(RST_ENG,0);  // 默认值是0x100009
//	delay_ms(200);		
//	bl0910_2_read_reg_function(MODE3_REG,0);  // 默认值是0x100009
//	delay_ms(200);
	while(1)
	{
		bl0910_2_read_reg_function(0x94,0);
		bl0910_2_analysis_data_function();
//		bl0910_2_work_process_function();	// 数据获取函数
		HAL_Delay(2000);		
	}

}









