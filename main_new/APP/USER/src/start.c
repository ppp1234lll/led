
#include "appconfig.h"

ChipID_t g_chipid_t;

/******************************************************************************************************/
/*FreeRTOS配置*/

/* START_TASK 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define START_TASK_PRIO         3           /* 任务优先级 */
#define START_STK_SIZE          512         /* 任务堆栈大小 */
TaskHandle_t StartTask_Handler;             /* 任务句柄 */
void start_task(void *pvParameters);        /* 任务函数 */


/* APP线程 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define APP_TASK_PRIO     20        /* 任务优先级 */
#define APP_STK_SIZE      1024      /* 任务堆栈大小 */
TaskHandle_t APP_Task_Handler;     /* 任务句柄 */
void app_task(void *pvParameters);  /* 任务函数 */

/* 网络线程 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define ETH_TASK_PRIO           19          /* 任务优先级 */
#define ETH_STK_SIZE            512         /* 任务堆栈大小 */
TaskHandle_t ETH_Task_Handler;              /* 任务句柄 */
void eth_task(void *pvParameters);          /* 任务函数 */

/* 网络线程 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define DET_TASK_PRIO           18          /* 任务优先级 */
#define DET_STK_SIZE            512         /* 任务堆栈大小 */
TaskHandle_t DET_Task_Handler;              /* 任务句柄 */
void det_task(void *pvParameters);          /* 任务函数 */

/* 网络线程 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define GSM_TASK_PRIO           17          /* 任务优先级 */
#define GSM_STK_SIZE            512         /* 任务堆栈大小 */
TaskHandle_t GSM_Task_Handler;              /* 任务句柄 */
void gsm_task(void *pvParameters);          /* 任务函数 */

/* 网络线程 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define PRINT_TASK_PRIO           10          /* 任务优先级 */
#define PRINT_STK_SIZE            512         /* 任务堆栈大小 */
TaskHandle_t PRINT_Task_Handler;              /* 任务句柄 */
void print_task(void *pvParameters);          /* 任务函数 */

/* 查看任务堆栈 */
#define LIST_TASK_PRIO		    9
#define LIST_TASK_STK_SIZE 		128
TaskHandle_t LIST_Task_Handler;
void list_task(void *p_arg);
/******************************************************************************************************/


/*
*********************************************************************************************************
*	函 数 名:  start_system_init_function
*	功能说明:  初始化所有的硬件设备
*	形    参:  无
*	返 回 值:  无
*********************************************************************************************************
*/
void start_task_create(void)
{
	/* start_task任务 */
	xTaskCreate((TaskFunction_t )start_task,
							(const char *   )"start_task",
							(uint16_t       )START_STK_SIZE,
							(void *         )NULL,
							(UBaseType_t    )START_TASK_PRIO,
							(TaskHandle_t * )&StartTask_Handler);

	vTaskStartScheduler(); /* 开启任务调度 */
}

/*
*********************************************************************************************************
*	函 数 名:  start_task
*	功能说明:   
*	形    参:  pvParameters : 传入参数(未用到)
*	返 回 值:  无
*********************************************************************************************************
*/
void start_task(void *pvParameters)
{
	pvParameters = pvParameters;
    
	cJSON_Hooks hook;                // 初始化JSON 

	iwdg_init(IWDG_PRESCALER_64, 1000);// 初始化看门狗(硬件、软件2s)
	start_get_device_id_function();    // 获取本机ID
	my_mem_init(SRAMIN);              // 内存初始化
	
	hook.malloc_fn = mymalloc_sramin;// 内存分配
	hook.free_fn   = myfree_sramin;  // 内存释放
	cJSON_InitHooks(&hook);          // 初始化自定义的内存分配和释放函数
	
	bsp_Init_DWT();
	bsp_InitLed();                   // LED初始化（已测试）
	bsp_InitRelay();				         // 继电器初始化（未测试）	
	bsp_InitKey();				           // 按键初始化	
	bsp_InitRTC();								   // RTC初始化 (已测试)
	bsp_InitUsart1(115200);
	bsp_InitUsart2(115200);
  bsp_InitUsart3(115200);
	bsp_InitUsart4(115200);
	bsp_InitUart5(115200);
	bsp_InitUsart6(9600);
	bsp_InitUart7(115200);
//	bsp_InitUart8(115200);	
	bsp_InitLpuart1(115200);	
	iwdg_feed();
	printf("\r\nCPU : STM32H743XIH6, BGA240, 主频: %dMHz\r\n", SystemCoreClock / 1000000);
	printf("main run...\n");
	
	bsp_InitTimers(TIM2,1000,2,0);
	bsp_InitTimers(TIM3,1000,2,0);
	bsp_InitTimers(TIM4,1000,2,0);
	bsp_InitTimers(TIM5,1000,2,0);
	bsp_InitTimers(TIM6,1000,2,0);
	bsp_InitTimers(TIM7,1000,2,0); 
//	aht20_init_function();

	bl0910_init_function();
	bl0939_init_function();
//ATGM338H_test();
	iwdg_feed();
//	bsp_InitSPIBus();	/* 配置SPI总线 */		
//	bsp_InitSFlash();	/* 初始化SPI 串行Flash */	
//	lfs_init_function();

    
	if (lwip_comm_init() != 0)
	{
		printf("lwIP Init failed!!\n");
		delay_ms(500);
		printf("Retrying...       \n");
		delay_ms(500);
	}
    
	taskENTER_CRITICAL();           /* 进入临界区 */

	xTaskCreate((TaskFunction_t )app_task,
							(const char *   )"app_task",
							(uint16_t       )APP_STK_SIZE,
							(void *         )NULL,
							(UBaseType_t    )APP_TASK_PRIO,
							(TaskHandle_t * )&APP_Task_Handler);

	xTaskCreate((TaskFunction_t )eth_task,
							(const char *   )"eth_task",
							(uint16_t       )ETH_STK_SIZE,
							(void *         )NULL,
							(UBaseType_t    )ETH_TASK_PRIO,
							(TaskHandle_t * )&ETH_Task_Handler);

	xTaskCreate((TaskFunction_t )det_task,
							(const char *   )"det_task",
							(uint16_t       )DET_STK_SIZE,
							(void *         )NULL,
							(UBaseType_t    )DET_TASK_PRIO,
							(TaskHandle_t * )&DET_Task_Handler);

	xTaskCreate((TaskFunction_t )gsm_task,
							(const char *   )"gsm_task",
							(uint16_t       )GSM_STK_SIZE,
							(void *         )NULL,
							(UBaseType_t    )GSM_TASK_PRIO,
							(TaskHandle_t * )&GSM_Task_Handler);

	xTaskCreate((TaskFunction_t )print_task,
							(const char *   )"print_task",
							(uint16_t       )PRINT_STK_SIZE,
							(void *         )NULL,
							(UBaseType_t    )PRINT_TASK_PRIO,
							(TaskHandle_t * )&PRINT_Task_Handler);

	xTaskCreate((TaskFunction_t )list_task,
							(const char *   )"list_task",
							(uint16_t       )LIST_TASK_STK_SIZE,
							(void *         )NULL,
							(UBaseType_t    )LIST_TASK_PRIO,
							(TaskHandle_t * )&LIST_Task_Handler);
		
	vTaskDelete(StartTask_Handler); /* 删除开始任务 */
	taskEXIT_CRITICAL();            /* 退出临界区 */					 
}

/*
*********************************************************************************************************
*	函 数 名: start_get_device_id_function
*	功能说明: 获取本机ID.  
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void start_get_device_id_function(void)
{
	volatile uint32_t addr = 0x1FF1E800;
	
	g_chipid_t.id[0] = *(__I uint32_t *)(addr + 0x00);
	g_chipid_t.id[1] = *(__I uint32_t *)(addr + 0x04);
	g_chipid_t.id[2] = *(__I uint32_t *)(addr + 0x08);
}

/*
*********************************************************************************************************
*	函 数 名: start_get_device_id_str
*	功能说明: 获取本机ID.  
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void start_get_device_id_str(uint8_t *str)
{
	sprintf((char*)str,"%04X%04X%04X",g_chipid_t.id[0],g_chipid_t.id[1],g_chipid_t.id[2]);
}

void start_get_device_id(uint32_t *id)
{
	id[0] = g_chipid_t.id[0];
	id[1] = g_chipid_t.id[1];
	id[2] = g_chipid_t.id[2];
}

/*
*********************************************************************************************************
*	函 数 名: app_task
*	功能说明: 主任务
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void app_task(void *pvParameters)
{
	while(1){

		vTaskDelay(500);
	}
//	app_task_function();
}

/*
*********************************************************************************************************
*	函 数 名: eth_task
*	功能说明: 网口检测任务:一直对网口进行轮询
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void eth_task(void *pvParameters)
{
	while(1){

		vTaskDelay(500);
	}
//	eth_network_line_status_detection_function();
}

/*
*********************************************************************************************************
*	函 数 名: det_task
*	功能说明: 检测任务
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_task(void *pvParameters)
{
	while(1){

		vTaskDelay(500);
	}
//	det_task_function();
}

/*
*********************************************************************************************************
*	函 数 名: gsm_task
*	功能说明: 无线通信任务
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void gsm_task(void *pvParameters)
{
	pvParameters = pvParameters;

	gsm_task_function();
}

/*
*********************************************************************************************************
*	函 数 名: print_task
*	功能说明: 打印任务
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void print_task(void *pvParameters)
{
	while(1){

		vTaskDelay(500);
	}	
//	print_task_function();
}	

/*
*********************************************************************************************************
*	函 数 名: list_task
*	功能说明: 任务堆栈
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void list_task(void *pvParameters)
{
	while(1){

		vTaskDelay(500);
	}	
}


