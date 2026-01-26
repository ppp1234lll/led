#include "appconfig.h"
#include "./TASK/inc/single.h"

#define SEND_HEAD_BYTE1 (0x0F) 	 
#define SEND_HEAD_BYTE2 (0x0F) 	 
																 
#define SEND_END_BYTE1 (0xFF) 	 
#define SEND_END_BYTE2 (0xFF) 	 
		
#define SEND_CMD       (0xA1) 

#define RECV_HAED_BYTE1 (0xF0) 	 
#define RECV_HAED_BYTE2 (0xF0) 	 
																 
#define RECV_END_BYTE1 (0xFF) 	 
#define RECV_END_BYTE2 (0xFF) 	 
		

typedef void (*UartSendFuncPtr)(uint8_t *data, uint16_t len);	
	
static const UartSendFuncPtr uart_send_table[BOARD_MAX] = {
  Uart5_Send_Data,    
	Usart4_Send_Data,
	Lpuart1_Send_Data,  
  Usart1_Send_Data,    
};		
		
__attribute__((at (0x30000000))) single_data_t g_singleboard_t[BOARD_MAX]={0};

uint8_t g_single_time_t[BOARD_MAX][12]={0};

uint8_t  single_send_buf[16];  
uint8_t  single_send_len;
uint8_t  single_recv_buf[BOARD_MAX][256];
uint16_t single_recv_sta[BOARD_MAX];  

 
CurrentData_t  current_data = {0};
CurrentAverageData_t current_sum_data = {0};


void Single_Send_Data(borad_id_t num, uint8_t *data, uint16_t len);
/*
*********************************************************************************************************
* 单色灯结构定义：参数
* 包含：电压/电流/脉冲/故障状态
*********************************************************************************************************
*/
typedef struct {
    Params_t *p_params; // 指向参数结构体（电压/电流/脉冲/故障状态）
} SingleColorLight_t;

/*
*********************************************************************************************************
* 相位灯结构定义：包含3个颜色灯
*********************************************************************************************************
*/
typedef struct {
    SingleColorLight_t *p_color[COLOR_MAX]; // 指向3个颜色灯的指针数组
} PhaseLight_t;

/*
*********************************************************************************************************
* 方向灯结构定义：包含9个相位
*********************************************************************************************************
*/
typedef struct {
    PhaseLight_t *p_phase[PHASE_MAX]; // 指向9个相位的指针数组
} DirectionLight_t;

/*
*********************************************************************************************************
* 类型灯结构定义：远灯/近灯各4个方向
*********************************************************************************************************
*/
typedef struct {
    DirectionLight_t *p_direction[DIR_MAX]; // 指向4个方向的指针数组
} LightType_t;

/*
*********************************************************************************************************
* 信号灯总结构：包含类型+方向+相位+颜色
*********************************************************************************************************
*/
typedef struct {
    LightType_t *p_light_type[Type_MAX]; // 指向远灯/近灯的指针数组
} SingleLight_t;


// 全局变量定义
// Params_t数组
Params_t params[Type_MAX][DIR_MAX][PHASE_MAX][COLOR_MAX] = {0};

// SingleColorLight_t数组
SingleColorLight_t single_color_light[Type_MAX][DIR_MAX][PHASE_MAX][COLOR_MAX] = {0};

// PhaseLight_t数组
PhaseLight_t phase_light[Type_MAX][DIR_MAX][PHASE_MAX] = {0};

// DirectionLight_t数组
DirectionLight_t direction_light[Type_MAX][DIR_MAX] = {0};

// LightType_t数组
LightType_t light_type[Type_MAX] = {0};

// 信号灯总结构
SingleLight_t single_light = {0};

// 配置数据
ConfigData_t g_config_data = {0};


#define CURRENT_AVERAGE_TIME  36000 // 1小时
void single_ch2_light_timer_run(void);
/*
*********************************************************************************************************
* 函 数 名: single_task_function
* 功能描述: 信号灯任务函数
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_task_function(void)
{
 
	printf("run single\n");
	uint8_t index;
	uint16_t current_time = 0;
	
	single_led_init_memory();
//	single_cmd_board_data(single_send_buf,&single_send_len);  
//	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,0,1,0,0,0,0);
//  Single_Bind_InpuToTraffic( PARAM_CURRENT,0,1,0,0,0,0);
	single_load_config_from_flash(); // 读取配置信息
	
	// 初始化定时器
	single_led_timer_init();
	
	while(1)
	{
		for(index = 0;index<BOARD_MAX;index++)
		{
//			Single_Send_Data((borad_id_t)index,single_send_buf,single_send_len);
			single_deal_board_data(index);
		}
		
//		single_led_timer_run();
		single_ch2_light_timer_run();
		single_update_current_average();
		
		
		// 电流平均值计算时间
		current_time++;
		if(current_time >= CURRENT_AVERAGE_TIME)
		{
			single_calculate_current_average();
		  current_time = 0;
		}
		iwdg_feed();         		
		vTaskDelay(100);     	  	
	}
}

/*
*********************************************************************************************************
* 函 数 名: Single_Send_Data
* 功能描述: 发送数据到板卡
* 参    数: num - 板卡ID
*           data - 数据缓冲区
*           len - 数据长度
* 返 回 值: 无
*********************************************************************************************************
*/
void Single_Send_Data(borad_id_t num, uint8_t *data, uint16_t len)
{
	if (num >= BOARD_MAX || data == NULL || len == 0) {
		return; 
	}
	uart_send_table[num](data, len);
}

/*
*********************************************************************************************************
* 函 数 名: single_cmd_board_data
* 功能描述: 命令板卡数据
* 参    数: data - 数据缓冲区
*           len - 数据长度
* 返 回 值: 无
*********************************************************************************************************
*/
void single_cmd_board_data(uint8_t *data, uint8_t *len)
{
	uint8_t index = 0;
	uint8_t crc   = 0;
	
	/* 帧头 */
	data[index++] = SEND_HEAD_BYTE1; 
	data[index++] = SEND_HEAD_BYTE2;

	/* 命令 */
	data[index++] = SEND_CMD;
	
	/* 板卡地址 */
	data[index++] = 0x01;

	/* 数据长度 */
	data[index++] = 0x01;

	/* crc校验 */
	crc = calc_crc8(&data[2],index-2);
	data[index++] = crc;
	
	/* 帧尾 */
	data[index++] = SEND_END_BYTE1;
	data[index++] = SEND_END_BYTE2;
	*len = index;
}
/*
*********************************************************************************************************
* 函 数 名: single_recv_board_data
* 功能描述: 接收板卡数据
* 参    数: id - 板卡ID
*           data - 数据缓冲区
*           len - 数据长度
* 返 回 值: 无
*********************************************************************************************************
*/
void single_recv_board_data(uint8_t id, uint8_t *data, uint8_t len)
{
	memcpy(single_recv_buf[id], data, len );
	single_recv_sta[id] = (len | 0x8000);
	memset(data,0,len);
}

/*
*********************************************************************************************************
* 函 数 名: single_recv_board_data_0
* 功能描述: 接收板卡0数据
* 参    数: data - 数据缓冲区
*           len - 数据长度
* 返 回 值: 无
*********************************************************************************************************
*/
void single_recv_board_data_0(uint8_t *data, uint8_t len)
{
	memcpy(single_recv_buf[0], data, len );
	single_recv_sta[0] = (len | 0x8000);
	memset(data,0,len);
}

/*
*********************************************************************************************************
* 函 数 名: single_recv_board_data_1
* 功能描述: 接收板卡1数据
* 参    数: data - 数据缓冲区
*           len - 数据长度
* 返 回 值: 无
*********************************************************************************************************
*/
void single_recv_board_data_1(uint8_t *data, uint8_t len)
{
	memcpy(single_recv_buf[1], data, len );
	single_recv_sta[1] = (len | 0x8000);
	memset(data,0,len);
}

/*
*********************************************************************************************************
* 函 数 名: single_recv_board_data_2
* 功能描述: 接收板卡2数据
* 参    数: data - 数据缓冲区
*           len - 数据长度
* 返 回 值: 无
*********************************************************************************************************
*/
void single_recv_board_data_2(uint8_t *data, uint8_t len)
{
	memcpy(single_recv_buf[2], data, len );
	single_recv_sta[2] = (len | 0x8000);
	memset(data,0,len);
}

/*
*********************************************************************************************************
* 函 数 名: single_recv_board_data_3
* 功能描述: 接收板卡3数据
* 参    数: data - 数据缓冲区
*           len - 数据长度
* 返 回 值: 无
*********************************************************************************************************
*/
void single_recv_board_data_3(uint8_t *data, uint8_t len)
{
	memcpy(single_recv_buf[3], data, len );
	single_recv_sta[3] = (len | 0x8000);
	memset(data,0,len);
}

/*
*********************************************************************************************************
* 函 数 名: single_deal_board_data
* 功能描述: 处理板卡数据
* 参    数: id - 板卡ID
* 返 回 值: 错误码
*********************************************************************************************************
*/
uint8_t single_deal_board_data(uint8_t id)
{
	uint8_t crc  = 0;
	uint8_t size = 0;
	uint8_t err = 0;
	if (single_recv_sta[id] & 0x8000)  
	{
		size = single_recv_sta[id] & 0x07FF;
		if((single_recv_buf[id][0] == RECV_HAED_BYTE1) && (single_recv_buf[id][1] == RECV_HAED_BYTE2))
		{
			if(size >= 5)
			{
				crc = calc_crc8(&single_recv_buf[id][2],size-5);
				if(crc == single_recv_buf[id][size-3])
				{
					g_singleboard_t[id].cmd  = single_recv_buf[id][2];
					g_singleboard_t[id].size = single_recv_buf[id][3];
					memcpy(&g_singleboard_t[id].data, &single_recv_buf[id][4], sizeof(board_t));
				}
				else
					err = 3;
			}
			else
				err = 4; // 数据长度不足
		}
		else
			err = 2;
	}
	else
		err = 1;
	single_recv_sta[id] = 0;
	memset(single_recv_buf[id],0,128);
	return err;
}

/*
*********************************************************************************************************
* 函 数 名: single_led_init_memory
* 功能描述: 初始化内存
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_led_init_memory(void)
{
	Type_e type;
	Direction_e dir;
	Phase_e phase;
	Color_e color;
	
	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 将light_type数组中的元素绑定到single_light中
		single_light.p_light_type[type] = &light_type[type];
		
		// 遍历所有方向：北/东/南/西
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			// 将direction_light数组中的元素绑定到light_type中
			single_light.p_light_type[type]->p_direction[dir] = &direction_light[type][dir];
			
			// 遍历所有相位
			for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
			{
				// 将phase_light数组中的元素绑定到direction_light中
				single_light.p_light_type[type]->p_direction[dir]->p_phase[phase] = &phase_light[type][dir][phase];
				
				// 遍历所有颜色：红/绿/黄
				for (color = COLOR_RED; color < COLOR_MAX; color++)
				{
					// 将single_color_light数组中的元素绑定到phase_light中
					single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color] = &single_color_light[type][dir][phase][color];
					
					// 将params数组中的元素绑定到single_color_light中
					single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params = &params[type][dir][phase][color];
				}
			}
		}
	}
}

/*
*********************************************************************************************************
* 函 数 名: Single_Bind_InpuToTraffic
* 功能描述: 将板卡数据绑定到信号灯结构中
* 参    数: param_type - 参数类型
*           board_id - 板卡ID
*           ch - 通道号
*           p_type - 灯类型：远灯/近灯
*           p_dir - 方向
*           p_phase - 相位
*           p_color - 颜色
* 返 回 值: 无
*********************************************************************************************************
*/
void Single_Bind_InpuToTraffic(ParamType_e param_type, 
								uint8_t board_id,uint8_t ch,	
								Type_e p_type,Direction_e p_dir, 
								Phase_e p_phase,Color_e p_color)
{
	// 检查board_id是否有效
	if (board_id >= BOARD_MAX)
	{
		return;
	}
	
	// 根据参数类型绑定不同的数据到信号灯结构中
	switch (param_type) 
	{
			case PARAM_CURRENT:
			// 检查通道号是否在有效范围内
			if (ch >= 24)  
				return;

			// 将current数据绑定到板卡对应通道
			single_light.p_light_type[p_type]->p_direction[p_dir]
											->p_phase[p_phase]
											->p_color[p_color]
											->p_params->current
			   = &g_singleboard_t[board_id].data.current[ch];
			break;
			
		case PARAM_VOLTAGE:
			// 检查通道号是否在有效范围内
			if (ch >= 12)  
				return;

			// 将voltage数据绑定到板卡对应通道
			single_light.p_light_type[p_type]->p_direction[p_dir]
											->p_phase[p_phase]
											->p_color[p_color]
											->p_params->voltage
			   = &g_singleboard_t[board_id].data.voltage[ch];
			
			// 将pulse数据绑定到板卡对应通道
		single_light.p_light_type[p_type]->p_direction[p_dir]
									->p_phase[p_phase]
									->p_color[p_color]
									->p_params->pulse
	   = &g_singleboard_t[board_id].data.pulse[ch];
		break;
	default:
		break;
	}
	
	// 记录配置信息到g_config_data
	single_record_config(param_type, board_id, ch, p_type, p_dir, p_phase, p_color);
}

/*********************************************************************************************************
* 函 数 名: single_record_config
* 功能描述: 记录配置信息到g_config_data
* 参    数: param_type - 参数类型
*           board_id - 板ID
*           ch - 通道号
*           p_type - 灯类型
*           p_dir - 方向
*           p_phase - 相位
*           p_color - 颜色
* 返 回 值: 无
*********************************************************************************************************
*/
void single_record_config(ParamType_e param_type, 
					uint8_t board_id, uint8_t ch, 
					Type_e p_type, Direction_e p_dir, 
					Phase_e p_phase, Color_e p_color)
{
	uint32_t i;
	uint8_t config_exists = 0;
	
	if (param_type == PARAM_CURRENT)
	{
		// 电流通道：1对1关系，同一通道只能对应一个信号灯
		// 检查是否已存在相同的电流通道配置
		for (i = 0; i < g_config_data.config_count; i++)
		{
			ConfigItem_t *existing_item = &g_config_data.config_items[i];
			if (existing_item->param_type == PARAM_CURRENT &&
				existing_item->board_id == board_id &&
				existing_item->ch == ch)
			{
				// 更新现有配置的信号灯信息
				existing_item->p_type = p_type;
				existing_item->p_dir = p_dir;
				existing_item->p_phase = p_phase;
				existing_item->p_color = p_color;
				config_exists = 1;
				break;
			}
		}
		
		// 检查是否已存在相同的信号灯配置
		if (!config_exists)
		{
			for (i = 0; i < g_config_data.config_count; i++)
			{
				ConfigItem_t *existing_item = &g_config_data.config_items[i];
				if (existing_item->param_type == PARAM_CURRENT &&
					existing_item->board_id == board_id &&
					existing_item->p_type == p_type &&
					existing_item->p_dir == p_dir &&
					existing_item->p_phase == p_phase &&
					existing_item->p_color == p_color)
				{
					// 更新现有配置的通道号
					existing_item->ch = ch;
					config_exists = 1;
					break;
				}
			}
		}
	}
	else if (param_type == PARAM_VOLTAGE)
	{
		// 电压通道：1对多关系，同一通道可以对应多个信号灯
		// 但同一个信号灯只能对应一个电压通道
		// 检查是否已存在相同的信号灯配置
		for (i = 0; i < g_config_data.config_count; i++)
		{
			ConfigItem_t *existing_item = &g_config_data.config_items[i];
			if (existing_item->param_type == PARAM_VOLTAGE &&
				existing_item->board_id == board_id &&
				existing_item->p_type == p_type &&
				existing_item->p_dir == p_dir &&
				existing_item->p_phase == p_phase &&
				existing_item->p_color == p_color)
			{
				// 更新现有配置的通道号
				existing_item->ch = ch;
				config_exists = 1;
				break;
			}
		}
	}
	
	// 如果不存在相同配置，添加新配置
	if (!config_exists && g_config_data.config_count < MAX_CONFIG_ITEMS)
	{
		ConfigItem_t *item = &g_config_data.config_items[g_config_data.config_count];
		item->param_type = param_type;
		item->board_id = board_id;
		item->ch = ch;
		item->p_type = p_type;
		item->p_dir = p_dir;
		item->p_phase = p_phase;
		item->p_color = p_color;
		g_config_data.config_count++;
	}
	
	// 保存配置
	single_save_config_to_flash();
}

/*********************************************************************************************************
* 定时器相关定义
*********************************************************************************************************/

typedef struct {
	uint32_t count;  // 记录上次状态变化的时间戳(ms)
	uint32_t off_time; // 记录灯熄灭的时间戳(ms)，用于检测脉冲信号
	uint8_t  last_v; // 记录上一次的电压状态变化
	uint8_t  new_v; // 记录新的电压状态变化
	uint8_t  is_pulsing; // 是否处于脉冲状态
} TimerInfo_t;

// 板卡定时器信息结构体
typedef struct {
    TimerInfo_t timer[12]; // 每个板卡12个通道的定时器
} BoardTimerInfo_t;

// 板卡定时器信息数组
BoardTimerInfo_t g_timer_info[BOARD_MAX] = {0};

/*
*********************************************************************************************************
* 函 数 名: single_led_timer_init
* 功能描述: 初始化定时器
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_led_timer_init(void)
{
	uint8_t board_id;
	uint8_t channel;

	// 初始化所有板卡的所有通道定时器
	for (board_id = 0; board_id < BOARD_MAX; board_id++) {
		for (channel = 0; channel < 12; channel++) {
			g_timer_info[board_id].timer[channel].count = 0;
			g_timer_info[board_id].timer[channel].off_time = 0;
			g_timer_info[board_id].timer[channel].new_v = 0xFF;
			g_timer_info[board_id].timer[channel].last_v = 0xFF; // 初始化为无效值
			g_timer_info[board_id].timer[channel].is_pulsing = 0;
		}
	}
}
/*
*********************************************************************************************************
* 函 数 名: single_led_timer_run
* 功能描述: 运行定时器
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
uint8_t pluse_test[4][12] = {0};
void single_led_timer_run(void)
{
	uint8_t board_id=0;
	uint8_t channel=0;
	uint32_t current_time=0;
  uint32_t elapsed_time=0;
	
  for (board_id = 0; board_id < BOARD_MAX; board_id++) 
	{
		for (channel = 0; channel < 12; channel++) 
		{
			if(g_singleboard_t[board_id].data.pulse[channel] == 1)
			 pluse_test[board_id][channel]++;
			
			g_timer_info[board_id].timer[channel].new_v = g_singleboard_t[board_id].data.voltage[channel];
			if (g_timer_info[board_id].timer[channel].new_v != g_timer_info[board_id].timer[channel].last_v) 
			{
				g_timer_info[board_id].timer[channel].last_v = g_timer_info[board_id].timer[channel].new_v;
				if (g_timer_info[board_id].timer[channel].new_v == 0) 
				{
					g_timer_info[board_id].timer[channel].count = bsp_GetRunTime();
				}
				else if(g_timer_info[board_id].timer[channel].new_v == 1) 
				{
					current_time = bsp_GetRunTime();
					if (g_timer_info[board_id].timer[channel].count != 0) 
					{
						elapsed_time = current_time - g_timer_info[board_id].timer[channel].count;
						g_single_time_t[board_id][channel] = (uint16_t)((elapsed_time+200)/1000);
						g_timer_info[board_id].timer[channel].count = 0;
					}
				}
			}
    }
  }

}

void single_traffic_light_timer_run(void)
{
	uint8_t board_id=0;
	uint8_t channel=0;
	uint32_t current_time=0;
  uint32_t elapsed_time=0;
	uint32_t off_duration=0;
	#define PULSE_THRESHOLD 400 // 脉冲信号阈值（ms）

  for (board_id = 0; board_id < BOARD_MAX; board_id++) 
	{
		for (channel = 0; channel < 12; channel++) 
		{
			if(g_singleboard_t[board_id].data.pulse[channel] == 1)
			 pluse_test[board_id][channel]++;

			g_timer_info[board_id].timer[channel].new_v = g_singleboard_t[board_id].data.voltage[channel];
			if (g_timer_info[board_id].timer[channel].new_v != g_timer_info[board_id].timer[channel].last_v) 
			{
				g_timer_info[board_id].timer[channel].last_v = g_timer_info[board_id].timer[channel].new_v;
				if (g_timer_info[board_id].timer[channel].new_v == 0) 
				{
					// 灯亮（低电平），记录点亮时间
					g_timer_info[board_id].timer[channel].count = bsp_GetRunTime();
				}
				else if(g_timer_info[board_id].timer[channel].new_v == 1) 
				{
					// 灯灭（高电平），计算点亮持续时间
					current_time = bsp_GetRunTime();
					if (g_timer_info[board_id].timer[channel].count != 0) 
					{
						elapsed_time = current_time - g_timer_info[board_id].timer[channel].count;
						
						// 检查是否是脉冲信号（短暂点亮）
						if (elapsed_time < PULSE_THRESHOLD)
						{
							// 是脉冲信号，不更新时间
							g_timer_info[board_id].timer[channel].count = 0;
						}
						else
						{
							// 不是脉冲信号，正常更新时间
							g_single_time_t[board_id][channel] = (uint16_t)((elapsed_time+200)/1000);
							g_timer_info[board_id].timer[channel].count = 0;
						}
					}
				}
			}
    }
  }

}

typedef struct
{
	uint32_t low_time[10];   // 低电平跳变时间
	uint32_t high_time[10];  // 高电平跳变时间
	uint8_t high_jump_num;  // 高电平跳变计数
	uint8_t low_jump_num;  // 低电平跳变计数		
	uint8_t high_jump_flag;  // 高电平跳变标志
}single_time_t;

void single_ch2_light_timer_run(void)
{
	single_time_t single_ch2_light_time;
	static   uint32_t high_duration = 0;  // 高电平持续时间 
	uint32_t elapsed_time = 0;
	#define PULSE_MIN_THRESHOLD 250 // 脉冲信号最小阈值（ms）
	#define PULSE_MAX_THRESHOLD 400 // 脉冲信号最大阈值（ms）

	g_timer_info[BOARD_2].timer[2].new_v = g_singleboard_t[BOARD_2].data.voltage[2];

	if (g_timer_info[BOARD_2].timer[2].new_v != g_timer_info[BOARD_2].timer[2].last_v) 
	{
		if (g_timer_info[BOARD_2].timer[2].new_v == 0) // 当前为低电平（点亮）
		{
			single_ch2_light_time.low_time[low_jump_num] = bsp_GetRunTime(); // 获取时间
			single_ch2_light_time.low_jump_num++; // 低电平跳变计数
		}
		else if(g_timer_info[BOARD_2].timer[2].new_v == 1) // 当前为高电平（熄灭）
		{
			single_ch2_light_time.high_jump_flag = 1; // 跳变标志
			single_ch2_light_time.high_time[high_jump_num] = bsp_GetRunTime(); // 获取时间
			single_ch2_light_time.high_jump_num++; // 高电平跳变计数
		}
	}
	else
	{
		if(single_ch2_light_time.low_jump_num > 0)  // 有低电平跳变
		{
			if(single_ch2_light_time.high_jump_flag == 1)  // 有高电平跳变
			{
				if(g_singleboard_t[BOARD_2].data.voltage[2] == 1) // 判断是否是高电平
				{
					high_duration++; // 高电平持续时间增加
					if(high_duration >= 150) // 如果高电平时间超过150*10 = 1.5s,认为这轮结束，计算时间
					{
						elapsed_time = single_ch2_light_time.high_time[high_jump_num-1] - single_ch2_light_time.low_time[low_jump_num-1]; // 计算高电平持续时间
						g_single_time_t[BOARD_2][2] = (uint16_t)((elapsed_time+200)/1000);
						high_duration = 0; // 重置高电平持续时间
						single_ch2_light_time.high_jump_num = 0; // 重置高电平跳变计数
						single_ch2_light_time.low_jump_num = 0; // 重置低电平跳变计数
					}
				}
			}


		}




	}
}

/*********************************************************************************************************
* 函 数 名: single_collect_current_data
* 功能描述: 收集single_light中的电流数据并存储到CurrentData_t结构体中
* 参    数: data - 指向电流数据结构体的指针
* 返 回 值: 无
*********************************************************************************************************
*/
void single_collect_current_data(CurrentData_t *data)
{
	Type_e type;                     // 灯类型：远灯/近灯
	Direction_e dir;                 // 方向：北/东/南/西
	Phase_e phase;                   // 相位
	Color_e color;                   // 颜色：红/绿/黄
	
	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 检查灯类型指针是否有效
		if (single_light.p_light_type[type] == NULL)
		{
			continue;
		}
		
		// 遍历所有方向：北/东/南/西
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			// 检查方向指针是否有效
			if (single_light.p_light_type[type]->p_direction[dir] == NULL)
			{
				continue;
			}
			
			// 遍历所有相位
			for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
			{
				// 检查相位指针是否有效
				if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase] == NULL)
				{
					continue;
				}
				
				// 遍历所有颜色：红/绿/黄
				for (color = COLOR_RED; color < COLOR_MAX; color++)
				{
					// 检查颜色灯指针是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color] == NULL)
					{
						continue;
					}
					
					// 检查参数指针是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params == NULL)
					{
						continue;
					}
					
					// 检查电流指针是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params->current != NULL)
					{
						// 读取电流值
						data->current[type][dir][phase][color] = *single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params->current;
					}
					else
					{
						// 指针无效时设为0
						data->current[type][dir][phase][color] = 0.0f;
					}
				}
			}
		}
	}
}

/*********************************************************************************************************
* 函 数 名: single_update_current_average
* 功能描述: 更新电流平均值（每1秒计算一次）
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_update_current_average(void)
{
	Type_e type;
	Direction_e dir;
	Phase_e phase;
	Color_e color;
	float current_value;
	
	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 检查灯类型指针是否有效
		if (single_light.p_light_type[type] == NULL)
		{
			continue;
		}
		
		// 遍历所有方向：北/东/南/西
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			// 检查方向指针是否有效
			if (single_light.p_light_type[type]->p_direction[dir] == NULL)
				continue;
			
			// 遍历所有相位
			for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
			{
				// 检查相位指针是否有效
				if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase] == NULL)
					continue;
				
				// 遍历所有颜色：红/绿/黄
				for (color = COLOR_RED; color < COLOR_MAX; color++)
				{
					// 检查颜色灯指针是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color] == NULL)
						continue;
					
					// 检查参数指针是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params == NULL)
						continue;
					
					// 检查电流指针是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params->current != NULL)
					{
						// 读取电流值
						current_value = *single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params->current;
						// 累加电流值和计数
						current_sum_data.sum[type][dir][phase][color] += current_value;
						current_sum_data.count[type][dir][phase][color]++;
					}
				}
			}
		}
	}
}

/*********************************************************************************************************
* 函 数 名: single_calculate_current_average
* 功能描述: 计算电流平均值（每1秒计算一次）
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_calculate_current_average(void)
{
	Type_e type;
	Direction_e dir;
	Phase_e phase;
	Color_e color;
	
	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
			{
				for (color = COLOR_RED; color < COLOR_MAX; color++)
				{
					// 计算平均值
					if (current_sum_data.count[type][dir][phase][color] > 0)
					{
						current_data.current[type][dir][phase][color] = current_sum_data.sum[type][dir][phase][color] / current_sum_data.count[type][dir][phase][color];
					}
					else
					{
						current_data.current[type][dir][phase][color] = 0.0f;
					}
				}
			}
		}
	}
}

/*********************************************************************************************************
* 故障检测相关函数
*********************************************************************************************************/

/*
*********************************************************************************************************
* 函 数 名: single_led_check_all_off
* 功能描述: 检查所有信号灯是否全灭
* 参    数: 无
* 返 回 值: uint8_t - 1表示全灭，0表示不全灭
*********************************************************************************************************
*/
uint8_t single_led_check_all_off(void)
{
	Type_e type;                     // 灯类型：远灯/近灯
	Direction_e dir;                 // 方向：北/东/南/西
	Phase_e phase;                   // 相位
	Color_e color;                   // 颜色：红/绿/黄
	float current_value;             // 电流值
	uint32_t voltage_value;          // 电压值
	uint8_t has_valid_channel = 0;   // 是否有有效通道
	uint8_t is_dir_all_off;          // 方向是否全灭
	uint8_t dir_channel_count;       // 方向通道数量
	uint8_t dir_all_off_mask = 0;    // 方向全灭掩码，用于标记哪些方向全灭
	uint8_t is_all_system_off = 1;   // 系统是否全灭
	
	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 检查灯类型指针是否有效
		if (single_light.p_light_type[type] == NULL)
		{
			continue;
		}
		
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			// 检查方向指针是否有效
			if (single_light.p_light_type[type]->p_direction[dir] == NULL)
			{
				continue;
			}
			
			// 初始化方向全灭标志和通道计数
			is_dir_all_off = 1;
			dir_channel_count = 0;
			
			// 遍历所有相位
			for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
			{
				// 检查相位指针是否有效
				if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase] == NULL)
				{
					continue;
				}
				
				// 遍历所有颜色：红/绿/黄
				for (color = COLOR_RED; color < COLOR_MAX; color++)
				{
					// 检查颜色灯指针是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color] == NULL)
					{
						continue;
					}
					
					// 检查参数指针是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params == NULL)
					{
						continue;
					}
					
					// 检查电压和电流指针是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params->voltage != NULL &&
						single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params->current != NULL)
					{
						// 标记有有效通道
						has_valid_channel = 1;
						dir_channel_count++;
						
						// 读取电压和电流值
						voltage_value = *single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params->voltage;
						current_value = *single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params->current;
						
						// 检查是否有灯亮
						if (voltage_value != 0 || current_value >= 0.1f) // 电流大于0.1A认为灯亮
						{
							// 只要有一个灯亮，方向就不是全灭，系统也不是全灭
							is_dir_all_off = 0;
							is_all_system_off = 0;
						}
					}
				}
			}
			
			// 方向是否全灭
			if (is_dir_all_off && dir_channel_count > 0)
			{
				// 标记方向全灭
				dir_all_off_mask |= (1 << dir);
			}
		}
	}
	
	// 返回结果
	// 低4位表示方向全灭掩码，高4位表示状态
	// 高4位为0表示系统不全灭，1表示系统全灭，2表示无有效通道
	if (has_valid_channel)
	{
		if (is_all_system_off)
		{
			return 0x10 | dir_all_off_mask; // 系统全灭
		}
		else
		{
			return dir_all_off_mask; // 系统不全灭，返回方向全灭掩码
		}
	}
	else
	{
		return 0x20; // 无有效通道，返回特殊标记
	}
}

/*********************************************************************************************************
* 函 数 名: single_check_phase_red_green_simultaneous
* 功能描述: 检查是否有相位同时出现红绿灯亮的情况（同一个相位）
* 参    数: 无
* 返 回 值: uint32_t - 相位红绿同亮的掩码
*           低16位表示相位掩码，每一位对应一个相位是否有红绿同亮
*           位0-11对应PHASE_LEFT到PHASE_SERVICE共12个相位
*           位12-13对应方向0-3（保留）
*           位14-15对应类型0-1（保留）
*********************************************************************************************************
*/
uint32_t single_check_phase_red_green_simultaneous(void)
{
	Type_e type;
	Direction_e dir;
	Phase_e phase;
	float red_current = 0.0f;
	float green_current = 0.0f;
	uint8_t red_on = 0;
	uint8_t green_on = 0;
	uint32_t fault_mask = 0;
	
	// 遍历single_light结构检查每个相位
	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 检查灯类型指针是否有效
		if (single_light.p_light_type[type] == NULL)
		{
			continue;
		}
		
		// 遍历所有方向：北/东/南/西
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			// 检查方向指针是否有效
			if (single_light.p_light_type[type]->p_direction[dir] == NULL)
			{
				continue;
			}
			
			// 遍历所有相位
			for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
			{
				// 检查相位指针是否有效
				if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase] == NULL)
				{
					continue;
				}
				
				// 初始化标志
				red_on = 0;
				green_on = 0;
				
				// 检查红灯是否有效
				if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[COLOR_RED] != NULL &&
					single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[COLOR_RED]->p_params != NULL &&
					single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[COLOR_RED]->p_params->current != NULL)
				{
					// 读取红灯电流
					red_current = *single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[COLOR_RED]->p_params->current;
					// 电流大于等于0.1A认为灯亮
					if (red_current >= 0.1f)
					{
						red_on = 1;
					}
				}
				
				// 检查绿灯是否有效
				if (single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[COLOR_GREEN] != NULL &&
					single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[COLOR_GREEN]->p_params != NULL &&
					single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[COLOR_GREEN]->p_params->current != NULL)
				{
					// 读取绿灯电流
					green_current = *single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[COLOR_GREEN]->p_params->current;
					// 电流大于等于0.1A认为灯亮
					if (green_current >= 0.1f)
					{
						green_on = 1;
					}
				}
				
				// 检查是否红绿同亮
				if (red_on && green_on)
				{
					// 构建故障掩码
					// 位0-11：相位
					// 位12-13：方向
					// 位14-15：类型
					uint32_t current_fault = 0;
					current_fault |= (1 << phase);                          // 相位位
					current_fault |= ((uint32_t)dir << 12);                 // 方向位
					current_fault |= ((uint32_t)type << 14);                // 类型位
					
					// 保存到故障掩码
					fault_mask |= current_fault;
				}
			}
		}
	}
	
	return fault_mask;
}

/*********************************************************************************************************
* 函 数 名: single_save_config_to_flash
* 功能描述: 将配置保存到FLASH
* 参    数: 无
* 返 回 值: 错误状态
*********************************************************************************************************
*/
int single_save_config_to_flash(void)
{
	// 使用文件系统保存配置数据
	if (save_stroage_single_led_blind_function(g_config_data) == 0)
	{
		return SUCCESS;
	}
	else
	{
		return ERROR;
	}
}

/*********************************************************************************************************
* 函 数 名: single_load_config_from_flash
* 功能描述: 从FLASH读取配置
* 参    数: 无
* 返 回 值: 错误状态
*********************************************************************************************************
*/
int single_load_config_from_flash(void)
{
	// 使用文件系统读取配置数据
	if (save_read_single_led_blind_function(&g_config_data) == 0)
	{
		// 应用配置
		for (uint32_t i = 0; i < g_config_data.config_count; i++)
		{
			ConfigItem_t *item = &g_config_data.config_items[i];
			Single_Bind_InpuToTraffic(item->param_type, 
								item->board_id, item->ch, 
								item->p_type, item->p_dir, 
								item->p_phase, item->p_color);
		}
		return SUCCESS;
	}
	else
	{
		// 配置数据无效，初始化为默认值
		memset(&g_config_data, 0, sizeof(ConfigData_t));
		return ERROR;
	}
}

/*********************************************************************************************************
* 函 数 名: single_clear_config_function
* 功能描述: 清除所有信号灯配置
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_clear_config_function(void)
{
	// 清空配置数据
	memset(&g_config_data, 0, sizeof(ConfigData_t));
	single_save_config_to_flash();
}






















