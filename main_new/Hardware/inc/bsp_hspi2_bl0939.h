#ifndef __BSP_HSPI2_BL0939_H
#define __BSP_HSPI2_BL0939_H

#include "./SYSTEM/sys/sys.h"
								  
/* 提供给其他C文件调用的函数 */
void bsp_InitSPI2(void);			 		   // 初始化SPI口
void HSPI2_Send_Data(uint8_t *buff, uint16_t len);
void HSPI2_Read_Data(uint8_t *txdata,uint8_t *rxdata, uint16_t len);

#endif


