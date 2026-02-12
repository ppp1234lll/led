/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include "BL0910.h"
#include "BL0910_2.h"
#include "BL0906.h"
#include "pack.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint8_t Usart1Rx[10];
uint8_t SPI1_tx[2] = {0x82,0x69};
uint8_t SPI1_rx[4] = {0};

uint8_t seng_state = 0;
uint8_t seng_state1 = 0;

uint8_t  send_buffer[256] = {0};
uint8_t  cmd = 0xA1;
uint32_t buffer_len = 0;

data_collection_t sg_datacollec_t;


int fputc(int ch, FILE *p)
{
	while(!(USART3->SR & (1<<7)));
	
	USART3->DR = ch;
	
	return ch;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	
  uint8_t index = 0;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	
//	__HAL_UART_ENABLE_IT(&huart1,UART_IT_IDLE);
//	__HAL_UART_CLEAR_IDLEFLAG(&huart1);
//	HAL_UART_Receive_DMA(&huart1,Usart1Rx,10);

  HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_Base_Start_IT(&htim1);

  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Usart1Rx, 20);
	
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
	
	bl0910_init_function();
//	bl0910_test();
	bl0910_2_init_function();
	bl0906_init_function();
//	bl0906_test();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		
		HAL_Delay(10);
		
//		HAL_SPI_Transmit(&hspi1, SPI1_tx, 2, HAL_MAX_DELAY);
//		HAL_SPI_Receive(&hspi1, SPI1_rx, 4, HAL_MAX_DELAY);
		
		bl0910_work_process_function();
		bl0910_2_work_process_function();
		bl0906_work_process_function();
		
//		voltage_sensor();
		
		if(seng_state == 1)
		{
			buffer_len = pack_data(cmd,&sg_datacollec_t,send_buffer);
			
			HAL_UART_Transmit(&huart1, send_buffer, buffer_len, 1000);
			seng_state = 0;
		}	
		if(seng_state1 == 1)
		{
			if(index == 0)
			{
				buffer_len = pack_data(cmd,&sg_datacollec_t,send_buffer);
				HAL_UART_Transmit(&huart1, send_buffer, buffer_len, 1000);
			}
			index++;
			if(index >= 30)
			{
				index = 0;
				seng_state1 = 0;
				for (int i = 0; i < 12; i++) 
				{
					sg_datacollec_t.pulse[i] = 0;
				}			
			}	
		}	
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/************************************************************
*
* Function name	: det_set_total_energy
* Description	: 电能计量参数
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_set_total_energy(uint8_t num,float data)
{
	switch(num)
	{
		case 1: sg_datacollec_t.current[0] = data / CURR_KP - 0.5f; 		break;
		case 2: sg_datacollec_t.current[1] = data / CURR_KP - 0.5f; 		break;
		case 3: sg_datacollec_t.current[2] = data / CURR_KP - 0.4f; 		break;
		case 4: sg_datacollec_t.current[3] = data / CURR_KP - 0.5f; 		break;
		case 5: sg_datacollec_t.current[4] = data / CURR_KP - 0.5f; 		break;
		case 6: sg_datacollec_t.current[5] = data / CURR_KP - 0.5f; 		break;
		case 7: sg_datacollec_t.current[6] = data / CURR_KP - 0.4f; 		break;
		case 8: sg_datacollec_t.current[7] = data / CURR_KP - 0.5f; 		break;
		case 9: sg_datacollec_t.current[8] = data / CURR_KP - 0.5f; 		break;
		case 10: sg_datacollec_t.current[9] = data / CURR_KP - 0.5f; 		break;
		case 11: sg_datacollec_t.current[10] = data / CURR_KP - 0.4f; 		break;
		case 12: sg_datacollec_t.current[11] = data / CURR_KP - 0.5f; 		break;
		case 13: sg_datacollec_t.current[12] = data / CURR_KP - 0.5f; 		break;
		case 14: sg_datacollec_t.current[13] = data / CURR_KP - 0.5f; 		break;
		case 15: sg_datacollec_t.current[14] = data / CURR_KP - 0.4f; 		break;
		case 16: sg_datacollec_t.current[15] = data / CURR_KP - 0.5f; 		break;
		case 17: sg_datacollec_t.current[16] = data / CURR_KP - 0.5f; 		break;
		case 18: sg_datacollec_t.current[17] = data / CURR_KP - 0.5f; 		break;
		case 19: sg_datacollec_t.current[18] = data / CURR_KP - 0.4f; 		break;
		case 20: sg_datacollec_t.current[19] = data / CURR_KP - 0.5f; 		break;
		case 21: sg_datacollec_t.current[20] = data / CURR_KP - 0.5f; 		break;
		case 22: sg_datacollec_t.current[21] = data / CURR_KP - 0.5f; 		break;
		case 23: sg_datacollec_t.current[22] = data / CURR_KP - 0.4f; 		break;
		case 24: sg_datacollec_t.current[23] = data / CURR_KP - 0.5f; 		break;
		
  }
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
