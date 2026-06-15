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
#include "Sys.h"
#include "App.h"

RTC_HandleTypeDef hrtc;

/**
  * @brief RTC MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hrtc: RTC handle pointer
  * @retval None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    if(hrtc->Instance == RTC)
    {
        __HAL_RCC_PWR_CLK_ENABLE();
        HAL_PWR_EnableBkUpAccess();

        __HAL_RCC_LSI_ENABLE();
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET);

        __HAL_RCC_BACKUPRESET_FORCE();
        __HAL_RCC_BACKUPRESET_RELEASE();

        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_RCC_RTC_ENABLE();

        /* FIX: Enable the ALARM interrupt, not the WKUP interrupt! */
        HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
    }
}

static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
//  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
//  {
//    Error_Handler();
//  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void TASKS_Init(void)
{
	APP_Init();
	MX_RTC_Init();
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
//	__HAL_RCC_GPIOB_CLK_ENABLE();
//	GPIO_Init.Pin = GPIO_PIN_3;
//	GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
//	GPIO_Init.Pull = GPIO_NOPULL;
//
//	HAL_GPIO_Init(GPIOB, &GPIO_Init);
//
//
//	/*Configure GPIO pin : PB7 */
//	GPIO_Init.Pin = GPIO_PIN_7;
//	GPIO_Init.Mode = GPIO_MODE_IT_FALLING;
//	GPIO_Init.Pull = GPIO_PULLUP;
//	HAL_GPIO_Init(GPIOB, &GPIO_Init);
//
//	HAL_NVIC_SetPriority(EXTI9_5_IRQn,5, 0);
//	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
//
//	while(1)
//	{
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
//		for(uint8_t i = 0 ; i < 10 ;i++)
//		{
//			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
//			vTaskDelay(pdMS_TO_TICKS(1000));
//		}
//
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
//
//		printf("Enter Sleep Mode\n");
//
//		Enter_SleepMode();
//
//
////	Sys_Init();
//		xTaskResumeAll();
//		HAL_ResumeTick();
//		SysTick->CTRL  |= SysTick_CTRL_TICKINT_Msk;
//		vTaskDelay(pdMS_TO_TICKS(1000));
//		printf("wakeup via button pressed\n");
//	}
//
//}
