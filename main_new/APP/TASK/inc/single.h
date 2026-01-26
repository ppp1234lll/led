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
* 相位枚举
*********************************************************************************************************/
typedef enum {
	PHASE_LEFT = 0,  // 左转向
	PHASE_STRAIGHT,  // 直行车
	PHASE_RIGHT,     // 右转向
	PHASE_PERSON1,   // 行人灯
	PHASE_PERSON2,   // 行人灯
	PHASE_NONMOTOR1, // 非机动车灯 
	PHASE_NONMOTOR2, // 非机动车灯 
  PHASE_CTD,       // 倒计时
	PHASE_VARIABLE,  // 可变车道
	PHASE_WATE,      // 待行
	PHASE_SERVICE,   // 辅道
	PHASE_MAX        // 相位总数
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
	float *current;    // 电流值指针（指向g_singleboard_t中的电流数据）
	uint8_t *voltage;    // 电压值指针（指向g_singleboard_t中的电压数据）
	uint8_t *pulse;   // 脉冲值指针（指向g_singleboard_t中的脉冲数据）
} Params_t;

/*********************************************************************************************************
* 电流数据结构（用于保存到FLASH）
*********************************************************************************************************/
typedef struct __attribute__((aligned(4)))
{
	float current[Type_MAX][DIR_MAX][PHASE_MAX][COLOR_MAX];
} CurrentData_t;

/*********************************************************************************************************
* 电流平均值数据结构（用于1小时平均值计算）
*********************************************************************************************************/
typedef struct __attribute__((aligned(4)))
{
	float sum[Type_MAX][DIR_MAX][PHASE_MAX][COLOR_MAX];      // 电流累计值
	uint32_t count[Type_MAX][DIR_MAX][PHASE_MAX][COLOR_MAX];  // 采样次数
} CurrentAverageData_t;

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
	Phase_e p_phase;            // 相位
	Color_e p_color;            // 颜色
} ConfigItem_t;

// 最大配置项数量
#define MAX_CONFIG_ITEMS (Type_MAX*DIR_MAX*PHASE_MAX*COLOR_MAX)

/*********************************************************************************************************
* 配置数据结构（用于保存到FLASH）
*********************************************************************************************************/
typedef struct __attribute__((aligned(4)))
{
	uint32_t config_count;                      // 配置项数量
	ConfigItem_t config_items[MAX_CONFIG_ITEMS]; // 配置项数组
} ConfigData_t;

/*********************************************************************************************************
* FLASH配置区域定义
*********************************************************************************************************/
#define FLASH_CONFIG_START_ADDR 0x081E0000 // FLASH配置区域起始地址
#define FLASH_CONFIG_SIZE       sizeof(ConfigData_t) // 配置数据大小

/*********************************************************************************************************
* 函数声明
*********************************************************************************************************/
void single_task_function(void);

void single_cmd_board_data(uint8_t *data, uint8_t *len);

uint8_t single_deal_board_data(uint8_t id);

void single_led_init_memory(void);
void Single_Bind_InpuToTraffic(ParamType_e param_type, 
															 uint8_t board_id,uint8_t ch,	
															 Type_e p_type,Direction_e p_dir, 
															 Phase_e p_phase,Color_e p_color);

void single_led_timer_init(void);
void single_led_timer_run(void);
void single_ch2_light_timer_run(void);
// 电流数据收集函数
void single_collect_current_data(CurrentData_t *data);

// 电流平均值相关函数
void single_update_current_average(void);
void single_calculate_current_average(void);

// 相位灯状态检查函数
uint32_t single_check_phase_red_green_simultaneous(void);

// 配置管理函数
int single_save_config_to_flash(void);
int single_load_config_from_flash(void);
void single_record_config(ParamType_e param_type, 
					uint8_t board_id, uint8_t ch, 
					Type_e p_type, Direction_e p_dir, 
					Phase_e p_phase, Color_e p_color);

// 板卡数据接收函数
void single_recv_board_data(uint8_t id, uint8_t *data, uint8_t len);
void single_recv_board_data_0(uint8_t *data, uint8_t len);
void single_recv_board_data_1(uint8_t *data, uint8_t len);
void single_recv_board_data_2(uint8_t *data, uint8_t len);
void single_recv_board_data_3(uint8_t *data, uint8_t len);
 
void single_clear_config_function(void);


#endif


