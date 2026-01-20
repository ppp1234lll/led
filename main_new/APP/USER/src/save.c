#include "appconfig.h"

#define SAVE_LOCAL_NETWORK_NAME     ("network_name")      /* 本地网络信息 */
#define SAVE_REMOTE_NETWORK_NAME    ("remote_name")       /* 远端网络信息 */
#define SAVE_REMOTE_BACKUPS_NAME    ("backups")           /* 远端网络——备份 20231023*/ 
#define SAVE_DEVICE_PARAMETER_NAME  ("device_name")       /* 设备详细:id、名称、密码等 */
#define SAVE_COM_PARAMETER_NAME     ("comparameter")      /* 通信相关参数 */
#define SAVE_HTTP_UPDATE_ADDR_NAME  ("HTTP_OTA")          /* 更新地址 */
#define SAVE_THRESHOLD_PARAMETER    ("threshold_params")  /* 相关阈值：电压 电流 角度 */ // 20230720
#define SAVE_UPDATE_FILE_INFOR_NAME ("upfileinfor.bin")   /* 更新文件信息 */
#define SAVE_ELECTRICITY_PARAM      ("electricity")           /* 用电量信息，防止重启后用电量变为0 */

/*
*********************************************************************************************************
*	函 数 名: save_init_function
*	功能说明: 存储功能初始化函数
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void save_init_function(void)
{
	lfs_init_function();     						// 挂载文件系统
}

/*
*********************************************************************************************************
*	函 数 名: save_clear_file_function
*	功能说明: 恢复出厂化
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void save_clear_file_function(uint8_t mode)
{
	if(mode == 0)
	{
		lfs_remove(&g_lfs_t,SAVE_LOCAL_NETWORK_NAME);
		lfs_remove(&g_lfs_t,SAVE_REMOTE_NETWORK_NAME);
		lfs_remove(&g_lfs_t,SAVE_DEVICE_PARAMETER_NAME);
		lfs_remove(&g_lfs_t,SAVE_COM_PARAMETER_NAME);
		lfs_remove(&g_lfs_t,SAVE_THRESHOLD_PARAMETER);  //	20230720
		lfs_remove(&g_lfs_t,SAVE_HTTP_UPDATE_ADDR_NAME);  //	20230720	
	}
	else if(mode == 1)
	{
		lfs_remove(&g_lfs_t,SAVE_LOCAL_NETWORK_NAME);
	} 
	else if(mode == 2) 
	{
    lfs_remove(&g_lfs_t,SAVE_UPDATE_FILE_INFOR_NAME);
  }
}

/*
*********************************************************************************************************
*	函 数 名: save_stroage_local_network
*	功能说明: 存储本地网络参数
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
int8_t save_stroage_local_network(struct local_ip_t *local)
{
	int8_t		ret      = 0;
 	int 		err 	 = 0;
	lfs_file_t  lfs_fp   = {0};
	
	/* 数据保存 */
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_LOCAL_NETWORK_NAME, LFS_O_RDWR | LFS_O_CREAT);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)local, sizeof(struct local_ip_t));
		if(err != sizeof(struct local_ip_t)) {
			err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)local, sizeof(struct local_ip_t));
		}
	}
	else
	{
		ret = -1;
	}
	err = lfs_file_close(&g_lfs_t, &lfs_fp);
	
	return ret;
	
}

/************************************************************
*
* Function name	: save_read_local_network
* Description	: 读取本地网络设置
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_read_local_network(struct local_ip_t *local)
{
	int8_t ret  = 0;
	int    err  = 0;
	lfs_file_t  lfs_fp   = {0};
	
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_LOCAL_NETWORK_NAME, LFS_O_RDWR);
	
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_read(&g_lfs_t, &lfs_fp, local,sizeof(struct local_ip_t));
		err = lfs_file_close(&g_lfs_t, &lfs_fp);
	}
	else
	{
		save_read_default_local_network(local);
		save_stroage_local_network(local);
		ret = -1;
	}
	return ret;
}

/************************************************************
*
* Function name	: save_read_default_local_network
* Description	: 读取默认参数
* Parameter		: 
* Return		: 
*	
************************************************************/
void save_read_default_local_network(struct local_ip_t *local)
{
	uint32_t data= 0;
	uint8_t  mac[6] = {0}; 			// MAC地址 
	uint8_t  ret = 0;
	extern ChipID_t g_chipid_t;
	data = g_chipid_t.id[0];
	
	/* 本机ip地址 */
	local->ip[0] = DEFALUT_LOCAL_IP0;
	local->ip[1] = DEFALUT_LOCAL_IP1;
	local->ip[2] = DEFALUT_LOCAL_IP2;
	local->ip[3] = DEFALUT_LOCAL_IP3;

	/* 本机MAC */
	bsp_ReadCpuFlash(DEVICE_MAC_ADDR, (uint8_t *)mac, 6);
	for(uint8_t i=0;i<6;i++)
	{
		if(mac[i] == 0xFF)
			ret++;
	}
	if(ret == 6)
	{
		local->mac[0]=2;//高三字节(IEEE称之为组织唯一ID,OUI)地址固定为:2.0.0
		local->mac[1]=0;
		local->mac[2]=0;
		local->mac[3]=(data>>16)&0XFF;//低三字节用STM32的唯一ID
		local->mac[4]=(data>>8)&0XFF;
		local->mac[5]=data&0XFF; 	
	}	
	else
	{
		local->mac[0]=mac[0];
		local->mac[1]=mac[1];
		local->mac[2]=mac[2];
		local->mac[3]=mac[3];
		local->mac[4]=mac[4];
		local->mac[5]=mac[5]; 
	}
	cpuflash_write_save(DEVICE_FLASH_STORE,DEVICE_MAC_ADDR,(uint8_t *)&local->mac,6);		

	/* 本机子网掩码 */
	local->netmask[0]=DEFALUT_NETMASK0;	
	local->netmask[1]=DEFALUT_NETMASK1;
	local->netmask[2]=DEFALUT_NETMASK2;
	local->netmask[3]=DEFALUT_NETMASK3;
	
	/* 本机默认网关 */
	local->gateway[0]=DEFALUT_GATEWAY0;	
	local->gateway[1]=DEFALUT_GATEWAY1;
	local->gateway[2]=DEFALUT_GATEWAY2;
	local->gateway[3]=DEFALUT_GATEWAY3;	
	
	/* 本机DNS */
	local->dns[0] = DEFALUT_DNS0;
	local->dns[1] = DEFALUT_DNS1;
	local->dns[2] = DEFALUT_DNS2;
	local->dns[3] = DEFALUT_DNS3;
	
	local->server_mode = DEFALUT_SERVERMODE;  
	
	/* 组播地址 */
	local->multicast_ip[0] = DEFALUT_MULTICAST_IP0;
	local->multicast_ip[1] = DEFALUT_MULTICAST_IP1;
	local->multicast_ip[2] = DEFALUT_MULTICAST_IP2;
	local->multicast_ip[3] = DEFALUT_MULTICAST_IP3;

	local->multicast_port = DEFALUT_MULTICAST_PORT;
	memset(local->ping_ip,0,sizeof(local->ping_ip));
	memset(local->ping_sub_ip,0,sizeof(local->ping_sub_ip));
	
	local->search_mode = 1; // PING模式
	
	memset(local->single_ip,0,sizeof(local->single_ip));
}

/************************************************************
*
* Function name	: save_stroage_remote_ip_function
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_stroage_remote_ip_function(struct remote_ip *remote)
{
	int8_t  ret      = 0;
 	int 		err 	 = 0;
	lfs_file_t  lfs_fp	 = {0};
	
	/* 数据保存 */
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_REMOTE_NETWORK_NAME, LFS_O_RDWR | LFS_O_CREAT);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)remote, sizeof(struct remote_ip));
		if(err != sizeof(struct remote_ip)) {
			err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)remote, sizeof(struct remote_ip));
		}
	}
	else
	{
		ret = -1;
	}
	err = lfs_file_close(&g_lfs_t, &lfs_fp);
	
	return ret;
}

/************************************************************
*
* Function name	: save_read_remote_ip_function
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_read_remote_ip_function(struct remote_ip *remote)
{
	int8_t		ret      = 0;
	int 		err 	 = 0;
	lfs_file_t  lfs_fp   = {0};
	
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_REMOTE_NETWORK_NAME, LFS_O_RDWR);

	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_read(&g_lfs_t, &lfs_fp, remote,sizeof(struct remote_ip));
		err = lfs_file_close(&g_lfs_t, &lfs_fp);
	}
	else
	{
		save_read_default_remote_ip(remote);
		save_stroage_remote_ip_function(remote);
		ret = -1;
	}
	return ret;
}

/************************************************************
*
* Function name	: save_read_default_remote_ip
* Description	: 读取默认值
* Parameter		: 
* Return		: 
*	
************************************************************/
void save_read_default_remote_ip(struct remote_ip *remote)
{
	/* 远程服务器数据 */
	memset(remote->outside_iporname,0,sizeof(remote->outside_iporname));
	strcpy((char*)remote->outside_iporname,"47.104.250.225");
	remote->outside_port  = 6012;

	memset(remote->inside_iporname,0,sizeof(remote->inside_iporname));
	strcpy((char*)remote->inside_iporname,"47.104.250.225");
	remote->inside_port  = 6012;
}

/************************************************************
*
* Function name	: save_storage_device_parameter_function
* Description	: 存储设备相关参数：ID、密码等
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_storage_device_parameter_function(struct device_param *param)
{
	int8_t  ret  = 0;
 	int 		err  = 0;
	lfs_file_t  lfs_fp   = {0};
	
	/* 数据保存 */
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_DEVICE_PARAMETER_NAME, LFS_O_RDWR | LFS_O_CREAT);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(struct device_param));
		if(err != sizeof(struct device_param)) {
			err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(struct device_param));
		}
	}
	else
	{
		ret = -1;
	}
	err = lfs_file_close(&g_lfs_t, &lfs_fp);
	
	return ret;
	
}

/************************************************************
*
* Function name	: save_read_default_device_paramter_function
* Description	: 读取设备相关参数：ID、密码等
* Parameter		: 
* Return		: 
*	
************************************************************/
void save_read_default_device_paramter_function(struct device_param *param)
{
	union i_c data_id;		  		// id
	
	bsp_ReadCpuFlash(DEVICE_ID_ADDR,(uint8_t*)data_id.c,4);
	if((data_id.i >= 0xFFFFF)||(data_id.i == 0))
	{
		param->id.i = 3;
	}	
	else
	{
		param->id.i = data_id.i;
	}
	cpuflash_write_save(DEVICE_FLASH_STORE,DEVICE_ID_ADDR,(uint8_t*)param->id.c,1);
	
	memset(param->name,0,sizeof(param->name));
	memset(param->password,0,sizeof(param->password));
	strcpy((char*)param->password,DEFALUT_PASSWORD);
	param->default_password = 1; // 默认开机需要修改密码
}

/************************************************************
*
* Function name	: save_read_device_paramter_function
* Description	: 读取设备相关参数：ID、密码等
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_read_device_paramter_function(struct device_param *param)
{
	int8_t  ret  = 0;
	int 		err  = 0;
	lfs_file_t  lfs_fp   = {0};
	
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_DEVICE_PARAMETER_NAME, LFS_O_RDWR);

	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_read(&g_lfs_t, &lfs_fp, param,sizeof(struct device_param));
		err = lfs_file_close(&g_lfs_t, &lfs_fp);
	}
	else
	{
		save_read_default_device_paramter_function(param);
		save_storage_device_parameter_function(param);
		ret = -1;
	}
	return ret;
}

/************************************************************
*
* Function name	: save_stroage_com_param_function
* Description	: 存储通信相关参数
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_stroage_com_param_function(com_param_t *param)
{
	int8_t  ret  = 0;
 	int 		err  = 0;
	lfs_file_t  lfs_fp   = {0};
	
	/* 数据保存 */
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_COM_PARAMETER_NAME, LFS_O_RDWR | LFS_O_CREAT);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(com_param_t));
		if(err != sizeof(com_param_t)) {
			err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(com_param_t));
		}
	}
	else
	{
		ret = -1;
	}
	err = lfs_file_close(&g_lfs_t, &lfs_fp);
	
	return ret;
}

/************************************************************
*
* Function name	: 
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
void save_read_default_com_param_function(com_param_t *param)
{
	param->heart  			= DEFALUT_HEART; // 90s
	param->report 			= DEFALUT_REPORT; // 60s
	param->ping   			= DEFALUT_PING;
	param->dev_ping 		= DEFALUT_DEV_PING;
	param->network_time = DEFALUT_NETWORK_DELAY;  // 网络延时时间  20220308
}

/************************************************************
*
* Function name	: save_read_com_param_function
* Description	: 读取通信相关参数
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_read_com_param_function(com_param_t *param)
{
	int8_t		ret      = 0;
	int 		err 	 = 0;
	lfs_file_t  lfs_fp   = {0};
	
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_COM_PARAMETER_NAME, LFS_O_RDWR);

	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_read(&g_lfs_t, &lfs_fp, param,sizeof(com_param_t));
		err = lfs_file_close(&g_lfs_t, &lfs_fp);
	}
	else
	{
		save_read_default_com_param_function(param);
		save_stroage_com_param_function(param);
		ret = -1;
	}
	return ret;
}

/************************************************************
*
* Function name	: save_stroage_threshold_parameter
* Description	: 
* Parameter		: 
* Return		: 
*	        20230720
************************************************************/
int8_t save_stroage_threshold_parameter(struct threshold_params *param)
{
	int8_t	ret    = 0;
 	int 		err 	 = 0;
	lfs_file_t  lfs_fp   = {0};
	
	/* 数据保存 */
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_THRESHOLD_PARAMETER, LFS_O_RDWR | LFS_O_CREAT);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(struct threshold_params));
		if(err != sizeof(struct threshold_params)) 
		{
			err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(struct threshold_params));
		}
	}
	else
	{
		ret = -1;
	}
	err = lfs_file_close(&g_lfs_t, &lfs_fp);
	return ret;
	
}

/************************************************************
*
* Function name	: save_read_default_carema_parameter
* Description	: 读取默认参数
* Parameter		: 
* Return		: 
*	   20230720
************************************************************/
void save_read_default_threshold_parameter(struct threshold_params *param)
{
	param->volt_max 	 		= DEFALUT_VOLT_MAX;
	param->volt_min 	 		= DEFALUT_VOLT_MIN;
	param->current 		 		= DEFALUT_CURRENT_MAX;
	param->angle  		 		= DEFAULT_ANGLE;
	param->humi_high 			= DEFALUT_HUMI_HIGH;
	param->humi_low 			= DEFAULT_HUMI_LOW;
	param->temp_high 			= DEFALUT_TEMP_HIGH;
	param->temp_low 			= DEFALUT_TEMP_LOW;
	param->miu  	 		    = DEFAULT_MIU;
	param->led_not_bright 	 	= DEFALUT_LED_NOT_BRIGHT;
	param->led_part_bright 	 	= DEFALUT_LED_PART_BRIGHT;
}

/************************************************************
*
* Function name	: save_read_threshold_parameter
* Description	: 
* Parameter		: 
* Return		: 
*	   20230720
************************************************************/
int8_t save_read_threshold_parameter(struct threshold_params *param)
{
	int8_t ret  = 0;
	int    err  = 0;
	lfs_file_t  lfs_fp   = {0};
	
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_THRESHOLD_PARAMETER, LFS_O_RDWR);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_read(&g_lfs_t, &lfs_fp, param,sizeof(struct threshold_params));
		err = lfs_file_close(&g_lfs_t, &lfs_fp);
	}
	else
	{
		save_read_default_threshold_parameter(param);
		save_stroage_threshold_parameter(param);
		ret = -1;
	}
	return ret;
}

/************************************************************
*
* Function name	: save_stroage_backups_function
* Description	: 
* Parameter		: 
* Return		: 
*	  20231022
************************************************************/
int8_t save_stroage_backups_function(sys_backups_t *param)
{
	int8_t  ret  = 0;
 	int     err  = 0;
	lfs_file_t  lfs_fp	 = {0};
	
	/* 数据保存 */
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_REMOTE_BACKUPS_NAME, LFS_O_RDWR | LFS_O_CREAT);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(sys_backups_t));
		if(err != sizeof(sys_backups_t)) {
			err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(sys_backups_t));
		}
	}
	else
	{
		ret = -1;
	}
	err = lfs_file_close(&g_lfs_t, &lfs_fp);
	return ret;
}

/************************************************************
*
* Function name	: save_read_backups_function
* Description	: 
* Parameter		: 
* Return		: 
*	 20231022
************************************************************/
int8_t save_read_backups_function(sys_backups_t *param)
{
	int8_t		ret  = 0;
	int 		err 	 = 0;
	lfs_file_t  lfs_fp   = {0};
	
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_REMOTE_BACKUPS_NAME, LFS_O_RDWR);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_read(&g_lfs_t, &lfs_fp, param,sizeof(sys_backups_t));
		err = lfs_file_close(&g_lfs_t, &lfs_fp);
	}
	else
	{
		save_read_default_backups(param);
		save_stroage_backups_function(param);	
		ret = -1;
	}
	return ret;
}

/************************************************************
*
* Function name	: save_read_default_backups
* Description	: 读取默认值
* Parameter		: 
* Return		: 
*	 20231022
************************************************************/
void save_read_default_backups(sys_backups_t *param)
{
	/* 远程服务器数据 */
	memset(param->remote.outside_iporname,0,sizeof(param->remote.outside_iporname));
	strcpy((char*)param->remote.outside_iporname,"test1.fnwlw.net");
	param->remote.outside_port  = 6102;
	
	memset(param->remote.inside_iporname,0,sizeof(param->remote.inside_iporname));
	strcpy((char*)param->remote.inside_iporname,"test1.fnwlw.net");
	param->remote.inside_port  = 6102;
	param->config_flag =0;
}


/************************************************************
*
* Function name	: save_stroage_http_ota_function
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_stroage_http_ota_function(struct update_addr *param)
{
	int8_t		ret      = 0;
 	int 		err 	 = 0;
	lfs_file_t  lfs_fp	 = {0};
	
	/* 数据保存 */
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_HTTP_UPDATE_ADDR_NAME, LFS_O_RDWR | LFS_O_CREAT);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(struct update_addr));
		if(err != sizeof(struct update_addr)) {
			err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)param, sizeof(struct update_addr));
		}
	}
	else
	{
		ret = -1;
	}
	err = lfs_file_close(&g_lfs_t, &lfs_fp);
	
	return ret;
}

/************************************************************
*
* Function name	: save_read_http_ota_function
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_read_http_ota_function(struct update_addr *param)
{
	int8_t		ret      = 0;
	int 		err 	 = 0;
	lfs_file_t  lfs_fp   = {0};
	
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_HTTP_UPDATE_ADDR_NAME, LFS_O_RDWR);

	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_read(&g_lfs_t, &lfs_fp, param,sizeof(struct update_addr));
		err = lfs_file_close(&g_lfs_t, &lfs_fp);
	}
	else
	{
		save_read_default_http_ota(param);
		save_stroage_http_ota_function(param);
		ret = -1;
	}
	return ret;
}

/************************************************************
*
* Function name	: save_read_default_http_ota
* Description	: 读取默认值
* Parameter		: 
* Return		: 
*	
************************************************************/
void save_read_default_http_ota(struct update_addr *param)
{
	memset(param->ip,0,sizeof(param->ip));
	param->ip[0] = 47;
	param->ip[1] = 104;
	param->ip[2] = 98;
	param->ip[3] = 214;
	param->port  = 8989;
}


/************************************************************
*
* Function name	: save_stroage_electricity_function
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_stroage_electricity_function(electricity_t param)
{
	int8_t  ret = 0;
 	int     err = 0;
	lfs_file_t  lfs_fp	 = {0};
	
	/* 数据保存 */
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_ELECTRICITY_PARAM, LFS_O_RDWR | LFS_O_CREAT);
	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)&param, sizeof(electricity_t));
		if(err != sizeof(electricity_t)) {
			err = lfs_file_write(&g_lfs_t, &lfs_fp, (uint8_t*)&param, sizeof(electricity_t));
		}
	}
	else
	{
		ret = -1;
	}
	err = lfs_file_close(&g_lfs_t, &lfs_fp);
	
	return ret;
}

/************************************************************
*
* Function name	: save_read_http_ota_function
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t save_read_electricity_function(electricity_t *param)
{
	int8_t		ret      = 0;
	int 		err 	 = 0;
	lfs_file_t  lfs_fp   = {0};
	
	err = lfs_file_open(&g_lfs_t, &lfs_fp, SAVE_ELECTRICITY_PARAM, LFS_O_RDWR);

	if(err == 0)
	{
		err = lfs_file_rewind(&g_lfs_t, &lfs_fp);
		err = lfs_file_read(&g_lfs_t, &lfs_fp, param,sizeof(electricity_t));
		err = lfs_file_close(&g_lfs_t, &lfs_fp);
	}
	else
	{
		save_read_default_electricity(param);
		save_stroage_electricity_function(*param);
		ret = -1;
	}
	return ret;
}
/************************************************************
*
* Function name	: save_read_default_electricity
* Description	: 读取默认值
* Parameter		: 
* Return		: 
*	
************************************************************/
void save_read_default_electricity(electricity_t *param)
{
	memset(param,0,sizeof(electricity_t));
}


