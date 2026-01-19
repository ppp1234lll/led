#include "bsp_led.h"
#include "bsp.h"

/*
	2、指示灯（原理图）：
		系统状态指示灯  STATE      : PE6
		网口指示灯      LAN        : PE5
		4G指示灯        GPRS       : PE4
*/
/******************************************************************************************/
/* 引脚 定义 */

#define LED_STATE_GPIO_PORT						GPIOE
#define LED_STATE_GPIO_PIN						GPIO_PIN_6
#define LED_STATE_GPIO_CLK_ENABLE()  	__HAL_RCC_GPIOE_CLK_ENABLE();

#define LED_LAN_GPIO_PORT				  		GPIOE
#define LED_LAN_GPIO_PIN				  		GPIO_PIN_5
#define LED_LAN_GPIO_CLK_ENABLE()  	  __HAL_RCC_GPIOE_CLK_ENABLE();

#define LED_GPRS_GPIO_PORT						GPIOE
#define LED_GPRS_GPIO_PIN							GPIO_PIN_4
#define LED_GPRS_GPIO_CLK_ENABLE()  	__HAL_RCC_GPIOE_CLK_ENABLE();
/******************************************************************************************/

#define LED_STATE(x)  	x ? \
												HAL_GPIO_WritePin(LED_STATE_GPIO_PORT, LED_STATE_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(LED_STATE_GPIO_PORT, LED_STATE_GPIO_PIN, GPIO_PIN_RESET);  
#define LED_LAN(x)    	x ? \
												HAL_GPIO_WritePin(LED_LAN_GPIO_PORT, LED_LAN_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(LED_LAN_GPIO_PORT, LED_LAN_GPIO_PIN, GPIO_PIN_RESET);  
#define LED_GPRS(x)   	x ? \
												HAL_GPIO_WritePin(LED_GPRS_GPIO_PORT, LED_GPRS_GPIO_PIN, GPIO_PIN_SET) : \
												HAL_GPIO_WritePin(LED_GPRS_GPIO_PORT, LED_GPRS_GPIO_PIN, GPIO_PIN_RESET);  

#define LED_STATE_TOG()   HAL_GPIO_TogglePin(LED_STATE_GPIO_PORT,LED_STATE_GPIO_PIN) 
#define LED_LAN_STA()     HAL_GPIO_TogglePin(LED_LAN_GPIO_PORT,LED_LAN_GPIO_PIN) 
#define LED_GPRS_TOG()    HAL_GPIO_TogglePin(LED_GPRS_GPIO_PORT,LED_GPRS_GPIO_PIN) 

/* 指示灯闪烁时间*/
#define FLICKER_TIME_Q	(200)
#define FLICKER_TIME 		(500)
#define FLICKER_TIME_1S (1000)

/* 指示灯状态变量*/
typedef struct
{
	uint8_t gprs;
	uint8_t lan;
	uint8_t state;
	uint8_t lan_out;
	uint8_t power_out;
} led_flicker_t;

led_flicker_t sg_ledflicker_t = {0};

/*
*********************************************************************************************************
*	函 数 名: bsp_InitLed
*	功能说明: 初始化指示灯控制io:默认不开启
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitLed(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	LED_STATE_GPIO_CLK_ENABLE();
	LED_LAN_GPIO_CLK_ENABLE();
	LED_GPRS_GPIO_CLK_ENABLE();
	
	HAL_GPIO_WritePin(LED_STATE_GPIO_PORT,LED_STATE_GPIO_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_LAN_GPIO_PORT,LED_LAN_GPIO_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_GPRS_GPIO_PORT,LED_GPRS_GPIO_PIN, GPIO_PIN_SET);
	
  GPIO_InitStruct.Pin = LED_STATE_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_STATE_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED_LAN_GPIO_PIN;
  HAL_GPIO_Init(LED_LAN_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED_GPRS_GPIO_PIN;
  HAL_GPIO_Init(LED_GPRS_GPIO_PORT, &GPIO_InitStruct);	
}

/*
*********************************************************************************************************
*	函 数 名: led_control_function
*	功能说明: 点亮指定的LED指示灯。
*	形    参:  dev  : 指示灯序号
*	           state: 指示灯状态
*	返 回 值: 无
*********************************************************************************************************
*/
void led_control_function(LD_DEV dev, LED_STATUS state)
{
	switch(dev)
	{
		case LD_STATE:  // 系统运行指示灯
			sg_ledflicker_t.state = state;
			switch(state) 
			{
        case LD_ON:	 LED_STATE(0);break;
        case LD_OFF: LED_STATE(1);break;
        default:break;
      }
      break;
			
		case LD_GPRS:  // 4G指示灯
			sg_ledflicker_t.gprs = state;
			switch(state) 
			{
        case LD_ON:			LED_GPRS(0);break;
        case LD_OFF:		LED_GPRS(1);break;
        default:break;
			}
			break;
			
		case LD_LAN:   // 有线指示灯
			sg_ledflicker_t.lan = state;
			switch(state) 
			{
        case LD_ON:			LED_LAN(0);break;
        case LD_OFF:		LED_LAN(1);break;
        default:break;
			}
			break;

		default:		break;
	}
}

/*
*********************************************************************************************************
*	函 数 名: led_flicker_control_timer_function
*	功能说明: led闪动
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void led_flicker_control_timer_function(void)
{
	static uint16_t count   = 0;
	static uint16_t count3	= 0;
	
	count++;
	if(count > FLICKER_TIME)
	{
		count = 0;
		if(sg_ledflicker_t.gprs == LD_FLICKER)		/* 显示无线网络状态 */
		{
			LED_GPRS_TOG();
		}

		if(sg_ledflicker_t.lan == LD_FLICKER)		/* 显示有线网络状态 */
		{
			LED_LAN_STA();
		} 
	
		if(sg_ledflicker_t.state == LD_FLICKER) 	/* 系统状态灯 */
		{
			LED_STATE_TOG();
		}
	}

	count3++;
	if(count3 > FLICKER_TIME_Q)
	{
		count3 = 0;
		if(sg_ledflicker_t.gprs == LD_FLIC_Q)		/* 显示无线网络状态 */
			LED_GPRS_TOG();

		if(sg_ledflicker_t.lan == LD_FLIC_Q)		/* 显示有线网络状态 */
			LED_LAN_STA();
	}
}

/*
*********************************************************************************************************
*	函 数 名: led_test
*	功能说明: led测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void led_test(void)
{
	while(1)
	{
    HAL_GPIO_WritePin(GPIOE,GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);
		delay_ms(1000);
    HAL_GPIO_WritePin(GPIOE,GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_SET);
		delay_ms(1000);	
	}
}
/******************************************  (END OF FILE) **********************************************/


