#ifndef _ATGM336H_H_
#define _ATGM336H_H_

#include "sys.h"
#include "ATGM336H_nmea_msg.h"

/* ATGM336H模块UTC结构体 */
typedef struct
{
    uint16_t year;               /* 年 */
    uint8_t month;               /* 月 */
    uint8_t day;                 /* 日 */
    uint8_t hour;                /* 时 */
    uint8_t minute;              /* 分 */
    uint8_t second;              /* 秒 */
    uint16_t millisecond;        /* 毫秒 */
} atgm336h_time_t;

/* ATGM336H模块位置结构体 */
typedef struct
{
    atgm336h_latitude_t latitude;         /* 纬度信息 */
    atgm336h_longitude_t longitude;       /* 经度信息 */
} atgm336h_position_t;

/* ATGM336H模块定位信息结构体 */
typedef struct
{
    atgm336h_gps_quality_indicator_t quality; /* 定位质量 */
    uint8_t satellite_num;                    /* 用于定位的卫星数量 */
    atgm336h_fix_type_t type;                 /* 定位类型 */
    uint16_t satellite_id[12];                /* 用于定位的卫星编号 */
    uint16_t pdop;                            /* 位置精度因子（扩大10倍） */
    uint16_t hdop;                            /* 水平精度因子（扩大10倍） */
    uint16_t vdop;                            /* 垂直精度因子（扩大10倍） */
} atgm336h_fix_info_t;

/* ATGM336H模块可见卫星信息结构体 */
typedef struct
{
    uint8_t satellite_num;                        /* 可见卫星数量 */
    atgm336h_satellite_info_t satellite_info[12]; /* 可见卫星信息 */
} atgm336h_visible_satellite_info_t;

/* 错误代码 */
#define ATGM336H_EOK      0       /* 没有错误 */
#define ATGM336H_ERROR    1       /* 错误 */
#define ATGM336H_ETIMEOUT 2       /* 超时错误 */
#define ATGM336H_EINVAL   3       /* 参数错误 */


// Function declarations
void ATGM336H_init(uint32_t bound);
/* 操作函数 */


#endif /* _ATGM338H_H_ */
