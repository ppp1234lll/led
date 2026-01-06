/*
*********************************************************************************************************
*
*	模块名称 : cpu内部falsh操作模块(for STM32H743 H750)
*	文件名称 : bsp_cpu_flash.c
*	版    本 : V1.1
*	说    明 : 提供读写CPU内部Flash的函数
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2019-09-20  armfly  正式发布
*		V1.1    2019-10-03  armfly  写flash函数，修正长度不是32字节整数倍的bug，末尾补0写入。
*									解决HAL库函数的 HAL_FLASH_Program（）的bug，
*									问题现象是大批量连续编程失败（报编程指令顺序错， PGSERR1、 PGSERR2）
*	Copyright (C), 2019-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#include "bsp.h"
#include "bsp_cpu_flash.h"

/*
*********************************************************************************************************
*	函 数 名: bsp_GetSector
*	功能说明: 根据地址计算扇区首地址
*	形    参: 无
*	返 回 值: 扇区号（0-7)
*********************************************************************************************************
*/
uint32_t bsp_GetSector(uint32_t Address)
{
	uint32_t sector = 0;

	if (((Address < ADDR_FLASH_SECTOR_1_BANK1) && (Address >= ADDR_FLASH_SECTOR_0_BANK1)) || \
		((Address < ADDR_FLASH_SECTOR_1_BANK2) && (Address >= ADDR_FLASH_SECTOR_0_BANK2)))    
	{
		sector = FLASH_SECTOR_0;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_2_BANK1) && (Address >= ADDR_FLASH_SECTOR_1_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_2_BANK2) && (Address >= ADDR_FLASH_SECTOR_1_BANK2)))    
	{
		sector = FLASH_SECTOR_1;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_3_BANK1) && (Address >= ADDR_FLASH_SECTOR_2_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_3_BANK2) && (Address >= ADDR_FLASH_SECTOR_2_BANK2)))    
	{
		sector = FLASH_SECTOR_2;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_4_BANK1) && (Address >= ADDR_FLASH_SECTOR_3_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_4_BANK2) && (Address >= ADDR_FLASH_SECTOR_3_BANK2)))    
	{
		sector = FLASH_SECTOR_3;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_5_BANK1) && (Address >= ADDR_FLASH_SECTOR_4_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_5_BANK2) && (Address >= ADDR_FLASH_SECTOR_4_BANK2)))    
	{
		sector = FLASH_SECTOR_4;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_6_BANK1) && (Address >= ADDR_FLASH_SECTOR_5_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_6_BANK2) && (Address >= ADDR_FLASH_SECTOR_5_BANK2)))    
	{
		sector = FLASH_SECTOR_5;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_7_BANK1) && (Address >= ADDR_FLASH_SECTOR_6_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_7_BANK2) && (Address >= ADDR_FLASH_SECTOR_6_BANK2)))    
	{
		sector = FLASH_SECTOR_6;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_0_BANK2) && (Address >= ADDR_FLASH_SECTOR_7_BANK1)) || \
	  ((Address < CPU_FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7_BANK2)))
	{
		sector = FLASH_SECTOR_7;  
	}
	else
	{
		sector = FLASH_SECTOR_7;
	}

	return sector;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_ReadCpuFlash
*	功能说明: 读取CPU Flash的内容
*	形    参:  _ucpDst : 目标缓冲区
*			 _ulFlashAddr : 起始地址
*			 _ulSize : 数据大小（单位是字节）
*	返 回 值: 0=成功，1=失败
*********************************************************************************************************
*/
uint8_t bsp_ReadCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpDst, uint32_t _ulSize)
{
	uint32_t i;

	if (_ulFlashAddr + _ulSize > CPU_FLASH_BASE_ADDR + CPU_FLASH_SIZE)
	{
		return 1;
	}

	/* 长度为0时不继续操作,否则起始地址为奇地址会出错 */
	if (_ulSize == 0)
	{
		return 1;
	}

	for (i = 0; i < _ulSize; i++)
	{
		*_ucpDst++ = *(uint8_t *)_ulFlashAddr++;
	}

	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_CmpCpuFlash
*	功能说明: 比较Flash指定地址的数据.
*	形    参: _ulFlashAddr : Flash地址
*			 _ucpBuf : 数据缓冲区
*			 _ulSize : 数据大小（单位是字节）
*	返 回 值:
*			FLASH_IS_EQU		0   Flash内容和待写入的数据相等，不需要擦除和写操作
*			FLASH_REQ_WRITE		1	Flash不需要擦除，直接写
*			FLASH_REQ_ERASE		2	Flash需要先擦除,再写
*			FLASH_PARAM_ERR		3	函数参数错误
*********************************************************************************************************
*/
uint8_t bsp_CmpCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpBuf, uint32_t _ulSize)
{
	uint32_t i;
	uint8_t ucIsEqu;	/* 相等标志 */
	uint8_t ucByte;

	/* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
	if (_ulFlashAddr + _ulSize > CPU_FLASH_BASE_ADDR + CPU_FLASH_SIZE)
	{
		return FLASH_PARAM_ERR;		/*　函数参数错误　*/
	}

	/* 长度为0时返回正确 */
	if (_ulSize == 0)
	{
		return FLASH_IS_EQU;		/* Flash内容和待写入的数据相等 */
	}

	ucIsEqu = 1;			/* 先假设所有字节和待写入的数据相等，如果遇到任何一个不相等，则设置为 0 */
	for (i = 0; i < _ulSize; i++)
	{
		ucByte = *(uint8_t *)_ulFlashAddr;

		if (ucByte != *_ucpBuf)
		{
			if (ucByte != 0xFF)
			{
				return FLASH_REQ_ERASE;		/* 需要擦除后再写 */
			}
			else
			{
				ucIsEqu = 0;	/* 不相等，需要写 */
			}
		}

		_ulFlashAddr++;
		_ucpBuf++;
	}

	if (ucIsEqu == 1)
	{
		return FLASH_IS_EQU;	/* Flash内容和待写入的数据相等，不需要擦除和写操作 */
	}
	else
	{
		return FLASH_REQ_WRITE;	/* Flash不需要擦除，直接写 */
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_EraseCpuFlash
*	功能说明: 擦除CPU FLASH一个扇区 （128KB)
*	形    参: _ulFlashAddr : Flash地址
*	返 回 值: 0 成功， 1 失败
*			  HAL_OK       = 0x00,
*			  HAL_ERROR    = 0x01,
*			  HAL_BUSY     = 0x02,
*			  HAL_TIMEOUT  = 0x03
*
*********************************************************************************************************
*/
uint8_t bsp_EraseCpuFlash(uint32_t _ulFlashAddr)
{
	uint32_t FirstSector = 0, NbOfSectors = 0;
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError = 0;
	uint8_t re;

	/* 解锁 */
	HAL_FLASH_Unlock();
	
	/* 获取此地址所在的扇区 */
	FirstSector = bsp_GetSector(_ulFlashAddr);
	
	/* 固定1个扇区 */
	NbOfSectors = 1;	

	/* 擦除扇区配置 */
	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
	
	if (_ulFlashAddr >= ADDR_FLASH_SECTOR_0_BANK2)
	{
		EraseInitStruct.Banks         = FLASH_BANK_2;
	}
	else
	{
		EraseInitStruct.Banks         = FLASH_BANK_1;
	}
	
	EraseInitStruct.Sector        = FirstSector;
	EraseInitStruct.NbSectors     = NbOfSectors;
	
	/* 扇区擦除 */	
	re = HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError);
	
	/* 擦除完毕后，上锁 */
	HAL_FLASH_Lock();	
	
	return re;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_WriteCpuFlash
*	功能说明: 写数据到CPU 内部Flash。 必须按32字节整数倍写。不支持跨扇区。扇区大小128KB. \
*			  写之前需要擦除扇区. 长度不是32字节整数倍时，最后几个字节末尾补0写入.
*	形    参: _ulFlashAddr : Flash地址
*			 _ucpSrc : 数据缓冲区
*			 _ulSize : 数据大小（单位是字节, 必须是32字节整数倍）
*	返 回 值: 0-成功，1-数据长度或地址溢出，2-写Flash出错(估计Flash寿命到)
*********************************************************************************************************
*/
uint8_t bsp_WriteCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize)
{
	uint32_t i;
	uint8_t ucRet;

	/* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
	if (_ulFlashAddr + _ulSize > CPU_FLASH_BASE_ADDR + CPU_FLASH_SIZE)
	{
		return 1;
	}

	/* 长度为0时不继续操作  */
	if (_ulSize == 0)
	{
		return 0;
	}

	ucRet = bsp_CmpCpuFlash(_ulFlashAddr, _ucpSrc, _ulSize);

	if (ucRet == FLASH_IS_EQU)
	{
		return 0;
	}

	__set_PRIMASK(1);  		/* 关中断 */

	/* FLASH 解锁 */
	HAL_FLASH_Unlock();

	for (i = 0; i < _ulSize / 32; i++)	
	{
		uint64_t FlashWord[4];
		
		memcpy((char *)FlashWord, _ucpSrc, 32);
		_ucpSrc += 32;
		
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, _ulFlashAddr, (uint64_t)((uint32_t)FlashWord)) == HAL_OK)
		{
			_ulFlashAddr = _ulFlashAddr + 32; /* 递增，操作下一个256bit */				
		}		
		else
		{
			goto err;
		}
	}
	
	/* 长度不是32字节整数倍 */
	if (_ulSize % 32)
	{
		uint64_t FlashWord[4];
		
		FlashWord[0] = 0;
		FlashWord[1] = 0;
		FlashWord[2] = 0;
		FlashWord[3] = 0;
		memcpy((char *)FlashWord, _ucpSrc, _ulSize % 32);
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, _ulFlashAddr, (uint64_t)((uint32_t)FlashWord)) == HAL_OK)
		{
			; // _ulFlashAddr = _ulFlashAddr + 32;		
		}		
		else
		{
			goto err;
		}
	}
	
  	/* Flash 加锁，禁止写Flash控制寄存器 */
  	HAL_FLASH_Lock();

  	__set_PRIMASK(0);  		/* 开中断 */

	return 0;
	
err:
  	/* Flash 加锁，禁止写Flash控制寄存器 */
  	HAL_FLASH_Lock();

  	__set_PRIMASK(0);  		/* 开中断 */

	return 1;
}


#define STM_SECTOR_SIZE	  512  // 最多是2K字节
uint8_t cpuflash_save_buf[STM_SECTOR_SIZE];

void cpuflash_write_save(uint32_t ReadAddr,uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite)	
{
	uint32_t secoff;	   //扇区内偏移地址(32位字计算)
 	uint16_t i; 
	secoff = WriteAddr - ReadAddr;		//扇区内偏移地址
	bsp_ReadCpuFlash(ReadAddr, cpuflash_save_buf, STM_SECTOR_SIZE); //读出2K扇区的内容
	for(i=0;i<NumToWrite;i++)//复制
	{
		cpuflash_save_buf[i+secoff]=pBuffer[i];	  
	}
	iwdg_feed();
	bsp_EraseCpuFlash((uint32_t)ReadAddr);
	bsp_WriteCpuFlash(ReadAddr, cpuflash_save_buf,STM_SECTOR_SIZE);
}

/* 全局参数 */
typedef struct
{
	uint8_t   ParamVer;			
	uint16_t  ucBackLight;
	uint32_t  Baud485;
	float     ucRadioMode;		
	
	uint8_t   array[6];	
}
PARAM_T;
/*
*********************************************************************************************************
*	函 数 名: DemoFlash
*	功能说明: 串行EEPROM读写例程
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void DemoFlash(void)
{
	char ch  ;
	uint8_t  ucTest, *ptr8;
	uint16_t uiTest, *ptr16;
	uint32_t ulTest, *ptr32;
	PARAM_T tPara, *paraptr;
  uint8_t read_flash[64] = {0};
	
	/* 初始化数据 */
	tPara.Baud485 = 0x5555AAAA;
	tPara.ParamVer = 0x99;
	tPara.ucBackLight = 0x7788;
	tPara.ucRadioMode = 99.99f;
	tPara.array[0] = 0x01;
	tPara.array[1] = 0x02;
	tPara.array[2] = 0x03;
	tPara.array[3] = 0x04;
	tPara.array[4] = 0x05;	
	tPara.array[5] = 0x06;	
	
	uint32_t para_flash_area = 0X081E0000;
	printf("DemoFlash 2\r\n");
	while(1)
	{
		ch=getchar();
		printf("接收到字符：%c\n",ch);
		{
			switch (ch)
			{
				case '1':
					/*
						1、对于同一个地址空间，仅支持一次编程(不推荐二次编程，即使是将相应bit由数值1编程0)。
				        2、只能对已经擦除的空间做编程，擦除1个扇区是128KB。
				        3、H7的Flash编程时，务必保证要编程的地址是32字节对齐的，即此地址对32求余为0。并且编程的数据必须32字节整数倍。
				           函数bsp_WriteCpuFlash对字节数不够32字节整数倍的情况自动补0。
					*/
					/* 擦除扇区 */
					bsp_EraseCpuFlash((uint32_t)para_flash_area);
				
					ucTest = 0xAA;
					uiTest = 0x55AA;
					ulTest = 0x11223344;
					
					/* 扇区写入数据 */
					bsp_WriteCpuFlash((uint32_t)para_flash_area + 32*0,  (uint8_t *)&ucTest, sizeof(ucTest));
					bsp_WriteCpuFlash((uint32_t)para_flash_area + 32*1,  (uint8_t *)&uiTest, sizeof(uiTest));
					bsp_WriteCpuFlash((uint32_t)para_flash_area + 32*2,  (uint8_t *)&ulTest, sizeof(ulTest));				
					
					/* 读出数据并打印 */
					ptr8  = (uint8_t  *)(para_flash_area + 32*0);
					ptr16 = (uint16_t *)(para_flash_area + 32*1);
					ptr32 = (uint32_t *)(para_flash_area + 32*2);
				
					printf("写入数据：ucTest = %x, uiTest = %x, ulTest = %x\r\n", ucTest, uiTest, ulTest);
					printf("读取数据：ptr8 = %x, ptr16 = %x, ptr32 = %x\r\n", *ptr8, *ptr16, *ptr32);
					
					break;

				case '2':			/* K2键按下， 将结构体数据写入到内部Flash */
					/* 擦除扇区 */
					bsp_EraseCpuFlash((uint32_t)para_flash_area);

					/* 扇区写入数据 */
					bsp_WriteCpuFlash((uint32_t)para_flash_area,  (uint8_t *)&tPara, sizeof(tPara));			
					
					/* 读出数据并打印 */
					paraptr  = (PARAM_T  *)((uint32_t)para_flash_area);
				  bsp_ReadCpuFlash(para_flash_area, read_flash, 64);

					printf("写入数据：Baud485=%x, ParamVer=%x, ucBackLight=%x, ucRadioMode=%f\r\n", 
																				tPara.Baud485,
																				tPara.ParamVer,
																				tPara.ucBackLight,
																				tPara.ucRadioMode);
				
					printf("读取数据：Baud485=%x, ParamVer=%x, ucBackLight=%x, ucRadioMode=%f\r\n", 
																				paraptr->Baud485,
																				paraptr->ParamVer,
																				paraptr->ucBackLight,
																				paraptr->ucRadioMode);
					break;	

				case '3':			/* K2键按下， 将结构体数据写入到内部Flash */

				  cpuflash_write_save(para_flash_area,para_flash_area+32,(uint8_t *)&tPara, sizeof(tPara));
				
				  paraptr  = (PARAM_T  *)((uint32_t)(para_flash_area+32));
					printf("读取数据：Baud485=%x, ParamVer=%x, ucBackLight=%x, ucRadioMode=%f\r\n", 
																				paraptr->Baud485,
																				paraptr->ParamVer,
																				paraptr->ucBackLight,
																				paraptr->ucRadioMode);				
				break;
			}
		}
	}
}


/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
