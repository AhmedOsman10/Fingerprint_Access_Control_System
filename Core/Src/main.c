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
#include "usb_host.h"
#include "Sys.h"
#include "TASKS.h"
#include "FreeRTOS.h"
#include "task.h"
void Enter_SleepMode(void)
{
	//		vTaskSuspendAll();
	HAL_SuspendTick();

	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_SLEEPENTRY_WFI );
}

void EXTI9_5_IRQHandler(void)
{
	/* USER CODE BEGIN EXTI9_5_IRQn 0 */

	/* USER CODE END EXTI9_5_IRQn 0 */
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
	/* USER CODE BEGIN EXTI9_5_IRQn 1 */

	/* USER CODE END EXTI9_5_IRQn 1 */
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_7)
	{


		int x = 0;
		x++;
	}
}

int main(void)
{
	Sys_Init();
	//	TASKS_Init();
	BaseType_t ret_st;

	//	ret_st = xTaskCreate(FP_Main_Cyclic, "FP_Main_Cyclic", 100, NULL, 2, NULL);
	//	configASSERT(ret_st == pdPASS);
	//	//
	//	ret_st = xTaskCreate(TASKS_APP_Cyclic, "TASKS_APP_Cyclic", 400, NULL, 2, NULL);
	//	configASSERT(ret_st == pdPASS);
	//
	//	ret_st = xTaskCreate(TASKS_RELAY_Cyclic, "TASKS_RELAY_Cyclic", 400, NULL,2, NULL);
	//	configASSERT(ret_st == pdPASS);


	//	ret_st = xTaskCreate(TASKS_Led_Toggle, "TASKS_Led_Toggle", 400, NULL,2, NULL);
	//	configASSERT(ret_st == pdPASS);
	//
	//	vTaskStartScheduler();

	GPIO_InitTypeDef GPIO_Init ;
	GPIO_Init.Pin = GPIO_PIN_3;
	GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Init.Pull = GPIO_NOPULL;

	HAL_GPIO_Init(GPIOB, &GPIO_Init);

	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin : PB7 */
	GPIO_Init.Pin = GPIO_PIN_7;
	GPIO_Init.Mode = GPIO_MODE_IT_FALLING;
	GPIO_Init.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_Init);

	HAL_NVIC_SetPriority(EXTI9_5_IRQn,5, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
	while(1)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
		for(uint8_t i = 0 ; i < 10 ;i++)
		{
			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
			HAL_Delay(1000);
		}

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);


		Enter_SleepMode();
		HAL_ResumeTick();
	}





	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		MX_USB_HOST_Process();
		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}


