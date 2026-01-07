#ifndef _ETH_H_
#define _ETH_H_

#include "./SYSTEM/sys/sys.h"

/* º¯ÊýÉùÃ÷ */
void eth_task_function(void);
int8_t eth_network_line_status_check(void);
void eth_ping_timer_function(void);
void eth_ping_detection_function(void);
void eth_tcp_connect_control_function(void);
void eth_set_tcp_connect_reset(void);
void eth_set_tcp_cmd(uint8_t cmd);

uint8_t eth_get_network_cable_status(void);
uint8_t eth_get_tcp_status(void);

void eth_udp_connect_control_function(void);

uint8_t eth_get_udp_status(void);
void eth_set_network_reset(void);
void eth_set_udp_connect_reset(void);

void eth_save_ipc_info(char *ip,uint8_t id);

void eth_snmp_connect_control_function(void);
void eth_snmp_connect_reset(void);

#endif


