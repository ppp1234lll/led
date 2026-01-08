#include "./DRIVER/inc/BL0906.h"
#include "bsp.h"
#include "./TASK/inc/det.h"

/*											 
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


struct bl0906_data_t {
	uint8_t auto_cmd;		// 自动采集功能
	uint8_t checksum;		// 和校验 - 主要是读使用
	uint8_t repeat;			// 重复计数
	uint8_t overtime;		// 超时倒计时 在等于1的状态下，计数清零，标识超时，result使用2
	uint8_t result;			// 结果：0-等待 1-获取到 2-超时
	uint8_t send; 			// 1-有发送数据
	uint8_t reg;				// 本次操作类型-寄存器地址			
	uint8_t mode;				// 0-读 0x80-写
	uint8_t flag ;         // 采集标志位  19:电压  10-18:电流1-9  1-9: 功率1-9
};

static uint16_t      sg_bl0906_rec_sta = 0;
static uint8_t       sg_bl0906_buff[16] = {0};
struct bl0906_data_t sg_bl0906data_t = {0};

/* 接口与参数 */
#define BL0906_BAUDRATE (4800)
#define BL0906_INIT() 							bsp_InitHSPI()
#define BL0906_SEND_STR(buff,len) 	HSPI_Send_Data(buff,len)
#define BL0906_READ_STR(tx,rx,len)  HSPI_Read_Data(tx,rx,len)
//#define BL0910_READ_STR							HSPI_ReadByte

/* 宏定义数据 */
#define BL0906_DET_NUM   			20  		// 采集次数 
#define BL0906_TIME_OUT  			100     // 超时时间 20ms
#define BL0906_AUTO_TIME   		4000 	  // 2s (采集18次，每次100ms)
#define BL0906_SEND_TIME   		50 	    // 发送时间 10ms

/* 数据 */
#define BL0906_REC_STA  sg_bl0906_rec_sta
#define BL0906_REC_BUFF sg_bl0906_buff

/*
*********************************************************************************************************
*	函 数 名: bl0906_init_function
*	功能说明: 初始化
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_init_function(void)
{
	BL0906_INIT();
	
	/* 开启自动采集功能-电压、电流 */
	sg_bl0906data_t.auto_cmd = 1;
		
	/* 写命令使能 */
	bl0906_write_enable_function(1);
	delay_ms(10);
	bl0906_set_mode_function();
	delay_ms(10);
	bl0906_reset_numreg_function();
	/* 写命令失能 */
	bl0906_write_enable_function(0);
	delay_ms(100);
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_sending_data_function
*	功能说明: 发送数据函数
*	形    参: 
*	@reg		: 寄存器值
*	@mode		: 发送模式
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_sending_data_function(uint8_t reg, uint8_t mode)
{
	sg_bl0906data_t.overtime 	= 1;  // 超时倒计时
	sg_bl0906data_t.result  	= 0;   
	sg_bl0906data_t.mode    	= mode;
	sg_bl0906data_t.send    	= 1;
	sg_bl0906data_t.reg     	= reg;
	sg_bl0906data_t.repeat  	= 0;
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_send_over_function
*	功能说明: 数据发送操作完成
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_send_over_function(void)
{
	sg_bl0906data_t.overtime = 0;
	sg_bl0906data_t.result  = 0;
	sg_bl0906data_t.send    = 0;
	sg_bl0906data_t.repeat  = 0;
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_deal_read_data_function
*	功能说明: 处理读取到的数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
int8_t bl0906_deal_read_data_function(void)
{
	uint32_t data  = 0;
	int8_t   ret   = 0;
	uint8_t  index = 0;
	
	for(index=0; index<3; index++) 
		sg_bl0906data_t.checksum += BL0906_REC_BUFF[index];

	if(BL0906_REC_BUFF[3] != (0xff - sg_bl0906data_t.checksum)) 
	{
		ret = -1;
	}
	data = BL0906_REC_BUFF[0];
	data = (data<<8) | BL0906_REC_BUFF[1];
	data = (data<<8) | BL0906_REC_BUFF[2];
	
	switch(sg_bl0906data_t.reg) 
	{
		case BL0906_I1_RMS:    		det_set_total_energy_bl0906(0,data); break;
		case BL0906_I2_RMS:    		det_set_total_energy_bl0906(2,data); break;
		case BL0906_I3_RMS:    		det_set_total_energy_bl0906(3,data); break;
		case BL0906_I4_RMS:    		det_set_total_energy_bl0906(4,data); break;
		case BL0906_I5_RMS:    		det_set_total_energy_bl0906(5,data); break;	
		case BL0906_I6_RMS:    		det_set_total_energy_bl0906(6,data); break;
		default:			break;
	}
	return ret;
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_repeat_function
*	功能说明: 重复操作函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_repeat_function(void)
{
	/* 数据等待超时 */
	if( (++sg_bl0906data_t.repeat) <= 5) 
	{
		sg_bl0906data_t.overtime = 1;	/* 重复获取或写入 */
		bl0906_read_reg_function(sg_bl0906data_t.reg,0);
	} 
	else 
		bl0906_send_over_function();		/* 结束任务 */
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_analysis_data_function
*	功能说明: 数据读取函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_analysis_data_function(void)
{
	int8_t   ret = 0;
	/* 等待回传数据 */
	if(BL0906_REC_STA&0x8000) 
	{
		if(sg_bl0906data_t.mode == 0) 		/* 数据处理 */
		{
			ret = bl0906_deal_read_data_function();		/* 读取数据 */

			if(ret != 0) 
				bl0906_repeat_function();
			else 
				bl0906_send_over_function();
		}
		BL0906_REC_STA = 0;
	}
	
	/* 检测本次操作是否超时 */
	if(sg_bl0906data_t.result == 2) 
	{
		sg_bl0906data_t.result = 0;
		bl0906_repeat_function();
	}
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_write_reg_function
*	功能说明: 写寄存器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_write_reg_function(uint8_t reg,uint8_t *data, uint8_t len,uint8_t mode)
{
	uint8_t buff[64] = {0};
	uint8_t index = 0;		
	
	buff[0] = BL0906_CMD_WRITE;
	buff[1] = reg;
	
	for(index=0; index<len; index++) 
		buff[2+index] = data[index];
	
	/* 计数和校验 */
	sg_bl0906data_t.checksum = 0;
	for(index=0; index<(2+len); index++)
		sg_bl0906data_t.checksum+=buff[index];
	
	/* 填充和校验 */
	buff[2+len] = 0xff - sg_bl0906data_t.checksum;
	
	if( mode == 0) 	/* 更新标志 */
		bl0906_sending_data_function(reg,0x80);
	
	/* 数据发送 */
	BL0906_SEND_STR(buff,3+len);
}
/*
*********************************************************************************************************
*	函 数 名: bl0906_read_reg_function
*	功能说明: 寄存器读取命令
*	形    参: 
*	@reg		: 寄存器值
*	@mode		: 0-更新标志 other-不更新
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_read_reg_function(uint8_t reg, uint8_t mode)
{
	uint8_t buff[6] = {0};

	buff[0] = BL0906_CMD_READ;	/* 数据 */
	buff[1] = reg;	/* 数据 */

	sg_bl0906data_t.checksum = buff[0]+buff[1];   /* 计数和校验 */
	
	if( mode == 0) 	/* 更新标志 */
		bl0906_sending_data_function(reg,0);
	
//	BL0910_SEND_STR(buff,sizeof(buff));	/* 数据发送 */
//	for(index=0; index < 4; index++) {
//		BL0910_REC_BUFF[index] = HSPI_ReadByte();
//	}

	HSPI_Read_Data(buff,BL0906_REC_BUFF,6);
	BL0906_REC_BUFF[0] = BL0906_REC_BUFF[2];
	BL0906_REC_BUFF[1] = BL0906_REC_BUFF[3];
	BL0906_REC_BUFF[2] = BL0906_REC_BUFF[4];
	BL0906_REC_BUFF[3] = BL0906_REC_BUFF[5];
	
	BL0906_REC_STA = 0x8000+4;
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_send_data_function
*	功能说明: 数据发送函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_send_data_function(void)
{
	/* 允许进行发送操作 */
	if(sg_bl0906data_t.send == 0)
	{
		if(sg_bl0906data_t.flag > 0)
		{
			switch(sg_bl0906data_t.flag)
			{
				case 1: 	bl0906_read_reg_function(BL0906_I1_RMS,0);   break;   
				case 2: 	bl0906_read_reg_function(BL0906_I2_RMS,0);   break; // 电流 3
				case 3: 	bl0906_read_reg_function(BL0906_I3_RMS,0);   break; // 电流 4
				case 4: 	bl0906_read_reg_function(BL0906_I4_RMS,0);   break; // 电流 4
				case 5: 	bl0906_read_reg_function(BL0906_I5_RMS,0);   break; // 电流 3
				case 6: 	bl0906_read_reg_function(BL0906_I6_RMS,0);   break; // 电流 2
				default: break;			
			}
			sg_bl0906data_t.flag--;
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_run_timer_function
*	功能说明: 运行计时相关函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_run_timer_function(void)
{
	static uint16_t  times = 0;
	static uint16_t  auto_time = 0;

	if(sg_bl0906data_t.overtime != 0) 	/* 计数更新 */
	{
		if(sg_bl0906data_t.overtime == 1) 
		{
			sg_bl0906data_t.overtime = 2;
			times = 0;
		}
		/* 计数值 */
		if((++times) >= BL0906_TIME_OUT)  // 超时时间
		{
			times = 0;
			sg_bl0906data_t.overtime = 0;
			sg_bl0906data_t.result = 2;
		}
	} 
	else 
		times = 0;
	
	/* 自动采集 */
	if(sg_bl0906data_t.auto_cmd == 1) 
	{
		if((++auto_time) >= BL0906_AUTO_TIME) 
		{
			auto_time = 0;
			sg_bl0906data_t.flag = BL0906_DET_NUM;
			sg_bl0906data_t.send = 0;
		}
	} 
	else 
		auto_time = 0;
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_work_process_function
*	功能说明: 工作进程函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_work_process_function(void)
{
	bl0906_analysis_data_function();
	bl0906_send_data_function();
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_get_rec_data_function
*	功能说明: 获取通信数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_get_rec_data_function(uint8_t *buff, uint16_t len)
{
	uint16_t index = 0;

	if(buff == NULL || len == 0) {
		return;
	}
	
	for(index=0; index<len; index++) {
		BL0906_REC_BUFF[index] = buff[index];
	}
	
	sg_bl0906_rec_sta = 0x8000|len;
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_write_enable_function
*	功能说明: 写使能控制函数
*	形    参: 
*	@cmd		: 0-失能 1-使能
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_write_enable_function(uint8_t cmd)
{
	uint8_t buff[3] = {0};
	
	if(cmd == 1) 
	{
		buff[0] = 0x00;
		buff[1] = 0x55;
		buff[2] = 0x55;
		bl0906_write_reg_function(BL0906_US_WRPROT,buff,3,0);
	} 
	else 
	{
		buff[0] = 0x00;
		buff[1] = 0x00;
		buff[2] = 0x00;
		bl0906_write_reg_function(BL0906_US_WRPROT,buff,3,0);
	}
}
/*
*********************************************************************************************************
*	函 数 名: bl0906_set_gain_function
*	功能说明: 设置增益寄存器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_set_gain_function(void)
{
	uint8_t gain1_buff[3] = {0};
	uint8_t gain2_buff[3] = {0};
	
	gain1_buff[0] = BL0906_GAIN_1<<4 | BL0906_GAIN_1;  // 通道1 电压通道
	gain1_buff[1] = BL0906_GAIN_1<<4 | BL0906_GAIN_1;  // 通道3 通道2
	gain1_buff[2] = BL0906_GAIN_1<<4 | BL0906_GAIN_1;  // 通道5 通道4

	gain2_buff[0] = BL0906_GAIN_1<<4 | BL0906_GAIN_1;  // 通道7 通道6
	gain2_buff[1] = BL0906_GAIN_1<<4 | BL0906_GAIN_1;  // 通道9 通道8
	gain2_buff[2] = BL0906_GAIN_1;  						 // 通道10
	
	bl0906_write_reg_function(BL0906_GAIN1_REG,gain1_buff,3,0);
	delay_ms(50);
	bl0906_write_reg_function(BL0906_GAIN2_REG,gain2_buff,3,0);

}
/*
*********************************************************************************************************
*	函 数 名: bl0906_set_ch_function
*	功能说明: 设置通道寄存器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_set_ch_function(void)
{
	uint8_t ch_buff[3] = {0};
	
	ch_buff[0] = 0x00;  // 全部使用
	ch_buff[1] = 0x04;  // 关闭通道10
	ch_buff[2] = 0x00;  // 关闭通道10	
	bl0906_write_reg_function(BL0906_ADC_PD,ch_buff,3,0);
}
/*
*********************************************************************************************************
*	函 数 名: bl0906_set_mode_function
*	功能说明: 设置模式
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_set_mode_function(void)
{
	uint8_t mode_buff[3] = {0};
	
	mode_buff[0] = 0x00;  // 
	mode_buff[1] = 0x02;  // 打开cf
	mode_buff[2] = 0x00;  // 	
	bl0906_write_reg_function(BL0906_MODE3,mode_buff,3,0);
}
/*
*********************************************************************************************************
*	函 数 名: bl0906_set_eng_rst_function
*	功能说明: 能量读后清零
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_set_eng_rst_function(void)
{
	uint8_t ch_buff[3] = {0};
	
	ch_buff[0] = 0x00;  // 全部使用
	ch_buff[1] = 0x00;  
	ch_buff[2] = 0x00; 
	bl0906_write_reg_function(BL0906_RST_ENG,ch_buff,3,0);
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_reset_numreg_function
*	功能说明: 复位数字部分的状态机和寄存器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_reset_numreg_function(void)
{
	uint8_t ch_buff[3] = {0};
	
	ch_buff[0] = 0x5A;  // 全部使用
	ch_buff[1] = 0x5A;  
	ch_buff[2] = 0x5A; 
	bl0906_write_reg_function(BL0906_SOFT_RESET,ch_buff,3,0);
}

/*
*********************************************************************************************************
*	函 数 名: bl0906_test
*	功能说明: 电压、电流测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0906_test(void)
{
	bl0906_read_reg_function(BL0906_VAR_CREEP,0);  // 0x04C04C
	delay_ms(20);	
	bl0906_read_reg_function(BL0906_RMS_CREEP,0);  // 0x200
	delay_ms(20);	
	bl0906_read_reg_function(BL0906_FAST_RMS_CTRL,0);  // 0x20FFFF
	delay_ms(20);
	bl0906_read_reg_function(BL0906_SAGLVL,0);  // 0x100009 0x000000
	delay_ms(20);	

	while(1)
	{
	  bl0906_read_reg_function(BL0906_VAR_CREEP,0);  // 默认值是0x1010

//		bl0906_work_process_function();	// 数据获取函数
//		delay_ms(20);		
	}
}









