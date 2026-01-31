#ifndef _SINGLE_H_
#define _SINGLE_H_

#include "./SYSTEM/sys/sys.h"

/*********************************************************************************************************
* 类型定义
*********************************************************************************************************/
typedef enum
{
	BOARD_1 = 0,
	BOARD_2,
	BOARD_3,
	BOARD_4,
	
	BOARD_MAX
}borad_id_t;

/*********************************************************************************************************
* 板数据结构
*********************************************************************************************************/
typedef struct 
{
  float   current[24];
  uint8_t voltage[12];
  uint8_t pulse[12];
}board_t;

/*********************************************************************************************************
* 单板数据结构
*********************************************************************************************************/
typedef struct
{
	uint8_t  cmd;   // 命令
	uint8_t  size;  // 数据长度
	board_t  data;  // 数据内容
} single_data_t;

/*********************************************************************************************************
* 灯类型枚举
*********************************************************************************************************/
typedef enum {
	FAR = 0,   // 远灯
	NEAR,      // 近灯
	Type_MAX   // 总数
} Type_e;

/*********************************************************************************************************
* 方向枚举
*********************************************************************************************************/
typedef enum {
    DIR_NORTH = 0,   // 北
    DIR_EAST,        // 东
    DIR_SOUTH,       // 南
    DIR_WEST,        // 西
    DIR_MAX          // 方向总数
} Direction_e;

/*********************************************************************************************************
* 道路类型枚举
*********************************************************************************************************/
typedef enum {
    ROAD_MAIN = 0,   // 主道
    ROAD_AUXILIARY,  // 辅道
    ROAD_MAX         // 道路类型总数
} RoadType_e;

/*********************************************************************************************************
* 相位枚举
*********************************************************************************************************/
typedef enum {
	PHASE_LEFT = 0,      // 左转
	PHASE_STRAIGHT,      // 直行
	PHASE_RIGHT,         // 右转
	PHASE_PERSON1,       // 人行1
	PHASE_PERSON2,       // 人行2
	PHASE_NONMOTOR1,     // 非机动车1
	PHASE_NONMOTOR2,     // 非机动车2
	PHASE_U_TURN,        // 掉头
	PHASE_VARIABLE,      // 可变
	PHASE_REVERSE,       // 逆向
	PHASE_TIDAL,         // 潮汐
	PHASE_MAX            // 相位总数
} Phase_e;

/*********************************************************************************************************
* 颜色枚举
*********************************************************************************************************/
typedef enum {
    COLOR_RED = 0,   // 红色
    COLOR_YELLOW,    // 黄色
    COLOR_GREEN,     // 绿色
    COLOR_MAX        // 颜色总数
} Color_e;

/*********************************************************************************************************
* 参数类型枚举
*********************************************************************************************************/
typedef enum {
    PARAM_CURRENT = 0,
    PARAM_VOLTAGE,
    PARAM_MAX
} ParamType_e;

/*********************************************************************************************************
* 灯参数结构体
*********************************************************************************************************/
typedef struct {
	float 	*current;    // 电流值指针（指向g_singleboard_t中的电流数据）
	uint8_t *voltage;    // 电压值指针（指向g_singleboard_t中的电压数据）
	uint8_t *pulse;   // 脉冲值指针（指向g_singleboard_t中的脉冲数据）
	uint8_t *times;    // 配时值指针（指向g_single_time_t中的配时数据）
} Params_t;

/*********************************************************************************************************
* 电流参数类型枚举
*********************************************************************************************************/
typedef enum {
    CTD_SINGLE = 0,      // 单灯
    CTD_SINGLE_CTD,      // 单灯+倒计时灯
    CTD_MAX
} CtdType_e;


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
* 道路灯结构定义：主道/辅道各包含相位
*********************************************************************************************************
*/
typedef struct {
    PhaseLight_t *p_phase[PHASE_MAX]; // 指向多个相位的指针数组
} RoadLight_t;

/*
*********************************************************************************************************
* 方向灯结构定义：北/东/南/西各包含道路类型
*********************************************************************************************************
*/
typedef struct {
    RoadLight_t *p_road[ROAD_MAX]; // 指向主道/辅道的指针数组
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
* 信号灯总结构：包含类型+方向+道路+相位+颜色
*********************************************************************************************************
*/
typedef struct {
    LightType_t *p_light_type[Type_MAX]; // 指向远灯/近灯的指针数组
} SingleLight_t;


/*********************************************************************************************************
* 配置数据结构（用于保存到FLASH）
*********************************************************************************************************/
typedef struct __attribute__((aligned(4)))
{
	// 保存Single_Bind_InpuToTraffic的配置参数
	ParamType_e param_type;     // 参数类型
	uint8_t board_id;           // 板ID
	uint8_t ch;                 // 通道号
	Type_e p_type;              // 灯类型
	Direction_e p_dir;          // 方向
	RoadType_e p_road;          // 道路类型
	Phase_e p_phase;            // 相位
	Color_e p_color;            // 颜色
} ConfigItem_t;

// 最大配置项数量
#define MAX_CONFIG_ITEMS (Type_MAX*DIR_MAX*ROAD_MAX*PHASE_MAX*COLOR_MAX)

/*********************************************************************************************************
* 配置数据结构（用于保存到FLASH）
*********************************************************************************************************/
typedef struct __attribute__((aligned(4)))
{
	uint32_t config_count;                      // 配置项数量
	ConfigItem_t config_items[MAX_CONFIG_ITEMS]; // 配置项数组
} ConfigData_t;


/*********************************************************************************************************
* 电流数据结构（用于保存到FLASH）
*********************************************************************************************************/
typedef struct __attribute__((aligned(4)))
{
	float current[CTD_MAX][Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX][COLOR_MAX]; 
} CurrentData_t;

/*********************************************************************************************************
* 配时数据结构（用于保存到FLASH）
*********************************************************************************************************/

// 最大配时项数量
#define MAX_TIMING_ITEMS (20)

typedef struct __attribute__((aligned(4)))
{
    uint32_t timing_count;                      // 配时项数量
    uint8_t times[MAX_TIMING_ITEMS][Type_MAX][DIR_MAX][ROAD_MAX][PHASE_MAX][COLOR_MAX]; // 配时时间（秒）
} TimingData_t;

// 定义错误码
typedef struct __attribute__((aligned(4)))
{
	uint8_t fault;
	uint8_t type;
	uint8_t dir;
	uint8_t road;
	uint16_t phase;
	uint8_t color;
} ErrorCode_t;

typedef enum {
	LED_NORMAL = 0,
	LED_ALL_NO_LIGHT ,  
	LED_PART_NO_LIGHT,   
	LED_RED_GREEN_SAME_LIGHT,   
	LED_SINGLE_NO_LIGHT,   
	LED_SINGLE_PART_LIGHT,  

} LedError_e;         


/*********************************************************************************************************
* 函数声明
*********************************************************************************************************/
void single_task_function(void);

void single_cmd_board_data(uint8_t *data, uint8_t *len);

uint8_t single_deal_board_data(uint8_t id);

void single_led_init_memory(void);
void single_data_init(void);

void single_led_timer_init(void);
void single_led_timer_run(void);


// 电流平均值相关函数
void single_current_state_init(void);
void single_update_current_average_enhanced(void); // 增强版电流平均值更新
void single_calculate_current_average(void);
int single_save_current_to_flash(void);
int single_load_current_from_flash(void);

// 配置管理函数
int single_save_config_to_flash(void);
int single_load_config_from_flash(void);
void single_clear_config_function(void);
void single_record_config(ParamType_e param_type,
					uint8_t board_id, uint8_t ch, 
					Type_e p_type, Direction_e p_dir, 
					RoadType_e p_road, Phase_e p_phase, Color_e p_color);
void Single_Bind_InpuToTraffic(ParamType_e param_type, 
							uint8_t board_id,uint8_t ch,	
							Type_e p_type,Direction_e p_dir, 
							RoadType_e p_road,Phase_e p_phase,Color_e p_color);

// 配时管理函数
void single_clear_timing_function(void);
int single_save_timing_to_flash(void);
int single_load_timing_from_flash(void);
void single_timing_assign_function(void);

// 电流、配时检测函数
void single_current_times_recalculate(void);
void single_current_times_detect_task(void);


// 板卡数据接收函数
void single_recv_board_data(uint8_t id, uint8_t *data, uint8_t len);
void single_recv_board_data_0(uint8_t *data, uint8_t len);
void single_recv_board_data_1(uint8_t *data, uint8_t len);
void single_recv_board_data_2(uint8_t *data, uint8_t len);
void single_recv_board_data_3(uint8_t *data, uint8_t len);
 




// 故障检测函数
void single_led_fault_detection_task(void);

int single_check_signal_led_status(void);
// 相位灯状态检查函数
uint32_t single_check_phase_red_green_simultaneous(void);
uint8_t single_led_check_all_off(void);
ErrorCode_t single_check_signal_status(void);



void single_current_times_recalculate(void);


// 测试函数
void single_light_channel_config_test(void);
void single_ch2_light_timer_run(void);
void single_non_motor_update_current_test(void);
void single_non_motor_calculate_current_average_test(void);


#endif


