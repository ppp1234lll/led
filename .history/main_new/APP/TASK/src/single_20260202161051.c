#include "appconfig.h"
#include "./TASK/inc/single.h"
#include "bsp_timers.h"
#include "semphr.h"

#define SEND_HEAD_BYTE1 (0x0F) 	 
#define SEND_HEAD_BYTE2 (0x0F) 	 
																 
#define SEND_END_BYTE1 (0xFF) 	 
#define SEND_END_BYTE2 (0xFF) 	 
		
#define SEND_CMD       (0xA1) 

#define RECV_HAED_BYTE1 (0xF0) 	 
#define RECV_HAED_BYTE2 (0xF0) 	 
																 
#define RECV_END_BYTE1 (0xFF) 	 
#define RECV_END_BYTE2 (0xFF) 	 
		

#define SINGLE_C_T_RECAL_TIME  	1200 //3600 // 1H = 60min*60s
#define SINGLE_C_T_SAVE_TIME		600 //3600 // 1H = 60min*60s

#define STARTUP_CHECK_DELAY_MS	600  // 5MIN * 60S * 10

// 创建信号量
SemaphoreHandle_t xSemaphore_CT_Recal;


/*
*********************************************************************************************************
* 发送指针函数定义
* 包含：4个串口
*********************************************************************************************************
*/
typedef void (*UartSendFuncPtr)(uint8_t *data, uint16_t len);	
	
static const UartSendFuncPtr uart_send_table[BOARD_MAX] = {
	Uart5_Send_Data,    
	Usart4_Send_Data,
	Lpuart1_Send_Data,  
	Usart1_Send_Data,    
};		
		
/*
*********************************************************************************************************
* 发送、接收缓存
*********************************************************************************************************
*/
uint8_t  single_send_buf[16];  
uint8_t  single_send_len;

uint8_t  single_recv_buf[BOARD_MAX][256];
uint16_t single_recv_sta[BOARD_MAX];  

/*
*********************************************************************************************************
* 各板的数据数组（固定地址）：电流、电压、脉冲
*********************************************************************************************************
*/
__attribute__((at (0x30000000))) single_data_t g_singleboard_t[BOARD_MAX]={0};

/*
*********************************************************************************************************
* 各电压通道的亮灯时间（固定地址）
*********************************************************************************************************
*/
__attribute__((at (0x30002000))) uint8_t g_single_time_t[BOARD_MAX][12]={0};


/*
*********************************************************************************************************
* 信号灯结构定义：参数
* 包含：类型、方向、相位、颜色
*********************************************************************************************************
*/
// Params_t数组
Params_t params[Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX][COLOR_MAX] = {0};

// SingleColorLight_t数组
SingleColorLight_t single_color_light[Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX][COLOR_MAX] = {0};

// PhaseLight_t数组
PhaseLight_t phase_light[Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX] = {0};

// RoadLight_t数组
RoadLight_t road_light[Type_MAX][DIR_MAX][ROAD_MAX] = {0};

// DirectionLight_t数组
DirectionLight_t direction_light[Type_MAX][DIR_MAX] = {0};

// LightType_t数组
LightType_t light_type[Type_MAX] = {0};

// 信号灯总结构
SingleLight_t single_light = {0};

/*
*********************************************************************************************************
* 配置数据结构定义：用于存储
* 包含：参数类型、板号、通道、信号灯类型、方向、道路类型、相位、颜色
*********************************************************************************************************
*/
ConfigData_t g_config_data = {0};

/*
*********************************************************************************************************
* 配时结构定义：用于存储
* 包含：数量、信号灯类型、方向、道路类型、相位、颜色
*********************************************************************************************************
*/
TimingData_t g_timing_data = {0};

ErrorCode_t g_error_code = {0};

/*
*********************************************************************************************************
* 各通道平均电流计算，用于故障判断
*********************************************************************************************************
*/
#define CURRENT_AVERAGE_TIME  3000 // 10min = 5*60*10(100ms基数)

typedef struct __attribute__((aligned(4)))
{
	float sum[Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX][COLOR_MAX][CTD_MAX];      // 电流累计值
	uint32_t count[Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX][COLOR_MAX][CTD_MAX];  // 采样次数
} CurrentAvgData_t;
CurrentAvgData_t current_sum_data = {0};
CurrentAvgData_t current_test_watch = {0}; // 用于观测电流数据
/*
*********************************************************************************************************
* 电流数据结构定义：用于存储
* 包含：信号灯类型、方向、相位、颜色
*********************************************************************************************************
*/
CurrentData_t  current_data = {0}; 


uint32_t start_code_time = 0;
uint32_t end_code_time = 0;
uint32_t run_code_time = 0;

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
	uint16_t fault_detection_started = 0;	// 标记是否已经开始故障检测
	uint8_t index;
   
	single_led_init_memory();
	single_data_init(); // 初始化数据变量
	single_led_timer_init();	// 初始化定时器
	single_current_state_init(); // 初始化电流状态结构体	
	
//	single_cmd_board_data(single_send_buf,&single_send_len);  
	
	single_light_channel_config_test();
//	single_load_config_from_flash(); // 读取配置信息

	// 创建信号量
	xSemaphore_CT_Recal = xSemaphoreCreateBinary();
	// 发送信号量
	xSemaphoreGive(xSemaphore_CT_Recal);
	
	
	while(1)
	{
		for(index = 0;index<BOARD_MAX;index++)
		{
//			Single_Send_Data((borad_id_t)index,single_send_buf,single_send_len);
			single_deal_board_data(index);
		}
		
		// 测试
//		single_ch2_light_timer_run();
//		single_non_motor_update_current_test();
//		single_non_motor_calculate_current_average_test();

		// 测试运行时间（1ms内）
		start_code_time = bsp_GetRunTime();
//		single_led_timer_run();
//		single_update_current_average_enhanced();
//		single_calculate_current_average();
//		single_timing_assign_function();  // 更新时间
		single_check_single_light_status_test();
		
		// 电流、配时检测任务
		single_current_times_detect_task();

		fault_detection_started++;
		if (fault_detection_started >= STARTUP_CHECK_DELAY_MS)
		{		
			fault_detection_started = STARTUP_CHECK_DELAY_MS;
			single_led_fault_detection_task(); // 故障检测任务
		}
		
		end_code_time = bsp_GetRunTime();
		run_code_time = end_code_time - start_code_time;
		iwdg_feed();          		
		vTaskDelay(100);      	   	
	}
}

/*
*********************************************************************************************************
* 函 数 名: single_data_init
* 功能描述: 初始化数据变量
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_data_init(void)
{
    uint8_t board;
    uint8_t channel;
    
    // 初始化g_singleboard_t为0
    for (board = 0; board < BOARD_MAX; board++)
    {
        for (channel = 0; channel < 12; channel++)
        {
            g_singleboard_t[board].data.current[channel] = 0;
            g_singleboard_t[board].data.voltage[channel] = 0;
            g_singleboard_t[board].data.pulse[channel] = 0;
        }
    }
    
    // 初始化g_single_time_t为0
    for (board = 0; board < BOARD_MAX; board++)
    {
        for (channel = 0; channel < 12; channel++)
        {
            g_single_time_t[board][channel] = 0;
        }
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
	RoadType_e road;
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
			
			// 遍历所有道路类型：主道/辅道
			for (road = ROAD_MAIN; road < ROAD_MAX; road++) 
			{
				// 将road_light数组中的元素绑定到direction_light中
				single_light.p_light_type[type]->p_direction[dir]->p_road[road] = &road_light[type][dir][road];
				
				// 遍历所有相位
				for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
				{
					// 将phase_light数组中的元素绑定到road_light中
					single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase] = &phase_light[type][dir][road][phase];
					
					// 遍历所有颜色：红/绿/黄
					for (color = COLOR_RED; color < COLOR_MAX; color++)
					{
						// 将single_color_light数组中的元素绑定到phase_light中
						single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color] = &single_color_light[type][dir][road][phase][color];
						
						// 将params数组中的元素绑定到single_color_light中
						single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params = &params[type][dir][road][phase][color];
					}
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
void Single_Bind_InpuToTraffic(ParamType_e param_type, uint8_t board_id,uint8_t ch,	
							Type_e p_type,Direction_e p_dir, 
							RoadType_e p_road,Phase_e p_phase,Color_e p_color)
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
								->p_road[p_road]
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
								->p_road[p_road]
								->p_phase[p_phase]
								->p_color[p_color]
								->p_params->voltage
		   = &g_singleboard_t[board_id].data.voltage[ch];
		
		// 将pulse数据绑定到板卡对应通道
		single_light.p_light_type[p_type]->p_direction[p_dir]
							->p_road[p_road]
							->p_phase[p_phase]
							->p_color[p_color]
							->p_params->pulse
		= &g_singleboard_t[board_id].data.pulse[ch];

		// 将配时信息据绑定到板卡对应通道
		single_light.p_light_type[p_type]->p_direction[p_dir]
							->p_road[p_road]
							->p_phase[p_phase]
							->p_color[p_color]
							->p_params->times
		   = &g_single_time_t[board_id][ch];
	break;
	default:
		break;
	}
	
	// 记录配置信息到g_config_data
	single_record_config(param_type, board_id, ch, p_type, p_dir, p_road, p_phase, p_color);
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
void single_record_config(ParamType_e param_type,uint8_t board_id, uint8_t ch, 
					Type_e p_type, Direction_e p_dir, 
					RoadType_e p_road, Phase_e p_phase, Color_e p_color)
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
				existing_item->p_road = p_road;
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
					existing_item->p_type == p_type &&
					existing_item->p_dir == p_dir &&
					existing_item->p_road == p_road &&
					existing_item->p_phase == p_phase &&
					existing_item->p_color == p_color)
				{
					// 更新现有配置的通道号
					existing_item->board_id = board_id;
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
				existing_item->p_type == p_type &&
				existing_item->p_dir == p_dir &&
				existing_item->p_road == p_road &&
				existing_item->p_phase == p_phase &&
				existing_item->p_color == p_color)
			{
				// 更新现有配置的board_id和通道号
				existing_item->board_id = board_id;
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
		item->p_road = p_road;
		item->p_phase = p_phase;
		item->p_color = p_color;
		g_config_data.config_count++;
	}
	
}

/*********************************************************************************************************
* 定时器相关定义
*********************************************************************************************************/

typedef struct {
	uint8_t  last_v; // 记录上一次的电压状态变化
	uint8_t  new_v; // 记录新的电压状态变化

	uint32_t low_time[10];   // 低电平跳变时间
	uint32_t high_time[10];  // 高电平跳变时间
	uint8_t high_jump_num;  // 高电平跳变计数
	uint8_t low_jump_num;  // 低电平跳变计数		
	uint8_t high_duration;  // 高电平持续时间
} TimerInfo_t;

// 为每个板子的每个通道定义TimerInfo_t变量
TimerInfo_t g_timer_info[BOARD_MAX][12] = {0};

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
			g_timer_info[board_id][channel].new_v = 0xFF;
			g_timer_info[board_id][channel].last_v = 0xFF; // 初始化为无效值
			g_timer_info[board_id][channel].high_jump_num = 0;
			g_timer_info[board_id][channel].low_jump_num  = 0;
			g_timer_info[board_id][channel].high_duration = 0;
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
void single_led_timer_run(void)
{
	uint8_t board_id=0;
	uint8_t channel=0;
	uint32_t elapsed_time=0;
	
	for (board_id = 0; board_id < BOARD_MAX; board_id++) 
	{
		for (channel = 0; channel < 12; channel++) 
		{
			g_timer_info[board_id][channel].new_v = g_singleboard_t[board_id].data.voltage[channel];

			if (g_timer_info[board_id][channel].new_v != g_timer_info[board_id][channel].last_v) 
			{
				if (g_timer_info[board_id][channel].new_v == 0) // 当前为低电平（点亮）
				{
					if (g_timer_info[board_id][channel].low_jump_num < 10) // 防止数组越界
					{
						g_timer_info[board_id][channel].low_time[g_timer_info[board_id][channel].low_jump_num] = bsp_GetRunTime(); // 获取时间
						g_timer_info[board_id][channel].low_jump_num++; // 低电平跳变计数
					}
				}
				else if(g_timer_info[board_id][channel].new_v == 1) // 当前为高电平（熄灭）
				{
					if (g_timer_info[board_id][channel].high_jump_num < 10) // 防止数组越界
					{
						g_timer_info[board_id][channel].high_time[g_timer_info[board_id][channel].high_jump_num] = bsp_GetRunTime(); // 获取时间
						g_timer_info[board_id][channel].high_jump_num++; // 高电平跳变计数
					}
					g_timer_info[board_id][channel].high_duration = 0; // 重置高电平持续时间
				}
				// 更新上一次的电平状态
				g_timer_info[board_id][channel].last_v = g_timer_info[board_id][channel].new_v;
			}
			else
			{
				if(g_timer_info[board_id][channel].low_jump_num > 0)  // 有低电平跳变
				{
					if(g_timer_info[board_id][channel].high_jump_num > 0)  // 有高电平跳变
					{
						if(g_timer_info[board_id][channel].new_v == 1) // 判断是否是高电平
						{
							g_timer_info[board_id][channel].high_duration++; // 高电平持续时间增加
							if(g_timer_info[board_id][channel].high_duration >= 15) // 如果高电平时间超过150*100 = 1.5s,认为这轮结束，计算时间
							{
								elapsed_time = g_timer_info[board_id][channel].high_time[g_timer_info[board_id][channel].high_jump_num-1] - g_timer_info[board_id][channel].low_time[0]; // 计算低电平持续时间
								g_single_time_t[board_id][channel] = (uint16_t)((elapsed_time+200)/1000);
								g_timer_info[board_id][channel].high_duration = 0; // 重置高电平持续时间
								g_timer_info[board_id][channel].high_jump_num = 0; // 重置高电平跳变计数
								g_timer_info[board_id][channel].low_jump_num = 0; // 重置低电平跳变计数
							}
						}
					}
				}
			}
		}
	}
}


/*
*********************************************************************************************************
* 信号灯电流结构定义：参数
* 包含：电流类型、高电平计数、脉冲检测开始时间、脉冲检测标志、数据采集开始时间、数据采集标志、数据采集完成标志、上一次脉冲值
*********************************************************************************************************
*/
// 定义电流状态结构体
typedef struct {
	CtdType_e	target_ctd_type;					// 电流类型:单灯、双灯
	uint8_t 	high_level_count;					// 高电平持续计数（每100ms递增1）
	uint32_t 	pulse_start_time;					// 脉冲检测开始时间
	uint8_t 	pulse_detected;						// 标记是否检测到脉冲信号
	uint32_t 	data_collect_start_time;	// 数据采集开始时间
	uint8_t 	data_collecting;					// 标记是否处于数据采集期
	uint8_t 	data_collect_completed;		// 标记是否已完成4秒数据采集
	uint8_t 	last_pulse_value;					// 上一次的脉冲值，用于检测跳变
} SingleCurrent_t;

// 全局变量，用于存储电流的状态
SingleCurrent_t single_current_state[Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX][COLOR_MAX] ;
/*
*********************************************************************************************************
* 函 数 名: single_current_state_init
* 功能描述: 初始化电流状态结构体
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_current_state_init(void)
{
	Type_e type;
	Direction_e dir;
	RoadType_e road;
	Phase_e phase;
	Color_e color;
	
	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 遍历所有方向：北/东/南/西
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			// 遍历所有道路类型：主道/辅道
			for (road = ROAD_MAIN; road < ROAD_MAX; road++) 
			{
				// 遍历所有相位
				for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
				{
					// 遍历所有颜色：红/绿/黄
					for (color = COLOR_RED; color < COLOR_MAX; color++)
					{
						// 初始化电流状态结构体
						single_current_state[type][dir][road][phase][color].target_ctd_type = CTD_SINGLE;         // 默认单灯
						single_current_state[type][dir][road][phase][color].high_level_count = 0;                  // 重置高电平计数器
						single_current_state[type][dir][road][phase][color].pulse_start_time = 0;                 // 重置脉冲检测开始时间
						single_current_state[type][dir][road][phase][color].pulse_detected = 0;                    // 重置脉冲检测标志
						single_current_state[type][dir][road][phase][color].data_collect_start_time = 0;          // 重置数据采集开始时间
						single_current_state[type][dir][road][phase][color].data_collecting = 0;                   // 重置数据采集标志
						single_current_state[type][dir][road][phase][color].data_collect_completed = 0;            // 重置数据采集完成标志
						single_current_state[type][dir][road][phase][color].last_pulse_value = 0;                  // 重置上一次脉冲值
					}
				}
			}
		}
	}
}

/*********************************************************************************************************
* 函 数 名: single_update_current_average_enhanced
* 功能描述: 增强版电流平均值更新（支持单灯和单灯+倒计时灯）
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_update_current_average_enhanced(void)
{
	Type_e type;
	Direction_e dir;
	RoadType_e road;
	Phase_e phase;
	Color_e color;
	float current_value;
	uint8_t pulse_value;
	
	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 遍历所有方向：北/东/南/西
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			// 遍历所有道路类型：主道/辅道
			for (road = ROAD_MAIN; road < ROAD_MAX; road++) 
			{
				// 遍历所有相位
				for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
				{
					// 遍历所有颜色：红/绿/黄
					for (color = COLOR_RED; color < COLOR_MAX; color++)
					{
						// 首先检查电压是否为低电平（灯点亮）
						if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->voltage != NULL && 
							*single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->voltage == 0)
						{
							// 重置高电平计数器
								single_current_state[type][dir][road][phase][color].high_level_count = 0;
					
							// 检查电流指针是否有效
							if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->current != NULL)
							{
								// 读取电流值
								current_value = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->current;
						
								// 检查脉冲指针是否有效
								if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->pulse != NULL)
								{
									pulse_value = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->pulse;	
							
									// 检测脉冲信号跳变
									if (single_current_state[type][dir][road][phase][color].last_pulse_value == 0 && pulse_value == 1)
									{
										// 记录脉冲检测开始时间
										single_current_state[type][dir][road][phase][color].pulse_start_time = bsp_GetRunTime();
										single_current_state[type][dir][road][phase][color].pulse_detected = 1;
									}
									
									// 更新上一次的脉冲值
									single_current_state[type][dir][road][phase][color].last_pulse_value = pulse_value;
								}
							
								// 检查脉冲检测延时
								if (single_current_state[type][dir][road][phase][color].pulse_detected)
								{
									// 计算经过的时间
									uint32_t current_time = bsp_GetRunTime();
									uint32_t elapsed_time = current_time - single_current_state[type][dir][road][phase][color].pulse_start_time;
										
									// 如果延时达到3秒（3000毫秒），则设置为单灯+倒计时灯
									if (elapsed_time >= 3000)
									{
										single_current_state[type][dir][road][phase][color].target_ctd_type = CTD_SINGLE_CTD; // 单灯+倒计时灯
										single_current_state[type][dir][road][phase][color].pulse_detected = 0; // 重置标记
										// 开始4秒数据采集期
										single_current_state[type][dir][road][phase][color].data_collect_start_time = bsp_GetRunTime();
										single_current_state[type][dir][road][phase][color].data_collecting = 1;
									}
								}
								else
								{
									// 检查是否处于数据采集状态
									if (single_current_state[type][dir][road][phase][color].data_collecting)
									{
										// 计算数据采集经过的时间
										uint32_t current_time = bsp_GetRunTime();
										uint32_t collect_elapsed_time = current_time - single_current_state[type][dir][road][phase][color].data_collect_start_time;
										
										// 如果在4秒数据采集期内，保存数据
										if (collect_elapsed_time <= 4000)
										{
											current_test_watch.sum[type][dir][road][phase][color][single_current_state[type][dir][road][phase][color].target_ctd_type] = current_value;
											current_sum_data.sum[type][dir][road][phase][color][single_current_state[type][dir][road][phase][color].target_ctd_type] += current_value;
											current_sum_data.count[type][dir][road][phase][color][single_current_state[type][dir][road][phase][color].target_ctd_type]++;
										}
									}
									// 只有在非数据采集期且未完成4秒采集的情况下，才正常保存数据
									else
									{
										// 非数据采集期，正常保存数据
										current_test_watch.sum[type][dir][road][phase][color][single_current_state[type][dir][road][phase][color].target_ctd_type] = current_value;
										current_sum_data.sum[type][dir][road][phase][color][single_current_state[type][dir][road][phase][color].target_ctd_type] += current_value;
										current_sum_data.count[type][dir][road][phase][color][single_current_state[type][dir][road][phase][color].target_ctd_type]++;
									}
								}
							}
						}
						// 当电压为高电平时，延迟1.5秒后强制置为单灯模式
						else
						{
							// 如果高电平持续时间达到1.5秒（15个100ms），则设置为单灯模式
							if (single_current_state[type][dir][road][phase][color].high_level_count >= 12)
							{
								single_current_state[type][dir][road][phase][color].target_ctd_type = CTD_SINGLE;
								single_current_state[type][dir][road][phase][color].high_level_count = 0; // 重置计数器
								single_current_state[type][dir][road][phase][color].data_collecting = 0;
							}
						}
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
	RoadType_e road;
	Phase_e phase;
	Color_e color;
	CtdType_e ctd_type;
	
	// 遍历所有CTD类型：单灯/单灯+倒计时灯
	for (ctd_type = CTD_SINGLE; ctd_type < CTD_MAX; ctd_type++)
	{
		// 遍历所有灯类型：远灯/近灯
		for (type = FAR; type < Type_MAX; type++) 
		{
			for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
			{
				for (road = ROAD_MAIN; road < ROAD_MAX; road++) 
				{
					for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
					{
						for (color = COLOR_RED; color < COLOR_MAX; color++)
						{
							// 计算平均值
							if (current_sum_data.count[type][dir][road][phase][color][ctd_type] > 0)
							{
								current_data.current[ctd_type][type][dir][road][phase][color] = \
								current_sum_data.sum[type][dir][road][phase][color][ctd_type] / current_sum_data.count[type][dir][road][phase][color][ctd_type];
							}
						}
					}
				}
			}
		}
	}
	
	// 清除电流的计算值和计数值
	for (ctd_type = CTD_SINGLE; ctd_type < CTD_MAX; ctd_type++)
	{
		for (type = FAR; type < Type_MAX; type++) 
		{
			for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
			{
				for (road = ROAD_MAIN; road < ROAD_MAX; road++) 
				{
					for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
					{
						for (color = COLOR_RED; color < COLOR_MAX; color++)
						{
							current_sum_data.sum[type][dir][road][phase][color][ctd_type] = 0.0f;
							current_sum_data.count[type][dir][road][phase][color][ctd_type] = 0;
						}
					}
				}
			}
		}
	}
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
		return 0;
	}
	else
	{
		return -1;
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
								item->p_road, item->p_phase, item->p_color);
		}
		return 0;
	}
	else
	{
		// 配置数据无效，初始化为默认值
		memset(&g_config_data, 0, sizeof(ConfigData_t));
		return -1;
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

/*********************************************************************************************************
* 函 数 名: single_clear_timing_function
* 功能描述: 清除所有信号灯配时
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_clear_timing_function(void)
{
	// 清空配置数据
	memset(&g_timing_data, 0, sizeof(TimingData_t));
	single_save_config_to_flash();
	single_clear_timing_function();
}

/*********************************************************************************************************
* 函 数 名: single_save_timing_to_flash
* 功能描述: 将配时数据保存到FLASH
* 参    数: 无
* 返 回 值: 错误状态
*********************************************************************************************************
*/
int single_save_timing_to_flash(void)
{
	// 使用文件系统保存配时数据
	if (save_stroage_single_led_timing_function(g_timing_data) == 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

/*********************************************************************************************************
* 函 数 名: single_load_timing_from_flash
* 功能描述: 从FLASH读取配时数据
* 参    数: 无
* 返 回 值: 错误状态
*********************************************************************************************************
*/
int single_load_timing_from_flash(void)
{
	// 使用文件系统读取配时数据
	if (save_read_single_led_timing_function(&g_timing_data) == 0)
	{
		// 配时数据读取成功
		return 0;
	}
	else
	{
		// 配时数据无效，初始化为默认值
		memset(&g_timing_data, 0, sizeof(TimingData_t));
		return -1;
	}
}

/*********************************************************************************************************
* 函 数 名: single_timing_assign_function
* 功能描述: 根据single_light中的配对信息，将g_single_time_t中对应的配时时间赋值给g_timing_data
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_timing_assign_function(void)
{
    uint8_t type;
    uint8_t dir;
    uint8_t road;
    uint8_t phase;
    uint8_t color;
    uint8_t current_time;
    uint8_t is_same = 0;
    uint8_t timing_index;
    
    // 遍历所有现有的配时索引，检查是否与single_light中的数据相同
    for (timing_index = 0; timing_index < g_timing_data.timing_count; timing_index++)
    {
        // 检查当前配时索引是否有数据
        if (g_timing_data.times[timing_index][0][0][0][0][0] != 0)
        {
            // 重置相同标志
            is_same = 1;
            
            // 遍历所有灯的组合
            for (type = 0; type < Type_MAX; type++)
            {
                if (single_light.p_light_type[type] != NULL)
                {
                    for (dir = 0; dir < DIR_MAX; dir++)
                    {
                        if (single_light.p_light_type[type]->p_direction[dir] != NULL)
                        {
                            for (road = 0; road < ROAD_MAX; road++)
                            {
                                if (single_light.p_light_type[type]->p_direction[dir]->p_road[road] != NULL)
                                {
                                    for (phase = 0; phase < PHASE_MAX; phase++)
                                    {
                                        if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase] != NULL)
                                        {
                                            for (color = 0; color < COLOR_MAX; color++)
                                            {
                                                if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color] != NULL)
                                                {
                                                    // 检查必要的指针
                                                    if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params != NULL &&
                                                        single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->times != NULL)
                                                    {
                                                        // 获取当前配时时间
                                                        current_time = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->times;
                                                        
                                                        // 检查是否有不同
                                                        if (g_timing_data.times[timing_index][type][dir][road][phase][color] != current_time)
                                                        {
                                                            is_same = 0;
                                                            break; // 跳出颜色循环
                                                        }
                                                    }
                                                }
                                            }
                                            if (!is_same) break; // 跳出相位循环
                                        }
                                    }
                                    if (!is_same) break; // 跳出道路循环
                                }
                            }
                            if (!is_same) break; // 跳出方向循环
                        }
                    }
                    if (!is_same) break; // 跳出类型循环
                }
            }
            
            // 如果找到相同的配时，直接返回
            if (is_same)
            {
                return;
            }
        }
    }
    
    // 如果没有找到相同的配时，在g_timing_data中新增配时数据
    // 检查是否已达到最大配时数量
    if (g_timing_data.timing_count >= MAX_TIMING_ITEMS)
    {
        // 已达到最大配时数量，无法新增
        return;
    }
    
    // 直接使用timing_count作为新的配时索引
    uint8_t new_timing_index = g_timing_data.timing_count;
    
    // 确保索引有效
    if (new_timing_index < MAX_TIMING_ITEMS)
    {
        // 复制当前配时数据到新的索引位置
        for (type = 0; type < Type_MAX; type++)
        {
            if (single_light.p_light_type[type] != NULL)
            {
                for (dir = 0; dir < DIR_MAX; dir++)
                {
                    if (single_light.p_light_type[type]->p_direction[dir] != NULL)
                    {
                        for (road = 0; road < ROAD_MAX; road++)
                        {
                            if (single_light.p_light_type[type]->p_direction[dir]->p_road[road] != NULL)
                            {
                                for (phase = 0; phase < PHASE_MAX; phase++)
                                {
                                    if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase] != NULL)
                                    {
                                        for (color = 0; color < COLOR_MAX; color++)
                                        {
                                            if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color] != NULL)
                                            {
                                                // 检查必要的指针
                                                if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params != NULL &&
                                                    single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->times != NULL)
                                                {
                                                    // 获取当前配时时间并更新到新索引
                                                    current_time = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->times;
                                                    g_timing_data.times[new_timing_index][type][dir][road][phase][color] = current_time;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 计数值加1
        g_timing_data.timing_count++;
    }
}

/*********************************************************************************************************
* 函 数 名: single_save_current_to_flash
* 功能描述: 将电流平均值保存到FLASH
* 参    数: 无
* 返 回 值: 错误状态
*********************************************************************************************************
*/
int single_save_current_to_flash(void)
{	
    // 使用文件系统保存电流数据
    if (save_stroage_single_led_current_function(current_data) == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*********************************************************************************************************
* 函 数 名: single_load_current_from_flash
* 功能描述: 从FLASH读取电流平均值
* 参    数: 无
* 返 回 值: 错误状态
*********************************************************************************************************
*/
int single_load_current_from_flash(void)
{
    // 使用文件系统读取电流数据
    if (save_read_single_led_current_function(&current_data) == 0)
    {
        // 电流数据读取成功
        return 0;
    }
    else
    {
        // 电流数据无效，初始化为默认值
        memset(&current_data, 0, sizeof(CurrentData_t));
        return -1;
    }
}



/*
*********************************************************************************************************
* 函 数 名: single_current_times_recalculate
* 功能描述: 电流、配时重新检测（1h间隔）
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_current_times_recalculate(void)
{
	static uint16_t recal_time = 0;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	recal_time++;
	if(recal_time >= SINGLE_C_T_RECAL_TIME)	
	{
		recal_time= 0;
		// 发送信号量
		xSemaphoreGiveFromISR(xSemaphore_CT_Recal,&xHigherPriorityTaskWoken);
		// 如果发生了上下文切换，执行必要的上下文切换
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

/*
*********************************************************************************************************
* 函 数 名: single_current_times_detect_task
* 功能描述: 信号灯电流、配时检测任务（10分钟检测，状态机模式）
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_current_times_detect_task(void)
{
	static uint8_t detect_state = 0;
	static uint32_t start_time = 0;
	static uint32_t current_time = 0;
	static uint16_t detect_time = 0;
	
	switch (detect_state)
	{
		case 0:
			if (xSemaphoreTake(xSemaphore_CT_Recal, 0) == pdTRUE)
			{
				start_time = bsp_GetRunTime();
				detect_time = 0;
				detect_state = 1;
			}
			break;
			
		case 1:
			current_time = bsp_GetRunTime();
			detect_time = (current_time - start_time) / 1000;
		
			single_led_timer_run();
			single_update_current_average_enhanced();
	
			if (detect_time >= SINGLE_C_T_SAVE_TIME) 
			{
				printf("detect_time: %d\n", detect_time);
				detect_state = 2;
			}
			break;
			
		case 2:
			single_calculate_current_average();
			single_timing_assign_function();  // 更新时间
			// single_save_timing_to_flash();
			// single_save_current_to_flash();
				
//			xSemaphoreGive(xSemaphore_CT_Recal);
			
			detect_state = 0;
			break;
	}
}


/*********************************************************************************************************
* 故障检测相关函数
*********************************************************************************************************/

void single_led_fault_detection_task(void)
{
	static ErrorCode_t led_off_fault = {0}; // 全灭故障
	static ErrorCode_t led_rg_fault = {0};  // 红绿同时亮故障
	ErrorCode_t fault_flag = {0};
	uint8_t led_off_state = 0;
	uint8_t led_rg_state = 0;

	// 1、信号灯全灭、单方向信号灯不亮
	fault_flag = single_check_signal_status();
	// 检查故障状态是否改变
	if (fault_flag.fault != led_off_fault.fault ||
		fault_flag.type != led_off_fault.type ||
		fault_flag.dir != led_off_fault.dir)
	{
		led_off_state = 1;
	}
	
	if (fault_flag.fault & (1<<LED_ALL_NO_LIGHT))
	{
		// 所有灯全灭，添加故障标志
		if (led_off_state)
		{
			app_report_information_immediately();
			// printf("led_all_off\n");
		}
	}
	else if (fault_flag.fault & (1<<LED_PART_NO_LIGHT))
	{
		// 单方向信号灯不亮，添加故障标志
		if (led_off_state)
		{
			app_report_information_immediately();
			// printf("led_dir_off: 0x%2x...0x%2x\n", fault_flag.type,fault_flag.dir);
		}
	}
	else
	{
	}	
	// 更新上一次的故障状态
	led_off_fault = fault_flag;

	// 2、检查相位红绿同时亮
	fault_flag = single_check_phase_red_green_simultaneous();
	// 检查故障状态是否改变
	if (fault_flag.type != led_rg_fault.type ||
		fault_flag.dir != led_rg_fault.dir ||
		fault_flag.road != led_rg_fault.road ||
		fault_flag.phase != led_rg_fault.phase)
	{
		led_rg_state = 1;
	}

	if (fault_flag.fault & (1<<LED_RED_GREEN_SAME_LIGHT))
	{
		if (led_rg_state)
		{
			// app_report_information_immediately();
			printf("rg_sim: 0x%02x...0x%02x...0x%02x...0x%02x\n", fault_flag.type,fault_flag.dir,fault_flag.road,fault_flag.phase);
		}
	}
	// 更新上一次的故障状态
	led_rg_fault = fault_flag;
}

/*********************************************************************************************************
* 函 数 名: single_check_signal_status
* 功能描述: 检查所有信号灯状态-所有灯全灭、单方向信号灯不亮、单个相位灯不亮
* 参    数: fault_mask - 输出参数，用于存储有问题的方向掩码（每一位代表一个方向是否有问题）
*           type_out - 输出参数，用于存储有问题的信号灯类型
*           dir_out - 输出参数，用于存储有问题的方向（仅返回第一个发现的问题方向）
* 返 回 值: ErrorCode_t - 错误码结构，包含故障类型、方向等信息
*********************************************************************************************************/
ErrorCode_t single_check_signal_status(void)
{
	Type_e type;                     // 灯类型：远灯/近灯
	Direction_e dir;                 // 方向：北/东/南/西
	RoadType_e road;                 // 道路类型：主道/辅道
	Phase_e phase;                   // 相位
	Color_e color;                   // 颜色：红/绿/黄
	float current_value;             // 电流值
	uint8_t voltage_value;          // 电压值
	uint8_t light_dir_off;          // 方向是否全灭
	uint8_t light_all_off = 1;      // 所有灯是否全灭

	uint8_t has_valid_channel = 0;   // 是否有有效通道
	ErrorCode_t error_code = {0};

	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 遍历所有方向：北/东/南/西
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			// 初始化方向全灭标志
			light_dir_off = 1;
			has_valid_channel = 0;
			
			// 遍历所有道路类型：主道/辅道
			for (road = ROAD_MAIN; road < ROAD_MAX; road++) 
			{
				// 遍历所有相位
				for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
				{
					// 遍历所有颜色：红/绿/黄
					for (color = COLOR_RED; color < COLOR_MAX; color++)
					{
						// 检查电压和电流指针是否有效
						if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->voltage != NULL &&
							  single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->current != NULL)
						{

							// 标记有有效通道
							has_valid_channel = 1;
							
							// 读取电压和电流值
							voltage_value = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->voltage;
							current_value = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->current;
							
							// 检查是否有灯亮：有电压(低电平)且有电流>10ma才认为灯亮
							if (voltage_value == 0 && current_value > 10)
							{
								// 只要有一个灯亮，方向就不是全灭，系统也不是全灭
								light_dir_off = 0;
								light_all_off = 0;
							}
						}
					}
					// 如果方向不全灭，继续检查下一个相位
					if (!light_dir_off)
					{
						break;
					}
				}
				// 如果方向不全灭，继续检查下一个道路类型
				if (!light_dir_off)
				{
					break;
				}
			}
			
			// 方向是否全灭
			if (light_dir_off && has_valid_channel )
			{				
				TrafficFault_Set(TRAFFIC_PART_NO_LIGHT, type, dir, 0, 0, 0);
				
				error_code.fault |= 1<<LED_PART_NO_LIGHT;
				error_code.type |= 1<<type;
				error_code.dir |= 1 <<dir;
				error_code.road = 0;
				error_code.phase = 0;
				error_code.color = 0;
			}
			else 
			{
				TrafficFault_Clear(TRAFFIC_PART_NO_LIGHT, type, dir, 0, 0, 0);
			}
		}
	}
	
	// 全灭返回错误码
	if (light_all_off)
	{
		error_code.fault |= 1<<LED_ALL_NO_LIGHT;
		TrafficFault_Set(TRAFFIC_ALL_NO_LIGHT,0,0,0,0,0);
		// 清除所有的方向不亮故障
		for (type = FAR; type < Type_MAX; type++) 
		{
			for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
			{
				TrafficFault_Clear(TRAFFIC_PART_NO_LIGHT, type, dir, 0, 0, 0);
			}
		}
	}
	else
	{
		TrafficFault_Clear(TRAFFIC_ALL_NO_LIGHT,0,0,0,0,0);
	}

	return error_code;
}


/*********************************************************************************************************
* 函 数 名: single_check_phase_red_green_simultaneous
* 功能描述: 检查是否有相位同时出现红绿灯亮的情况（同一个相位）
* 参    数: 无
* 返 回 值: ErrorCode_t - 错误码结构，包含故障类型、方向等信息
*********************************************************************************************************
*/

// 定义绿灯电压状态结构体
typedef struct {
	uint8_t prev;
	uint32_t change_time;
	uint8_t changed;
} GreenVoltageState_t;

// 全局绿灯电压状态
static GreenVoltageState_t green_voltage_state[Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX] = {0};

ErrorCode_t single_check_phase_red_green_simultaneous(void)
{
	Type_e type;
	Direction_e dir;
	RoadType_e road;
	Phase_e phase;
	float red_current = 0.0f;
	float green_current = 0.0f;
	uint8_t red_on = 0;
	uint8_t green_on = 0;
	uint32_t current_time = bsp_GetRunTime();
	uint8_t green_voltage = 255;
	ErrorCode_t error_code = {0};	

	// 遍历single_light结构检查每个相位
	// 遍历所有灯类型：远灯/近灯
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 遍历所有方向：北/东/南/西
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++) 
		{
			// 遍历所有道路类型：主道/辅道
			for (road = ROAD_MAIN; road < ROAD_MAX; road++) 
			{
				// 遍历所有相位
				for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
				{
					// 初始化标志
					red_on = 0;
					green_on = 0;
					
					// 检查红灯是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_RED]->p_params->current != NULL)	
					{
						// 读取红灯电流
						red_current = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_RED]->p_params->current;
						// 电流大于等于10ma认为灯亮
						if (red_current >= 10)
						{
							red_on = 1;
						}
					}
					
					// 检查绿灯是否有效
					if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_GREEN]->p_params->current != NULL)
					{
						// 读取绿灯电流
						green_current = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_GREEN]->p_params->current;
						// 电流大于等于10ma认为灯亮
						if (green_current >= 10)
						{
							green_on = 1;
						}
					}
					
					// 读取绿灯电压并检测变化
					if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_GREEN]->p_params->voltage != NULL)
					{
						green_voltage = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_GREEN]->p_params->voltage;
						
						// 检测绿灯电压变化
						if (green_voltage_state[type][dir][road][phase].prev != 255 && 
							green_voltage_state[type][dir][road][phase].prev != green_voltage)
						{
							green_voltage_state[type][dir][road][phase].change_time = current_time;
							green_voltage_state[type][dir][road][phase].changed = 1;
						}
						// 更新绿灯电压状态
						green_voltage_state[type][dir][road][phase].prev = green_voltage;
					}
					
					// 检查是否在绿灯电压变化后的1秒内
					if (green_voltage_state[type][dir][road][phase].changed)
					{
						if (current_time - green_voltage_state[type][dir][road][phase].change_time > 1200) // 1.5秒
						{
							// 1秒后重置状态
							green_voltage_state[type][dir][road][phase].changed = 0;
						}
					}
					
					// 检查是否红绿同亮，排除绿灯电压变化后的1秒内
					if (red_on && green_on && !green_voltage_state[type][dir][road][phase].changed)
					{
						printf("led_phase_rg_simultaneous: %d,%d,%d,%d\n",type,dir,road,phase);
						// 打印电流地址、大小
						printf("red_current: %p, %f\n", single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_RED]->p_params->current, red_current);
						printf("green_current: %p, %f\n", single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_GREEN]->p_params->current, green_current);
						// 打印电压
						printf("red_voltage: %p, %d\n", single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_RED]->p_params->voltage, *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_RED]->p_params->voltage);
						printf("green_voltage: %p, %d\n", single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[COLOR_GREEN]->p_params->voltage, green_voltage);
						
						TrafficFault_Set(TRAFFIC_RED_GREEN_SAME_LIGHT, type, dir, road, phase, 0);
						
						error_code.fault = 1<<LED_RED_GREEN_SAME_LIGHT;
						error_code.type |= 1<<type;
						error_code.dir |= 1<<dir;
						error_code.road |= 1<<road;
						error_code.phase |= 1<<phase;
						error_code.color = 0;
					}
					else
					{
						TrafficFault_Clear(TRAFFIC_RED_GREEN_SAME_LIGHT, type, dir, road, phase, 0);
					}
				}
			}
		}
	}
	return error_code;	
}
/*********************************************************************************************************
* 函 数 名: single_check_single_light_status
* 功能描述: 检查是否有单灯故障
* 参    数: 无
* 返 回 值: ErrorCode_t - 单灯故障掩码
*           低16位表示单灯故障掩码，每一位对应一个单灯是否有故障
*           位0-11对应PHASE_LEFT到PHASE_SERVICE共12个相位
*           位12-13对应方向0-3（保留）
*           位14-15对应类型0-1（保留）
*********************************************************************************************************/

// 定义电压结构体
typedef struct {
	uint8_t prev;   
	uint32_t high_change_time;
	uint8_t high_changed;
	uint32_t low_change_time;
	uint8_t low_changed;
} VoltageState_t;

// 定义脉冲结构体
typedef struct {
	uint8_t prev;     // 前一次脉冲状态
	uint32_t detect_time; // 脉冲检测开始时间
	uint8_t detected;
} PulseState_t;

// 定义测试电压状态结构体
typedef struct {
	VoltageState_t voltage_state;
	PulseState_t pulse_state;
	CtdType_e cur_ctd_type;	// 目标类型，保持状态
} CurrentCheckState_t;

CurrentCheckState_t single_check_state[Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX][COLOR_MAX] = {0};

// 定义电流阈值
#define CURRENT_THRESHOLD_1 10.0f  // 电流阈值1
#define CURRENT_THRESHOLD_2 50.0f  // 电流阈值2

ErrorCode_t single_check_single_light_status(void)
{
	Type_e type;
	Direction_e dir;
	RoadType_e road;
	Phase_e phase;
	Color_e color;
	uint8_t voltage_value;
	float current_value;
	uint8_t pulse_value = 0;
	uint32_t current_time = bsp_GetRunTime();
	const uint32_t TRANSITION_TIME = 1000; // 切换过程时间，单位毫秒
	const uint32_t PULSE_WAIT_TIME = 5000; // 脉冲检测后等待时间，单位毫秒
	ErrorCode_t error_code = {0};

	for (type = FAR; type < Type_MAX; type++)
	{
		for (dir = DIR_NORTH; dir < DIR_MAX; dir++)
		{
			for (road = ROAD_MAIN; road < ROAD_MAX; road++)
			{
				for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++)
				{
					for (color = COLOR_RED; color < COLOR_MAX; color++)
					{
						if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->voltage == NULL ||
							single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->current == NULL)
						{
							continue;
						}

						voltage_value = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->voltage;
						current_value = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->current;

						// 读取脉冲值
						if (single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->pulse != NULL)
						{
							pulse_value = *single_light.p_light_type[type]->p_direction[dir]->p_road[road]->p_phase[phase]->p_color[color]->p_params->pulse;

							// 检测脉冲跳变
						if (p_state[type][dir][road][phase][color].p_prev != 255 && 
							p_state[type][dir][road][phase][color].p_prev != pulse_value)
						{
							p_state[type][dir][road][phase][color].p_detected = 1;
							p_state[type][dir][road][phase][color].p_detect_time = current_time;
							p_state[type][dir][road][phase][color].p_wait = 1;
						}

						// 更新前一次脉冲状态
						p_state[type][dir][road][phase][color].p_prev = pulse_value;
						}

						// 检查是否在脉冲检测后的等待期内
						if (p_state[type][dir][road][phase][color].p_wait)
						{
							if (current_time - p_state[type][dir][road][phase][color].p_detect_time < PULSE_WAIT_TIME)
							{
								// 在等待期内，跳过检测
								continue;
							}
							else
							{
								// 等待期结束，重置状态
								p_state[type][dir][road][phase][color].p_detected = 0;
								p_state[type][dir][road][phase][color].p_wait = 0;
							}
						}

						// 检测电压变化
						uint8_t in_transition = 0;
						
						if (v_state[type][dir][road][phase][color].v_prev != 255 && 
							v_state[type][dir][road][phase][color].v_prev != voltage_value)
						{
							v_state[type][dir][road][phase][color].v_change_time = current_time;
							v_state[type][dir][road][phase][color].v_changed = 1;
						}
						
						// 检查是否在切换过程中
						if (v_state[type][dir][road][phase][color].v_changed)
						{
							if (current_time - v_state[type][dir][road][phase][color].v_change_time < TRANSITION_TIME)
							{
								in_transition = 1;
							}
							else
							{
								v_state[type][dir][road][phase][color].v_changed = 0;
							}
						}
						
						// 更新电压状态
						v_state[type][dir][road][phase][color].v_prev = voltage_value;
						
						// 在切换过程中不进行判断
						if (!in_transition)
						{
							if (voltage_value == 0 && current_value <= CURRENT_THRESHOLD_1)
							{
								error_code.fault |= 1<<LED_SINGLE_NO_LIGHT;
								error_code.type |= 1<<type;
								error_code.dir |= 1<<dir;
								error_code.road |= 1<<road;
								error_code.phase |= 1<<phase;
								error_code.color |= 1<<color;
								TrafficFault_Set(TRAFFIC_SINGLE_NO_LIGHT, type, dir, road, phase, color);
							}
							else if (voltage_value == 0 && (current_value > CURRENT_THRESHOLD_1 && current_value < CURRENT_THRESHOLD_2))
							{
								error_code.fault |= 1<<LED_SINGLE_PART_LIGHT;
								error_code.type |= 1<<type;
								error_code.dir |= 1<<dir;
								error_code.road |= 1<<road;
								error_code.phase |= 1<<phase;
								error_code.color |= 1<<color;
								TrafficFault_Set(TRAFFIC_SINGLE_PART_LIGHT, type, dir, road, phase, color);
							}
							else
							{
								TrafficFault_Clear(TRAFFIC_SINGLE_NO_LIGHT, type, dir, road, phase, color);
								TrafficFault_Clear(TRAFFIC_SINGLE_PART_LIGHT, type, dir, road, phase, color);
							}
						}
					}
				}
			}
		}
	}

	return error_code;
}



/*********************************************************************************************************
* 测试检测相关函数
*********************************************************************************************************/


void single_light_channel_config_test(void)
{
	// 电流
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_2,0,FAR,DIR_EAST,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_2,1,FAR,DIR_EAST,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_3,0,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_LEFT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_3,1,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_LEFT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_3,2,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_LEFT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_3,3,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_STRAIGHT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_3,4,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_STRAIGHT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_3,5,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_STRAIGHT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_3,6,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_PERSON1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_CURRENT,BOARD_3,7,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_PERSON1,COLOR_GREEN);
	
	// 电压-板1
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,11,FAR,DIR_NORTH,ROAD_MAIN,PHASE_LEFT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,10,FAR,DIR_NORTH,ROAD_MAIN,PHASE_LEFT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,9,FAR,DIR_NORTH,ROAD_MAIN,PHASE_LEFT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,8,FAR,DIR_NORTH,ROAD_MAIN,PHASE_STRAIGHT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,7,FAR,DIR_NORTH,ROAD_MAIN,PHASE_STRAIGHT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,6,FAR,DIR_NORTH,ROAD_MAIN,PHASE_STRAIGHT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,5,FAR,DIR_NORTH,ROAD_MAIN,PHASE_PERSON1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,4,FAR,DIR_NORTH,ROAD_MAIN,PHASE_PERSON1,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,3,FAR,DIR_NORTH,ROAD_MAIN,PHASE_PERSON1,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,2,FAR,DIR_NORTH,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,1,FAR,DIR_NORTH,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_1,0,FAR,DIR_NORTH,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_GREEN);

	// 电压-板2
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,11,FAR,DIR_EAST,ROAD_MAIN,PHASE_LEFT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,10,FAR,DIR_EAST,ROAD_MAIN,PHASE_LEFT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,9,FAR,DIR_EAST,ROAD_MAIN,PHASE_LEFT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,8,FAR,DIR_EAST,ROAD_MAIN,PHASE_STRAIGHT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,7,FAR,DIR_EAST,ROAD_MAIN,PHASE_STRAIGHT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,6,FAR,DIR_EAST,ROAD_MAIN,PHASE_STRAIGHT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,5,FAR,DIR_EAST,ROAD_MAIN,PHASE_PERSON1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,4,FAR,DIR_EAST,ROAD_MAIN,PHASE_PERSON1,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,3,FAR,DIR_EAST,ROAD_MAIN,PHASE_PERSON1,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,2,FAR,DIR_EAST,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,1,FAR,DIR_EAST,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_2,0,FAR,DIR_EAST,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_GREEN);
	// 电压-板3
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,11,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_LEFT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,10,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_LEFT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,9,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_LEFT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,8,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_STRAIGHT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,7,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_STRAIGHT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,6,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_STRAIGHT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,5,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,4,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,3,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,2,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_PERSON1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,1,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_PERSON1,COLOR_YELLOW);	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_3,0,FAR,DIR_SOUTH,ROAD_MAIN,PHASE_PERSON1,COLOR_GREEN);
	// 电压-板4
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,11,FAR,DIR_WEST,ROAD_MAIN,PHASE_LEFT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,10,FAR,DIR_WEST,ROAD_MAIN,PHASE_LEFT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,9,FAR,DIR_WEST,ROAD_MAIN,PHASE_LEFT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,8,FAR,DIR_WEST,ROAD_MAIN,PHASE_STRAIGHT,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,7,FAR,DIR_WEST,ROAD_MAIN,PHASE_STRAIGHT,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,6,FAR,DIR_WEST,ROAD_MAIN,PHASE_STRAIGHT,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,5,FAR,DIR_WEST,ROAD_MAIN,PHASE_PERSON1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,4,FAR,DIR_WEST,ROAD_MAIN,PHASE_PERSON1,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,3,FAR,DIR_WEST,ROAD_MAIN,PHASE_PERSON1,COLOR_GREEN);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,2,FAR,DIR_WEST,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_RED);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,1,FAR,DIR_WEST,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_YELLOW);
	Single_Bind_InpuToTraffic( PARAM_VOLTAGE,BOARD_4,0,FAR,DIR_WEST,ROAD_MAIN,PHASE_NONMOTOR1,COLOR_GREEN);


}
/*
*********************************************************************************************************
* 函 数 名: single_ch2_light_timer_run
* 功能描述: 板2通道2时间检测测试
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
TimerInfo_t single_ch2_light_time = {0}; // 静态变量，保持状态
void single_ch2_light_timer_run(void)
{
	uint32_t elapsed_time = 0;
	single_ch2_light_time.new_v = g_singleboard_t[BOARD_2].data.voltage[2];

	if (single_ch2_light_time.new_v != single_ch2_light_time.last_v) 
	{
		if (single_ch2_light_time.new_v == 0) // 当前为低电平（点亮）
		{
			if (single_ch2_light_time.low_jump_num < 10) // 防止数组越界
			{
				single_ch2_light_time.low_time[single_ch2_light_time.low_jump_num] = bsp_GetRunTime(); // 获取时间
				single_ch2_light_time.low_jump_num++; // 低电平跳变计数
			}
		}
		else if(single_ch2_light_time.new_v == 1) // 当前为高电平（熄灭）
		{
			if (single_ch2_light_time.high_jump_num < 10) // 防止数组越界
			{
				single_ch2_light_time.high_time[single_ch2_light_time.high_jump_num] = bsp_GetRunTime(); // 获取时间
				single_ch2_light_time.high_jump_num++; // 高电平跳变计数
			}
			single_ch2_light_time.high_duration = 0; // 重置高电平持续时间
		}
		// 更新上一次的电平状态
		single_ch2_light_time.last_v = single_ch2_light_time.new_v;
	}
	else
	{
		if(single_ch2_light_time.low_jump_num > 0)  // 有低电平跳变
		{
			if(single_ch2_light_time.high_jump_num > 0)  // 有高电平跳变
			{
				if(single_ch2_light_time.new_v == 1) // 判断是否是高电平
				{
					single_ch2_light_time.high_duration++; // 高电平持续时间增加
					if(single_ch2_light_time.high_duration >= 15) // 如果高电平时间超过150*100 = 1.5s,认为这轮结束，计算时间
					{
						elapsed_time = single_ch2_light_time.high_time[single_ch2_light_time.high_jump_num-1] - single_ch2_light_time.low_time[0]; // 计算低电平持续时间
						g_single_time_t[BOARD_2][2] = (uint16_t)((elapsed_time+200)/1000);
						single_ch2_light_time.high_duration = 0; // 重置高电平持续时间
						single_ch2_light_time.high_jump_num = 0; // 重置高电平跳变计数
						single_ch2_light_time.low_jump_num = 0; // 重置低电平跳变计数
					}
				}
			}
		}
	}
}


/*********************************************************************************************************
* 函 数 名: single_non_motor_update_current_test
* 功能描述: 非机动车测试函数
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
Params_t test_nmotor[COLOR_MAX] = {
	{.current = &g_singleboard_t[2].data.current[6], .voltage = &g_singleboard_t[2].data.voltage[2], .pulse = &g_singleboard_t[2].data.pulse[2]},
	{.current = &g_singleboard_t[2].data.current[8], .voltage = &g_singleboard_t[2].data.voltage[1], .pulse = &g_singleboard_t[2].data.pulse[1]},
	{.current = &g_singleboard_t[2].data.current[7], .voltage = &g_singleboard_t[2].data.voltage[0], .pulse = &g_singleboard_t[2].data.pulse[0]}
};
/* 新增全局平均电流变量，用于记录每个灯类型、方向、相位、颜色下的平均电流值 */
float g_avg_current[COLOR_MAX][CTD_MAX] = {0.0f};
uint32_t g_avg_current_count[COLOR_MAX][CTD_MAX] = {0};
float g_sum_current[COLOR_MAX][CTD_MAX] = {0.0f};

// 定义非机动车测试的状态结构体
typedef struct {
	CtdType_e target_ctd_type;         // 目标类型，保持状态
	uint8_t high_level_count;          // 高电平持续计数（每100ms递增1）
	uint32_t pulse_start_time;         // 脉冲检测开始时间
	uint8_t pulse_detected;            // 标记是否检测到脉冲信号
	uint32_t data_collect_start_time;  // 数据采集开始时间
	uint8_t data_collecting;           // 标记是否处于数据采集期
	uint8_t data_collect_completed;    // 标记是否已完成4秒数据采集
} NonMotorTestState_t;

// 全局变量，用于存储每个颜色的测试状态
NonMotorTestState_t non_motor_test_state[COLOR_MAX] = {
	{CTD_SINGLE, 0, 0, 0, 0, 0, 0},
	{CTD_SINGLE, 0, 0, 0, 0, 0, 0},
	{CTD_SINGLE, 0, 0, 0, 0, 0, 0}
};

void single_non_motor_update_current_test(void)
{
	Color_e color;
	float current_value;
	uint8_t pulse_value;
	// 静态变量，用于存储上一次的脉冲值，检测跳变
	static uint8_t last_pulse_value[COLOR_MAX] = {0};
	
	// 遍历所有颜色：红/绿/黄
	for (color = COLOR_RED; color < COLOR_MAX; color++)
	{
		// 首先检查电压是否为低电平（灯点亮）
		if((test_nmotor[color].voltage != NULL)&&(*test_nmotor[color].voltage == 0))
		{
			non_motor_test_state[color].high_level_count = 0;
			
			// 检查电流指针是否有效
			if (test_nmotor[color].current != NULL)
			{
					current_value = *test_nmotor[color].current;
					
					if (test_nmotor[color].pulse != NULL)
					{
						pulse_value = *test_nmotor[color].pulse;
						
						// 检测脉冲信号跳变
						if (last_pulse_value[color] == 0 && pulse_value == 1)
						{
							// 记录脉冲检测开始时间
							non_motor_test_state[color].pulse_start_time = bsp_GetRunTime();
							non_motor_test_state[color].pulse_detected = 1;
						}
						last_pulse_value[color] = pulse_value;// 更新上一次的脉冲值
					}
					// 检查脉冲检测延时
					if (non_motor_test_state[color].pulse_detected)
					{
						// 计算经过的时间
						uint32_t current_time = bsp_GetRunTime();
						uint32_t elapsed_time = current_time - non_motor_test_state[color].pulse_start_time;
						
						// 如果延时达到3秒（3000毫秒），则设置为单灯+倒计时灯
						if (elapsed_time >= 3000)
						{
							non_motor_test_state[color].target_ctd_type = CTD_SINGLE_CTD; // 单灯+倒计时灯
							non_motor_test_state[color].pulse_detected = 0; // 重置标记
							// 开始4秒数据采集期
							non_motor_test_state[color].data_collect_start_time = bsp_GetRunTime();
							non_motor_test_state[color].data_collecting = 1;
						}
					}
					else
					{
						if (non_motor_test_state[color].data_collecting)
						{
							// 计算数据采集经过的时间
							uint32_t current_time = bsp_GetRunTime();
							uint32_t collect_elapsed_time = current_time - non_motor_test_state[color].data_collect_start_time;
							
							// 如果在4秒数据采集期内，保存数据
							if (collect_elapsed_time <= 4000)
							{
								g_avg_current[color][non_motor_test_state[color].target_ctd_type] = current_value;
								g_sum_current[color][non_motor_test_state[color].target_ctd_type] += current_value;
								g_avg_current_count[color][non_motor_test_state[color].target_ctd_type]++;
							}
						}
						else
						{
							// 非数据采集期，正常保存数据
							g_avg_current[color][non_motor_test_state[color].target_ctd_type] = current_value;
							g_sum_current[color][non_motor_test_state[color].target_ctd_type] += current_value;
							g_avg_current_count[color][non_motor_test_state[color].target_ctd_type]++;
						}
					}
				}
			}
			// 当电压为高电平时，延迟1.5秒后强制置为单灯模式
			else
			{
				non_motor_test_state[color].high_level_count++;
				// 如果高电平持续时间达到1.5秒（15个100ms），则设置为单灯模式
				if (non_motor_test_state[color].high_level_count >= 12)
				{
					non_motor_test_state[color].target_ctd_type = CTD_SINGLE;
					non_motor_test_state[color].high_level_count = 0; // 重置计数器
					non_motor_test_state[color].data_collecting = 0;
				}
			}
		}

}
/*********************************************************************************************************
* 函 数 名: single_non_motor_calculate_current_average_test
* 功能描述: 计算电流平均值 
* 参    数: 无
* 返 回 值: 无
*********************************************************************************************************
*/
float non_motor_avg_current[CTD_MAX][COLOR_MAX] = {0}; 
void single_non_motor_calculate_current_average_test(void)
{
	Color_e color;
	CtdType_e ctd_type;
	
	// 遍历所有CTD类型和颜色
	for (color = COLOR_RED; color < COLOR_MAX; color++)
	{
		for (ctd_type = CTD_SINGLE; ctd_type < CTD_MAX; ctd_type++)
		{
			// 计算平均值
			if (g_avg_current_count[color][ctd_type] > 0)
			{
				non_motor_avg_current[color][ctd_type] = g_sum_current[color][ctd_type] / g_avg_current_count[color][ctd_type];
			}
			else
			{
//				non_motor_avg_current[ctd_type][color] = 0.0f;
			}
		}
	}
//	g_sum_current[target_ctd_type[color]][color] = 0.0f;
//	g_avg_current_count[target_ctd_type[color]][color] = 0;
}

/*********************************************************************************************************
* 函 数 名: single_check_single_light_status
* 功能描述: 检查是否有单灯故障
* 参    数: 无
* 返 回 值: ErrorCode_t - 单灯故障掩码
*           低16位表示单灯故障掩码，每一位对应一个单灯是否有故障
*           位0-11对应PHASE_LEFT到PHASE_SERVICE共12个相位
*           位12-13对应方向0-3（保留）
*           位14-15对应类型0-1（保留）
*********************************************************************************************************
*/

// 定义电压结构体
typedef struct {
	uint8_t prev;   
	uint32_t high_change_time;
	uint8_t high_changed;
	uint32_t low_change_time;
	uint8_t low_changed;
} TestVoltageState_t;

// 定义脉冲结构体
typedef struct {
	uint8_t prev;     // 前一次脉冲状态
	uint32_t detect_time; // 脉冲检测开始时间
	uint8_t detected;
} TestPulseState_t;

// 定义测试电压状态结构体
typedef struct {
	VoltageState_t voltage_state;
	TestPulseState_t pulse_state;
	CtdType_e cur_ctd_type;	// 目标类型，保持状态
} TestCheckState_t;

// 全局测试电压状态
TestCheckState_t test_voltage_state[COLOR_MAX] = {0};

// 定义电流阈值
#define CURRENT_THRESHOLD_1 40.0f  // 电流阈值1
#define CURRENT_THRESHOLD_2 40.0f  // 电流阈值2

ErrorCode_t single_check_single_light_status_test(void)
{
	Color_e color;
	uint8_t voltage_value;
	float current_value;
	uint8_t pulse_value = 0;
	uint32_t current_time = bsp_GetRunTime();
	uint32_t transition_time;
	const uint32_t TRANSITION_TIME = 500; // 切换过程时间，单位毫秒
	const uint32_t PULSE_WAIT_TIME = 3000; // 脉冲检测后等待时间，单位毫秒
	ErrorCode_t error_code = {0};
	float current_threshold[CTD_MAX] = {CURRENT_THRESHOLD_1, CURRENT_THRESHOLD_2}; // 电流判断阈值

	for (color = COLOR_RED; color < COLOR_MAX; color++)
	{
		if (test_nmotor[color].voltage == NULL ||\
			test_nmotor[color].current == NULL ||\
			test_nmotor[color].pulse == NULL)
		{
			continue;
		}

		voltage_value = *test_nmotor[color].voltage;
		current_value = *test_nmotor[color].current;
		pulse_value = *test_nmotor[color].pulse;
		// 检测脉冲信号跳变
		if (test_voltage_state[color].pulse_state.prev == 0 && pulse_value == 1)
		{
			// 记录脉冲检测开始时间
			test_voltage_state[color].pulse_state.detect_time = bsp_GetRunTime();
			test_voltage_state[color].pulse_state.detected = 1; 

		}
		test_voltage_state[color].pulse_state.prev = pulse_value;// 更新上一次的脉冲值

		// 检查是否在脉冲检测后的等待期内
		if (test_voltage_state[color].pulse_state.detected)	
		{
			if (current_time - test_voltage_state[color].pulse_state.detect_time < PULSE_WAIT_TIME)
			{
				// 在等待期内，跳过检测
				continue;
			}
			else
			{
				// 等待期结束，重置状态
				test_voltage_state[color].cur_ctd_type = CTD_SINGLE_CTD;
				test_voltage_state[color].pulse_state.detected = 0;
			}
		}
		else 
		{
			// 低电平跳变 0 -> 1
			if (test_voltage_state[color].voltage_state.prev == 0 && 
				voltage_value == 1)
			{
				test_voltage_state[color].voltage_state.low_change_time = current_time; 
				test_voltage_state[color].voltage_state.low_changed = 1;
			} 
			// 高电平跳变 1 -> 0
			if (test_voltage_state[color].voltage_state.prev == 1 && 
				voltage_value == 0)
			{
				test_voltage_state[color].voltage_state.high_change_time = current_time; 
				test_voltage_state[color].voltage_state.high_changed = 1;
			} 
			test_voltage_state[color].voltage_state.prev = voltage_value;

			// 低电平跳变后延时1.5秒判断
			if (test_voltage_state[color].voltage_state.low_changed == 1)
			{
				transition_time = current_time - test_voltage_state[color].voltage_state.high_change_time;
				if (transition_time < 600)
				{
					continue;
				}
				else
				{
					if((transition_time >= 1500)&&(voltage_value == 1))
					{
						test_voltage_state[color].cur_ctd_type = CTD_SINGLE;	
						test_voltage_state[color].voltage_state.low_changed = 0;
					}
				}
			}
			
			// 检查是否在切换过程中
			if (test_voltage_state[color].voltage_state.high_changed == 1)
			{
				if (current_time - test_voltage_state[color].voltage_state.high_change_time < TRANSITION_TIME)
				{
					// 在等待期内，跳过检测
					continue;
				}
				else
				{
					test_voltage_state[color].voltage_state.high_changed = 0;
				}
			}
			else 
			{
				if(voltage_value == 0)
				{
					// current_threshold 电流判断阈值小于阈值*25%为不亮，25%-60%为部分灯亮，60%以上为全灯亮
					if ( current_value <= current_threshold[test_voltage_state[color].cur_ctd_type]*0.25f)
					{
						printf("led_no_light: %d,%f,%d\n",color,current_value,voltage_value);
						error_code.fault |= 1<<LED_SINGLE_NO_LIGHT;
						error_code.color |= 1<<color;
					}
					else if (current_value <= current_threshold[test_voltage_state[color].cur_ctd_type]*0.6f)
					{
						printf("led_part_light: %d,%f,%d,%d\n",color,current_value,voltage_value,transition_time);
						error_code.fault |= 1<<LED_SINGLE_PART_LIGHT;
						error_code.color |= 1<<color;
					}
					else if (current_value > current_threshold[test_voltage_state[color].cur_ctd_type]*2)
					{
						printf("led_light: %d,%f,%d\n",color,current_value,voltage_value);
					}	
				}		
			}			
		}
	}
	return error_code;
}






