#ifndef __BSP_HSPI2_BL0939_H
#define __BSP_HSPI2_BL0939_H

#include "./SYSTEM/sys/sys.h"
								  
/* 提供给其他C文件调用的函数 */
void bsp_InitSPI2(void);			 		   // 初始化SPI口
uint8_t SPI2_ReadWriteByte(uint8_t TxData);
void SPI2_Write_Multi_Byte(uint8_t *buff, uint16_t len);
uint8_t HSPI2_ReadByte(void);
void HSPI2_test(void);

#endif


