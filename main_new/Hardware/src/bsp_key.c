#include "bsp_key.h"
#include "appconfig.h"
/*
	4、输入检测

		12V电源输入监测:       PD0
		箱门检测1:             PA12
		箱门检测2:             PA11
		箱门检测3:             PC8
		箱门检测4:             PD15
		水浸1 :                PD14
    水浸2 :                PD11		

    空开前220输入检测 :    PE15
*/
#define RESET_KEY_READ 			HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_4)	 
#define PWR_TST_READ  		  HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_0)  
#define DOOR1_READ    	 		HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_12)   
#define DOOR2_READ   	 			HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_11)  
#define DOOR3_READ   	 		  HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_8)  
#define DOOR4_READ   	 		  HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_15)  
#define WATER1_READ   	 		HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_14)   
#define WATER2_READ   	 		HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_11)
#define MCB_220_READ   	 		HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_15)

/*
*********************************************************************************************************
*	函 数 名: bsp_InitKey
*	功能说明: 初始化按键.  
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitKey(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
	
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
	
  /*Configure GPIO pin : PE15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);	
  /*Configure GPIO pins : PD11 PD14 PD15 PD0
                           PD4 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0
                          |GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);	
	
}

/*
*********************************************************************************************************
*	函 数 名: bsp_KeyScan10ms
*	功能说明: 扫描所有按键。非阻塞，被systick中断周期性的调用，10ms一次
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t KeyScan10ms(uint8_t id)
{
	switch(id)
	{
		case 0: return  PWR_TST_READ;
		case 1: return  DOOR1_READ;
	  case 2: return  DOOR2_READ;
		case 3: return  DOOR3_READ;
		case 4: return  DOOR4_READ;
	  case 5: return  WATER1_READ;
		case 6: return  WATER2_READ;
	  case 7: return  MCB_220_READ;
		default: return 0;
	}
}

/*
*********************************************************************************************************
*	函 数 名: key_test
*	功能说明: 按键测试.  
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void key_test(void)
{
	while(1)
	{
		printf("RESET = %d...",RESET_KEY_READ);
		printf("PWR   = %d...",PWR_TST_READ);
		printf("DOOR1 = %d...",DOOR1_READ);
		printf("DOOR2 = %d...",DOOR2_READ);
		printf("DOOR3 = %d...",DOOR3_READ);
		printf("DOOR4 = %d...",DOOR4_READ);
		printf("WATER1= %d...",WATER1_READ);
		printf("WATER2= %d...",WATER2_READ);
		printf("220IN = %d\n ",MCB_220_READ);		
		
		delay_ms(1000);		
	}
}

