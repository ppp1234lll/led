#ifndef _ERROR_H_
#define _ERROR_H_

#include "./SYSTEM/sys/sys.h"

/* 参数 */

/* 错误码 */
typedef struct {
    uint32_t    err_index; // 组内错误索引（替代原有bit_mask，仅用于组内标识）
    const char* err_code;  // 数字错误码（如"100"、"409"）     
} ErrorItem_t;             

typedef struct {
    uint32_t 		group_mask;   // 组对应的bit掩码（1UL << n，n=0~63）
    const ErrorItem_t* items; // 指向该组的错误项数组
    uint8_t   item_count;   		// 该组内错误项的数量
} ErrorGroup_t;

typedef enum {  
    ERR_TYPE_ELEC=0,  
    ERR_TYPE_NET,      
    ERR_TYPE_SENSOR,   
    ERR_TYPE_TRAFFIC,  
    ERR_MAX            
} ErrorType_e;         

#define  ELEC_MAIN_AC				0  
#define  ELEC_ACDC_MODULE		1  
#define  ELEC_AC_OVER_V			2  
#define  ELEC_AC_LOW_V			3  
#define  ELEC_AC_OVER_C			4  
#define  ELEC_AC_LEAKAGE		5  
#define  ELEC_AC_MCB				6  
															 
#define NET_LAN_PORT				0
#define NET_MAIN_IP					1
#define NET_SINGLE_IP				2

#define SENSOR_TEMP_HIGH		0
#define SENSOR_TEMP_LOW			1
#define SENSOR_HUMI_HIGH		2
#define SENSOR_BOX_TILT     3
#define SENSOR_DOOR_OPEN    4
#define SENSOR_WATER_LEAK   5

// 信号灯故障类型定义
#define TRAFFIC_FAULT_TYPE_NORMAL      0 // 正常
#define TRAFFIC_FAULT_TYPE_ALL_OFF     1 // 全不亮
#define TRAFFIC_FAULT_TYPE_PARTIAL_ON  2 // 部分亮
#define TRAFFIC_FAULT_TYPE_RED_GREEN_ON 3 // 红绿同亮

// 信号灯故障错误代码定义（可配置形式）
// 5位数字编码格式：ABCDE
// A: 类型 (0=远灯, 1=近灯)
// B: 方向 (0=北, 1=东, 2=南, 3=西)
// C: 相位 (0=左转, 1=直行, 2=右转, 3=行人1, 4=行人2, 5=非机动车1, 6=非机动车2, 7=倒计时, 8=可变车道, 9=待行, 0=辅道)
// D: 颜色 (0=红, 1=绿, 2=黄)
// E: 故障类型 (0=正常, 1=全不亮, 2=部分亮, 3=红绿同亮)

// 辅助宏：生成信号灯故障错误索引
#define TRAFFIC_FAULT_INDEX(type, dir, phase, color, fault) \
    ((((type) & 0x01) << 8) | (((dir) & 0x03) << 6) | (((phase) & 0x0F) << 2) | ((color) & 0x03))

// 辅助宏：生成信号灯故障错误码字符串
#define TRAFFIC_FAULT_CODE(type, dir, phase, color, fault) \
    ((char[]){'4', (type)+'0', (dir)+'0', ((phase)%10)+'0', (color)+'0', (fault)+'0', '\0'})

// 信号灯故障管理函数声明

/**
 * @brief 标记信号灯故障
 * @param type 类型 (0=远灯, 1=近灯)
 * @param dir 方向 (0=北, 1=东, 2=南, 3=西)
 * @param phase 相位 (0=左转, 1=直行, 2=右转, 3=行人1, 4=行人2, 5=非机动车1, 6=非机动车2, 7=倒计时, 8=可变车道, 9=待行, 0=辅道)
 * @param color 颜色 (0=红, 1=绿, 2=黄)
 * @param fault 故障类型 (0=正常, 1=全不亮, 2=部分亮, 3=红绿同亮)
 * @return 成功返回1，失败返回0
 */
uint8_t TrafficFault_Set(uint8_t type, uint8_t dir, uint8_t phase, uint8_t color, uint8_t fault);

/**
 * @brief 清除信号灯故障
 * @param type 类型 (0=远灯, 1=近灯)
 * @param dir 方向 (0=北, 1=东, 2=南, 3=西)
 * @param phase 相位 (0=左转, 1=直行, 2=右转, 3=行人1, 4=行人2, 5=非机动车1, 6=非机动车2, 7=倒计时, 8=可变车道, 9=待行, 0=辅道)
 * @param color 颜色 (0=红, 1=绿, 2=黄)
 * @param fault 故障类型 (0=正常, 1=全不亮, 2=部分亮, 3=红绿同亮)
 * @return 成功返回1，失败返回0
 */
uint8_t TrafficFault_Clear(uint8_t type, uint8_t dir, uint8_t phase, uint8_t color, uint8_t fault);

/**
 * @brief 检查信号灯故障
 * @param type 类型 (0=远灯, 1=近灯)
 * @param dir 方向 (0=北, 1=东, 2=南, 3=西)
 * @param phase 相位 (0=左转, 1=直行, 2=右转, 3=行人1, 4=行人2, 5=非机动车1, 6=非机动车2, 7=倒计时, 8=可变车道, 9=待行, 0=辅道)
 * @param color 颜色 (0=红, 1=绿, 2=黄)
 * @param fault 故障类型 (0=正常, 1=全不亮, 2=部分亮, 3=红绿同亮)
 * @return 故障存在返回1，不存在返回0
 */
uint8_t TrafficFault_Check(uint8_t type, uint8_t dir, uint8_t phase, uint8_t color, uint8_t fault);

/**
 * @brief 获取信号灯故障错误码
 * @param type 类型 (0=远灯, 1=近灯)
 * @param dir 方向 (0=北, 1=东, 2=南, 3=西)
 * @param phase 相位 (0=左转, 1=直行, 2=右转, 3=行人1, 4=行人2, 5=非机动车1, 6=非机动车2, 7=倒计时, 8=可变车道, 9=待行, 0=辅道)
 * @param color 颜色 (0=红, 1=绿, 2=黄)
 * @param fault 故障类型 (0=正常, 1=全不亮, 2=部分亮, 3=红绿同亮)
 * @return 错误码字符串，失败返回NULL
 */
const char* TrafficFault_GetCode(uint8_t type, uint8_t dir, uint8_t phase, uint8_t color, uint8_t fault);

/**
 * @brief 清除所有信号灯故障
 * @return 成功返回1，失败返回0
 */
uint8_t TrafficFault_ClearAll(void);

/**
 * @brief 获取信号灯故障数量
 * @return 故障数量
 */
uint8_t TrafficFault_GetCount(void);

/* 函数声明 */
void Error_Set(ErrorType_e group, uint32_t item_idx);
void Error_Clear(ErrorType_e group, uint32_t item_idx);
uint8_t Error_Check(ErrorType_e group, uint32_t item_idx);
int8_t Error_GetAllCodes(char* buf, uint16_t buf_len);
	
#endif
