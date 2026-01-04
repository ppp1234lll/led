#ifndef __BSP_SSPI_BL0939_H
#define __BSP_SSPI_BL0939_H

#include "./SYSTEM/sys/sys.h"
								  
/* 提供给其他C文件调用的函数 */
void bsp_InitSSPI(void);			 		   // 初始化SPI口
uint8_t SSPI_ReadWriteByte(uint8_t TxData); // SPI总线读写一个字节
		 
void SSPI_WriteByte(uint8_t TxData);
void SSPI_Write_Multi_Byte(uint8_t *buff, uint16_t len);
uint8_t SSPI_ReadByte(void);
		 
void SSPI_test(void);
		 
#endif

