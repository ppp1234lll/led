#ifndef _ETH_H_
#define _ETH_H_

#include "sys.h"
#include "lwip/dhcp.h"
#include "lwip/timers.h"
#include "lwip/tcp_impl.h"
#include "lwip/ip_frag.h"
#include "lwip/tcpip.h" 

/* 参数 */
#define IPC_NUM_MAX  10 // Maximum number of IPCs

typedef struct
{
	uint8_t search_flag; // Search flag bit (bit 0: 0=stop, 1=search)
	uint8_t ipc_num;  // Device count
	uint8_t ip[IPC_NUM_MAX][4];
}IPC_Info_t; // 摄像机信息
extern IPC_Info_t ipcInfo; // IPC information header structure

/* 函数声明 */
void eth_network_line_status_detection_function(void);
int8_t eth_network_line_status_check(void);
void eth_ping_timer_function(void);
void eth_ping_detection_function(void);
void dns_serverFound(const char *name, struct ip_addr *ipaddr, void *arg);
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


