#ifndef __BSP_HSPI_BL0910_H
#define __BSP_HSPI_BL0910_H

#include "./SYSTEM/sys/sys.h"
								  
/* 提供给其他C文件调用的函数 */
void    bsp_InitHSPI(void); // 初始化SPI口

void HSPI_Send_Data(uint8_t *buff, uint16_t len);	 
void HSPI_Read_Data(uint8_t *txdata,uint8_t *rxdata, uint16_t len);	 
void HSPI_test(void);

#endif


