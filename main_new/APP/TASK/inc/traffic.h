#ifndef _TRAFFIC_H_
#define _TRAFFIC_H_

#include "./SYSTEM/sys/sys.h"

// 方向枚举（东西南北）
typedef enum {
    DIR_EAST = 0,    // 东
    DIR_WEST,        // 西
    DIR_SOUTH,       // 南
    DIR_NORTH,       // 北
    DIR_MAX          // 方向总数（边界检查用）
} TrafficDirection;

// 红绿灯类型枚举（左转、直行、右转、人行）
typedef enum {
    LIGHT_LEFT = 0,  // 左转灯
    LIGHT_STRAIGHT,  // 直行灯
    LIGHT_RIGHT,     // 右转灯
    LIGHT_PEDESTRIAN1,// 人行灯
	  LIGHT_PEDESTRIAN2,// 人行灯
	  LIGHT_PEDESTRIAN2,// 人行灯 
    LIGHT_MAX        // 灯类型总数
} TrafficLightType;


// 红绿灯种类
typedef enum {
    Type_MAX        // 灯类型总数
} TrafficType;

// 红绿灯颜色枚举（红、绿、黄）
typedef enum {
    TRAFFIC_COLOR_RED = 0,   // 红灯
    TRAFFIC_COLOR_GREEN,     // 绿灯
    TRAFFIC_COLOR_YELLOW,    // 黄灯
    TRAFFIC_COLOR_MAX        // 颜色总数
} TrafficLightColor;

// 红绿灯参数
typedef enum {
    TIMES = 0,   // 时间
    CURRENT,     // 电流
    VOLT,         // 电压
    PARAM_MAX        // 参数
} TrafficParam;

typedef struct {
    // 基础核心参数
    uint8_t  dir[TRAFFIC_DIR_MAX];     // 红绿灯所在方向（东/西/南/北）
    uint8_t  type[TRAFFIC_LIGHT_MAX];  // 灯类型（左转/直行/右转/人行）
    uint8_t  color[TRAFFIC_COLOR_MAX]; // 当前颜色（红/绿/黄）
}Traffic_t;


#endif
