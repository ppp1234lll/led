#ifndef __BSP_HSPI4_H
#define __BSP_HSPI4_H

#include "./SYSTEM/sys/sys.h"
								  
/* 提供给其他C文件调用的函数 */
void bsp_InitHSPI4(void); // 初始化SPI口

void HSPI4_Send_Data(uint8_t *buff, uint16_t len);	 
void HSPI4_Read_Data(uint8_t *txdata,uint8_t *rxdata, uint16_t len);	 
 
#endif


