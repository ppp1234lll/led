/********************************************************************************
* @File name  : 按键模块
* @Description: 头文件
* @Author     : ZHLE
*  Version Date        Modification Description
	4、输入检测
		    按键(恢复出厂设置):    PD2
		    箱门检测:              PA11
		    12V电源输入监测:       PD0
		    水浸 :                 PD13	
        输入检测1：           PD14
        输入检测2：           PD15
        输入检测3：           PC8
        市电火-地：           PA3
        市电零-地：           PA4
        市电零火线：          PA5
********************************************************************************/

#ifndef _BSP_KEY_H_
#define _BSP_KEY_H_


#include "./SYSTEM/sys/sys.h"

//引脚定义
/*******************************************************/
typedef enum
{
	KEY_PWR = 0,
	KEY_DOOR1,
	KEY_DOOR2,
	KEY_DOOR3,
	KEY_DOOR4,
	KEY_WATER0,
	KEY_WATER1,
	KEY_MCB,

	KEY_MAX // 最大按键，用于提供按键数量
} key_id_t;

/* 函数声明 */
void bsp_InitKey(void);
uint8_t KeyScan10ms(uint8_t id);
void key_test(void);

#endif
