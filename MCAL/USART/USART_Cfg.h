/*
 * =========================================================================================
 *  File      : USART_Cfg.h
 *  Author    : Ahmed
 *  Created   : Nov 27, 2025
 *
 *  Description:
 *  ------------
 *  Driver configuration header for the STM32F4 USART/UART module.
 *
 *  This file contains:
 *   - GPIO pin/port helper macros for readable configuration tables
 *   - Common USART parameter macros (baud, word length, stop bits, parity, oversampling)
 *   - Queue buffer length (USART_MAX_BUFF)
 *   - Configuration structures:
 *        * USART_Pin_Config_t : TX/RX pin mapping per instance
 *        * USART_Config_t     : UART peripheral parameters per instance
 *   - External configuration tables (defined in USART_Cfg.c)
 *   - Compile-time selection of polling vs interrupt operation for TX/RX
 *
 *  How to use:
 *  -----------
 *   1) Edit USART_Pin_Config[] to map each USART_NUM_x to the correct pins/ports.
 *   2) Edit USART_Config[] to set baud rate, parity, stop bits, etc. per instance.
 *   3) Select interrupt/polling mode using USART_RX_INT and USART_TX_INT.
 *
 *  Notes:
 *  ------
 *  - Macros here map directly to STM32 HAL constants (UART_WORDLENGTH_8B, etc.).
 *  - USART_MAX_BUFF controls the FreeRTOS queue depth for TX and RX.
 *  - If enabling interrupts with FreeRTOS, confirm NVIC priority settings are
 *    compatible with FreeRTOSConfig.h (max syscall interrupt priority).
 * =========================================================================================
 */

#ifndef USART_USART_CFG_H_
#define USART_USART_CFG_H_

#include "stm32f4xx.h"      /* GPIO_TypeDef, GPIO_PIN_x, GPIOx base pointers */
#include "stm32f4xx_hal_uart.h" /* UART_* macros used below */

/* =========================================================================================
 *                                GPIO Pin Helper Macros
 * =========================================================================================
 *
 * These aliases allow configuration tables to be written in a consistent style:
 *   .Tx_Pin = USART_PIN_2, .Tx_Port = USART_PORT_A, etc.
 *
 * They map 1:1 to HAL GPIO pin defines (GPIO_PIN_x).
 */
#define USART_PIN_0   GPIO_PIN_0
#define USART_PIN_1   GPIO_PIN_1
#define USART_PIN_2   GPIO_PIN_2
#define USART_PIN_3   GPIO_PIN_3
#define USART_PIN_4   GPIO_PIN_4
#define USART_PIN_5   GPIO_PIN_5
#define USART_PIN_6   GPIO_PIN_6
#define USART_PIN_7   GPIO_PIN_7
#define USART_PIN_8   GPIO_PIN_8
#define USART_PIN_9   GPIO_PIN_9
#define USART_PIN_10  GPIO_PIN_10
#define USART_PIN_11  GPIO_PIN_11
#define USART_PIN_12  GPIO_PIN_12
#define USART_PIN_13  GPIO_PIN_13
#define USART_PIN_14  GPIO_PIN_14
#define USART_PIN_15  GPIO_PIN_15

/* =========================================================================================
 *                                GPIO Port Helper Macros
 * =========================================================================================
 *
 * These aliases map to STM32 GPIO port base addresses (GPIOA..GPIOH).
 */
#define USART_PORT_A	GPIOA
#define USART_PORT_B	GPIOB
#define USART_PORT_C	GPIOC
#define USART_PORT_D	GPIOD
#define USART_PORT_E	GPIOE
#define USART_PORT_F	GPIOF
#define USART_PORT_G	GPIOG
#define USART_PORT_H	GPIOH

/* =========================================================================================
 *                                Common UART Parameter Macros
 * =========================================================================================
 *
 * Baud rate presets (plain integers used directly by HAL init).
 */
#define USART_BAUDRATE_9600    9600U
#define USART_BAUDRATE_19200   19200U
#define USART_BAUDRATE_57600   57600U
#define USART_BAUDRATE_115200  115200U

/*
 * Word length macros map to HAL UART defines.
 * NOTE:
 *  - 9-bit mode with parity may reduce usable data bits (HAL behavior depends on config).
 */
#define USART_WORD_LEN_8_    UART_WORDLENGTH_8B
#define USART_WORD_LEN_9_    UART_WORDLENGTH_9B

/* Stop bits macros map to HAL UART defines. */
#define USART_STOPBIT_1_   UART_STOPBITS_1
#define USART_STOPBIT_2_   UART_STOPBITS_2

/* Parity macros map to HAL UART defines. */
#define USART_PARITY_NONE_ UART_PARITY_NONE
#define USART_PARITY_EVEN_ UART_PARITY_EVEN
#define USART_PARITY_ODD_  UART_PARITY_ODD

/* Oversampling macros map to HAL UART defines. */
#define USART_OVERSAMPLING_8_  UART_OVERSAMPLING_8
#define USART_OVERSAMPLING_16_ UART_OVERSAMPLING_16

/* =========================================================================================
 *                              FreeRTOS Queue Buffer Length
 * =========================================================================================
 *
 * USART_MAX_BUFF:
 *  - Queue depth for BOTH TX and RX queues per USART instance.
 *  - Larger values reduce the chance of overflow at the cost of heap usage.
 *  - Each instance allocates:
 *      TX queue: USART_MAX_BUFF bytes
 *      RX queue: USART_MAX_BUFF bytes
 *    plus queue control overhead.
 */
#define USART_MAX_BUFF  200U

/* =========================================================================================
 *                              Configuration Structures
 * =========================================================================================
 */

/**
 * @brief TX/RX pin mapping for one USART instance.
 *
 * Tx_Port / Rx_Port:
 *  - GPIO port base address (GPIOA..GPIOH)
 *
 * Tx_Pin / Rx_Pin:
 *  - GPIO pin mask (GPIO_PIN_x)
 */
typedef struct USART_Pin_Config_s
{
	GPIO_TypeDef *Tx_Port;
	GPIO_TypeDef *Rx_Port;
	uint16_t       Tx_Pin;
	uint16_t       Rx_Pin;
} USART_Pin_Config_t;

/**
 * @brief USART peripheral configuration for one instance.
 *
 * stop_bit:
 *  - HAL stop bit selection (UART_STOPBITS_1/2)
 *
 * Parity:
 *  - HAL parity selection (UART_PARITY_NONE/EVEN/ODD)
 *
 * WordLength:
 *  - HAL word length selection (UART_WORDLENGTH_8B/9B)
 *
 * BaudRate:
 *  - integer baud rate (e.g. 115200)
 *
 * OverSampling:
 *  - HAL oversampling selection (UART_OVERSAMPLING_8/16)
 */
typedef struct USART_Config_s
{
	uint8_t  stop_bit;
	uint8_t  Parity;
	uint8_t  WordLength;
	uint32_t BaudRate;
	uint32_t OverSampling;
} USART_Config_t;

/* =========================================================================================
 *                             External Configuration Tables
 * =========================================================================================
 *
 * These arrays are defined in USART_Cfg.c and must have size USART_MAX_NUM.
 * Each index corresponds to the logical USART number (USART_NUM_1..USART_NUM_6).
 */
extern const USART_Pin_Config_t USART_Pin_Config[USART_MAX_NUM];
extern const USART_Config_t     USART_Config[USART_MAX_NUM];

/* =========================================================================================
 *                         Compile-Time Feature / Mode Selection
 * =========================================================================================
 *
 * ENABLE / DISABLE:
 *  - Simple macros used for selecting RX/TX interrupt modes.
 *  - (They are intentionally kept as 1/0 for use in preprocessor conditions.)
 */
#define ENABLE  1
#define DISABLE 0

/*
 * USART_RX_INT / USART_TX_INT:
 *  - Controls whether the driver uses interrupts (ENABLE) or polling cyclic
 *    functions (DISABLE) for RX and TX respectively.
 *
 * Examples:
 *  - RX_INT = ENABLE,  TX_INT = DISABLE:
 *      RX via interrupts, TX via polling + USART_TxCyclic().
 *
 *  - RX_INT = DISABLE, TX_INT = DISABLE:
 *      Fully polling. You must call USART_RxCyclic() and USART_TxCyclic().
 *
 *  - RX_INT = ENABLE,  TX_INT = ENABLE:
 *      Fully interrupt-driven. Cyclic functions are not required.
 */
#define USART_RX_INT ENABLE
#define USART_TX_INT  ENABLE

#endif /* USART_USART_CFG_H_ */
