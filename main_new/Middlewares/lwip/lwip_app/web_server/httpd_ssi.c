#include "./web_server/httpd_cgi_ssi.h"
#include "appconfig.h"

const char cg_ssi_open[]   = {0xe5,0xbc,0x80,0x00};				  // 开启
const char cg_ssi_close[]  = {0xe5,0x85,0xb3,0x00};				  // 关闭
const char cg_ssi_normal[] = {0xe6,0xad,0xa3,0xe5,0xb8,0xb8,0x00}; // 正常
const char cg_ssi_error[]  = {0xe6,0x95,0x85,0xe9,0x9a,0x9c,0x00}; // 故障

const char spd_ssi_none[]  = {0xE4,0xB8,0x8D,0xE6,0xA3,0x80,0xE6,0xB5,0x8B,0x00}; // 不检测
const char spd_ssi_error[]  = {0xE5,0xB7,0xB2,0xE5,0xA4,0xB1,0xE6,0x95,0x88,0x00}; // 已失效
const char spd_ssi_ok[]  = {0xe6,0xad,0xa3,0xe5,0xb8,0xb8,0x00}; // 正常

const char water_ssi_none[]  = {0xE4,0xB8,0x8D,0xE6,0xA3,0x80,0xE6,0xB5,0x8B,0x00}; // 不检测
const char water_ssi_error[] = {0xe6,0xbc,0x8f,0xe6,0xb0,0xb4,0x00}; // 漏水
const char water_ssi_ok[]    = {0xe6,0xad,0xa3,0xe5,0xb8,0xb8,0x00}; // 正常



/************************************************************
*
* Function name	: Vin220V_Handler
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
void Vin220_Handler(char *pcInsert, uint8_t num)
{
	fp32 	 temp 	 =  det_get_vin220v_handler(num);
//	uint32_t data[2] = {0};
//	
//	data[0] = (uint32_t)temp;
//	temp	= temp - data[0];  
//	data[1] = temp*100;
//	
//	sprintf(pcInsert,"%d.%02d",data[0],data[1]);

	sprintf(pcInsert,"%.2f",temp);
}

/************************************************************
*
* Function name	: open_door_status_Handler
* Description	: 箱门状态
* Parameter		: 
* Return		: 
*	
************************************************************/
static void open_door_status_Handler(char *pcInsert,uint8_t id)
{
	uint8_t data = det_get_door_status(id);
	
	if(data == 1)
	{
		sprintf(pcInsert,"%s",cg_ssi_open);
	}
	else
	{
		sprintf(pcInsert,"%s",cg_ssi_close);
	}
}

/************************************************************
*
* Function name	: cabinet_posture_Handler
* Description	: 箱体姿态
* Parameter		: 
* Return		: 
*	
************************************************************/
static void cabinet_posture_Handler(char *pcInsert)
{
	uint8_t data = det_get_cabinet_posture();
	sprintf(pcInsert,"%d",data);
}

/************************************************************
*
* Function name	: water_status_Handler
* Description	: 浸水状态
* Parameter		: 
* Return		: 
*	
************************************************************/
static void water_status_Handler(char *pcInsert,uint8_t id)
{
	uint8_t data = det_get_water_status(id);
	switch(data)
	{
		case 0: sprintf(pcInsert,"%s",water_ssi_ok);  break;
		case 1: sprintf(pcInsert,"%s",water_ssi_error); break;
		case 2: sprintf(pcInsert,"%s",water_ssi_none); break;
		default:break;
	}
}

/************************************************************
*
* Function name	: 漏电_Handler
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
static void Miu_Handler(char *pcInsert, uint8_t id)
{
	fp32 	 temp 	 =  sg_datacollec_t.residual_c[id];
//	uint32_t data[2] = {0};
//	
//	data[0] = (uint32_t)temp;
//	temp	= temp - data[0];  
//	data[1] = temp*100;
//	
//	sprintf(pcInsert,"%d.%02d",data[0],data[1]);
	sprintf(pcInsert,"%.2f",temp);
}

/************************************************************
*
* Function name	: httpd_ssi_volt_cur_data_collection_function
* Description	: 电能参数界面更新
* Parameter		: 
* Return		: 
*	
************************************************************/
void httpd_ssi_volt_cur_data_collection_function(char *pcInsert)
{
	char buff[6][8] = {0};
	
	Vin220_Handler(buff[0],0);		// 总电压
	Vin220_Handler(buff[1],1);		// 总电流
	Vin220_Handler(buff[2],2);		// 电流通道1
	Vin220_Handler(buff[3],3);		// 电流通道2
	Vin220_Handler(buff[4],4);		// 电流通道3
	Vin220_Handler(buff[5],5);		// 电流通道4
	
	sprintf(pcInsert,"[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]",
										buff[0],buff[1],buff[2],buff[3],buff[4],buff[5]);
}


/************************************************************
*
* Function name	: httpd_ssi_other_data_collection_function
* Description	: 其他参数界面更新
* Parameter		: 
* Return		: 
*	
************************************************************/
void httpd_ssi_other_data_collection_function(char *pcInsert)
{
	char buff[4][8] = {0};
	char new_buff[5][7] = {0};
	char new_buff1[4][30] = {0};
//	uint16_t data[2] = {0};
	float    temp    = 0;
	uint8_t  unit[] = {0xe2,0x84,0x83};
	
//	temp = det_get_inside_temp(0);
//	if(temp < 0) {
//		temp = 0-temp;
//		data[0] = (uint16_t)temp;
//		temp	= temp - data[0];  
//		data[1] = temp*100;
//		sprintf(buff[0],"-%d.%02d",data[0],data[1]);		// 温度
//	} else {
//		data[0] = (uint16_t)temp;
//		temp	= temp - data[0];  
//		data[1] = temp*100;
//		sprintf(buff[0],"%d.%02d",data[0],data[1]);			// 温度
//	}
//	
//	temp = det_get_inside_humi(0);
//	data[0] = (uint16_t)temp;
//	temp	= temp - data[0];  
//	data[1] = temp*100;
//	sprintf(buff[1],"%d.%02d",data[0],data[1]);			// 湿度
	temp = det_get_inside_temp(0);
	sprintf(buff[0],"%.2f",temp);
	temp = det_get_inside_humi(0);
	sprintf(buff[1],"%.2f",temp);

	temp = det_get_inside_temp(1);
	sprintf(buff[2],"%.2f",temp);
	temp = det_get_inside_humi(1);
	sprintf(buff[3],"%.2f",temp);
	
	cabinet_posture_Handler(new_buff[0]);		 
	open_door_status_Handler(new_buff[1],0);	 
	open_door_status_Handler(new_buff[2],1);	
	open_door_status_Handler(new_buff[3],2);	
	open_door_status_Handler(new_buff[4],3);	
	
	water_status_Handler(new_buff1[0],0);
	water_status_Handler(new_buff1[1],1);
	
	Miu_Handler (new_buff1[2],0);
	Miu_Handler (new_buff1[3],1);	
	sprintf(pcInsert,"[\"%s%s\",\"%s%%\",\"%s%s\",\"%s%%\",\"%s\",\
										 \"%s\",\"%s\",\"%s\",\"%s\",\
										 \"%s\",\"%s\",\"%s\",\"%s\"]",
			buff[0],unit,buff[1],buff[2],unit,buff[3],new_buff[0],\
			new_buff[1],new_buff[2],new_buff[3],new_buff[4],\
			new_buff1[0],new_buff1[1],new_buff1[2],new_buff1[3]);
}

/************************************************************
*
* Function name	: httpd_ssi_threshold_seting_function
* Description	: 阈值信息更新
* Parameter		: 
* Return		: 
*	  20230720
************************************************************/
void httpd_ssi_threshold_seting_function(char *pcInsert)
{
	char buff[11][5]   = {0};
	struct threshold_params *param = app_get_threshold_param_function();

	sprintf(buff[0],"%d",param->volt_max);
	sprintf(buff[1],"%d",param->volt_min);
	sprintf(buff[2],"%d",param->current);
	sprintf(buff[3],"%d",param->angle);
	sprintf(buff[4],"%d",param->temp_high);
	sprintf(buff[5],"%d",param->temp_low);
	sprintf(buff[6],"%d",param->humi_high);
	sprintf(buff[7],"%d",param->humi_low);
	sprintf(buff[8],"%d",param->miu);
	sprintf(buff[9],"%d",param->led_current);


	sprintf(pcInsert,"[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]",
										buff[0],buff[1],buff[2],buff[3],buff[4],buff[5],buff[6],buff[7],buff[8],buff[9]);

}
/************************************************************
*
* Function name	: httpd_ssi_threshold_seting_function
* Description	: 阈值信息更新
* Parameter		: 
* Return		: 
*	  20230720
************************************************************/
void httpd_ssi_bd_data_collection_function(char *pcInsert)
{
	char buff[6][16] = {0};
	atgm336h_data_t *param = atgm336h_get_gnss_data();	
	float    temp    = 0;
	uint32_t data[2] = {0};
	
	temp = param->altitude;
	data[0] = (uint16_t)temp;
	temp	= temp - data[0];  
	data[1] = temp*100;
	sprintf(buff[2],"%d.%02d",data[0],data[1]);		
	
	sprintf(buff[0],"%d",param->num_satellites);
	sprintf(buff[1],"%d",param->num_satellites);
	sprintf(buff[2],"%.2f",param->altitude);
	sprintf(buff[4],"%f",param->latitude);
	sprintf(buff[3],"%f",param->longitude);
	sprintf(buff[5],"%d",param->fix_status);
	
	sprintf(pcInsert,"[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]",
										buff[0],buff[1],buff[2],buff[3],buff[4],buff[5]);
}

/************************************************************
*
* Function name	: Switch_status_Handler
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
static void Switch_status_Handler(char *pcInsert,RELAY_DEV dev)
{
	int8_t state = relay_get_status(dev);
	
	if(state == RELAY_OFF)
	{
		sprintf(pcInsert,"0");
	}
	else
	{
		sprintf(pcInsert,"1");
	}	
}


/************************************************************
*
* Function name	: httpd_ssi_switch_status_function
* Description	: 开关状态
* Parameter		: 
* Return		: 
*	
************************************************************/
void httpd_ssi_switch_status_function(char *pcInsert)
{
	char buff[8][3] = {0};
	
	Switch_status_Handler(buff[0],RELAY_1);	// 适配器1的开关状态
	Switch_status_Handler(buff[1],RELAY_2);	// 适配器2的开关状态
	Switch_status_Handler(buff[2],RELAY_3);	// 适配器1的开关状态
	Switch_status_Handler(buff[3],RELAY_4);	// 适配器2的开关状态

		
	sprintf(pcInsert,"[\"%s\",\"%s\",\"%s\",\"%s\"]",
										buff[0],buff[1],buff[2],buff[3]);
}

/************************************************************
*
* Function name	: system_param_Handler
* Description	: 系统参数
* Parameter		: 
*	@num		: 0:终端序列号 1：软件版本号 2：硬件版本号 
* Return		: 
*	
************************************************************/
static void system_param_Handler(char *pcInsert, uint8_t num)
{
  extern ChipID_t g_chipid_t;
	switch(num)
	{
		case 0:
			sprintf(pcInsert,"%04X%04X%04X",g_chipid_t.id[0],g_chipid_t.id[1],g_chipid_t.id[2]);
			break;
		case 1:
			sprintf(pcInsert,"%s",SOFT_NO_STR);
			break;
		case 2:
			sprintf(pcInsert,"%s",HARD_NO_STR);
			break;
	}
}

/************************************************************
*
* Function name	: httpd_ssi_system_status_functions
* Description	: 系统状态
* Parameter		: 
* Return		: 
*	
************************************************************/
void httpd_ssi_system_status_function(char *pcInsert)
{
	char buff[3][30] = {0};
	
	system_param_Handler(buff[0],0);
	system_param_Handler(buff[1],1);
	system_param_Handler(buff[2],2);

	
	sprintf(pcInsert,"[\"%s\",\"%s\",\"%s\"]",
			buff[0],buff[1],buff[2]);
	
}

/************************************************************
*
* Function name	: get_now_time_handler
* Description	: 获取当前时间
* Parameter		: 
* Return		: 
*	
************************************************************/
static void get_now_time_handler(char *pcInsert)
{
	app_get_current_time(pcInsert);
}

/************************************************************
*
* Function name	: device_parameter_handler
* Description	: 获取设备参数
* Parameter		: 
*	@pcInsert	:
*	@num		: 0:ID 1:name 2:password
* Return		: 
*	
************************************************************/
static void device_parameter_handler(char *pcInsert,uint8_t num)
{
	struct device_param *param;
	param = app_get_device_param_function();
	
	switch(num)
	{
		case 0:
//			sprintf(pcInsert,"%08d",param->id);
			sprintf(pcInsert,"%06X",param->id.i);
			break;
		case 1:
			sprintf(pcInsert,"%s",param->name);
			break;
		case 2:
			sprintf(pcInsert,"%s",param->password);
			break;
	}
}

/************************************************************
*
* Function name	: local_network_Handler
* Description	: 本地网络信息
* Parameter		: 
*	@pcInsert	: 
*	@mode		: 0：IP 1：网关 2：掩码 3：DNS 4: 升级地址 5：升级端口 6：传输模式 7: 主网检测地址 8:主网检测地址2
* Return		: 
*	
************************************************************/
static void local_network_Handler(char *pcInsert, uint8_t mode)
{
	struct local_ip_t *local = app_get_local_network_function();
	
	switch(mode)
	{
		case 0:
			sprintf(pcInsert,"%d.%d.%d.%d",local->ip[0],local->ip[1],local->ip[2],local->ip[3]);
			break;
		case 1:
			sprintf(pcInsert,"%d.%d.%d.%d",local->gateway[0],local->gateway[1],local->gateway[2],local->gateway[3]);
			break;
		case 2:
			sprintf(pcInsert,"%d.%d.%d.%d",local->netmask[0],local->netmask[1],local->netmask[2],local->netmask[3]);
			break;
		case 3:
			sprintf(pcInsert,"%d.%d.%d.%d",local->dns[0],local->dns[1],local->dns[2],local->dns[3]);
			break;
		case 4:
			sprintf(pcInsert,"%s","0.0.0.0");
			break;
		case 5:
			sprintf(pcInsert,"%d",0);
			break;
		case 6:
			switch(local->server_mode) {
				case 1:
					sprintf(pcInsert,"%d",0);
					break;
				case 2:
					sprintf(pcInsert,"%d",1);
					break;
				case 4:
					sprintf(pcInsert,"%d",2);
					break;
			}
			break;
		case 7:
			sprintf(pcInsert,"%d.%d.%d.%d",local->ping_ip[0],local->ping_ip[1],local->ping_ip[2],local->ping_ip[3]);
			break;
		case 8:
			sprintf(pcInsert,"%d.%d.%d.%d",local->ping_sub_ip[0],local->ping_sub_ip[1],local->ping_sub_ip[2],local->ping_sub_ip[3]);
			break;
		case 9:
			sprintf(pcInsert,"%02x-%02x-%02x-%02x-%02x-%02x",local->mac[0],local->mac[1],local->mac[2],local->mac[3],local->mac[4],local->mac[5]);
			break;
		case 10:
			sprintf(pcInsert,"%d.%d.%d.%d",local->multicast_ip[0],local->multicast_ip[1],local->multicast_ip[2],local->multicast_ip[3]);
			break;
		case 11:
			sprintf(pcInsert,"%d",local->multicast_port);
			break;
		case 12:
			sprintf(pcInsert,"%d",local->search_mode - 1);
			break;
		case 13:
			sprintf(pcInsert,"%d.%d.%d.%d",local->single_ip[0],local->single_ip[1],local->single_ip[2],local->single_ip[3]);
			break;
	}
}

/************************************************************
*
* Function name	: httpd_ssi_system_seting_function
* Description	: 系统设置信息更新
* Parameter		: 
* Return		: 
*	
************************************************************/
void httpd_ssi_system_seting_function(char *pcInsert)
{
	char buff[4][20] = {0};
	char time[30] 	 = {0};
	char buff2[2][3]    = {0};
	
	get_now_time_handler(time);
	device_parameter_handler(buff[0],0);
	device_parameter_handler(buff[1],1);
	device_parameter_handler(buff[2],2);
	app_get_network_connect_status(buff[3]);
	local_network_Handler(buff2[0],6);
	local_network_Handler(buff2[1],12);
	
	sprintf(pcInsert,"[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]", 
			time,buff[0],buff[1],buff[2],buff[3],buff2[0],buff2[1]);

}


/************************************************************
*
* Function name	: httpd_ssi_nework_gprs_show_function
* Description	: 无线网络设置更新
* Parameter		: 
* Return		: 
*	
************************************************************/
void httpd_ssi_nework_gprs_show_function(char *pcInsert)
{
	char buff1[8] = {0};
  char buff2[24] = {0};
  char buff3[3][30] = {0};
	
	gsm_gst_run_status_function(buff1,0);
	gsm_gst_run_status_function(buff2,1);
	sprintf(buff3[0],"%s",gsm_get_sim_ccid_function());
	sprintf(buff3[1],"%s",gprs_get_model_soft_function());
	sprintf(buff3[2],"%s",gprs_get_imei_function());
	
	sprintf(pcInsert,"[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]",buff1,buff2,buff3[0],buff3[1],buff3[2]);
}

/************************************************************
*
* Function name	: httpd_ssi_network_setting_function
* Description	: 网络参数信息更新
* Parameter		: 
* Return		: 
*	
************************************************************/
void httpd_ssi_network_setting_function(char* pcInsert)
{
	char buff[10][20] = {0};
	char time[5] = {0};   // 网络延时时间  20220308
	
	local_network_Handler(buff[0],0);	// IP
	local_network_Handler(buff[1],2);	// 子网掩码
	local_network_Handler(buff[2],1);	// 网关
	local_network_Handler(buff[3],3);	// DNS
	local_network_Handler(buff[6],9);	// MAC
	local_network_Handler(buff[4],7);   // 主网地址
	local_network_Handler(buff[5],8);   // 主网地址
	local_network_Handler(buff[9],13);		// 信号机地址
	sprintf(time,"%d",app_get_network_delay_time());		// 网络延时时间  20220308
	local_network_Handler(buff[7],10);	// IP
	local_network_Handler(buff[8],11);	// 组播
	
	sprintf(pcInsert,"[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]",
			buff[0],buff[1],buff[2],buff[3],buff[6],buff[4],buff[5],buff[9],time,buff[7],buff[8]);

}

/************************************************************
*
* Function name	: http_ssi_server_setting_function
* Description	: 服务器设置更新函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void http_ssi_server_setting_function(char *pcInsert)
{
	struct remote_ip *remote = app_get_remote_network_function();
	struct remote_ip *p_back = app_get_backups_function();	
	
	char ip[20]  = {0};
	char port[6] = {0};

	local_network_Handler(ip,4);
	local_network_Handler(port,5);
	
	sprintf(pcInsert,"[\"%s\",\"%d\",\"%s\",\"%d\",\"%s\",\"%d\",\"%s\",\"%d\",\"%ds\"]",\
			remote->inside_iporname,remote->inside_port,\
			remote->outside_iporname,remote->outside_port,
			p_back->inside_iporname,p_back->inside_port,\
			p_back->outside_iporname,p_back->outside_port,
			app_get_report_time()/1000);
}

/************************************************************
*
* Function name	: http_ssi_update_addr_function
* Description	: 更新地址
* Parameter		: 
* Return		: 
*	
************************************************************/
void http_ssi_update_addr_function(char *pcInsert)
{
	char ip[20]  = {0};
	struct update_addr *param = app_get_http_ota_function();
	sprintf(ip,"%d.%d.%d.%d",param->ip[0],param->ip[1],param->ip[2],param->ip[3]);
	sprintf(pcInsert,"[\"%s\",\"%d\"]",ip,param->port);
}




