/**
 ****************************************************************************************************
 * @file        stmflash.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-06
 * @brief       STM32内部FLASH读写 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 阿波罗 H743开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20220906
 * 第一次发布
 *
 ****************************************************************************************************
 */

#ifndef __STMFLASH_H
#define __STMFLASH_H

#include "./SYSTEM/sys/sys.h"


/* FLASH起始地址 */
#define STM32_FLASH_SIZE        0X200000                    /* STM32 FLASH 总大小 */
#define STM32_FLASH_BASE        0x08000000                  /* STM32 FLASH 起始地址 */

#define FLASH_WAITETIME         50000                       /* FLASH等待超时时间 */

/* FLASH 扇区的起始地址,分2个bank,每个bank 1MB */
/* BANK1 */
#define BANK1_FLASH_SECTOR_0    ((uint32_t)0x08000000)      /* Bank1扇区0起始地址, 128 Kbytes */
#define BANK1_FLASH_SECTOR_1    ((uint32_t)0x08020000)      /* Bank1扇区1起始地址, 128 Kbytes */
#define BANK1_FLASH_SECTOR_2    ((uint32_t)0x08040000)      /* Bank1扇区2起始地址, 128 Kbytes */
#define BANK1_FLASH_SECTOR_3    ((uint32_t)0x08060000)      /* Bank1扇区3起始地址, 128 Kbytes */
#define BANK1_FLASH_SECTOR_4    ((uint32_t)0x08080000)      /* Bank1扇区4起始地址, 128 Kbytes */
#define BANK1_FLASH_SECTOR_5    ((uint32_t)0x080A0000)      /* Bank1扇区5起始地址, 128 Kbytes */
#define BANK1_FLASH_SECTOR_6    ((uint32_t)0x080C0000)      /* Bank1扇区6起始地址, 128 Kbytes */
#define BANK1_FLASH_SECTOR_7    ((uint32_t)0x080E0000)      /* Bank1扇区7起始地址, 128 Kbytes */

/* BNAK2 */
#define BANK2_FLASH_SECTOR_0    ((uint32_t)0x08100000)      /* Bank2扇区0起始地址, 128 Kbytes */
#define BANK2_FLASH_SECTOR_1    ((uint32_t)0x08120000)      /* Bank2扇区1起始地址, 128 Kbytes */
#define BANK2_FLASH_SECTOR_2    ((uint32_t)0x08140000)      /* Bank2扇区2起始地址, 128 Kbytes */
#define BANK2_FLASH_SECTOR_3    ((uint32_t)0x08160000)      /* Bank2扇区3起始地址, 128 Kbytes */
#define BANK2_FLASH_SECTOR_4    ((uint32_t)0x08180000)      /* Bank2扇区4起始地址, 128 Kbytes */
#define BANK2_FLASH_SECTOR_5    ((uint32_t)0x081A0000)      /* Bank2扇区5起始地址, 128 Kbytes */
#define BANK2_FLASH_SECTOR_6    ((uint32_t)0x081C0000)      /* Bank2扇区6起始地址, 128 Kbytes */
#define BANK2_FLASH_SECTOR_7    ((uint32_t)0x081E0000)      /* Bank2扇区7起始地址, 128 Kbytes */

/******************************************************************************************/

uint32_t stmflash_read_word(uint32_t faddr);                             /* 读出字 */
void stmflash_write(uint32_t waddr, uint32_t *pbuf, uint32_t length);    /* 从指定地址开始写入指定长度的数据 */
void stmflash_read(uint32_t raddr, uint32_t *pbuf, uint32_t length);     /* 从指定地址开始读出指定长度的数据 */

void test_write(uint32_t waddr, uint32_t wdata);   /* 测试写入 */

#endif

