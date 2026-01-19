#include "appconfig.h"
#include "./TASK/inc/traffic.h"
#include "./TASK/inc/single.h"

// 最底层：单个颜色灯的状态+参数（核心扩展）
// 每个颜色灯包含：亮灭状态 + 电流/电压/时间/脉冲参数
typedef struct {
    Params_t *p_params; // 灯参数指针（指向电流/电压/时间/脉冲参数）
} SingleColorLight_t;

// 单个相位的3种颜色灯（包含参数）
typedef struct {
    SingleColorLight_t *p_color[COLOR_MAX]; // 指向3种颜色灯的指针数组
} PhaseLight_t;

// 单个方位的9个相位
typedef struct {
    PhaseLight_t *p_phase[PHASE_MAX]; // 指向9个相位的指针数组
} DirectionLight_t;

// 单种灯类型（远灯/近灯）的4个方位
typedef struct {
    DirectionLight_t *p_direction[DIR_MAX]; // 指向4个方位的指针数组
} LightType_t;

// 顶层：所有红绿灯（远灯+近灯）
typedef struct {
    LightType_t *p_light_type[Type_MAX]; // 指向远灯/近灯的指针数组
} TrafficLight_t;


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
static TrafficLight_t traffic_light = {0};


// 初始化traffic_light结构体中的指针数组内存
void traffic_init_memory(void)
{
	Type_e type;
	Direction_e dir;
	Phase_e phase;
	Color_e color;
	
	// 遍历所有类型（远灯/近灯）
	for (type = FAR; type < Type_MAX; type++) 
	{
		// 将traffic_light中的指针指向全局变量
		traffic_light.p_light_type[type] = &light_type[type];
		
		// 遍历所有方向（东/西/南/北）
		for (dir = DIR_EAST; dir < DIR_MAX; dir++) 
		{
			// 将light_type中的指针指向全局变量
			traffic_light.p_light_type[type]->p_direction[dir] = &direction_light[type][dir];
			
			// 遍历所有相位
			for (phase = PHASE_LEFT; phase < PHASE_MAX; phase++) 
			{
				// 将direction_light中的指针指向全局变量
				traffic_light.p_light_type[type]->p_direction[dir]->p_phase[phase] = &phase_light[type][dir][phase];
				
				// 遍历所有颜色（红/绿/黄）
				for (color = COLOR_RED; color < COLOR_MAX; color++)
				{
					// 将phase_light中的指针指向全局变量
					traffic_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color] = &single_color_light[type][dir][phase][color];
					
					// 将single_color_light中的指针指向全局变量
					traffic_light.p_light_type[type]->p_direction[dir]->p_phase[phase]->p_color[color]->p_params = &params[type][dir][phase][color];
				}
			}
		}
	}
}

void Bind_InpuToTraffic( ParamType_e param_type, single_data_t *single_data, 
												 Type_e p_type,Direction_e p_dir, 
												 Phase_e p_phase,Color_e p_color, 
                         uint8_t ch)
{
	// 空指针检查（STM32必做，避免HardFault）
	if (single_data == NULL) {
			return;
		}

	// 根据参数类型，让结构体参数指针指向数组元素地址
	switch (param_type) 
	{
			case PARAM_CURRENT:
			// 检查current_index是否在有效范围内
			if (ch >= 24) 	return;

			traffic_light.p_light_type[p_type]->p_direction[p_dir]
																				->p_phase[p_phase]
																				->p_color[p_color]
																				->p_params->current
		   = single_data->data.current[ch];
			break;
			
		case PARAM_VOLTAGE:
			// 检查current_index是否在有效范围内
			if (ch >= 12) 	return;

			traffic_light.p_light_type[p_type]->p_direction[p_dir]
																				->p_phase[p_phase]
																				->p_color[p_color]
																				->p_params->voltage
		   = single_data->data.voltage[ch];
		
			traffic_light.p_light_type[p_type]->p_direction[p_dir]
																				->p_phase[p_phase]
																				->p_color[p_color]
																				->p_params->pulse
		   = single_data->data.pulse[ch];
			break;
		default:
			break;
	}
}




