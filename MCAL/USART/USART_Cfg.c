/*
 * =========================================================================================
 *  File      : USART_Cfg.c
 *  Author    : Ahmed
 *  Created   : Nov 27, 2025
 *
 *  Description:
 *  ------------
 *  Static configuration tables for the USART driver.
 *
 *  This file provides two constant arrays (one entry per USART instance):
 *
 *   1) USART_Pin_Config[]:
 *      - Maps each logical USART index (USART_NUM_1 .. USART_NUM_6) to:
 *          * TX GPIO port + pin
 *          * RX GPIO port + pin
 *
 *   2) USART_Config[]:
 *      - Defines UART peripheral parameters per instance:
 *          * Baud rate
 *          * Stop bits
 *          * Parity
 *          * Word length
 *          * Oversampling
 *
 *  How the driver uses these tables:
 *  -------------------------------
 *  - USART_Init() reads USART_Pin_Config[] to configure GPIO pins in AF mode.
 *  - USART_Init() reads USART_Config[] to populate the HAL UART init structure.
 *
 *  IMPORTANT NOTES:
 *  ---------------
 *  - Array sizes MUST match USART_MAX_NUM.
 *  - Each logical USART index must have a valid entry.
 *  - If you only want to use fewer USARTs, still fill the remaining entries with
 *    safe defaults (or map them to unused pins) to avoid undefined data access.
 *
 *  Pin mapping reminder (example):
 *  -------------------------------
 *  USART1 can be mapped to different pins depending on STM32 package/pinout:
 *   - TX: PB6 or PA9 (common options on STM32F4)
 *   - RX: PB7 or PA10 / PA10 depending on AF mapping (verify in datasheet)
 *
 *  Always verify pin AF selection and availability for your exact STM32 variant
 *  and board layout (CubeMX/datasheet).
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

#include "USART_Prv.h"
#include "USART_Cfg.h"

/*
 * =========================================================================================
 *                               USART Pin Configuration Table
 * =========================================================================================
 *
 * Each entry corresponds to the logical USART number used by the driver:
 *
 *   Index 0 -> USART_NUM_1
 *   Index 1 -> USART_NUM_2
 *   Index 2 -> USART_NUM_3
 *   Index 3 -> USART_NUM_4
 *   Index 4 -> USART_NUM_5
 *   Index 5 -> USART_NUM_6
 *
 * The driver will configure:
 *   - TX pin as AF_PP (alternate function push-pull)
 *   - RX pin as AF_PP (same AF selection; pin differs)
 *
 * NOTE:
 *  - In your current table you only provided 2 entries (USART1 and USART2).
 *    But the array is declared with size USART_MAX_NUM (6).
 *    In C, the remaining entries will be zero-initialized, meaning:
 *      Tx_Port = NULL, Rx_Port = NULL, Tx_Pin = 0, Rx_Pin = 0
 *    That can be okay IF you never initialize those USARTs, but it’s safer to
 *    explicitly fill all entries to make the config self-documenting.
 */
const USART_Pin_Config_t USART_Pin_Config[USART_MAX_NUM] =
{
	/* ===================================== USART_1 =======================================
	 * Common STM32F4 pin option shown here:
	 *   TX = PB6, RX = PB7
	 * Verify against your exact MCU datasheet / board schematic.
	 */
	{ .Tx_Port = USART_PORT_B, .Tx_Pin = USART_PIN_6, .Rx_Port = USART_PORT_B, .Rx_Pin = USART_PIN_7 },

	/* ===================================== USART_2 =======================================
	 * Typical STM32F4 mapping:
	 *   TX = PA2, RX = PA3
	 */
	{ .Tx_Port = USART_PORT_A, .Tx_Pin = USART_PIN_2, .Rx_Port = USART_PORT_A, .Rx_Pin = USART_PIN_3 },

	/* ===================================== USART_3 =======================================
	 * TODO: Fill with your actual board mapping if used.
	 * Example placeholders (set to safe/unused until defined):
	 */
	{ .Tx_Port = USART_PORT_B, .Tx_Pin = USART_PIN_10, .Rx_Port = USART_PORT_B, .Rx_Pin = USART_PIN_11 },

	/* ===================================== UART_4 =======================================
	 * TODO: Fill with your actual board mapping if used.
	 */
	{ .Tx_Port = NULL, .Tx_Pin = 0, .Rx_Port = NULL, .Rx_Pin = 0 },

	/* ===================================== UART_5 =======================================
	 * TODO: Fill with your actual board mapping if used.
	 */
	{ .Tx_Port = NULL, .Tx_Pin = 0, .Rx_Port = NULL, .Rx_Pin = 0 },

	/* ===================================== USART_6 ======================================
	 * TODO: Fill with your actual board mapping if used.
	 */
	{ .Tx_Port = NULL, .Tx_Pin = 0, .Rx_Port = NULL, .Rx_Pin = 0 },
};

/*
 * =========================================================================================
 *                             USART Peripheral Configuration Table
 * =========================================================================================
 *
 * Each entry defines UART parameters for the corresponding logical USART index.
 *
 * NOTE about oversampling:
 *  - Oversampling by 8 can reduce sampling margin but allows higher baud rates in
 *    some clock configurations. Oversampling by 16 is more common/stable.
 *  - Use what you validated on your target clock tree and baud accuracy.
 */
const USART_Config_t USART_Config[USART_MAX_NUM] =
{
	/* ===================================== USART_1 ===================================== */
	{
		.BaudRate     = USART_BAUDRATE_9600,
		.stop_bit     = USART_STOPBIT_1_,
		.Parity       = USART_PARITY_NONE_,
		.WordLength   = USART_WORD_LEN_8_,
		.OverSampling = USART_OVERSAMPLING_8_
	},

	/* ===================================== USART_2 ===================================== */
	{
		.BaudRate     = USART_BAUDRATE_57600,
		.stop_bit     = USART_STOPBIT_1_,
		.Parity       = USART_PARITY_NONE_,
		.WordLength   = USART_WORD_LEN_8_,
		.OverSampling = USART_OVERSAMPLING_8_
	},

	/* ===================================== USART_3 ===================================== */
	{
		.BaudRate     = USART_BAUDRATE_9600,
		.stop_bit     = USART_STOPBIT_1_,
		.Parity       = USART_PARITY_NONE_,
		.WordLength   = USART_WORD_LEN_8_,
		.OverSampling = USART_OVERSAMPLING_8_
	},

	/* ===================================== UART_4 ====================================== */
	{
		.BaudRate     = USART_BAUDRATE_9600,
		.stop_bit     = USART_STOPBIT_1_,
		.Parity       = USART_PARITY_NONE_,
		.WordLength   = USART_WORD_LEN_8_,
		.OverSampling = USART_OVERSAMPLING_8_
	},

	/* ===================================== UART_5 ====================================== */
	{
		.BaudRate     = USART_BAUDRATE_9600,
		.stop_bit     = USART_STOPBIT_1_,
		.Parity       = USART_PARITY_NONE_,
		.WordLength   = USART_WORD_LEN_8_,
		.OverSampling = USART_OVERSAMPLING_8_
	},

	/* ===================================== USART_6 ===================================== */
	{
		.BaudRate     = USART_BAUDRATE_9600,
		.stop_bit     = USART_STOPBIT_1_,
		.Parity       = USART_PARITY_NONE_,
		.WordLength   = USART_WORD_LEN_8_,
		.OverSampling = USART_OVERSAMPLING_8_
	},
};
