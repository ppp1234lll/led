/********************************************************************************
* @File name  : RTC实时时钟驱动文件
* @Description: 基于STM32的RTC驱动，支持时间/日期设置、读取、闹钟配置及唤醒定时器功能
* @Author     : ZHLE
*  Version Date        Modification Description
********************************************************************************/
#include "bsp_rtc.h"
#include "bsp.h"
#include <time.h>

// 闹钟相关宏定义
#define ALARM_HOURS               12          // 0~23
#define ALARM_MINUTES             10          // 0~59
#define ALARM_SECONDS             00          // 0~59

#define RTC_BKP_DATA_LSI         0x32F2 
#define RTC_BKP_DATA_LSE         0x32F3 

RTC_HandleTypeDef g_rtc_handle;     /* RTC句柄 */

/**
 * @brief       RTC底层驱动, 时钟配置
 * @param       hrtc : RTC句柄
 * @note        此函数会被HAL_RTC_Init()调用
 * @retval      无
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    uint16_t retry = 200;

    RCC_OscInitTypeDef rcc_osc_init_handle;
    RCC_PeriphCLKInitTypeDef rcc_periphclk_init_handle;

    __HAL_RCC_RTC_ENABLE();                                                 /* RTC使能 */
    HAL_PWR_EnableBkUpAccess();                                             /* 取消备份区域写保护 */
    __HAL_RCC_RTC_ENABLE();                                                 /* RTC使能 */

    /* 使用寄存器的方式去检测LSE是否可以正常工作 */
    RCC->BDCR |= 1 << 0;                                                    /* 尝试开启LSE */

    while (retry && ((RCC->BDCR & 0x02) == 0))                              /* 等待LSE准备好 */
    {
        retry--;
        delay_ms(5);
    }
    
//    if (retry == 0)                                                         /* LSE起振失败 使用LSI */
    {
        rcc_osc_init_handle.OscillatorType = RCC_OSCILLATORTYPE_LSI;        /* 选择要配置的振荡器 */
        rcc_osc_init_handle.LSIState = RCC_LSI_ON;                          /* LSI状态：开启 */
        rcc_osc_init_handle.PLL.PLLState = RCC_PLL_NONE;                    /* PLL无配置 */
        HAL_RCC_OscConfig(&rcc_osc_init_handle);                            /* 配置设置的rcc_oscinitstruct */

        rcc_periphclk_init_handle.PeriphClockSelection = RCC_PERIPHCLK_RTC; /* 选择要配置外设 RTC */
        rcc_periphclk_init_handle.RTCClockSelection = RCC_RTCCLKSOURCE_LSI; /* RTC时钟源选择LSI */
        HAL_RCCEx_PeriphCLKConfig(&rcc_periphclk_init_handle);              /* 配置设置的rcc_periphclkinitstruct */
        rtc_write_bkr(0, RTC_BKP_DATA_LSI);
    }
//    else
//    {
//        rcc_osc_init_handle.OscillatorType = RCC_OSCILLATORTYPE_LSE;        /* 选择要配置的振荡器 */
//        rcc_osc_init_handle.PLL.PLLState = RCC_PLL_NONE;                    /* PLL不配置 */
//        rcc_osc_init_handle.LSEState = RCC_LSE_ON;                          /* LSE状态:开启 */
//        HAL_RCC_OscConfig(&rcc_osc_init_handle);                            /* 配置设置的rcc_oscinitstruct */

//        rcc_periphclk_init_handle.PeriphClockSelection = RCC_PERIPHCLK_RTC; /* 选择要配置外设RTC */
//        rcc_periphclk_init_handle.RTCClockSelection = RCC_RTCCLKSOURCE_LSE; /* RTC时钟源选择LSE */
//        HAL_RCCEx_PeriphCLKConfig(&rcc_periphclk_init_handle);              /* 配置设置的rcc_periphclkinitstruct */
//        rtc_write_bkr(0, RTC_BKP_DATA_LSE);
//    }
}


/*
*********************************************************************************************************
*	函 数 名: bsp_InitRTC
*	功能说明: RTC初始化。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t bsp_InitRTC(void)
{
	uint16_t bkpflag = 0;
	
	g_rtc_handle.Instance = RTC;
	g_rtc_handle.Init.HourFormat = RTC_HOURFORMAT_24;   /* RTC设置为24小时格式 */
	g_rtc_handle.Init.AsynchPrediv = 0x7F;              /* RTC异步分频系数(1~0x7F) */
	g_rtc_handle.Init.SynchPrediv = 0xFF;               /* RTC同步分频系数(0~0x7FFF) */
	g_rtc_handle.Init.OutPut = RTC_OUTPUT_DISABLE;
	g_rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	g_rtc_handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

	/* 检查是不是第一次配置时钟 */
	bkpflag = rtc_read_bkr(0);                          /* 读取BKP0的值 */

	if (HAL_RTC_Init(&g_rtc_handle) != HAL_OK)
	{
		return 1;
	}

	if ((bkpflag != RTC_BKP_DATA_LSI) && (bkpflag != RTC_BKP_DATA_LSE))     /* 之前未初始化过, 重新配置 */
	{
		RTC_Set_Time(6, 59, 56, RTC_HOURFORMAT12_AM);   /* 设置时间, 根据实际时间修改 */
		RTC_Set_Date(20, 4, 22, 3);                     /* 设置日期 */
	}
	
	RTC_AlarmSet();
	return 0;
}


/**
 * @brief       RTC写入后备区域SRAM
 * @param       bkrx : 后备区寄存器编号,范围:0~31
 *                     对应 RTC_BKP_DR0~RTC_BKP_DR31
 * @param       data : 要写入的数据,32位长度
 * @retval      无
 */
void rtc_write_bkr(uint32_t bkrx, uint32_t data)
{
    HAL_PWR_EnableBkUpAccess();     /* 取消备份区写保护 */
    HAL_RTCEx_BKUPWrite(&g_rtc_handle, bkrx, data);
}

/**
 * @brief       RTC读取后备区域SRAM
 * @param       bkrx : 后备区寄存器编号,范围:0~31 
 * @retval      读取到的值
 */
uint32_t rtc_read_bkr(uint32_t bkrx)
{
    uint32_t temp = 0;
    temp = HAL_RTCEx_BKUPRead(&g_rtc_handle, bkrx);
    return temp;                    /* 返回读取到的值 */
}

/*
*********************************************************************************************************
*	函 数 名: RTC_Set_Time
*	功能说明: 设置RTC时间（支持12小时制）
*	形    参: 
*	@hour		: 小时（12小时制：1-12；24小时制：0-23，需配合HourFormat）
*	@min		: 分钟（0-59）
*	@sec		: 秒钟（0-59）
*	@ampm		: AM/PM, 0=AM/24H; 1=PM/12H
*	返 回 值:		0,成功
*				      其他,异常状态
*********************************************************************************************************
*/
HAL_StatusTypeDef RTC_Set_Time(uint8_t hour,uint8_t min,uint8_t sec,uint8_t ampm)
{
	RTC_TimeTypeDef rtc_time_handle;

	rtc_time_handle.Hours = hour;
	rtc_time_handle.Minutes = min;
	rtc_time_handle.Seconds = sec;
	rtc_time_handle.TimeFormat = ampm;
	rtc_time_handle.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	rtc_time_handle.StoreOperation = RTC_STOREOPERATION_RESET;

	return HAL_RTC_SetTime(&g_rtc_handle, &rtc_time_handle, RTC_FORMAT_BIN);
}

/*
*********************************************************************************************************
*	函 数 名: RTC_Set_Date
*	功能说明: 设置RTC日期
*	形    参: 
*	@year		: 年份（两位数，如24代表2024年）
*	@month		: 月份（1-12）
*	@date		: 日期（1-31）
*	@week		: 星期（1-7，1=周一，7=周日，具体需配合RTC硬件定义）
*	返 回 值: 0,成功
*          其他,异常状态
*********************************************************************************************************
*/
HAL_StatusTypeDef RTC_Set_Date(uint8_t year, uint8_t month, uint8_t date, uint8_t week)
{
	RTC_DateTypeDef rtc_date_handle;

	rtc_date_handle.Date = date;
	rtc_date_handle.Month = month;
	rtc_date_handle.WeekDay = week;
	rtc_date_handle.Year = year;

	return HAL_RTC_SetDate(&g_rtc_handle, &rtc_date_handle, RTC_FORMAT_BIN);
}

/*
*********************************************************************************************************
*	函 数 名: TimeBySecond
*	功能说明: 根据Unix时间戳（秒数）设置RTC时间（含时区转换：UTC+8）
*	形    参: 
*	@second		: Unix时间戳（从1970-01-01 00:00:00 UTC开始的秒数）
*	返 回 值: 无
*********************************************************************************************************
*/
void TimeBySecond(uint32_t second)
{
	struct tm *pt,t;
	rtc_time_t time_t;
	second += 8*60*60;  // 时区转换：UTC+8，将传入的UTC时间戳转为本地时间
	pt = localtime(&second);
	
	if(pt == NULL)
		return;
	t=*pt;
	t.tm_year+=1900;  // tm_year是从1900年开始的偏移，转为4位完整年份
	t.tm_mon++;        // tm_mon是0-11，转为1-12的月份
	time_t.year  = t.tm_year;  // 4位年份（如2024）
	time_t.month = t.tm_mon;   // 月份（1-12）
	time_t.data  = t.tm_mday;  // 日期（1-31，变量名data实际为day）
	time_t.hour  = t.tm_hour;  // 小时（24小时制，0-23）
	time_t.min   = t.tm_min;   // 分钟（0-59）
	time_t.sec   = t.tm_sec;   // 秒钟（0-59）
	/* 调用统一时间设置接口 */
	RTC_set_Time(time_t);
}

/* 月份天数对照表（用于计算星期） */										 
const uint8_t cg_table_week[12]={0,3,3,6,1,4,6,2,5,0,3,5};
/*
*********************************************************************************************************
*	函 数 名: get_week_form_time
*	功能说明: 通过年月日计算对应的星期几
*	形    参: 
*	@year		: 4位完整年份（如2024）
*	@month		: 月份（1-12）
*	@day		: 日期（1-31）
*	返 回 值: 星期标识（0=周日，1=周一，...，6=周六，需根据实际需求调整）
*********************************************************************************************************
*/
static uint8_t get_week_form_time(uint16_t year,uint8_t month,uint8_t day)
{	
	uint16_t temp2;
	uint8_t yearH,yearL;
	
	yearH=year/100;	yearL=year%100; 
	// 若为21世纪（yearH>19），年份低位加100（适配计算公式）
	if (yearH>19)yearL+=100;
	// 计算核心逻辑（仅支持1900年以后）  
	temp2=yearL+yearL/4;
	temp2=temp2%7; 
	temp2=temp2+day+cg_table_week[month-1];
	// 若为闰年且月份小于3，需减1（修正闰年2月的影响）
	if (yearL%4==0&&month<3)temp2--;
	
	temp2 %= 7;

	if (temp2 == 0)
			temp2 = 7;
	
	return temp2;
}	

/*
*********************************************************************************************************
*	函 数 名: RTC_set_Time
*	功能说明: 统一设置RTC时间和日期（自动计算星期）
*	形    参: 
*	@rtc		: rtc_time_t类型结构体，包含时间日期信息
*				  rtc.year：4位完整年份（如2024）
*				  rtc.month：月份（1-12）
*				  rtc.data：日期（1-31，变量名data实际为day）
*				  rtc.hour：小时（24小时制，0-23）
*				  rtc.min：分钟（0-59）
*				  rtc.sec：秒钟（0-59）
*	返 回 值: 无
*********************************************************************************************************
*/
void RTC_set_Time(rtc_time_t rtc)
{
	// 自动计算星期并赋值
	rtc.week = get_week_form_time(rtc.year,rtc.month,rtc.data);
	
	// 根据小时判断12小时制的上下午（小时>12为下午，否则为上午）
	if(rtc.hour > 12)
		RTC_Set_Time(rtc.hour,rtc.min,rtc.sec,RTC_HOURFORMAT12_PM);
	else
		RTC_Set_Time(rtc.hour,rtc.min,rtc.sec,RTC_HOURFORMAT12_AM);

	// 设置日期（年份需转为两位数：如2024→24）
	RTC_Set_Date(rtc.year-2000,rtc.month,rtc.data,rtc.week);
	// 重复设置一次日期（确保硬件稳定写入）
	RTC_Set_Date(rtc.year-2000,rtc.month,rtc.data,rtc.week);
	
}

/*
*********************************************************************************************************
*	函 数 名: RTC_Get_Time
*	功能说明: 读取当前RTC时间和日期
*	形    参: 
*	@rtc		: 指向rtc_time_t类型的指针，用于存储读取到的时间日期
*				  rtc.year：4位完整年份（如2024，由硬件两位数年份+2000得到）
*				  rtc.month：月份（1-12）
*				  rtc.data：日期（1-31，变量名data实际为day）
*				  rtc.hour：小时（24小时制，0-23）
*				  rtc.min：分钟（0-59）
*				  rtc.sec：秒钟（0-59）
*				  rtc.week：星期（1-7，由硬件直接读取）
*	返 回 值: 无
*********************************************************************************************************
*/
void RTC_Get_Time(rtc_time_t *rtc)
{
	RTC_TimeTypeDef rtc_time_handle;
	RTC_DateTypeDef rtc_date_handle;
	
	HAL_RTC_GetTime(&g_rtc_handle, &rtc_time_handle, RTC_FORMAT_BIN);
	rtc->hour = rtc_time_handle.Hours;
	rtc->min  = rtc_time_handle.Minutes;
	rtc->sec  = rtc_time_handle.Seconds;
  
	HAL_RTC_GetDate(&g_rtc_handle, &rtc_date_handle, RTC_FORMAT_BIN);
	rtc->year  = rtc_date_handle.Year+2000;
	rtc->month = rtc_date_handle.Month;
	rtc->data  = rtc_date_handle.Date;
	rtc->week  = rtc_date_handle.WeekDay;
}

/*
*********************************************************************************************************
*	函 数 名: time_to_second_function
*	功能说明: 时间（年月日时分秒）转换为Unix时间戳（秒数）- 适配NB通信
*	形    参: 
*	@time		: 时间数组，格式为[年,月,日,时,分,秒]（年为4位完整年份，如2024）
*	@second		: 指向uint32_t的指针，用于存储转换后的Unix时间戳
*	返 回 值: 无
*********************************************************************************************************
*/
void time_to_second_function(uint32_t *time, uint32_t *second)
{
	struct tm pt;
	
	pt.tm_year = time[0]+100;  // tm_year是从1900年开始的偏移（如2024→124=2024-1900）
	pt.tm_mon  = time[1] - 1;  // tm_mon是0-11，需将1-12的月份减1
	pt.tm_mday = time[2];      // 日期（1-31）
	pt.tm_hour = time[3];      // 小时（24小时制，0-23）
	pt.tm_min  = time[4];      // 分钟（0-59）
	pt.tm_sec  = time[5];      // 秒钟（0-59）
	
	*second = mktime(&pt);  // 转换为Unix时间戳
}

/*
*********************************************************************************************************
*	函 数 名: local_to_utc_time
*	功能说明: 本地时间转换为UTC时间（支持时区偏移计算，处理跨月/跨年）
*	形    参:  
*	@utc_time	: 指向rtc_time_t的指针，用于存储转换后的UTC时间
*	@timezone	: 时区偏移（本地时间=UTC时间+timezone，如UTC+8时区传入-8，UTC-5传入5）
*	@local_time	: 输入的本地时间（rtc_time_t结构体，含年/月/日/时/分/秒/星期）
*	返 回 值: 无
*********************************************************************************************************
*/
void local_to_utc_time(rtc_time_t *utc_time, int8_t timezone, rtc_time_t local_time)
{
	int year,month,day,hour,week;
	int lastday = 0;			// 当前月份的总天数
	int lastlastday = 0;		// 上一个月份的总天数

	// 初始化本地时间参数
	year	= local_time.year;	// 4位完整年份
	month = local_time.month;	// 月份（1-12）
	day 	= local_time.data;	// 日期（1-31，变量名data实际为day）
	hour 	= local_time.hour + timezone;  // 计算UTC小时（本地小时+时区偏移）
	week  = local_time.week;	// 星期（1-7）
	
	// 计算当前月份和上一个月份的总天数（处理闰年2月）
	// 大月（1,3,5,7,8,10,12）：31天；小月（4,6,9,11）：30天；2月：平年28天，闰年29天
	if(month==1 || month==3 || month==5 || month==7 || month==8 || month==10 || month==12)
	{
		lastday = 31;  // 当前月份为大月，31天
		lastlastday = 30;  // 上一个月份默认30天（需特殊处理3月和1月）
		
		if(month == 3)  // 当前月份是3月，上一个月份是2月（需判断闰年）
		{
			if((year%400 == 0)||(year%4 == 0 && year%100 != 0))// 闰年判断
				lastlastday = 29;
			else
				lastlastday = 28;
		}
		
		if(month == 8 || month == 1)  // 当前月份是8月（上一个月7月，31天）或1月（上一个月12月，31天）
			lastlastday = 31;
	}
	else if(month == 4 || month == 6 || month == 9 || month == 11)  // 当前月份为小月，30天
	{
		lastday = 30;
		lastlastday = 31;  // 上一个月份为大月，31天
	}
	else  // 当前月份是2月（需判断闰年）
	{
		lastlastday = 31;  // 上一个月份是1月，31天
		if((year%400 == 0)||(year%4 == 0 && year%100 != 0))// 闰年
			lastday = 29;
		else
			lastday = 28;
	}

	// 处理小时>=24的情况（跨天，日期+1，星期+1）
	if(hour >= 24)
	{					
		hour -= 24;
		day += 1;
		week += 1;		// 星期加1
		if(week > 7)
			week = 1;  // 星期超过7则重置为1（1=周一）  
		// 处理日期超过当前月份总天数（跨月，月份+1）
		if(day > lastday)
		{ 		
			day -= lastday;
			month += 1;
			// 处理月份超过12（跨年，年份+1）
			if(month > 12)
			{
				month -= 12;
				year += 1;
			}
		}
	}
	
	// 处理小时<0的情况（跨天，日期-1，星期-1）
	if(hour < 0)
	{
		hour += 24;
		day -= 1;
		week -= 1;
		if(week < 1)
			week = 7;  // 星期小于1则重置为7（7=周日）
		// 处理日期<1（跨月，月份-1）
		if(day < 1)
		{
			day = lastlastday;  // 日期设为上一个月份的总天数
			month -= 1;
			// 处理月份<1（跨年，年份-1）
			if(month < 1)
			{
				month = 12;
				year -= 1;
			}
		}
	}

	// 赋值转换后的UTC时间
	utc_time->year  = year;
	utc_time->month = month;
	utc_time->data  = day;
	utc_time->week 	= week;
	utc_time->hour  = hour;
	utc_time->min	 	= local_time.min;  // 分钟不变
	utc_time->sec	 	= local_time.sec;  // 秒钟不变
}

/*
*********************************************************************************************************
*	函 数 名: RTC_AlarmSet
*	功能说明: 使能 RTC 闹钟中断
*	形    参: 无
*	返 回 值: 无
*    要使能 RTC 闹钟中断，需按照以下顺序操作：
* 1. 将 EXTI 线 17 配置为中断模式并将其使能，然后选择上升沿有效。
* 2. 配置 NVIC 中的 RTC_Alarm IRQ 通道并将其使能。
* 3. 配置 RTC 以生成 RTC 闹钟（闹钟 A 或闹钟 B）。
*********************************************************************************************************
*/
void RTC_AlarmSet(void)
{
	RTC_AlarmTypeDef rtc_alarm_handle;
	
	rtc_alarm_handle.AlarmTime.Hours = ALARM_HOURS;         	     /* 小时 */
	rtc_alarm_handle.AlarmTime.Minutes = ALARM_MINUTES;     	        /* 分钟 */
	rtc_alarm_handle.AlarmTime.Seconds = ALARM_SECONDS;     	        /* 秒 */
	rtc_alarm_handle.AlarmTime.SubSeconds = 0;	
	rtc_alarm_handle.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;
	
	rtc_alarm_handle.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;			/* 日期或者星期无效 */
	rtc_alarm_handle.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
	rtc_alarm_handle.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE; /* 按星期 */
	rtc_alarm_handle.AlarmDateWeekDay = 1;                               /* 星期 */
	rtc_alarm_handle.Alarm = RTC_ALARM_A;                                   /* 闹钟A */
	HAL_RTC_SetAlarm_IT(&g_rtc_handle, &rtc_alarm_handle, RTC_FORMAT_BIN);
	
	HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 3, 3);                             /* 抢占优先级1,子优先级2 */
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
}
/**
 * @breif       RTC闹钟中断服务函数
 * @param       无
 * @retval      无
 */
void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&g_rtc_handle);
}

/**
 * @breif       RTC闹钟A中断处理回调函数
 * @param       hrtc : RTC句柄
 * @retval      无
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    printf("ALARM A!\r\n");
}

/*
*********************************************************************************************************
*	函 数 名: RTC_Get_Time_Test
*	功能说明: RTC时间读取测试函数（循环打印当前时间）
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void RTC_Get_Time_Test(void)
{
	static rtc_time_t rtc_test;
	while(1)
	{
		RTC_Get_Time(&rtc_test);
		// 打印格式：日期-月份-年份，星期，时间（时:分:秒）
		printf("data:%d-%d-%d,week:%d,time:%d:%d:%d",\
		       rtc_test.year,rtc_test.month,rtc_test.data,\
           rtc_test.week,rtc_test.hour,rtc_test.min,rtc_test.sec);
		delay_ms(1000);  // 每秒刷新一次
	}
}







