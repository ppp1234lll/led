#include "./USER/inc/error.h"
#include "stdbool.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"

// ===================== 第三步：定义各组的错误项数组（数量无上限） =====================
// ---------------------- 1. 电力组错误项（可无限新增） ----------------------
// 错误码格式：0xFTTTTTTT
// F: 故障类型 (1=电力, 2=网络, 3=传感器, 4=信号灯)
// TTTTTTT: 具体故障信息
static const ErrorItem_t err_items_elec[] = {
    {ELEC_NORMAL			, 0x10000000, },  // 正常
    {ELEC_MAIN_AC			, 0x10100000, },  // 电力主AC故障
    {ELEC_ACDC_MODULE	, 0x10200000, },  // AC/DC模块故障
    {ELEC_AC_OVER_V		, 0x10300000, },  // AC过压
    {ELEC_AC_LOW_V		, 0x10400000, },  // AC欠压
    {ELEC_AC_OVER_C		, 0x10500000, },  // AC过流
    {ELEC_AC_LEAKAGE	, 0x10600000, },  // AC漏电
    {ELEC_AC_MCB			, 0x10700000, },  // AC断路器故障
    // ...
};
#define ERR_ELEC_ITEM_COUNT  (sizeof(err_items_elec) / sizeof(err_items_elec[0]))

static const ErrorItem_t err_items_net[] = {
    {NET_NORMAL		, 0x20000000,},  // 正常
    {NET_LAN_PORT	, 0x20100000,},  // 局域网端口故障
    {NET_MAIN_IP  , 0x20200000,},  // 主IP故障
    {NET_SINGLE_IP, 0x20300000,},  // 单灯IP故障
};
#define ERR_NET_ITEM_COUNT  (sizeof(err_items_net) / sizeof(err_items_net[0]))

static const ErrorItem_t err_items_sensor[] = {
    {SENSOR_NORMAL		, 0x30000000,},  // 正常
    {SENSOR_TEMP_HIGH	, 0x30100000,},  // 温度过高
    {SENSOR_TEMP_LOW  , 0x30200000,},  // 温度过低
    {SENSOR_HUMI_HIGH	, 0x30300000,},  // 湿度过高
    {SENSOR_BOX_TILT  , 0x30400000,},  // 箱体倾斜
    {SENSOR_DOOR_OPEN , 0x30500000,},  // 门打开
    {SENSOR_WATER_LEAK, 0x30600000,},  // 漏水
    // ...
};
#define ERR_SENSOR_ITEM_COUNT  (sizeof(err_items_sensor) / sizeof(err_items_sensor[0]))

// ---------------------- 4. 信号灯组错误项 ----------------------
// 信号灯故障使用动态生成的错误码，格式：0x4FTTDPRC
// 4: 故障类型 (4=信号灯)
// F: 故障类型 (0=正常, 1=全不亮, 2=部分亮, 3=红绿同亮)
// T: 灯类型 (0=远灯, 1=近灯)
// D: 方向 (0=北, 1=东, 2=南, 3=西)
// P: 相位 (0=左转, 1=直行, 2=右转, 3=行人1, 4=行人2, 5=非机动车1, 6=非机动车2, 7=倒计时, 8=可变车道, 9=待行)
// R: 道路类型 (0=主道, 1=辅道)
// C: 颜色 (0=红, 1=绿, 2=黄)

// 空的信号灯故障数组，仅用于占位
static const ErrorItem_t err_items_traffic[] = {
    // 仅作为占位符，实际故障码通过TrafficFault_GetCode动态生成
};
#define ERR_TRAFFIC_ITEM_COUNT  (sizeof(err_items_traffic) / sizeof(err_items_traffic[0]))


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
// 1. 组级错误状态（使用布尔值数组，标记哪个组有错误）
uint8_t	g_err_group_status[ERR_MAX] = {false}; // 每组是否有错误

// 2. 组内错误状态（使用数组存储错误索引，不使用bit操作）
// 每组最多存储256个错误
uint32_t g_err_item_status[ERR_MAX][256] = {0}; // 存储错误索引
uint8_t g_err_item_count[ERR_MAX] = {0}; // 每组错误数量

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

	// 检查错误是否已存在
	for (uint8_t i = 0; i < g_err_item_count[group]; i++)
	{
		if (g_err_item_status[group][i] == item_idx)
		{
			return; // 错误已存在，无需重复添加
		}
	}
	
	// 添加新错误
	if (g_err_item_count[group] < 256)
	{
		g_err_item_status[group][g_err_item_count[group]] = item_idx;
		g_err_item_count[group]++;
		g_err_group_status[group] = true; // 标记组有错误
	}
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
	
	// 查找并清除错误
	for (uint8_t i = 0; i < g_err_item_count[group]; i++)
	{
		if (g_err_item_status[group][i] == item_idx)
		{
			// 移除错误（将最后一个错误移到当前位置）
			g_err_item_status[group][i] = g_err_item_status[group][g_err_item_count[group] - 1];
			g_err_item_count[group]--;
			break;
		}
	}
	
	// 组内无错误则清零组级状态
	if (g_err_item_count[group] == 0) {
		g_err_group_status[group] = false;
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
	if (!g_err_group_status[group]) return 0;
	
	// 特殊处理信号灯故障组，允许任意有效的错误索引
	if (group != ERR_TYPE_TRAFFIC && item_idx >= error_groups[group].item_count) return 0;
	
	// 查找错误是否存在
	for (uint8_t i = 0; i < g_err_item_count[group]; i++)
	{
		if (g_err_item_status[group][i] == item_idx)
		{
			return 1; // 错误存在
		}
	}
	
	return 0; // 错误不存在
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
*	返 回 值: 错误码（uint32_t），失败返回0
*********************************************************************************************************
*/
uint32_t Error_GetCode(ErrorType_e group, uint32_t item_idx)
{
	if (group >= ERR_MAX) return 0;
	
	// 特殊处理信号灯故障组，动态生成错误码
	if (group == ERR_TYPE_TRAFFIC) {
		// 从item_idx中解析出故障类型、灯类型、方向、道路类型、相位、颜色
		uint8_t fault = (item_idx >> 10) & 0xFF;
		uint8_t type = (item_idx >> 8) & 0x01;
		uint8_t dir = (item_idx >> 6) & 0x03;
		uint8_t road = (item_idx >> 5) & 0x01;
		uint8_t phase = (item_idx >> 1) & 0x0F;
		uint8_t color = item_idx & 0x01;
		
		// 生成信号灯错误码：0x4FTTDPRC
		return (0x40000000 | ((fault & 0xFF) << 26) | ((type & 0x01) << 24) | ((dir & 0x03) << 22) | ((road & 0x01) << 21) | ((phase & 0x0F) << 16) | ((color & 0x03) << 14));
	}
	
	// 其他组使用静态错误码
	if (item_idx >= error_groups[group].item_count) return 0;
	return error_groups[group].items[item_idx].err_code;
}

/*
*********************************************************************************************************
*	函 数 名: Error_GetAllCodes
*	功能说明: 遍历所有错误，返回错误码数组
*	形    参: 
 * @param codes: 输出错误码数组
 * @param max_count: 数组最大容量
*	返 回 值: 错误数量；-1=参数无效
*********************************************************************************************************
*/
int8_t Error_GetAllCodes(uint32_t* codes, uint16_t max_count)
{
	if (codes == NULL || max_count == 0) return -1;
	
	uint8_t err_count = 0;
    
	// 遍历所有组
	for (uint8_t group = 0; group < ERR_MAX; group++) 
	{
		if (!g_err_group_status[group]) 
			continue;
			
		// 遍历组内错误
		for (uint8_t i = 0; i < g_err_item_count[group]; i++) 
		{
			if (err_count >= max_count) break;
			
			uint32_t item_idx = g_err_item_status[group][i];
			uint32_t code = Error_GetCode((ErrorType_e)group, item_idx);
			if (code == 0) continue;
            
			// 存储错误码到数组
			codes[err_count++] = code;
		}
	}
	return err_count;
}


/*********************************************************************************************************
* 信号灯故障管理函数实现
*********************************************************************************************************/

/**
 * @brief 标记信号灯故障
 */
uint8_t TrafficFault_Set(uint8_t fault, uint8_t type, uint8_t dir, uint8_t road, uint8_t phase, uint8_t color)
{
	// 生成错误索引（所有故障类型都使用相同的格式）
	uint32_t fault_index = (((fault) & 0xFF) << 20) | (((type) & 0x0F) << 16) | (((dir) & 0x0F) << 12) | (((road) & 0x0F) << 8) | (((phase) & 0x0F) << 4) | ((color) & 0x0F);
	
	// 调用现有的错误设置函数
	Error_Set(ERR_TYPE_TRAFFIC, fault_index);
	
	return 1;
}

/**
 * @brief 清除信号灯故障
 */
uint8_t TrafficFault_Clear(uint8_t fault, uint8_t type, uint8_t dir, uint8_t road, uint8_t phase, uint8_t color)
{
	// 生成错误索引（所有故障类型都使用相同的格式）
	uint32_t fault_index = (((fault) & 0xFF) << 20) | (((type) & 0x0F) << 16) | (((dir) & 0x0F) << 12) | (((road) & 0x0F) << 8) | (((phase) & 0x0F) << 4) | ((color) & 0x0F);
	
	// 调用现有的错误清除函数
	Error_Clear(ERR_TYPE_TRAFFIC, fault_index);
	
	return 1;
}

/**
 * @brief 检查信号灯故障
 */
uint8_t TrafficFault_Check(uint8_t fault, uint8_t type, uint8_t dir, uint8_t road, uint8_t phase, uint8_t color)
{
	// 生成错误索引（所有故障类型都使用相同的格式）
	uint32_t fault_index = (((fault) & 0xFF) << 20) | (((type) & 0x0F) << 16) | (((dir) & 0x0F) << 12) | (((road) & 0x0F) << 8) | (((phase) & 0x0F) << 4) | ((color) & 0x0F);
	
	// 调用现有的错误检查函数
	return Error_Check(ERR_TYPE_TRAFFIC, fault_index);
}

/**
 * @brief 清除所有信号灯故障
 */
uint8_t TrafficFault_ClearAll(void)
{
	// 清除信号灯故障组的所有错误
	g_err_item_count[ERR_TYPE_TRAFFIC] = 0;
	g_err_group_status[ERR_TYPE_TRAFFIC] = false;
	
	return 1;
}

/**
 * @brief 获取信号灯故障数量
 */
uint8_t TrafficFault_GetCount(void)
{
	// 返回信号灯组的错误数量
	return g_err_item_count[ERR_TYPE_TRAFFIC];
}

