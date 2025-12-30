/**
 ****************************************************************************************************
 * @file        rs485.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-06
 * @brief       RS485 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 * 实验平台:正点原子 阿波罗 H743开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20220906
 * 第一次发布
 *
 ****************************************************************************************************
 */

#ifndef __BSP_USART2_H
#define __BSP_USART2_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/

extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern UART_HandleTypeDef huart2;
extern UART_RxCpltCallbackFunc huart2_rxcplt_cb;
/******************************************************************************************/

void bsp_InitUsart2(uint32_t bound);
void usart2_send_data(uint8_t *buf, uint8_t len);
 
void usart2_test(void);

#endif
