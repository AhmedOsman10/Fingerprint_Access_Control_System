/*
 * =========================================================================================
 *  File      : USART.c
 *  Author    : Ahmed
 *  Created   : Nov 27, 2025
 *
 *  Description:
 *  ------------
 *  STM32F4 USART/UART driver with optional FreeRTOS queue buffering.
 *
 *  This module supports two operation styles (compile-time selectable):
 *    1) Polling mode (USART_TX_INT / USART_RX_INT = DISABLE)
 *       - Application pushes TX bytes into a queue (or direct-send when possible)
 *       - Cyclic functions (USART_TxCyclic / USART_RxCyclic) poll HW flags (TXE / RXNE)
 *         and move data between HW registers and RTOS queues.
 *
 *    2) Interrupt mode (USART_TX_INT / USART_RX_INT = ENABLE)
 *       - RX uses HAL_UART_Receive_IT() and HAL_UART_RxCpltCallback()
 *       - TX uses HAL_UART_Transmit_IT() and HAL_UART_TxCpltCallback()
 *       - Queues are used as async buffers between tasks and ISR context
 *
 *  Notes:
 *  ------
 *  - Queues are created per USART instance using USART_MAX_BUFF length.
 *  - This file mixes HAL init/IT services with LL flag checks for polling loops.
 *  - For ISR usage with FreeRTOS, prefer passing pxHigherPriorityTaskWoken to
 *    xQueueSendFromISR/xQueueReceiveFromISR if you want immediate task switch.
 *
 * =========================================================================================
 */

#include <stdio.h>
#include <stdint.h>

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_rcc_ex.h"

#include "stm32f4xx_hal_usart.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_ll_usart.h"
#include "FreeRTOS.h"
#include "queue.h"

#include "USART.h"
#include "USART_Prv.h"
#include "USART_Cfg.h"

/* =========================================================================================
 *                                  Global Driver Objects
 * =========================================================================================
 *
 * USART_Handler:
 *  - HAL UART handle array, one handle per supported USART instance.
 *  - The .Instance pointer is set in USART_Init(), and used later in cyclic/ISR paths.
 *
 * USART_Base_Num:
 *  - Maps logical USART_Num_t (0..USART_MAX_NUM-1) to the actual STM32 peripheral base.
 *
 * USART_IRQ:
 *  - Maps logical USART to the proper IRQ number for NVIC configuration.
 *
 * IMPORTANT:
 *  - The size of these arrays must match USART_MAX_NUM, and USART_Num_t enum ordering
 *    must align with this mapping.
 */
UART_HandleTypeDef USART_Handler[USART_MAX_NUM] = {NULL};
USART_TypeDef *USART_Base_Num[USART_MAX_NUM] = {USART1 , USART2 , USART3 , UART4, UART5, USART6};
IRQn_Type USART_IRQ[USART_MAX_NUM] = {USART1_IRQn , USART2_IRQn , USART3_IRQn , UART4_IRQn , UART5_IRQn , USART6_IRQn} ;

/* =========================================================================================
 *                              FreeRTOS Queue Buffers (TX/RX)
 * =========================================================================================
 *
 * USART_Tx_Buffer / USART_Rx_Buffer:
 *  - Each USART instance owns a TX and RX queue.
 *  - Tasks send TX bytes to USART_Tx_Buffer[], and cyclic/ISR code drains to hardware.
 *  - Cyclic/ISR code pushes RX bytes into USART_Rx_Buffer[], and tasks read from it.
 *
 * usart_active_flag:
 *  - Used in TX interrupt mode to indicate that a TX "chain" is active.
 *  - When set to 0, the first queued byte triggers HAL_UART_Transmit_IT().
 *  - When queue becomes empty, callback clears the flag back to 0.
 */
QueueHandle_t USART_Tx_Buffer[USART_MAX_NUM];
QueueHandle_t USART_Rx_Buffer[USART_MAX_NUM];

volatile uint8_t usart_active_flag[USART_MAX_NUM] = {0};

/* Per-instance single-byte staging variables (shared between cyclic/ISR code). */
uint8_t USART_Rx_Byte[USART_MAX_NUM];
uint8_t USART_Tx_Byte[USART_MAX_NUM];

/* Tracks init state per USART instance (prevents using non-initialized peripheral). */
static USART_Err_St_t  USART_Init_St[USART_MAX_NUM] = {USART_Not_Init};

/* =========================================================================================
 *                                  USART_Init()
 * =========================================================================================
 *
 * General peripheral init pattern used here:
 *   1) Validate arguments
 *   2) Enable peripheral clock
 *   3) Enable GPIO clocks
 *   4) Configure GPIO pins for Alternate Function (TX/RX)
 *   5) Configure USART parameters via HAL handle
 *   6) Initialize USART with HAL_UART_Init()
 *   7) Create RTOS resources (TX/RX queues)
 *   8) Configure NVIC + enable interrupts (optional)
 *   9) Start RX interrupt reception (optional)
 *
 * Returns:
 *   - USART_InitSuccess on success
 *   - Error codes on invalid args, HAL init failure, queue creation failure, etc.
 */
USART_Err_St_t USART_Init(USART_Num_t USART_Num)
{
	USART_Err_St_t USART_Err_Ret = USART_InitSuccess;
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* 1) Validate argument: USART_Num must be within configured range. */
	if(USART_Num >= USART_MAX_NUM)
	{
		USART_Err_Ret = USART_Invalid_Arg;
	}
	else
	{
		/* 2) Enable USART peripheral clock. */
		usart_clk_enable(USART_Num);

		/* 3) Enable GPIO clocks for configured TX/RX ports. */
		usart_gpio_clk_enable(USART_Pin_Config[USART_Num].Tx_Port);
		usart_gpio_clk_enable(USART_Pin_Config[USART_Num].Rx_Port);

		/* 4) Configure TX pin as Alternate Function Push-Pull. */
		GPIO_InitStruct.Pin = USART_Pin_Config[USART_Num].Tx_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

		/* Select AF mapping: USART1/2/3 use AF7, UART4/5/USART6 often AF8 on STM32F4. */
		if((USART_Num == USART_NUM_1) || (USART_Num == USART_NUM_2) || (USART_Num == USART_NUM_3))
		{
			GPIO_InitStruct.Alternate = USART_GPIO_AF7;
		}
		else
		{
			GPIO_InitStruct.Alternate = USART_GPIO_AF8;
		}

		HAL_GPIO_Init(USART_Pin_Config[USART_Num].Tx_Port, &GPIO_InitStruct);

		/* Configure RX pin with the same AF parameters (Pin field changed only). */
		GPIO_InitStruct.Pin = USART_Pin_Config[USART_Num].Rx_Pin;
		HAL_GPIO_Init(USART_Pin_Config[USART_Num].Rx_Port, &GPIO_InitStruct);

		/* 5) Fill HAL handle init parameters from configuration tables. */
		USART_Handler[USART_Num].Instance          = USART_Base_Num[USART_Num];
		USART_Handler[USART_Num].Init.BaudRate     = USART_Config[USART_Num].BaudRate;
		USART_Handler[USART_Num].Init.WordLength   = USART_Config[USART_Num].WordLength;
		USART_Handler[USART_Num].Init.StopBits     = USART_Config[USART_Num].stop_bit;
		USART_Handler[USART_Num].Init.Parity       = USART_Config[USART_Num].Parity;
		USART_Handler[USART_Num].Init.Mode         = UART_MODE_TX_RX;
		USART_Handler[USART_Num].Init.HwFlowCtl    = UART_HWCONTROL_NONE;
		USART_Handler[USART_Num].Init.OverSampling = USART_Config[USART_Num].OverSampling;

		/* 6) Initialize hardware via HAL. */
		if(HAL_UART_Init(&USART_Handler[USART_Num]) != HAL_OK)
		{
			USART_Err_Ret = USART_InitFailed;
		}

		if( USART_Err_Ret == USART_InitSuccess)
		{
			/* 7) Create TX/RX queues (byte-sized elements). */
			USART_Tx_Buffer[USART_Num] =  xQueueCreate(USART_MAX_BUFF, sizeof(uint8_t));
			USART_Rx_Buffer[USART_Num] =  xQueueCreate(USART_MAX_BUFF, sizeof(uint8_t));

			if( USART_Tx_Buffer[USART_Num] == NULL ||  USART_Rx_Buffer[USART_Num] == NULL)
			{
				USART_Err_Ret = USART_CreateBuff_Failed;
			}

			/* 8) Configure NVIC only if at least one direction uses interrupts. */
#if  ((USART_RX_INT ==  ENABLE) || (USART_TX_INT ==  ENABLE))
			HAL_NVIC_SetPriority(USART_IRQ[USART_Num], USART_NVIC_GROUP_PRIORITY, USART_NVIC_SUB_PRIORITY);
			HAL_NVIC_EnableIRQ(USART_IRQ[USART_Num]);
#endif

			/* 9) Start RX interrupt reception (one byte at a time) if enabled. */
#if (USART_RX_INT ==  ENABLE)
			HAL_UART_Receive_IT(&USART_Handler[USART_Num] , &USART_Rx_Byte[USART_Num], 1);
#endif

			USART_Init_St[USART_Num] = USART_InitSuccess;
		}
	}
	return USART_Err_Ret;
}

/* =========================================================================================
 *                                  USART_RxCyclic()
 * =========================================================================================
 *
 * Polling-based receive "service routine" (used when RX interrupts are disabled).
 *
 * What it does:
 *  - Iterates over all configured USART instances
 *  - For each initialized instance, polls RXNE flag
 *  - While RXNE is set, reads DR (which clears RXNE) and pushes byte to RX queue
 *
 * Why this exists:
 *  - In polling mode, application tasks should NOT touch hardware registers directly.
 *    Instead they read from an RTOS queue; this function performs the HW draining.
 *
 * Call frequency:
 *  - Should be called periodically from a dedicated task or main loop.
 *  - The faster you call it, the less chance of RX overrun if incoming data is fast.
 */
void USART_RxCyclic(void)
{
	USART_TypeDef *USART_Instance = NULL ;

	for(uint8_t usart_num = 0 ; usart_num < USART_MAX_NUM ; usart_num++)
	{
		USART_Instance = USART_Handler[usart_num].Instance;

		if(USART_Instance != NULL)
		{
#if USART_RX_INT == DISABLE
			/* RXNE = 1 means there is unread data in the receive data register. */
			while(LL_USART_IsActiveFlag_RXNE(USART_Instance) != RESET)
			{
				/* Reading DR returns the received byte and clears RXNE. */
				USART_Rx_Byte[usart_num] = LL_USART_ReceiveData8(USART_Instance);

				/* Push received byte into the RTOS queue (non-blocking). */
				xQueueSend(USART_Rx_Buffer[usart_num] , &USART_Rx_Byte[usart_num] , 0);
			}
#endif
		}
	}
}

/* =========================================================================================
 *                                  USART_TxCyclic()
 * =========================================================================================
 *
 * Polling-based transmit "service routine" (used when TX interrupts are disabled).
 *
 * What it does:
 *  - Iterates over all USART instances
 *  - For each initialized instance:
 *      - If TXE is set (data register empty) AND TX queue has bytes waiting
 *      - Pop a byte from TX queue and write it to DR to start transmission
 *
 * TXE flag:
 *  - TXE = 1 means the transmit data register can accept a new byte.
 *  - Once you write DR, hardware clears TXE until it copies DR into the shift register.
 *
 * Note on pdPASS:
 *  - xQueueReceive() returns pdPASS when it successfully removed an item from the queue.
 *  - If it fails (empty queue), it returns errQUEUE_EMPTY (usually pdFALSE).
 *
 * NOTE (pdPASS vs pdTRUE):
 *  - In FreeRTOS, pdPASS is the standard success return for queue/semaphore operations.
 *  - pdTRUE is commonly used for boolean-style returns (timers, some APIs).
 *  - Many ports define both as 1, but you should compare against the documented macro
 *    used by that API (for queues: pdPASS).
 */
void USART_TxCyclic(void)
{
	USART_TypeDef *USART_Instance = NULL ;

	for(uint8_t usart_num = 0 ; usart_num < USART_MAX_NUM ; usart_num++)
	{
		USART_Instance = USART_Handler[usart_num].Instance;

		if(USART_Instance != NULL)
		{
#if USART_TX_INT == DISABLE
			/* TXE must be set and there must be something to send in the queue. */
			while((LL_USART_IsActiveFlag_TXE(USART_Instance) != RESET) &&
				  (uxQueueMessagesWaiting(USART_Tx_Buffer[usart_num]) > 0))
			{
				/* Pop one byte from queue (non-blocking). */
				if(xQueueReceive(USART_Tx_Buffer[usart_num], &USART_Tx_Byte[usart_num], 0) == pdPASS)
				{
					/* Write byte to DR -> starts hardware transmission. */
					LL_USART_TransmitData8(USART_Instance, USART_Tx_Byte[usart_num]);
				}
				else
				{
					/* Queue unexpectedly empty -> exit the loop. */
					break;
				}
			}
#endif
		}
	}
}

/* =========================================================================================
 *                                  USART_ReceiveByte()
 * =========================================================================================
 *
 * Application API to receive one byte from the RX queue.
 *
 * Behavior:
 *  - Non-blocking read from RX queue
 *  - If a byte is available -> returns USART_Rx_Ok and stores it in *Rx_data
 *  - If no data -> returns USART_Rx_NoData
 */

USART_Err_St_t USART_ReceiveByte(USART_Num_t USART_Num , uint8_t *Rx_data)
{
	USART_Err_St_t USART_Err_Ret =  USART_InitFailed;

	/* Validate arguments. */
	if(USART_Num >= USART_MAX_NUM ||  Rx_data == NULL)
	{
		USART_Err_Ret =  USART_Invalid_Arg;
	}

	/* Non-blocking queue read. */
	if(xQueueReceive(USART_Rx_Buffer[USART_Num], Rx_data, 0) == pdPASS)
	{
		USART_Err_Ret =  USART_Rx_Ok;
	}
	else
	{
		USART_Err_Ret =  USART_Rx_NoData;
	}

	return USART_Err_Ret;
}

/* Debug counters (optional): used to observe buffering/draining behavior while testing. */
uint8_t Buffering_Counter = 0;
uint8_t D_C = 0;

/* =========================================================================================
 *                                  USART_SendByte()
 * =========================================================================================
 *
 * Application API to send one byte.
 *
 * Polling TX mode:
 *  - First drains queue into HW as long as TXE is ready (reduces backlog)
 *  - If queue empty and TXE ready -> write directly to DR (fast path)
 *  - Otherwise -> push byte into TX queue (buffering path)
 *
 * Interrupt TX mode:
 *  - Push byte into TX queue inside a critical section
 *  - If TX not active, start the first TX using HAL_UART_Transmit_IT()
 *  - Further bytes are sent in HAL_UART_TxCpltCallback(), chaining until queue empty
 *
 * Return values:
 *  - USART_Tx_Ok: byte accepted (sent immediately or queued)
 *  - USART_Tx_Busy: queue full, byte rejected
 *  - USART_Not_Init / USART_Invalid_Arg: invalid usage
 */
USART_Err_St_t USART_SendByte(USART_Num_t USART_Num , uint8_t Tx_data)
{
	USART_Err_St_t USART_Err_Ret =  USART_Tx_Ok;
	USART_TypeDef *USART_Instance = NULL ;

	/* Validate arguments. */
	if(USART_Num >= USART_MAX_NUM )
	{
		USART_Err_Ret =  USART_Invalid_Arg;
	}
	else if(USART_Init_St[USART_Num] == USART_Not_Init)
	{
		USART_Err_Ret =  USART_Not_Init;
	}
	else
	{
#if USART_TX_INT == DISABLE

		USART_Instance = USART_Handler[USART_Num].Instance;

		/* 1) Drain queued bytes into HW while TXE is ready. */
		while( (uxQueueMessagesWaiting(USART_Tx_Buffer[USART_Num]) > 0) &&
			   (LL_USART_IsActiveFlag_TXE(USART_Instance) != RESET) )
		{
			if(xQueueReceive(USART_Tx_Buffer[USART_Num], &USART_Tx_Byte[USART_Num], 0) == pdPASS)
			{
				LL_USART_TransmitData8(USART_Instance, USART_Tx_Byte[USART_Num]);
				D_C++;

				/* Optional debug: track buffered bytes count. */
				if(Buffering_Counter > 0)
				{
					Buffering_Counter--;
				}
			}
		}

		/* 2) If nothing buffered and TXE is ready, send directly (no queue latency). */
		if((uxQueueMessagesWaiting(USART_Tx_Buffer[USART_Num]) == 0) &&
		   (LL_USART_IsActiveFlag_TXE(USART_Instance) != RESET))
		{
			LL_USART_TransmitData8(USART_Instance,Tx_data);
			D_C++;
		}
		else
		{
			/* 3) Otherwise buffer the byte in the TX queue. */
			USART_Tx_Byte[USART_Num] = Tx_data;

			if(xQueueSend(USART_Tx_Buffer[USART_Num], &USART_Tx_Byte[USART_Num], 0) != pdPASS)
			{
				/* Queue full -> cannot accept new byte. */
				UBaseType_t Buff_Curr_Count = uxQueueMessagesWaiting(USART_Tx_Buffer[USART_Num]);
				(void)Buff_Curr_Count; /* Keep local var for debugging (avoid unused warning). */

				USART_Err_Ret =  USART_Tx_Busy;
			}
			else
			{
				Buffering_Counter++;
			}
		}
#endif

#if USART_TX_INT == ENABLE
		/*
		 * Critical section protects:
		 *  - Queue operations from concurrent ISR access (Tx callback)
		 *  - usart_active_flag state from race conditions
		 *
		 * NOTE:
		 *  - FreeRTOS recommends taskENTER_CRITICAL() for short regions only.
		 *  - Alternative design: use a mutex for task-to-task, and FromISR APIs for ISR.
		 */
		taskENTER_CRITICAL();

		USART_Tx_Byte[USART_Num] = Tx_data;

		if(xQueueSend(USART_Tx_Buffer[USART_Num], &USART_Tx_Byte[USART_Num], 0) != pdPASS)
		{
			USART_Err_Ret =  USART_Tx_Busy;
		}
		else
		{
			/* If no active TX in progress, kick-start the interrupt chain. */
			if(usart_active_flag[USART_Num] == 0)
			{
				usart_active_flag[USART_Num] = 1;

				/* Pop first byte and start IT transmit of 1 byte. */
				xQueueReceive(USART_Tx_Buffer[USART_Num], &USART_Tx_Byte[USART_Num], 0);
				HAL_UART_Transmit_IT(&USART_Handler[USART_Num], &USART_Tx_Byte[USART_Num], 1);
			}
		}

		taskEXIT_CRITICAL();
#endif
	}
	return USART_Err_Ret;
}

/* =========================================================================================
 *                                  Clock Enable Helpers
 * =========================================================================================
 *
 * usart_clk_enable():
 *  - Enables APB clock for the selected USART/UART peripheral.
 *
 * usart_gpio_clk_enable():
 *  - Enables AHB clock for the GPIO port used by TX/RX pins.
 */
void usart_clk_enable(USART_Num_t USART_Num)
{
	switch(USART_Num)
	{
	case USART_NUM_1:
		__HAL_RCC_USART1_CLK_ENABLE();
		break;

	case USART_NUM_2:
		__HAL_RCC_USART2_CLK_ENABLE();
		break;

	case USART_NUM_3:
		__HAL_RCC_USART3_CLK_ENABLE();
		break;

	case USART_NUM_4:
		__HAL_RCC_UART4_CLK_ENABLE();
		break;

	case USART_NUM_5:
		__HAL_RCC_UART5_CLK_ENABLE();
		break;

	case USART_NUM_6:
		__HAL_RCC_USART6_CLK_ENABLE();
		break;

	default:
		/* Defensive: if USART_Num is invalid, do nothing. */
		break;
	}
}

void usart_gpio_clk_enable(GPIO_TypeDef *port)
{
	/*
	 * Why do we cast 'port' to (uint32_t) before switching?
	 *
	 * A switch–case statement in C only works with integer types (char, int, enum, etc.).
	 * It cannot compare pointer types directly.
	 *
	 * 'port' is a pointer to a GPIO_TypeDef structure (e.g., GPIOA, GPIOB…),
	 * which are defined as memory-mapped peripheral base addresses.
	 *
	 * By casting the pointer to (uint32_t), we convert the pointer address into its
	 * raw numerical value. This allows the switch statement to match the provided
	 * GPIO port against the known base addresses of GPIOA, GPIOB, etc.
	 */
	switch((uint32_t)port)
	{
	case (uint32_t)GPIOA:
		__HAL_RCC_GPIOA_CLK_ENABLE();
		break;

	case (uint32_t)GPIOB:
		__HAL_RCC_GPIOB_CLK_ENABLE();
		break;

	case (uint32_t)GPIOC:
		__HAL_RCC_GPIOC_CLK_ENABLE();
		break;

	case (uint32_t)GPIOD:
		__HAL_RCC_GPIOD_CLK_ENABLE();
		break;

	case (uint32_t)GPIOE:
		__HAL_RCC_GPIOE_CLK_ENABLE();
		break;

	case (uint32_t)GPIOF:
		__HAL_RCC_GPIOF_CLK_ENABLE();
		break;

	case (uint32_t)GPIOG:
		__HAL_RCC_GPIOG_CLK_ENABLE();
		break;

	case (uint32_t)GPIOH:
		__HAL_RCC_GPIOH_CLK_ENABLE();
		break;

	default:
		/* Unknown port pointer -> do nothing. */
		break;
	}
}

/* =========================================================================================
 *                           HAL Callbacks (Interrupt Mode)
 * =========================================================================================
 *
 * HAL_UART_TxCpltCallback():
 *  - Called by HAL when the previously requested IT transmit completes.
 *  - We use it to pop the next byte from the TX queue and start a new IT transfer.
 *  - If queue is empty, clear usart_active_flag -> TX idle.
 *
 * HAL_UART_RxCpltCallback():
 *  - Called when one byte has been received (Receive_IT length = 1).
 *  - Push the received byte into RX queue, then restart Receive_IT for the next byte.
 *
 * FreeRTOS note:
 *  - xQueueSendFromISR / xQueueReceiveFromISR can optionally request a context switch.
 *    For best responsiveness, use a BaseType_t xHigherPriorityTaskWoken and call
 *    portYIELD_FROM_ISR(xHigherPriorityTaskWoken).
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	USART_Num_t USRAT_Num ;

	/* Map HAL handle instance to driver logical USART number. */
	if(huart->Instance == USART1)      { USRAT_Num = USART_NUM_1; }
	else if(huart->Instance == USART2) { USRAT_Num = USART_NUM_2; }
	else if(huart->Instance == USART3) { USRAT_Num = USART_NUM_3; }
	else if(huart->Instance == UART4)  { USRAT_Num = USART_NUM_4; }
	else if(huart->Instance == UART5)  { USRAT_Num = USART_NUM_5; }
	else                               { USRAT_Num = USART_NUM_6; }

	/* If more bytes queued, continue transmitting next byte. */
	if(xQueueReceiveFromISR(USART_Tx_Buffer[USRAT_Num], &USART_Tx_Byte[USRAT_Num], NULL) == pdPASS)
	{
		HAL_UART_Transmit_IT(&USART_Handler[USRAT_Num], &USART_Tx_Byte[USRAT_Num], 1);
	}
	else
	{
		/* Queue empty -> mark TX chain inactive. */
		usart_active_flag[USRAT_Num] = 0;
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	USART_Num_t USRAT_Num ;
	/* Map HAL handle instance to driver logical USART number. */
	if(huart->Instance == USART1)      { USRAT_Num = USART_NUM_1; }
	else if(huart->Instance == USART2) { USRAT_Num = USART_NUM_2; }
	else if(huart->Instance == USART3) { USRAT_Num = USART_NUM_3; }
	else if(huart->Instance == UART4)  { USRAT_Num = USART_NUM_4; }
	else if(huart->Instance == UART5)  { USRAT_Num = USART_NUM_5; }
	else                               { USRAT_Num = USART_NUM_6; }

	/* Push received byte into RX queue (ISR context). */
	 if(xQueueSendFromISR(USART_Rx_Buffer[USRAT_Num], &USART_Rx_Byte[USRAT_Num], NULL) != pdPASS)
	 {
	   // nothing
	 }

	/* Restart single-byte reception for continuous stream capture. */
	HAL_UART_Receive_IT(&USART_Handler[USRAT_Num], &USART_Rx_Byte[USRAT_Num], 1);
}

/* =========================================================================================
 *                                   IRQ Handlers
 * =========================================================================================
 *
 * Each IRQ handler forwards the interrupt to HAL_UART_IRQHandler() with the proper handle.
 * HAL then calls the relevant callbacks (TxCplt/RxCplt/Error, etc.).
 *
 * NOTE:
 *  - These handlers must match the vector table names for STM32F4 startup code.
 */
void USART1_IRQHandler(void)
{
	HAL_UART_IRQHandler(&USART_Handler[USART_NUM_1]);
}

void USART2_IRQHandler(void)
{
	HAL_UART_IRQHandler(&USART_Handler[USART_NUM_2]);
}

void USART3_IRQHandler(void)
{
	HAL_UART_IRQHandler(&USART_Handler[USART_NUM_3]);
}

void UART4_IRQHandler(void)
{
	HAL_UART_IRQHandler(&USART_Handler[USART_NUM_4]);
}

void UART5_IRQHandler(void)
{
	HAL_UART_IRQHandler(&USART_Handler[USART_NUM_5]);
}

void USART6_IRQHandler(void)
{
	HAL_UART_IRQHandler(&USART_Handler[USART_NUM_6]);
}
