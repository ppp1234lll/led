#include "./USER/inc/error.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"

// 错误状态（使用数组存储错误索引，不使用bit操作）
// 最多存储256个错误
uint32_t g_err_item_status[ERR_MAX][256] = {0}; // 存储错误索引
uint8_t g_err_item_count[ERR_MAX] = {0}; // 每组错误数量

/*
*********************************************************************************************************
*	函 数 名: Error_Set
*	功能说明: 标记错误发生（支持任意数量错误）
*	形    参: 
* @param group: 错误组（ERR_GROUP_ELEC等）
* @param item_idx: 错误索引（ELEC_NORMAL等）
*	返 回 值: 无
*********************************************************************************************************
*/
void Error_Set(ErrorType_e group, uint32_t item_idx)
{
	if (group >= ERR_MAX) return;
	
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
int8_t Error_GetAllCodes(uint32_t* codes)
{
	if (codes == NULL) return -1;
	
	uint8_t err_count = 0;
    for (uint8_t group = 0; group < ERR_MAX; group++) 
	{
		if (g_err_item_count[group] == 0) 
			continue;
			
		// 遍历组内错误
		for (uint8_t i = 0; i < g_err_item_count[group]; i++) 
		{
			codes[err_count++] = g_err_item_status[group][i];
		}
	}
	return err_count;
}

int8_t Error_Get_Codesbuf(uint8_t* codes)
{
	if (codes == NULL) return -1;
	
	uint8_t err_count = 0;
	uint8_t buf[16] = {0};

	// 计算故障数量
	for (uint8_t group = 0; group < ERR_MAX; group++) 
	{
		err_count += g_err_item_count[group];
	}
	
	// 生成格式：ERR:数量,故障码1,故障码2;
	// 写入前缀和数量
	sprintf((char*)buf, "ERR:%d", err_count);
	strcat((char*)codes,(char*)buf);
	
	// 写入故障码
	if (err_count > 0)
	{
		// 遍历所有组
		for (uint8_t group = 0; group < ERR_MAX; group++) 
		{
			if (g_err_item_count[group] == 0) 
				continue;
				
			// 遍历组内错误
			for (uint8_t i = 0; i < g_err_item_count[group]; i++) 
			{
				// 添加逗号分隔符（不是第一个故障码时）
				strcat((char*)codes,",");
				
				// 将故障码转换为字符串
				sprintf((char*)buf, "%08X", g_err_item_status[group][i]);
				strcat((char*)codes,(char*)buf);
			}
		}
	}
	
	// 添加分号结尾
	strcat((char*)codes,(char*)";");
	return err_count;
}


/**
 * @brief 设置信号灯故障
 */
uint8_t TrafficFault_Set(uint8_t fault, uint8_t type, uint8_t dir, uint8_t road, uint8_t phase, uint8_t color)
{
	uint32_t fault_index = (((fault) & 0xFF) << 20) | (((type) & 0x0F) << 16) | (((dir) & 0x0F) << 12) | (((road) & 0x0F) << 8) | (((phase) & 0x0F) << 4) | ((color) & 0x0F);
	fault_index = ERR_TYPE_TRAFFIC_BASE + fault_index;
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
	fault_index = ERR_TYPE_TRAFFIC_BASE + fault_index;
	// 调用现有的错误清除函数
	Error_Clear(ERR_TYPE_TRAFFIC, fault_index);
	
	return 1;
}

