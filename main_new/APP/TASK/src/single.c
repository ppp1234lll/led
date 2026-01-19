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
    Lpuart1_Send_Data,  
    Usart1_Send_Data,    
    Usart4_Send_Data,    
    Uart5_Send_Data     
};		
		
single_data_t g_singleboard_t[BOARD_MAX]={0};
uint8_t g_single_time_t[BOARD_MAX][12]={0};

uint8_t  single_send_buf[16];  
uint8_t  single_send_len;
uint8_t  single_recv_buf[BOARD_MAX][256];
uint16_t single_recv_sta[BOARD_MAX];  


void Single_Send_Data(borad_id_t num, uint8_t *data, uint16_t len);
/*
*********************************************************************************************************
* 最底层：单个颜色灯的状态+参数（核心扩展）
* 每个颜色灯包含：亮灭状态 + 电流/电压/时间/脉冲参数
*********************************************************************************************************
*/
typedef struct {
    Params_t *p_params; // 灯参数指针（指向电流/电压/时间/脉冲参数）
} SingleColorLight_t;

/*
*********************************************************************************************************
* 单个相位的3种颜色灯（包含参数）
*********************************************************************************************************
*/
typedef struct {
    SingleColorLight_t *p_color[COLOR_MAX]; // 指向3种颜色灯的指针数组
} PhaseLight_t;

/*
*********************************************************************************************************
* 单个方位的9个相位
*********************************************************************************************************
*/
typedef struct {
    PhaseLight_t *p_phase[PHASE_MAX]; // 指向9个相位的指针数组
} DirectionLight_t;

/*
*********************************************************************************************************
* 单种灯类型（远灯/近灯）的4个方位
*********************************************************************************************************
*/
typedef struct {
    DirectionLight_t *p_direction[DIR_MAX]; // 指向4个方位的指针数组
} LightType_t;

/*
*********************************************************************************************************
* 顶层：所有红绿灯（远灯+近灯）
*********************************************************************************************************
*/
typedef struct {
    LightType_t *p_light_type[Type_MAX]; // 指向远灯/近灯的指针数组
} SingleLight_t;


// 全局变量定义
// Params_t变量
static Params_t params[Type_MAX][DIR_MAX][PHASE_MAX][COLOR_MAX] = {0};

// SingleColorLight_t变量
static SingleColorLight_t single_color_light[Type_MAX][DIR_MAX][PHASE_MAX][COLOR_MAX] = {0};

// PhaseLight_t变量
static PhaseLight_t phase_light[Type_MAX][DIR_MAX][PHASE_MAX] = {0};

// DirectionLight_t变量
static DirectionLight_t direction_light[Type_MAX][DIR_MAX] = {0};

// LightType_t变量
static LightType_t light_type[Type_MAX] = {0};

// 顶层红绿灯对象
static SingleLight_t single_light = {0};


/*
*********************************************************************************************************
* 函 数 名: single_task_function
* 功能说明: 检测板线程
* 形    参: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_task_function(void)
{
	uint8_t send_status = 0;
	printf("run here\n");
	uint8_t index;
	
	single_led_init_memory();
	single_cmd_board_data(single_send_buf,&single_send_len);  
	
  Single_Bind_InpuToTraffic( PARAM_CURRENT,g_singleboard_t,BOARD_4,1,
				                     FAR,DIR_EAST,PHASE_LEFT,COLOR_YELLOW);

	
	// 初始化计时信息
	single_led_timer_init();
	
	while(1)
	{
//		for(index = 0;index<BOARD_MAX;index++)
//		{
//			Uart_Send_Data((borad_id_t)index,single_send_buf,single_send_len);
//		}
		
		Lpuart1_Send_Data(single_send_buf,single_send_len);
		if(single_deal_board_data(BOARD_4) == 0)
		{
			send_status = 0;
		}
		
		single_led_timer_run();
		
		iwdg_feed();        			
		vTaskDelay(100);    	  	
	}
}

/*
*********************************************************************************************************
* 函 数 名: Single_Send_Data
* 功能说明: 获取检测板数据
* 形    参: 无
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
* 功能说明: 获取检测板数据
* 形    参: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_cmd_board_data(uint8_t *data, uint8_t *len)
{
	uint8_t index = 0;
	uint8_t crc   = 0;
	
	/* 数据头 */
	data[index++] = SEND_HEAD_BYTE1; 
	data[index++] = SEND_HEAD_BYTE2;

	/* 命令 */
	data[index++] = SEND_CMD;
	
	/* 数据长度 */
	data[index++] = 0x01;

	/* 数据内容 */
	data[index++] = 0x01;

	/* crc校验 */
	crc = calc_crc8(&data[2],index-2);
	data[index++] = crc;
	
	/* 数据尾 */
	data[index++] = SEND_END_BYTE1;
	data[index++] = SEND_END_BYTE2;
	*len = index;
}
/*
*********************************************************************************************************
* 函 数 名: single_recv_board_data
* 功能说明: 接收检测板数据
* 形    参: 无
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
* 函 数 名: single_deal_board_data
* 功能说明: 处理数据
* 形    参: 无
* 返 回 值: 无
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
* 函 数 名: singleled_init_memory
* 功能说明: 初始化 内存
* 形    参: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_led_init_memory(void)
{
	Type_e type;
	Direction_e dir;
	Phase_e phase;
	Color_e color;
	
	// 遍历所有类型（远灯/近灯）
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 将single_light中的指针指向全局变量
		single_light.p_light_type[type] = &light_type[type];
		
		// 遍历所有方向（东/西/南/北）
		for (dir = DIR_EAST; dir < DIR_MAX; dir++) 
		{
			// 将light_type中的指针指向全局变量
			single_light.p_light_type[type]->p_direction[dir] = &direction_light[type][dir];
			
			// 遍历所有相位
			for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
			{
				// 将direction_light中的指针指向全局变量
				single_light.p_light_type[type]->p_direction[dir]->p_phase[phase] = &phase_light[type][dir][phase];
				
				// 遍历所有颜色（红/绿/黄）
				for (color = COLOR_RED; color < COLOR_MAX; color++)
				{
					// 将phase_light中的指针指向全局变量
					single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color] = &single_color_light[type][dir][phase][color];
					
					// 将single_color_light中的指针指向全局变量
					single_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params = &params[type][dir][phase][color];
				}
			}
		}
	}
}

/*
*********************************************************************************************************
* 函 数 名: Single_Bind_InpuToTraffic
* 功能说明: 将single_data_t中的数据绑定到single_light中
* 形    参: param_type - 参数类型
*           single_data - 检测板数据
*           p_type - 灯类型（远灯/近灯）
*           p_dir - 方向
*           p_phase - 相位
*           p_color - 颜色
*           ch - 通道索引
* 返 回 值: 无
*********************************************************************************************************
*/
void Single_Bind_InpuToTraffic(ParamType_e param_type, single_data_t *single_data, 
															 uint8_t board_id,uint8_t ch,	
															 Type_e p_type,Direction_e p_dir, 
															 Phase_e p_phase,Color_e p_color)
{
	// 检查board_id是否有效
	if (board_id >= BOARD_MAX)
	{
		return;
	}
	
	// 检查single_data是否为NULL
	if (single_data == NULL)
	{
		return;
	}
	
	// 根据参数类型，让结构体参数指针指向数组元素地址
	switch (param_type) 
	{
			case PARAM_CURRENT:
			// 检查通道索引是否在有效范围内
			if (ch >= 24)  
				return;

			// 将current指针指向g_singleboard_t中的电流数据
			single_light.p_light_type[p_type]->p_direction[p_dir]
															->p_phase[p_phase]
															->p_color[p_color]
															->p_params->current
		   = &single_data[board_id].data.current[ch];
			break;
			
		case PARAM_VOLTAGE:
			// 检查通道索引是否在有效范围内
			if (ch >= 12)  
				return;

			// 将voltage指针指向g_singleboard_t中的电压数据
			single_light.p_light_type[p_type]->p_direction[p_dir]
															->p_phase[p_phase]
															->p_color[p_color]
															->p_params->voltage
		   = &single_data[board_id].data.voltage[ch];
			
			// 将pulse指针指向g_singleboard_t中的脉冲数据
			single_light.p_light_type[p_type]->p_direction[p_dir]
															->p_phase[p_phase]
															->p_color[p_color]
															->p_params->pulse
		   = &single_data[board_id].data.pulse[ch];
			break;
		default:
			break;
	}
}

/*********************************************************************************************************
* 计时相关函数
*********************************************************************************************************/

typedef struct {
	uint32_t count;  // 开始计时的时间戳（单位：ms）
	uint8_t  last_v; // 上一次的电压值，用于检测变化
	uint8_t  new_v; // 上一次的电压值，用于检测变化
} TimerInfo_t;

// 每一块板的计时信息结构体
typedef struct {
    TimerInfo_t timer[12]; // 每一路电压的计时信息
} BoardTimerInfo_t;

// 全局计时信息变量
BoardTimerInfo_t g_timer_info[BOARD_MAX] = {0};

/*
*********************************************************************************************************
* 函 数 名: single_led_timer_init
* 功能说明: 初始化计时信息
* 形    参: 无
* 返 回 值: 无
*********************************************************************************************************
*/
void single_led_timer_init(void)
{
	uint8_t board_id;
	uint8_t channel;

	// 初始化所有板的所有通道的计时信息
	for (board_id = 0; board_id < BOARD_MAX; board_id++) {
		for (channel = 0; channel < 12; channel++) {
			g_timer_info[board_id].timer[channel].count = 0;
			g_timer_info[board_id].timer[channel].new_v = 0xFF;
			g_timer_info[board_id].timer[channel].last_v = 0xFF; // 初始值设为无效值
		}
	}
}
/*
*********************************************************************************************************
* 函 数 名: single_led_timer_run
* 功能说明: 更新计时信息
* 形    参: 无
* 返 回 值: 无
*********************************************************************************************************
*/
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
