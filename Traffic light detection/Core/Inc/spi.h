/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.h
  * @brief   This file contains all the function prototypes for
  *          the spi.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern SPI_HandleTypeDef hspi1;

extern SPI_HandleTypeDef hspi2;

extern SPI_HandleTypeDef hspi3;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_SPI1_Init(void);
void MX_SPI2_Init(void);
void MX_SPI3_Init(void);

/* USER CODE BEGIN Prototypes */

uint8_t hardSPI_ReadByte(void);
uint8_t hardSPI_ReadWriteByte(uint8_t TxData);
void hardSPI_WriteByte(uint8_t TxData);
void hardSPI_Write_Multi_Byte(uint8_t *buff, uint16_t len);


uint8_t hardSPI_2_ReadByte(void);
uint8_t hardSPI_2_ReadWriteByte(uint8_t TxData);
void hardSPI_2_WriteByte(uint8_t TxData);
void hardSPI_2_Write_Multi_Byte(uint8_t *buff, uint16_t len);


uint8_t hardSPI_3_ReadByte(void);
uint8_t hardSPI_3_ReadWriteByte(uint8_t TxData);
void hardSPI_3_WriteByte(uint8_t TxData);
void hardSPI_3_Write_Multi_Byte(uint8_t *buff, uint16_t len);


/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */

