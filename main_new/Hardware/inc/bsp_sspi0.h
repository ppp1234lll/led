#ifndef __BSP_SSPI0_H
#define __BSP_SSPI0_H

#include "./SYSTEM/sys/sys.h"
								  
/* 提供给其他C文件调用的函数 */
void bsp_InitSSPI0(void);			 		   // 初始化SPI口

void SSPI0_WriteByte(uint8_t TxData) ;
void SSPI0_Write_Multi_Byte(uint8_t *buff, uint16_t len);	 
uint8_t SSPI0_ReadByte(void);	 
void sspi0_test(void);

#endif


