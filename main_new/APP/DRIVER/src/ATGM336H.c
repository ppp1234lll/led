/********************************************************************************
* @File name  : GPS模块
* @Description: 串口6-对应GPS
* @Author     : ZHLE
*  Version Date        Modification Description
	13、GPS(4G模块)： 串口6， 波特率：9600，引脚分配为： 
	      BDS_TX：    PC6
        BDS_RX：    PC7	
********************************************************************************/

#include "ATGM336H.h"
#include "appconfig.h"

CCMRAM atgm336h_data_t sg_atgm336h_param_t; // 定位信息

#define UART_GPS_TX_GPIO_CLK               RCC_AHB1Periph_GPIOC
#define UART_GPS_TX_GPIO_PORT              GPIOC
#define UART_GPS_TX_PIN                    GPIO_Pin_6

#define UART_GPS_RX_GPIO_CLK               RCC_AHB1Periph_GPIOC
#define UART_GPS_RX_GPIO_PORT              GPIOC
#define UART_GPS_RX_PIN                    GPIO_Pin_7

#define UART_GPS_CLK                       RCC_APB2Periph_USART6
#define UART_GPS                           USART6
#define UART_GPS_IRQn                      USART6_IRQn
#define UART_GPS_IRQHandler                USART6_IRQHandler

#define UART_GPS_RX_DMA 

#ifdef  UART_GPS_RX_DMA
#define UART_GPS_TX_DMA_STREAM             DMA2_Stream7
#define UART_GPS_RX_DMA_STREAM             DMA2_Stream1

#define UART_GPS_TX_DMA_CH               	 DMA_Channel_5
#define UART_GPS_RX_DMA_CH                 DMA_Channel_5
#endif

/* 参数 */
#define UART_GPS_RX_MAX  2048

uint8_t uart_gps_rx_buff[UART_GPS_RX_MAX] = {0};

/*
*********************************************************************************************************
*	函 数 名: bsp_InitUart_GPS
*	功能说明: 初始化串口硬件 
*	形    参: baudrate: 波特率
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitUart_GPS(uint32_t bound)
{
	/* GPIO端口设置 */
	GPIO_InitTypeDef  GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;

	/* 时钟初始化 */
	RCC_AHB1PeriphClockCmd(UART_GPS_TX_GPIO_CLK|UART_GPS_RX_GPIO_CLK, ENABLE); 
	RCC_APB2PeriphClockCmd(UART_GPS_CLK , ENABLE);

	/* 串口对应引脚复用映射 */
	GPIO_PinAFConfig(UART_GPS_TX_GPIO_PORT,GPIO_PinSource6, GPIO_AF_USART6); 
	GPIO_PinAFConfig(UART_GPS_RX_GPIO_PORT,GPIO_PinSource7, GPIO_AF_USART6);
	
	/* USART端口配置 */
	GPIO_InitStructure.GPIO_Pin   = UART_GPS_TX_PIN; 
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;			// 输出
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  		// 推挽输出
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;   	// 上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; 	// 高速GPIO
	GPIO_Init(UART_GPS_TX_GPIO_PORT, &GPIO_InitStructure);	

	GPIO_InitStructure.GPIO_Pin   = UART_GPS_RX_PIN;		 
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;			// 输出
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  		// 推挽输出
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;   	// 上拉
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; 	// 高速GPIO
	GPIO_Init(UART_GPS_RX_GPIO_PORT, &GPIO_InitStructure);			

	NVIC_InitStructure.NVIC_IRQChannel = UART_GPS_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  
	NVIC_Init(&NVIC_InitStructure);			

	/* USART 初始化设置 */
	USART_DeInit(UART_GPS);
	USART_InitStructure.USART_BaudRate = bound;									// 串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 收发模式
	USART_Init(UART_GPS, &USART_InitStructure);	 

	#ifdef UART_GPS_RX_DMA
	DMA_Config( UART_GPS_RX_DMA_STREAM,\
	            UART_GPS_RX_DMA_CH,\
							(uint32_t)&(UART_GPS->DR),\
							(uint32_t)uart_gps_rx_buff,
							DMA_DIR_PeripheralToMemory,\
							UART_GPS_RX_MAX);
	DMA_Enable(UART_GPS_RX_DMA_STREAM,UART_GPS_RX_MAX);
	USART_ITConfig(UART_GPS, USART_IT_IDLE, ENABLE);
	USART_DMACmd(UART_GPS,USART_DMAReq_Rx,ENABLE);
	#else
	USART_ITConfig(UART_GPS, USART_IT_RXNE, ENABLE);  // 开启串口接受中断
	#endif
	
	USART_ClearITPendingBit(UART_GPS, USART_IT_TC);
	USART_Cmd(UART_GPS, ENABLE);                      // 使能串口 
}

/*
*********************************************************************************************************
*	函 数 名: uart_gps_send_char
*	功能说明: 向串口发送1个字节。
*	形    参: 
*	@ch			: 待发送的字节数据
*	返 回 值: 无
*********************************************************************************************************
*/
void uart_gps_send_char(uint8_t ch)
{
	UART_GPS->DR = (uint8_t)ch;
	while ((UART_GPS->SR & 0X40) == 0);
}

/*
*********************************************************************************************************
*	函 数 名: uart_gps_send_str
*	功能说明: 向串口发送字符串。
*	形    参:  
*	@buff		: 字符串指针
*	@len		: 发送数据长度
*	返 回 值: 无
*********************************************************************************************************
*/
void uart_gps_send_str(uint8_t *buff, uint16_t len)
{
	while(len--) {
		uart_gps_send_char(buff[0]);
		buff++;
	}
}

/*
*********************************************************************************************************
*	函 数 名: UART_GPS_IRQHandler
*	功能说明: 供中断服务程序调用，通用串口中断处理函数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void UART_GPS_IRQHandler(void)
{
#ifdef UART_GPS_RX_DMA
	uint16_t size = 0;
	size = size;
	if (USART_GetITStatus(UART_GPS, USART_IT_IDLE) != RESET) 
	{
		USART_ClearITPendingBit(UART_GPS, USART_IT_IDLE);
		USART_ReceiveData(UART_GPS);
		
		DMA_Cmd(UART_GPS_RX_DMA_STREAM, DISABLE);/* 停止DMA */
		size = UART_GPS_RX_MAX - DMA_GetCurrDataCounter(UART_GPS_RX_DMA_STREAM);
    sg_atgm336h_param_t.status = 1;
		
		DMA_Enable(UART_GPS_RX_DMA_STREAM,UART_GPS_RX_MAX);/* 设置传输模式 */
	}
#else
	static uint8_t test = 0;
	uint8_t res = 0;
	
	if (USART_GetITStatus(UART_GPS, USART_IT_RXNE) != RESET) {
		USART_ClearITPendingBit(UART_GPS, USART_IT_RXNE);
		res = USART_ReceiveData(UART_GPS);
//		printf("%c",res);
		uart_gps_rx_buff[test++] = res;
	}
#endif
}

/*
*********************************************************************************************************
*	函 数 名: atgm336h_decode_nmea_xxgga
*	功能说明: 解析$XXGGA类型的NMEA消息
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t atgm336h_decode_nmea_xxgga(void)
{
	if(sg_atgm336h_param_t.status == 1)
	{
		sg_atgm336h_param_t.status = 0;
		sg_atgm336h_param_t.is_valid = 1;
		
		// Find GNGGA sentence (GNSS combined positioning data including GPS and BDS)
		char *gga_sentence = strstr((char*)uart_gps_rx_buff, "$GNGGA");
		if (gga_sentence == NULL) {
			// If GNGGA not found, try to find GPGGA
			gga_sentence = strstr((char*)uart_gps_rx_buff, "$GPGGA");
			if (gga_sentence == NULL) {
				// If still not found, try to find BDGA (BDS-only GGA)
				gga_sentence = strstr((char*)uart_gps_rx_buff, "$BDGGA");
				if (gga_sentence == NULL) {
					return 1;
				}
			}
		}
		// The rest of the parsing logic remains the same for all GGA variants
		// Split each field of GGA sentence
		char *token = strtok(gga_sentence, ",");
		uint8_t field_index = 0;
		char latitude_str[20] = {0};
		char longitude_str[20] = {0};
		char altitude_str[20] = {0};
		char hdop_str[20] = {0};
		char satellites_str[5] = {0};
		char fix_quality_str[5] = {0};
		
		while (token != NULL && field_index <= 10) {
			switch (field_index) {
			case 1: // Time (HHMMSS format)
				break;
			case 2: // Latitude (ddmm.mmmm format)
				strncpy(latitude_str, token, sizeof(latitude_str) - 1);
				break;
			case 3: // Latitude direction (N/S)
				sg_atgm336h_param_t.lat_dir = token[0];
				break;
			case 4: // Longitude (dddmm.mmmm format)
				strncpy(longitude_str, token, sizeof(longitude_str) - 1);
				break;
			case 5: // Longitude direction (E/W)
				sg_atgm336h_param_t.lon_dir = token[0];
				break;
			case 6: // Fix quality indicator
				strncpy(fix_quality_str, token, sizeof(fix_quality_str) - 1);
				break;
			case 7: // Number of satellites in use
				strncpy(satellites_str, token, sizeof(satellites_str) - 1);
				break;
			case 8: // Horizontal dilution of precision
				strncpy(hdop_str, token, sizeof(hdop_str) - 1);
				break;
			case 9: // Altitude
				strncpy(altitude_str, token, sizeof(altitude_str) - 1);
				break;
			case 10:  
				break;
			}
			token = strtok(NULL, ",");
			field_index++;
		}
		
		// Convert fix quality
		if (strlen(fix_quality_str) > 0) {
			sg_atgm336h_param_t.fix_quality = atoi(fix_quality_str);
		}
		
		// Convert number of satellites
		if (strlen(satellites_str) > 0) {
			sg_atgm336h_param_t.num_satellites = atoi(satellites_str);
		}
		
		// Convert horizontal dilution of precision
		if (strlen(hdop_str) > 0) {
			sg_atgm336h_param_t.hdop = atof(hdop_str);
		}
		
		// Convert altitude
		if (strlen(altitude_str) > 0) {
			sg_atgm336h_param_t.altitude = atof(altitude_str);
		}
		
		// Convert latitude (from ddmm.mmmm format to decimal format)
		if (strlen(latitude_str) > 0) {
			char deg_str[4] = {0};
			char min_str[10] = {0};
			int deg = 0;
			double min = 0.0;
			
			// For latitude, first two digits are degrees, the rest are minutes
			strncpy(deg_str, latitude_str, 2);
			deg = atoi(deg_str);
			strcpy(min_str, latitude_str + 2);
			min = atof(min_str);
			
			// Convert to decimal format
			sg_atgm336h_param_t.latitude = deg + min / 60.0;
			
			// Adjust sign according to direction
			if (sg_atgm336h_param_t.lat_dir == 'S') {
				sg_atgm336h_param_t.latitude = -sg_atgm336h_param_t.latitude;
			}
		}
		
		// Convert longitude (from dddmm.mmmm format to decimal format)
		if (strlen(longitude_str) > 0) {
			char deg_str[5] = {0};
			char min_str[10] = {0};
			int deg = 0;
			double min = 0.0;
			
			// For longitude, first three digits are degrees, the rest are minutes
			strncpy(deg_str, longitude_str, 3);
			deg = atoi(deg_str);
			strcpy(min_str, longitude_str + 3);
			min = atof(min_str);
			
			// Convert to decimal format
			sg_atgm336h_param_t.longitude = deg + min / 60.0;
			
			// Adjust sign according to direction
			if (sg_atgm336h_param_t.lon_dir == 'W') {
				sg_atgm336h_param_t.longitude = -sg_atgm336h_param_t.longitude;
			}
		}
		
		// Only consider data valid when fix quality is greater than 0
		if (sg_atgm336h_param_t.fix_quality > 0) {
			sg_atgm336h_param_t.is_valid = 0;
		}
		memset(uart_gps_rx_buff,0,UART_GPS_RX_MAX);
		return sg_atgm336h_param_t.is_valid;
	}
	else
	{
		return 2;
	}
}

/*
*********************************************************************************************************
*	函 数 名: atgm336h_decode_nmea_xxgga
*	功能说明: 解析$XXGGA类型的NMEA消息
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
atgm336h_data_t* atgm336h_get_gnss_data(void)
{
	return &sg_atgm336h_param_t;
}

/*
*********************************************************************************************************
*	函 数 名: ATGM338H_test
*	功能说明: 测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void ATGM338H_test(void)
{
	while(1)
	{
		atgm336h_decode_nmea_xxgga();
		
		if (sg_atgm336h_param_t.is_valid == 0) {

			printf("\n=== GNSS Data Parsing Result ===\n");
			printf("Fix Status: Valid\n");
			printf("Fix Quality: %d (0=Invalid, 1=GPS Fix, 2=DGPS Fix, 3=PPS Fix, 4=RTK, 5=Float RTK)\n", sg_atgm336h_param_t.fix_quality);
			printf("Number of Satellites: %d\n", sg_atgm336h_param_t.num_satellites);
			printf("Horizontal Dilution of Precision: %.2f\n", sg_atgm336h_param_t.hdop);
			printf("Latitude: %.10f %c\n", fabs(sg_atgm336h_param_t.latitude), sg_atgm336h_param_t.lat_dir);
			printf("Longitude: %.10f %c\n", fabs(sg_atgm336h_param_t.longitude), sg_atgm336h_param_t.lon_dir);
			printf("Decimal Latitude: %.10f\n", sg_atgm336h_param_t.latitude);
			printf("Decimal Longitude: %.10f\n", sg_atgm336h_param_t.longitude);
			printf("Altitude: %.10f\n", sg_atgm336h_param_t.altitude);
			printf("==============================\n\n");

		} else {

			printf("\n=== GNSS Data Parsing Result ===\n");
			printf("Fix Status: Invalid\n");
			printf("No valid positioning data found. Please ensure the device is connected to satellites and working properly\n");
			printf("==============================\n\n");

		}
		delay_ms(1000);
	}
}










