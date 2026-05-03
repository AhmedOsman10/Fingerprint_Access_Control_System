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
#include "USART.h"
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


void TASKS_USART_30ms(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	while(1)
	{
		USART_RxCyclic();
		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(1));
	}
}



void TASKS_USART_Tx_Cyclic(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	while(1)
	{
		USART_TxCyclic();
		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(1));
	}
}



void FP_Test(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	uint8_t match_st;
	uint16_t user_id;
	uint8_t enroll_flag = 0 ;
	while(1)
	{

		//		if(enroll_flag == 0)
		//		{
		//			FP_SetMode(FP_ENROLL_MODE);
		//			enroll_flag = 1;
		//		}
		//		else
		//		{

		if(FP_Get_User(&match_st,&user_id) == FP_GetUser_Ok)
		{
			if(match_st == FP_MATCH_ST)
			{
				printf("Access granted: user id %d\n" , user_id);

			}
			else
			{
				printf("Access Denied\n");

			}
		}
		//		}
		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(50));
	}
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
