/*
 * TASKS.c
 *
 *  Created on: Dec 19, 2025
 *      Author: Ahmed
 */


#include <stdio.h>
#include <stdint.h>

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "TASKS.h"
#include "RELAY.h"
#include "FP.h"

#include "App.h"

void TASKS_Init(void)
{
	APP_Init();
}


void FP_Main_Cyclic(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	//	FP_SetMode(FP_ENROLL_MODE);
	while(1)
	{
		FP_MainFunction_Cyclic();
		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(400));
	}
}

void TASKS_APP_Cyclic(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	//	FP_SetMode(FP_ENROLL_MODE);
	while(1)
	{
		APP_Cyclic();
		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(400));
	}
}


void TASKS_RELAY_Cyclic(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	//	FP_SetMode(FP_ENROLL_MODE);
	while(1)
	{
		RELAY_Time_Manager_Cyclic();
		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(1));
	}
}

//void Enter_SleepMode(void)
//{
//	vTaskSuspendAll();
//	HAL_SuspendTick();
//
//	for(uint32_t i = 0 ; i < 5000000 ;i++);
//	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_SLEEPENTRY_WFI );
//	for(uint32_t i = 0 ; i < 5000000 ;i++);
//}
//
//void EXTI9_5_IRQHandler(void)
//{
//	/* USER CODE BEGIN EXTI9_5_IRQn 0 */
//
//	/* USER CODE END EXTI9_5_IRQn 0 */
//	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
//	/* USER CODE BEGIN EXTI9_5_IRQn 1 */
//
//	/* USER CODE END EXTI9_5_IRQn 1 */
//}
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//	if(GPIO_Pin == GPIO_PIN_7)
//	{
//
//
//		int x = 0;
//		x++;
//	}
//}
//void TASKS_Led_Toggle(void *pram)
//{
//	GPIO_InitTypeDef GPIO_Init ;
//	GPIO_Init.Pin = GPIO_PIN_3;
//	GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
//	GPIO_Init.Pull = GPIO_NOPULL;
//
//	HAL_GPIO_Init(GPIOB, &GPIO_Init);
//
//	__HAL_RCC_GPIOB_CLK_ENABLE();
//
//	/*Configure GPIO pin : PB7 */
//	GPIO_Init.Pin = GPIO_PIN_7;
//	GPIO_Init.Mode = GPIO_MODE_IT_FALLING;
//	GPIO_Init.Pull = GPIO_PULLUP;
//	HAL_GPIO_Init(GPIOB, &GPIO_Init);
//
//	HAL_NVIC_SetPriority(EXTI9_5_IRQn,5, 0);
//	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
//	TickType_t last_wake = xTaskGetTickCount();
//	while(1)
//	{
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
//		for(uint8_t i = 0 ; i < 10 ;i++)
//		{
//			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
//			vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(1000));
//		}
//
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
//
//		printf("Enter Sleep Mode\n");
//
//		Enter_SleepMode();
//		xTaskResumeAll();
//		HAL_ResumeTick();
//		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(1000));
//		printf("wakeup via button pressed\n");
//	}
//
//}
