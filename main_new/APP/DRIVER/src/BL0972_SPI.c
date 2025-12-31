#include "BL0972.h"
#include "bsp.h"
#include "det.h"

/*									 
实际电压值(V) = [电压有效值寄存器值*Vref*(R11+R12+R13+R14+R15)]/[79931*R17*1000]

电压系数Kv  = [79931*R17*1000]/[Vref*(R11+R12+R13+R14+R15)]
(K 欧)     = [79931*0.0249*1000]/[1.218*(20+20+20+20+20)]
					 = 1990281.9 / 121.8
					 = 16340.534


实际电流值(A) = [电流有效值寄存器值*Vref] / [324004*(R5*1000)/Rt]
               Rt=1000

电流系数 Ki = [324004*R5*1000/Rt] / Vref
						= [324004*2.2*1000/1000] / 1.218
						= 585228.900
						= 585.229 (mA)

实际有功功率值(W) = [有功功率寄存器值*Vref*Vref*(R11+R12+R13+R14+R15)]/
										[4046*(R5*1000/Rt*R17*1000]
功率系数Kp = [4046*(R5*1000/Rt*R17*1000]/[Vref*Vref*(R25+R26+R35+R36+R37)]
           = [4046*(2.2*1000/1000*24.9*1000]/[1.218*1.218*(20+20+20+20+20)]  
					 = [4046*(2.2*0.0249*1000]/[1.218*1.218*(20+20+20+20+20)]  
					 = 221639.88 / 148.3524
					 = 1494.009
					 
每个电能脉冲对应的电量 = [1638.4*256*Vref*Vref*(R11+R12+R13+R14+R15)]/
												[3600000*4046*(R5*1000/Rt*R17*1000]
功率系数Ke = [1638.4*256*Vref*Vref*(R11+R12+R13+R14+R15)]/[3600000*4046*(R5*1000/Rt*R17*1000]
           = [1638.4*256*1.218*1.218*(20*5)]/[3600000*4046*(2.2*1000/1000*0.0249*1000]
           = [1638.4*256*1.218*1.218*100]/[3600000*4046*(2.2*24.9)]
           = 62223506.47296	/	797903568000
					 = 0.000078
*/


struct bl0972_data_t {
	uint8_t auto_cmd;		// 自动采集功能
	uint8_t checksum;		// 和校验 - 主要是读使用
	uint8_t repeat;			// 重复计数
	uint8_t overtime;		// 超时倒计时 在等于1的状态下，计数清零，标识超时，result使用2
	uint8_t result;			// 结果：0-等待 1-获取到 2-超时
	uint8_t send; 			// 1-有发送数据
	uint8_t reg;				// 本次操作类型-寄存器地址			
	uint8_t mode;				// 0-读 0x80-写
	uint8_t flag;  			// 采集标志位  19:电压  10-18:电流1-9  1-9: 功率1-9
};

static uint16_t sg_bl0972_rec_sta = 0;
static uint8_t  sg_bl0972_buff[8] = {0};
struct bl0972_data_t sg_bl0972data_t = {0};

/* 接口与参数 */
#define bl0972_INIT() 							bsp_InitSSPI_BL0972()
#define bl0972_SEND_STR(buff,len) 	SSPI1_Write_Multi_Byte(buff,len)

/* 宏定义数据 */
#define bl0972_DET_NUM   			4  		  // 采集次数 
#define bl0972_TIME_OUT  			200 		// 超时时间 200ms
#define bl0972_AUTO_TIME   		2000 	  // 2s (采集18次，每次100ms)
#define bl0972_SEND_TIME   		100 	  // 发送时间 100ms
/* 数据 */
#define bl0972_REC_STA  sg_bl0972_rec_sta
#define bl0972_REC_BUFF sg_bl0972_buff

/*
*********************************************************************************************************
*	函 数 名: bl0972_init_function
*	功能说明: 初始化
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_init_function(void)
{
	bl0972_INIT();
	
	/* 开启自动采集功能-电压、电流 */
	sg_bl0972data_t.auto_cmd = 1;
		
	/* 写命令使能 */
	bl0972_write_enable_function(1);
	bl0972_reset_numreg_function();
	bl0972_set_mode1_function();
	
	bl0972_set_mode2_function();
	bl0972_set_gain_function();
	bl0972_set_gonghao_function();
	/* 写命令失能 */
	bl0972_write_enable_function(0);
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_sending_data_function
*	功能说明: 发送数据函数
*	形    参: 
*	@reg		: 寄存器值
*	@mode		: 发送模式
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_sending_data_function(uint8_t reg, uint8_t mode)
{
	sg_bl0972data_t.overtime 	= 1;  // 超时倒计时
	sg_bl0972data_t.result  	= 0;   
	sg_bl0972data_t.mode    	= mode;
	sg_bl0972data_t.send    	= 1;
	sg_bl0972data_t.reg     	= reg;
	sg_bl0972data_t.repeat  	= 0;
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_send_over_function
*	功能说明: 数据发送操作完成
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_send_over_function(void)
{
	sg_bl0972data_t.overtime = 0;
	sg_bl0972data_t.result  = 0;
	sg_bl0972data_t.send    = 0;
	sg_bl0972data_t.repeat  = 0;
}

/*
*********************************************************************************************************
*	函 数 名: Complement_2_Original
*	功能说明: 补码转换为原码
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static uint32_t Complement_2_Original(uint32_t data)
{
	uint32_t temp;
	if((data&0x00800000) == 0x00800000)  // 判断最高位是否为0，Bit[23]为符号位，Bit[23]=0为正
	{
		data &= 0x007FFFFF;  // 清除符号位 	
		temp =~data;         // 反码
		data = temp & 0x007FFFFF;  // 清除左边多余位
		data += 1;				
	}
	else  // 当前为负功
	{
		data &= 0x007FFFFF;  // 清除符号位
	}
	return data;
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_deal_read_data_function
*	功能说明: 处理读取到的数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
int8_t bl0972_deal_read_data_function(void)
{
	uint32_t data  = 0;
	int8_t   ret   = 0;
	uint8_t  index = 0;
	
	for(index=0; index<3; index++) 
		sg_bl0972data_t.checksum += bl0972_REC_BUFF[index];

	if(bl0972_REC_BUFF[3] != (0xff - sg_bl0972data_t.checksum)) 
	{
		ret = -1;
	}
	data = bl0972_REC_BUFF[0];
	data = (data<<8) | bl0972_REC_BUFF[1];
	data = (data<<8) | bl0972_REC_BUFF[2];
	
	//printf("%d\n",data);

	switch(sg_bl0972data_t.reg) 
	{
		case BL0972_V_RMS: 	 det_set_total_energy_bl0972(0,data); break;
		case BL0972_IA_RMS:  det_set_total_energy_bl0972(1,data); break;
		case BL0972_A_WATT:  
			det_set_total_energy_bl0972(2,Complement_2_Original(data));
		break;
		case BL0972_CFA_CNT: det_set_total_energy_bl0972(3,data); break;

		default:			break;
	}
	return ret;
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_repeat_function
*	功能说明: 重复操作函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_repeat_function(void)
{
	/* 数据等待超时 */
	if( (++sg_bl0972data_t.repeat) <= 5) 
	{
		sg_bl0972data_t.overtime = 1;	/* 重复获取或写入 */
		bl0972_read_reg_function(sg_bl0972data_t.reg,0);
	} 
	else 
		bl0972_send_over_function();		/* 结束任务 */
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_analysis_data_function
*	功能说明: 数据读取函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_analysis_data_function(void)
{
	int8_t   ret = 0;
	/* 等待回传数据 */
	if(bl0972_REC_STA&0x8000) 
	{
		
		if(sg_bl0972data_t.mode == 0) 		/* 数据处理 */
		{
			ret = bl0972_deal_read_data_function();		/* 读取数据 */
					
			if(ret != 0) 
				bl0972_repeat_function();
			else 
				bl0972_send_over_function();
		}
		bl0972_REC_STA = 0;
	}
	
	/* 检测本次操作是否超时 */
	if(sg_bl0972data_t.result == 2) 
	{
		sg_bl0972data_t.result = 0;
		bl0972_repeat_function();
	}
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_write_reg_function
*	功能说明: 写寄存器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_write_reg_function(uint8_t reg,uint8_t *data, uint8_t len,uint8_t mode)
{
	uint8_t buff[64] = {0};
	uint8_t index = 0;		
	
	buff[0] = BL0972_CMD_WRITE;
	buff[1] = reg;
	
	for(index=0; index<len; index++) 
		buff[2+index] = data[index];
	
	/* 计数和校验 */
	sg_bl0972data_t.checksum = 0;
	for(index=0; index<(2+len); index++)
		sg_bl0972data_t.checksum+=buff[index];
	
	/* 填充和校验 */
	buff[2+len] = 0xff - sg_bl0972data_t.checksum;
	
	if( mode == 0) 	/* 更新标志 */
		bl0972_sending_data_function(reg,0x80);
	
	/* 数据发送 */
	bl0972_SEND_STR(buff,3+len);
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_read_reg_function
*	功能说明: 寄存器读取命令
*	形    参: 
*	@reg		: 寄存器值
*	@mode		: 0-更新标志 other-不更新
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_read_reg_function(uint8_t reg, uint8_t mode)
{
	uint8_t buff[2] = {0};
	uint16_t index = 0;
	memset(bl0972_REC_BUFF,0,8);
	buff[0] = BL0972_CMD_READ;	/* 读取 */
	buff[1] = reg;	/* 数据 */

	sg_bl0972data_t.checksum = buff[0]+buff[1];   /* 计数和校验 */
	
	if( mode == 0) 	/* 更新标志 */
		bl0972_sending_data_function(reg,0);
	
	bl0972_SEND_STR(buff,sizeof(buff));	/* 数据发送 */
	
	for(index=0; index < 4; index++) {
		bl0972_REC_BUFF[index] = SSPI1_ReadByte();
	}
	bl0972_REC_STA = 0x8000+4;	
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_send_data_function
*	功能说明: 数据发送函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_send_data_function(void)
{
	/* 允许进行发送操作 */
	if(sg_bl0972data_t.send == 0) 
	{	
		if(sg_bl0972data_t.flag > 0)
		{
			switch(sg_bl0972data_t.flag)
			{	
				case 1: 	bl0972_read_reg_function(BL0972_IA_RMS,0); break; // 电流 A
				case 2: 	bl0972_read_reg_function(BL0972_V_RMS,0); break;  // 电压
				case 3: 	
					bl0972_read_reg_function(BL0972_A_WATT,0); 
				break; // 功率 A
				case 4: 	bl0972_read_reg_function(BL0972_CFA_CNT,0); break; // 功率 B			
				default: break;			
			}
			sg_bl0972data_t.flag--;
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_run_timer_function
*	功能说明: 运行计时相关函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_run_timer_function(void)
{
	static uint16_t  times = 0;
	static uint16_t  auto_time = 0;
	
	if(sg_bl0972data_t.overtime != 0) 	/* 计数更新 */
	{
		if(sg_bl0972data_t.overtime == 1) 
		{
			sg_bl0972data_t.overtime = 2;
			times = 0;
		}
		/* 计数值 */
		if((++times) >= bl0972_TIME_OUT)  // 超时时间
		{
			times = 0;
			sg_bl0972data_t.overtime = 0;
			sg_bl0972data_t.result = 2;
		}
	} 
	else 
		times = 0;
	
	/* 自动采集 */
	if(sg_bl0972data_t.auto_cmd == 1) 
	{
		if((++auto_time) >= bl0972_AUTO_TIME) 
		{
			auto_time = 0;
			sg_bl0972data_t.flag = bl0972_DET_NUM;
			sg_bl0972data_t.send = 0;
		}
	} 
	else 
		auto_time = 0;
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_work_process_function
*	功能说明: 工作进程函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_work_process_function(void)
{
	bl0972_analysis_data_function();
	bl0972_send_data_function();
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_write_enable_function
*	功能说明: 写使能控制函数
*	形    参: 
*	@cmd		: 0-失能 1-使能
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_write_enable_function(uint8_t cmd)
{
	uint8_t buff[3] = {0};
	
	if(cmd == 1) 
	{
		buff[0] = 0x00;
		buff[1] = 0x55;
		buff[2] = 0x55;
		bl0972_write_reg_function(BL0972_USR_WRPROT,buff,3,0);
	} 
	else 
	{
		buff[0] = 0x00;
		buff[1] = 0x00;
		buff[2] = 0x00;
		bl0972_write_reg_function(BL0972_USR_WRPROT,buff,3,0);
	}
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_set_mode1_function
*	功能说明: 设置模式1
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_set_mode1_function(void)
{
	uint8_t mode1_buff[3] = {0};
	
	mode1_buff[0] = 0x00;  // 
	mode1_buff[1] = 0x07;  // 打开cf
	mode1_buff[2] = 0xFF;  // 	
	bl0972_write_reg_function(BL0972_MODE1,mode1_buff,3,0);
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_set_mode2_function
*	功能说明: 设置模式2
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_set_mode2_function(void)
{
	uint8_t mode2_buff[3] = {0};
	
	mode2_buff[0] = 0x2A;  // 
	mode2_buff[1] = 0xAA;  // 打开cf
	mode2_buff[2] = 0xAA;  // 	
	bl0972_write_reg_function(BL0972_MODE2,mode2_buff,3,0);
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_set_gain_function
*	功能说明: 设置增益寄存器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_set_gain_function(void)
{
	uint8_t gain_buff[3] = {0};
	
	gain_buff[0] = 0x30;  // 电流通道
	gain_buff[1] = 0x00;
	gain_buff[2] = 0x03;

	bl0972_write_reg_function(BL0972_GAIN,gain_buff,3,0);
	delay_ms(50);
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_set_gonghao_function
*	功能说明: 设置功耗寄存器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_set_gonghao_function(void)
{
	uint8_t gh_buff[3] = {0};
	
	gh_buff[0] = 0x00;  // 电流通道
	gh_buff[1] = 0x07;
	gh_buff[2] = 0xDE;

	bl0972_write_reg_function(BL0972_GH,gh_buff,3,0);
//	delay_ms(50);
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_reset_numreg_function
*	功能说明: 用户区寄存器复位
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_reset_numreg_function(void)
{
	uint8_t ch_buff[3] = {0};
	
	ch_buff[0] = 0x5A;  // 全部使用
	ch_buff[1] = 0x5A;  
	ch_buff[2] = 0x5A; 
	bl0972_write_reg_function(BL0972_SOFT_RESET,ch_buff,3,0);
}

/*
*********************************************************************************************************
*	函 数 名: bl0972_test
*	功能说明: 电压、电流测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bl0972_test(void)
{
	bl0972_read_reg_function(BL0972_PHASE_I,0);  // 0x1010
	delay_ms(200);
	bl0972_read_reg_function(BL0972_VAR_CREEP,0);  // 0x04C04C
	delay_ms(200);	
	bl0972_read_reg_function(BL0972_RMS_CREEP,0);  // 0x200
	delay_ms(200);	
	bl0972_read_reg_function(BL0972_SAGLVL_LINECYC,0);  // 0x100009
	delay_ms(200);	
	while(1)
	{
		bl0972_work_process_function();	// 数据获取函数
		delay_ms(100);	
	}
}






