#ifndef _START_H_
#define _START_H_

#include "./SYSTEM/sys/sys.h"

typedef struct
{
	uint8_t id_flag;				// 更新标志位
	uint32_t crc_dat;       // 校验数据				
}code_id_t;

/* 参数 */
typedef struct
{
  uint32_t id[3];
} ChipID_t;

/* 提供给其他C文件调用的函数 */
void start_bsp_init(void);
void start_task_create(void);   // 创建任务

void start_system_init_function(void);  // 系统初始化函数
void start_get_device_id_function(void);  // 获取本机ID
void start_get_device_id_str(uint8_t *str); // 获取本机ID
void start_get_device_id(uint32_t *id);

void task_suspend_function(void);

#endif
