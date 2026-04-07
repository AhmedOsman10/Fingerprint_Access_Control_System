/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "USART_Prv.h"
#include "usb_host.h"
#include "Sys.h"
#include "USART.h"
#include "USART_Cfg.h"
#include "TASKS.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include "FP.h"

int main(void)
{
	Sys_Init();
	TASKS_Init();
	BaseType_t ret_st;

	ret_st = xTaskCreate(FP_Main_Cyclic, "FP_Main_Cyclic", 400, NULL, 2, NULL);
	configASSERT(ret_st == pdPASS);
//
	ret_st = xTaskCreate(FP_Test, "FP Test", 400, NULL, 2, NULL);
	configASSERT(ret_st == pdPASS);

//	ret_st = xTaskCreate(FP_Simple_Search_Task, "FP_Simple_Search_Task", 500, NULL, 2, NULL);
//	configASSERT(ret_st == pdPASS);

	vTaskStartScheduler();



	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		MX_USB_HOST_Process();
		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}


