/*
 * TASKS.c
 *
 *  Created on: Dec 19, 2025
 *      Author: Ahmed
 *
 *  OS Task Layer Implementation.
 *
 *  This file defines the FreeRTOS task functions that drive the system's
 *  cyclic behavior. It handles the periodic execution of:
 *    - USART communication (RX/TX)
 *    - Fingerprint sensor state machine
 *    - Application logic
 *    - Relay timing management
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

/******************************************************************************************
 *                              TASKS_Init()
 *
 *  Initializes the system before the OS scheduler starts.
 ******************************************************************************************/
void TASKS_Init(void)
{
	/* Initialize Application layer which in turn initializes drivers (USART, RTC, FP, RELAY) */
	APP_Init();
}


/******************************************************************************************
 *                              TASKS_USART_30ms()
 *
 *  High-frequency task to handle incoming USART data.
 *  Period: 1ms (Highest priority for data integrity)
 ******************************************************************************************/
void TASKS_USART_30ms(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	while(1)
	{
		/* Poll and parse raw bytes from USART hardware registers into driver buffers */
		USART_RxCyclic();

		/* Delay task to maintain a 1ms period */
		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(1));
	}
}


/******************************************************************************************
 *                              TASKS_USART_Tx_Cyclic()
 *
 *  High-frequency task to handle outgoing USART data.
 *  Period: 1ms
 ******************************************************************************************/
void TASKS_USART_Tx_Cyclic(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	while(1)
	{
		/* Process the transmission buffer and send bytes to the UART hardware */
		USART_TxCyclic();

		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(1));
	}
}


/******************************************************************************************
 *                              FP_Test()
 *
 *  Debug/Test task for verifying fingerprint matches via console.
 *  Period: 50ms
 ******************************************************************************************/
void FP_Test(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	uint8_t match_st;
	uint16_t user_id;

	while(1)
	{
		/* Check if a user identification result is available from the driver */
		if(FP_Get_User(&match_st,&user_id) == FP_GetUser_Ok)
		{
			if(match_st == FP_MATCH_ST)
			{
				/* Output match result to ITM/Console */
				printf("Access granted: user id %d\n" , user_id);
			}
			else
			{
				printf("Access Denied\n");
			}
		}

		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(50));
	}
}

/******************************************************************************************
 *                              FP_Main_Cyclic()
 *
 *  Handles the low-level Fingerprint sensor communication state machine.
 *  Period: 400ms
 ******************************************************************************************/
void FP_Main_Cyclic(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	while(1)
	{
		/* Drive the internal state machine of the fingerprint driver (Handshake, Read, etc.) */
		FP_MainFunction_Cyclic();

		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(400));
	}

}

/******************************************************************************************
 *                              TASKS_APP_Cyclic()
 *
 *  Main Application logic task.
 *  Period: 400ms (Synchronized with FP_Main_Cyclic)
 ******************************************************************************************/
void TASKS_APP_Cyclic(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	while(1)
	{
		/* Execute high-level logic: command processing, logging, and coordination */
		APP_Cyclic();

		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(400));
	}
}


/******************************************************************************************
 *                              TASKS_RELAY_Cyclic()
 *
 *  Monitors relay timing for auto-off functionality.
 *  Period: 1ms
 ******************************************************************************************/
void TASKS_RELAY_Cyclic(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	while(1)
	{
		/* Update relay timers and turn off relays if their set time has expired */
		RELAY_Time_Manager_Cyclic();

		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(1));
	}
}
