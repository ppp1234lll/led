#include "appconfig.h"
#include "./TASK/inc/app.h"

/* 发送状态 */
#define SEND_STATUS_NO      (0) // 当前没有发送
#define SEND_STATUS_SENGING (1) // 当前正在发送
#define SEND_STATUS_RESULT  (2) // 当前发送有了结果

#define SERVER_LINK_TIME    (30000) // 服务器连接时间  5min = 30000ms/10ms

typedef struct
{
	struct
	{
		send_result_e send_result; // 发送结果
		uint32_t 	  heart_time;    // 心跳计时
		uint32_t	  report_time;	 // 上报时间
		uint16_t 	  send_time;	   // 发送计时
		uint8_t  	  send_mode;	   // send mode:0-LWIP 1-GPRS
		uint8_t  	  repeat;		     // 重复次数: 针对发送
		uint8_t  	  send_status;   // 发送状态: 0-当前没有发送 1-当前有发送 2-发送有结果
		uint8_t		  send_cmd;  	   // 发送内容，直接使用对应命令字
		uint8_t 	  return_cmd; 	 // 回复标志
		uint8_t 	  return_error;	 // 回复内容
	} com;
	struct
	{
		uint8_t save_device_param;   	// 存储设备参数
		uint8_t save_local_network;  	// 保存本地网络参数
		uint8_t save_remote_network; 	// 保存远端网络参数
		uint8_t save_update_addr;    	// 保存更新地址
		uint8_t com_parameter;			 	// 通信相关参数
		uint8_t save_threshold;       // 阈值
		uint8_t save_reset;		     		// 恢复出厂化
	} save_flag;
	struct
	{
		uint8_t report_normally;	   // 正常上报
		uint8_t query_configuration; // 查询配置上传
		uint8_t heart_pack;			     // 心跳包
		uint8_t version;			       // 版本信息
		uint8_t config_return;		   // 配置回复
	} com_flag;
	struct
	{
		uint8_t adapter_num;		  // 适配器 - 编号
	} sys;
	struct
	{
		uint8_t lwip_reset;         // 网络重启标志
		uint8_t adapter_reset;		  // 适配器-重启标志
		uint8_t relay_reset[8];		   
	} sys_flag;
	struct
	{
		uint8_t status;         // 更新结果
	} update;
}sys_operate_t;


/* 参数定义 */
__attribute__((section (".RAM_D1")))  sys_backups_t sg_backups_t   	= {0}; // 备份信息 20231022
__attribute__((section (".RAM_D1")))  sys_operate_t sg_sysoperate_t; // 系统操作参数：包括通信、存储、计时
__attribute__((section (".RAM_D1")))  sys_param_t   sg_sysparam_t   = {0}; // 系统参数：本地、远端、设备、上报相关参数
__attribute__((section (".RAM_D1")))  com_param_t   sg_comparam_t	 	  = {
	90000,
	60000,
	60000,
	60000,
	200,    // 网络延时时间  20220308
	120,    // ONVIF搜索时间  20230811
}; 				// 通信参数：心跳、上报、ping间隔时间
__attribute__((section (".RAM_D1")))  uint8_t     sg_send_buff[2048] = {0}; // 发送缓存区
__attribute__((section (".RAM_D1")))  uint16_t    sg_send_size =  0;	 // 发送数据长度
__attribute__((section (".RAM_D1")))  rtc_time_t  sg_rtctime_t = {0}; // rtc采集间隔时间

/*
*********************************************************************************************************
*	函 数 名: app_task_function
*	功能说明: 应用程序主任务 
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void app_task_function(void)
{
	uint8_t get_time_cnt = 0;
	
	/* 开启系统指示灯 */
	led_control_function(LD_STATE,LD_FLICKER);

	for(;;)
	{
		app_task_save_function();   				// 存储相关任务
		com_deal_main_function(); 					// 处理接收数据
		app_com_send_function();						// 通信发送
		app_server_link_status_function();
		app_sys_ctrl_relay_reload();
		
		get_time_cnt++;
		if(get_time_cnt>100)
		{
			get_time_cnt = 0;
			RTC_Get_Time(&sg_rtctime_t);		/* 时间获取 */
			
			/* 内存利用率 */
			sg_sysparam_t.mem = my_mem_perused(SRAMIN);
		}
    iwdg_feed();	
		vTaskDelay(10);
	}
}

/*
*********************************************************************************************************
*	函 数 名: app_set_com_send_flag_function
*	功能说明: 设置发送函数 
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void app_set_com_send_flag_function(uint8_t cmd, uint8_t data)
{
	switch(cmd)
	{
		case CR_QUERY_CONFIG:
			sg_sysoperate_t.com_flag.query_configuration = 1;
			break;
		case CR_QUERY_INFO:
			sg_sysoperate_t.com_flag.report_normally = 1;
			break;
		case CR_QUERY_SOFTWARE_VERSION:
			sg_sysoperate_t.com_flag.version = 1;
			break;

	}
}

/************************************************************
*
* Function name	: app_set_reply_parameters_function
* Description	: 设置回复参数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_reply_parameters_function(uint8_t cmd, uint8_t error)
{
	sg_sysoperate_t.com.return_cmd 		     = cmd;
	sg_sysoperate_t.com.return_error       = error;
	sg_sysoperate_t.com_flag.config_return = 1;
}

/************************************************************
*
* Function name	: app_report_information_immediately
* Description	: 立即上报信息 - 主要针对于出现异常数据时的上报
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_report_information_immediately(void) 
{
	if ( eth_get_tcp_status() != 2 && gsm_get_network_connect_status_function() == 0)
	{
		/* 网络异常，存储本次数据到本地 */
		sg_sysoperate_t.com_flag.report_normally = 1;
		return;
	}
	/* 发送正常上报数据 */
	memset(sg_send_buff,0,sizeof(sg_send_buff));
	com_report_normally_function(sg_send_buff,&sg_send_size,CR_QUERY_INFO);
	/* 设置发送参数 */
	sg_sysoperate_t.com.send_cmd = CR_QUERY_INFO;
	sg_sysoperate_t.com.repeat   = 0; 		// 重启一次正常上报计时   
	
	/* 数据发送 */
	if(sg_sysoperate_t.com.send_cmd != 0)
	{
		app_send_data_task_function();	
		sg_sysoperate_t.com.send_status = SEND_STATUS_SENGING;
		
		return ;								 // 如果检测到需要重复发送，则不进行下一次发送
	}
}

/************************************************************
*
* Function name	: app_deal_com_flag_function
* Description	: 用来处理通信发送标志
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_deal_com_flag_function(void)
{
	 
	/* 数据发送 */
	if(sg_sysoperate_t.com.send_cmd != 0)
	{
		app_send_data_task_function();
		sg_sysoperate_t.com.send_status = SEND_STATUS_SENGING;		/* 转换为正在发送 */
		return ;								 // 如果检测到需要重复发送，则不进行下一次发送
	}
	
	if(sg_sysoperate_t.com_flag.heart_pack == 1)	/* 检测心跳发送 */
	{
		sg_sysoperate_t.com_flag.heart_pack = 0;
		com_heart_pack_function(sg_send_buff,&sg_send_size);		/* 发送心跳 */
		sg_sysoperate_t.com.send_cmd    = COM_HEART_UPDATA;		/* 设置发送参数 */
		sg_sysoperate_t.com.heart_time  = 0; 	 // 重启一次心跳计时
	}
	
	/* 立即上报设备状态 */
	if(sg_sysoperate_t.com_flag.report_normally == 1)
	{
		sg_sysoperate_t.com_flag.report_normally = 0;
		memset(sg_send_buff,0,sizeof(sg_send_buff));
		com_report_normally_function(sg_send_buff,&sg_send_size,CR_QUERY_INFO);		/* 发送正常上报数据 */
		sg_sysoperate_t.com.send_cmd = CR_QUERY_INFO;
		sg_sysoperate_t.com.repeat   = 0; 		// 重启一次正常上报计时   
	}
	
	/* 直接发送，不需要检测回传 */
	/* 查询配置当前参数设置 */
	if(sg_sysoperate_t.com_flag.query_configuration == 1)
	{
		sg_sysoperate_t.com_flag.query_configuration = 0;
		memset(sg_send_buff,0,sizeof(sg_send_buff));
		com_query_configuration_function(sg_send_buff,&sg_send_size);
		app_send_data_task_function();
	}
	
	/* 上报软件版本信息 */
	if(sg_sysoperate_t.com_flag.version == 1)
	{
		sg_sysoperate_t.com_flag.version = 0;
		com_version_information(sg_send_buff,&sg_send_size);
		app_send_data_task_function();
	}
		
	/* 回传信号 */
	if(sg_sysoperate_t.com_flag.config_return == 1)
	{
		sg_sysoperate_t.com_flag.config_return = 0;
		com_ack_function(sg_send_buff,&sg_send_size,sg_sysoperate_t.com.return_cmd,sg_sysoperate_t.com.return_error);
		app_send_data_task_function();
	}
	
}

/************************************************************
*
* Function name	: app_get_com_send_status_function
* Description	: 获取当前通信状态
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t app_get_com_send_status_function(void)
{
	return sg_sysoperate_t.com.send_status;
}

/************************************************************
*
* Function name	: app_deal_com_send_wait_function
* Description	: 发送等待处理任务
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_deal_com_send_wait_function(void)
{
	switch(sg_sysoperate_t.com.send_result)
	{
		case SR_TIMEOUT:											// 发送超时							
			/* 本次发送超时 */
			sg_sysoperate_t.com.repeat++;
			if(sg_sysoperate_t.com.repeat >= COM_SEND_MAX_NUM)
			{
				/* 发送响应超时 */
				sg_sysoperate_t.com.repeat = 0;
				/* 发送超时，服务器无响应或者网络已断开 */
				eth_set_tcp_connect_reset();						// 重启TCP连接
				gsm_set_network_reset_function();					// 重启GRPS连接
				
				/* 数据清空 */
				sg_sysoperate_t.com.send_status = SEND_STATUS_NO; 	// 进行下一次发送
				sg_sysoperate_t.com.send_cmd = 0;
				
				sg_sysoperate_t.com.heart_time  = 0; 				// 重启一次心跳计时
				sg_sysoperate_t.com.repeat   = 0; 					// 重启一次正常上报计时   
			}
			else
			{
				/* 重新发送 */
				sg_sysoperate_t.com.send_status = SEND_STATUS_NO;
			}
			break;
		case SR_OK:													// 发送成功
			/* 发送成功 */
			sg_sysoperate_t.com.send_status = SEND_STATUS_NO; 		// 进行下一次发送
			sg_sysoperate_t.com.send_cmd = 0;
			/* 清空数据 */
			memset(sg_send_buff,0,sizeof(sg_send_buff));
			sg_send_size = 0;
			break;
		case SR_ERROR:												// 响应错误
		case SR_SEND_ERROR:											// 发送错误
			sg_sysoperate_t.com.repeat++;
			if(sg_sysoperate_t.com.repeat >= COM_SEND_MAX_NUM)
			{
				/* 发送次数到达上限 */
				sg_sysoperate_t.com.repeat = 0;
				sg_sysoperate_t.com.send_status = SEND_STATUS_NO; 	// 进行下一次发送
				sg_sysoperate_t.com.send_cmd = 0;
				/* 清空数据 */
				memset(sg_send_buff,0,sizeof(sg_send_buff));
				sg_send_size = 0;
			}
			else
			{
				/* 重新发送 */
				sg_sysoperate_t.com.send_status = SEND_STATUS_NO; 	// 进行下一次发送
			}
			break;
		default:
			sg_sysoperate_t.com.send_cmd = 0;
			sg_sysoperate_t.com.send_status = SEND_STATUS_NO; 		// 进行下一次发送
			/* 清空数据 */
			memset(sg_send_buff,0,sizeof(sg_send_buff));
			sg_send_size = 0;
			break;
	}	
	/* 结果清空 */
	sg_sysoperate_t.com.send_result = SR_WAIT;
}

/************************************************************
*
* Function name	: app_com_send_function
* Description	: 通信发送函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_com_send_function(void)
{
	/* 检测网络状态 */
	if(gsm_get_network_connect_status_function() == 0 && eth_get_tcp_status() != 2)
	{
		return ;
	}
	if(sg_sysoperate_t.com.send_mode == 0 && eth_get_tcp_status() != 2)
	{
		/* 网络错误，准备重新连接，清除本次发送参数 */
		memset(&sg_sysoperate_t.com_flag,0,sizeof(sg_sysoperate_t.com_flag));
		sg_sysoperate_t.com.send_cmd = 0;
		/* 重启网络 */
		eth_set_tcp_connect_reset();
		gsm_set_network_reset_function(); // 重启GPRS
	}
	if(sg_sysoperate_t.com.send_mode == 1 && gsm_get_network_connect_status_function() == 0)
	{
		/* 网络错误，准备重新连接，清除本次发送参数 */
		memset(&sg_sysoperate_t.com_flag,0,sizeof(sg_sysoperate_t.com_flag));
		sg_sysoperate_t.com.send_cmd = 0;
		/* 重启网络 */
		eth_set_tcp_connect_reset();
		gsm_set_network_reset_function(); // 重启GPRS
	}
	
	/* 检测是否能发送数据 */
	if(sg_sysoperate_t.com.send_status == SEND_STATUS_NO)
	{
		app_deal_com_flag_function();
	}
	/* 检测到发送有结果了 */
	else if(sg_sysoperate_t.com.send_status == SEND_STATUS_RESULT)
	{
		app_deal_com_send_wait_function();
	}
	/* 正在发送中 */
	else
	{
		
	}
}

/************************************************************
*
* Function name	: app_set_send_result_function
* Description	: 设置发送结果
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_send_result_function(send_result_e data)
{
	sg_sysoperate_t.com.send_result = data;
	sg_sysoperate_t.com.send_status = SEND_STATUS_RESULT;
}
	
/************************************************************
*
* Function name	: app_com_time_function
* Description	: 通信计时函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_com_time_function(void)
{
	/* 心跳计时 */
	sg_sysoperate_t.com.heart_time++;
	if(sg_sysoperate_t.com.heart_time > sg_comparam_t.heart)
	{
		sg_sysoperate_t.com.heart_time = 0;
		/* 发送一次心跳 */
		sg_sysoperate_t.com_flag.heart_pack = 1;
	}
	
	/* 正常上报 */
	sg_sysoperate_t.com.report_time++;
	if(sg_sysoperate_t.com.report_time > (sg_comparam_t.report))
	{
		sg_sysoperate_t.com.report_time = 0;
		/* 进行一次上报 */
		sg_sysoperate_t.com_flag.report_normally = 1;
	}
	
	/* 发送计时 */
	if(sg_sysoperate_t.com.send_status == 1)
	{
		sg_sysoperate_t.com.send_time++;
		if(sg_sysoperate_t.com.send_time > COM_SEND_MAX_TIME)
		{
			sg_sysoperate_t.com.send_time = 0;
			
			sg_sysoperate_t.com.send_status = SEND_STATUS_RESULT;
			sg_sysoperate_t.com.send_result = SR_TIMEOUT;
		}
	}
	else
	{
		sg_sysoperate_t.com.send_time = 0;
	}
}


/************************************************************
*
* Function name	: app_set_sys_opeare_function
* Description	: 设置操作任务 - 立即回发
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_sys_opeare_function(uint8_t cmd, uint8_t data)
{
	int8_t ret = 0;
	int8_t cnt = 0;
	
	switch(cmd)
	{
		case CR_SINGLE_CAMERA_CONTROL:
			if(data>=1 && data <= 8)
			{
				if(sg_sysoperate_t.sys_flag.adapter_reset == 0)
				{
					sg_sysoperate_t.sys_flag.relay_reset[data] = 1;
//					sg_sysoperate_t.sys_flag.adapter_reset = 1;
//					sg_sysoperate_t.sys.adapter_num = data;
					app_set_reply_parameters_function(CR_SINGLE_CAMERA_CONTROL,0x01);
				}
				else
				{
					app_set_reply_parameters_function(CR_SINGLE_CAMERA_CONTROL,0x77);
				}
			}
			else
			{
				app_set_reply_parameters_function(CR_SINGLE_CAMERA_CONTROL,0x74);
			}
			break;
			
		case CR_POWER_RESETART:
			sg_sysoperate_t.com.return_error = 1;
			/* 数据回传 */
			com_ack_function(sg_send_buff,&sg_send_size,\
							 sg_sysoperate_t.com.return_cmd,\
							 sg_sysoperate_t.com.return_error);
			cnt = 0;
REPEAT1:
			/* 立即发送 */
			app_send_data_task_function();	

			if(ret < 0)
			{
				cnt++;
				if(cnt<=3)
				{
					vTaskDelay(100);
					goto REPEAT1;
				}
			}
				
			lfs_unmount(&g_lfs_t);
			vTaskDelay(100);
			/* 重启设备 */
			app_system_softreset(1000);
			break;

			case CR_GPRS_NETWORK_V_RESET:
			sg_sysoperate_t.com.return_error = 1;
			/* 数据回传 */
			com_ack_function(sg_send_buff,&sg_send_size,sg_sysoperate_t.com.return_cmd,sg_sysoperate_t.com.return_error);
			cnt = 0;
REPEAT4:
			/* 立即发送 */
			app_send_data_task_function();	
			if(ret < 0)
			{
				cnt++;
				if(cnt<=3)
				{
					vTaskDelay(100);
					goto REPEAT4;
				}
			}			
			vTaskDelay(100);
			/* 重启GPRS */
			gprs_v_reset_function();
			gsm_set_module_reset_function();
			break;
	}
}

/************************************************************
*
* Function name	: app_sys_operate_timer_function
* Description	: 操作命令的时间处理函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_sys_operate_timer_function(void)
{
	static uint16_t time = 0;
 	
	/* 重启指定适配器 */
	if(sg_sysoperate_t.sys_flag.adapter_reset == 1)
	{
		/* 关闭指的适配器 */
		if(sg_sysoperate_t.sys.adapter_num <= RELAY_NUM)
		{
			switch(sg_sysoperate_t.sys.adapter_num)
			{
				case 1:	relay_control(RELAY_1,RELAY_OFF);	break;
				case 2:	relay_control(RELAY_2,RELAY_OFF);	break;
				case 3:	relay_control(RELAY_3,RELAY_OFF);	break;
				case 4:	relay_control(RELAY_4,RELAY_OFF);	break;
				default:	break;

			}
			sg_sysoperate_t.sys_flag.adapter_reset = 2;
			time = 10*1000; // 15s重启
		}
		else
		{
			sg_sysoperate_t.sys_flag.adapter_reset = 0;
		}
	}	
	else if(sg_sysoperate_t.sys_flag.adapter_reset == 2)
	{
		time--;
		if(time == 0)
		{
			switch(sg_sysoperate_t.sys.adapter_num)
			{
				case 1:	relay_control(RELAY_1,RELAY_ON);	break;
				case 2:	relay_control(RELAY_2,RELAY_ON);	break;
				case 3:	relay_control(RELAY_3,RELAY_ON);	break;
				case 4:	relay_control(RELAY_4,RELAY_ON);	break;
				default:
					break;
			}
			sg_sysoperate_t.sys_flag.adapter_reset = 0;
		}
	}
}

/************************************************************
*
* Function name	: app_sys_ctrl_relay_reload
* Description	: 继电器重启
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_sys_ctrl_relay_reload(void)
{
	static uint16_t relay_time[8] = {0};
		
	for(uint8_t i=0;i<RELAY_NUM;i++)
	{
		if(sg_sysoperate_t.sys_flag.relay_reset[i] == 1)
		{
			relay_control((RELAY_DEV)i,RELAY_OFF);
			relay_time[i] = 10*100;
			sg_sysoperate_t.sys_flag.relay_reset[i] = 2;
		}
		else if(sg_sysoperate_t.sys_flag.relay_reset[i] == 2)
		{		
			relay_time[i]--;
			if(relay_time[i] == 0)
			{		
				relay_control((RELAY_DEV)i,RELAY_ON);
				sg_sysoperate_t.sys_flag.relay_reset[i]	= 0;			
			}
		}	
	}
}

/************************************************************
*
* Function name	: app_set_relay_switch
* Description	: 外设开关控制
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_relay_switch(uint8_t cmd, uint8_t id,uint8_t status)
{
	switch (cmd) 
	{
		case CTRL_RELAY_POWER:
			if (status == 1)
			{
				relay_control((RELAY_DEV)id,RELAY_ON);
			}
			else if(status == 2)
			{
				relay_control((RELAY_DEV)id,RELAY_OFF);
			}
			break;
		
		default:
			break;
	}
	app_set_reply_parameters_function(cmd,0x01);
}
/***********************************************************************************
					参数的配置与获取
***********************************************************************************/

/************************************************************
*
* Function name	: app_get_storage_param_function
* Description	: 用于获取存储的参数的函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_get_storage_param_function(void)
{
	save_read_local_network(&sg_sysparam_t.local);
	save_read_remote_ip_function(&sg_sysparam_t.remote);
	save_read_device_paramter_function(&sg_sysparam_t.device);
	save_read_com_param_function(&sg_comparam_t);
	save_read_threshold_parameter(&sg_sysparam_t.threshold); // 20230720
	save_read_http_ota_function(&sg_sysparam_t.ota);
	save_read_backups_function(&sg_backups_t); // 20231022
}

/************************************************************
*
* Function name	: app_task_save_function
* Description	: 用于存储本机的设置参数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_task_save_function(void)
{
	if(sg_sysoperate_t.save_flag.save_device_param == 1)
	{
		sg_sysoperate_t.save_flag.save_device_param = 0;
		save_storage_device_parameter_function(&sg_sysparam_t.device);
	}	
	
	if(sg_sysoperate_t.save_flag.save_local_network == 1)
	{
		sg_sysoperate_t.save_flag.save_local_network = 0;
		save_stroage_local_network(&sg_sysparam_t.local);
	}
	
	if(sg_sysoperate_t.save_flag.save_remote_network == 1)
	{
		sg_sysoperate_t.save_flag.save_remote_network = 0;
		save_stroage_remote_ip_function(&sg_sysparam_t.remote);

		app_system_softreset(1000);
	}

	/* 存储通信相关参数 */	
	if(sg_sysoperate_t.save_flag.com_parameter == 1)
	{
		sg_sysoperate_t.save_flag.com_parameter = 0;
		save_stroage_com_param_function(&sg_comparam_t);
	}
	
	if(sg_sysoperate_t.save_flag.save_threshold == 1)
	{
		sg_sysoperate_t.save_flag.save_threshold = 0;
		save_stroage_threshold_parameter(&sg_sysparam_t.threshold);
	}
	
	/* 恢复出厂化：产品序列号不变 */
	if(sg_sysoperate_t.save_flag.save_reset == 1)
	{
		sg_sysoperate_t.save_flag.save_reset = 0;
		save_clear_file_function(0);
		app_get_storage_param_function();
		
		eth_set_network_reset(); 			 // 重启网络
		gsm_set_module_reset_function(); 	// 重启GPRS
	}
}

/************************************************************
*
* Function name	: app_set_save_infor_function
* Description	: 设置存储标志位
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_save_infor_function(uint8_t mode)
{
	switch(mode)
	{
		case SAVE_LOCAL_NETWORK:
			sg_sysoperate_t.save_flag.save_local_network = 1;
			break;
		case SAVE_REMOTE_IP:
			sg_sysoperate_t.save_flag.save_remote_network = 1;
			break;
		case SAVE_DEVICE_PARAM:
			sg_sysoperate_t.save_flag.save_device_param = 1;
			break;
		case SAVE_COM_PARAMETER:
			sg_sysoperate_t.save_flag.com_parameter = 1;
			break;
		case SAVE_UPDATE:
			sg_sysoperate_t.save_flag.save_update_addr = 1;
			break;
		case SAVE_THRESHOLD:
			sg_sysoperate_t.save_flag.save_threshold = 1;
			break;
		default:
			break;
	}
}

/************************************************************
*
* Function name	: app_get_local_network_function
* Description	: 获取本机网络信息
* Parameter		: 
* Return		: 
*	
************************************************************/
void *app_get_local_network_function(void)
{
	return (&sg_sysparam_t.local);
}

/************************************************************
*
* Function name	: app_set_local_network_function_two
* Description	: 存储部分网络参数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_local_network_function_two(struct local_ip_t param)
{
	memcpy(sg_sysparam_t.local.ip,param.ip,4);
	memcpy(sg_sysparam_t.local.gateway,param.gateway,4);
	memcpy(sg_sysparam_t.local.netmask,param.netmask,4);
	memcpy(sg_sysparam_t.local.dns,param.dns,4);
	memcpy(sg_sysparam_t.local.mac,param.mac,6);
	memcpy(sg_sysparam_t.local.ping_ip,param.ping_ip,4);
	memcpy(sg_sysparam_t.local.ping_sub_ip,param.ping_sub_ip,4);
	
	memcpy(sg_sysparam_t.local.multicast_ip,param.multicast_ip,4);
	sg_sysparam_t.local.multicast_port = param.multicast_port;

	cpuflash_write_save(DEVICE_FLASH_STORE,DEVICE_MAC_ADDR,(uint8_t *)&sg_sysparam_t.local.mac,6);

	app_set_save_infor_function(SAVE_LOCAL_NETWORK);
}


/************************************************************
*
* Function name	: app_set_transfer_mode_function
* Description	: 存储传输模式信息
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_transfer_mode_function(uint8_t mode) 
{
	switch(mode) {
		case 0:
			sg_sysparam_t.local.server_mode = 1;
			break;
		case 1:
			sg_sysparam_t.local.server_mode = 2;
			break;
		case 2:
			sg_sysparam_t.local.server_mode = 4;
			break;
	}
	/* 保存 */
	app_set_save_infor_function(SAVE_LOCAL_NETWORK);
}

/************************************************************
*
* Function name	: app_get_remote_network_function
* Description	: 获取远端网络信息
* Parameter		: 
* Return		: 
*	
************************************************************/
void *app_get_remote_network_function(void)
{
	return (&sg_sysparam_t.remote);
}
/************************************************************
*
* Function name	: app_get_backups_function
* Description	: 获取备份信息
* Parameter		: 
* Return		: 
*	  20231022
************************************************************/
void *app_get_backups_function(void)
{
	return (&sg_backups_t.remote);
}
/************************************************************
*
* Function name	: app_set_remote_network_function
* Description	: 存储远端网络信息
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_remote_network_function(struct remote_ip param)
{
	app_save_backups_remote_param_function();  // 备份服务器信息
	memcpy(&sg_sysparam_t.remote,&param,sizeof(struct remote_ip));
	app_set_save_infor_function(SAVE_REMOTE_IP);
}

/************************************************************
*
* Function name	: app_set_reset_function
* Description	: 恢复出厂化
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_reset_function(void)
{
	sg_sysoperate_t.save_flag.save_reset = 1;
}

/************************************************************
*
* Function name	: app_set_fan_humi_param_function
* Description	: 设置风扇湿度启动参数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_fan_humi_param_function(uint8_t *data)
{
	sg_sysparam_t.threshold.humi_high = data[0];
	sg_sysparam_t.threshold.humi_low  = data[1];
	app_set_save_infor_function(SAVE_THRESHOLD);	/* 存储 */
}

/************************************************************
*
* Function name	: app_set_fan_param_function
* Description	: 配置风扇温度参数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_fan_param_function(int8_t *data)
{
	sg_sysparam_t.threshold.temp_high = data[0];
	sg_sysparam_t.threshold.temp_low = data[1];
	app_set_save_infor_function(SAVE_THRESHOLD);	/* 存储 */
}

/************************************************************
*
* Function name	: app_set_threshold_param_function
* Description	: 配置阈值
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_vol_current_param(uint16_t *data)
{
	sg_sysparam_t.threshold.volt_max = data[0];
	sg_sysparam_t.threshold.volt_min = data[1];
	sg_sysparam_t.threshold.current  = data[2];
	sg_sysparam_t.threshold.angle    = data[3];
	sg_sysparam_t.threshold.miu      = data[4];
	/* 保存 */
	app_set_save_infor_function(SAVE_THRESHOLD);	/* 存储 */
}
/************************************************************
*
* Function name	: app_get_device_param_function
* Description	: 获取设备参数
* Parameter		: 
* Return		: 
*	
************************************************************/
void *app_get_device_param_function(void)
{
	return (&sg_sysparam_t.device);
}

/************************************************************
*
* Function name	: app_match_password_function
* Description	: 密码比较函数
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t app_match_password_function(char *password)
{
	if(strcmp(password , (char*)sg_sysparam_t.device.password)==0)
	{
		return 0;
	}
	return -1;
}
/************************************************************
*
* Function name	: app_match_set_code_function
* Description	: 确认是否需要需要修改默认密码
* Parameter		: 
* Return		: 1-需要修改
*	
************************************************************/
int8_t app_match_set_code_function(void)
{
	return sg_sysparam_t.device.default_password;
}

/************************************************************
*
* Function name	: app_set_device_param_function
* Description	: 设置设备参数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_device_param_function(struct device_param param)
{
	memset((uint8_t*)&sg_sysparam_t.device,0,sizeof(struct device_param));
	memcpy((uint8_t*)&sg_sysparam_t.device,&param,sizeof(struct device_param));
	
	cpuflash_write_save(DEVICE_FLASH_STORE,DEVICE_ID_ADDR,(uint8_t*)param.id.c,4);
	app_set_save_infor_function(SAVE_DEVICE_PARAM);
}

/************************************************************
*
* Function name	: app_set_code_function
* Description	: 设置密码
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_code_function(struct device_param param)
{
	sg_sysparam_t.device.default_password = 0;
	memcpy(sg_sysparam_t.device.password,param.password,sizeof(param.password));
	app_set_save_infor_function(SAVE_DEVICE_PARAM);
}

/************************************************************
*
* Function name	: app_get_com_heart_time
* Description	: 获取心跳参数
* Parameter		: 
* Return		: 
*	
************************************************************/
uint16_t app_get_com_heart_time(void)
{
	return sg_comparam_t.heart;
}

/************************************************************
*
* Function name	: app_set_next_report_time
* Description	: 设置上报间隔时间
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_next_report_time(uint16_t time)
{
	/* 将时间单位转换为ms */
	sg_comparam_t.report = time*1000;
	app_set_save_infor_function(SAVE_COM_PARAMETER);
}

/************************************************************
*
* Function name	: app_set_next_report_time_other
* Description	: 设置上报间隔时间-可选择
* Parameter		: 
*	@time		: 上报时间
*	@sel		: 设置编号
* Return		: 
*	
************************************************************/
void app_set_next_report_time_other(uint16_t time,uint8_t sel)
{
	if(sel == 1) {
		sg_comparam_t.report = time*1000;
		app_set_save_infor_function(SAVE_COM_PARAMETER);
	}
}

/************************************************************
*
* Function name	: app_set_next_ping_time
* Description	: 设置下一次ping的时间
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_next_ping_time(uint16_t time, uint8_t time_dev)
{
	/* 将时间单位转换为ms */
	sg_comparam_t.ping = time*1000;
	sg_comparam_t.dev_ping = time_dev*1000;
	
	app_set_save_infor_function(SAVE_COM_PARAMETER);
}
/************************************************************
*
* Function name	: app_set_network_delay_time
* Description	: // 设置网络延时时间  20220308
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_network_delay_time(uint8_t time_dev)
{
	/* 将时间单位转换为ms */
	sg_comparam_t.network_time = time_dev;
	app_set_save_infor_function(SAVE_COM_PARAMETER);
}

/************************************************************
*
* Function name	: app_get_current_time
* Description	: 获取当前时间
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_get_current_time(char *time)
{
	
	sprintf(time,"%04d/%02d/%02d %02d:%02d:%02d",sg_rtctime_t.year,\
												 sg_rtctime_t.month,\
												 sg_rtctime_t.data,\
												 sg_rtctime_t.hour,\
												 sg_rtctime_t.min,\
												 sg_rtctime_t.sec);
}

/************************************************************
*
* Function name	: app_get_report_current_time
* Description	: 获取当前上报的实时时间
* Parameter		: 
*	@mode		: 0：秒 1：毫秒
* Return		: 
*	
************************************************************/
uint8_t* app_get_report_current_time(uint8_t mode)
{
	static uint8_t times[20];
	
	memset(times,0,sizeof(times));
	if(mode == 0) {
		sprintf((char*)times,"%04d%02d%02d%02d%02d%02d",sg_rtctime_t.year,
													sg_rtctime_t.month,
													sg_rtctime_t.data,
													sg_rtctime_t.hour,
													sg_rtctime_t.min,
													sg_rtctime_t.sec);
	} 
	else if(mode == 1) 
	{
		sprintf((char*)times,"%04d%02d%02d%02d%02d%02d",sg_rtctime_t.year,
													sg_rtctime_t.month,
													sg_rtctime_t.data,
													sg_rtctime_t.hour,
													sg_rtctime_t.min,
													sg_rtctime_t.sec);
		times[14] = sg_rtctime_t.hour%10+'0';
		times[15] = sg_rtctime_t.min%10+'0';
		times[16] = sg_rtctime_t.sec%10+'0';
	}
	else if(mode == 2) 
	{
		sprintf((char*)times,"%04d%02d%02d",sg_rtctime_t.year,
													sg_rtctime_t.month,
													sg_rtctime_t.data);
	} 
	return times;
	
}

/************************************************************
*
* Function name	: app_set_current_time
* Description	: 设置时间
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_current_time(int *time,uint8_t conv)
{
	rtc_time_t time_t;
	
	time_t.year  = time[0];
	time_t.month = time[1];
	time_t.data  = time[2];
	time_t.hour  = time[3];
	time_t.min   = time[4];
	time_t.sec   = time[5];
	
	if(conv) // 是否需要转换
	{
		rtc_time_t conv_time;
		local_to_utc_time(&conv_time,8,time_t);
		RTC_set_Time(conv_time);
	}
	else
	RTC_set_Time(time_t);
}

/************************************************************
*
* Function name	: app_get_next_ping_time
* Description	: 获取ping的间隔时间
* Parameter		: 
* Return		: 
*	
************************************************************/
uint32_t app_get_next_ping_time(void)
{
	return sg_comparam_t.ping;
}

/************************************************************
*
* Function name	: app_get_next_dev_ping_time
* Description	: 获取下一个设备的ping时间
* Parameter		: 
* Return		: 
*	
************************************************************/
uint32_t app_get_next_dev_ping_time(void)
{
	return sg_comparam_t.dev_ping;
}

/************************************************************
*
* Function name	: app_get_report_time
* Description	: 获取通信上报间隔时间
* Parameter		: 
* Return		: 
*	
************************************************************/
uint32_t app_get_report_time(void) 
{
	return sg_comparam_t.report;
}
/************************************************************
*
* Function name	: app_get_report_time
* Description	: 获取通信上报间隔时间
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t app_get_network_delay_time(void) 
{
	return sg_comparam_t.network_time;
}

/************************************************************
*
* Function name	: app_get_main_network_ping_ip_addr
* Description	: 获取主网络ping地址
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_get_main_network_ping_ip_addr(uint8_t* ip)
{
	ip[0] = sg_sysparam_t.local.ping_ip[0];
	ip[1] = sg_sysparam_t.local.ping_ip[1];
	ip[2] = sg_sysparam_t.local.ping_ip[2];
	ip[3] = sg_sysparam_t.local.ping_ip[3];
}

/************************************************************
*
* Function name	: app_get_main_network_sub_ping_ip_addr
* Description	: 获取主网pingip - 2
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_get_main_network_sub_ping_ip_addr(uint8_t* ip)
{
	ip[0] = sg_sysparam_t.local.ping_sub_ip[0];
	ip[1] = sg_sysparam_t.local.ping_sub_ip[1];
	ip[2] = sg_sysparam_t.local.ping_sub_ip[2];
	ip[3] = sg_sysparam_t.local.ping_sub_ip[3];
}
/************************************************************
*
* Function name	: app_get_single_ping_ip_addr
* Description	: 获取信号机IP
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_get_single_ping_ip_addr(uint8_t* ip)
{
	ip[0] = sg_sysparam_t.local.single_ip[0];
	ip[1] = sg_sysparam_t.local.single_ip[1];
	ip[2] = sg_sysparam_t.local.single_ip[2];
	ip[3] = sg_sysparam_t.local.single_ip[3];
}
/************************************************************
*
* Function name	: app_set_main_network_ping_ip
* Description	: 设置主网检测IP
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_main_network_ping_ip(uint8_t *ip)
{
	sg_sysparam_t.local.ping_ip[0] = ip[0];
	sg_sysparam_t.local.ping_ip[1] = ip[1];
	sg_sysparam_t.local.ping_ip[2] = ip[2];
	sg_sysparam_t.local.ping_ip[3] = ip[3];
	
	sg_sysparam_t.local.ping_sub_ip[0] = ip[4];
	sg_sysparam_t.local.ping_sub_ip[1] = ip[5];
	sg_sysparam_t.local.ping_sub_ip[2] = ip[6];
	sg_sysparam_t.local.ping_sub_ip[3] = ip[7];
	
	/* 保存 */
	app_set_save_infor_function(SAVE_LOCAL_NETWORK);
}

/*
*********************************************************************************************************
*	函 数 名: app_set_single_device_ping_ip
*	功能说明: 设置信号机IP.  
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void app_set_single_device_ping_ip(uint8_t *ip)
{
	sg_sysparam_t.local.single_ip[0] = ip[0];
	sg_sysparam_t.local.single_ip[1] = ip[1];
	sg_sysparam_t.local.single_ip[2] = ip[2];
	sg_sysparam_t.local.single_ip[3] = ip[3];

	/* 保存 */
	app_set_save_infor_function(SAVE_LOCAL_NETWORK);
}

/************************************************************
*
* Function name	: app_set_com_interface_selection_function
* Description	: 通信接口选择函数
* Parameter		: 
*	@mode		: 0:有线 1:外网
* Return		: 
*	
************************************************************/
void app_set_com_interface_selection_function(uint8_t mode)
{
	sg_sysoperate_t.com.send_mode = mode;
}


/************************************************************
*
* Function name	: app_get_com_interface_selection_function
* Description	: 通信接口选择函数
* Parameter		: 
*	@mode		: 0:有线 1:外网
* Return		: 
*	
************************************************************/
uint8_t app_get_com_interface_selection_function(void)
{
	return sg_sysoperate_t.com.send_mode;
}


/************************************************************
*
* Function name	: app_send_once_heart_infor
* Description	: 立刻进行一次心跳发生
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_send_once_heart_infor(void)
{
	sg_sysoperate_t.com_flag.heart_pack = 1;
}

/************************************************************
*
* Function name	: app_send_query_configuration_infor
* Description	: 连接平台后，发送配置
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_send_query_configuration_infor(void)
{
	sg_sysoperate_t.com_flag.query_configuration = 1;
}

/************************************************************
*
* Function name	: app_get_com_time_infor
* Description	: 获取通信间隔时间
* Parameter		: 
* Return		: 
*	
************************************************************/
void *app_get_com_time_infor(void)
{
	return &sg_comparam_t;
}

/************************************************************
*
* Function name	: app_get_network_mode
* Description	: 获取网络模式
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t app_get_network_mode(void) 
{
    switch(sg_sysparam_t.local.server_mode) {
        case 1:
            return 1;
        case 2:
            return 2;
        default:
            return 4;
    }
}

/************************************************************
*
* Function name	: app_set_com_time_param_function
* Description	: 设置通信相关时间参数:ping、上报
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_com_time_param_function(uint32_t *time,uint8_t mode) 
{
	if(mode == 0) 
	{
		sg_comparam_t.dev_ping   = time[1]*1000;
		sg_comparam_t.ping       = time[0]*1000;
		sg_comparam_t.onvif_time = time[2];  // ONVIF时间  20230811
	} 
	else 	if(mode == 1) 
	{
		sg_comparam_t.report  = time[0]*1000;
	}
	else 	if(mode == 2)   // 网络延时时间  20220308
	{
		sg_comparam_t.network_time  = time[0];
	}
	/* 保存 */
	app_set_save_infor_function(SAVE_COM_PARAMETER);
}

const char cg_network_no[] = {0xe6,0x97,0xa0,0xe8,0xbf,0x9e,0xe6,0x8e,0xa5,0x00};									// 未连接
const char cg_network_lan[] = {0xe6,0x9c,0x89,0xe7,0xba,0xbf,0xe5,0xb7,0xb2,0xe8,0xbf,0x9e,0xe6,0x8e,0xa5,0x00};	// 有线已连接
const char cg_netwokr_gprs[] = {0xe6,0x97,0xa0,0xe7,0xba,0xbf,0xe5,0xb7,0xb2 ,0xe8,0xbf,0x9e,0xe6,0x8e,0xa5,0x00};	// 无线已连接

/************************************************************
*
* Function name	: app_get_network_connect_status
* Description	: 获取网络连接状态
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_get_network_connect_status(char *buff)
{
	if(eth_get_tcp_status() == 2 ) {
		sprintf(buff,"%s",cg_network_lan);
	} else if( gsm_get_network_connect_status_function() == 1 ) {
		sprintf(buff,"%s",cg_netwokr_gprs);
	}else {
		sprintf(buff,"%s",cg_network_no);
	}
}

/************************************************************
*
* Function name	: app_get_device_name
* Description	: 获取设备名称
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t *app_get_device_name(void)
{
	return sg_sysparam_t.device.name;
}

/************************************************************
*
* Function name	: 
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_detect_function(void)
{
	if(det_get_pwr_status()==0) {
		sg_sysoperate_t.com_flag.report_normally = 1;
	}
}

/************************************************************
*
* Function name	: app_set_threshold_param_function
* Description	: 设置阈值
* Parameter		: 
* Return		: 
*	   20230720
************************************************************/
void app_set_threshold_param_function(struct threshold_params param)
{
	sg_sysparam_t.threshold.volt_max = param.volt_max;
	sg_sysparam_t.threshold.volt_min = param.volt_min; 
	sg_sysparam_t.threshold.current = param.current;
	sg_sysparam_t.threshold.angle = param.angle;
  sg_sysparam_t.threshold.miu = param.miu;	
	sg_sysparam_t.threshold.humi_high = param.humi_high;
	sg_sysparam_t.threshold.humi_low = param.humi_low; 
	sg_sysparam_t.threshold.temp_high = param.temp_high;
	sg_sysparam_t.threshold.temp_low = param.temp_low; 
	save_stroage_threshold_parameter(&sg_sysparam_t.threshold); 
}
/************************************************************
*
* Function name	: app_get_threshold_param_function
* Description	: 获取阈值
* Parameter		: 
* Return		: 
*	   20230720
************************************************************/
void *app_get_threshold_param_function(void)
{
	return (&sg_sysparam_t.threshold);
}

/************************************************************
*
* Function name	: app_get_backups_param_function
* Description	: 获取备份数据
* Parameter		: 
* Return		: 
*	   20231022
************************************************************/
void *app_get_backups_param_function(void)
{
	return (&sg_backups_t);
}
/************************************************************
*
* Function name	: app_server_link_status_function
* Description	: 服务器连接时间判断
* Parameter		: 
* Return		: 
*	   20231022
************************************************************/
void app_server_link_status_function(void)
{
	static uint32_t server_time_count = 0;

	server_time_count++;
	if(server_time_count >= SERVER_LINK_TIME)
	{
		server_time_count = 0;
		if(sg_backups_t.config_flag == 1)  // 配置过服务器
		{
			sg_backups_t.config_flag = 0;
			save_stroage_backups_function(&sg_backups_t);
			if((eth_get_tcp_status() != 2 ) && ( gsm_get_network_connect_status_function() != 1 ))
			{
				save_read_default_remote_ip(&sg_sysparam_t.remote);
				save_stroage_remote_ip_function(&sg_sysparam_t.remote);
				vTaskDelay(100);
				app_system_softreset(1000);
			}
		}
	}
}

/************************************************************
*
* Function name	: app_save_backups_remote_param_function
* Description	: 备份服务器信息
* Parameter		: 
* Return		: 
*	   20231022
************************************************************/
void app_save_backups_remote_param_function(void)
{
	memset(sg_backups_t.remote.outside_iporname,0,sizeof(sg_backups_t.remote.outside_iporname));
	memset(sg_backups_t.remote.inside_iporname,0,sizeof(sg_backups_t.remote.inside_iporname));
	strcpy((char*)sg_backups_t.remote.inside_iporname,(char*)sg_sysparam_t.remote.inside_iporname);
	sg_backups_t.remote.inside_port = sg_sysparam_t.remote.inside_port;
	strcpy((char*)sg_backups_t.remote.outside_iporname,(char*)sg_sysparam_t.remote.outside_iporname);	
	sg_backups_t.remote.outside_port = sg_sysparam_t.remote.outside_port;
	sg_backups_t.config_flag = 1;
	
	save_stroage_backups_function(&sg_backups_t); // 存储备份信息
}


/************************************************************
*
* Function name	: app_power_fail_protection_function
* Description	: 掉电保护
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_power_fail_protection_function(void)
{
	/* 开关全部关闭 */
	if(relay_get_status(RELAY_1) == RELAY_ON) 
		relay_control(RELAY_1,RELAY_OFF);
	
	if(relay_get_status(RELAY_2) == RELAY_ON) 
		relay_control(RELAY_2,RELAY_OFF);

	if(relay_get_status(RELAY_3) == RELAY_ON) 
		relay_control(RELAY_3,RELAY_OFF);
	
	if(relay_get_status(RELAY_4) == RELAY_ON) 
		relay_control(RELAY_4,RELAY_OFF);
}
/************************************************************
*
* Function name	: app_power_open_protection_function
* Description	: 打开继电器
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_power_open_protection_function(void)
{
	/* 开关全部打开 */
	relay_control(RELAY_1,RELAY_ON); // 开继电器
	vTaskDelay(1000);
	relay_control(RELAY_2,RELAY_ON); // 开继电器
	vTaskDelay(1000);
	relay_control(RELAY_3,RELAY_ON); // 开继电器
	vTaskDelay(1000);
	relay_control(RELAY_4,RELAY_ON); // 开继电器
	vTaskDelay(1000);
}

/************************************************************
*
* Function name	: app_send_data_task_function
* Description	: 发送数据
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_send_data_task_function(void)
{
	if(sg_sysoperate_t.com.send_mode == 0)
		tcp_set_send_buff(sg_send_buff,sg_send_size);
	else
		gsm_send_tcp_data(sg_send_buff,sg_send_size);
}

/************************************************************
*
* Function name	: app_set_update_status_function
* Description	: 更新结果
* Parameter		: 
*	@mode		: 
* Return		: 
*	
************************************************************/
void app_set_update_status_function(uint8_t flag)
{
	sg_sysoperate_t.update.status = flag;
}

/************************************************************
*
* Function name	: app_set_update_status_function
* Description	: 更新结果
* Parameter		: 
*	@mode		: 
* Return		: 
*	
************************************************************/
uint8_t app_get_update_status_function(void)
{
	return sg_sysoperate_t.update.status;
}

/************************************************************
*
* Function name	: app_get_http_ota_function
* Description	: 获取更新
* Parameter		: 
* Return		: 
*	
************************************************************/
void *app_get_http_ota_function(void)
{
	return (&sg_sysparam_t.ota);
} 
/************************************************************
*
* Function name	: app_set_http_ota_function
* Description	: 存储更新地址
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_set_http_ota_function(struct update_addr param)
{
	memcpy(&sg_sysparam_t.ota,&param,sizeof(struct update_addr));
	save_stroage_http_ota_function(&sg_sysparam_t.ota);
}
/************************************************************
*
* Function name	: app_system_softreset
* Description	: 系统重启
* Parameter		: 
* Return		: 
*	
************************************************************/
static uint32_t sg_reboot_time = 0;

void app_system_softreset(uint32_t time)
{
	vTaskDelay(100);
	lfs_unmount(&g_lfs_t);
	sg_reboot_time = time;
}
/*
*********************************************************************************************************
*	函 数 名: System_SoftReset
*	形    参: 无 
*	返 回 值: 无
*********************************************************************************************************
*/
void System_SoftReset(void)
{
	taskENTER_CRITICAL();
	sys_intx_disable( ); //关闭所有中断 
	HAL_NVIC_SystemReset(); //复位
}

/************************************************************
*
* Function name	: 
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_reboot_timer_run(void)
{
	if(sg_reboot_time != 0) {
		sg_reboot_time--;
		if(sg_reboot_time==0) {
			System_SoftReset();
		}
	}
}

