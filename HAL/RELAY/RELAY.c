/*
 * RELAY.c
 *
 *  Created on: 28 Apr 2026
 *      Author: Ahmed
 *
 *  Relay driver implementation.
 *
 *  This file implements:
 *    - relay GPIO initialization
 *    - relay ON/OFF control
 *    - timed relay ON operation
 *    - cyclic timeout management
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


/******************************************************************************************
 *                                  RELAY_Init()
 *
 *  Initialize selected relay GPIO.
 *
 *  Description:
 *    - Validates relay number.
 *    - Configures the selected relay GPIO pin as output push-pull.
 *
 *  Parameters:
 *    Relay_num: Relay number/index.
 *
 *  Returns:
 *    RELAY_Init_Success: Relay GPIO initialized successfully.
 *    RElAY_Invalid_Args: Invalid relay number.
 ******************************************************************************************/
RELAY_Err_St_t RELAY_Init(uint8_t Relay_num)
{
	/* Default return assumes initialization failure */
	RELAY_Err_St_t RELAY_Err_St = RELAY_Init_Failed;

	/* Validate relay number before accessing configuration table */
	if(Relay_num >= RELAY_MAX_NUM)
	{
		RELAY_Err_St = RElAY_Invalid_Args;
	}
	else
	{
		/* Configure selected relay pin as GPIO output.
		 *
		 * Pin   : comes from RELAY_Config[]
		 * Mode  : push-pull output
		 * Pull  : no internal pull-up/pull-down
		 * Speed : medium GPIO speed
		 */
		GPIO_InitTypeDef GPIO_Init =
		{
			.Pin   = RELAY_Config[Relay_num].Pin_Num,
			.Mode  = GPIO_MODE_OUTPUT_PP,
			.Pull  = GPIO_NOPULL,
			.Speed = GPIO_SPEED_FREQ_MEDIUM
		};

		/* Initialize the GPIO port/pin for selected relay */
		HAL_GPIO_Init(RELAY_Config[Relay_num].Port_Num, &GPIO_Init);

		RELAY_Err_St = RELAY_Init_Success;
	}

	return RELAY_Err_St;
}


/******************************************************************************************
 *                                  RELAY_On()
 *
 *  Turn selected relay ON.
 *
 *  Description:
 *    - Validates relay number.
 *    - Writes GPIO_PIN_SET to the configured relay pin.
 *
 *  Parameters:
 *    Relay_num: Relay number/index.
 *
 *  Returns:
 *    RELAY_On_Ok:        Relay turned ON.
 *    RElAY_Invalid_Args: Invalid relay number.
 ******************************************************************************************/
RELAY_Err_St_t RELAY_On(uint8_t Relay_num)
{
	/* Validate relay number */
	if(Relay_num >= RELAY_MAX_NUM)
	{
		return RElAY_Invalid_Args;
	}

	/* Set relay output pin HIGH */
	HAL_GPIO_WritePin(RELAY_Config[Relay_num].Port_Num, RELAY_Config[Relay_num].Pin_Num, GPIO_PIN_SET);

	return RELAY_On_Ok;
}


/******************************************************************************************
 *                                  RELAY_Off()
 *
 *  Turn selected relay OFF.
 *
 *  Description:
 *    - Validates relay number.
 *    - Writes GPIO_PIN_RESET to the configured relay pin.
 *
 *  Parameters:
 *    Relay_num: Relay number/index.
 *
 *  Returns:
 *    RELAY_Off_Ok:       Relay turned OFF.
 *    RElAY_Invalid_Args: Invalid relay number.
 ******************************************************************************************/
RELAY_Err_St_t RELAY_Off(uint8_t Relay_num)
{
	/* Validate relay number */
	if(Relay_num >= RELAY_MAX_NUM)
	{
		return RElAY_Invalid_Args;
	}

	/* Set relay output pin LOW */
	HAL_GPIO_WritePin(RELAY_Config[Relay_num].Port_Num, RELAY_Config[Relay_num].Pin_Num, GPIO_PIN_RESET);

	return RELAY_Off_Ok;
}


/******************************************************************************************
 *                              RELAY_On_With_Time()
 *
 *  Turn selected relay ON for a defined duration.
 *
 *  Description:
 *    - Validates relay number.
 *    - Turns relay ON immediately.
 *    - Starts software timing using FreeRTOS tick count.
 *
 *  Parameters:
 *    Relay_num:      Relay number/index.
 *    Relay_Time_ms:  Required ON time in milliseconds.
 *
 *  Internal Behavior:
 *    - Stores start tick.
 *    - Converts requested time from milliseconds to RTOS ticks.
 *    - Marks relay timing as active.
 *
 *  Returns:
 *    RELAY_On_Ok:        Relay turned ON and timing started.
 *    RElAY_Invalid_Args: Invalid relay number.
 *
 *  Note:
 *    RELAY_Time_Manager_Cyclic() must be called periodically to turn
 *    the relay OFF after the timeout expires.
 ******************************************************************************************/
RELAY_Err_St_t RELAY_On_With_Time(uint8_t Relay_num , uint32_t Relay_Time_ms)
{
	/* Validate relay number */
	if(Relay_num >= RELAY_MAX_NUM)
	{
		return RElAY_Invalid_Args;
	}

	/* Turn relay ON immediately */
	RELAY_On(Relay_num);

	/* Mark timed control as active */
	Relay_Control_Time[Relay_num].is_active = true;

	/* Save current RTOS tick as start time */
	Relay_Control_Time[Relay_num].start_time = xTaskGetTickCount();

	/* Convert requested milliseconds to RTOS ticks */
	Relay_Control_Time[Relay_num].deadline_ticks = pdMS_TO_TICKS(Relay_Time_ms);

	return RELAY_On_Ok;
}


/******************************************************************************************
 *                          RELAY_Time_Manager_Cyclic()
 *
 *  Manage timed relay OFF operation.
 *
 *  Description:
 *    - Checks all configured relays.
 *    - If a relay is active in timed mode, calculates elapsed time.
 *    - Turns relay OFF once the configured duration has expired.
 *
 *  Integration:
 *    - Must be called periodically from:
 *        - super loop
 *        - scheduler
 *        - RTOS task
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void RELAY_Time_Manager_Cyclic(void)
{
	/* Get current RTOS tick count once at the start of function */
	uint32_t curr_time = xTaskGetTickCount();

	/* Check all configured relays */
	for(uint8_t Relay_num = 0; Relay_num < RELAY_MAX_NUM; Relay_num++)
	{
		/* Only process relays currently active in timed mode */
		if(Relay_Control_Time[Relay_num].is_active == true)
		{
			/* Calculate elapsed ticks.
			 *
			 * Unsigned subtraction works correctly across tick overflow
			 * as long as the timeout value is within valid tick range.
			 */
			uint32_t elapsed_time = curr_time - Relay_Control_Time[Relay_num].start_time;

			/* If timeout expired, turn relay OFF */
			if(elapsed_time > Relay_Control_Time[Relay_num].deadline_ticks)
			{
				RELAY_Off(Relay_num);

				/* Mark timed operation as inactive */
				Relay_Control_Time[Relay_num].is_active = false;
			}
		}
	}
}
