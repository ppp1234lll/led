
#include "appconfig.h"

/*
*********************************************************************************************************
*	函 数 名: DemoSpiFlash
*	功能说明: 串行EEPROM读写例程
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_test(void)
{
	led_test();
	relay_test();
	key_test();
	RTC_Get_Time_Test();
	uart1_test();
	uart2_test();
	uart3_test();
	uart4_test();	
	uart5_test();	
	uart6_test();	
	uart7_test();
	lpuart1_test();
	
	lis3dh_test();
	
	while(1)
	{
		printf("bsp_test...\n");		
		delay_ms(1000);
	}
}


