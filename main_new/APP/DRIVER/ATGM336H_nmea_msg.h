
#ifndef __ATGM336H_NMEA_MSG_H
#define __ATGM336H_NMEA_MSG_H

#include "sys.h"

/* ATGM336H模块NMEA消息类型枚举 */
typedef enum
{
    ATGM336H_NMEA_MSG_GNGGA = 0x00,               /* $GNGGA */
    ATGM336H_NMEA_MSG_GNGLL,                      /* $GNGLL */
    ATGM336H_NMEA_MSG_GNGSA,                      /* $GNGSA */
    ATGM336H_NMEA_MSG_GPGSA,                      /* $GPGSA */
    ATGM336H_NMEA_MSG_BDGSA,                      /* $BDGSA */
    ATGM336H_NMEA_MSG_GPGSV,                      /* $GPGSV */
    ATGM336H_NMEA_MSG_BDGSV,                      /* $BDGSV */
    ATGM336H_NMEA_MSG_GNRMC,                      /* $GNRMC */
    ATGM336H_NMEA_MSG_GNVTG,                      /* $GNVTG */
    ATGM336H_NMEA_MSG_GNZDA,                      /* $GNZDA */
} atgm336h_nmea_msg_t;

/* ATGM336H模块北纬南纬枚举 */
typedef enum
{
    ATGM336H_LATITUDE_NORTH = 0x00,               /* 北纬 */
    ATGM336H_LATITUDE_SOUTH,                      /* 南纬 */
} atgm336h_latitude_indicator_t;

/* ATGM336H模块东经西经枚举 */
typedef enum
{
    ATGM336H_LONGITUDE_EAST = 0x00,               /* 东经 */
    ATGM336H_LONGITUDE_WEST,                      /* 西经 */
} atgm336h_longitude_indicator_t;

/* ATGM336H模块GPS质量指标枚举 */
typedef enum
{
    ATGM336H_GPS_UNAVAILABLE = 0x00,              /* 定位不可用 */
    ATGM336H_GPS_VALID_SPS,                       /* 定位有效，SPS模式 */
    ATGM336H_GPS_VALID_DIFFERENTIAL,              /* 定位有效，差分GPS模式 */
} atgm336h_gps_quality_indicator_t;

/* ATGM336H模块NMEA消息 XXGLL数据状态枚举 */
typedef enum
{
    ATGM336H_XXGLL_DATA_VALID = 0x00,             /* XXGLL数据有效 */
    ATGM336H_XXGLL_DATA_NO_VALID,                 /* XXGLL数据不可用 */
} atgm336h_xxgll_status_t;

/* ATGM336H模块GPS接收器操作模式枚举 */
typedef enum
{
    ATGM336H_GPS_OPERATING_MANUAL = 0x00,         /* 手动，强制在2D或3D模式下操作 */
    ATGM336H_GPS_OPERATING_AUTOMATIC,             /* 自动，允许自动切换2D、3D模式 */
} atgm336h_gps_operation_mode_t;

/* ATGM336H模块的定位类型 */
typedef enum
{
    ATGM336H_FIX_NOT_AVAILABLE = 0x00,            /* 未定位 */
    ATGM336H_FIX_2D,                              /* 2D */
    ATGM336H_FIX_3D,                              /* 3D */
} atgm336h_fix_type_t;

/* ATGM336H模块导航数据状态枚举 */
typedef enum
{
    ATGM336H_NAVIGATION_WARNING = 0x00,           /* 导航数据无效 */
    ATGM336H_NAVIGATION_VALID,                    /* 导航数据有效 */
} atgm336h_navigation_data_status_t;

/* ATGM336H模块定义系统模式枚举 */
typedef enum
{
    ATGM336H_POS_SYS_DATA_NOT_VALID = 0x00,       /* 数据无效 */
    ATGM336H_POS_SYS_AUTONOMOUS,                  /* 自主定位 */
    ATGM336H_POS_SYS_DIFFERENTIAL,                /* 差分 */
    ATGM336H_POS_SYS_ESTIMATED,                   /* 估算 */
} atgm336h_pos_sys_mode_t;

/* ATGM336H模块UTC时间结构体 */
typedef struct
{
    uint8_t hour;                                   /* 时 */
    uint8_t minute;                                 /* 分 */
    uint8_t second;                                 /* 秒 */
    uint16_t millisecond;                           /* 毫秒 */
} atgm336h_utc_time_t;

/* ATGM336H模块UTC日期结构体 */
typedef struct
{
    uint16_t year;                                  /* 年 */
    uint8_t month;                                  /* 月 */
    uint8_t day;                                    /* 日 */
} atgm336h_utc_date_t;

/* ATGM336H模块纬度结构体 */
typedef struct
{
    atgm336h_latitude_indicator_t indicator;      /* 指示北纬、南纬 */
    uint32_t degree;                                /* 纬度，扩大100000倍，单位：度 */
} atgm336h_latitude_t;

/* ATGM336H模块经度结构体 */
typedef struct
{
    atgm336h_longitude_indicator_t indicator;     /* 指示东经、西经 */
    uint32_t degree;                                /* 经度，扩大100000倍，单位：度 */
} atgm336h_longitude_t;

/* ATGM336H模块卫星信息 */
typedef struct
{
    uint16_t satellite_id;                          /* 卫星ID */
    uint8_t elevation;                              /* 卫星仰角，单位：度 */
    uint16_t azimuth;                               /* 卫星方位角，单位：度 */
    uint8_t snr;                                    /* 信噪比，单位：dB */
} atgm336h_satellite_info_t;

/* ATGM336H模块NMEA消息XXGGA消息结构体 */
typedef struct
{
    atgm336h_utc_time_t utc_time;                 /* UTC时间 */
    atgm336h_latitude_t latitude;                 /* 纬度 */
    atgm336h_longitude_t longitude;               /* 经度 */
    atgm336h_gps_quality_indicator_t gps_quality; /* GPS质量指标 */
    uint8_t satellite_num;                          /* 使用的卫星数量 */
    uint16_t hdop;                                  /* 水平精度因子，扩大10倍 */
    int32_t altitude;                               /* 海拔高度，扩大10倍 */
    /* 大地水准面差距 */
    uint16_t dgps_id;                               /* DGPS站ID */
} atgm336h_nmea_gga_msg_t;

/* ATGM336H模块NMEA消息XXGLL消息结构体 */
typedef struct
{
    atgm336h_latitude_t latitude;                 /* 纬度 */
    atgm336h_longitude_t longitude;               /* 经度 */
    atgm336h_utc_time_t utc_time;                 /* UTC时间 */
    atgm336h_xxgll_status_t status;               /* 数据状态 */
} atgm336h_nmea_gll_msg_t;

/* ATGM336H模块NMEA消息XXGSA消息结构体 */
typedef struct
{
    atgm336h_gps_operation_mode_t mode;           /* GPS接收器操作模式 */
    atgm336h_fix_type_t type;                     /* 定位类型 */
    uint8_t satellite_id[12];                       /* 卫星ID */
    uint16_t pdop;                                  /* 位置精度因子，扩大10倍 */
    uint16_t hdop;                                  /* 水平精度因子，扩大10倍 */
    uint16_t vdop;                                  /* 垂直精度因子，扩大10倍 */
} atgm336h_nmea_gsa_msg_t;

/* ATGM336H模块NMEA消息XXGSV消息结构体 */
typedef struct
{
    uint8_t satellite_view;                         /* 可见卫星总数 */
    atgm336h_satellite_info_t satellite_info[12]; /* 卫星信息 */
} atgm336h_nmea_gsv_msg_t;

/* ATGM336H模块NMEA消息XXRMC消息结构体 */
typedef struct
{
    atgm336h_utc_time_t utc_time;                 /* UTC时间 */
    atgm336h_utc_date_t utc_date;                 /* UTC日期 */
    atgm336h_navigation_data_status_t status;     /* 导航数据状态 */
    atgm336h_latitude_t latitude;                 /* 纬度 */
    atgm336h_longitude_t longitude;               /* 经度 */
    uint16_t speed_ground;                          /* 地面速度，扩大10倍，单位：节（knot） */
    uint16_t course_ground;                         /* 地面航向，扩大10倍，单位：度 */
    atgm336h_pos_sys_mode_t position_system_mode; /* 定位系统模式 */
} atgm336h_nmea_rmc_msg_t;

/* ATGM336H模块NMEA消息XXVTG消息结构体 */
typedef struct
{
    uint16_t course_true;                           /* 地面航向，扩大10倍，单位：度 */
    uint16_t course_magnetic;                       /* 地面航向，扩大10倍，单位：度 */
    uint16_t speed_knots;                           /* 地面速度，扩大10倍，单位：节（knot） */
    uint16_t speed_kph;                             /* 地面速度，扩大10倍，单位：千米/时 */
    atgm336h_pos_sys_mode_t position_system_mode; /* 定位系统模式 */
} atgm336h_nmea_vtg_msg_t;

/* ATGM336H模块NMEA消息XXZDA消息结构体 */
typedef struct
{
    atgm336h_utc_time_t utc_time;                 /* UTC时间 */
    atgm336h_utc_date_t utc_date;                 /* UTC日期 */
    int8_t local_zone_hour;                         /* 本地时区时，范围：-13~+13 */
    uint8_t local_zone_minute;                      /* 本地时区分，范围：0~59 */
} atgm336h_nmea_zda_msg_t;

/* 操作函数 */
uint8_t atgm336h_get_nmea_msg_from_buf(uint8_t *buf, atgm336h_nmea_msg_t nmea, uint8_t msg_index, uint8_t **msg);   /* 从数据缓冲中获取指定类型和索引的NMEA消息 */
uint8_t atgm336h_decode_nmea_xxgga(uint8_t *xxgga_msg, atgm336h_nmea_gga_msg_t *decode_msg);                        /* 解析$XXGGA类型的NMEA消息 */
uint8_t atgm336h_decode_nmea_xxgll(uint8_t *xxgll_msg, atgm336h_nmea_gll_msg_t *decode_msg);                        /* 解析$XXGLL类型的NMEA消息 */
uint8_t atgm336h_decode_nmea_xxgsa(uint8_t *xxgsa_msg, atgm336h_nmea_gsa_msg_t *decode_msg);                        /* 解析$XXGSA类型的NMEA消息 */
uint8_t atgm336h_decode_nmea_xxgsv(uint8_t *xxgsv_msg, atgm336h_nmea_gsv_msg_t *decode_msg);                        /* 解析$XXGSV类型的NMEA消息 */
uint8_t atgm336h_decode_nmea_xxrmc(uint8_t *xxrmc_msg, atgm336h_nmea_rmc_msg_t *decode_msg);                        /* 解析$XXRMC类型的NMEA消息 */
uint8_t atgm336h_decode_nmea_xxvtg(uint8_t *xxvtg_msg, atgm336h_nmea_vtg_msg_t *decode_msg);                        /* 解析$XXVTG类型的NMEA消息 */
uint8_t atgm336h_decode_nmea_xxzda(uint8_t *xxzda_msg, atgm336h_nmea_zda_msg_t *decode_msg);                        /* 解析$XXZDA类型的NMEA消息 */

#endif
