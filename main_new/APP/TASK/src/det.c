#include "appconfig.h"
#include "./TASK/inc/det.h"

__attribute__((section (".RAM_D1"))) data_collection_t sg_datacollec_t;

/*
*********************************************************************************************************
*	函 数 名: det_task_function
*	功能说明: 检测线程
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_task_function(void)
{
	while(1)
	{
		ebtn_APP_Process();              // 定期处理按键事件（建议5-20ms）
		det_get_key_status_function();	 // 按键检测函数
		det_get_temphumi_function(); 		 // 获取温湿度
		det_get_attitude_state_value();  // 获取姿态数据
		bl0910_work_process_function();	 // 数据获取函数
		bl0939_work_process_function();
		det_get_gps_value();             // 获取GPS数据
		iwdg_feed();			 		           // 喂狗			
		vTaskDelay(10);  	 	  
	}
}

/*
*********************************************************************************************************
*	函 数 名: det_get_key_status_function
*	功能说明: 处理按键值
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_get_key_status_function(void)
{
	uint8_t i=0;
	if(sg_datacollec_t.key_s[RESET_K1] == KEY_EVNT)	 // 恢复出厂化
	{
		det_set_key_value(RESET_K1,KEY_NONE);
		led_control_function(LD_GPRS,LD_ON); 
		app_set_reset_function();/* 将系统设置参数恢复为默认值，需要重启生效 */
	}
	else if(sg_datacollec_t.key_s[RESET_K1] == KEY_ERASE)	 // 擦除FLASH
	{
		det_set_key_value(RESET_K1,KEY_NONE);
		led_control_function(LD_GPRS,LD_ON); 
		app_set_reset_function();/* 将系统设置参数恢复为默认值，需要重启生效 */
	}
	
	for(i=0; i< KEY_MAX;i++ )
	{
		sg_datacollec_t.key_s[i+1] = KeyScan10ms(i);
	}
	
}

/*
*********************************************************************************************************
*	函 数 名: det_main_network_and_camera_network
*	功能说明: 主网络与摄像头网络状态检查
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t det_main_network_and_camera_network(void)
{
	static uint8_t main_ip[2] = {0};
 
	/* 检测主网络 */
	if(sg_datacollec_t.main_ip == 0 && sg_datacollec_t.main_sub_ip == 0) 
	{
		if(main_ip[0] == 1 || main_ip[1] == 1) 
		{
			main_ip[0] = sg_datacollec_t.main_ip;
			main_ip[1] = sg_datacollec_t.main_sub_ip;
			return 1;
		}
	}
	else if(sg_datacollec_t.main_ip == 1 || sg_datacollec_t.main_sub_ip == 1) 
	{
		if(main_ip[0] == 0 && main_ip[1] == 0) 
		{
			main_ip[0] = sg_datacollec_t.main_ip;
			main_ip[1] = sg_datacollec_t.main_sub_ip;
			return 1;
		}	
	}
	main_ip[0] = sg_datacollec_t.main_ip;
	main_ip[1] = sg_datacollec_t.main_sub_ip;
	
	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: det_get_temphumi_function
*	功能说明: 检测温湿度
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_get_temphumi_function(void)
{
	double det_temp = 0;
	double det_humi = 0;
	static uint8_t th_count_time = 0;
	
	th_count_time++;
	if(th_count_time >= 100)  // 2s 2000/20 = 100
	{
		th_count_time = 0;
	  if(aht20_measure(&det_humi,&det_temp) == 0) {
			sg_datacollec_t.humi_inside[0] = det_humi;
			sg_datacollec_t.temp_inside[0] = det_temp;
		}
	}
}


/*
*********************************************************************************************************
*	函 数 名: lean_check
*	功能说明: 角度计算
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
float lean_check(MENS_XYZ_STATUS_T *acc_xyz)
{
	float ax_ref = 0;
	float ay_ref = 0;
	float az_ref = 1;
	float norm_ref = 1;
	
	double temp = 0;
	double norm = 0;
	double dot_norm = 0;
//	bool Ret = false;
	float ax,ay,az,dot; 
	MENS_XYZ_STATUS_T tmp;
	static MENS_XYZ_STATUS_T acc;
	float angle;
	
	#if 1
	/*去除加速度计抖动,30mg变化视为抖动*/	
	if(abs(acc_xyz->AcceX-acc.AcceX)>=30){
		tmp.AcceX = acc_xyz->AcceX;
		acc.AcceX = acc_xyz->AcceX;
	}else{
		tmp.AcceX = acc.AcceX;
	}
	if(abs(acc_xyz->AcceY-acc.AcceY)>=30){
		tmp.AcceY = acc_xyz->AcceY;
		acc.AcceY = acc_xyz->AcceY;
	}else{
		tmp.AcceY = acc.AcceY;
	}

	if(abs(acc_xyz->AcceZ-acc.AcceZ)>=30){
		tmp.AcceZ = acc_xyz->AcceZ;
		acc.AcceZ = acc_xyz->AcceZ;
	}else{
		tmp.AcceZ = acc.AcceZ;
	}
	#endif

	/*归一化处理*/
	temp = tmp.AcceX*tmp.AcceX + tmp.AcceY*tmp.AcceY + tmp.AcceZ*tmp.AcceZ;
	norm = (float)sqrt(temp);
	if (norm == 0.0) norm = 0.000001;  //avoid Nan happen
	ax = tmp.AcceX / norm;
	ay = tmp.AcceY / norm;
	az = tmp.AcceZ / norm;

	/*检测倾角是否超限40°*/

	/*ax_ref,ay_ref,az_ref
	为归一化后的参考向量的分量，可以自己设置，如定义为ax_ref=0,ay_ref=0,az_ref=1，表示加速度计正立放置
	也可以让终端程序稳定运行一段时间后，取一个加速度值作为参考向量，这样就可以不论物体怎么摆放，当他稳定后就可以确定初始向量*/
	dot = ax*ax_ref+ay*ay_ref+az*az_ref;
	
	dot_norm = (float)sqrt((double)(ax*ax) + (double)(ay*ay) + (double)(az*az));
	if (dot_norm == 0.0) dot_norm = 0.000001;
	//针对不少人的疑问:norm_ref和dot_norm 一样是归一化后的向量的模，norm_ref是参考的向量而已,可以在开机时记录，或者在某个特定时刻指定,计算公式参考dot_norm ,需要注意的是参考向量也需要归一化
	angle = acos(dot/(dot_norm*norm_ref))*180/3.14;//弧度转化为角度，
	
	return angle;
}

/*
*********************************************************************************************************
*	函 数 名: det_get_attitude_state_value
*	功能说明: 检测陀螺仪姿态值
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_get_attitude_state_value(void)
{
#ifdef ANGLE_ENABLE
	MENS_XYZ_STATUS_T mens_t;
	short x = 0;
	short y = 0;
	short z = 0;
	
	hal_lis3dh_get_xyz(&x, &y, &z);
	
	/* 计算姿态值，并且判断箱体状态 */
	mens_t.AcceX = x;
	mens_t.AcceY = y;
	mens_t.AcceZ = z;
	sg_datacollec_t.attitude_acc = lean_check(&mens_t);

#endif
}

/*
*********************************************************************************************************
*	函 数 名: det_get_attitude_state_value
*	功能说明: 获取温度
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
fp32 det_get_inside_temp(uint8_t id)
{
	return (sg_datacollec_t.temp_inside[id]);
}

/*
*********************************************************************************************************
*	函 数 名: det_get_inside_humi
*	功能说明: 检测内部湿度
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
fp32 det_get_inside_humi(uint8_t id)
{
	return (sg_datacollec_t.humi_inside[id]);
}

/*
*********************************************************************************************************
*	函 数 名: det_get_vin220v_handler
*	功能说明: 获取220V电压输入参数:电压、电流
*	形    参: 
*	@num		: 0:220V电压 1:220电流
*	返 回 值: 无
*********************************************************************************************************
*/
fp32 det_get_vin220v_handler(uint8_t num)
{
	switch(num)
	{
		case 0: return sg_datacollec_t.vin_voltage;
		case 1: return sg_datacollec_t.vin_current; 
		case 2: return sg_datacollec_t.current[0];
		case 3: return sg_datacollec_t.current[1];
		case 4: return sg_datacollec_t.current[2];
		case 5: return sg_datacollec_t.current[3];
	}
	return  0;
}

/*
*********************************************************************************************************
*	函 数 名: det_get_cabinet_posture
*	功能说明: 获取箱体姿态
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint16_t det_get_cabinet_posture(void)
{
	return sg_datacollec_t.attitude_acc;
}

/*
*********************************************************************************************************
*	函 数 名: det_set_main_network_status
*	功能说明: 主网1 状态设置
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_set_main_network_status(uint8_t status)
{
	sg_datacollec_t.main_ip = status;
}

/*
*********************************************************************************************************
*	函 数 名: det_get_main_network_status
*	功能说明: 主网1 状态获取
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t det_get_main_network_status(void)
{
	return sg_datacollec_t.main_ip;
}

/*
*********************************************************************************************************
*	函 数 名: det_set_main_network_sub_status
*	功能说明: 主网2 状态设置
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_set_main_network_sub_status(uint8_t status)
{
	sg_datacollec_t.main_sub_ip = status;
}

/*
*********************************************************************************************************
*	函 数 名: det_get_main_network_sub_status
*	功能说明: 主网2 状态获取
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t det_get_main_network_sub_status(void)
{
	return sg_datacollec_t.main_sub_ip;
}

/*
*********************************************************************************************************
*	函 数 名: det_set_total_energy_bl0906
*	功能说明: 计算BL0906电量参数
*	形    参: 
*	@num		: 通道
*	@data		: 数据
*	返 回 值: 无
*********************************************************************************************************
*/
void det_set_total_energy_bl0906(uint8_t num,float data)
{
	switch(num)
	{
		case 0: sg_datacollec_t.current[0] = data / BL0906_CURR_KP; 		break;
		case 1: sg_datacollec_t.current[1] = data / BL0906_CURR_KP; 		break;
		case 2: sg_datacollec_t.current[2] = data / BL0906_CURR_KP; 		break;
		case 3: sg_datacollec_t.current[3] = data / BL0906_CURR_KP; 		break;
		case 4: sg_datacollec_t.residual_c[0] = data / BL0906_CURR_KP; 		break;
		case 5: sg_datacollec_t.residual_c[1] = data / BL0906_CURR_KP; 		break;
  }
}

/*
*********************************************************************************************************
*	函 数 名: det_set_total_energy_bl0939
*	功能说明: 计算BL0939电量参数
*	形    参: 
*	@num		: 通道
*	@data		: 数据
*	返 回 值: 无
*********************************************************************************************************
*/
void det_set_total_energy_bl0939(uint8_t num,float data)
{
	switch(num)
	{
		case 0: sg_datacollec_t.vin_voltage = data / BL0939_VOLT_KP;   break;
		case 1: sg_datacollec_t.vin_current = data / BL0939_CURR_KP ; 	break;
  }
}

/*
*********************************************************************************************************
*	函 数 名: det_set_ping_status
*	功能说明: 设置ping状态
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_set_ping_status(uint8_t status)
{
	sg_datacollec_t.ping_status = status;
}

/*
*********************************************************************************************************
*	函 数 名: det_set_key_value
*	功能说明: 设置按键数值
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_set_key_value(uint8_t key_id,uint8_t key_value)
{
  sg_datacollec_t.key_s[key_id] = key_value;
}

/*
*********************************************************************************************************
*	函 数 名: det_get_door_status
*	功能说明: 获取箱门状态
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t det_get_door_status(uint8_t id)
{
	return sg_datacollec_t.key_s[id + 2];
}
/*
*********************************************************************************************************
*	函 数 名: det_get_pwr_status
*	功能说明: 获取适配器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t det_get_pwr_status(void)
{
	return sg_datacollec_t.key_s[1];
}

/*
*********************************************************************************************************
*	函 数 名: det_get_water_status
*	功能说明: 获取浸水状态
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t det_get_water_status(uint8_t id)
{
	return sg_datacollec_t.key_s[id + 6];
}

/*
*********************************************************************************************************
*	函 数 名: det_get_gps_value
*	功能说明: 获取GPS数值
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void det_get_gps_value(void)
{
  atgm336h_decode_nmea_xxgga();
	
}

/*
*********************************************************************************************************
*	函 数 名: det_get_miu_value
*	功能说明: 获取漏电数值
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t det_get_miu_value(uint8_t id)
{
	return sg_datacollec_t.residual_c[id];
}

/*
*********************************************************************************************************
*	函 数 名: det_get_mcb220_value
*	功能说明: 获取空开
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t det_get_mcb220_value(void)
{
	return sg_datacollec_t.key_s[8];
}
/*
*********************************************************************************************************
*	函 数 名: det_get_collect_data
*	功能说明: 获取数据信息
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void *det_get_collect_data(void)
{
	return (&sg_datacollec_t);
}

