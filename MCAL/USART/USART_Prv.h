/*
 * =========================================================================================
 *  File      : USART_Prv.h
 *  Author    : Ahmed
 *  Created   : Nov 27, 2025
 *
 *  Description:
 *  ------------
 *  Private (internal) definitions for the USART driver.
 *
 *  This header is NOT intended to be included by application code.
 *  It contains:
 *   - Driver internal constants (max instances, AF selections, NVIC priorities)
 *   - Private helper function prototypes (clock enable helpers)
 *
 *  Note:
 *  -----
 *  Keep this file minimal and stable. Anything that application code must use
 *  should go to USART.h instead.
 * =========================================================================================
 */

#ifndef USART_USART_PRV_H_
#define USART_USART_PRV_H_

#include "USART.h"     /* for USART_Num_t and public error types */
#include "stm32f4xx.h" /* for GPIO_TypeDef and AF definitions */

/* =========================================================================================
 *                              Driver Internal Configuration
 * =========================================================================================
 *
 * USART_MAX_NUM:
 *  - Total number of USART/UART instances supported by this driver.
 *  - Must match the size of driver arrays in USART.c and your config tables.
 */
#define USART_MAX_NUM  6

/*
 * Alternate Function selection macros:
 *
 * On STM32F4, USART1/2/3 typically use AF7.
 * UART4/5 and USART6 can use AF8 depending on the pin mapping.
 *
 * These macros are used in USART.c when selecting GPIO_InitStruct.Alternate.
 *
 * NOTE:
 *  - The specific AF macro names shown here (GPIO_AF7_USART2, GPIO_AF8_UART4)
 *    come from STM32 HAL headers. They map to numeric AF values (AF7/AF8).
 *  - If you change pin mapping or target a different STM32 family, verify AF
 *    assignments in the datasheet / reference manual / CubeMX.
 */
#define USART_GPIO_AF7  GPIO_AF7_USART2
#define USART_GPIO_AF8  GPIO_AF8_UART4

/*
 * NVIC priority configuration:
 *
 * Used by HAL_NVIC_SetPriority() inside USART_Init() when interrupts are enabled.
 *
 * IMPORTANT (FreeRTOS):
 *  - On Cortex-M, interrupt priorities interact with FreeRTOS critical sections.
 *  - Ensure these values are compatible with configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
 *    in FreeRTOSConfig.h, otherwise FromISR APIs may assert or behave incorrectly.
 */
#define USART_NVIC_GROUP_PRIORITY   4
#define USART_NVIC_SUB_PRIORITY     4

/* =========================================================================================
 *                                Private Helper Prototypes
 * =========================================================================================
 *
 * usart_gpio_clk_enable():
 *  - Enables the RCC clock for a given GPIO port (GPIOA..GPIOH).
 *
 * usart_clk_enable():
 *  - Enables the RCC clock for the selected USART/UART peripheral.
 */
void usart_gpio_clk_enable(GPIO_TypeDef *port);
void usart_clk_enable(USART_Num_t USART_Num);

#endif /* USART_USART_PRV_H_ */
