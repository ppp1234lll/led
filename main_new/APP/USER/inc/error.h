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


/* 函数声明 */
void Error_Set(ErrorType_e group, uint32_t item_idx);
void Error_Clear(ErrorType_e group, uint32_t item_idx);
uint8_t Error_Check(ErrorType_e group, uint32_t item_idx);	
int8_t Error_GetAllCodes(char* buf, uint16_t buf_len);
	
#endif
