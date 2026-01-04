#ifndef _DET_H_
#define _DET_H_

#include "sys.h"

/* 按键事件 */
typedef enum
{
	KEY_NONE  = 0,     // 无事件
	KEY_EVNT,          // 事件触发
  KEY_ERASE,         // 擦除
}KEY_VALUE_E;

typedef struct {
	int32_t AcceX;				/*acceleration x*/
	int32_t AcceY;				/*acceleration y*/
	int32_t AcceZ;				/*acceleration z*/
} MENS_XYZ_STATUS_T;

typedef struct {
	float total_electricity;  	// 总用电量
	float electricity[5]; 			// 用电量
} electricity_t;


typedef struct
{
	float temp_inside; 	 	// 内部温度值
	float humi_inside; 	 	// 内部湿度值
	
	double attitude_acc; 	// 加速度

	float vin220v;		 // 220V电压检测
	float current[5]; 		// 电流检测
	float total_current; 	// 总电流检测
	float total_power;  	// 总功率1
	float power[5]; 			// 功率
	electricity_t kwh;    // 用电量
	
	uint8_t key_s[10];    // 按键数量定义
	uint8_t key_evnt[10]; // 按键事件
	
	float dc_vout;     // 直流输出电压
	float dc_current;  // 直流输出电流
	float dc_power;    // 直流输出功率
	float dc_kwh;      // 直流输出用电量
	uint8_t residual_c;   // 剩余电流

	uint8_t camera[10];	 // 摄像机状态x3：0；离线 1：在线，2：延时严重
	uint8_t main_ip;	   // 主网络状态：0：离线 1：在线，2：延时严重
	uint8_t main_sub_ip; // 主网络状态sub: 0：离线 1：在线，2：延时严重
	uint8_t ping_status; // ping结束标志位
} data_collection_t;

extern data_collection_t sg_datacollec_t;

void det_task_function(void);

void det_get_key_status_function(void);
uint8_t det_main_network_and_camera_network(void);
void det_get_temphumi_function(void);
float lean_check(MENS_XYZ_STATUS_T *acc_xyz);
void det_get_attitude_state_value(void);
fp32 det_get_inside_temp(void);								 // 获取内部温度
fp32 det_get_inside_humi(void); 							 // 获取内部湿度
fp32 det_get_vin220v_handler(uint8_t num);		 // 获取电压、电流
fp32 det_get_power_handler(uint8_t num);       // 功率
fp32 det_get_electricity_handler(uint8_t num); // 用电量
fp32 det_get_dc_electricity_handler(uint8_t num); // 用电量

void det_set_open_door(uint8_t mode);               // 设置箱门状态
void det_set_220v_in_function(uint8_t status); 			    // 设置市电状态
void det_set_camera_status(uint8_t num,uint8_t status);		// 设置摄像机状态
void det_set_main_network_status(uint8_t status);			// 设置主网络状态
void det_set_main_network_sub_status(uint8_t status);		// 设置主网络状态 - 2
void det_set_total_energy_bl0910(uint8_t num,float data);			// 设置电量参数
void det_set_total_energy_bl0942(uint8_t num,float data);
void det_set_total_energy_bl0972(uint8_t num,float data);
void det_set_total_energy_bl0939(uint8_t num,float data);
void det_set_ping_status(uint8_t status);
void det_set_spd_status(uint8_t mode);   //  设置防雷开关状态
void det_set_water_status(uint8_t mode);


uint8_t det_get_open_door(void);							// 获取箱门状态
uint16_t det_get_cabinet_posture(void);						// 获取箱体姿态
uint8_t det_get_camera_status(uint8_t num);					// 获取摄像机状态
uint8_t det_get_main_network_status(void);					// 获取主网络状态
uint8_t det_get_main_network_sub_status(void);				// 获取主网络状态 - 2
uint8_t det_get_spd_status(void);	   //  获取防雷开关状态
uint8_t det_get_water_status(void);
uint8_t det_get_miu_value(void);
void *det_get_collect_data(void);
void det_set_key_value(uint8_t key_id,uint8_t key_value);
void Miu_Handler(char *pcInsert, uint8_t num);

void det_get_gps_value(void);
void det_get_lux_function(void);
uint8_t det_get_pwr_status(void);
uint8_t det_get_door_status(void);
uint8_t det_get_spd_status(void);
uint8_t det_get_water_status(void);


#endif
