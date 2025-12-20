#ifndef __BSP_SPI_FLASH_H
#define __BSP_SPI_FLASH_H

#include "./SYSTEM/sys/sys.h"

void bsp_InitSPI_Flash(void);			 		   // 初始化SPI口
uint8_t SPI_ReadWriteByte(uint8_t txdata); // SPI总线读写一个字节
		 
#endif

