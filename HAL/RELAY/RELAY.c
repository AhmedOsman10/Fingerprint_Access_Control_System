/*
 * RELAY.c
 *
 *  Created on: 28 Apr 2026
 *      Author: Ahmed
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "RELAY_Cfg.h"
#include "RELAY_Prv.h"
#include "RELAY.h"


RELAY_Err_St_t RELAY_Init(uint8_t Relay_num)
{
	RELAY_Err_St_t RELAY_Err_St = RELAY_Init_Failed;

	if(Relay_num >= RELAY_MAX_NUM)
	{
		RELAY_Err_St =  RElAY_Invalid_Args;
	}
	else
	{
		GPIO_InitTypeDef GPIO_Init = {.Pin = RELAY_Config[Relay_num].Pin_Num, .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_MEDIUM};

		HAL_GPIO_Init(RELAY_Config[Relay_num].Port_Num, &GPIO_Init);

		RELAY_Err_St = RELAY_Init_Success;
	}

	return RELAY_Err_St;
}

RELAY_Err_St_t RELAY_On(uint8_t Relay_num)
{
	if(Relay_num >= RELAY_MAX_NUM)
	{
		return RElAY_Invalid_Args;
	}

	HAL_GPIO_WritePin(RELAY_Config[Relay_num].Port_Num, RELAY_Config[Relay_num].Pin_Num, GPIO_PIN_SET);

	return RELAY_On_Ok;
}
RELAY_Err_St_t RELAY_Off(uint8_t Relay_num)
{
	if(Relay_num >= RELAY_MAX_NUM)
	{
		return RElAY_Invalid_Args;
	}

	HAL_GPIO_WritePin(RELAY_Config[Relay_num].Port_Num, RELAY_Config[Relay_num].Pin_Num, GPIO_PIN_RESET);

	return RELAY_Off_Ok;
}

RELAY_Err_St_t RELAY_On_With_Time(uint8_t Relay_num , uint32_t Relay_Time_ms)
{

	if(Relay_num >= RELAY_MAX_NUM)
	{
		return RElAY_Invalid_Args;
	}

	RELAY_On(Relay_num);
	Relay_Control_Time[Relay_num].is_active = true;
	Relay_Control_Time[Relay_num].start_time = xTaskGetTickCount();
	Relay_Control_Time[Relay_num].deadline_ticks = pdMS_TO_TICKS(Relay_Time_ms);

	return RELAY_On_Ok;
}

void RELAY_Time_Manager_Cyclic(void)
{

	uint32_t curr_time = xTaskGetTickCount();
	for(uint8_t Relay_num =0 ;Relay_num < RELAY_MAX_NUM ; Relay_num++)
	{
		if(Relay_Control_Time[Relay_num].is_active == true)
		{
			uint32_t elapsed_time = curr_time - Relay_Control_Time[Relay_num].start_time;

			if(elapsed_time > Relay_Control_Time[Relay_num].deadline_ticks)
			{
				RELAY_Off(Relay_num);
				Relay_Control_Time[Relay_num].is_active = false;
			}
		}
	}
}



//void RELAY_Time_Manager_Cyclic(void)
//{
//
//	for(uint8_t Relay_num =0 ;Relay_num < RELAY_MAX_NUM ; Relay_num++)
//	{
//		if(Relay_Control_Time[Relay_num].is_active == true)
//		{
//
//			// incremnet counter[Relay_num]++
//			if(counter[Relay_num] > 5)
//			{
//				RELAY_Off(Relay_num);
//				Relay_Control_Time[Relay_num].is_active == false;
//				counter[Relay_num] = 0;
//			}
//		}
//	}
//}
