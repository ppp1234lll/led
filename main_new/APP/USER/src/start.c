
#include "appconfig.h"

ChipID_t g_chipid_t;

/*
*********************************************************************************************************
*	函 数 名:  start_system_init_function
*	功能说明:  初始化所有的硬件设备
*	形    参:  无
*	返 回 值:  无
*********************************************************************************************************
*/
void start_system_init_function(void)
{
	cJSON_Hooks hook;                // 初始化JSON 

	iwdg_init(IWDG_PRESCALER_64, 1000);// 初始化看门狗(硬件、软件2s)
	start_get_device_id_function();    // 获取本机ID
	my_mem_init(SRAMIN);              // 内存初始化
	
	hook.malloc_fn = mymalloc_sramin;// 内存分配
	hook.free_fn   = myfree_sramin;  // 内存释放
	cJSON_InitHooks(&hook);          // 初始化自定义的内存分配和释放函数
	
	bsp_InitLed();                   // LED初始化（已测试）
	bsp_InitRelay();				         // 继电器初始化（未测试）	
	bsp_InitKey();				         // 按键初始化	
	bsp_InitRTC();								   // RTC初始化 (已测试)
	bsp_InitUsart1(115200);
	bsp_InitUsart2(115200);
  bsp_InitUsart3(115200);
	bsp_InitUsart4(115200);
	bsp_InitUart5(115200);
	
	
	
	uart5_test();
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

#if 0
/* APP线程 */
#define APP_TASK_PRIO		6
#define APP_STK_SIZE		512
__align(8) CCMRAM static OS_STK START_TASK_STK[APP_STK_SIZE];
void app_task(void *argument);

/* 网络线程 */
#define ETH_TASK_PRIO		9
#define ETH_STK_SIZE		320
__align(8) CCMRAM static OS_STK ETH_TASK_STK[ETH_STK_SIZE];
void eth_task(void *argument);

/* 检测线程 */
#define DET_TASK_PRIO		7
#define DET_STK_SIZE		320
__align(8) CCMRAM static OS_STK DET_TASK_STK[DET_STK_SIZE];
void det_task(void *argument);

/* 无线线程 */
#define GSM_TASK_PRIO		8
#define GSM_STK_SIZE		320
CCMRAM OS_STK GSM_TASK_STK[GSM_STK_SIZE];
void gsm_task(void *argument);

/* 打印线程 */
#define PRINT_TASK_PRIO		29   /* 任务优先级 */
#define PRINT_STK_SIZE		256  /* 任务堆栈大小 */
CCMRAM OS_STK 	PRINT_TASK_STK[PRINT_STK_SIZE]; /* 任务堆栈 */
void print_task(void *argument); /* 任务函数 */

/* 查看任务堆栈 */
#define STORAGESTACK_PRIO		30
#define STORAGESTACK_STK_SIZE 		128
OS_STK STORAGESTACK_STK[STORAGESTACK_STK_SIZE];
void storagestack_task(void *p_arg);

/************************************************************
*
* Function name	: start_creat_task_function
* Description	: 创建任务
* Parameter		: 
* Return		: 
*	
************************************************************/
void start_creat_task_function(void)
{

										
}

/*
*********************************************************************************************************
*	函 数 名: app_task
*	功能说明: 主任务
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void app_task(void *argument)
{
	OSStatInit();	  	// 初始化统计任务
	app_task_function();
}

/*
*********************************************************************************************************
*	函 数 名: eth_task
*	功能说明: 网口检测任务:一直对网口进行轮询
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void eth_task(void *argument)
{
	eth_network_line_status_detection_function();
}

/*
*********************************************************************************************************
*	函 数 名: det_task
*	功能说明: 检测任务
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_task(void *argument)
{
	det_task_function();
}

/*
*********************************************************************************************************
*	函 数 名: gsm_task
*	功能说明: 无线通信任务
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void gsm_task(void *argument)
{
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
void print_task(void *argument)
{
	print_task_function();
}	

/*
*********************************************************************************************************
*	函 数 名: storagestack_task
*	功能说明: 任务堆栈
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void storagestack_task(void *p_arg)
{

}
#endif


