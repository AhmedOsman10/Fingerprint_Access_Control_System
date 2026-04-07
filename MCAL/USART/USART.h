/*
 * =========================================================================================
 *  File      : USART.h
 *  Author    : Ahmed
 *  Created   : Nov 27, 2025
 *
 *  Description:
 *  ------------
 *  Public API for the STM32F4 USART/UART driver.
 *
 *  This header exposes:
 *   - Driver error/status codes (USART_Err_St_t)
 *   - Logical USART numbering (USART_Num_t + USART_NUM_x macros)
 *   - Initialization + byte-based send/receive APIs
 *   - Optional polling “cyclic service” functions for TX/RX in polling mode
 *
 *  Usage summary:
 *  --------------
 *   1) Call USART_Init(USART_NUM_x) once per needed instance.
 *   2) For sending:
 *        - USART_SendByte() queues/sends one byte (non-blocking).
 *   3) For receiving:
 *        - USART_ReceiveByte() reads one byte from RX queue (non-blocking).
 *   4) If interrupts are DISABLED for TX/RX, call:
 *        - USART_TxCyclic() periodically to drain TX queue into hardware
 *        - USART_RxCyclic() periodically to move HW RX bytes into RX queue
 *
 *  Notes:
 *  ------
 *  - USART_Num_t is an index (0..USART_MAX_NUM-1). It must match config tables.
 *  - This interface is byte-oriented by design (simple + portable).
 *  - Consider adding future APIs for strings/buffers/timeouts if needed.
 * =========================================================================================
 */

#ifndef USART_USART_H_
#define USART_USART_H_

#include <stdint.h>   /* for uint8_t */

/* =========================================================================================
 *                               Driver Return / Error States
 * =========================================================================================
 *
 * USART_Not_Init:
 *  - Returned when API is called on an instance that wasn't initialized.
 *
 * USART_InitFailed:
 *  - HAL_UART_Init() failed, or init sequence could not complete.
 *
 * USART_InitSuccess:
 *  - Init completed successfully.
 *
 * USART_Invalid_Arg:
 *  - USART number out of range, or NULL pointer passed to API.
 *
 * USART_CreateBuff_Failed:
 *  - FreeRTOS queue creation failed (likely heap/config issue).
 *
 * USART_Rx_Ok / USART_Rx_NoData:
 *  - RX queue had data / no data available (non-blocking read).
 *
 * USART_Tx_Ok / USART_Tx_Busy:
 *  - TX byte accepted (sent or queued) / rejected due to full queue.
 */
typedef enum USART_Err_St_e
{
	USART_Not_Init = 0,
	USART_InitFailed,
	USART_InitSuccess,
	USART_Invalid_Arg,
	USART_CreateBuff_Failed,
	USART_Rx_Ok,
	USART_Rx_NoData,
	USART_Tx_Busy,
	USART_Tx_Ok,
} USART_Err_St_t;

/* =========================================================================================
 *                               Logical USART Instance IDs
 * =========================================================================================
 *
 * USART_Num_t:
 *  - Logical index used by this driver to select a USART instance.
 *  - Must align with:
 *      - USART_Handler[] array
 *      - USART_Base_Num[] mapping
 *      - USART_IRQ[] mapping
 *      - USART_Config[] / USART_Pin_Config[] tables (in USART_Cfg.*)
 *
 * The macros provide clear readable naming at call sites.
 */
typedef uint8_t USART_Num_t;

#define USART_NUM_1    ((USART_Num_t)0)
#define USART_NUM_2    ((USART_Num_t)1)
#define USART_NUM_3    ((USART_Num_t)2)
#define USART_NUM_4    ((USART_Num_t)3)
#define USART_NUM_5    ((USART_Num_t)4)
#define USART_NUM_6    ((USART_Num_t)5)

/* =========================================================================================
 *                                  Public API Prototypes
 * =========================================================================================
 */

/**
 * @brief  Initialize the selected USART instance and its RTOS queues.
 * @param  USART_Num  Logical USART instance ID (USART_NUM_1 .. USART_NUM_6)
 * @return USART_Err_St_t status code
 */
USART_Err_St_t USART_Init(USART_Num_t USART_Num);

/**
 * @brief  Non-blocking receive of one byte from the RX queue.
 * @param  USART_Num  Logical USART instance ID
 * @param  Rx_data    Pointer to store received byte
 * @return USART_Rx_Ok if a byte was read, USART_Rx_NoData if queue empty,
 *         or error codes for invalid args/not init.
 */
USART_Err_St_t USART_ReceiveByte(USART_Num_t USART_Num , uint8_t *Rx_data);

/**
 * @brief  Non-blocking send of one byte.
 *         In polling mode it may send directly if possible, otherwise queue it.
 *         In interrupt mode it queues and starts IT transmission if idle.
 * @param  USART_Num  Logical USART instance ID
 * @param  Tx_data    Byte to send
 * @return USART_Tx_Ok if accepted, USART_Tx_Busy if TX queue full,
 *         or error codes for invalid args/not init.
 */
USART_Err_St_t USART_SendByte(USART_Num_t USART_Num , uint8_t Tx_data);

/**
 * @brief  Polling RX service routine (only used when RX interrupts are disabled).
 *         Moves bytes from hardware DR into RX queue.
 * @note   Call periodically from main loop or a dedicated service task.
 */
void USART_RxCyclic(void);

/**
 * @brief  Polling TX service routine (only used when TX interrupts are disabled).
 *         Moves bytes from TX queue into hardware DR when TXE is ready.
 * @note   Call periodically from main loop or a dedicated service task.
 */
void USART_TxCyclic(void);

#endif /* USART_USART_H_ */
