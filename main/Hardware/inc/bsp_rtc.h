#ifndef __BSP_RTC_H
#define __BSP_RTC_H

#include "./SYSTEM/sys/sys.h"

extern RTC_HandleTypeDef g_rtc_handle;

typedef struct
{
	uint16_t year;
	uint8_t  month;
	uint8_t  data;
	uint8_t  week;
	uint8_t  hour;
	uint8_t  min;
	uint8_t  sec;
} rtc_time_t;

/******************************************************************************************/
uint8_t bsp_InitRTC(void);										// RTC初始化
void RTC_AlarmSet(void);

uint32_t rtc_read_bkr(uint32_t bkrx);					/* 读后备寄存器 */
void rtc_write_bkr(uint32_t bkrx, uint32_t data);		/* 写后备寄存器 */

HAL_StatusTypeDef RTC_Set_Time(uint8_t hour,uint8_t min,uint8_t sec,uint8_t ampm);	// RTC时间设置
HAL_StatusTypeDef RTC_Set_Date(uint8_t year, uint8_t month, uint8_t date, uint8_t week); // RTC日期设置
void RTC_Set_AlarmA(uint8_t week,uint8_t hour,uint8_t min,uint8_t sec);			// 设置闹钟时间(按星期闹铃,24小时制)
void RTC_Set_WakeUp(uint32_t wksel,uint16_t cnt);						// 周期性唤醒定时器设置
void RTC_Get_Time(rtc_time_t *rtc);
void RTC_set_Time(rtc_time_t rtc);
void TimeBySecond(uint32_t second);
void local_to_utc_time(rtc_time_t *utc_time, int8_t timezone, rtc_time_t local_time);

void RTC_Get_Time_Test(void);

#endif

















