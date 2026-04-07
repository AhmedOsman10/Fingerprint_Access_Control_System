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
#include "FP.h"

void TASKS_Init(void)
{
	FP_Init();
}

void TASKS_Print_Usart_Rx_Msg(void *pram)
{
	uint8_t Rx_data = 0;
	while(1)
	{
		USART_Err_St_t ret_st = USART_ReceiveByte(USART_NUM_2, &Rx_data);

		if(ret_st == USART_Rx_Ok)
		{
			printf("Rx data : %c\n" , Rx_data);
		}
		vTaskDelay(pdMS_TO_TICKS(2));
	}
}


void TASKS_Send_Data(void *pram)
{
	TickType_t last_wake = xTaskGetTickCount();
	char * Tx = "Ahmed ";

	while(1)
	{
		uint8_t i = 0 ;
		while(Tx[i] != '\0')
		{
			if(USART_SendByte(USART_NUM_2, (uint8_t)Tx[i]) == USART_Tx_Ok)
			{
				i++;
			}
			else
			{
				vTaskDelay(pdMS_TO_TICKS(1));
			}
			//			USART_SendByte(USART_NUM_2, (uint8_t)Tx[i]);
			//			i++;
		}
		vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(5));
	}
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


//void FP_Test(void *pram)
//{
//	TickType_t last_wake = xTaskGetTickCount();
//	while(1)
//	{
//		FP_SimpleTesT();
//		vTaskDelayUntil(&last_wake , pdMS_TO_TICKS(1));
//	}
//}

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
//
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




#define USART_MAX_NUM  6
/* Include your custom USART headers so the task knows about USART_Handler and USART_NUM_2 */
#include "USART_Cfg.h"
#include "USART.h"
/* Reference your global USART array instead of the default huart2 */
extern UART_HandleTypeDef USART_Handler[];

void FP_Simple_Search_Task(void *argument)
{
	/* 1. Hardcoded Command Frames */
	uint8_t cmd_get_img[12]   = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x01, 0x00, 0x05};
	uint8_t cmd_gen_char1[13] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x02, 0x01, 0x00, 0x08};

	/* Search Buffer 1, Start Page 0, Search 100 Pages (0x0064). Checksum = 0x0072 */
	uint8_t cmd_search[17] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x04, 0x01, 0x00, 0x00, 0x00, 0x64, 0x00, 0x72};

	uint8_t rx_buf[16];

	printf("--- Simple Search Task Started ---\n");



	while(1)
	{
		/* Add a 200ms delay so we don't spam the sensor too fast */
		vTaskDelay(pdMS_TO_TICKS(200));

		/* ==========================================
           STEP 1: GET IMAGE
           ========================================== */
		HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_get_img, 12, 100);

		HAL_StatusTypeDef rx_status = HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 5000) ;
		/* Expecting 12 bytes back. 100ms timeout. */
		if(rx_status  == HAL_OK)
		{
			/* Check the Confirmation Code at index 9 */
			if (rx_buf[9] != 0x00)
			{
				/* 0x02 = No Finger, 0x03 = Fail. Just continue polling. */
				continue;
			}
		}
		else
		{
			if (rx_status == HAL_TIMEOUT)
			{
				printf("Error: HAL_TIMEOUT\n");
			}
			else if (rx_status == HAL_BUSY)
			{
				printf("Error: HAL_BUSY\n");
			}
			else if (rx_status == HAL_ERROR)
			{
				/* Grab the exact hardware error code */
				uint32_t hal_err_code = USART_Handler[USART_NUM_2].ErrorCode;



				printf("Error: HAL_ERROR! Code: %lu -> ", hal_err_code);



				if (hal_err_code & HAL_UART_ERROR_PE) printf("Parity Error\n");
				if (hal_err_code & HAL_UART_ERROR_NE) printf("Noise Error\n");
				if (hal_err_code & HAL_UART_ERROR_FE) printf("Framing Error (Check Baud Rate!)\n");
				if (hal_err_code & HAL_UART_ERROR_ORE) printf("Overrun Error\n");



				/* Clear the error so the UART can try again */
				__HAL_UNLOCK(&USART_Handler[USART_NUM_2]);
				USART_Handler[USART_NUM_2].ErrorCode = HAL_UART_ERROR_NONE;
				USART_Handler[USART_NUM_2].RxState = HAL_UART_STATE_READY;
			}



			vTaskDelay(pdMS_TO_TICKS(1000));
			continue;
		}

		/* If we reached here, rx_buf[9] was 0x00! Finger is on the glass. */
		printf("Finger Detected! Processing...\n");

		/* ==========================================
		STEP 2: GENERATE CHARACTER FILE (To Buffer 1)
		========================================== */
		HAL_Delay(1000);
		HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_gen_char1, 13, 100);



		HAL_StatusTypeDef gen_status = HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 5000);



		if (gen_status == HAL_OK)
		{
			if (rx_buf[9] != 0x00)
			{
				printf("Error: Failed to generate character file. Code: 0x%02X\n", rx_buf[9]);



				if (rx_buf[9] == 0x06) printf("-> Reason: Fingerprint image is too messy/blurry.\n");
				if (rx_buf[9] == 0x15) printf("-> Reason: Image lacks validity.\n");



				vTaskDelay(pdMS_TO_TICKS(1000)); // Pause before retrying
				continue;
			}
		}
		else
		{
			printf("Error: HAL_UART_Receive timed out during Gen Char 1.\n");
			vTaskDelay(pdMS_TO_TICKS(1000));
			continue;
		}

		/* ==========================================
           STEP 3: SEARCH THE DATABASE
           ========================================== */
		HAL_Delay(1000);
		HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_search, 17, 100);

		/* Search response is 16 bytes long */
		if (HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 16, 5000) == HAL_OK)
		{
			if (rx_buf[9] == 0x00) // 0x00 = Match Found!
			{
				/* ID is stored in bytes 10 and 11 */
				uint16_t matched_id = (rx_buf[10] << 8) | rx_buf[11];

				/* Match Score is stored in bytes 12 and 13 */
				uint16_t match_score = (rx_buf[12] << 8) | rx_buf[13];

				printf(">>> ACCESS GRANTED! ID: %d (Score: %d)\n", matched_id, match_score);
			}
			else if (rx_buf[9] == 0x09) // 0x09 = Not Found in Database
			{
				printf(">>> ACCESS DENIED! Fingerprint not recognized.\n");
			}
			else
			{
				printf("Search error. Code: %02X\n", rx_buf[9]);
			}
		}

		/* Wait 1 second before allowing another scan so it doesn't print 10 times in a row */
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "USART.h"
#include "USART_Cfg.h"

extern UART_HandleTypeDef USART_Handler[];
uint8_t enroll_requested = 0; /* Bring in the global flag */

void FP_System_Task(void *argument)
{
	/* Hardcoded Command Frames */
	uint8_t cmd_get_img[12]   = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x01, 0x00, 0x05};
	uint8_t cmd_gen_char1[13] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x02, 0x01, 0x00, 0x08};
	uint8_t cmd_gen_char2[13] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x02, 0x02, 0x00, 0x09};
	uint8_t cmd_reg_model[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x05, 0x00, 0x09};

	/* UPDATED: Search Buffer 1, Start Page 0, Search 100 Pages (0x0064). Checksum = 0x0072 */
	uint8_t cmd_search[17]    = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x04, 0x01, 0x00, 0x00, 0x00, 0x64, 0x00, 0x72};

	/* We will auto-increment this every time we enroll someone new */
	static uint16_t next_enroll_id = 1;

	uint8_t rx_buf[16];

	HAL_UART_AbortReceive(&USART_Handler[USART_NUM_2]);
	printf("\n--- Unified Fingerprint System Started ---\n");

	while(1)
	{
		/* ------------------------------------------------------------------
           MODE 1: ENROLLMENT (Only runs if triggered by a button/admin)
           ------------------------------------------------------------------ */
		if (enroll_requested == 1)
		{
			printf("\n*** ENROLLMENT MODE ACTIVATED for ID #%d ***\n", next_enroll_id);
			printf("Please place finger on sensor...\n");

			/* Step 1: First Scan */
			while(1)
			{
				HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_get_img, 12, 100);
				if (HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 5000) == HAL_OK && rx_buf[9] == 0x00) break;
				vTaskDelay(pdMS_TO_TICKS(100));
			}

			HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_gen_char1, 13, 100);
			HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 5000); /* 1000ms Timeout */

			printf("Scan 1 OK. Remove finger...\n");

			/* Step 2: Wait for removal */
			while(1)
			{
				HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_get_img, 12, 100);
				if (HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 5000) == HAL_OK && rx_buf[9] == 0x02) break;
				vTaskDelay(pdMS_TO_TICKS(500));
			}

			printf("Place SAME finger again...\n");

			/* Step 3: Second Scan */
			while(1)
			{
				HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_get_img, 12, 100);
				if (HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 5000) == HAL_OK && rx_buf[9] == 0x00) break;
				vTaskDelay(pdMS_TO_TICKS(100));
			}
			vTaskDelay(pdMS_TO_TICKS(500));
			HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_gen_char2, 13, 100);
			HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 5000); /* 1000ms Timeout */
			vTaskDelay(pdMS_TO_TICKS(500));
			/* Step 4: Merge & Store */
			HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_reg_model, 12, 100);
			if (HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 5000) == HAL_OK && rx_buf[9] == 0x00)
			{
				/* 1. Calculate the 16-bit checksum properly */
				/* PID(1) + LenH(0) + LenL(6) + CMD(6) + Buffer(1) = 14 */
				uint16_t sum = 14 + (next_enroll_id >> 8) + (next_enroll_id & 0xFF);

				/* 2. Pack the 15-byte array exactly to the datasheet spec */
				uint8_t cmd_store[15] = {
						0xEF, 0x01,             /* Header */
						0xFF, 0xFF, 0xFF, 0xFF, /* Address */
						0x01,                   /* PID */
						0x00, 0x06,             /* Length (6 bytes follow) */
						0x06,                   /* Command: Store */
						0x01,                   /* Buffer: 1 */
						(uint8_t)(next_enroll_id >> 8),   /* ID High Byte */
						(uint8_t)(next_enroll_id & 0xFF), /* ID Low Byte */
						(uint8_t)(sum >> 8),              /* Checksum High Byte */
						(uint8_t)(sum & 0xFF)             /* Checksum Low Byte */
				};

				vTaskDelay(pdMS_TO_TICKS(1000));
				HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_store, 15, 1000);
				if (HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 5000) == HAL_OK)
				{
					if( rx_buf[9] == 0x00)
					{

						printf(">>> ENROLL SUCCESS! Saved as ID #%d\n", next_enroll_id);
						next_enroll_id++; /* Increment for the next person */
					}
					else
					{
						printf(">>> ENROLL FAILED! Saved as ID #%d\n", next_enroll_id);

					}
				}

			}
			else
			{
				printf("Enrollment Failed! Fingers did not match.\n");
			}

			/* Reset the flag to return to normal scanning */
			enroll_requested = 0;
			printf("\nReturning to Normal Search Mode...\n");
			vTaskDelay(pdMS_TO_TICKS(2000));
		}

		/* ------------------------------------------------------------------
           MODE 2: DEFAULT SEARCH (Runs constantly)
           ------------------------------------------------------------------ */
		else
		{
			HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_get_img, 12, 100);

			/* If a finger is detected... */
			if (HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 100) == HAL_OK && rx_buf[9] == 0x00)
			{
				/* Generate Char 1 */
				HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_gen_char1, 13, 100);
				HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 1000); /* 1000ms Timeout */

				/* Search Database */
				HAL_UART_Transmit(&USART_Handler[USART_NUM_2], cmd_search, 17, 100);

				/* UPDATED: Two-Part Read. Grab the first 12 bytes. */
				if (HAL_UART_Receive(&USART_Handler[USART_NUM_2], rx_buf, 12, 1000) == HAL_OK)
				{
					if (rx_buf[9] == 0x09)
					{
						printf(">>> ACCESS DENIED! Unknown Fingerprint.\n");
					}
					else if (rx_buf[9] == 0x00)
					{
						/* Match Found! Read the remaining 4 bytes */
						if (HAL_UART_Receive(&USART_Handler[USART_NUM_2], &rx_buf[12], 4, 100) == HAL_OK)
						{
							uint16_t matched_id = (rx_buf[10] << 8) | rx_buf[11];
							printf(">>> ACCESS GRANTED! Welcome User ID: %d\n", matched_id);
						}
					}
					else
					{
						printf("Search Error Code: 0x%02X\n", rx_buf[9]);
					}
				}

				/* Pause so it doesn't read the same finger 50 times in a row */
				vTaskDelay(pdMS_TO_TICKS(1000));
			}
		}

		/* System tick delay */
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}
