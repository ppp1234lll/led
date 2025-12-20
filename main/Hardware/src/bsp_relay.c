#include "bsp_relay.h"
#include "bsp.h"

/*
	3、继电器
         继电器1：    PE10 (左下)
         继电器2：    PE11
         继电器3：    PE12
         继电器4：    PE13	 
*/
/******************************************************************************************/
/* 引脚 定义 */

#define RELAY1_GPIO_PORT     GPIOB
#define RELAY1_GPIO_PIN      GPIO_PIN_8

#define RELAY2_GPIO_PORT     GPIOB
#define RELAY2_GPIO_PIN      GPIO_PIN_9

#define RELAY3_GPIO_PORT     GPIOB
#define RELAY3_GPIO_PIN      GPIO_PIN_0

#define RELAY4_GPIO_PORT     GPIOE
#define RELAY4_GPIO_PIN      GPIO_PIN_8
/******************************************************************************************/

#define RELAY1_CTRL(x)  x ? \
												HAL_GPIO_WritePin(RELAY1_GPIO_PORT, RELAY1_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(RELAY1_GPIO_PORT, RELAY1_GPIO_PIN, GPIO_PIN_RESET);  
#define RELAY2_CTRL(x)  x ? \
												HAL_GPIO_WritePin(RELAY2_GPIO_PORT, RELAY2_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(RELAY2_GPIO_PORT, RELAY2_GPIO_PIN, GPIO_PIN_RESET);  
#define RELAY3_CTRL(x)  x ? \
												HAL_GPIO_WritePin(RELAY3_GPIO_PORT, RELAY3_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(RELAY3_GPIO_PORT, RELAY3_GPIO_PIN, GPIO_PIN_RESET);  
#define RELAY4_CTRL(x)  x ? \
												HAL_GPIO_WritePin(RELAY4_GPIO_PORT, RELAY4_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(RELAY4_GPIO_PORT, RELAY4_GPIO_PIN, GPIO_PIN_RESET);  


typedef struct
{
	uint8_t relay[RELAY_NUM]; 
} relay_t;

relay_t sg_relay_t;

/*
*********************************************************************************************************
*	函 数 名: bsp_InitRelay
*	功能说明: 继电器初始化
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitRelay(void)
{

	relay_control(RELAY_1,RELAY_ON);
	relay_control(RELAY_2,RELAY_ON);
	relay_control(RELAY_3,RELAY_ON);
	relay_control(RELAY_4,RELAY_ON);
}

/*
*********************************************************************************************************
*	函 数 名: relay_control
*	功能说明: 继电器控制
*	形    参:  dev  : 继电器序号
*	           state: 继电器状态
*	返 回 值: 无
*********************************************************************************************************
*/
void relay_control(RELAY_DEV dev, RELAY_STATUS state)
{
	switch(dev)
	{
		case RELAY_1:
			sg_relay_t.relay[RELAY_1] = state;
			RELAY1_CTRL(state);
			break;
			
		case RELAY_2:
			sg_relay_t.relay[RELAY_2] = state;
			RELAY2_CTRL(state);
			break;

		case RELAY_3:
			sg_relay_t.relay[RELAY_3] = state;
			RELAY3_CTRL(state);
			break;

		case RELAY_4:
			sg_relay_t.relay[RELAY_4] = state;
			RELAY4_CTRL(state);
			break;
		default:	break;
	}
}

/*
*********************************************************************************************************
*	函 数 名: relay_get_status
*	功能说明: 获取继电器状态
*	形    参: 无
*	返 回 值: 继电器状态
*********************************************************************************************************
*/
uint8_t relay_get_status(RELAY_DEV dev)
{
	return sg_relay_t.relay[dev];
}

/*
*********************************************************************************************************
*	函 数 名: relay_test
*	功能说明: 继电器测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void relay_test(void)
{
	while(1)
	{
		for(uint8_t i=0;i<RELAY_NUM;i++)
		{
			relay_control((RELAY_DEV)i,RELAY_ON); 
			delay_ms(500);
		}
		delay_ms(2000);
		
		for(uint8_t i=0;i<RELAY_NUM;i++)
		{
			relay_control((RELAY_DEV)i,RELAY_OFF); 
			delay_ms(500);
		}
		delay_ms(2000);
	}
}


