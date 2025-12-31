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

#ifndef __BSP_UART8_H
#define __BSP_UART8_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/

extern UART_HandleTypeDef huart8;          /* UART句柄 */
extern DMA_HandleTypeDef hdma_uart8_rx;
extern DMA_HandleTypeDef hdma_uart8_tx;
extern UART_TxCpltCallbackFunc   usart8_txcplt_callback;
extern UART_RxCpltCallbackFunc   usart8_rxcplt_callback ;
extern UART_RxEventCallbackFunc  usart8_rxeventcplt_callback;
/******************************************************************************************/

void bsp_InitUart8(uint32_t baudrate);
void Uart8_Send_Data(uint8_t *buf, uint16_t len);

void uart8_test(void);

#endif
