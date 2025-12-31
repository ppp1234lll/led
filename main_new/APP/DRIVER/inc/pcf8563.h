#ifndef __PCF8563_H
#define __PCF8563_H

#include "sys.h"

//PCF8563的I2C总线从地址7位地址为0x51
#define PCF8563_SLAVE_ADDRESS   (0x51<<1)    //PCF8563器件写地址


#define PCF8563_CONTROL_STATUS_1_REG 0x00    //控制/状态寄存器 1
#define PCF8563_CONTROL_STATUS_2_REG 0x01    //控制/状态寄存器 2

#define PCF8563_VL_SECONDS_REG       0x02    //秒寄存器
#define PCF8563_MINUTES_REG          0x03    //分钟寄存器
#define PCF8563_HOURS_REG            0x04    //小时寄存器
#define PCF8563_DAYS_REG             0x05    //日寄存器
#define PCF8563_WEEK_REG             0x06    //星期寄存器
#define PCF8563_MONTHS_CENTURY_REG   0x07    //月/世纪寄存器
#define PCF8563_YEARS_REG            0x08    //年寄存器

#define PCF8563_MINUTE_ALARM_REG     0x09    //分钟报警寄存器
#define PCF8563_HOUR_ALARM_REG       0x0A    //小时报警寄存器
#define PCF8563_DAY_ALARM_REG        0x0B    //日报警寄存器
#define PCF8563_WEEK_ALARM_REG       0x0C    //星期报警寄存器

#define PCF8563_CLKOUT_FREQUENCY_REG 0x0D    //CLKOUT 频率寄存器

#define PCF8563_TIMER_CONTROL_REG    0x0E    //定时器控制寄存器
#define PCF8563_TIMER_COUNTDOWN_REG  0x0F    //定时器倒计数寄存器

//设置时间，BCD码
#define SECONDS                     0x00    //秒
#define MINUTES                     0x57    //分钟
#define HOURS                       0x11    //小时
#define DAYS                        0x20    //日
#define WEEK                        0x05    //星期
#define MONTHS_CENTURY              0x09    //月/世纪
#define YEARS                       0x24    //年

extern uint8_t clkout_frequency[5];

void PCF8563_ReadData(uint8_t reg_add,unsigned char *Read,uint8_t num);
void PCF8563_WriteReg(uint8_t reg_add,uint8_t reg_dat);

void PCF8563_Init(void);
void PCF8563_Countdown(uint8_t value);
void PCF8563_Clearflag(void);

uint8_t PCF8563_dec2bcd(uint8_t value);
uint8_t PCF8563_bcd2dec(uint8_t value);

void PCF8563_Test(void);
#endif  /*__BSP_I2C_PCF8563_H*/
