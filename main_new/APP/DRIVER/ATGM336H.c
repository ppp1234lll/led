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

/* ATGM336H结构体 */
typedef struct
{
	atgm336h_time_t utc;            // UTC时间
	atgm336h_position_t position;   // 定位信息（经纬度扩大100000倍）
	int16_t altitude;               // 海拔高度（扩大10倍），单位：米
	uint16_t speed;                 // 地面速度（扩大10倍），单位：千米/时
	atgm336h_fix_info_t fix_info;   // 定位信息
	atgm336h_visible_satellite_info_t gps_satellite_info ;// 可见GPS卫星信息
	atgm336h_visible_satellite_info_t beidou_satellite_info ;//可见北斗卫星信息 
} atgm336h_data_t;

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
#define UART_GPS_RX_MAX  512

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
	RCC_APB1PeriphClockCmd(UART_GPS_CLK , ENABLE);

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
	
	if (USART_GetITStatus(UART_GPS, USART_IT_IDLE) != RESET) 
	{
		USART_ClearITPendingBit(UART_GPS, USART_IT_IDLE);
		USART_ReceiveData(UART_GPS);
		
		DMA_Cmd(UART_GPS_RX_DMA_STREAM, DISABLE);/* 停止DMA */
		size = UART_GPS_RX_MAX - DMA_GetCurrDataCounter(UART_GPS_RX_DMA_STREAM);
//		gps_get_receive_data_function(uart_gps_rx_buff,size);

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

/**
 * @brief       获取并更新ATGM336H模块数据
 * @param       utc                  : UTC时间
 *              position             : 位置信息（经纬度扩大100000倍）
 *              altitude             : 海拔高度（扩大10倍），单位：米
 *              speed                : 地面速度（扩大10倍），单位：千米/时
 *              fix_info             : 定位信息
 *              gps_satellite_info   : 可见GPS卫星信息
 *              beidou_satellite_info: 可见北斗卫星信息
 *              timeout              : 等待超时时间，单位：1毫秒
 * @retval      ATGM336H_EOK     : 获取并更新ATGM336H模块数据成功
 *              ATGM336H_EINVAL  : 函数参数错误
 *              ATGM336H_ETIMEOUT: 等待超时
 */
uint8_t atgm336h_update(uint32_t timeout)
{
    uint8_t ret;
    uint8_t *buf;
    uint8_t *nmea;
    struct
    {
			atgm336h_nmea_gga_msg_t msg;
			uint8_t done;
    } gngga;
    struct
    {
        atgm336h_nmea_gsa_msg_t msg;
        uint8_t done;
    } gngsa;
    struct
    {
        atgm336h_nmea_gsv_msg_t msg;
        uint8_t done;
    } gpgsv;
    struct
    {
        atgm336h_nmea_gsv_msg_t msg;
        uint8_t done;
    } bdgsv;
    struct
    {
        atgm336h_nmea_rmc_msg_t msg;
        uint8_t done;
    } gnrmc;
    struct
    {
        atgm336h_nmea_vtg_msg_t msg;
        uint8_t done;
    } gnvtg;
    uint8_t satellite_index;
    
    if ((utc == NULL) && (position == NULL) && (altitude == NULL) && (speed == NULL) && (fix_info == NULL) && (gps_satellite_info == NULL) && (beidou_satellite_info == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    gngga.done = 0;
    gngsa.done = 0;
    gpgsv.done = 0;
    bdgsv.done = 0;
    gnrmc.done = 0;
    gnvtg.done = 0;
    
    atgm336h_uart_rx_restart();
    while (timeout > 0)
    {
        buf = atgm336h_uart_rx_get_frame();
        if (buf != NULL)
        {
            /* GNGGA */
            if (((altitude != NULL) || (fix_info != NULL)) && (gngga.done == 0))
            {
                ret = atgm336h_get_nmea_msg_from_buf(buf, ATGM336H_NMEA_MSG_GNGGA, 0, &nmea);
                if (ret == ATGM336H_EOK)
                {
                    ret = atgm336h_decode_nmea_xxgga(nmea, &gngga.msg);
                    if (ret == ATGM336H_EOK)
                    {
                        gngga.done = ~0;
                        if (altitude != NULL)
                        {
                            *altitude = gngga.msg.altitude;
                        }
                        if (fix_info != NULL)
                        {
                            fix_info->quality = gngga.msg.gps_quality;
                            fix_info->satellite_num = gngga.msg.satellite_num;
                        }
                    }
                }
            }
            else
            {
                gngga.done = ~0;
            }
            
            /* GNGSA */
            if ((fix_info != NULL) && (gngsa.done == 0))
            {
                ret = atgm336h_get_nmea_msg_from_buf(buf, ATGM336H_NMEA_MSG_GNGSA, 0, &nmea);
                if (ret == ATGM336H_EOK)
                {
                    ret = atgm336h_decode_nmea_xxgsa(nmea, &gngsa.msg);
                    if (ret == ATGM336H_EOK)
                    {
                        gngsa.done = ~0;
                        fix_info->type = gngsa.msg.type;
                        for (satellite_index=0; satellite_index<12; satellite_index++)
                        {
                            fix_info->satellite_id[satellite_index] = gngsa.msg.satellite_id[satellite_index];
                        }
                        fix_info->pdop = gngsa.msg.pdop;
                        fix_info->hdop = gngsa.msg.hdop;
                        fix_info->vdop = gngsa.msg.vdop;
                    }
                }
            }
            else
            {
                gngsa.done = ~0;
            }
            
            /* GPGSV */
            if ((gps_satellite_info != NULL) && (gpgsv.done == 0))
            {
                ret = atgm336h_get_nmea_msg_from_buf(buf, ATGM336H_NMEA_MSG_GPGSV, 0, &nmea);
                if (ret == ATGM336H_EOK)
                {
                    ret = atgm336h_decode_nmea_xxgsv(nmea, &gpgsv.msg);
                    if (ret == ATGM336H_EOK)
                    {
                        gpgsv.done = ~0;
                        gps_satellite_info->satellite_num = gpgsv.msg.satellite_view;
                        for (satellite_index=0; satellite_index<gpgsv.msg.satellite_view; satellite_index++)
                        {
                            gps_satellite_info->satellite_info[satellite_index].satellite_id = gpgsv.msg.satellite_info[satellite_index].satellite_id;
                            gps_satellite_info->satellite_info[satellite_index].elevation = gpgsv.msg.satellite_info[satellite_index].elevation;
                            gps_satellite_info->satellite_info[satellite_index].azimuth = gpgsv.msg.satellite_info[satellite_index].azimuth;
                            gps_satellite_info->satellite_info[satellite_index].snr = gpgsv.msg.satellite_info[satellite_index].snr;
                        }
                    }
                }
            }
            else
            {
                gpgsv.done = ~0;
            }
            
            /* BDGSV */
            if ((beidou_satellite_info != NULL) && (bdgsv.done == 0))
            {
                ret = atgm336h_get_nmea_msg_from_buf(buf, ATGM336H_NMEA_MSG_BDGSV, 0, &nmea);
                if (ret == ATGM336H_EOK)
                {
                    ret = atgm336h_decode_nmea_xxgsv(nmea, &bdgsv.msg);
                    if (ret == ATGM336H_EOK)
                    {
                        bdgsv.done = ~0;
                        beidou_satellite_info->satellite_num = bdgsv.msg.satellite_view;
                        for (satellite_index=0; satellite_index<bdgsv.msg.satellite_view; satellite_index++)
                        {
                            beidou_satellite_info->satellite_info[satellite_index].satellite_id = bdgsv.msg.satellite_info[satellite_index].satellite_id;
                            beidou_satellite_info->satellite_info[satellite_index].elevation = bdgsv.msg.satellite_info[satellite_index].elevation;
                            beidou_satellite_info->satellite_info[satellite_index].azimuth = bdgsv.msg.satellite_info[satellite_index].azimuth;
                            beidou_satellite_info->satellite_info[satellite_index].snr = bdgsv.msg.satellite_info[satellite_index].snr;
                        }
                    }
                }
            }
            else
            {
                bdgsv.done = ~0;
            }
            
            /* GNRMC */
            if (((utc != NULL) || (position != NULL)) && (gnrmc.done == 0))
            {
                ret = atgm336h_get_nmea_msg_from_buf(buf, ATGM336H_NMEA_MSG_GNRMC, 0, &nmea);
                if (ret == ATGM336H_EOK)
                {
                    ret = atgm336h_decode_nmea_xxrmc(nmea, &gnrmc.msg);
                    if (ret == ATGM336H_EOK)
                    {
                        gnrmc.done = ~0;
                        if (utc != NULL)
                        {
                            utc->year = gnrmc.msg.utc_date.year;
                            utc->month = gnrmc.msg.utc_date.month;
                            utc->day = gnrmc.msg.utc_date.day;
                            utc->hour = gnrmc.msg.utc_time.hour;
                            utc->minute = gnrmc.msg.utc_time.minute;
                            utc->second = gnrmc.msg.utc_time.second;
                            utc->millisecond = gnrmc.msg.utc_time.millisecond;
                        }
                        if (position != NULL)
                        {
                            position->latitude.degree = gnrmc.msg.latitude.degree;
                            position->latitude.indicator = gnrmc.msg.latitude.indicator;
                            position->longitude.degree = gnrmc.msg.longitude.degree;
                            position->longitude.indicator = gnrmc.msg.longitude.indicator;
                        }
                    }
                }
            }
            else
            {
                gnrmc.done = ~0;
            }
            
            /* GNVTG */
            if ((speed != NULL) && (gnvtg.done == 0))
            {
                ret = atgm336h_get_nmea_msg_from_buf(buf, ATGM336H_NMEA_MSG_GNVTG, 0, &nmea);
                if (ret == ATGM336H_EOK)
                {
                    gnvtg.done = ~0;
                    ret = atgm336h_decode_nmea_xxvtg(nmea, &gnvtg.msg);
                    if (ret == ATGM336H_EOK)
                    {
                        *speed = gnvtg.msg.speed_kph;
                    }
                }
            }
            else
            {
                gnvtg.done = ~0;
            }
        }
        
        if ((gngga.done != 0) && (gngsa.done != 0) && (gpgsv.done != 0) && (bdgsv.done != 0) && (gnrmc.done != 0) && (gnvtg.done != 0))
        {
            return ATGM336H_EOK;
        }
        
        timeout--;
        delay_ms(1);
    }
    
    return ATGM336H_ETIMEOUT;
}
