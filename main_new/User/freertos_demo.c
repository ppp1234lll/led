/**
 ****************************************************************************************************
 * @file        freertos_demo.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-08-01
 * @brief       lwIP HTTPS 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */
 
#include "freertos_demo.h"
#include "./SYSTEM/delay/delay.h"
#include "./MALLOC/malloc.h"
#include "lwip_comm.h"
#include "lwip_demo.h"
#include "lwipopts.h"
#include "stdio.h" 
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


/******************************************************************************************************/
/*FreeRTOS配置*/

/* START_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
//#define START_TASK_PRIO         5           /* 任务优先级 */
//#define START_STK_SIZE          512         /* 任务堆栈大小 */
//TaskHandle_t StartTask_Handler;             /* 任务句柄 */
//void start_task(void *pvParameters);        /* 任务函数 */

/* LWIP_DEMO 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define LWIP_DMEO_TASK_PRIO     11          /* 任务优先级 */
#define LWIP_DMEO_STK_SIZE      1024        /* 任务堆栈大小 */
TaskHandle_t LWIP_Task_Handler;             /* 任务句柄 */
void lwip_demo_task(void *pvParameters);    /* 任务函数 */

/* LED_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define LED_TASK_PRIO           10          /* 任务优先级 */
#define LED_STK_SIZE            128         /* 任务堆栈大小 */
TaskHandle_t LEDTask_Handler;               /* 任务句柄 */
void led_task(void *pvParameters);          /* 任务函数 */

/******************************************************************************************************/


/**
 * @breif       freertos_demo
 * @param       无
 * @retval      无
 */
//void freertos_demo(void)
//{
//    /* start_task任务 */
////    xTaskCreate((TaskFunction_t )start_task,
////                (const char *   )"start_task",
////                (uint16_t       )START_STK_SIZE,
////                (void *         )NULL,
////                (UBaseType_t    )START_TASK_PRIO,
////                (TaskHandle_t * )&StartTask_Handler);

//    vTaskStartScheduler(); /* 开启任务调度 */
//}

/**
 * @brief       start_task
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void freertos_demo(void)
{
    taskENTER_CRITICAL();           /* 进入临界区 */
    
    /* 创建lwIP任务 */
    xTaskCreate((TaskFunction_t )lwip_demo_task,
                (const char*    )"lwip_demo_task",
                (uint16_t       )LWIP_DMEO_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )LWIP_DMEO_TASK_PRIO,
                (TaskHandle_t*  )&LWIP_Task_Handler);

//    /* LED测试任务 */
//    xTaskCreate((TaskFunction_t )led_task,
//                (const char*    )"led_task",
//                (uint16_t       )LED_STK_SIZE,
//                (void*          )NULL,
//                (UBaseType_t    )LED_TASK_PRIO,
//                (TaskHandle_t*  )&LEDTask_Handler);

    taskEXIT_CRITICAL();            /* 退出临界区 */
}

/**
 * @brief       lwIP运行例程
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void lwip_demo_task(void *pvParameters)
{
    pvParameters = pvParameters;

    lwip_demo();            /* lwip测试代码 */
    
    while(1)
    {
        vTaskDelay(5);
    }
}

/**
 * @brief       系统再运行
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
extern void fault_test_by_div0(void);
extern void fault_test_by_unalign(void);
void led_task(void *pvParameters)
{
    pvParameters = pvParameters;

//	fault_test_by_div0();
	
    while (1)
    {
//        LED1_TOGGLE();
        vTaskDelay(100);
    }
}
