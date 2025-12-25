/*
*********************************************************************************************************
*
*	模块名称 : 头文件汇总
*	文件名称 : BSP.h
*	版    本 : V1.0
*	说    明 : 当前使用头文件汇总
*
*	修改记录 :
*		版本号    日期        作者     说明
*		V1.0    2015-08-02  Eric2013   首次发布
*
*	Copyright (C), 2015-2020,
*
*********************************************************************************************************
*/
#ifndef _BSP_H_
#define _BSP_H_

#include  <stdio.h>
#include  <string.h>

#include "EventRecorder.h"
#include "SEGGER_RTT.h"

//#define Enable_EventRecorder // 选择使用EVR
//#define Enable_RTTViewer   // 选择使用RTT
#define Enable_USART   // 选择使用UART

#ifdef Enable_EventRecorder

#elif defined Enable_RTTViewer
#define printf(...) do { SEGGER_RTT_SetTerminal(0);   \
		                     SEGGER_RTT_printf(0, __VA_ARGS__); \
                         }while(0)
												 
#elif defined Enable_USART

#endif

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"

#include "bsp_led.h"
#include "bsp_iwdg.h"
#include "stmflash.h"
#include "bsp_usart2.h"
#include "bsp_relay.h"
#include "bsp_key.h"
#include "bsp_rtc.h"
#include "bsp_usart1.h"
#include "bsp_usart3.h"
#include "bsp_usart4.h"
#include "bsp_uart5.h"
#include "bsp_usart6.h"
#include "bsp_uart7.h"
#include "bsp_uart8.h"
#include "bsp_lpuart1.h"
#include "bsp_timers.h"




#endif

