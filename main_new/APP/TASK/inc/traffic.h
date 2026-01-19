#ifndef _TRAFFIC_H_
#define _TRAFFIC_H_

#include "./SYSTEM/sys/sys.h"
#include "./TASK/inc/single.h"

// 种类
typedef enum {
	FAR = 0,   // 远灯
	NEAR,      // 近灯
	Type_MAX   // 灯
} Type_e;

// 方向枚举（东西南北）
typedef enum {
    DIR_EAST = 0,    // 东
    DIR_WEST,        // 西
    DIR_SOUTH,       // 南
    DIR_NORTH,       // 北
    DIR_MAX          // 方向总数（边界检查用）
} Direction_e;

// 红绿灯类型枚举（左转、直行、右转、人行）
typedef enum {
	PHASE_LEFT = 0,  // 左转灯
	PHASE_STRAIGHT,  // 直行灯
	PHASE_RIGHT,     // 右转灯
	PHASE_PERSON1,   // 人行灯
	PHASE_PERSON2,   // 人行灯
	PHASE_NONMOTOR,  // 非机动车 
  PHASE_CTD,       // 倒计时
	PHASE_VARIABLE,  // 可变车道
	PHASE_WATE,      // 待行
	PHASE_MAX        // 相位总数
} Phase_e;

// 红绿灯颜色枚举（红、绿、黄）
typedef enum {
    COLOR_RED = 0,   // 红灯
    COLOR_GREEN,     // 绿灯
    COLOR_YELLOW,    // 黄灯
    COLOR_MAX        // 颜色总数
} Color_e;

typedef enum {
    PARAM_CURRENT = 0,
    PARAM_VOLTAGE,
    PARAM_MAX
} ParamType_e;


// 灯的电气+时序参数结构体（每个颜色灯的专属参数）
typedef struct {
	float current;    // 电流（单位：A，浮点型保证精度）
	uint8_t voltage;    // 电压（单位：V）
	uint8_t pulse;   // 脉冲数/脉冲频率（根据实际需求定义，16位足够）
	uint16_t time_s; // 持续时间（单位：ms，32位适配STM32计时）
} Params_t;


void traffic_init_memory(void);
void Bind_InpuToTraffic( ParamType_e param_type, single_data_t *single_data, 
												 Type_e p_type,Direction_e p_dir, 
												 Phase_e p_phase,Color_e p_color, 
                         uint8_t ch);



#endif
