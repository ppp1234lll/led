#include "appconfig.h"
#include "./TASK/inc/single.h"

#define SEND_HEAD_BYTE1 (0x0F) 	 
#define SEND_HEAD_BYTE2 (0x0F) 	 
																 
#define SEND_END_BYTE1 (0xFF) 	 
#define SEND_END_BYTE2 (0xFF) 	 
			
#define SEND_CMD       (0xA1) 

#define RECV_HAED_BYTE1 (0xF0) 	 
#define RECV_HAED_BYTE2 (0xF0) 	 
																 
#define RECV_END_BYTE1 (0xFF) 	 
#define RECV_END_BYTE2 (0xFF) 	 
		
typedef void (*UartSendFuncPtr)(uint8_t *data, uint16_t len);	
	
static const UartSendFuncPtr uart_send_table[BOARD_MAX] = {
    Lpuart1_Send_Data,  
    Usart1_Send_Data,    
    Usart4_Send_Data,    
    Uart5_Send_Data     
};		
		
single_data_t g_singleboard_t[BOARD_MAX];

uint8_t single_send_buf[16];  
uint8_t single_send_len;
uint8_t single_recv_buf[BOARD_MAX][200] ;
uint16_t single_recv_sta[BOARD_MAX]  ;  


void Uart_Send_Data(borad_id_t num, uint8_t *data, uint16_t len);
/*
*********************************************************************************************************
*	函 数 名: single_task_function
*	功能说明: 检测板线程
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void single_task_function(void)
{
	uint8_t send_status = 0;
	printf("run here\n");
	uint8_t index;
	
	single_cmd_board_data(single_send_buf,&single_send_len);  
	while(1)
	{
//		for(index = 0;index<BOARD_MAX;index++)
//		{
//			Uart_Send_Data((borad_id_t)index,single_send_buf,single_send_len);
//		}
		
		Lpuart1_Send_Data(single_send_buf,single_send_len);
		if(single_deal_board_data(BOARD_4) == 0)
		{
			send_status = 0;
		}
		
		iwdg_feed();      			
		vTaskDelay(100);  	 	  
	}
}

/*
*********************************************************************************************************
*	函 数 名: single_cmd_board_data
*	功能说明: 获取检测板数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Uart_Send_Data(borad_id_t num, uint8_t *data, uint16_t len)
{
	if (num >= BOARD_MAX || data == NULL || len == 0) {
		return; 
	}
	uart_send_table[num](data, len);
}

/*
*********************************************************************************************************
*	函 数 名: single_cmd_board_data
*	功能说明: 获取检测板数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void single_cmd_board_data(uint8_t *data, uint8_t *len)
{
	uint8_t index = 0;
	uint8_t crc   = 0;
	
	/* 数据头 */
	data[index++] = SEND_HEAD_BYTE1; 
	data[index++] = SEND_HEAD_BYTE2;

	/* 命令 */
	data[index++] = SEND_CMD;
	
	/* 数据长度 */
	data[index++] = 0x01;

	/* 数据内容 */
	data[index++] = 0x01;

	/* crc校验 */
	crc = calc_crc8(&data[2],index-2);
	data[index++] = crc;
	
	/* 数据尾 */
	data[index++] = SEND_END_BYTE1;
	data[index++] = SEND_END_BYTE2;
	*len = index;
}
/*
*********************************************************************************************************
*	函 数 名: single_recv_board_data
*	功能说明: 接收检测板数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void single_recv_board_data(uint8_t id, uint8_t *data, uint8_t len)
{
	memcpy(single_recv_buf[id], data, len );
	single_recv_sta[id] = (len | 0x8000);
	memset(data,0,len);
}

/*
*********************************************************************************************************
*	函 数 名: single_deal_board_data
*	功能说明: 处理数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t single_deal_board_data(uint8_t id)
{
	uint8_t crc  = 0;
	uint8_t size = 0;
	uint8_t err = 0;
	if (single_recv_sta[id] & 0x8000)  
	{
		size = single_recv_sta[id] & 0x07FF;
		if((single_recv_buf[id][0] == RECV_HAED_BYTE1) && (single_recv_buf[id][1] == RECV_HAED_BYTE2))
		{
			crc = calc_crc8(&single_recv_buf[id][2],size-5);
			if(crc == single_recv_buf[id][size-3])
			{
				g_singleboard_t[id].cmd  = single_recv_buf[id][2];
				g_singleboard_t[id].size = single_recv_buf[id][3];
				memcpy(&g_singleboard_t[id].data, &single_recv_buf[id][4], sizeof(board_t));
			}		
			else
				err = 3;	
		}
		else
			err = 2;
	}
	else
		err = 1;
	single_recv_sta[id] = 0;
	memset(single_recv_buf[id],0,128);
	return err;
}

