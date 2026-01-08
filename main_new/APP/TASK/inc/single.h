#ifndef _SINGLE_H_
#define _SINGLE_H_

#include "./SYSTEM/sys/sys.h"

typedef enum
{
	BOARD_1 = 0,
  BOARD_2,
	BOARD_3,
	BOARD_4,
	
	BOARD_MAX
}borad_id_t;

typedef struct 
{
	float   current[24];
  uint8_t acin_s[12];
}board_t;

typedef struct
{
	uint8_t  cmd;   // 命令
	uint8_t  size;  // 数据长度
	board_t  data;  // 数据内容
} single_data_t;

void single_task_function(void);

void single_cmd_board_data(uint8_t *data, uint8_t *len);
void single_recv_board_data(uint8_t id, uint8_t *data, uint8_t len);
uint8_t single_deal_board_data(uint8_t id);

#endif
