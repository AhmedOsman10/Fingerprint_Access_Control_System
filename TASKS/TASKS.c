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
