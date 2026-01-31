#ifndef _ERROR_H_
#define _ERROR_H_

#include "./SYSTEM/sys/sys.h"

/* 参数 */

/* 错误码 */
typedef struct {
    uint32_t    err_index; // 组内错误索引（替代原有bit_mask，仅用于组内标识）
    uint32_t    err_code;  // 16进制错误码（如0x100, 0x409）     
} ErrorItem_t;             

typedef struct {
    uint32_t 		group_mask;   // 组对应的bit掩码（1UL << n，n=0~63）
    const ErrorItem_t* items; // 指向该组的错误项数组
    uint8_t   item_count;   		// 该组内错误项的数量
} ErrorGroup_t;

typedef enum {  
    ERR_TYPE_ELEC,  
    ERR_TYPE_NET,      
    ERR_TYPE_SENSOR,   
    ERR_TYPE_TRAFFIC,  
    ERR_MAX            
} ErrorType_e;         

// 错误类型基础值定义
#define ERR_TYPE_ELEC_BASE    0x10000000
#define ERR_TYPE_NET_BASE     0x20000000
#define ERR_TYPE_SENSOR_BASE  0x30000000
#define ERR_TYPE_TRAFFIC_BASE 0x40000000         

#define  ELEC_NORMAL            0
#define  ELEC_MAIN_AC				ERR_TYPE_ELEC_BASE | 1 << 20  
#define  ELEC_ACDC_MODULE		ERR_TYPE_ELEC_BASE | 2 << 20  
#define  ELEC_AC_OVER_V			ERR_TYPE_ELEC_BASE | 3 << 20  
#define  ELEC_AC_LOW_V			ERR_TYPE_ELEC_BASE | 4 << 20  
#define  ELEC_AC_OVER_C			ERR_TYPE_ELEC_BASE | 5 << 20  
#define  ELEC_AC_LEAKAGE		ERR_TYPE_ELEC_BASE | 6 << 20  
#define  ELEC_AC_MCB				ERR_TYPE_ELEC_BASE | 7 << 20  

#define NET_NORMAL             0
#define NET_LAN_PORT				ERR_TYPE_NET_BASE | 1 << 20
#define NET_MAIN_IP					ERR_TYPE_NET_BASE | 2 << 20
#define NET_SINGLE_IP				ERR_TYPE_NET_BASE | 3 << 20

#define SENSOR_NORMAL             0
#define SENSOR_TEMP_HIGH			ERR_TYPE_SENSOR_BASE | 1 << 20
#define SENSOR_TEMP_LOW			ERR_TYPE_SENSOR_BASE | 2 << 20
#define SENSOR_HUMI_HIGH		ERR_TYPE_SENSOR_BASE | 3 << 20
#define SENSOR_BOX_TILT     ERR_TYPE_SENSOR_BASE | 4 << 20
#define SENSOR_DOOR_OPEN    ERR_TYPE_SENSOR_BASE | 5 << 20
#define SENSOR_WATER_LEAK   ERR_TYPE_SENSOR_BASE | 6 << 20

// 信号灯故障类型定义
#define TRAFFIC_NORMAL      0 // 正常
#define TRAFFIC_PART_NO_LIGHT   1 // 某方向不亮
#define TRAFFIC_RED_GREEN_SAME_LIGHT 2 // 红绿同亮
#define TRAFFIC_SINGLE_NO_LIGHT 3 // 单灯不亮
#define TRAFFIC_SINGLE_PART_LIGHT  4 // 单灯部分亮

#define TRAFFIC_ALL_NO_LIGHT    0xFF // 全不亮

/**
 * @brief 标记信号灯故障
 * @param fault 故障类型 (0=正常, 1=全不亮, 2=部分亮, 3=红绿同亮)
 * @param type 类型 (0=远灯, 1=近灯)
 * @param dir 方向 (0=北, 1=东, 2=南, 3=西)
 * @param road 道路类型 (0=主道, 1=辅道)
 * @param phase 相位 (0=左转, 1=直行, 2=右转, 3=行人1, 4=行人2, 5=非机动车1, 6=非机动车2, 7=倒计时, 8=可变车道, 9=待行, 0=辅道)
 * @param color 颜色 (0=红, 1=绿, 2=黄)
 * @return 成功返回1，失败返回0
 */
uint8_t TrafficFault_Set(uint8_t fault, uint8_t type, uint8_t dir, uint8_t road, uint8_t phase, uint8_t color);

/**
 * @brief 清除信号灯故障
 * @param fault 故障类型 (0=正常, 1=全不亮, 2=部分亮, 3=红绿同亮)
 * @param type 类型 (0=远灯, 1=近灯)
 * @param dir 方向 (0=北, 1=东, 2=南, 3=西)
 * @param road 道路类型 (0=主道, 1=辅道)
 * @param phase 相位 (0=左转, 1=直行, 2=右转, 3=行人1, 4=行人2, 5=非机动车1, 6=非机动车2, 7=倒计时, 8=可变车道, 9=待行, 0=辅道)
 * @param color 颜色 (0=红, 1=绿, 2=黄)
 * @return 成功返回1，失败返回0
 */
uint8_t TrafficFault_Clear(uint8_t fault, uint8_t type, uint8_t dir, uint8_t road, uint8_t phase, uint8_t color);


/* 函数声明 */
void Error_Set(ErrorType_e group, uint32_t item_idx);
void Error_Clear(ErrorType_e group, uint32_t item_idx);
int8_t Error_GetAllCodes(uint32_t* codes);
int8_t Error_Get_Codesbuf(uint8_t* codes);	
#endif
