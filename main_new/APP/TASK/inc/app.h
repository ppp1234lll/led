#ifndef _APP_H_
#define _APP_H_

#include "./SYSTEM/sys/sys.h"

// 服务器连接模式
#define SERVER_MODE_LWIP (1) // 有线连接
#define SERVER_MODE_GPRS (2) // 无线连接
#define SERVER_MODE_ALL  (3) // 有线和无线同时连接
#define SERVER_MODE_AUTO (4) // 自动


struct local_ip_t				// 本地网络信息
{
	uint8_t ip[4];				// 本地IP
	uint8_t mac[6]; 			// MAC地址
	uint8_t netmask[4]; 		// 掩码
	uint8_t gateway[4]; 		// 网关
	uint8_t dns[4];				// 默认DNS
	
	uint8_t ping_ip[4];			// 主机需要测试的ping ip
	uint8_t ping_sub_ip[4];		// 主机需要测试的ip 2
	uint8_t server_mode;		// 服务器模式:1-有线 2-无线 4-自动
	uint8_t multicast_ip[4];	// 组播IP
	uint32_t multicast_port;	// 组播端口	
	uint8_t search_mode;			// 摄像机检测模式:1-PING 2-协议  20230810
	uint8_t single_ip[4];		
};

struct remote_ip				
{
	uint8_t	 outside_iporname[64];	  // 外网IP-域名
	uint32_t outside_port;			  // 外网端口
	
	uint8_t  inside_iporname[64];	  // 内网IP-域名
	uint32_t inside_port;			  // 内网端口
};

struct update_addr			
{	
	uint8_t  ip[4];	  // 更新IP-域名
	uint32_t port;    // 更新端口
};

struct device_param
{
	union i_c  id;		  		// id
	uint8_t  name[52];    // 设备名称
	uint8_t  password[20]; // 设备密码
	uint8_t  default_password; // 0-已修改过默认密码 1-未修改默认密码
};

 
typedef struct
{
	int8_t   JumpResult;
	int8_t   reset_num;    // 重启次数
	uint32_t jump_addr;    // 跳转地址
}run_result_t;


// 阈值检测 20230720
struct threshold_params {
	uint16_t volt_max;  // 高压
	uint16_t volt_min;  // 低压
	uint16_t current;
	uint8_t  angle;
	int8_t   temp_high;		// 
	int8_t   temp_low;	// 
	int8_t   humi_high;		// 
	int8_t   humi_low;  // 
 
	uint16_t miu;         //漏电阈值
};


/* 参数 */
typedef struct
{
	struct local_ip_t    local;  // 本机网络参数
	struct remote_ip     remote; // 远端网络参数
	struct device_param  device; // 设备参数
	struct threshold_params threshold; // 阈值 20230720
	struct update_addr ota;
	uint8_t mem;			   	 // 内存利用率
} sys_param_t;

/* 参数 */
typedef struct
{
	uint8_t config_flag;
	struct remote_ip     remote; // 远端网络参数
} sys_backups_t;


typedef struct
{
	uint32_t heart;			   // 心跳包
	uint32_t report;		   // 上报时间
	uint32_t ping;			   // ping的间隔时间
	uint32_t dev_ping;		 // 设备间隔ping时间
	uint8_t  network_time; // 网络延时时间  20220308
	uint8_t  onvif_time;   // 搜索协议  20230811
} com_param_t;


/* 发送结果 */
typedef enum
{
	SR_WAIT       = 0, // 发送等待
	SR_OK         = 1, // 发送完成
	SR_TIMEOUT    = 2, // 发送超时
	SR_ERROR      = 3, // 发送结束到错误提示
	SR_SEND_ERROR = 4, // 发送错误
} send_result_e;


/* 函数声明 */
void app_task_function(void);
void app_task_save_function(void);			   
void app_com_send_function(void);			   
void app_server_link_status_function(void);  
void app_deal_com_flag_function(void);
void app_deal_com_send_wait_function(void);
void app_com_send_function(void);
void app_com_time_function(void);
void app_detect_function(void);

void app_set_com_send_flag_function(uint8_t cmd, uint8_t data);
void app_set_reply_parameters_function(uint8_t cmd, uint8_t error);
void app_set_send_result_function(send_result_e data);
void app_set_sys_opeare_function(uint8_t cmd, uint8_t data);


void app_report_information_immediately(void);
void app_send_once_heart_infor(void);
void app_send_query_configuration_infor(void);

uint8_t app_get_com_send_status_function(void);

void app_sys_operate_timer_function(void);
void app_sys_ctrl_relay_reload(void);
void app_set_relay_switch(uint8_t cmd, uint8_t id,uint8_t status);

void app_get_storage_param_function(void);
void app_task_save_function(void);

void app_set_save_infor_function(uint8_t mode);
void app_set_local_network_function_two(struct local_ip_t param);
void app_set_transfer_mode_function(uint8_t mode) ;
void app_set_remote_network_function(struct remote_ip param);
void app_set_reset_function(void);
void app_set_fan_humi_param_function(uint8_t *data);
void app_set_fan_param_function(int8_t *data);
void app_set_vol_current_param(uint16_t *data);
void app_set_device_param_function(struct device_param param);	
void app_set_next_report_time(uint16_t time);
void app_set_next_report_time_other(uint16_t time,uint8_t sel);
void app_set_next_ping_time(uint16_t time, uint8_t time_dev);
void app_set_network_delay_time(uint8_t time_dev);
void app_set_code_function(struct device_param param);
void app_set_current_time(int *time,uint8_t conv);
void app_set_main_network_ping_ip(uint8_t *ip);
void app_set_single_device_ping_ip(uint8_t *ip);
void app_set_com_interface_selection_function(uint8_t mode);	
void app_set_com_time_param_function(uint32_t *time,uint8_t mode);
void app_set_threshold_param_function(struct threshold_params param);
void app_set_update_status_function(uint8_t flag);
void app_set_http_ota_function(struct update_addr param);


void *app_get_local_network_function(void);
void *app_get_remote_network_function(void);
void *app_get_backups_function(void);
void *app_get_device_param_function(void);
void app_get_current_time(char *time);
uint8_t* app_get_report_current_time(uint8_t mode);
uint32_t app_get_next_ping_time(void);
uint32_t app_get_next_dev_ping_time(void);
uint32_t app_get_report_time(void);
uint8_t app_get_network_delay_time(void); 
uint16_t app_get_com_heart_time(void);
void app_get_main_network_ping_ip_addr(uint8_t* ip);
void app_get_main_network_sub_ping_ip_addr(uint8_t* ip);
void app_get_single_ping_ip_addr(uint8_t* ip);
uint8_t app_get_com_interface_selection_function(void);
void *app_get_com_time_infor(void);
uint8_t app_get_network_mode(void);
void app_get_network_connect_status(char *buff);
uint8_t *app_get_device_name(void);
void *app_get_threshold_param_function(void);
void *app_get_backups_param_function(void);
uint8_t app_get_update_status_function(void);
void *app_get_http_ota_function(void);


int8_t app_match_password_function(char *password);
int8_t app_match_set_code_function(void);
void app_save_backups_remote_param_function(void);
void app_power_fail_protection_function(void);
void app_power_open_protection_function(void)	;
void app_send_data_task_function(void);
void app_system_softreset(uint32_t time);
void System_SoftReset(void);
void app_reboot_timer_run(void);


#endif
