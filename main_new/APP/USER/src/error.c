#include "./USER/inc/error.h"
#include "stdint.h"
#include "string.h"

// ===================== 第三步：定义各组的错误项数组（数量无上限） =====================
// ---------------------- 1. 电力组错误项（可无限新增） ----------------------
static const ErrorItem_t err_items_elec[] = {
    {ELEC_MAIN_AC			, "100", },
    {ELEC_ACDC_MODULE	, "101", },
    {ELEC_AC_OVER_V		, "102", },
    {ELEC_AC_LOW_V		, "103", },
    {ELEC_AC_OVER_C		, "104", },
    {ELEC_AC_LEAKAGE	, "105", },
    {ELEC_AC_MCB			, "106", },
    // ...
};
#define ERR_ELEC_ITEM_COUNT  (sizeof(err_items_elec) / sizeof(err_items_elec[0]))

static const ErrorItem_t err_items_net[] = {
    {NET_LAN_PORT	, "200",},
    {NET_MAIN_IP  , "201",},
    {NET_SINGLE_IP, "202",},
};
#define ERR_NET_ITEM_COUNT  (sizeof(err_items_net) / sizeof(err_items_net[0]))

static const ErrorItem_t err_items_sensor[] = {
    {SENSOR_TEMP_HIGH	, "300",},
    {SENSOR_TEMP_LOW  , "301",},
    {SENSOR_HUMI_HIGH	, "302",},
    {SENSOR_BOX_TILT  , "303",},
    {SENSOR_DOOR_OPEN , "304",},
    {SENSOR_WATER_LEAK, "305",},
    // ...
};
#define ERR_SENSOR_ITEM_COUNT  (sizeof(err_items_sensor) / sizeof(err_items_sensor[0]))

// ---------------------- 4. 信号灯组错误项 ----------------------
// 由于信号灯故障组合非常多，不使用固定数组，而是动态生成故障码
// 5位数字编码格式：ABCDE
// A: 类型 (0=远灯, 1=近灯)
// B: 方向 (0=北, 1=东, 2=南, 3=西)
// C: 相位 (0=左转, 1=直行, 2=右转, 3=行人1, 4=行人2, 5=非机动车1, 6=非机动车2, 7=倒计时, 8=可变车道, 9=待行, 0=辅道)
// D: 颜色 (0=红, 1=绿, 2=黄)
// E: 故障类型 (0=正常, 1=全不亮, 2=部分亮, 3=红绿同亮)

// 空的信号灯故障数组，仅用于占位
static const ErrorItem_t err_items_traffic[] = {
    // 仅作为占位符，实际故障码通过TrafficFault_GetCode动态生成
};
#define ERR_TRAFFIC_ITEM_COUNT  (sizeof(err_items_traffic) / sizeof(err_items_traffic[0]))

// 全局缓冲区，用于动态生成信号灯故障码
static char g_traffic_fault_code[7] = "400000"; // 7字节缓冲区，包含结束符
	
// ===================== 第四步：定义错误组数组（管理所有组） =====================
static const ErrorGroup_t error_groups[] = {
    // 组掩码              指向组内错误项数组      组内错误数量
    {1UL << ERR_TYPE_ELEC,   err_items_elec,    ERR_ELEC_ITEM_COUNT		},
    {1UL << ERR_TYPE_NET,    err_items_net,     ERR_NET_ITEM_COUNT		},
    {1UL << ERR_TYPE_SENSOR, err_items_sensor,  ERR_SENSOR_ITEM_COUNT	},
    {1UL << ERR_TYPE_TRAFFIC, err_items_traffic, ERR_TRAFFIC_ITEM_COUNT	},
    // 新增组只需追加一行...
};
#define ERROR_GROUP_COUNT  (sizeof(error_groups) / sizeof(error_groups[0]))

// ===================== 第五步：全局错误状态（双层管理） =====================
// 1. 组级错误状态（uint64_t，标记哪个组有错误，bit位操作）
uint32_t g_err_group_status = 0;
// 2. 组内错误状态（数组，存储每个组内的错误索引集合，用位图/列表均可）
// 方案：用uint32_t数组存储组内错误位图（每个组最多32个错误，超了可改用uint64_t/数组）
uint32_t g_err_item_status[ERR_MAX] = {0};

/*
*********************************************************************************************************
*	函 数 名: Error_Set
*	功能说明: 标记错误发生（支持任意数量错误）
*	形    参: 
* @param group: 错误组（ERR_GROUP_ELEC等）
* @param item_idx: 组内错误索引
*	返 回 值: 无
*********************************************************************************************************
*/
void Error_Set(ErrorType_e group, uint32_t item_idx)
{
	if (group >= ERR_MAX) return;
	
	// 特殊处理信号灯故障组，允许任意有效的错误索引
	if (group != ERR_TYPE_TRAFFIC && item_idx >= error_groups[group].item_count) return;

	g_err_item_status[group] |= (1UL << item_idx);      // 单组用uint32_t直接置位
	g_err_group_status |= error_groups[group].group_mask;
}

/*
*********************************************************************************************************
*	函 数 名: Error_Clear
*	功能说明: 清除指定错误
*	形    参: 
* @param group: 错误组（ERR_GROUP_ELEC等）
* @param item_idx: 组内错误索引
*	返 回 值: 无
*********************************************************************************************************
*/
void Error_Clear(ErrorType_e group, uint32_t item_idx)
{
	if (group >= ERR_MAX) return;
	
	// 特殊处理信号灯故障组，允许任意有效的错误索引
	if (group != ERR_TYPE_TRAFFIC && item_idx >= error_groups[group].item_count) return;
	
	g_err_item_status[group] &= ~(1UL << item_idx);
	// 组内无错误则清零组级状态
	if (g_err_item_status[group] == 0) {
		g_err_group_status &= ~error_groups[group].group_mask;
	}
}

/*
*********************************************************************************************************
*	函 数 名: Error_Check
*	功能说明: 查询错误是否发生
*	形    参: 
* @param group: 错误组
* @param item_idx: 组内错误索引
*	返 回 值: 1-发生，0-未发生
*********************************************************************************************************
*/
uint8_t Error_Check(ErrorType_e group, uint32_t item_idx)
{
	if (group >= ERR_MAX) return 0;
	if ((g_err_group_status & error_groups[group].group_mask) == 0) return 0;
	
	// 特殊处理信号灯故障组，允许任意有效的错误索引
	if (group != ERR_TYPE_TRAFFIC && item_idx >= error_groups[group].item_count) return 0;
	
	return (g_err_item_status[group] & (1UL << item_idx)) ? 1 : 0;
}


/**
 * @brief 获取错误码
 */
/*
*********************************************************************************************************
*	函 数 名: Error_GetCode
*	功能说明: 获取错误码
*	形    参: 
* @param group: 错误组
* @param item_idx: 组内错误索引
*	返 回 值: 错误数量
*********************************************************************************************************
*/
const char* Error_GetCode(ErrorType_e group, uint8_t item_idx)
{
	if (group >= ERR_MAX) return NULL;
	
	// 特殊处理信号灯故障组，动态生成错误码
	if (group == ERR_TYPE_TRAFFIC) {
		// 从item_idx中解析出类型、方向、相位、颜色
		uint8_t type = (item_idx >> 8) & 0x01;
		uint8_t dir = (item_idx >> 6) & 0x03;
		uint8_t phase = (item_idx >> 2) & 0x0F;
		uint8_t color = item_idx & 0x03;
		
		// 默认为全不亮故障
		uint8_t fault = 1;
		
		// 生成5位错误码：4ABCDE
		g_traffic_fault_code[0] = '4';
		g_traffic_fault_code[1] = type + '0';
		g_traffic_fault_code[2] = dir + '0';
		g_traffic_fault_code[3] = (phase % 10) + '0';
		g_traffic_fault_code[4] = color + '0';
		g_traffic_fault_code[5] = fault + '0';
		g_traffic_fault_code[6] = '\0';
		
		return g_traffic_fault_code;
	}
	
	// 其他组使用静态错误码
	if (item_idx >= error_groups[group].item_count) return NULL;
	return error_groups[group].items[item_idx].err_code;
}

/*
*********************************************************************************************************
*	函 数 名: Error_GetAllCodes
*	功能说明: 遍历所有错误，拼接错误码为逗号分隔的字符串（核心精简版）
*	形    参: 
 * @param buf: 输出缓冲区
 * @param buf_len: 缓冲区长度
*	返 回 值: 错误数量；-1=参数无效
*********************************************************************************************************
*/
int8_t Error_GetAllCodes(char* buf, uint16_t buf_len)
{
	if (buf == NULL || buf_len < 2) return -1;
	
	memset(buf, 0, buf_len);
	uint8_t err_count = 0;
	uint16_t buf_pos = 0;
    
	// 遍历所有组
	for (uint8_t group = 0; group < ERR_MAX; group++) 
	{
		if ((g_err_group_status & error_groups[group].group_mask) == 0) 
			continue;
			
		// 遍历组内错误（0~31）
		const ErrorGroup_t* curr_group = &error_groups[group];
		for (uint8_t idx = 0; idx < curr_group->item_count; idx++) 
		{
			if (Error_Check((ErrorType_e)group, idx)) 
			{
				const char* code = Error_GetCode((ErrorType_e)group, idx);
				if (code == NULL) continue;
                
				// 防止缓冲区溢出
				uint8_t code_len = strlen(code);
				if (buf_pos + code_len + 1 >= buf_len)
					break;
                
				// 非首个错误加逗号
				if (err_count > 0) buf[buf_pos++] = ',';
				
				// 复制错误码
				strncpy(&buf[buf_pos], code, code_len);
				buf_pos += code_len;
				err_count++;
			}
		}
	}
	buf[buf_pos] = '\0'; // 确保字符串结尾
	return err_count;
}


/*********************************************************************************************************
* 信号灯故障管理函数实现
*********************************************************************************************************/

/**
 * @brief 标记信号灯故障
 */
uint8_t TrafficFault_Set(uint8_t type, uint8_t dir, uint8_t phase, uint8_t color, uint8_t fault)
{
	// 参数有效性检查
	if (type > 1 || dir > 3 || phase > 10 || color > 2 || fault > 3) {
		return 0;
	}
	
	// 生成错误索引
	uint32_t fault_index = (((type) & 0x01) << 8) | (((dir) & 0x03) << 6) | (((phase) & 0x0F) << 2) | ((color) & 0x03);
	
	// 调用现有的错误设置函数
	Error_Set(ERR_TYPE_TRAFFIC, fault_index);
	
	return 1;
}

/**
 * @brief 清除信号灯故障
 */
uint8_t TrafficFault_Clear(uint8_t type, uint8_t dir, uint8_t phase, uint8_t color, uint8_t fault)
{
	// 参数有效性检查
	if (type > 1 || dir > 3 || phase > 10 || color > 2 || fault > 3) {
		return 0;
	}
	
	// 生成错误索引
	uint32_t fault_index = (((type) & 0x01) << 8) | (((dir) & 0x03) << 6) | (((phase) & 0x0F) << 2) | ((color) & 0x03);
	
	// 调用现有的错误清除函数
	Error_Clear(ERR_TYPE_TRAFFIC, fault_index);
	
	return 1;
}

/**
 * @brief 检查信号灯故障
 */
uint8_t TrafficFault_Check(uint8_t type, uint8_t dir, uint8_t phase, uint8_t color, uint8_t fault)
{
	// 参数有效性检查
	if (type > 1 || dir > 3 || phase > 10 || color > 2 || fault > 3) {
		return 0;
	}
	
	// 生成错误索引
	uint32_t fault_index = (((type) & 0x01) << 8) | (((dir) & 0x03) << 6) | (((phase) & 0x0F) << 2) | ((color) & 0x03);
	
	// 调用现有的错误检查函数
	return Error_Check(ERR_TYPE_TRAFFIC, fault_index);
}

/**
 * @brief 获取信号灯故障错误码
 */
const char* TrafficFault_GetCode(uint8_t type, uint8_t dir, uint8_t phase, uint8_t color, uint8_t fault)
{
	// 参数有效性检查
	if (type > 1 || dir > 3 || phase > 10 || color > 2 || fault > 3) {
		return NULL;
	}
	
	// 直接生成5位错误码：4ABCDE
	g_traffic_fault_code[0] = '4';
	g_traffic_fault_code[1] = type + '0';
	g_traffic_fault_code[2] = dir + '0';
	g_traffic_fault_code[3] = (phase % 10) + '0';
	g_traffic_fault_code[4] = color + '0';
	g_traffic_fault_code[5] = fault + '0';
	g_traffic_fault_code[6] = '\0';
	
	return g_traffic_fault_code;
}

/**
 * @brief 清除所有信号灯故障
 */
uint8_t TrafficFault_ClearAll(void)
{
	// 直接清除信号灯故障组的所有错误
	g_err_item_status[ERR_TYPE_TRAFFIC] = 0;
	g_err_group_status &= ~error_groups[ERR_TYPE_TRAFFIC].group_mask;
	
	return 1;
}

/**
 * @brief 获取信号灯故障数量
 */
uint8_t TrafficFault_GetCount(void)
{
	uint8_t count = 0;
	uint32_t status = g_err_item_status[ERR_TYPE_TRAFFIC];
	
	// 统计状态位中置1的位数
	while (status) {
		count += status & 1;
		status >>= 1;
	}
	
	return count;
}

