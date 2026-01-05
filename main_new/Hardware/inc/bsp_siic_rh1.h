/**
 ****************************************************************************************************
 * @file        myiic.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-06
 * @brief       IIC 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
	9、AHT20温湿度传感器：(模拟IIC方式)，引脚分配为：  
		SCL:	PA8
		SDA:  PC9

		SCL:	PD12
		SDA:  PD13

 ****************************************************************************************************
 */
 
#ifndef __BSP_SIIC_RH1_H
#define __BSP_SIIC_RH1_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* IIC所有操作函数 */
void bsp_siic_rh1_init(void);                /* 初始化IIC的IO口 */
void rh1_iic_start(void);               /* 发送IIC开始信号 */
void rh1_iic_stop(void);                /* 发送IIC停止信号 */
void rh1_iic_ack(void);                 /* IIC发送ACK信号 */
void rh1_iic_nack(void);                /* IIC不发送ACK信号 */
uint8_t rh1_iic_wait_ack(void);         /* IIC等待ACK信号 */
void rh1_iic_send_byte(uint8_t data);   /* IIC发送一个字节 */
uint8_t rh1_iic_read_byte(uint8_t ack); /* IIC读取一个字节 */

#endif

