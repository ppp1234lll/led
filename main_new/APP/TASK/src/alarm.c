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
		uint16_t 	  fault_code;	   // 故障码
	} com;
	struct
	{
		uint8_t save_device_param;   	// 存储设备参数
		uint8_t save_local_network;  	// 保存本地网络参数
		uint8_t save_remote_network; 	// 保存远端网络参数
		uint8_t save_update_addr;    	// 保存更新地址
		uint8_t com_parameter;			 	// 通信相关参数
    uint8_t save_carema;       		// 摄像头参数 20230712
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
		uint8_t lbs_info;					   // 参数查询   20220329
	} com_flag;
	struct
	{
		uint8_t adapter_num;		  // 适配器 - 编号
		uint8_t dc_num;		        // 适配器 - 编号
		uint8_t current_protection;	  // 电流保护
		uint8_t volt_protection;	  // 电压保护
		uint8_t miu_protection;	  // 漏电
	} sys;
	struct
	{
		uint8_t lwip_reset;         // 网络重启标志
		uint8_t adapter_reset;		  // 适配器-重启标志
		uint8_t relay_reset[8];		   
    uint8_t dc_reset;		        // 适配器-重启标志	
	} sys_flag;
	struct
	{
		uint8_t status;         // 更新结果
	} update;
}sys_operate_t;

struct switch_control_t {
	uint8_t adapter1; // 适配器1
	uint8_t adapter2; // 适配器2
	uint8_t adapter3; // 适配器3
	uint8_t adapter4; // 适配器4
	uint8_t adapter5; // 适配器5
	uint8_t adapter6; // 适配器6
	uint8_t adapter7; // 适配器7
	uint8_t adapter8; // 适配器8
};

/* 参数定义 */
__attribute__((section (".RAM_D1")))  sys_backups_t sg_backups_t   	= {0}; // 备份信息 20231022
__attribute__((section (".RAM_D1")))  sys_operate_t sg_sysoperate_t; // 系统操作参数：包括通信、存储、计时
__attribute__((section (".RAM_D1")))  sys_param_t   sg_sysparam_t   = {0}; // 系统参数：本地、远端、设备、上报相关参数
__attribute__((section (".RAM_D1")))  carema_t      sg_carema_param_t;   //onvif 摄像头信息
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
__attribute__((section (".RAM_D1")))  struct switch_control_t sg_swcontrol_t = {0};

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
	for(;;)
	{
		alarm_collection_param(); 	// 采集数据监测任务
    iwdg_feed();	
		vTaskDelay(10);
	}
}

/*
*********************************************************************************************************
*	函 数 名: alarm_elec_collection_param
*	功能说明: 电源判断
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void alarm_elec_collection_param(void)
{
	static uint32_t elec_error  = 0;  // 故障上报状态
	static uint32_t elec_normal	= 0;  // 正常上报状态：
 
	/* 适配器、断电上报 */
	if(det_get_pwr_status() == 0) // 12V断电
	{
		if(det_get_vin220v_handler(0) < 50)  // 市电电压 < 50V，说明断电
		{
			if( (elec_error & 0x01) == 0) 
			{
				elec_error |= 0x01;
				elec_normal &=~0x01;
				app_power_fail_protection_function();  // 关闭继电器
				app_report_information_immediately();
			}						
		}
		else 					   // 市电电压 > 50V，说明适配器故障
		{
			if((elec_error & 0x02) == 0) 
			{
				elec_error  |= 0x02;
				elec_normal &=~0x02;
				app_report_information_immediately();
			}
		}
	}		
	else            // 12V有电，220V存在
	{
		if(((elec_normal & 0x01) == 0) || ((elec_normal&0x02) == 0))
		{
			if((elec_error&0x01) || (elec_error&0x02))// 适配器故障、220断电
			{
				/* 适配器重新上电 - 重启设备*/
				lfs_unmount(&g_lfs_t);
				vTaskDelay(100);
				app_system_softreset(1000);
			}
			vTaskDelay(100);
			elec_normal |= 0x01;
			elec_error  &=~0x01;
			
			elec_normal |= 0x02;
			elec_error &=~ 0x02;
			app_report_information_immediately();
			vTaskDelay(2000);
			app_power_open_protection_function();  // 打开继电器

			sg_sysoperate_t.sys.current_protection = 0;
			sg_sysoperate_t.sys.volt_protection = 0;
		}		
	}

	// 高压报警
	if( sg_sysparam_t.threshold.volt_max == 0 )  // 阈值为0，不作处理
	{	}
	else
	{
		if(det_get_vin220v_handler(0) >= sg_sysparam_t.threshold.volt_max) 
		{	
			if((elec_error & 0x04) == 0)  // 故障上报标志位是0
			{
				elec_error |= 0x04;        // 标志位置1，表示已上报
				elec_normal &=~ 0x04;      // 正常上报标志位清0
				app_power_fail_protection_function(); // 关闭继电器		
				sg_sysoperate_t.sys.volt_protection = 1;
				app_report_information_immediately();	
			}
		}
		else if(det_get_vin220v_handler(0) >= 50) // 市电有电情况下
		{
			if((elec_normal & 0x04) == 0)  // 正常上报标志位是0
			{
				elec_normal |= 0x04;  // 标志位置1，表示已上报
				elec_error &=~ 0x04; // 故障上报标志位清0
				if(sg_sysoperate_t.sys.volt_protection == 1)
				{
						sg_sysoperate_t.sys.volt_protection = 0;
						app_report_information_immediately();
						vTaskDelay(2000);
						app_power_open_protection_function();  // 打开继电器
				}
			}
		}	
	}

	// 低压报警
	if( sg_sysparam_t.threshold.volt_min == 0 )  // 阈值为0，不作处理
	{	}
	else
	{
		if((det_get_vin220v_handler(0) <= sg_sysparam_t.threshold.volt_min) && \
			  det_get_vin220v_handler(0) >= 20)
		{	
			if((elec_error & 0x08) == 0)  // 故障上报标志位是0
			{
				elec_error |= 0x08;        // 标志位置1，表示已上报
				elec_normal &=~ 0x08;      // 正常上报标志位清0		
				
				app_power_fail_protection_function();  // 关闭继电器
				sg_sysoperate_t.sys.volt_protection = 2;
				app_report_information_immediately();
			}
		}
		else if(det_get_vin220v_handler(0) >= sg_sysparam_t.threshold.volt_min) // 市电有电情况下
		{
			if((elec_normal & 0x08) == 0)  // 正常上报标志位是0
			{
				elec_normal |= 0x08;  // 标志位置1，表示已上报
				elec_error &=~ 0x08; // 故障上报标志位清0
				if(sg_sysoperate_t.sys.volt_protection == 2)
				{
						sg_sysoperate_t.sys.volt_protection = 0;
					  app_report_information_immediately();
						app_power_open_protection_function();  // 打开继电器
				}
			}
		}
	}

	/* 检测市电电流使用情况 */
	if( sg_sysparam_t.threshold.current == 0 )  // 阈值为0，不作处理
	{	}
	else
	{
		if(det_get_vin220v_handler(1) >= sg_sysparam_t.threshold.current)
		{
			/* 电流过大 , 关闭所有外设，并报警 */
			app_power_fail_protection_function(); // 关闭继电器
			if( (elec_error & 0x10) == 0) 
			{
				elec_error |= 0x10;
				elec_normal &=~ 0x10;
				sg_sysoperate_t.sys.current_protection = 1;
				
				app_report_information_immediately();
			}
		} 
		else  if(det_get_vin220v_handler(1) >= 30)
		{
			if( (elec_normal & 0x10) == 0)
			{
				elec_normal |= 0x10;
				elec_error &=~ 0x10;
				sg_sysoperate_t.sys.current_protection = 0;

				app_report_information_immediately();
			}
		}
	}
	/* 漏电预警 */
  if(det_get_miu_value(0) > sg_sysparam_t.threshold.miu)
	{
		if((elec_error & 0x20) == 0) 
		{
			elec_error  |= 0x20;
			elec_normal &=~0x20;
			app_power_fail_protection_function();  // 关闭继电器
			sg_sysoperate_t.sys.miu_protection = 2;
			app_report_information_immediately();
		}
	} 
	else 
	{
		if((elec_normal & 0x40) == 0) 
		{
			elec_normal |= 0x40;
			elec_error  &=~0x40;
			if(sg_sysoperate_t.sys.miu_protection == 2)
			{
				sg_sysoperate_t.sys.miu_protection = 0;
				app_report_information_immediately();
				app_power_open_protection_function();  // 打开继电器
			}
		}	
	}	

	if(det_get_mcb220_value() == 0)
	{
		if(det_get_vin220v_handler(0) < 50)
		{
			if((elec_error & 0x0800) == 0) 
			{
				elec_error  |= 0x0800;
				elec_normal &=~0x0800;
				sg_sysoperate_t.sys.mcb_status = 2;	
				app_report_information_immediately();
			}
		}
		else
		{
			if((elec_normal & 0x0800) == 0) 
			{
				elec_normal |= 0x0800;
				elec_error  &=~0x0800;
				sg_sysoperate_t.sys.mcb_status = 1;	
				app_report_information_immediately();
			}		
		}
	} 
	else  
	{
		sg_sysoperate_t.sys.mcb_status = 0;	
	}				
}

