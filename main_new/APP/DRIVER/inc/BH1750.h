#ifndef __BH1750_H_
#define __BH1750_H_

#include "sys.h"

// 命令
#define BH1750_Addr                       0x46   //address =0
#define Power_Down                        0x00 
#define Power_On                          0x01
#define Reset                             0x07
#define Continuously_HResolution_Mode     0x10
#define Continuously_HResolution_Mode2    0x11   
#define Continuously_LResolution_Mode     0x13    
#define OneTime_HResolution_Mode          0x20   
#define OneTime_HResolution_Mode2         0x21     
#define OneTime_LResolution_Mode          0x23     


/* 提供给其他C文件调用的函数 */
void BH1750_Init(void);
uint16_t BH1750_ReadI2C_Data(uint8_t Slave_Address);
void BH1750_WriteI2C_Byte(uint8_t Slave_Address,uint8_t REG_Address);

void bh1750_test(void);
#endif
